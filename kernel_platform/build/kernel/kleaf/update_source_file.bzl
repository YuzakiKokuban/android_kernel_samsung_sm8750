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

"""Upon `bazel run`, updates a source file."""

load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")

def _update_source_file_impl(ctx):
    # --copy-links because src is a symlink inside the sandbox. We need to copy
    # the referent.
    # --no-perms because generated files usually have exec bit set, which
    # we don't want in source files.
    hermetic_tools = hermetic_toolchain.get(ctx)
    script = hermetic_tools.run_setup + """
        rsync --copy-links --no-perms --times {src} $(readlink -m {dst})
    """.format(
        src = ctx.file.src.short_path,
        dst = ctx.file.dst.short_path,
    )

    ctx.actions.write(
        content = script,
        output = ctx.outputs.executable,
        is_executable = True,
    )
    runfiles = [
        ctx.file.src,
        ctx.file.dst,
    ]
    return [DefaultInfo(runfiles = ctx.runfiles(files = runfiles, transitive_files = hermetic_tools.deps))]

update_source_file = rule(
    implementation = _update_source_file_impl,
    attrs = {
        "src": attr.label(allow_single_file = True),
        "dst": attr.label(allow_single_file = True),
    },
    executable = True,
    toolchains = [hermetic_toolchain.type],
)
