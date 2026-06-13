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
Build vendor_dlkm.img for vendor modules.
"""

load("@bazel_skylib//lib:shell.bzl", "shell")
load(
    ":common_providers.bzl",
    "ImagesInfo",
)
load(
    ":image/image_utils.bzl",
    "SYSTEM_DLKM_MODULES_LOAD_NAME",
    "SYSTEM_DLKM_STAGING_ARCHIVE_NAME",
    "VENDOR_DLKM_STAGING_ARCHIVE_NAME",
    "image_utils",
)
load(":utils.bzl", "utils")

visibility("//build/kernel/kleaf/...")

def _vendor_dlkm_image_impl(ctx):
    vendor_dlkm_img = ctx.actions.declare_file("{}/vendor_dlkm.img".format(ctx.label.name))
    vendor_dlkm_modules_load = ctx.actions.declare_file("{}/vendor_dlkm.modules.load".format(ctx.label.name))
    vendor_dlkm_modules_blocklist = ctx.actions.declare_file("{}/vendor_dlkm.modules.blocklist".format(ctx.label.name))
    modules_staging_dir = vendor_dlkm_img.dirname + "/staging"
    vendor_dlkm_staging_dir = modules_staging_dir + "/vendor_dlkm_staging"
    vendor_dlkm_fs_type = ctx.attr.vendor_dlkm_fs_type
    vendor_dlkm_etc_files = " ".join([f.path for f in ctx.files.vendor_dlkm_etc_files])

    vendor_dlkm_staging_archive = None
    if ctx.attr.vendor_dlkm_archive:
        vendor_dlkm_staging_archive = ctx.actions.declare_file("{}/{}".format(ctx.label.name, VENDOR_DLKM_STAGING_ARCHIVE_NAME))

    command = ""
    additional_inputs = []
    if ctx.file.vendor_boot_modules_load:
        command += """
                # Restore vendor_boot.modules.load or vendor_kernel_boot.modules.load
                # to modules.load, where build_utils.sh build_vendor_dlkm uses
                  cat {vendor_boot_modules_load} >> ${{DIST_DIR}}/modules.load
        """.format(
            vendor_boot_modules_load = ctx.file.vendor_boot_modules_load.path,
        )
        additional_inputs.append(ctx.file.vendor_boot_modules_load)

    exclude_system_dlkm_step = _exclude_system_dlkm(
        ctx,
        modules_staging_dir = modules_staging_dir,
    )
    command += exclude_system_dlkm_step.cmd
    additional_inputs += exclude_system_dlkm_step.inputs

    outputs = []
    vendor_dlkm_flatten_img = None
    vendor_dlkm_flatten_img_name = None
    if ctx.attr.build_vendor_dlkm_flatten_image:
        vendor_dlkm_flatten_img = ctx.actions.declare_file("{}/vendor_dlkm.flatten.img".format(ctx.label.name))
        outputs.append(vendor_dlkm_flatten_img)
        vendor_dlkm_flatten_img_name = "vendor_dlkm.flatten.img"

    command += """
            # Use `strip_modules` intead of relying on this.
               unset DO_NOT_STRIP_MODULES
            # Build vendor_dlkm
              mkdir -p {vendor_dlkm_staging_dir}
              (
                MODULES_STAGING_DIR={modules_staging_dir}
                VENDOR_DLKM_ETC_FILES={quoted_vendor_dlkm_etc_files}
                VENDOR_DLKM_FS_TYPE={vendor_dlkm_fs_type}
                VENDOR_DLKM_STAGING_DIR={vendor_dlkm_staging_dir}
                VENDOR_DLKM_GEN_FLATTEN_IMAGE={build_flatten_image}
                build_vendor_dlkm {vendor_dlkm_archive}
              )
            # Move output files into place
              mv "${{DIST_DIR}}/vendor_dlkm.img" {vendor_dlkm_img}
               if [[ {build_flatten_image} == "1" ]]; then
                mv "${{DIST_DIR}}/{vendor_dlkm_flatten_img_name}" {vendor_dlkm_flatten_img}
               fi
              mv "${{DIST_DIR}}/vendor_dlkm.modules.load" {vendor_dlkm_modules_load}
              if [[ -f "${{DIST_DIR}}/vendor_dlkm_staging_archive.tar.gz" ]]; then
                mv "${{DIST_DIR}}/vendor_dlkm_staging_archive.tar.gz" {vendor_dlkm_staging_archive}
              fi
              if [[ -f "${{DIST_DIR}}/vendor_dlkm.modules.blocklist" ]]; then
                mv "${{DIST_DIR}}/vendor_dlkm.modules.blocklist" {vendor_dlkm_modules_blocklist}
              else
                : > {vendor_dlkm_modules_blocklist}
              fi
            # Remove staging directories
              rm -rf {vendor_dlkm_staging_dir}
    """.format(
        build_flatten_image = int(ctx.attr.build_vendor_dlkm_flatten_image),
        modules_staging_dir = modules_staging_dir,
        quoted_vendor_dlkm_etc_files = shell.quote(vendor_dlkm_etc_files),
        vendor_dlkm_fs_type = vendor_dlkm_fs_type,
        vendor_dlkm_staging_dir = vendor_dlkm_staging_dir,
        vendor_dlkm_flatten_img = vendor_dlkm_flatten_img.path if vendor_dlkm_flatten_img else "/dev/null",
        vendor_dlkm_flatten_img_name = vendor_dlkm_flatten_img_name,
        vendor_dlkm_img = vendor_dlkm_img.path,
        vendor_dlkm_modules_load = vendor_dlkm_modules_load.path,
        vendor_dlkm_modules_blocklist = vendor_dlkm_modules_blocklist.path,
        vendor_dlkm_archive = "1" if ctx.attr.vendor_dlkm_archive else "",
        vendor_dlkm_staging_archive = vendor_dlkm_staging_archive.path if ctx.attr.vendor_dlkm_archive else None,
    )

    additional_inputs += ctx.files.vendor_dlkm_etc_files

    outputs += [
        vendor_dlkm_img,
        vendor_dlkm_modules_load,
        vendor_dlkm_modules_blocklist,
    ]

    if ctx.attr.vendor_dlkm_archive:
        outputs.append(vendor_dlkm_staging_archive)

    default_info = image_utils.build_modules_image_impl_common(
        ctx = ctx,
        what = "vendor_dlkm",
        outputs = outputs,
        build_command = command,
        modules_staging_dir = modules_staging_dir,
        set_ext_modules = True,
        additional_inputs = additional_inputs,
        mnemonic = "VendorDlkmImage",
    )

    images_info = ImagesInfo(files_dict = {
        vendor_dlkm_img.basename: depset([vendor_dlkm_img]),
    })

    return [
        default_info,
        images_info,
    ]

def _exclude_system_dlkm(ctx, modules_staging_dir):
    if not ctx.attr.dedup_dlkm_modules:
        return struct(cmd = "", inputs = [])

    inputs = []

    if ctx.attr.system_dlkm_image:
        system_dlkm_files = ctx.files.system_dlkm_image
        src_attr = "system_dlkm_image"
    elif ctx.attr.base_kernel_images:
        system_dlkm_files = ctx.files.base_kernel_images
        src_attr = "base_kernel_images"
    else:
        fail("{}: With dedup_dlkm_modules, either build_system_dlkm or base_kernel_images must be set".format(
            ctx.label,
        ))

    system_dlkm_staging_archive = utils.find_file(
        name = SYSTEM_DLKM_STAGING_ARCHIVE_NAME,
        files = system_dlkm_files,
        what = "{} ({} for {})".format(ctx.attr.base_kernel_images.label, src_attr, ctx.label),
        required = True,
    )
    system_dlkm_modules_load = utils.find_file(
        name = SYSTEM_DLKM_MODULES_LOAD_NAME,
        files = system_dlkm_files,
        what = "{} ({} for {})".format(ctx.attr.base_kernel_images.label, src_attr, ctx.label),
        required = True,
    )
    inputs += [system_dlkm_staging_archive, system_dlkm_modules_load]

    cmd = """
            # Extract modules from system_dlkm staging archive for depmod
              mkdir -p {modules_staging_dir}
              tar xf {system_dlkm_staging_archive} --wildcards -C {modules_staging_dir} '*.ko'
            # Ensure system_dlkm modules aren't loaded
              cat {system_dlkm_modules_load} >> ${{DIST_DIR}}/modules.load
    """.format(
        system_dlkm_staging_archive = system_dlkm_staging_archive.path,
        modules_staging_dir = modules_staging_dir,
        system_dlkm_modules_load = system_dlkm_modules_load.path,
    )

    return struct(cmd = cmd, inputs = inputs)

vendor_dlkm_image = rule(
    implementation = _vendor_dlkm_image_impl,
    doc = """Build vendor_dlkm image.

Execute `build_vendor_dlkm` in `build_utils.sh`.

When included in a `copy_to_dist_dir` rule, this rule copies the following to `DIST_DIR`:
- `vendor_dlkm.img`
- `vendor_dlkm_flatten.img` if build_vendor_dlkm_flatten is True
""",
    attrs = image_utils.build_modules_image_attrs_common({
        "build_vendor_dlkm_flatten_image": attr.bool(
            default = False,
            doc = "When True it builds vendor_dlkm image with no `uname -r` in the path",
        ),
        "vendor_boot_modules_load": attr.label(
            allow_single_file = True,
            doc = """File to `vendor_boot.modules.load`.

Modules listed in this file is stripped away from the `vendor_dlkm` image.""",
        ),
        "vendor_dlkm_archive": attr.bool(doc = "Whether to archive the `vendor_dlkm` modules"),
        "vendor_dlkm_fs_type": attr.string(doc = """vendor_dlkm.img fs type""", values = ["ext4", "erofs"]),
        "vendor_dlkm_modules_list": attr.label(allow_single_file = True),
        "vendor_dlkm_etc_files": attr.label_list(allow_files = True),
        "vendor_dlkm_modules_blocklist": attr.label(allow_single_file = True),
        "vendor_dlkm_props": attr.label(allow_single_file = True),
        "dedup_dlkm_modules": attr.bool(doc = "Whether to exclude `system_dlkm` modules"),
        "system_dlkm_image": attr.label(),
        "base_kernel_images": attr.label(allow_files = True),
    }),
)
