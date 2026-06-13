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
Defines repositories in a Kleaf workspace.
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("//build/kernel/kleaf:key_value_repo.bzl", "key_value_repo")
load("//build/kernel/kleaf/impl:declare_host_tools.bzl", "kleaf_host_tools_repo")
load(
    "//build/kernel/kleaf/impl:kernel_prebuilt_repo.bzl",
    "kernel_prebuilt_repo",
)
load(
    "//build/kernel/kleaf/impl:kernel_prebuilt_utils.bzl",
    "CI_TARGET_MAPPING",
)
load(
    "//build/kernel/kleaf/impl:local_repository.bzl",
    "kleaf_local_repository",
    "new_kleaf_local_repository",
)
load("//prebuilts/clang/host/linux-x86/kleaf:clang_toolchain_repository.bzl", "clang_toolchain_repository")

# buildifier: disable=unnamed-macro
def define_kleaf_workspace(
        common_kernel_package = None,
        include_remote_java_tools_repo = False,
        artifact_url_fmt = None):
    """Common macro for defining repositories in a Kleaf workspace.

    **This macro must only be called from `WORKSPACE` or `WORKSPACE.bazel`
    files, not `BUILD` or `BUILD.bazel` files!**

    If [`define_kleaf_workspace_epilog`](workspace_epilog.md#define_kleaf_workspace_epilog) is
    called, it must be called after `define_kleaf_workspace` is called.

    Args:
      common_kernel_package: Default is `"@//common"`. The package to the common
        kernel source tree.

        As a legacy behavior, if the provided string does not start with
        `@` or `//`, it is prepended with `@//`.

        Do not provide the trailing `/`.
      include_remote_java_tools_repo: Default is `False`. Whether to vendor two extra
        repositories: remote_java_tools and remote_java_tools_linux.

        These respositories should exist under `//prebuilts/bazel/`
      artifact_url_fmt: API endpoint for Android CI artifacts.
        The format may include anchors for the following properties:
          * {build_number}
          * {target}
          * {filename}

    Deprecated:
      The use of legacy WORKSPACE is deprecated. Please migrate to Bazel modules.
      See [bzlmod.md](../bzlmod.md).
    """

    # buildifier: disable=print
    print("""
WARNING: The use of legacy WORKSPACE is deprecated. Please migrate to Bazel modules.
  For details, see build/kernel/kleaf/docs/bzlmod.md.
""")

    if common_kernel_package == None:
        common_kernel_package = str(Label("//common:x")).removesuffix(":x")
    if not common_kernel_package.startswith("@") and not common_kernel_package.startswith("//"):
        common_kernel_package = str(Label("//{}:x".format(common_kernel_package))).removesuffix(":x")

        # buildifier: disable=print
        print("""
WARNING: define_kleaf_workspace() should be called with common_kernel_package={}.
    This will become an error in the future.""".format(
            repr(common_kernel_package),
        ))

    maybe(
        repo_rule = kleaf_local_repository,
        name = "bazel_skylib",
        path = "external/bazel-skylib",
    )

    maybe(
        repo_rule = kleaf_local_repository,
        name = "io_abseil_py",
        path = "external/python/absl-py",
    )

    maybe(
        repo_rule = kleaf_local_repository,
        name = "rules_license",
        path = "external/bazelbuild-rules_license",
    )

    maybe(
        repo_rule = kleaf_local_repository,
        name = "rules_pkg",
        path = "external/bazelbuild-rules_pkg",
    )

    maybe(
        repo_rule = kleaf_local_repository,
        name = "rules_rust",
        path = "external/bazelbuild-rules_rust",
    )

    # Superset of all tools we need from host.
    # For the subset of host tools we typically use for a kernel build,
    # see //build/kernel:hermetic-tools.
    kleaf_host_tools_repo(
        name = "kleaf_host_tools",
        host_tools = [
            "bash",
            "perl",
            "rsync",
            "sh",
            # For BTRFS (b/292212788)
            "find",
        ],
    )

    # External repos without Bazel support.
    # https://docs.bazel.build/versions/main/external.html#non-bazel-projects
    new_kleaf_local_repository(
        name = "prebuilt_ndk",
        path = "prebuilts/ndk-r26",
        build_file = "build/kernel/kleaf/ndk.BUILD",
    )

    kleaf_workspace_name = Label("//build/kernel/kleaf").workspace_name
    new_kleaf_local_repository(
        name = "libcap",
        path = "external/libcap",
        build_file = "build/kernel/kleaf/libcap.BUILD",
        repo_mapping = {"@kleaf": "@" + kleaf_workspace_name},
    )

    new_kleaf_local_repository(
        name = "libcap_ng",
        path = "external/libcap-ng",
        build_file = "build/kernel/kleaf/libcap_ng.BUILD",
    )

    new_kleaf_local_repository(
        name = "zlib",
        path = "external/zlib",
        build_file = "build/kernel/kleaf/zlib.BUILD",
    )

    new_kleaf_local_repository(
        name = "zopfli",
        path = "external/zopfli",
        build_file = "build/kernel/kleaf/zopfli.BUILD",
    )

    new_kleaf_local_repository(
        name = "pigz",
        path = "external/pigz",
        build_file = "build/kernel/kleaf/pigz.BUILD",
    )

    key_value_repo(
        name = "kernel_toolchain_info",
        srcs = ["{}:build.config.constants".format(common_kernel_package)],
        additional_values = {
            "common_kernel_package": common_kernel_package,
        },
    )

    for target, value in CI_TARGET_MAPPING.items():
        kernel_prebuilt_repo(
            name = value["repo_name"],
            apparent_name = value["repo_name"],
            artifact_url_fmt = artifact_url_fmt,
            target = target,
        )

    maybe(
        repo_rule = kleaf_local_repository,
        name = "rules_python",
        path = "external/bazelbuild-rules_python",
    )

    # The following 2 repositories contain prebuilts that are necessary to the Java Rules.
    # They are vendored locally to avoid the need for CI bots to download them.
    if include_remote_java_tools_repo:
        kleaf_local_repository(
            name = "remote_java_tools",
            path = "prebuilts/bazel/common/remote_java_tools",
        )

        kleaf_local_repository(
            name = "remote_java_tools_linux",
            path = "prebuilts/bazel/linux-x86_64/remote_java_tools_linux",
        )

    # Use checked-in JDK from prebuilts as local_jdk
    #   Needed for stardoc
    # Note: This was not added directly to avoid conflicts with roboleaf,
    #   see https://android-review.googlesource.com/c/platform/build/bazel/+/2457390
    #   for more details.
    new_kleaf_local_repository(
        name = "local_jdk",
        path = "prebuilts/jdk/jdk11/linux-x86",
        build_file = "build/kernel/kleaf/jdk11.BUILD",
    )

    # Need rules_cc for any py_binary targets (especially copy_to_dist_dir).
    maybe(
        repo_rule = kleaf_local_repository,
        name = "rules_cc",
        path = "external/bazelbuild-rules_cc",
    )

    # Stub out @remote_coverage_tools required for testing.
    kleaf_local_repository(
        name = "remote_coverage_tools",
        path = "build/bazel_common_rules/rules/coverage/remote_coverage_tools",
    )

    # Use checked-in JDK from prebuilts as local_jdk
    #   Needed for stardoc
    native.register_toolchains(
        "@local_jdk//:all",
    )

    # Label(): Resolve the label against this extension (register.bzl) so the
    # workspace name is injected properly when //prebuilts is in a subworkspace.
    # str(): register_toolchains() only accepts strings, not Labels.
    native.register_toolchains(
        str(Label("//prebuilts/build-tools:py_toolchain")),
        str(Label("//build/kernel:hermetic_tools_toolchain")),
    )

    clang_toolchain_repository(
        name = "kleaf_clang_toolchain",
    )
    native.register_toolchains("@kleaf_clang_toolchain//:all")
