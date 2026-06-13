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

"""Internal constants."""

visibility("//build/kernel/kleaf/...")

# List of images produced by non-x86 kernels.
DEFAULT_IMAGES = [
    "Image",
    "Image.lz4",
    "Image.gz",
]

# List of output files from gki_artifacts()
GKI_ARTIFACTS_AARCH64_OUTS = [
    "boot-img.tar.gz",
    "gki-info.txt",
] + [
    "boot.img" if e == "Image" else "boot-{}.img".format(e[len("Image."):])
    for e in DEFAULT_IMAGES
]

SYSTEM_DLKM_COMMON_OUTS = [
    "system_dlkm_staging_archive.tar.gz",
    "system_dlkm.modules.load",
    "system_dlkm.modules.blocklist",
]

MODULES_STAGING_ARCHIVE = "modules_staging_dir.tar.gz"

MODULE_ENV_ARCHIVE_SUFFIX = "_module_env.tar.gz"

UNSTRIPPED_MODULES_ARCHIVE = "unstripped_modules.tar.gz"

# Archive emitted by kernel_build that contains the kernel_filegroup
# definition and extra files.
FILEGROUP_DEF_BUILD_FRAGMENT_NAME = "filegroup_decl_build_frag.txt"
FILEGROUP_DEF_ARCHIVE_SUFFIX = "_filegroup_decl.tar.gz"
