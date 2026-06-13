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
Provide tools for a hermetic build.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load(
    "//build/kernel/kleaf/impl:hermetic_exec.bzl",
    _hermetic_exec = "hermetic_exec",
    _hermetic_exec_test = "hermetic_exec_test",
)
load("//build/kernel/kleaf/impl:hermetic_genrule.bzl", _hermetic_genrule = "hermetic_genrule")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", _hermetic_toolchain = "hermetic_toolchain")
load("//build/kernel/kleaf/impl:utils.bzl", "utils")

# Re-export functions
hermetic_exec = _hermetic_exec
hermetic_exec_test = _hermetic_exec_test
hermetic_genrule = _hermetic_genrule
hermetic_toolchain = _hermetic_toolchain

_HermeticToolchainInfo = provider(
    doc = "Toolchain information provided by [hermetic_tools](#hermetic_tools).",
    fields = {
        "deps": "a depset containing the hermetic tools",
        "setup": "setup script to initialize the environment to only use the hermetic tools",
        "run_setup": """**IMPLEMENTATION DETAIL; DO NOT USE.**

setup script to initialize the environment to only use the hermetic tools in
[execution phase](https://docs.bazel.build/versions/main/skylark/concepts.html#evaluation-model),
e.g. for generated executables and tests""",
        "run_additional_setup": """**IMPLEMENTATION DETAIL; DO NOT USE.**

Like `run_setup` but preserves original `PATH`.""",
        "internal_hermetic_base": """**IMPLEMENTATION DETAIL; DO NOT USE.**

Path to hermetic tools relative to execroot""",
    },
)

def _get_single_file(ctx, target):
    label = ctx.label.same_package_label(ctx.attr.outer_target_name)
    files_list = target.files.to_list()
    if len(files_list) != 1:
        fail("{}: {} does not contain a single file".format(
            label,
            target.label,
        ))
    return files_list[0]

def _handle_tool(ctx, tool_name, actual_target):
    label = ctx.label.same_package_label(ctx.attr.outer_target_name)

    out = ctx.actions.declare_file("{}/{}".format(ctx.attr.outer_target_name, tool_name))
    target_file = _get_single_file(ctx, actual_target)

    if tool_name not in ctx.attr.extra_args:
        ctx.actions.symlink(
            output = out,
            target_file = target_file,
            is_executable = True,
            progress_message = "Creating symlink to in-tree tool {}/{}".format(
                label,
                tool_name,
            ),
        )
        return [out]

    internal_symlink = ctx.actions.declare_file("{}/kleaf_internal_do_not_use/{}".format(ctx.attr.outer_target_name, tool_name))
    ctx.actions.symlink(
        output = internal_symlink,
        target_file = target_file,
        is_executable = True,
        progress_message = "Creating internal symlink to in-tree tool {}/{}".format(
            label,
            tool_name,
        ),
    )

    ctx.actions.symlink(
        output = out,
        target_file = ctx.executable._arg_wrapper,
        is_executable = True,
        progress_message = "Creating symlink to in-tree tool {}/{}".format(
            label,
            tool_name,
        ),
    )
    extra_args = "\n".join(ctx.attr.extra_args[tool_name])
    extra_args_file = ctx.actions.declare_file("{}/kleaf_internal_do_not_use/{}_args.txt".format(ctx.attr.outer_target_name, tool_name))
    ctx.actions.write(extra_args_file, extra_args)
    return [out, internal_symlink, extra_args_file]

def _handle_hermetic_symlinks(ctx, symlinks_attr):
    all_outputs = []
    for actual_target, tool_names in symlinks_attr.items():
        for tool_name in tool_names.split(":"):
            tool_outs = _handle_tool(ctx, tool_name, actual_target)
            all_outputs.extend(tool_outs)

    return all_outputs

def _hermetic_tools_internal_impl(ctx):
    all_outputs = _handle_hermetic_symlinks(ctx, ctx.attr.symlinks)

    if ctx.attr._disable_symlink_source[BuildSettingInfo].value:
        transitive_deps = []
    else:
        transitive_deps = [target.files for target in ctx.attr.symlinks]

    transitive_deps += [target.files for target in ctx.attr.deps]
    transitive_deps.append(ctx.attr._arg_wrapper.files)

    fail_hard = """
         # error on failures
           set -e
           set -o pipefail
    """

    hermetic_base = paths.join(
        utils.package_bin_dir(ctx),
        ctx.attr.outer_target_name,
    )
    hermetic_base_short = paths.join(
        ctx.label.workspace_root,
        ctx.label.package,
        ctx.attr.outer_target_name,
    )

    hashbang = """#!/bin/bash -e
"""

    setup = fail_hard + """
                export PATH=$({path}/readlink -m {path})
                # Ensure _setup_env.sh keeps the original items in PATH
                export KLEAF_INTERNAL_BUILDTOOLS_PREBUILT_BIN={path}
""".format(path = hermetic_base)
    run_setup = hashbang + fail_hard + """
                export PATH=$({path}/readlink -m {path})
""".format(path = hermetic_base_short)
    run_additional_setup = fail_hard + """
                export PATH=$({path}/readlink -m {path}):$PATH
""".format(path = hermetic_base_short)

    hermetic_toolchain_info = _HermeticToolchainInfo(
        deps = depset(all_outputs, transitive = transitive_deps),
        setup = setup,
        run_setup = run_setup,
        run_additional_setup = run_additional_setup,
        internal_hermetic_base = hermetic_base,
    )

    infos = [
        DefaultInfo(files = depset(all_outputs)),
        platform_common.ToolchainInfo(
            hermetic_toolchain_info = hermetic_toolchain_info,
        ),
        OutputGroupInfo(**{
            file.basename: depset([file])
            for file in all_outputs
            if "kleaf_internal_do_not_use" not in file.path
        }),
    ]

    return infos

_hermetic_tools_internal = rule(
    implementation = _hermetic_tools_internal_impl,
    doc = """Internal helper rule for hermetic tools without any transition""",
    attrs = {
        "outer_target_name": attr.string(),
        "deps": attr.label_list(allow_files = True),
        "symlinks": attr.label_keyed_string_dict(allow_files = True),
        "extra_args": attr.string_list_dict(),
        "_disable_symlink_source": attr.label(
            default = "//build/kernel/kleaf:incompatible_disable_hermetic_tools_symlink_source",
        ),
        "_arg_wrapper": attr.label(
            default = "//build/kernel/kleaf/impl:arg_wrapper",
            executable = True,
            # Prevent inadvertent exec transition that messes up the
            # path calculation. Exec transition needs to be done on the whole
            # hermetic_tools target.
            cfg = "target",
        ),
    },
)

def _hermetic_tools_transition_wrapper_impl(ctx):
    actual = ctx.attr.actual
    return [
        actual[DefaultInfo],
        actual[OutputGroupInfo],
        actual[platform_common.ToolchainInfo],
    ]

_hermetic_tools_transition_wrapper = rule(
    implementation = _hermetic_tools_transition_wrapper_impl,
    doc = "Provide tools for a hermetic build.",
    attrs = {
        "actual": attr.label(cfg = "exec"),
    },
)

def hermetic_tools(
        name,
        deps = None,
        symlinks = None,
        extra_args = None,
        **kwargs):
    """Provide tools for a hermetic build.

    Args:
        name: name of the target
        deps: additional dependencies. These aren't added to the `PATH`.
        symlinks: A dictionary, where keys are labels to an executable, and
            values are names to the tool, separated with `:`. e.g.

            ```
            {"//label/to:toybox": "cp:realpath"}
            ```
        extra_args: Keys are names to the tool (see `symlinks`). Values are
            extra arguments added to the tool at the end.
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """
    private_kwargs = kwargs | {
        "visibility": ["//visibility:private"],
    }

    _hermetic_tools_internal(
        name = name + "_actual",
        outer_target_name = name,
        deps = deps,
        symlinks = symlinks,
        extra_args = extra_args,
        **private_kwargs
    )

    _hermetic_tools_transition_wrapper(
        name = name,
        actual = name + "_actual",
        **kwargs
    )
