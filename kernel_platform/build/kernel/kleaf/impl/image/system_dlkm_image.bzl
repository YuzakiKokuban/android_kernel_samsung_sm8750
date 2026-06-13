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
Build system_dlkm image for GKI modules.
"""

load("//build/kernel/kleaf/impl:constants.bzl", "SYSTEM_DLKM_COMMON_OUTS")
load("//build/kernel/kleaf/impl:utils.bzl", "utils")
load(
    ":common_providers.bzl",
    "ImagesInfo",
    "KernelModuleInfo",
)
load(
    ":image/image_utils.bzl",
    "image_utils",
    _MODULES_LOAD_NAME = "SYSTEM_DLKM_MODULES_LOAD_NAME",
    _STAGING_ARCHIVE_NAME = "SYSTEM_DLKM_STAGING_ARCHIVE_NAME",
)

visibility("//build/kernel/kleaf/...")

def _system_dlkm_image_impl(ctx):
    system_dlkm_modules_load = ctx.actions.declare_file("{}/{}".format(ctx.label.name, _MODULES_LOAD_NAME))
    system_dlkm_staging_archive = ctx.actions.declare_file("{}/{}".format(ctx.label.name, _STAGING_ARCHIVE_NAME))
    system_dlkm_modules_blocklist = ctx.actions.declare_file("{}/system_dlkm.modules.blocklist".format(ctx.label.name))

    modules_staging_dir = system_dlkm_staging_archive.dirname + "/staging"
    system_dlkm_staging_dir = modules_staging_dir + "/system_dlkm_staging"

    system_dlkm_fs_type = ctx.attr.system_dlkm_fs_type  # str | None
    system_dlkm_fs_types = ctx.attr.system_dlkm_fs_types  # list[str]

    if system_dlkm_fs_type and system_dlkm_fs_types:
        fail("""{}: Both system_dlkm_fs_type="{}" and system_dlkm_fs_types={} are specified. system_dlkm_fs_type is deprecated, use system_dlkm_fs_types instead.""".format(ctx.label, system_dlkm_fs_type, system_dlkm_fs_types))

    # Build system_dlkm.img with ext4 fs as default
    if not system_dlkm_fs_type and not system_dlkm_fs_types:
        system_dlkm_fs_type = "ext4"

    # if system_dlkm_fs_type: Build system_dlkm.img with given fs type
    if system_dlkm_fs_type:
        system_dlkm_fs_types = [system_dlkm_fs_type]

    additional_inputs = []
    restore_modules_install = True
    extract_staging_archive_cmd = ""
    extra_flags_cmd = ""

    kernel_build_infos = ctx.attr.kernel_modules_install[KernelModuleInfo].kernel_build_infos
    if kernel_build_infos.images_info.base_kernel_label != None:
        if ctx.attr.base_kernel_images == None:
            fail("""{this_label}: Building device-specific system_dlkm ({kernel_build} has base_kernel {base_kernel_label}), but base_kernel_images is not set. Perhaps add the following?
    base_kernel_images = "{base_kernel_label}_images"
                 """.format(
                this_label = ctx.label,
                kernel_build = kernel_build_infos.label,
                base_kernel_label = kernel_build_infos.images_info.base_kernel_label,
            ))

        # When building device-specific system_dlkm against GKI's
        # system_dlkm_staging_archive.tar.gz, do not restore the modules_install archive from
        # the device build.
        restore_modules_install = False
        base_kernel_system_dlkm_staging_archive = utils.find_file(
            name = _STAGING_ARCHIVE_NAME,
            files = ctx.files.base_kernel_images,
            what = "{} (images for {})".format(ctx.attr.base_kernel_images.label, ctx.label),
            required = True,
        )
        additional_inputs.append(base_kernel_system_dlkm_staging_archive)

        extract_staging_archive_cmd = """
                # Extract staging archive
                  mkdir -p {modules_staging_dir}
                  tar xf {base_kernel_system_dlkm_staging_archive} -C {modules_staging_dir}
        """.format(
            base_kernel_system_dlkm_staging_archive = base_kernel_system_dlkm_staging_archive.path,
            modules_staging_dir = modules_staging_dir,
            system_dlkm_staging_dir = system_dlkm_staging_dir,
        )

        extra_flags_cmd = """
                     # Trick create_modules_staging to not strip, because they are already stripped and signed
                       DO_NOT_STRIP_MODULES=
                     # Trick create_modules_staging to not look at external modules. They aren't related.
                       EXT_MODULES=
                       EXT_MODULES_MAKEFILE=
                     # Tell build_system_dlkm to not sign, because they are already signed and stripped
                       SYSTEM_DLKM_RE_SIGN=0
        """

    command = ""
    outputs = []
    outputs_to_compare = []
    for fs_type in system_dlkm_fs_types:
        if system_dlkm_fs_type:
            system_dlkm_img = ctx.actions.declare_file("{}/system_dlkm.img".format(ctx.label.name))
            system_dlkm_img_name = "system_dlkm.img"
        else:
            system_dlkm_img = ctx.actions.declare_file("{}/system_dlkm.{}.img".format(ctx.label.name, fs_type))
            system_dlkm_img_name = "system_dlkm.{}.img".format(fs_type)
        outputs.append(system_dlkm_img)
        outputs_to_compare.append(system_dlkm_img_name)

        system_dlkm_flatten_img = None
        system_dlkm_flatten_img_name = None
        if ctx.attr.build_system_dlkm_flatten_image:
            system_dlkm_flatten_img = ctx.actions.declare_file("{}/system_dlkm.flatten.{}.img".format(ctx.label.name, fs_type))
            outputs.append(system_dlkm_flatten_img)
            system_dlkm_flatten_img_name = "system_dlkm.flatten.{}.img".format(fs_type)
            outputs_to_compare.append(system_dlkm_flatten_img_name)

        command += """
                   {extract_staging_archive_cmd}
                 # Build {system_dlkm_img_name}
                   mkdir -p {system_dlkm_staging_dir}
                   (
                     MODULES_STAGING_DIR={modules_staging_dir}
                     SYSTEM_DLKM_FS_TYPE={system_dlkm_fs_type}
                     SYSTEM_DLKM_STAGING_DIR={system_dlkm_staging_dir}
                     SYSTEM_DLKM_IMAGE_NAME={system_dlkm_img_name}
                     SYSTEM_DLKM_GEN_FLATTEN_IMAGE={build_flatten_image}
                     {extra_flags_cmd}
                     build_system_dlkm
                   )
                 # Move output files into place
                   mv "${{DIST_DIR}}/{system_dlkm_img_name}" {system_dlkm_img}
                   if [[ {build_flatten_image} == "1" ]]; then
                     mv "${{DIST_DIR}}/{system_dlkm_flatten_img_name}" {system_dlkm_flatten_img}
                   fi
                   mv "${{DIST_DIR}}/system_dlkm.modules.load" {system_dlkm_modules_load}
                   mv "${{DIST_DIR}}/system_dlkm_staging_archive.tar.gz" {system_dlkm_staging_archive}
                   if [ -f "${{DIST_DIR}}/system_dlkm.modules.blocklist" ]; then
                     mv "${{DIST_DIR}}/system_dlkm.modules.blocklist" {system_dlkm_modules_blocklist}
                   else
                     : > {system_dlkm_modules_blocklist}
                   fi

                 # Remove staging directories
                   rm -rf {system_dlkm_staging_dir}
        """.format(
            build_flatten_image = int(ctx.attr.build_system_dlkm_flatten_image),
            extract_staging_archive_cmd = extract_staging_archive_cmd,
            extra_flags_cmd = extra_flags_cmd,
            modules_staging_dir = modules_staging_dir,
            system_dlkm_fs_type = fs_type,
            system_dlkm_staging_dir = system_dlkm_staging_dir,
            system_dlkm_flatten_img = system_dlkm_flatten_img.path if system_dlkm_flatten_img else "/dev/null",
            system_dlkm_flatten_img_name = system_dlkm_flatten_img_name,
            system_dlkm_img = system_dlkm_img.path,
            system_dlkm_img_name = system_dlkm_img_name,
            system_dlkm_modules_load = system_dlkm_modules_load.path,
            system_dlkm_staging_archive = system_dlkm_staging_archive.path,
            system_dlkm_modules_blocklist = system_dlkm_modules_blocklist.path,
        )

    outputs += [
        system_dlkm_img,
        system_dlkm_modules_load,
        system_dlkm_staging_archive,
        system_dlkm_modules_blocklist,
    ]

    default_info = image_utils.build_modules_image_impl_common(
        ctx = ctx,
        what = "system_dlkm",
        outputs = outputs,
        additional_inputs = additional_inputs,
        restore_modules_install = restore_modules_install,
        build_command = command,
        modules_staging_dir = modules_staging_dir,
        mnemonic = "SystemDlkmImage",
    )

    utils.compare_file_names(
        default_info.files.to_list(),
        SYSTEM_DLKM_COMMON_OUTS + outputs_to_compare,
        what = "{}: Internal error: not producing the expected list of outputs".format(ctx.label),
    )

    images_info = ImagesInfo(files_dict = {
        file.basename: depset([file])
        for file in outputs
        if file.extension == "img"
    })

    return [
        default_info,
        images_info,
    ]

system_dlkm_image = rule(
    implementation = _system_dlkm_image_impl,
    doc = """Build system_dlkm partition image with signed GKI modules.

When included in a `copy_to_dist_dir` rule, this rule copies the following to `DIST_DIR`:
- `system_dlkm.img` if system_dlkm_fs_type is specified
- `system_dlkm.[erofs|ext4].img` if system_dlkm_fs_types is specified
- `system_dlkm.flatten.[erofs|ext4].img` if build_system_dlkm_flatten_image is True
- `system_dlkm.modules.load`

""",
    attrs = image_utils.build_modules_image_attrs_common({
        "base_kernel_images": attr.label(allow_files = True),
        "build_system_dlkm_flatten_image": attr.bool(
            default = False,
            doc = "When True it builds system_dlkm image with no `uname -r` in the path.",
        ),
        "modules_list": attr.label(allow_single_file = True),
        "modules_blocklist": attr.label(allow_single_file = True),
        "system_dlkm_fs_type": attr.string(doc = """Deprecated. system_dlkm image fs type""", values = ["ext4", "erofs"]),
        "system_dlkm_fs_types": attr.string_list(doc = """system_dlkm image fs types""", allow_empty = True),
        "system_dlkm_modules_list": attr.label(allow_single_file = True),
        "system_dlkm_modules_blocklist": attr.label(allow_single_file = True),
        "system_dlkm_props": attr.label(allow_single_file = True),
    }),
)
