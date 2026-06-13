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

"""Returns `src_protected_exports_list` and `src_protected_modules_list` for a `kernel_build`.

"""

load(
    ":common_providers.bzl",
    "KernelBuildAbiInfo",
)

visibility("//build/kernel/kleaf/...")

def _get_src_protected_exports_list_impl(ctx):
    src_protected_exports_list = ctx.attr.kernel_build[KernelBuildAbiInfo].src_protected_exports_list
    if not src_protected_exports_list:
        fail("{} does not set src_protected_exports_list correctly.".format(
            ctx.attr.kernel_build.label,
        ))
    return DefaultInfo(files = depset([
        src_protected_exports_list,
    ]))

get_src_protected_exports_list = rule(
    doc = "Returns `src_protected_exports_list` for a `kernel_build`.",
    implementation = _get_src_protected_exports_list_impl,
    attrs = {
        "kernel_build": attr.label(providers = [KernelBuildAbiInfo]),
    },
)

def _get_src_protected_modules_list_impl(ctx):
    src_protected_modules_list = ctx.attr.kernel_build[KernelBuildAbiInfo].src_protected_modules_list
    if not src_protected_modules_list:
        fail("{} does not set src_protected_modules_list correctly.".format(
            ctx.attr.kernel_build.label,
        ))
    return DefaultInfo(files = depset([
        src_protected_modules_list,
    ]))

get_src_protected_modules_list = rule(
    doc = "Returns `src_protected_modules_list` for a `kernel_build`.",
    implementation = _get_src_protected_modules_list_impl,
    attrs = {
        "kernel_build": attr.label(providers = [KernelBuildAbiInfo]),
    },
)
