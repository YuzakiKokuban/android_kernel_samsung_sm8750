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

"""Utilities for configuring trim_nonlisted_kmi."""

visibility("//build/kernel/kleaf/...")

FORCE_DISABLE_TRIM = "//build/kernel/kleaf/impl:force_disable_trim"
_FORCE_DISABLE_TRIM_IS_TRUE = "//build/kernel/kleaf/impl:force_disable_trim_is_true"
_GCOV_IS_TRUE = "//build/kernel/kleaf:gcov_is_true"
_KASAN_IS_TRUE = "//build/kernel/kleaf:kasan_is_true"
_KCSAN_IS_TRUE = "//build/kernel/kleaf:kcsan_is_true"
TRIM_NONLISTED_KMI_ATTR_NAME = "trim_nonlisted_kmi"

def _selected_attr(attr_val):
    return select({
        Label(_FORCE_DISABLE_TRIM_IS_TRUE): False,
        Label(_GCOV_IS_TRUE): False,
        Label(_KASAN_IS_TRUE): False,
        Label(_KCSAN_IS_TRUE): False,
        "//conditions:default": attr_val,
    })

def _trim_nonlisted_kmi_get_value(ctx):
    """Returns the value of the real `trim_nonlisted_kmi` configuration."""
    return getattr(ctx.attr, TRIM_NONLISTED_KMI_ATTR_NAME)

trim_nonlisted_kmi_utils = struct(
    get_value = _trim_nonlisted_kmi_get_value,
    selected_attr = _selected_attr,
)
