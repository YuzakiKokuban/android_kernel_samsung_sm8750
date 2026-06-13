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

"""Helper to resolve toolchain for a single platform."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load(
    "@bazel_tools//tools/build_defs/cc:action_names.bzl",
    "CPP_LINK_EXECUTABLE_ACTION_NAME",
    "C_COMPILE_ACTION_NAME",
)
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")
load(":common_providers.bzl", "KernelPlatformToolchainInfo")

visibility("//build/kernel/kleaf/...")

def _kernel_platform_toolchain_impl(ctx):
    cc_info = cc_common.merge_cc_infos(
        cc_infos = [src[CcInfo] for src in ctx.attr.deps],
    )

    cc_toolchain = find_cpp_toolchain(ctx, mandatory = False)

    if not cc_toolchain:
        # Intentionally not put any keys so kernel_toolchains emit a hard error
        return KernelPlatformToolchainInfo()

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features + [
            "kleaf-lld-compiler-rt",
        ],
        unsupported_features = [
            # -no-canonical-prefixes is added to work around
            # https://github.com/bazelbuild/bazel/issues/4605
            # "cxx_builtin_include_directory doesn't work with non-absolute path"
            # Disable it.
            "kleaf-no-canonical-prefixes",
            # Disable flags for C++. These only applies to cc_* rules with
            # C++ code.
            "kleaf-host-cc-rules-flags",
        ],
    )
    compile_variables = cc_common.create_compile_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        user_compile_flags = [],  # copts
        include_directories = cc_info.compilation_context.includes,
        quote_include_directories = cc_info.compilation_context.quote_includes,
        system_include_directories = cc_info.compilation_context.system_includes,
    )
    compile_command_line = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = C_COMPILE_ACTION_NAME,
        variables = compile_variables,
    )

    # Handle "//prebuilts/kernel-build-tools:linux_x86_imported_libs",
    user_link_flags = []
    additional_libs = []
    library_search_directories = []
    for dep in ctx.attr.deps:
        if dep[CcInfo].linking_context:
            for linker_input in dep[CcInfo].linking_context.linker_inputs.to_list():
                for lib in linker_input.libraries:
                    if lib.dynamic_library:
                        additional_libs.append(lib.dynamic_library)
                        library_search_directories.append(lib.dynamic_library.dirname)

    link_variables = cc_common.create_link_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        user_link_flags = user_link_flags,  # linkopts
        library_search_directories = depset(library_search_directories),
    )
    link_command_line = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        # Use CPP_LINK_EXECUTABLE_ACTION_NAME to get rid of "-shared"
        action_name = CPP_LINK_EXECUTABLE_ACTION_NAME,
        variables = link_variables,
    )

    all_files = depset(transitive = [
        depset(cc_info.compilation_context.direct_headers),
        cc_info.compilation_context.headers,
        cc_toolchain.all_files,
        depset(additional_libs),
    ])

    # All executables are in the same place, so just use the compiler executable
    # to locate PATH.
    compiler_executable = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = C_COMPILE_ACTION_NAME,
    )
    bin_path = paths.dirname(compiler_executable)

    return KernelPlatformToolchainInfo(
        compiler_version = cc_toolchain.compiler,
        toolchain_id = cc_toolchain.toolchain_id,
        all_files = all_files,
        cflags = compile_command_line,
        ldflags = link_command_line,
        bin_path = bin_path,
    )

kernel_platform_toolchain = rule(
    doc = """Helper to resolve toolchain for a single platform.""",
    implementation = _kernel_platform_toolchain_impl,
    attrs = {
        "deps": attr.label_list(providers = [CcInfo]),
        # For using mandatory = False
        "_cc_toolchain": attr.label(default = "@bazel_tools//tools/cpp:optional_current_cc_toolchain"),
    },
    toolchains = use_cpp_toolchain(mandatory = False),
    fragments = ["cpp"],
)
