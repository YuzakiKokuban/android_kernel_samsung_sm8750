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

cc_library(
    name = "zlib",
    srcs = [
        "adler32.c",
        "compress.c",
        "contrib/optimizations/insert_string.h",
        "cpu_features.c",
        "crc32.c",
        "crc_folding.c",
        "deflate.c",
        "gzclose.c",
        "gzlib.c",
        "gzread.c",
        "gzwrite.c",
        "infback.c",
        "inffast.c",
        "inflate.c",
        "inftrees.c",
        "trees.c",
        "uncompr.c",
        "zutil.c",
    ],
    hdrs = [
        "cpu_features.h",
        "crc32.h",
        "deflate.h",
        "gzguts.h",
        "inffast.h",
        "inffixed.h",
        "inflate.h",
        "inftrees.h",
        "trees.h",
        "zconf.h",
        "zlib.h",
        "zutil.h",
    ],
    copts = [
        "-O3",
        "-Wall",
        "-Werror",
        "-Wno-deprecated-non-prototype",
        "-Wno-unused",
        "-Wno-unused-parameter",
        # Use the traditional Rabin-Karp rolling hash to match zlib DEFLATE output exactly.
        "-DCHROMIUM_ZLIB_NO_CASTAGNOLI",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
)
