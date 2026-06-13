# Copyright (C) 2024 The Android Open Source Project
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

"""Rules to display dependencies between binaries."""

load(":abi/abi_transitions.bzl", "abi_common_attrs", "notrim_transition")
load(
    ":common_providers.bzl",
    "KernelBuildAbiInfo",
    "KernelModuleInfo",
    "KernelSerializedEnvInfo",
)
load(":debug.bzl", "debug")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

def _dependency_graph_extractor_impl(ctx):
    out = ctx.actions.declare_file("{}/dependency_graph.json".format(ctx.attr.name))
    intermediates_dir = utils.intermediates_dir(ctx)
    vmlinux = utils.find_file(
        name = "vmlinux",
        files = ctx.files.kernel_build,
        what = "{}: kernel_build".format(
            ctx.attr.name,
        ),
        required = True,
    )
    srcs = [vmlinux]
    if not ctx.attr.exclude_base_kernel_modules:
        include_in_tree_modules = utils.find_files(suffix = ".ko", files = ctx.files.kernel_build)
        srcs += include_in_tree_modules

    # External modules
    for kernel_module in ctx.attr.kernel_modules:
        if KernelModuleInfo in kernel_module:
            srcs += kernel_module[KernelModuleInfo].files.to_list()
        else:
            srcs += kernel_module.files.to_list()

    inputs = [] + srcs
    transitive_inputs = [ctx.attr.kernel_build[KernelSerializedEnvInfo].inputs]
    tools = [ctx.executable._dependency_graph_extractor]
    transitive_tools = [ctx.attr.kernel_build[KernelSerializedEnvInfo].tools]

    base_modules_archive_cmd = ""
    if not ctx.attr.exclude_base_kernel_modules:
        # Get the signed and stripped module archive for the GKI modules
        base_modules_archive = ctx.attr.kernel_build[KernelBuildAbiInfo].base_modules_staging_archive
        if not base_modules_archive:
            base_modules_archive = ctx.attr.kernel_build[KernelBuildAbiInfo].modules_staging_archive
        inputs.append(base_modules_archive)
        base_modules_archive_cmd = """
            mkdir -p {intermediates_dir}/temp
            tar xf {base_modules_archive} -C {intermediates_dir}/temp
            find {intermediates_dir}/temp -name '*.ko' -exec mv -t {intermediates_dir} {{}} \\;
            rm -rf {intermediates_dir}/temp
        """.format(
            base_modules_archive = base_modules_archive.path,
            intermediates_dir = intermediates_dir,
        )

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ctx.attr.kernel_build[KernelSerializedEnvInfo],
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )
    command += """
        mkdir -p {intermediates_dir}
        # Extract archive and copy the modules from the base kernel first.
        {base_modules_archive_cmd}
        # Copy other inputs including vendor modules; This will overwrite modules being overridden.
        cp -pfl {srcs} {intermediates_dir}
        {dependency_graph_extractor} {intermediates_dir} {output}
        rm -rf {intermediates_dir}
    """.format(
        srcs = " ".join([file.path for file in srcs]),
        intermediates_dir = intermediates_dir,
        dependency_graph_extractor = ctx.executable._dependency_graph_extractor.path,
        output = out.path,
        base_modules_archive_cmd = base_modules_archive_cmd,
    )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = [out],
        command = command,
        tools = depset(tools, transitive = transitive_tools),
        progress_message = "Obtaining dependency graph {}".format(ctx.label),
        mnemonic = "KernelDependencyGraphExtractor",
    )

    return DefaultInfo(files = depset([out]))

dependency_graph_extractor = rule(
    implementation = _dependency_graph_extractor_impl,
    doc = """ A rule that extracts a symbol dependency graph from a kernel build and modules.

      It works by matching undefined symbols from one module with exported symbols from other.

      * Inputs:
        It receives a Kernel build target, where the analysis will run (vmlinux + in-tree modules),
         aditionally a list of external modules can be accepted.

      * Outputs:
        A `dependency_graph.json` file describing the graph as an adjacency list.

      * Example:
        ```
        dependency_graph_extractor(
            name = "db845c_dependencies",
            kernel_build = ":db845c",
            # kernel_modules = [],
        )
        ```
    """,
    attrs = {
        "kernel_build": attr.label(providers = [KernelSerializedEnvInfo, KernelBuildAbiInfo]),
        # For label targets they should provide KernelModuleInfo.
        "kernel_modules": attr.label_list(allow_files = True),
        "exclude_base_kernel_modules": attr.bool(
            doc = "Whether the analysis should made for only external modules.",
        ),
        "_dependency_graph_extractor": attr.label(
            default = "//build/kernel:dependency_graph_extractor",
            cfg = "exec",
            executable = True,
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    } | abi_common_attrs(),
    cfg = notrim_transition,
)

def _dependency_graph_drawer_impl(ctx):
    out = ctx.actions.declare_file("{}/dependency_graph.dot".format(ctx.attr.name))
    input = ctx.file.adjacency_list
    tool = ctx.executable._dependency_graph_drawer
    flags = []
    if ctx.attr.colorful:
        flags.append("--colors")

    command = """
        {dependency_graph_drawer} {input} {output} {flags}
    """.format(
        input = input.path,
        dependency_graph_drawer = tool.path,
        flags = " ".join(flags),
        output = out.path,
    )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        inputs = depset([input]),
        outputs = [out],
        command = command,
        tools = depset([ctx.executable._dependency_graph_drawer]),
        progress_message = "Drawing a dependency graph {}".format(ctx.label),
        mnemonic = "KernelDependencyGraphDrawer",
    )

    return DefaultInfo(files = depset([out]))

dependency_graph_drawer = rule(
    implementation = _dependency_graph_drawer_impl,
    doc = """ A rule that creates a [Graphviz](https://graphviz.org/) diagram file.

      * Inputs:
        A json file describing a graph as an adjacency list.

      * Outputs:
        A `dependency_graph.dot` file containing the diagram representation.

      * NOTE: For further simplification of the resulting diagram
        [tred utility](https://graphviz.org/docs/cli/tred/) from the CLI can
        be used as in the following example:
        ```
        tred dependency_graph.dot > simplified.dot
        ```

      * Example:
        ```
        dependency_graph_drawer(
            name = "db845c_dependency_graph",
            adjacency_list = ":db845c_dependencies",
        )
        ```
    """,
    attrs = {
        "adjacency_list": attr.label(allow_single_file = True, mandatory = True),
        "colorful": attr.bool(
            doc = "Whether outgoing edges from every node are colored.",
        ),
        "_dependency_graph_drawer": attr.label(
            default = "//build/kernel:dependency_graph_drawer",
            cfg = "exec",
            executable = True,
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
)

def dependency_graph(
        name,
        kernel_build,
        kernel_modules,
        colorful = None,
        exclude_base_kernel_modules = None,
        **kwargs):
    """Declare targets for dependency graph visualization.

    Output:
        File with a diagram representing a graph in DOT language.

    Args:
        name: Name of this target.
        kernel_build: The [`kernel_build`](#kernel_build).
        kernel_modules: A list of external [`kernel_module()`](#kernel_module)s.
        colorful: When set to True, outgoing edges from every node are colored differently.
        exclude_base_kernel_modules: Whether the analysis should made for only external modules.
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).

    """

    dependency_graph_extractor(
        name = name + "_extractor",
        kernel_build = kernel_build,
        kernel_modules = kernel_modules,
        exclude_base_kernel_modules = exclude_base_kernel_modules,
        **kwargs
    )

    dependency_graph_drawer(
        name = name + "_drawer",
        adjacency_list = name + "_extractor",
        colorful = colorful,
    )

    native.filegroup(
        name = name,
        srcs = [
            name + "_drawer",
        ],
        **kwargs
    )
