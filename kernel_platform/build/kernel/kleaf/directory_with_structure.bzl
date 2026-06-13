# Copyright (C) 2022 The Android Open Source Project
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

"""
When a directory created dy ctx.actions.declare_directory is referred to
in a sandbox, if it is empty, or a subdirectory of it is empty, the empty
directory won't be created in the sandbox.
These functions resolve the problem by also recording the directory structure
in a text file.
"""

def _make(ctx, filename):
    """The replacement of [ctx.actions.declare_directory](https://bazel.build/rules/lib/actions#declare_directory) that also preserves empty directories.

    Return a struct with the following fields:
        - `directory`: A [File](https://bazel.build/rules/lib/File) object from
          [ctx.actions.declare_directory](https://bazel.build/rules/lib/actions#declare_directory).
        - `structure_file`: A [File](https://bazel.build/rules/lib/File) object that will the
          directory structure.

    Args:
        ctx: ctx
        filename: See [ctx.actions.declare_directory](https://bazel.build/rules/lib/actions#declare_directory).
    """
    directory = ctx.actions.declare_directory(filename)
    structure_file = ctx.actions.declare_file(filename + ".structure.txt")
    return struct(directory = directory, structure_file = structure_file)

def _record(directory_with_structure):
    """Return a command that records the directory structure to the `structure_file`.

    It is expected that the shell has properly set up [hermetic tools](hermetic_tools.md#hermetic_tools).

    Args:
        directory_with_structure: struct returned by [`directory_with_structure.declare`](#directory_with_structuredeclare).
    """
    return """
        mkdir -p {structure_file_dir}
        : > {structure_file}
        (
            real_structure_file=$(readlink -e {structure_file})
            cd {directory}
            find . -type d > $real_structure_file
        )
    """.format(
        structure_file_dir = directory_with_structure.structure_file.dirname,
        directory = directory_with_structure.directory.path,
        structure_file = directory_with_structure.structure_file.path,
    )

def _files(directory_with_structure):
    """Return the list of declared [File](https://bazel.build/rules/lib/File) objects in a `directory_with_structure`."""
    return [
        directory_with_structure.directory,
        directory_with_structure.structure_file,
    ]

def _restore(
        directory_with_structure,
        dst,
        options = None):
    """Return a command that restores a `directory_with_structure`.

    It is expected that the shell has properly set up [hermetic tools](hermetic_tools.md#hermetic_tools).

    Args:
        directory_with_structure: struct returned by `declare_directory_with_structure`.
        dst: a string containing the path to the destination directory.
        options: a string containing options to `rsync`. If `None`, default to `"-a"`.
    """

    if options == None:
        options = "-a"

    return """
        cat {structure_file} | sed 's:^:{dst}/:' | xargs mkdir -p
        rsync {options} {src}/ {dst}/
    """.format(
        structure_file = directory_with_structure.structure_file.path,
        options = options,
        src = directory_with_structure.directory.path,
        dst = dst,
    )

def _isinstance(obj):
    return hasattr(obj, "directory") and hasattr(obj, "structure_file")

directory_with_structure = struct(
    make = _make,
    record = _record,
    files = _files,
    restore = _restore,
    isinstance = _isinstance,
)
