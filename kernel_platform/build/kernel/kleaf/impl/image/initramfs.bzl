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
Build initramfs.
"""

load(":image/image_utils.bzl", "image_utils")

visibility("//build/kernel/kleaf/...")

InitramfsInfo = provider(
    doc = "Provides information about initramfs outputs.",
    fields = {
        "initramfs_img": "Output image",
        "initramfs_staging_archive": "Archive of initramfs staging directory",
    },
)

def _initramfs_impl(ctx):
    initramfs_img = ctx.actions.declare_file("{}/initramfs.img".format(ctx.label.name))
    modules_load = ctx.actions.declare_file("{}/modules.load".format(ctx.label.name))
    vendor_boot_modules_load = ctx.outputs.vendor_boot_modules_load
    initramfs_staging_archive = ctx.actions.declare_file("{}/initramfs_staging_archive.tar.gz".format(ctx.label.name))

    outputs = [
        initramfs_img,
        modules_load,
    ]
    cp_vendor_boot_modules_load_cmd = ""
    if vendor_boot_modules_load:
        cp_vendor_boot_modules_load_cmd = """
               cp ${{modules_root_dir}}/modules.load {vendor_boot_modules_load}
        """.format(
            vendor_boot_modules_load = vendor_boot_modules_load.path,
        )
        outputs.append(vendor_boot_modules_load)

    cp_modules_load_recovery_cmd = ""
    if ctx.attr.modules_recovery_list:
        modules_load_recovery = ctx.actions.declare_file("{}/modules.load.recovery".format(ctx.label.name))
        cp_modules_load_recovery_cmd = """
               cp ${{modules_root_dir}}/modules.load.recovery {modules_load_recovery}
        """.format(
            modules_load_recovery = modules_load_recovery.path,
        )
        outputs.append(modules_load_recovery)

    cp_vendor_boot_modules_load_recovery_cmd = ""
    vendor_boot_modules_load_recovery = ctx.outputs.vendor_boot_modules_load_recovery
    if vendor_boot_modules_load_recovery:
        cp_vendor_boot_modules_load_recovery_cmd = """
               cp ${{modules_root_dir}}/modules.load.recovery {vendor_boot_modules_load_recovery}
        """.format(
            vendor_boot_modules_load_recovery = vendor_boot_modules_load_recovery.path,
        )
        outputs.append(vendor_boot_modules_load_recovery)

    cp_modules_load_charger_cmd = ""
    if ctx.attr.modules_charger_list:
        modules_load_charger = ctx.actions.declare_file("{}/modules.load.charger".format(ctx.label.name))
        cp_modules_load_charger_cmd = """
               cp ${{modules_root_dir}}/modules.load.charger {modules_load_charger}
        """.format(
            modules_load_charger = modules_load_charger.path,
        )
        outputs.append(modules_load_charger)

    cp_vendor_boot_modules_load_charger_cmd = ""
    vendor_boot_modules_load_charger = ctx.outputs.vendor_boot_modules_load_charger
    if vendor_boot_modules_load_charger:
        cp_vendor_boot_modules_load_charger_cmd = """
               cp ${{modules_root_dir}}/modules.load.charger {vendor_boot_modules_load_charger}
        """.format(
            vendor_boot_modules_load_charger = vendor_boot_modules_load_charger.path,
        )
        outputs.append(vendor_boot_modules_load_charger)

    modules_staging_dir = initramfs_img.dirname + "/staging"
    initramfs_staging_dir = modules_staging_dir + "/initramfs_staging"

    additional_inputs = []
    if ctx.file.modules_options:
        cp_modules_options_cmd = """
            cp {modules_options} ${{modules_root_dir}}/modules.options
    """.format(
            modules_options = ctx.file.modules_options.path,
        )
        additional_inputs.append(ctx.file.modules_options)
    else:
        cp_modules_options_cmd = """
            : > ${modules_root_dir}/modules.options
    """

    ramdisk_compress = image_utils.ramdisk_options(
        ramdisk_compression = ctx.attr.ramdisk_compression,
        ramdisk_compression_args = ctx.attr.ramdisk_compression_args,
    ).ramdisk_compress

    command = """
             # Use `strip_modules` intead of relying on this.
               unset DO_NOT_STRIP_MODULES
               mkdir -p {initramfs_staging_dir}
             # Build initramfs
               create_modules_staging "${{MODULES_LIST}}" {modules_staging_dir} \
                       {initramfs_staging_dir} "${{MODULES_BLOCKLIST}}" \
                       "${{MODULES_RECOVERY_LIST:-""}}" "${{MODULES_CHARGER_LIST:-""}}" "-e"
               modules_root_dir=$(readlink -e {initramfs_staging_dir}/lib/modules/*) || exit 1
               cp ${{modules_root_dir}}/modules.load {modules_load}
               {cp_vendor_boot_modules_load_cmd}
               {cp_modules_load_recovery_cmd}
               {cp_vendor_boot_modules_load_recovery_cmd}
               {cp_modules_load_charger_cmd}
               {cp_vendor_boot_modules_load_charger_cmd}
               {cp_modules_options_cmd}
               mkbootfs "{initramfs_staging_dir}" >"{modules_staging_dir}/initramfs.cpio"
               {ramdisk_compress} "{modules_staging_dir}/initramfs.cpio" >"{initramfs_img}"
             # Archive initramfs_staging_dir
               tar czf {initramfs_staging_archive} -C {initramfs_staging_dir} .
             # Remove staging directories
               rm -rf {initramfs_staging_dir}
    """.format(
        modules_staging_dir = modules_staging_dir,
        initramfs_staging_dir = initramfs_staging_dir,
        ramdisk_compress = ramdisk_compress,
        modules_load = modules_load.path,
        initramfs_img = initramfs_img.path,
        initramfs_staging_archive = initramfs_staging_archive.path,
        cp_vendor_boot_modules_load_cmd = cp_vendor_boot_modules_load_cmd,
        cp_modules_load_recovery_cmd = cp_modules_load_recovery_cmd,
        cp_vendor_boot_modules_load_recovery_cmd = cp_vendor_boot_modules_load_recovery_cmd,
        cp_modules_load_charger_cmd = cp_modules_load_charger_cmd,
        cp_vendor_boot_modules_load_charger_cmd = cp_vendor_boot_modules_load_charger_cmd,
        cp_modules_options_cmd = cp_modules_options_cmd,
    )

    default_info = image_utils.build_modules_image_impl_common(
        ctx = ctx,
        what = "initramfs",
        outputs = outputs,
        build_command = command,
        modules_staging_dir = modules_staging_dir,
        set_ext_modules = True,
        implicit_outputs = [
            initramfs_staging_archive,
        ],
        additional_inputs = additional_inputs,
        mnemonic = "Initramfs",
    )
    return [
        default_info,
        InitramfsInfo(
            initramfs_img = initramfs_img,
            initramfs_staging_archive = initramfs_staging_archive,
        ),
    ]

initramfs = rule(
    implementation = _initramfs_impl,
    doc = """Build initramfs.

When included in a `copy_to_dist_dir` rule, this rule copies the following to `DIST_DIR`:
- `initramfs.img`
- `modules.load`
- `modules.load.recovery`
- `modules.load.charger`
- `vendor_boot.modules.load`
- `vendor_boot.modules.load.recovery`
- `vendor_boot.modules.load.charger`

An additional label, `{name}/vendor_boot.modules.load`, is declared to point to the
corresponding files.
""",
    attrs = image_utils.build_modules_image_attrs_common({
        "vendor_boot_modules_load": attr.output(
            doc = "`vendor_boot.modules.load` or `vendor_kernel_boot.modules.load`",
        ),
        "vendor_boot_modules_load_recovery": attr.output(
            doc = "`vendor_boot.modules.load.recovery` or `vendor_kernel_boot.modules.load.recovery`",
        ),
        "vendor_boot_modules_load_charger": attr.output(
            doc = "`vendor_boot.modules.load.charger` or `vendor_kernel_boot.modules.load.charger`",
        ),
        "modules_list": attr.label(allow_single_file = True),
        "modules_recovery_list": attr.label(allow_single_file = True),
        "modules_charger_list": attr.label(allow_single_file = True),
        "modules_blocklist": attr.label(allow_single_file = True),
        "modules_options": attr.label(allow_single_file = True),
        "ramdisk_compression": attr.string(
            doc = "If provided it specfies the format used for any ramdisks generated." +
                  "If not provided a fallback value from build.config is used.",
            values = ["lz4", "gzip"],
        ),
        "ramdisk_compression_args": attr.string(
            doc = "Command line arguments passed only to lz4 command to control compression level.",
        ),
    }),
)
