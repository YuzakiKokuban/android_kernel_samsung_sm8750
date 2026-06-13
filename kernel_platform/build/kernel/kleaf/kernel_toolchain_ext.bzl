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

"""Extension that helps building Android kernel and drivers."""

load(
    "//build/kernel/kleaf/impl:declare_host_tools.bzl",
    "declare_host_tools",
)
load(
    "//build/kernel/kleaf/impl:declare_toolchain_constants.bzl",
    "declare_toolchain_constants",
)

visibility("public")

def _kernel_toolchain_ext_impl(module_ctx):
    declare_toolchain_constants.declare_repos(module_ctx, "declare_toolchain_constants")
    declare_host_tools.declare_repos(module_ctx, "declare_host_tools")

kernel_toolchain_ext = module_extension(
    doc = "Extension that manages what toolchain Kleaf should use.",
    implementation = _kernel_toolchain_ext_impl,
    tag_classes = {
        "declare_toolchain_constants": declare_toolchain_constants.tag_class,
        "declare_host_tools": declare_host_tools.tag_class,
    },
)
