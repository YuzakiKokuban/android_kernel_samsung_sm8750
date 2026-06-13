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

"""Module extension that instantiates key_value_repo."""

load("//build/kernel/kleaf:key_value_repo.bzl", "key_value_repo")
load("//prebuilts/clang/host/linux-x86/kleaf:clang_toolchain_repository.bzl", "clang_toolchain_repository")

visibility("public")

# Usually we do not refer to //common in build/kernel. This is an exception because
# - It is a sensible default
# - It may be overridden by calling declare_toolchain_constants at the root module
_DEFAULT_TOOLCHAIN_CONSTANTS = "//common:build.config.constants"

def _declare_repos(module_ctx, tag_name):
    root_toolchain_constants = []
    kleaf_toolchain_constants = []
    for module in module_ctx.modules:
        installed_constants = [installed.toolchain_constants for installed in getattr(module.tags, tag_name)]
        if module.is_root:
            root_toolchain_constants += installed_constants
        if module.name == "kleaf":
            kleaf_toolchain_constants += installed_constants

    toolchain_constants = None
    if root_toolchain_constants:
        if len(root_toolchain_constants) > 1:
            fail("kernel_toolchain_ext is installed {} times at root module, expected once".format(len(toolchain_constants)))
        toolchain_constants = root_toolchain_constants[0]
    elif kleaf_toolchain_constants:
        if len(kleaf_toolchain_constants) > 1:
            fail("kernel_toolchain_ext is installed {} times at @kleaf, expected once".format(len(toolchain_constants)))
        toolchain_constants = kleaf_toolchain_constants[0]

    if not toolchain_constants:
        toolchain_constants = _DEFAULT_TOOLCHAIN_CONSTANTS

    key_value_repo(
        name = "kernel_toolchain_info",
        srcs = [toolchain_constants],
    )
    clang_toolchain_repository(
        name = "kleaf_clang_toolchain",
    )

_tag_class = tag_class(
    doc = "Declares a potential location that contains toolchain information.",
    attrs = {
        "toolchain_constants": attr.label(
            doc = """Label to `build.config.constants`.

                If `declare_toolchain_constants()` is never called, or called
                with `toolchain_constants = None`, default is `{}`.
            """.format(repr(_DEFAULT_TOOLCHAIN_CONSTANTS)),
        ),
    },
)

declare_toolchain_constants = struct(
    tag_class = _tag_class,
    declare_repos = _declare_repos,
)
