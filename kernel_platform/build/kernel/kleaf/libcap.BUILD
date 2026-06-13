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
load("@kleaf//build/kernel/kleaf:hermetic_tools.bzl", "hermetic_genrule")

cc_library(
    name = "libcap",
    srcs = [
        "libcap/cap_alloc.c",
        "libcap/cap_extint.c",
        "libcap/cap_file.c",
        "libcap/cap_flag.c",
        "libcap/cap_proc.c",
        "libcap/cap_text.c",
    ] + glob(["**/*.h"]),
    copts = [
        "-Wno-pointer-arith",
        "-Wno-tautological-compare",
        "-Wno-unused-parameter",
        "-Wno-unused-result",
        "-Wno-unused-variable",
    ],
    includes = [
        "libcap",
        "libcap/include",
        "libcap/include/uapi",
    ],
    visibility = ["//visibility:public"],
    deps = ["cap_names_hdr"],
)

hermetic_genrule(
    name = "cap_names_list",
    srcs = ["libcap/include/uapi/linux/capability.h"],
    outs = ["cap_names.list.h"],
    cmd = "awk -f $(location generate_cap_names_list.awk) $(location libcap/include/uapi/linux/capability.h) > $@",
    tools = ["generate_cap_names_list.awk"],
)

cc_library(
    name = "cap_names_list_hdr",
    hdrs = [":cap_names_list"],
)

hermetic_genrule(
    name = "cap_names",
    outs = ["cap_names.h"],
    cmd = "$(location :_makenames) > $@",
    tools = [":_makenames"],
)

cc_library(
    name = "cap_names_hdr",
    hdrs = [":cap_names"],
)

cc_binary(
    name = "_makenames",
    srcs = ["libcap/_makenames.c"],
    copts = [
        "-Wno-pointer-arith",
        "-Wno-tautological-compare",
        "-Wno-unused-parameter",
        "-Wno-unused-result",
        "-Wno-unused-variable",
    ],
    deps = [":cap_names_list_hdr"],
)
