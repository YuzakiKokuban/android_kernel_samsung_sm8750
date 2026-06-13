# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Generate a DDK module Makefile
"""

import absl.flags.argparse_flags
import argparse
import json
import logging
import os
import pathlib
import shlex
import sys
import textwrap
from typing import Optional, TextIO, Any

_SOURCE_SUFFIXES = (
    ".c",
    ".rs",
    ".S",
)


class DieException(SystemExit):
    def __init__(self, *args, **kwargs):
        super().__init__(1)
        self.args = args
        self.kwargs = kwargs

    @property
    def msg(self):
        return self.args[0] % tuple(self.args[1:])

    @staticmethod
    def handle(die_exception: Optional["DieException"], msg: Optional[str]):
        if msg:
            if die_exception is None:
                logging.error(
                    f"Expect build failure %s, but there's no failure", msg)
                sys.exit(1)
            if die_exception.msg != msg:
                logging.error(*die_exception.args, **die_exception.kwargs)
                logging.error(
                    f"Expect build failure %s, but got a different failure", msg)
                sys.exit(1)
            return

        if die_exception is not None:
            logging.error(*die_exception.args, **die_exception.kwargs)
            sys.exit(1)


def die(*args, **kwargs):
    raise DieException(*args, **kwargs)


def _get_license_str():
  return textwrap.dedent("""\
    # SPDX-License-Identifier: GPL-2.0

  """)

def _gen_makefile(
        module_symvers_list: list[pathlib.Path],
        output_makefile: pathlib.Path,
):
    content = _get_license_str()

    for module_symvers in module_symvers_list:
        content += textwrap.dedent(f"""\
            # Include symbol: {module_symvers}
            EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/{module_symvers}
            """)

    content += textwrap.dedent("""\
        modules modules_install clean:
        \t$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(@)
        """)

    os.makedirs(output_makefile.parent, exist_ok=True)
    with open(output_makefile, "w") as out_file:
        out_file.write(content)


def _merge_directories(output_makefiles: pathlib.Path, submodule_makefile_dir: pathlib.Path):
    """Merges the content of submodule_makefile_dir into output_makefiles.

    File of the same relative path are concatenated.
    """

    if not submodule_makefile_dir.is_dir():
        die("Can't find directory %s", submodule_makefile_dir)

    for root, dirs, files in os.walk(submodule_makefile_dir):
        for file in files:
            submodule_file = pathlib.Path(root) / file
            file_rel = submodule_file.relative_to(submodule_makefile_dir)
            dst_path = output_makefiles / file_rel
            dst_path.parent.mkdir(parents=True, exist_ok=True)
            with open(dst_path, "a") as dst, \
                    open(submodule_file, "r") as src:
                # Comments are not allowed in .cflags files
                if dst_path.suffix != ".cflags":
                    dst.write(f"# {submodule_file}\n")
                dst.write(src.read())
                dst.write("\n")


def gen_ddk_makefile(
        output_makefiles: pathlib.Path,
        module_symvers_list: list[pathlib.Path],
        package: pathlib.Path,
        produce_top_level_makefile: Optional[bool],
        submodule_makefiles: list[pathlib.Path],
        kernel_module_out: Optional[pathlib.Path],
        **kwargs
):
    if produce_top_level_makefile:
        _gen_makefile(
            module_symvers_list=module_symvers_list,
            output_makefile=output_makefiles / "Makefile",
        )

    if kernel_module_out:
        _gen_ddk_makefile_for_module(
            output_makefiles=output_makefiles,
            package=package,
            kernel_module_out=kernel_module_out,
            **kwargs
        )

    for submodule_makefile_dir in submodule_makefiles:
        _merge_directories(output_makefiles, submodule_makefile_dir)


def _gen_ddk_makefile_for_module(
        output_makefiles: pathlib.Path,
        package: pathlib.Path,
        kernel_module_out: pathlib.Path,
        kernel_module_srcs_json: TextIO,
        include_dirs: list[pathlib.Path],
        linux_include_dirs: list[pathlib.Path],
        local_defines: list[str],
        copt_file: Optional[TextIO],
        **unused_kwargs
):
    kernel_module_srcs_json_content = json.load(kernel_module_srcs_json)
    # List of JSON objects (dictionaries) with keys like "file", "config", "value", etc.
    rel_srcs = []
    for kernel_module_srcs_json_item in kernel_module_srcs_json_content:
        rel_item = dict(kernel_module_srcs_json_item)
        rel_item["files"] = [pathlib.Path(src).relative_to(package)
                             for src in rel_item["files"]
                             if pathlib.Path(src).is_relative_to(package)]
        rel_srcs.append(rel_item)

    if kernel_module_out.suffix != ".ko":
        die("Invalid output: %s; must end with .ko", kernel_module_out)

    _check_srcs_valid(rel_srcs, kernel_module_out)

    kbuild = output_makefiles / kernel_module_out.parent / "Kbuild"
    os.makedirs(kbuild.parent, exist_ok=True)

    # rel to this package
    out_cflags_subpath = kernel_module_out.with_suffix(".cflags")

    # Output cflags file path
    out_cflags_path = output_makefiles / out_cflags_subpath

    copts = json.load(copt_file) if copt_file else None

    with open(kbuild, "w") as out_file, open(out_cflags_path, "w") as out_cflags:
        out_file.write(_get_license_str())
        out_file.write(textwrap.dedent(f"""\
            # Build {package / kernel_module_out}
            obj-m += {kernel_module_out.with_suffix('.o').name}
            """))
        out_file.write("\n")

        for src_item in rel_srcs:
            config = src_item.get("config")
            value = src_item.get("value")
            obj_suffix = "y"

            if config is not None:
                if value == True:
                    # The special value True means y or m.
                    obj_suffix = f"$({config})"
                else:
                    conditional = f"ifeq ($({config}),{value})"
                    out_file.write(f"{conditional}\n")

            for src in src_item["files"]:
                _handle_src(
                    src=src,
                    out_file=out_file,
                    kernel_module_out=kernel_module_out,
                    package=package,
                    obj_suffix=obj_suffix,
                )

            if config is not None and value != True:
                out_file.write(textwrap.dedent(f"""\
                    endif # {conditional}
                """))

        out_file.write(f"\n# Common flags for {kernel_module_out.with_suffix('.o').name}\n")
        _handle_linux_includes(out_file, linux_include_dirs)
        # At this time of writing (2022-11-01), this is the order how cc_library
        # constructs arguments to the compiler.
        _handle_defines(out_cflags, local_defines)
        _handle_includes(out_cflags, include_dirs)
        _handle_copts(out_cflags, copts)

        for src_item in rel_srcs:
            config = src_item.get("config")
            value = src_item.get("value")

            if config is not None and value != True:
                conditional = f"ifeq ($({config}),{value})"
                out_file.write(f"{conditional}\n")

            for src in src_item["files"]:

                out = src.with_suffix(".o").relative_to(
                    kernel_module_out.parent)

                # kernel_module() copies makefiles and .cflags files to
                # $(ROOT_DIR)/<package> (aka $ROOT_DIR/<ext_mod>) and fix up
                # .cflags files there before building.
                out_file.write(textwrap.dedent(f"""\
                    CFLAGS_{out} += @$(ROOT_DIR)/{package / out_cflags_subpath}
                    """))

            if config is not None and value != True:
                out_file.write(f"endif # {conditional}\n\n")

    top_kbuild = output_makefiles / "Kbuild"
    if top_kbuild != kbuild:
        os.makedirs(output_makefiles, exist_ok=True)
        with open(top_kbuild, "w") as out_file:
            out_file.write(_get_license_str())
            out_file.write(textwrap.dedent(f"""\
                # Build {package / kernel_module_out}
                obj-y += {kernel_module_out.parent}/
                """))


def _check_srcs_valid(rel_srcs: list[dict[str, Any]],
                      kernel_module_out: pathlib.Path):
    """Checks that the list of srcs is valid.

    Args:
        rel_srcs: Like content in kernel_module_srcs_json, but only includes files
          relative to the current package.
        kernel_module_out: The `out` attribute.
    """
    # List of paths of source files (minus headers)
    rel_srcs_flat: list[pathlib.Path] = []
    for rel_item in rel_srcs:
        files = rel_item["files"]
        rel_srcs_flat.extend(
            file for file in files if file.suffix in _SOURCE_SUFFIXES)

    source_files_with_name_of_kernel_module = \
        [src for src in rel_srcs_flat if src.with_suffix(
            ".ko") == kernel_module_out]

    if source_files_with_name_of_kernel_module and len(rel_srcs_flat) > 1:
        die("Source files %s are not allowed to build %s when multiple source files exist. "
            "Please change the name of the output file.",
            [str(e) for e in source_files_with_name_of_kernel_module],
            kernel_module_out)


def _handle_src(
        src: pathlib.Path,
        out_file: TextIO,
        kernel_module_out: pathlib.Path,
        package: pathlib.Path,
        obj_suffix: str,
):
    # Ignore non-exported headers specified in srcs
    if src.suffix in (".h",):
        return
    if src.suffix not in _SOURCE_SUFFIXES:
        die("Invalid source %s", src)
    if not src.is_relative_to(kernel_module_out.parent):
        die("%s is not a valid source because it is not under %s",
            src, kernel_module_out.parent)

    out = src.with_suffix(".o").relative_to(kernel_module_out.parent)
    # Ignore self (don't omit obj-foo += foo.o)
    if src.with_suffix(".ko") == kernel_module_out:
        out_file.write(textwrap.dedent(f"""\
                        # The module {kernel_module_out} has a source file {src}
                    """))
    else:
        out_file.write(textwrap.dedent(f"""\
                        # Source: {package / src}
                        {kernel_module_out.with_suffix('').name}-{obj_suffix} += {out}
                    """))


def _handle_linux_includes(out_file: TextIO,
                           linux_include_dirs: list[pathlib.Path]):
    if not linux_include_dirs:
        return
    out_file.write("\n")
    out_file.write(textwrap.dedent("""\
        LINUXINCLUDE := \\
    """))
    for linux_include_dir in linux_include_dirs:
        out_file.write(f"  -I$(ROOT_DIR)/{linux_include_dir} \\")
        out_file.write("\n")
    out_file.write("  $(LINUXINCLUDE)")
    out_file.write("\n\n")


def _handle_defines(out_cflags: TextIO,
                    local_defines: list[str]):
    if not local_defines:
        return
    for local_define in local_defines:
        out_cflags.write(textwrap.dedent(f"""\
            {shlex.quote(f"-D{local_define}")}
            """))


def _handle_includes(out_cflags: TextIO,
                     include_dirs: list[pathlib.Path]):
    for include_dir in include_dirs:
        out_cflags.write(textwrap.dedent(f"""\
            -I$(ROOT_DIR)/{shlex.quote(str(include_dir))}
            """))


def _handle_copts(out_cflags: TextIO,
                  copts: Optional[list[dict[str, str | bool]]]):
    if not copts:
        return

    for d in copts:
        expanded: str = d["expanded"]
        is_path: bool = d["is_path"]

        if is_path:
            out_cflags.write(textwrap.dedent(f"""\
                $(ROOT_DIR)/{shlex.quote(expanded)}
                """))
        else:
            out_cflags.write(textwrap.dedent(f"""\
                {shlex.quote(expanded)}
                """))


if __name__ == "__main__":
    # argparse_flags.ArgumentParser only accepts --flagfile if there
    # are some DEFINE'd flags
    # https://github.com/abseil/abseil-py/issues/199
    absl.flags.DEFINE_string("flagfile_hack_do_not_use", "", "")

    logging.basicConfig(level=logging.INFO,
                        format="%(levelname)s: %(message)s")

    parser = absl.flags.argparse_flags.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=pathlib.Path)
    parser.add_argument("--kernel-module-out", type=pathlib.Path)
    parser.add_argument("--kernel-module-srcs-json",
                        type=argparse.FileType("r"), required=True)
    parser.add_argument("--output-makefiles", type=pathlib.Path)
    parser.add_argument("--linux-include-dirs",
                        type=pathlib.Path, nargs="*", default=[])
    parser.add_argument("--include-dirs", type=pathlib.Path,
                        nargs="*", default=[])
    parser.add_argument("--module-symvers-list",
                        type=pathlib.Path, nargs="*", default=[])
    parser.add_argument("--local-defines", nargs="*", default=[])
    parser.add_argument("--copt-file", type=argparse.FileType("r"))
    parser.add_argument("--produce-top-level-makefile", action="store_true")
    parser.add_argument("--submodule-makefiles",
                        type=pathlib.Path, nargs="*", default=[])
    parser.add_argument("--internal-target-fail-message", default=None)

    args = parser.parse_args()

    die_exception = None
    try:
        gen_ddk_makefile(**vars(args))
    except DieException as exc:
        die_exception = exc
    finally:
        DieException.handle(die_exception, args.internal_target_fail_message)
