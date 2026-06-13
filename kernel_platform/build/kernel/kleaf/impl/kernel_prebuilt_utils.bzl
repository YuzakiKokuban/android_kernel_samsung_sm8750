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

"""Utilities to define a repository for kernel prebuilts."""

load(
    "//build/kernel/kleaf:constants.bzl",
    "DEFAULT_GKI_OUTS",
)
load(
    ":constants.bzl",
    "FILEGROUP_DEF_ARCHIVE_SUFFIX",
    "GKI_ARTIFACTS_AARCH64_OUTS",
    "SYSTEM_DLKM_COMMON_OUTS",
    "UNSTRIPPED_MODULES_ARCHIVE",
)

visibility("//build/kernel/kleaf/...")

# Key: Bazel target name in common_kernels.bzl
# repo_name: name of download_artifacts_repo in bazel.WORKSPACE
# outs: list of outs associated with that target name
# arch: Architecture associated with this mapping.
CI_TARGET_MAPPING = {
    # TODO(b/206079661): Allow downloaded prebuilts for x86_64 and debug targets.
    "kernel_aarch64": {
        "arch": "arm64",
        # TODO: Rename this when more architectures are added.
        "repo_name": "gki_prebuilts",
        # Key: local file name.
        "download_configs": {
            "kernel-uapi-headers.tar.gz": {
                "target_suffix": "uapi_headers",
                "mandatory": True,
                "remote_filename_fmt": "kernel-uapi-headers.tar.gz",
            },
            UNSTRIPPED_MODULES_ARCHIVE: {
                "target_suffix": "unstripped_modules_archive",
                "mandatory": True,
                "remote_filename_fmt": UNSTRIPPED_MODULES_ARCHIVE,
            },
            "kernel-headers.tar.gz": {
                "target_suffix": "headers",
                "mandatory": True,
                "remote_filename_fmt": "kernel-headers.tar.gz",
            },
            "boot-img.tar.gz": {
                "target_suffix": "boot_img_archive",
                "mandatory": True,
                "remote_filename_fmt": "boot-img.tar.gz",
                # The others can be found by extracting the archive, see gki_artifacts_prebuilts
            },
            "signed/boot-img.tar.gz": {
                "target_suffix": "boot_img_archive_signed",
                # Do not fail immediately if this file cannot be downloaded, because it does not
                # exist for unsigned builds. A build error will be emitted by gki_artifacts_prebuilts
                # if --use_signed_prebuilts and --use_gki_prebuilts=<an unsigned build number>.
                "mandatory": False,
                # The basename is kept boot-img.tar.gz so it works with
                # gki_artifacts_prebuilts. It is placed under the signed/
                # directory to avoid conflicts with boot_img_archive in
                # download_artifacts_repo.
                # The others can be found by extracting the archive, see gki_artifacts_prebuilts
                "remote_filename_fmt": "signed/certified-boot-img-{build_number}.tar.gz",
            },
            "kernel_aarch64" + FILEGROUP_DEF_ARCHIVE_SUFFIX: {
                "target_suffix": "ddk_artifacts",
                "mandatory": True,
                "remote_filename_fmt": "kernel_aarch64" + FILEGROUP_DEF_ARCHIVE_SUFFIX,
            },
            "abi_symbollist": {
                "target_suffix": "kmi_symbol_list",
                "mandatory": False,
                "remote_filename_fmt": "abi_symbollist",
            },
            "abi_symbollist.report": {
                "target_suffix": "kmi_symbol_list",
                "mandatory": False,
                "remote_filename_fmt": "abi_symbollist.report",
            },
            "gki_aarch64_protected_modules": {
                "target_suffix": "protected_modules",
                "mandatory": False,
                "remote_filename_fmt": "gki_aarch64_protected_modules",
            },
        } | {
            item: {
                "target_suffix": "files",
                "mandatory": True,
                "remote_filename_fmt": item,
            }
            for item in DEFAULT_GKI_OUTS
        } | {
            item: {
                "target_suffix": "images",
                "mandatory": True,
                "remote_filename_fmt": item,
            }
            for item in SYSTEM_DLKM_COMMON_OUTS
        } | {
            item: {
                "target_suffix": "gki_prebuilts_outs",
                "mandatory": True,
                "remote_filename_fmt": item,
            }
            for item in GKI_ARTIFACTS_AARCH64_OUTS
        } | {
            # TODO(b/328770706): download_configs.json should be a proper rule to
            # get the name of the file from :kernel_aarch64_ddk_headers_archive
            "kernel_aarch64_ddk_headers_archive.tar.gz": {
                "target_suffix": "init_ddk_files",
                "mandatory": True,
                "remote_filename_fmt": "kernel_aarch64_ddk_headers_archive.tar.gz",
            },
            "build.config.constants": {
                "target_suffix": "init_ddk_files",
                "mandatory": True,
                "remote_filename_fmt": "build.config.constants",
            },
            "manifest.xml": {
                "target_suffix": "init_ddk_files",
                "mandatory": False,
                "remote_filename_fmt": "manifest_{build_number}.xml",
            },
        },
    },
}
