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

cc_library(
    name = "libcap-ng-config",
    hdrs = ["config.h"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "libcap-ng",
    srcs = [
        "libcap-ng-0.7/src/cap-ng.c",
        "libcap-ng-0.7/src/lookup_table.c",
    ],
    hdrs = [
        "libcap-ng-0.7/src/cap-ng.h",
        "libcap-ng-0.7/src/captab.h",
    ],
    copts = [
        "-Wall",
        "-Werror",
        "-Wno-enum-conversion",
        "-Wno-unused-parameter",
    ],
    strip_include_prefix = "libcap-ng-0.7/src",
    visibility = ["//visibility:public"],
    deps = [":libcap-ng-config"],
)
