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
Rules for building boot images.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load(":common_providers.bzl", "KernelBuildInfo", "KernelSerializedEnvInfo")
load(":debug.bzl", "debug")
load(":image/image_utils.bzl", "image_utils")
load(":image/initramfs.bzl", "InitramfsInfo")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

def _boot_images_impl(ctx):
    ## Declare implicit outputs of the command
    ## This is like ctx.actions.declare_directory(ctx.label.name) without actually declaring it.
    outdir_short = paths.join(
        ctx.label.workspace_root,
        ctx.label.package,
        ctx.label.name,
    )
    outdir = paths.join(
        ctx.bin_dir.path,
        outdir_short,
    )
    modules_staging_dir = outdir + "/staging"
    mkbootimg_staging_dir = modules_staging_dir + "/mkbootimg_staging"

    # Initialized conditionally below.
    initramfs_staging_archive = None
    initramfs_staging_dir = None

    if ctx.attr.initramfs:
        initramfs_staging_archive = ctx.attr.initramfs[InitramfsInfo].initramfs_staging_archive
        initramfs_staging_dir = modules_staging_dir + "/initramfs_staging"

    outs = []
    for out in ctx.outputs.outs:
        outs.append(out.short_path[len(outdir_short) + 1:])

    kernel_build_outs = depset(transitive = [
        ctx.attr.kernel_build[KernelBuildInfo].outs,
        ctx.attr.kernel_build[KernelBuildInfo].base_kernel_files,
    ])

    inputs = [
        ctx.file.mkbootimg,
    ]
    if ctx.attr.initramfs:
        inputs += [
            ctx.attr.initramfs[InitramfsInfo].initramfs_img,
            initramfs_staging_archive,
        ]
    inputs += ctx.files.deps
    inputs += ctx.files.vendor_ramdisk_binaries
    inputs += ctx.files.vendor_ramdisk_dev_nodes
    if ctx.attr.gki_ramdisk_prebuilt_binary:
        inputs += [ctx.file.gki_ramdisk_prebuilt_binary]

    transitive_inputs = [
        kernel_build_outs,
        ctx.attr.kernel_build[KernelSerializedEnvInfo].inputs,
    ]

    tools = [ctx.executable._search_and_cp_output]
    transitive_tools = [ctx.attr.kernel_build[KernelSerializedEnvInfo].tools]

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ctx.attr.kernel_build[KernelSerializedEnvInfo],
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )

    command += """
        MKBOOTIMG_PATH={mkbootimg}
    """.format(mkbootimg = ctx.file.mkbootimg.path)

    if ctx.attr.build_boot:
        boot_flag_cmd = "BUILD_BOOT_IMG=1"
    else:
        boot_flag_cmd = "BUILD_BOOT_IMG="

    if not ctx.attr.vendor_boot_name:
        vendor_boot_flag_cmd = """
            BUILD_VENDOR_BOOT_IMG=
            SKIP_VENDOR_BOOT=1
            BUILD_VENDOR_KERNEL_BOOT=
        """
    elif ctx.attr.vendor_boot_name == "vendor_boot":
        vendor_boot_flag_cmd = """
            BUILD_VENDOR_BOOT_IMG=1
            SKIP_VENDOR_BOOT=
            BUILD_VENDOR_KERNEL_BOOT=
        """
    elif ctx.attr.vendor_boot_name == "vendor_kernel_boot":
        vendor_boot_flag_cmd = """
            BUILD_VENDOR_BOOT_IMG=1
            SKIP_VENDOR_BOOT=
            BUILD_VENDOR_KERNEL_BOOT=1
        """
    else:
        fail("{}: unknown vendor_boot_name {}".format(ctx.label, ctx.attr.vendor_boot_name))

    if ctx.files.vendor_ramdisk_binaries:
        # build_utils.sh uses singular VENDOR_RAMDISK_BINARY
        command += """
            VENDOR_RAMDISK_BINARY="{vendor_ramdisk_binaries}"
        """.format(
            vendor_ramdisk_binaries = " ".join([file.path for file in ctx.files.vendor_ramdisk_binaries]),
        )

    if ctx.attr.gki_ramdisk_prebuilt_binary:
        command += """
            GKI_RAMDISK_PREBUILT_BINARY="{gki_ramdisk_prebuilt_binary}"
        """.format(gki_ramdisk_prebuilt_binary = ctx.file.gki_ramdisk_prebuilt_binary.path)

    if ctx.files.vendor_ramdisk_dev_nodes:
        command += """
            VENDOR_RAMDISK_DEV_NODES="{vendor_ramdisk_dev_nodes}"
        """.format(
            vendor_ramdisk_dev_nodes = " ".join([file.path for file in ctx.files.vendor_ramdisk_dev_nodes]),
        )

    command += """
             # Create and restore DIST_DIR.
             # We don't need all of *_for_dist. Copying all declared outputs of kernel_build is
             # sufficient.
               mkdir -p ${{DIST_DIR}}
               cp {kernel_build_outs} ${{DIST_DIR}}
    """.format(
        kernel_build_outs = " ".join([out.path for out in kernel_build_outs.to_list()]),
    )

    if ctx.attr.initramfs:
        command += """
               cp {initramfs_img} ${{DIST_DIR}}/initramfs.img
             # Create and restore initramfs_staging_dir
               mkdir -p {initramfs_staging_dir}
               tar xf {initramfs_staging_archive} -C {initramfs_staging_dir}
        """.format(
            initramfs_img = ctx.attr.initramfs[InitramfsInfo].initramfs_img.path,
            initramfs_staging_dir = initramfs_staging_dir,
            initramfs_staging_archive = initramfs_staging_archive.path,
        )
        set_initramfs_var_cmd = """
               BUILD_INITRAMFS=1
               INITRAMFS_STAGING_DIR={initramfs_staging_dir}
        """.format(
            initramfs_staging_dir = initramfs_staging_dir,
        )
    else:
        set_initramfs_var_cmd = """
               BUILD_INITRAMFS=
               INITRAMFS_STAGING_DIR=
        """
    if ctx.attr.unpack_ramdisk:
        boot_flag_cmd += """
            if [[ -n ${SKIP_UNPACKING_RAMDISK} ]]; then
                echo "WARNING: Using SKIP_UNPACKING_RAMDISK in build config is deprecated." >&2
                echo "  Use unpack_ramdisk in kernel_image instead." >&2
            fi
        """
    else:
        boot_flag_cmd += """
            SKIP_UNPACKING_RAMDISK=1
        """
    if ctx.attr.avb_sign_boot_img:
        if not ctx.attr.avb_boot_partition_size or \
           not ctx.attr.avb_boot_key or not ctx.attr.avb_boot_algorithm or \
           not ctx.attr.avb_boot_partition_name:
            fail("avb_sign_boot_img is true, but one of [avb_boot_partition_size, avb_boot_key," +
                 " avb_boot_algorithm, avb_boot_partition_name] is not specified.")

        boot_flag_cmd += """
            AVB_SIGN_BOOT_IMG=1
            AVB_BOOT_PARTITION_SIZE={avb_boot_partition_size}
            AVB_BOOT_KEY={avb_boot_key}
            AVB_BOOT_ALGORITHM={avb_boot_algorithm}
            AVB_BOOT_PARTITION_NAME={avb_boot_partition_name}
        """.format(
            avb_boot_partition_size = ctx.attr.avb_boot_partition_size,
            avb_boot_key = ctx.file.avb_boot_key.path,
            avb_boot_algorithm = ctx.attr.avb_boot_algorithm,
            avb_boot_partition_name = ctx.attr.avb_boot_partition_name,
        )

    ramdisk_options = image_utils.ramdisk_options(
        ramdisk_compression = ctx.attr.ramdisk_compression,
        ramdisk_compression_args = ctx.attr.ramdisk_compression_args,
    )

    command += """
             # Build boot images
               (
                 {boot_flag_cmd}
                 {vendor_boot_flag_cmd}
                 {set_initramfs_var_cmd}
                 MKBOOTIMG_STAGING_DIR=$(readlink -m {mkbootimg_staging_dir})
                 # Quote because they may contain spaces. Use double quotes because they
                 # may be a variable.
                 RAMDISK_COMPRESS="{ramdisk_compress}"
                 RAMDISK_DECOMPRESS="{ramdisk_decompress}"
                 RAMDISK_EXT="{ramdisk_ext}"
                 build_boot_images
               )
               {search_and_cp_output} --srcdir ${{DIST_DIR}} --dstdir {outdir} {outs}
             # Remove staging directories
               rm -rf {modules_staging_dir}
    """.format(
        mkbootimg_staging_dir = mkbootimg_staging_dir,
        search_and_cp_output = ctx.executable._search_and_cp_output.path,
        outdir = outdir,
        outs = " ".join(outs),
        modules_staging_dir = modules_staging_dir,
        boot_flag_cmd = boot_flag_cmd,
        vendor_boot_flag_cmd = vendor_boot_flag_cmd,
        set_initramfs_var_cmd = set_initramfs_var_cmd,
        ramdisk_compress = ramdisk_options.ramdisk_compress,
        ramdisk_decompress = ramdisk_options.ramdisk_decompress,
        ramdisk_ext = ramdisk_options.ramdisk_ext,
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "BootImages",
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = ctx.outputs.outs,
        tools = depset(tools, transitive = transitive_tools),
        progress_message = "Building boot images {}".format(ctx.label),
        command = command,
    )

boot_images = rule(
    implementation = _boot_images_impl,
    doc = """Build boot images, including `boot.img`, `vendor_boot.img`, etc.

Execute `build_boot_images` in `build_utils.sh`.""",
    attrs = {
        "kernel_build": attr.label(
            mandatory = True,
            providers = [KernelSerializedEnvInfo, KernelBuildInfo],
        ),
        "initramfs": attr.label(
            providers = [InitramfsInfo],
        ),
        "deps": attr.label_list(
            allow_files = True,
        ),
        "outs": attr.output_list(),
        "mkbootimg": attr.label(
            allow_single_file = True,
            default = "//tools/mkbootimg:mkbootimg.py",
        ),
        "build_boot": attr.bool(),
        "vendor_boot_name": attr.string(doc = """
* If `"vendor_boot"`, build `vendor_boot.img`
* If `"vendor_kernel_boot"`, build `vendor_kernel_boot.img`
* If `None`, skip `vendor_boot`.
""", values = ["vendor_boot", "vendor_kernel_boot"]),
        "vendor_ramdisk_binaries": attr.label_list(allow_files = True),
        "gki_ramdisk_prebuilt_binary": attr.label(allow_single_file = True),
        "vendor_ramdisk_dev_nodes": attr.label_list(allow_files = True),
        "unpack_ramdisk": attr.bool(
            doc = """ When false it skips unpacking the vendor ramdisk and copy it as
            is, without modifications, into the boot image. Also skip the mkbootfs step.

            It defaults to True. (Allowing falling back to the value in build config.
            This will change in the future, after giving notice about its deprecation.)
            """,
            default = True,
        ),
        "avb_sign_boot_img": attr.bool(
            doc = """ If set to `True` signs the boot image using the avb_boot_key.
            The kernel prebuilt tool `avbtool` is used for signing.""",
        ),
        "avb_boot_partition_size": attr.int(doc = """Size of the boot partition
            in bytes. Used when `avb_sign_boot_img` is True."""),
        "avb_boot_key": attr.label(
            doc = """ Key used for signing.
            Used when `avb_sign_boot_img` is True.""",
            allow_single_file = True,
        ),
        # Note: The actual values comes from:
        # https://cs.android.com/android/platform/superproject/+/master:external/avb/avbtool.py
        "avb_boot_algorithm": attr.string(
            doc = """ `avb_boot_key` algorithm
            used e.g. SHA256_RSA2048. Used when `avb_sign_boot_img` is True.""",
            values = [
                "NONE",
                "SHA256_RSA2048",
                "SHA256_RSA4096",
                "SHA256_RSA8192",
                "SHA512_RSA2048",
                "SHA512_RSA4096",
                "SHA512_RSA8192",
            ],
        ),
        "avb_boot_partition_name": attr.string(doc = """Name of the boot partition.
            Used when `avb_sign_boot_img` is True."""),
        "ramdisk_compression": attr.string(
            doc = "If provided it specfies the format used for any ramdisks generated." +
                  "If not provided a fallback value from build.config is used.",
            values = ["lz4", "gzip"],
        ),
        "ramdisk_compression_args": attr.string(
            doc = "Command line arguments passed only to lz4 command to control compression level.",
        ),
        "_debug_print_scripts": attr.label(
            default = "//build/kernel/kleaf:debug_print_scripts",
        ),
        "_search_and_cp_output": attr.label(
            default = Label("//build/kernel/kleaf:search_and_cp_output"),
            cfg = "exec",
            executable = True,
        ),
    },
)
