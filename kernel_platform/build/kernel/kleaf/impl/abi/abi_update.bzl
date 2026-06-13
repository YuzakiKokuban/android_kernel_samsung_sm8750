# Copyright (C) 2023 The Android Open Source Project
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

"""When `bazel run`, updates an ABI definition."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")

visibility("//build/kernel/kleaf/...")

def _abi_update_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)

    script = """
        {hermetic_setup}
        {nodiff_update}

        if [[ $1 == "--commit" ]]; then
            echo "WARNING: --commit is deprecated. Please add {abi_definition} and commit manually." >&2
            echo "  You may use --print_git_commands to print sample git commands to run." >&2
        fi

        if [[ $1 == "--commit" ]] || [[ $1 == "--print_git_commands" ]]; then
            echo "  git -C ${{BUILD_WORKSPACE_DIRECTORY}}/{abi_definition_dir} commit \\\\"
            echo "    -F ${{BUILD_WORKSPACE_DIRECTORY}}/{git_message} \\\\"
            echo "    --signoff --edit -- {abi_definition_name}"
        fi

        {diff}
    """.format(
        hermetic_setup = hermetic_tools.run_setup,
        nodiff_update = ctx.executable.nodiff_update.short_path,
        abi_definition = ctx.file.abi_definition_stg.short_path,
        diff = ctx.executable.diff.short_path,
        # Use .path because these are displayed to the user relative
        # to BUILD_WORKSPACE_DIRECTORY
        abi_definition_dir = paths.dirname(ctx.file.abi_definition_stg.path),
        abi_definition_name = paths.basename(ctx.file.abi_definition_stg.path),
        git_message = ctx.file.git_message.path,
    )

    executable = ctx.actions.declare_file("{}.sh".format(ctx.attr.name))
    ctx.actions.write(executable, script, is_executable = True)

    runfiles = ctx.runfiles(files = [
        ctx.file.abi_definition_stg,
        ctx.file.git_message,
    ], transitive_files = hermetic_tools.deps)
    runfiles = runfiles.merge_all([
        ctx.attr.nodiff_update[DefaultInfo].default_runfiles,
        ctx.attr.diff[DefaultInfo].default_runfiles,
    ])

    return DefaultInfo(
        files = depset([executable]),
        executable = executable,
        runfiles = runfiles,
    )

# Sync with kleaf/bazel.py
abi_update = rule(
    implementation = _abi_update_impl,
    attrs = {
        "abi_definition_stg": attr.label(
            doc = "source ABI definition file",
            allow_single_file = True,
        ),
        "git_message": attr.label(
            doc = "git commit message",
            allow_single_file = True,
        ),
        "nodiff_update": attr.label(
            doc = "executable to update without showing diff result",
            executable = True,
            # Use target platform for the executable because the underlying
            # ABI definition abi_dump from kernel_build is built against the
            # target platform.
            cfg = "target",
        ),
        "diff": attr.label(
            doc = "show diff result",
            executable = True,
            # Use target platform for the executable because the underlying
            # ABI definition abi_dump from kernel_build is built against the
            # target platform.
            cfg = "target",
        ),
    },
    toolchains = [hermetic_toolchain.type],
    executable = True,
)
