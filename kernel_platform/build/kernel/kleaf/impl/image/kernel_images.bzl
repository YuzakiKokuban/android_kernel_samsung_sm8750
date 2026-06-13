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
Build multiple kernel images.
"""

load(
    ":common_providers.bzl",
    "ImagesInfo",
)
load(":image/boot_images.bzl", "boot_images")
load(":image/dtbo.bzl", "dtbo")
load(":image/image_utils.bzl", "image_utils")
load(":image/initramfs.bzl", "initramfs")
load(":image/system_dlkm_image.bzl", "system_dlkm_image")
load(":image/vendor_dlkm_image.bzl", "vendor_dlkm_image")

visibility("//build/kernel/kleaf/...")

def kernel_images(
        name,
        kernel_modules_install,
        kernel_build = None,
        base_kernel_images = None,
        build_initramfs = None,
        build_vendor_dlkm = None,
        build_vendor_dlkm_flatten = None,
        build_boot = None,
        build_vendor_boot = None,
        build_vendor_kernel_boot = None,
        build_system_dlkm = None,
        build_system_dlkm_flatten = None,
        build_dtbo = None,
        dtbo_srcs = None,
        dtbo_config = None,
        mkbootimg = None,
        deps = None,
        boot_image_outs = None,
        gki_ramdisk_prebuilt_binary = None,
        modules_list = None,
        modules_recovery_list = None,
        modules_charger_list = None,
        modules_blocklist = None,
        modules_options = None,
        vendor_ramdisk_binaries = None,
        vendor_ramdisk_dev_nodes = None,
        system_dlkm_fs_type = None,
        system_dlkm_fs_types = None,
        system_dlkm_modules_list = None,
        system_dlkm_modules_blocklist = None,
        system_dlkm_props = None,
        vendor_dlkm_archive = None,
        vendor_dlkm_etc_files = None,
        vendor_dlkm_fs_type = None,
        vendor_dlkm_modules_list = None,
        vendor_dlkm_modules_blocklist = None,
        vendor_dlkm_props = None,
        ramdisk_compression = None,
        ramdisk_compression_args = None,
        unpack_ramdisk = None,
        avb_sign_boot_img = None,
        avb_boot_partition_size = None,
        avb_boot_key = None,
        avb_boot_algorithm = None,
        avb_boot_partition_name = None,
        dedup_dlkm_modules = None,
        create_modules_order = None,
        **kwargs):
    """Build multiple kernel images.

    You may use `filegroup.output_group` to request certain files. Example:

    ```
    kernel_images(
        name = "my_images",
        build_vendor_dlkm = True,
    )
    filegroup(
        name = "my_vendor_dlkm",
        srcs = [":my_images"],
        output_group = "vendor_dlkm.img",
    )
    ```

    Allowed strings in `filegroup.output_group`:
    * `vendor_dlkm.img`, if `build_vendor_dlkm` is set
    * `vendor_dlkm_flatten.img` if `build_vendor_dlkm_flatten` is not empty
    * `system_dlkm.img`, if `build_system_dlkm` and `system_dlkm_fs_type` is set
    * `system_dlkm.<type>.img` for each of `system_dlkm_fs_types`, if
        `build_system_dlkm` is set and `system_dlkm_fs_types` is not empty.
    * `system_dlkm.flatten.<type>.img` for each of `sytem_dlkm_fs_types, if
        `build_system_dlkm_flatten` is set and `system_dlkm_fs_types` is not empty.

    If no output files are found, the filegroup resolves to an empty one.
    You may also read `OutputGroupInfo` on the `kernel_images` rule directly
    in your rule implementation.

    For details, see
    [Requesting output files](https://bazel.build/extending/rules#requesting_output_files).

    Args:
        name: name of this rule, e.g. `kernel_images`,
        kernel_modules_install: A `kernel_modules_install` rule.

          The main kernel build is inferred from the `kernel_build` attribute of the
          specified `kernel_modules_install` rule. The main kernel build must contain
          `System.map` in `outs` (which is included if you use `DEFAULT_GKI_OUTS` or
          `X86_64_OUTS` from `common_kernels.bzl`).
        kernel_build: A `kernel_build` rule. Must specify if `build_boot`.
        mkbootimg: Path to the mkbootimg.py script which builds boot.img.
          Only used if `build_boot`. If `None`,
          default to `//tools/mkbootimg:mkbootimg.py`.
          NOTE: This overrides `MKBOOTIMG_PATH`.
        deps: Additional dependencies to build images.

          This must include the following:
          - For `initramfs`:
            - The file specified by `MODULES_LIST`
            - The file specified by `MODULES_BLOCKLIST`, if `MODULES_BLOCKLIST` is set
            - The file containing the list of modules needed for booting into recovery.
            - The file containing the list of modules needed for booting into charger mode.
          - For `vendor_dlkm` image:
            - The file specified by `VENDOR_DLKM_MODULES_LIST`
            - The file specified by `VENDOR_DLKM_MODULES_BLOCKLIST`, if set
            - The file specified by `VENDOR_DLKM_PROPS`, if set
            - The file specified by `selinux_fc` in `VENDOR_DLKM_PROPS`, if set

        boot_image_outs: A list of output files that will be installed to `DIST_DIR` when
          `build_boot_images` in `build/kernel/build_utils.sh` is executed.

          You may leave out `vendor_boot.img` from the list. It is automatically added when
          `build_vendor_boot = True`.

          If `build_boot` is equal to `False`, the default is empty.

          If `build_boot` is equal to `True`, the default list assumes the following:
          - `BOOT_IMAGE_FILENAME` is not set (which takes default value `boot.img`), or is set to
            `"boot.img"`
          - `vendor_boot.img` if `build_vendor_boot`
          - `RAMDISK_EXT=lz4`. Is used when `ramdisk_compression`(see below) is not specified.
            - The list contains `ramdisk.<ramdisk_ext>` which means it assumes `build_boot_images`
              generates this file. See `build_utils.sh` on conditions for when it is actually
              generated.
          - if `build_vendor_boot`, it assumes `VENDOR_BOOTCONFIG` is set and
            `BOOT_IMAGE_HEADER_VERSION >= 4`, which creates `vendor-bootconfig.img` to contain
            `VENDOR_BOOTCONFIG` .
          - The list contains `dtb.img`
        build_initramfs: Whether to build initramfs. Keep in sync with `BUILD_INITRAMFS`.
        build_system_dlkm: Whether to build system_dlkm.img an image with GKI modules.
        build_system_dlkm_flatten: Whether to build system_dlkm.flatten.<fs>.img.
          This image have directory structure as `/lib/modules/*.ko` i.e. no `uname -r` in the path.
        build_vendor_dlkm: Whether to build `vendor_dlkm` image. It must be set if
          `vendor_dlkm_modules_list` is set.
        build_vendor_dlkm_flatten: Whether to build `vendor_dlkm_flatten` image. The image
          have directory structure as `/lib/modules/*.ko` i.e. no `uname -r` in the path

          Note: at the time of writing (Jan 2022),
          `vendor_dlkm.modules.blocklist` is **always** created regardless of
          the value of `VENDOR_DLKM_MODULES_BLOCKLIST`.
          If `build_vendor_dlkm()` in `build_utils.sh` does not generate
          `vendor_dlkm.modules.blocklist`, an empty file is created.
        build_boot: Whether to build boot image. It must be set if either `BUILD_BOOT_IMG`
          or `BUILD_VENDOR_BOOT_IMG` is set.

          This depends on `kernel_build`. Hence, if this is set to `True`,
          `kernel_build` must be set.

          If `True`, adds `boot.img` to `boot_image_outs` if not already in the list.
        build_vendor_boot: Whether to build `vendor_boot.img`. It must be set if either
          `BUILD_BOOT_IMG` or `BUILD_VENDOR_BOOT_IMG` is set, and `SKIP_VENDOR_BOOT` is not set,
          and `BUILD_VENDOR_KERNEL_BOOT` is not set.

          At most **one** of `build_vendor_boot` and `build_vendor_kernel_boot` may be set to
          `True`.

          If `True`, adds `vendor_boot.img` to `boot_image_outs` if not already in the list.

        build_vendor_kernel_boot: Whether to build `vendor_kernel_boot.img`. It must be set if either
          `BUILD_BOOT_IMG` or `BUILD_VENDOR_BOOT_IMG` is set, and `SKIP_VENDOR_BOOT` is not set,
          and `BUILD_VENDOR_KERNEL_BOOT` is set.

          At most **one** of `build_vendor_boot` and `build_vendor_kernel_boot` may be set to
          `True`.

          If `True`, adds `vendor_kernel_boot.img` to `boot_image_outs` if not already in the list.
        build_dtbo: Whether to build dtbo image. Keep this in sync with `BUILD_DTBO_IMG`.

          If `dtbo_srcs` is non-empty, `build_dtbo` is `True` by default. Otherwise it is `False`
          by default.
        dtbo_srcs: list of `*.dtbo` files used to package the `dtbo.img`. Keep this in sync
          with `MKDTIMG_DTBOS`; see example below.

          If `dtbo_srcs` is non-empty, `build_dtbo` must not be explicitly set to `False`.

          Example:
          ```
          kernel_build(
              name = "tuna_kernel",
              outs = [
                  "path/to/foo.dtbo",
                  "path/to/bar.dtbo",
              ],
          )
          kernel_images(
              name = "tuna_images",
              kernel_build = ":tuna_kernel",
              dtbo_srcs = [
                  ":tuna_kernel/path/to/foo.dtbo",
                  ":tuna_kernel/path/to/bar.dtbo",
              ]
          )
          ```
        dtbo_config: a config file to create dtbo image by cfg_create command.
        base_kernel_images: The `kernel_images()` corresponding to the `base_kernel` of the
          `kernel_build`. This is required for building a device-specific `system_dlkm` image.
          For example, if `base_kernel` of `kernel_build()` is `//common:kernel_aarch64`,
          then `base_kernel_images` is `//common:kernel_aarch64_images`.

          This is also required if `dedup_dlkm_modules and not build_system_dlkm`.
        modules_list: A file containing list of modules to use for `vendor_boot.modules.load`.
        modules_recovery_list: A file containing a list of modules to load when booting into
          recovery.
        modules_charger_list: A file containing a list of modules to load when booting into
          charger mode.
        modules_blocklist: A file containing a list of modules which are
          blocked from being loaded.

          This file is copied directly to staging directory, and should be in the format:
          ```
          blocklist module_name
          ```
        modules_options: Label to a file copied to `/lib/modules/<kernel_version>/modules.options` on the ramdisk.

          Lines in the file should be of the form:
          ```
          options <modulename> <param1>=<val> <param2>=<val> ...
          ```
        system_dlkm_fs_type: Deprecated. Use `system_dlkm_fs_types` instead.

            Supported filesystems for `system_dlkm` image are `ext4` and `erofs`.
            Defaults to `ext4` if not specified.
        system_dlkm_fs_types: List of file systems type for `system_dlkm` images.

            Supported filesystems for `system_dlkm` image are `ext4` and `erofs`.
            If not specified, builds `system_dlkm.img` with ext4 else builds
            `system_dlkm.<fs>.img` for each file system type in the list.
        system_dlkm_modules_list: location of an optional file
          containing the list of kernel modules which shall be copied into a
          system_dlkm partition image.
        system_dlkm_modules_blocklist: location of an optional file containing a list of modules
          which are blocked from being loaded.

          This file is copied directly to the staging directory and should be in the format:
          ```
          blocklist module_name
          ```
        system_dlkm_props: location of a text file containing
          the properties to be used for creation of a `system_dlkm` image
          (filesystem, partition size, etc). If this is not set (and
          `build_system_dlkm` is), a default set of properties will be used
          which assumes an ext4 filesystem and a dynamic partition.
        vendor_dlkm_archive: If set, enable archiving the vendor_dlkm staging directory.
        vendor_dlkm_fs_type: Supported filesystems for `vendor_dlkm.img` are `ext4` and `erofs`. Defaults to `ext4` if not specified.
        vendor_dlkm_etc_files: Files that need to be copied to `vendor_dlkm.img` etc/ directory.
        vendor_dlkm_modules_list: location of an optional file
          containing the list of kernel modules which shall be copied into a
          `vendor_dlkm` partition image. Any modules passed into `MODULES_LIST` which
          become part of the `vendor_boot.modules.load` will be trimmed from the
          `vendor_dlkm.modules.load`.
        vendor_dlkm_modules_blocklist: location of an optional file containing a list of modules
          which are blocked from being loaded.

          This file is copied directly to the staging directory and should be in the format:
          ```
          blocklist module_name
          ```
        vendor_dlkm_props: location of a text file containing
          the properties to be used for creation of a `vendor_dlkm` image
          (filesystem, partition size, etc). If this is not set (and
          `build_vendor_dlkm` is), a default set of properties will be used
          which assumes an ext4 filesystem and a dynamic partition.
        vendor_ramdisk_binaries: List of vendor ramdisk binaries
          which includes the device-specific components of ramdisk like the fstab
          file and the device-specific rc files. If specifying multiple vendor ramdisks
          and identical file paths exist in the ramdisks, the file from last ramdisk is used.

          Note: **order matters**. To prevent buildifier from sorting the list, add the following:
          ```
          # do not sort
          ```
        vendor_ramdisk_dev_nodes: List of dev nodes description files
          which describes special device files to be added to the vendor
          ramdisk. File format is as accepted by mkbootfs.
        ramdisk_compression: If provided it specfies the format used for any ramdisks generated.
          If not provided a fallback value from build.config is used.
          Possible values are `lz4`, `gzip`, None.
        ramdisk_compression_args: Command line arguments passed only to lz4 command
          to control compression level. It only has effect when used with
          `ramdisk_compression` equal to "lz4".
        unpack_ramdisk: When set to `False`, skips unpacking the vendor ramdisk and
          copy it as is, without modifications, into the boot image.
          Also skips the mkbootfs step.
        avb_sign_boot_img: If set to `True` signs the boot image using the avb_boot_key.
          The kernel prebuilt tool `avbtool` is used for signing.
        avb_boot_partition_size: Size of the boot partition in bytes.
          Used when `avb_sign_boot_img` is True.
        avb_boot_key: Path to the key used for signing.
          Used when `avb_sign_boot_img` is True.
        avb_boot_algorithm: `avb_boot_key` algorithm used e.g. SHA256_RSA2048.
          Used when `avb_sign_boot_img` is True.
        avb_boot_partition_name: = Name of the boot partition.
          Used when `avb_sign_boot_img` is True.
        dedup_dlkm_modules: If set, modules already in `system_dlkm` is
          excluded in `vendor_dlkm.modules.load`. Modules in `vendor_dlkm`
          is allowed to link to modules in `system_dlkm`.

          The `system_dlkm` image is defined by the following:

          - If `build_system_dlkm` is set, the `system_dlkm` image built by
            this rule.
          - If `build_system_dlkm` is not set, the `system_dlkm` image in
            `base_kernel_images`. If `base_kernel_images` is not set, build
            fails.

          If set, **additional changes in the userspace is required** so that
          `system_dlkm` modules are loaded before `vendor_dlkm` modules.
        create_modules_order: Whether to create and keep a modules.order file
          generated by a postorder traversal of the `kernel_modules_install` sources.
          It applies to building `initramfs` and `vendor_dlkm`.
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """
    all_rules = []

    build_any_boot_image = build_boot or build_vendor_boot or build_vendor_kernel_boot or \
                           avb_sign_boot_img
    if build_any_boot_image:
        if kernel_build == None:
            fail("{}: Must set kernel_build if any of these are true: build_boot={}, build_vendor_boot={}, build_vendor_kernel_boot={}".format(
                name,
                build_boot,
                build_vendor_boot,
                build_vendor_kernel_boot,
            ))

    # Set default value for boot_image_outs according to build_boot
    if boot_image_outs == None:
        if not build_any_boot_image:
            boot_image_outs = []
        else:
            ramdisk_out = "ramdisk." + image_utils.ramdisk_options(
                ramdisk_compression,
                ramdisk_compression_args,
            ).ramdisk_ext
            boot_image_outs = [
                "dtb.img",
                ramdisk_out,
            ]

    boot_image_outs = list(boot_image_outs)

    if build_boot and "boot.img" not in boot_image_outs:
        boot_image_outs.append("boot.img")

    if gki_ramdisk_prebuilt_binary and "init_boot.img" not in boot_image_outs:
        boot_image_outs.append("init_boot.img")

    if build_vendor_boot and "vendor_boot.img" not in boot_image_outs:
        boot_image_outs.append("vendor_boot.img")
        boot_image_outs.append("vendor-bootconfig.img")

    if build_vendor_kernel_boot and "vendor_kernel_boot.img" not in boot_image_outs:
        boot_image_outs.append("vendor_kernel_boot.img")

    vendor_boot_name = None
    if build_vendor_boot:
        vendor_boot_name = "vendor_boot"
    elif build_vendor_kernel_boot:
        vendor_boot_name = "vendor_kernel_boot"

    vendor_boot_modules_load = None
    vendor_boot_modules_load_recovery = None
    vendor_boot_modules_load_charger = None
    if build_initramfs:
        if vendor_boot_name:
            vendor_boot_modules_load = "{}_initramfs/{}.modules.load".format(name, vendor_boot_name)

            if modules_recovery_list:
                vendor_boot_modules_load_recovery = "{}_initramfs/{}.modules.load.recovery".format(name, vendor_boot_name)

            if modules_charger_list:
                vendor_boot_modules_load_charger = "{}_initramfs/{}.modules.load.charger".format(name, vendor_boot_name)

        if ramdisk_compression_args and ramdisk_compression != "lz4":
            fail(
                "ramdisk_compress_args provided but ramdisk_compression={} is not lz4.".format(
                    ramdisk_compression,
                ),
            )

        initramfs(
            name = "{}_initramfs".format(name),
            kernel_modules_install = kernel_modules_install,
            deps = deps,
            vendor_boot_modules_load = vendor_boot_modules_load,
            vendor_boot_modules_load_recovery = vendor_boot_modules_load_recovery,
            vendor_boot_modules_load_charger = vendor_boot_modules_load_charger,
            modules_list = modules_list,
            modules_recovery_list = modules_recovery_list,
            modules_charger_list = modules_charger_list,
            modules_blocklist = modules_blocklist,
            modules_options = modules_options,
            ramdisk_compression = ramdisk_compression,
            ramdisk_compression_args = ramdisk_compression_args,
            create_modules_order = create_modules_order,
            **kwargs
        )
        all_rules.append(":{}_initramfs".format(name))

    if build_system_dlkm:
        system_dlkm_image(
            name = "{}_system_dlkm_image".format(name),
            # For GKI system_dlkm
            kernel_modules_install = kernel_modules_install,
            # For device system_dlkm, give GKI's system_dlkm_staging_archive.tar.gz
            base_kernel_images = base_kernel_images,
            build_system_dlkm_flatten_image = build_system_dlkm_flatten,
            deps = deps,
            modules_list = modules_list,
            modules_blocklist = modules_blocklist,
            system_dlkm_fs_type = system_dlkm_fs_type,
            system_dlkm_fs_types = system_dlkm_fs_types,
            system_dlkm_modules_list = system_dlkm_modules_list,
            system_dlkm_modules_blocklist = system_dlkm_modules_blocklist,
            system_dlkm_props = system_dlkm_props,
            create_modules_order = False,
            **kwargs
        )
        all_rules.append(":{}_system_dlkm_image".format(name))

    if build_vendor_dlkm:
        if vendor_dlkm_fs_type == None:
            vendor_dlkm_fs_type = "ext4"

        vendor_dlkm_image(
            name = "{}_vendor_dlkm_image".format(name),
            kernel_modules_install = kernel_modules_install,
            vendor_boot_modules_load = vendor_boot_modules_load,
            build_vendor_dlkm_flatten_image = build_vendor_dlkm_flatten,
            deps = deps,
            vendor_dlkm_archive = vendor_dlkm_archive,
            vendor_dlkm_etc_files = vendor_dlkm_etc_files,
            vendor_dlkm_fs_type = vendor_dlkm_fs_type,
            vendor_dlkm_modules_list = vendor_dlkm_modules_list,
            vendor_dlkm_modules_blocklist = vendor_dlkm_modules_blocklist,
            vendor_dlkm_props = vendor_dlkm_props,
            dedup_dlkm_modules = dedup_dlkm_modules,
            system_dlkm_image = "{}_system_dlkm_image".format(name) if build_system_dlkm else None,
            base_kernel_images = base_kernel_images,
            create_modules_order = create_modules_order,
            **kwargs
        )
        all_rules.append(":{}_vendor_dlkm_image".format(name))

    if build_any_boot_image:
        boot_images(
            name = "{}_boot_images".format(name),
            kernel_build = kernel_build,
            outs = ["{}_boot_images/{}".format(name, out) for out in boot_image_outs],
            deps = deps,
            initramfs = ":{}_initramfs".format(name) if build_initramfs else None,
            mkbootimg = mkbootimg,
            vendor_ramdisk_binaries = vendor_ramdisk_binaries,
            gki_ramdisk_prebuilt_binary = gki_ramdisk_prebuilt_binary,
            vendor_ramdisk_dev_nodes = vendor_ramdisk_dev_nodes,
            build_boot = build_boot,
            vendor_boot_name = vendor_boot_name,
            unpack_ramdisk = unpack_ramdisk,
            avb_sign_boot_img = avb_sign_boot_img,
            avb_boot_partition_size = avb_boot_partition_size,
            avb_boot_key = avb_boot_key,
            avb_boot_algorithm = avb_boot_algorithm,
            avb_boot_partition_name = avb_boot_partition_name,
            ramdisk_compression = ramdisk_compression,
            ramdisk_compression_args = ramdisk_compression_args,
            **kwargs
        )
        all_rules.append(":{}_boot_images".format(name))

    if build_dtbo == None:
        build_dtbo = bool(dtbo_srcs)

    if dtbo_srcs:
        if not build_dtbo:
            fail("{}: build_dtbo must be True if dtbo_srcs is non-empty.")

    if build_dtbo:
        dtbo(
            name = "{}_dtbo".format(name),
            srcs = dtbo_srcs,
            config_file = dtbo_config,
            kernel_build = kernel_build,
            **kwargs
        )
        all_rules.append(":{}_dtbo".format(name))

    _kernel_images(
        name = name,
        srcs = all_rules,
        **kwargs
    )

def _kernel_images_impl(ctx):
    default_info = DefaultInfo(files = depset(transitive = [
        target.files
        for target in ctx.attr.srcs
    ]))

    # Combine Images from dependencies into OutputGroupInfo
    output_group_info_depsets = {}
    for target in ctx.attr.srcs:
        if ImagesInfo not in target:
            continue
        for key, the_depset in target[ImagesInfo].files_dict.items():
            if key not in output_group_info_depsets:
                output_group_info_depsets[key] = []
            output_group_info_depsets[key].append(the_depset)
    output_group_info = OutputGroupInfo(**{
        key: depset(transitive = value_list)
        for key, value_list in output_group_info_depsets.items()
    })

    return [
        default_info,
        output_group_info,
    ]

_kernel_images = rule(
    implementation = _kernel_images_impl,
    attrs = {
        "srcs": attr.label_list(),
    },
)
