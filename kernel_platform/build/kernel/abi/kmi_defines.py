#!/usr/bin/env python3
#
# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""kmi_defines extract #define compile time constants from a Linux build.

The kmi_defines tool is used to examine the output of a Linux build
and extract from it C #define statements that define compile time
constant expressions for the purpose of tracking them as part of the
KMI (Kernel Module Interface) so that changes to their values can be
prevented so as to ensure a constant KMI for kernel modules for the
AOSP GKI Linux kernel project.

This code is python3 only, it does not require any from __future__
imports.  This is a standalone program, it is not meant to be used as
a module by other programs.

This program runs under the multiprocessing module.  Work done within
a multiprocessing.Pool does not perform error logging or affects any
state other than the value that it computes and returns via the function
mapped through the pool's map() function.  The reason that no external
state is affected (for example error loggiing) is to avoid to have to
even think about what concurrent updates would cause to shuch a facility.
"""

#   TODO(pantin): per Matthias review feedback: "drop the .py from the
#   filename after(!) the review has completed. As last action. Until
#   then we can have the syntax highlighting here in Gerrit."

import argparse
import collections
import logging
import multiprocessing
import os
import pathlib
import re
import subprocess
import sys
from typing import List, Optional, Tuple
from typing import Set  # pytype needs this, pylint: disable=unused-import

INDENT = 4  # number of spaces to indent for each depth level
COMPILER = "clang"  # TODO(pantin): should be determined at run-time

#   Dependency that is hidden by the transformation of the .o.d file into
#   the .o.cmd file as part of the Linux build environment.  This header is
#   purposely removed and replaced by fictitious set of empty header files
#   that were never part of the actual compilation of the .o files.  Those
#   fictitious empty files are generated under the build environment output
#   directory in this subdirectory:
#       include/config
#
#   This is the actual header file that was part of the compilation of every
#   .o file, the HIDDEN_DEP are added to the dependencies of every .o file.
#
#   It is important that this file be added because it is unknowable whether
#   the #defines in it were depended upon by a module to alter its behaviour
#   at compile time.  For example to pass some flags or not pass some flags
#   to a function.

HIDDEN_DEP = "include/generated/autoconf.h"


class StopError(Exception):
    """Exception raised to stop work when an unexpected error occurs."""


def dump(this) -> None:
    """Dump the data in this.

    This is for debugging purposes, it does not handle every type, only
    the types used by the underlying code are handled.  This will not be
    part of the final code, or if it is, it will be significantly enhanced
    or replaced by some other introspection mechanism to serialize data.
    """
    def dump_this(this, name: str, depth: int) -> None:
        """Dump the data in this."""
        if name:
            name += " = "
        if isinstance(this, str):
            indent = " " * (depth * INDENT)
            print(indent + name + this)
        elif isinstance(this, bool):
            indent = " " * (depth * INDENT)
            print(indent + name + str(this))
        elif isinstance(this, List):
            dump_list(this, name, depth)
        elif isinstance(this, Set):
            dump_set(this, name, depth)
        else:
            dump_object(this, name, depth)

    def dump_list(lst: List[str], name: str, depth: int) -> None:
        """Dump the data in lst."""
        indent = " " * (depth * INDENT)
        print(indent + name + "{")
        index = 0
        for entry in lst:
            dump_this(entry, f"[{index}]", depth + 1)
            index += 1
        print(indent + "}")

    def dump_set(aset: Set[str], name: str, depth: int) -> None:
        """Dump the data in aset."""
        lst = list(aset)
        lst.sort()
        dump_list(lst, name, depth)

    def dump_object(this, name: str, depth: int) -> None:
        """Dump the data in this."""
        indent = " " * (depth * INDENT)
        print(indent + name +
              re.sub(r"(^<class '__main__\.|'>$)", "", str(type(this))) + " {")
        for key, val in this.__dict__.items():
            dump_this(val, key, depth + 1)
        print(indent + "}")

    dump_this(this, "", 0)


def readfile(name: str) -> str:
    """Open a file and return its contents in a string as its value."""
    try:
        with open(name) as file:
            return file.read()
    except OSError as os_error:
        raise StopError("readfile() failed for: " + name + "\n"
                        "original OSError: " + str(os_error.args))


def file_must_exist(file: str) -> None:
    """If file is invalid print raise a StopError."""
    if not os.path.exists(file):
        raise StopError("file does not exist: " + file)
    if not os.path.isfile(file):
        raise StopError("file is not a regular file: " + file)


def makefile_depends_get_dependencies(depends: str) -> List[str]:
    """Return list with the dependencies of a makefile target.

    Split the makefile depends specification, the name of the dependent is
    followed by ":" its dependencies follow the ":".  There could be spaces
    around the ":".  Line continuation characters, i.e. "\" are consumed by
    the regular expression that splits the specification.

    This results in a list with the dependent first, and its dependencies
    in the remainder of the list, return everything in the list other than
    the first element.
    """
    return re.split(r"[:\s\\]+", re.sub(r"[\s\\]*\Z", "", depends))[1:]


def makefile_assignment_split(assignment: str) -> Tuple[str, str]:
    """Split left:=right into a tuple with the left and right parts.

    Spaces around the := are also removed.
    """
    result = re.split(r"\s*:=\s*", assignment, maxsplit=1)
    if len(result) != 2:
        raise StopError(
            "expected: 'left<optional_spaces>:=<optional_spaces>right' in: " +
            assignment)
    return result[0], result[1]  # left, right


def get_src_ccline_deps(obj: str) -> Optional[Tuple[str, str, List[str]]]:
    """Get the C source file, its cc_line, and non C source dependencies.

    If the tool used to produce the object is not the compiler, or if the
    source file is not a C source file None is returned.

    Otherwise it returns a triplet with the C source file name, its cc_line,
    the remaining dependencies.
    """
    o_cmd = os.path.join(os.path.dirname(obj),
                         "." + os.path.basename(obj) + ".cmd")

    contents = readfile(o_cmd)
    contents = re.sub(r"\$\(wildcard[^)]*\)", " ", contents)
    contents = re.sub(r"[ \t]*\\\n[ \t]*", " ", contents)
    lines = lines_to_list(contents)

    cc_line = None
    deps = None
    source = None
    for line in lines:
        if line.startswith("cmd_"):
            cc_line = line
        elif line.startswith("deps_"):
            deps = line
        elif line.startswith("source_"):
            source = line

    if cc_line is None:
        raise StopError("missing cmd_* variable in: " + o_cmd)
    _, cc_line = makefile_assignment_split(cc_line)
    if cc_line.split(maxsplit=1)[0] != COMPILER:
        #   The object file was made by strip, symbol renames, etc.
        #   i.e. it was not the result of running the compiler, thus
        #   it can not contribute to #define compile time constants.
        return None

    if source is None:
        raise StopError("missing source_* variable in: " + o_cmd)
    _, source = makefile_assignment_split(source)
    source = source.strip()
    if not source.endswith(".c"):
        return None

    if deps is None:
        raise StopError("missing deps_* variable in: " + o_cmd)
    _, deps = makefile_assignment_split(deps)
    dependendencies = deps.split()
    dependendencies.append(HIDDEN_DEP)

    return source, cc_line, dependendencies


def lines_to_list(lines: str) -> List[str]:
    """Split a string into a list of non-empty lines."""
    return [line for line in lines.strip().splitlines() if line]


def lines_get_first_line(lines: str) -> str:
    """Return the first non-empty line in lines."""
    return lines.strip().splitlines()[0]


def shell_line_to_o_files_list(line: str) -> List[str]:
    """Return a list of .o files in the files list."""
    return [entry for entry in line.split() if entry.endswith(".o")]


def run(args: List[str],
        raise_on_failure: bool = True) -> subprocess.CompletedProcess:
    """Run the program specified in args[0] with the arguments in args[1:]."""
    try:
        #   This argument does not always work for subprocess.run() below:
        #       check=False
        #   neither that nor:
        #       check=True
        #   prevents an exception from being raised if the program that
        #   will be executed is not found

        completion = subprocess.run(args, capture_output=True, text=True)
        if completion.returncode != 0 and raise_on_failure:
            raise StopError("execution failed for: " + " ".join(args))
        return completion
    except OSError as os_error:
        raise StopError("failure executing: " + " ".join(args) + "\n"
                        "original OSError: " + str(os_error.args))


class KernelModule:
    """A kernel module, i.e. a *.ko file."""
    def __init__(self, kofile: str) -> None:
        """Construct a KernelModule object."""
        #   An example argument is used below, assuming kofile is:
        #       possibly/empty/dirs/modname.ko
        #
        #   Meant to refer to this module, shown here relative to the top of
        #   the build directory:
        #       drivers/usb/gadget/udc/modname.ko
        #   the values assigned to the members are shown in the comments below.

        self._file = os.path.realpath(kofile)  # /abs/dirs/modname.ko
        self._base = os.path.basename(self._file)  # modname.ko
        self._directory = os.path.dirname(self._file)  # /abs/dirs
        self._cmd_file = os.path.join(self._directory,
                                      "." + self._base + ".cmd")
        self._cmd_text = readfile(self._cmd_file)

        #   Some builds append a '; true' to the .modname.ko.cmd, remove it

        self._cmd_text = re.sub(r";\s*true\s*$", "", self._cmd_text)

        #   The modules .modname.ko.cmd file contains a makefile snippet,
        #   for example:
        #       cmd_drivers/usb/gadget/udc/dummy_hcd.ko := ld.lld -r ...
        #
        #   Split the string prior to the spaces followed by ":=", and get
        #   the first element of the resulting list.  If the string was not
        #   split (because it did not contain a ":=" then the input string
        #   is returned, by the re.sub() below, as the only element of the list.

        left, _ = makefile_assignment_split(self._cmd_text)
        self._rel_file = re.sub(r"^cmd_", "", left)
        if self._rel_file == left:
            raise StopError("expected: 'cmd_' at start of content of: " +
                            self._cmd_file)

        base = os.path.basename(self._rel_file)
        if base != self._base:
            raise StopError("module name mismatch: " + base + " vs " +
                            self._base)

        self._rel_dir = os.path.dirname(self._rel_file)

        #   The final step in the build of kernel modules is based on two .o
        #   files, one with the module name followed by .o and another followed
        #   by .mod.o
        #
        #   The following test verifies that assumption, in case a module is
        #   built differently in the future.
        #
        #   Even when there are multiple source files, the .o files that result
        #   from compiling them are all linked into a single .o file through an
        #   intermediate link step, that .o files is named:
        #       os.path.join(self._rel_dir, kofile_name + ".o")

        kofile_name, _ = os.path.splitext(self._base)
        objs = shell_line_to_o_files_list(self._cmd_text)
        objs.sort()
        expected = [  # sorted, i.e.: .mod.o < .o
            os.path.join(self._rel_dir, kofile_name + ".mod.o"),
            os.path.join(self._rel_dir, kofile_name + ".o")
        ]
        if objs != expected:
            raise StopError("unexpected .o files in: " + self._cmd_file)

    def get_build_dir(self) -> str:
        """Return the top level build directory.

        I.e. the directory where the output of the Linux build is stored.

        Note that this, like pretty much all the code, can raise an exception,
        by construction, if an exception is raised while an object is being
        constructed, or after it is constructed, the object will not be used
        thereafter (at least not any object explicitly created by this
        program).  Many other places, for example the ones that call readfile()
        can raise exceptions, the code is located where it belongs.

        In this specific case, the computation of index, and the derived
        invariant that it be >= 0, is predicated by the condition checked
        below, if the exception is not raised, then index is >= 0.
        """
        if not self._file.endswith(self._rel_file):
            raise StopError("could not find: " + self._rel_file +
                            " at end of: " + self._file)
        index = len(self._file) - len(self._rel_file)
        if index > 0 and self._file[index - 1] == os.sep:
            index -= 1
        build_dir = self._file[0:index]
        return build_dir

    def get_object_files(self, build_dir: str) -> List[str]:
        """Return a list object files that used to link the kernel module.

        The ocmd_file is the file with extension ".o.cmd" (see below).
        If the ocmd_file has a more than one line in it, its because the
        module is made of a single source file and the ocmd_file has the
        compilation rule and dependencies to build it.  If it has a single
        line single line it is because it builds the .o file by linking
        multiple .o files.
        """

        kofile_name, _ = os.path.splitext(self._base)
        ocmd_file = os.path.join(build_dir, self._rel_dir,
                                 "." + kofile_name + ".o.cmd")
        ocmd_content = readfile(ocmd_file)

        olines = lines_to_list(ocmd_content)
        if len(olines) > 1:  # module made from a single .o file
            return [os.path.join(build_dir, self._rel_dir, kofile_name + ".o")]

        #   Multiple .o files in the module

        _, ldline = makefile_assignment_split(olines[0])
        return [
            os.path.realpath(os.path.join(build_dir, obj))
            for obj in shell_line_to_o_files_list(ldline)
        ]


class Kernel:
    """The Linux kernel component itself, i.e. vmlinux.o."""
    def __init__(self, kernel: str) -> None:
        """Construct a Kernel object."""
        self._kernel = os.path.realpath(kernel)
        self._build_dir = os.path.dirname(self._kernel)
        libs = os.path.join(self._build_dir, "vmlinux.libs")
        objs = os.path.join(self._build_dir, "vmlinux.objs")
        file_must_exist(libs)
        file_must_exist(objs)
        contents = readfile(libs)
        archives_and_objects = contents.split()
        contents = readfile(objs)
        archives_and_objects += contents.split()
        self._archives_and_objects = [(os.path.join(self._build_dir, file)
                                       if not os.path.isabs(file) else file)
                                      for file in archives_and_objects]

    def get_build_dir(self) -> str:
        """Return the top level build directory.

        I.e. the directory where the output of the Linux build is stored.
        """
        return self._build_dir

    def get_object_files(self, build_dir: str) -> List[str]:
        """Return a list object files that where used to link the kernel."""
        olist = []
        for file in self._archives_and_objects:
            if file.endswith(".o"):
                if not os.path.isabs(file):
                    file = os.path.join(build_dir, file)
                olist.append(os.path.realpath(file))
                continue

            if not file.endswith(".a"):
                raise StopError("unknown file type: " + file)

            completion = run(["ar", "t", file])
            objs = lines_to_list(completion.stdout)

            for obj in objs:
                if not os.path.isabs(obj):
                    obj = os.path.join(build_dir, obj)
                olist.append(os.path.realpath(obj))

        return olist


class Target:  # pylint: disable=too-few-public-methods
    """Target of build and the information used to build it."""

    #   The compiler invocation has this form:
    #       clang -Wp,-MD,file.o.d  ... -c -o file.o file.c
    #   these constants reflect that knowledge in the code, e.g.:
    #   - the "-Wp,_MD,file.o.d" is at WP_MD_FLAG_INDEX
    #   - the "-c" is at index C_FLAG_INDEX
    #   - the "-o" is at index O_FLAG_INDEX
    #   - the "file.o" is at index OBJ_INDEX
    #   - the "file.c" is at index SRC_INDEX
    #
    #   There must be at least MIN_CC_LIST_LEN options in that command line.
    #   This knowledge is verified at run time in __init__(), see comments
    #   there.

    MIN_CC_LIST_LEN = 6
    WP_MD_FLAG_INDEX = 1
    C_FLAG_INDEX = -4
    O_FLAG_INDEX = -3
    OBJ_INDEX = -2
    SRC_INDEX = -1

    def __init__(self, obj: str, src: str, cc_line: str,
                 deps: List[str]) -> None:
        self._obj = obj
        self._src = src
        self._deps = deps

        #   The cc_line, eventually slightly modified, will be used to run
        #   the compiler in various ways.  The cc_line could be fed through
        #   the shell to deal with the single-quotes in the cc_line that are
        #   there to quote the double-quotes meant to be part of a C string
        #   literal.  Specifically, this occurs in to pass KBUILD_MODNAME and
        #   KBUILD_BASENAME, for example:
        #       -DKBUILD_MODNAME='"aes_ce_cipher"'
        #       -DKBUILD_BASENAME='"aes_cipher_glue"'
        #
        #   Causing an extra execve(2) of the shell, just to deal with a few
        #   quotes is wasteful, so instead, here the quotes, in this specific
        #   case are removed.  This can be done, easiest just by removing the
        #   single quotes with:
        #       cc_cmd = re.sub(r"'", "", cc_line)
        #
        #   But this could mess up other quote usage in the future, for example
        #   using double quotes or backslash to quote a single quote meant to
        #   actually be seen by the compiler.
        #
        #   As an alternative, and for this to be more robust, the specific
        #   cases that are known, i.e. the two -D shown above, are dealt with
        #   individually and if there are any single or double quotes, or
        #   backslashes the underlying work is stopped.
        #
        #   Note that the cc_line comes from the .foo.o.cmd file which is a
        #   makefile snippet, so the actual syntax there is also subject to
        #   whatever other things make would want to do with them.  Instead
        #   of doing the absolutely correct thing, which would actually be
        #   to run this through make to have make run then through the shell
        #   this program already has knowledge about these .cmd files and how
        #   they are formed.  This compromise, or coupling of knowledge, is a
        #   source of fragility, but not expected to cause much trouble in the
        #   future as the Linux build evolves.

        cc_cmd = re.sub(
            r"""-D(KBUILD_BASENAME|KBUILD_MODNAME)='("[a-zA-Z0-9_.:]*")'""",
            r"-D\1=\2", cc_line)
        cc_list = cc_cmd.split()

        #   TODO(pantin): the handling of -D... arguments above is done better
        #   in a later commit by using shlex.split().  Please ignore for now.
        #   TODO(pantin): possibly use ArgumentParser to make this more robust.

        #   The compiler invocation has this form:
        #       clang -Wp,-MD,file.o.d  ... -c -o file.o file.c
        #
        #   The following checks are here to ensure that if this assumption is
        #   broken, failures occur.  The indexes *_INDEX are hardcoded, they
        #   could in principle be determined at run time, the -o argument could
        #   be in a future update to the Linux build could changed to be a
        #   single argument with the object file name (as in: -ofile.o) which
        #   could also be detected in code at a later time.

        if (len(cc_list) < Target.MIN_CC_LIST_LEN
                or not cc_list[Target.WP_MD_FLAG_INDEX].startswith("-Wp,-MD,")
                or cc_list[Target.C_FLAG_INDEX] != "-c"
                or cc_list[Target.O_FLAG_INDEX] != "-o"):
            raise StopError("unexpected or missing arguments for: " + obj +
                            " cc_line: " + cc_line)

        #   Instead of blindly normalizing the source and object arguments,
        #   they are only normalized if that allows the expected invariants
        #   to be verified, otherwise they are left undisturbed.  Note that
        #   os.path.normpath() does not turn relative paths into absolute
        #   paths, it just removes up-down walks (e.g. a/b/../c -> a/c).

        def verify_file(file: str, index: int, kind: str, cc_list: List[str],
                        target_file: str) -> None:
            file_in_cc_list = cc_list[index]
            if not file.endswith(file_in_cc_list):
                file_normalized = os.path.normpath(file_in_cc_list)
                if not file.endswith(file_normalized):
                    raise StopError(f"unexpected {kind} argument for: "
                                    f"{target_file} value was: "
                                    f"{file_in_cc_list}")
                cc_list[index] = file_normalized

        verify_file(obj, Target.OBJ_INDEX, "object", cc_list, obj)
        verify_file(src, Target.SRC_INDEX, "source", cc_list, obj)

        self._cc_list = cc_list


class KernelComponentBase:  # pylint: disable=too-few-public-methods
    """Base class for KernelComponentCreationError and KernelComponent.

    There is not much purpose for this class other than to satisfy the strong
    typing checks of pytype, with looser typing, this could be removed but at
    the risk of invoking member functions at run-time on objects that do not
    provide them.  Having this class makes the code more reliable.
    """
    def get_error(self) -> Optional[str]:  # pylint: disable=no-self-use
        """Return None for the error, means there was no error."""
        return None

    def get_deps_set(self) -> Set[str]:  # pylint: disable=no-self-use
        """Return the set of dependencies for the kernel component."""
        return set()

    def is_kernel(self) -> bool:  # pylint: disable=no-self-use
        """Is this the kernel?"""
        return False


class KernelComponentCreationError(KernelComponentBase):  # pylint: disable=too-few-public-methods
    """A KernelComponent creation error.

    When a KernelComponent creation fails, or the creation of its subordinate
    Kernel or KernelModule creation fails, a KernelComponentCreationError
    object is created to store the information relevant to the failure.
    """
    def __init__(self, filename: str, error: str) -> None:
        """Construct a KernelComponentCreationError object."""
        self._error = error
        self._filename = filename

    def get_error(self) -> Optional[str]:
        """Return the error."""
        return self._filename + ": " + self._error


class KernelComponent(KernelComponentBase):
    """A kernel component, either vmlinux.o or a *.ko file.

    Inspect a Linux kernel module (a *.ko file) or the Linux kernel to
    determine what was used to build it: object filess, source files, header
    files, and other information that is produced as a by-product of its build.
    """
    def __init__(self, filename: str) -> None:
        """Construct a KernelComponent object."""
        if filename.endswith("vmlinux.o"):
            self._kernel = True
            self._kind = Kernel(filename)
        else:
            self._kernel = False
            self._kind = KernelModule(filename)
        self._build_dir = self._kind.get_build_dir()
        self._source_dir = self._get_source_dir()
        self._files_o = self._kind.get_object_files(self._build_dir)
        self._files_o.sort()

        #   using a set because there is no unique flag to list.sort()
        deps_set = set()

        self._targets = []
        for obj in self._files_o:
            file_must_exist(obj)
            result = get_src_ccline_deps(obj)
            if result is None:
                continue
            src, cc_line, dependendencies = result

            file_must_exist(src)
            depends = []
            for dep in dependendencies:
                if not os.path.isabs(dep):
                    dep = os.path.join(self._build_dir, dep)
                dep = os.path.realpath(dep)
                depends.append(dep)
                deps_set.add(dep)

            if not os.path.isabs(src):
                src = os.path.join(self._build_dir, src)
            src = os.path.realpath(src)
            self._targets.append(Target(obj, src, cc_line, depends))

        for dep in [dep for dep in list(deps_set) if not dep.endswith(".h")]:
            deps_set.remove(dep)
        self._deps_set = deps_set

    def _get_source_dir(self) -> str:
        """Return the top level Linux kernel source directory."""
        source = os.path.join(self._build_dir, "source")
        if not os.path.islink(source):
            raise StopError("could not find source symlink: " + source)

        if not os.path.isdir(source):
            raise StopError("source symlink not a directory: " + source)

        source_dir = os.path.realpath(source)
        if not os.path.isdir(source_dir):
            raise StopError("source directory not a directory: " + source_dir)

        return source_dir

    def get_deps_set(self) -> Set[str]:
        """Return the set of dependencies for the kernel component."""
        return self._deps_set

    def is_kernel(self) -> bool:
        """Is this the kernel?"""
        return self._kernel


def kernel_component_factory(filename: str) -> KernelComponentBase:
    """Make an InfoKmod or an InfoKernel object for file and return it."""
    try:
        return KernelComponent(filename)
    except StopError as stop_error:
        return KernelComponentCreationError(filename,
                                            " ".join([*stop_error.args]))


class KernelComponentProcess(multiprocessing.Process):
    """Process to make the KernelComponent concurrently."""
    def __init__(self) -> None:
        multiprocessing.Process.__init__(self)
        self._queue = multiprocessing.Queue()
        self.start()

    def run(self) -> None:
        """Create and save the KernelComponent."""
        self._queue.put(kernel_component_factory("vmlinux.o"))

    def get_component(self) -> KernelComponentBase:
        """Return the kernel component."""
        kernel_component = self._queue.get()
        self.join()  # must be after queue.get() otherwise it deadlocks
        return kernel_component


def work_on_all_components(options) -> List[KernelComponentBase]:
    """Return a list of KernelComponentBase objects."""
    files = [str(ko) for ko in pathlib.Path().rglob("*.ko")]
    if options.sequential:
        return [
            kernel_component_factory(file) for file in ["vmlinux.o"] + files
        ]

    #  There is significantly more work to be done for the vmlinux.o than
    #  the *.ko kernel modules.  A dedicated process is started to do the
    #  work for vmlinux.o as soon as possible instead of leaving it to the
    #  vagaries of multiprocessing.Pool() and how it would spreads the work.
    #  This significantly reduces the elapsed time for this work.

    kernel_component_process = KernelComponentProcess()

    chunk_size = 128
    processes = max(1, len(files) // (chunk_size * 3))
    processes = min(processes, os.cpu_count())
    with multiprocessing.Pool(processes) as pool:
        components = pool.map(kernel_component_factory, files, chunk_size)

    kernel_component = kernel_component_process.get_component()

    return [kernel_component] + components


def work_on_whole_build(options) -> int:
    """Work on the whole build to extract the #define constants."""
    if not os.path.isfile("vmlinux.o"):
        logging.error("file not found: vmlinux.o")
        return 1
    components = work_on_all_components(options)
    failed = False
    header_count = collections.defaultdict(int)
    for comp in components:
        error = comp.get_error()
        if error:
            logging.error(error)
            failed = True
            continue
        deps_set = comp.get_deps_set()
        for header in deps_set:
            header_count[header] += 1
    if failed:
        return 1
    if options.dump:
        dump(components)
    if options.dump and options.includes:
        print()
    if options.includes:
        for header, count in header_count.items():
            if count >= 2:
                print(header)
    return 0


def main() -> int:
    """Extract #define compile time constants from a Linux build."""
    def existing_file(file):
        if not os.path.isfile(file):
            raise argparse.ArgumentTypeError(
                "{0} is not a valid file".format(file))
        return file

    parser = argparse.ArgumentParser()
    parser.add_argument("-d",
                        "--dump",
                        action="store_true",
                        help="dump internal state")
    parser.add_argument("-s",
                        "--sequential",
                        action="store_true",
                        help="execute without concurrency")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-i",
                       "--includes",
                       action="store_true",
                       help="show relevant include files")
    group.add_argument("-c",
                       "--component",
                       type=existing_file,
                       help="show information for a component")
    options = parser.parse_args()

    if not options.component:
        return work_on_whole_build(options)

    comp = kernel_component_factory(options.component)

    error = comp.get_error()
    if error:
        logging.error(error)
        return 1
    if options.dump:
        dump([comp])
    return 0


if __name__ == "__main__":
    sys.exit(main())
