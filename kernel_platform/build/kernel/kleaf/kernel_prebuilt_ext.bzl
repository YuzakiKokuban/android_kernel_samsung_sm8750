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
    "//build/kernel/kleaf/impl:declare_kernel_prebuilts.bzl",
    "declare_kernel_prebuilts",
)

visibility("public")

def _kernel_prebuilt_ext_impl(module_ctx):
    declare_kernel_prebuilts.declare_repos(module_ctx, "declare_kernel_prebuilts")

kernel_prebuilt_ext = module_extension(
    doc = "Extension that manages what prebuilts Kleaf should use.",
    implementation = _kernel_prebuilt_ext_impl,
    tag_classes = {
        "declare_kernel_prebuilts": declare_kernel_prebuilts.tag_class,
    },
)
