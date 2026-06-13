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
Utility public constants.
"""

load(
    "//build/kernel/kleaf/impl:constants.bzl",
    "DEFAULT_IMAGES",
)

_common_outs = [
    "System.map",
    "modules.builtin",
    "modules.builtin.modinfo",
    "vmlinux",
    "vmlinux.symvers",
]

# Common output files for aarch64 kernel builds.
# Sync with build.config.gki.{aarch64,riscv64}
DEFAULT_GKI_OUTS = _common_outs + DEFAULT_IMAGES

# Common output files for x86_64 kernel builds.
X86_64_OUTS = _common_outs + ["bzImage"]

# Deprecated; use AARCH64_GKI_OUTS
aarch64_outs = DEFAULT_GKI_OUTS

# Deprecated; use X86_64_OUTS
x86_64_outs = X86_64_OUTS

LTO_VALUES = (
    "default",
    "none",
    "thin",
    "full",
    "fast",
)
