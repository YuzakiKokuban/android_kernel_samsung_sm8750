# Build configs

This document provides reference to the Bazel equivalent or alternative for
the legacy build configs.

For build configs with a Bazel equivalent / alternative, a code snippet and a
link to the [documentation for all rules] is provided. You may look up the
relevant macros, rules and attributes in the documentation.

For build configs that should be kept in `build.config` files, the text
_"Specify in the build config"_ is displayed.

For build configs that are set to a fixed value in Bazel, the text
_"Not customizable in Bazel"_ is displayed. Contact [owners](../OWNERS) if you
need to customize this.

For build configs that are not used in Bazel with alternatives provided, the
text _"Not used in Bazel."_ is displayed.

For build configs that are not supported, the text
_"Not supported"_ is displayed. Contact [owners](../OWNERS) if you need support.

## Table of contents

* [`BUILD_CONFIG`](#build_config)
* [`BUILD_CONFIG_FRAGMENTS`](#build_config_fragments)
* [`FAST_BUILD`](#fast_build)
* [`OUT_DIR`](#out_dir)
* [`DIST_DIR`](#dist_dir)
* [`MAKE_GOALS`](#make_goals)
* [`EXT_MODULES`](#ext_modules)
* [`EXT_MODULES_MAKEFILE`](#ext_modules_makefile)
* [`KCONFIG_EXT_PREFIX`](#kconfig_ext_prefix)
* [`UNSTRIPPED_MODULES`](#unstripped_modules)
* [`COMPRESS_UNSTRIPPED_MODULES`](#compress_unstripped_modules)
* [`COMPRESS_MODULES`](#compress_modules)
* [`LD`](#ld)
* [`HERMETIC_TOOLCHAIN`](#hermetic_toolchain)
* [`ADDITIONAL_HOST_TOOLS`](#additional_host_tools)
* [`ABI_DEFINITION`](#abi_definition)
* [`KMI_SYMBOL_LIST`](#kmi_symbol_list)
* [`ADDITIONAL_KMI_SYMBOL_LISTS`](#additional_kmi_symbol_lists)
* [`KMI_ENFORCED`](#kmi_enforced)
* [`GENERATE_VMLINUX_BTF`](#generate_vmlinux_btf)
* [`SKIP_MRPROPER`](#skip_mrproper)
* [`SKIP_DEFCONFIG`](#skip_defconfig)
* [`SKIP_IF_VERSION_MATCHES`](#skip_if_version_matches)
* [`PRE_DEFCONFIG_CMDS`](#pre_defconfig_cmds)
* [`POST_DEFCONFIG_CMDS`](#post_defconfig_cmds)
* [`POST_KERNEL_BUILD_CMDS`](#post_kernel_build_cmds)
* [`LTO`](#lto)
* [`TAGS_CONFIG`](#tags_config)
* [`IN_KERNEL_MODULES`](#in_kernel_modules)
* [`SKIP_EXT_MODULES`](#skip_ext_modules)
* [`DO_NOT_STRIP_MODULES`](#do_not_strip_modules)
* [`EXTRA_CMDS`](#extra_cmds)
* [`DIST_CMDS`](#dist_cmds)
* [`SKIP_CP_KERNEL_HDR`](#skip_cp_kernel_hdr)
* [`BUILD_BOOT_IMG`](#build_boot_img)
* [`BUILD_VENDOR_BOOT_IMG`](#build_vendor_boot_img)
* [`SKIP_VENDOR_BOOT`](#skip_vendor_boot)
* [`VENDOR_RAMDISK_CMDS`](#vendor_ramdisk_cmds)
* [`SKIP_UNPACKING_RAMDISK`](#skip_unpacking_ramdisk)
* [`AVB_SIGN_BOOT_IMG`](#avb_sign_boot_img)
* [`AVB_BOOT_PARTITION_SIZE`](#avb_boot_partition_size)
* [`AVB_BOOT_KEY`](#avb_boot_key)
* [`AVB_BOOT_ALGORITHM`](#avb_boot_algorithm)
* [`AVB_BOOT_PARTITION_NAME`](#avb_boot_partition_name)
* [`BUILD_INITRAMFS`](#build_initramfs)
* [`MODULES_OPTIONS`](#modules_options)
* [`MODULES_ORDER`](#modules_order)
* [`GKI_MODULES_LIST`](#gki_modules_list)
* [`VENDOR_DLKM_ETC_FILES`](#vendor_dlkm_etc_files)
* [`VENDOR_DLKM_FS_TYPE`](#vendor_dlkm_fs_type)
* [`VENDOR_DLKM_MODULES_LIST`](#vendor_dlkm_modules_list)
* [`VENDOR_DLKM_MODULES_BLOCKLIST`](#vendor_dlkm_modules_blocklist)
* [`VENDOR_DLKM_PROPS`](#vendor_dlkm_props)
* [`SYSTEM_DLKM_FS_TYPE`](#system_dlkm_fs_type)
* [`SYSTEM_DLKM_MODULES_LIST`](#system_dlkm_modules_list)
* [`SYSTEM_DLKM_MODULES_BLOCKLIST`](#system_dlkm_modules_blocklist)
* [`SYSTEM_DLKM_PROPS`](#system_dlkm_props)
* [`LZ4_RAMDISK`](#lz4_ramdisk)
* [`LZ4_RAMDISK_COMPRESS_ARGS`](#lz4_ramdisk_compress_args)
* [`TRIM_NONLISTED_KMI`](#trim_nonlisted_kmi)
* [`KMI_SYMBOL_LIST_STRICT_MODE`](#kmi_symbol_list_strict_mode)
* [`KMI_STRICT_MODE_OBJECTS`](#kmi_strict_mode_objects)
* [`GKI_DIST_DIR`](#gki_dist_dir)
* [`GKI_BUILD_CONFIG`](#gki_build_config)
* [`GKI_PREBUILTS_DIR`](#gki_prebuilts_dir)
* [`BUILD_DTBO_IMG`](#build_dtbo_img)
* [`DTS_EXT_DIR`](#dts_ext_dir)
* [`BUILD_GKI_CERTIFICATION_TOOLS`](#build_gki_certification_tools)
* [`BUILD_VENDOR_KERNEL_BOOT`](#build_vendor_kernel_boot)
* [`MKBOOTIMG_PATH`](#mkbootimg_path)
* [`BUILD_GKI_ARTIFACTS`](#build_gki_artifacts)
* [`GKI_KERNEL_CMDLINE`](#gki_kernel_cmdline)
* [`KMI_SYMBOL_LIST_ADD_ONLY`](#kmi_symbol_list_add_only)

## BUILD\_CONFIG

```python
kernel_build(build_config=...)
```

See [documentation for all rules].

## BUILD\_CONFIG\_FRAGMENTS

```python
kernel_build_config()
```

See [documentation for all rules].

## FAST\_BUILD

Not used in Bazel. Alternatives:

You may disable LTO or use thin LTO; see [`LTO`](#LTO).

You may use `--config=fast` to build faster.
See [fast.md](fast.md) for details.

You may build just the kernel binary and GKI modules, without headers and
installing modules by building the `kernel_build` target, e.g.

```shell
$ bazel build //common:kernel_aarch64
```

## OUT\_DIR

Not used in Bazel. Alternatives:

You may customize [`DIST_DIR`](#dist_dir). See below.

## DIST\_DIR

You may specify it statically with

```python
copy_to_dist_dir(dist_dir=...)
```

You may override it in the command line with `--dist_dir`:

```shell
$ bazel run ..._dist -- --dist_dir=...
```

See [documentation for all rules].

## MAKE\_GOALS

```python
kernel_build(
  make_goals = ...
)
```

See [documentation for all rules].

## EXT\_MODULES

```python
ddk_module()
```

NOTE: Prefer `ddk_module` over the legacy `kernel_module`.

See [documentation for all rules].

## EXT\_MODULES\_MAKEFILE

Not used in Bazel.

Reason: `EXT_MODULES_MAKEFILE` supports building external kernel modules in
parallel. This is naturally supported in Bazel.

## KCONFIG\_EXT\_PREFIX

```python
kernel_build(kconfig_ext=...)
```

See [documentation for all rules].

## UNSTRIPPED\_MODULES

```python
kernel_build(collect_unstripped_modules=...)
kernel_filegroup(collect_unstripped_modules=...)
```

See [documentation for all rules].

## COMPRESS\_UNSTRIPPED\_MODULES

```python
kernel_unstripped_modules_archive()
```

See [documentation for all rules].

## COMPRESS\_MODULES

Not supported. Contact [owners](../OWNERS) if you need support for this config.

## LD

Not used in Bazel. Alternatives:

You may customize the clang toolchain version via

```python
kernel_build(toolchain_version=...)
```

See [documentation for all rules].

## HERMETIC\_TOOLCHAIN

Not customizable in Bazel.

Reason: This is the default for Bazel builds. Its value cannot be changed.

See [Ensuring hermeticity](hermeticity.md) for details about ensuring
hermeticity.

See [documentation for all rules].

## ADDITIONAL\_HOST\_TOOLS

Not customizable in Bazel.

Reason: The list of host tools are fixed and specified in `hermetic_tools()`.

See [documentation for all rules].

## ABI\_DEFINITION

Not used in Bazel. Alternatives:

The XML format of ABI definition is no longer supported. The STG format
of ABI definition may be set with the following:

```python
kernel_abi(abi_definition_stg=...)
```

See [documentation for all rules].

See [documentation for ABI monitoring].

## KMI\_SYMBOL\_LIST

```python
kernel_build(kmi_symbol_list=...)
```

See [documentation for all rules].

See [documentation for ABI monitoring].

## ADDITIONAL\_KMI\_SYMBOL\_LISTS

```python
kernel_build(additional_kmi_symbol_lists=...)
```

See [documentation for all rules].

See [documentation for ABI monitoring].

## KMI\_ENFORCED

```python
kernel_abi(kmi_enforced=...)
```

See [documentation for all rules].

See [documentation for ABI monitoring].

## GENERATE\_VMLINUX\_BTF

```python
kernel_build(generate_vmlinux_btf=...)
```

See [documentation for all rules].

## SKIP\_MRPROPER

Not used in Bazel. Alternatives:

- For sandbox builds, the `$OUT_DIR` always starts with no contents (as if
  `SKIP_MRPROPER=`).
- For non-sandbox builds, the `$OUT_DIR` is always cached (as if
  `SKIP_MRPROPER=1`). You may clean its contents with `bazel clean`.

See [sandbox.md](sandbox.md).

## SKIP\_DEFCONFIG

Not used in Bazel.

Reason: Bazel automatically rebuild `make defconfig` when its relevant sources
change, as if `SKIP_DEFCONFIG` is determined automatically.

## SKIP\_IF\_VERSION\_MATCHES

Not used in Bazel.

Reason: Incremental builds are supported by default.

## PRE\_DEFCONFIG\_CMDS

Specify in the build config.

Or, remove from the build config, and use `kernel_build_config` and
`hermetic_genrule`.

In some cases, you may want to use `kernel_build.defconfig_fragments`. However,
keep in mind that `kernel_build.defconfig_fragments` are applied **after**
`make defconfig`, unlike `PRE_DEFCONFIG_CMDS`. Double check the resulting
`.config` file if you are migrating from `PRE_DEFCONFIG_CMDS` to
`kernel_build.defconfig_fragments` by building the `_config` target, for
example:

```
$ tools/bazel build //common:kernel_aarch64_config
$ find $(realpath bazel-bin) -name .config
```

To support `--config=local` builds, `PRE_DEFCONFIG_CMDS` must not write to the
source tree, including `$ROOT_DIR/$KERNEL_DIR`. See
[errors.md#defconfig-readonly](errors.md#defconfig-readonly) for details.

See [documentation for all rules].

## POST\_DEFCONFIG\_CMDS

Specify in the build config.

If the command only contains a single `merge_config.sh` invocation, you may
also use `kernel_build.defconfig_fragments`. They are applied **after**
`make defconfig`, just like `POST_DEFCONFIG_CMDS`.

Or, remove from the build config, and use `kernel_build_config` and
`hermetic_genrule`.

See [documentation for all rules].

## POST\_KERNEL\_BUILD\_CMDS

Not supported.

Reason: commands are disallowed in general because of unclear dependency.

You may define a `genrule` target with appropriate inputs (possibly from a
`kernel_build` macro), then add the target to your `copy_to_dist_dir` macro.

## LTO

```shell
$ bazel build --lto={default,none,thin,full} TARGETS
$ bazel run   --lto={default,none,thin,full} TARGETS
```

See [disable LTO during development](lto.md).

## TAGS\_CONFIG

Not supported. Contact [owners](../OWNERS) if you need support for this config.

## IN\_KERNEL\_MODULES

Not customizable in Bazel.

Reason: This is set by default in `build.config.common`. Its value cannot be
changed.

## SKIP\_EXT\_MODULES

Not used in Bazel. Alternatives:

You may skip building external modules by leaving them out in the
`bazel build` command.

## DO\_NOT\_STRIP\_MODULES

```python
kernel_build(strip_modules=...)
```

See [documentation for all rules].

## EXTRA\_CMDS

Not used in Bazel.

Reason: commands are disallowed in general because of unclear dependency.

Alternatives: You may define a `genrule` or `exec` target with appropriate
inputs, then add the target to your `copy_to_dist_dir` macro.

See [documentation for `genrule`].

## DIST\_CMDS

Not used in Bazel.

Reason: commands are disallowed in general because of unclear dependency.

Alternatives: You may define a `genrule` or `exec` target with appropriate
inputs, then add the target to your `copy_to_dist_dir` macro.

See [documentation for `genrule`].

## SKIP\_CP\_KERNEL\_HDR

Not used in Bazel. Alternatives:

You may skip building headers by leaving them out in the
`bazel build` command.

## BUILD\_BOOT\_IMG

```python
kernel_images(build_boot=...)
```

See [documentation for all rules].

## BUILD\_VENDOR\_BOOT\_IMG

```python
kernel_images(build_vendor_boot=...)
```

The flags are rather straightforward. `build_boot` controls the `boot` image.
`build_vendor_boot` controls the `vendor_boot` image. Setting
`build_vendor_boot = True` requires `build_boot = True`.

See [documentation for all rules].

## SKIP\_VENDOR\_BOOT

```python
kernel_images(build_vendor_boot=...)
```

See [`BUILD_VENDOR_BOOT_IMG`](#build_vendor_boot_img).

See [documentation for all rules].

## VENDOR\_RAMDISK\_CMDS

Not used in Bazel.

Reason: Commands are disallowed in general because of unclear dependency.

Alternatives: you may define a `genrule` or `exec` target with appropriate
inputs, then add the target to your `copy_to_dist_dir` macro.

## SKIP\_UNPACKING\_RAMDISK

```python
kernel_images(unpack_ramdisk=...)
```

See [documentation for all rules].

## AVB\_SIGN\_BOOT\_IMG

```python
kernel_images(avb_sign_boot_img=...)
```

See [documentation for all rules].

## AVB\_BOOT\_PARTITION\_SIZE

```python
kernel_images(avb_boot_partition_size=...)
```

Note: It only has effect when [`AVB_SIGN_BOOT_IMG`](#avb_sign_boot_img) is set.

See [documentation for all rules].

## AVB\_BOOT\_KEY

```python
kernel_images(avb_boot_key=...)
```

Note: It only has effect when [`AVB_SIGN_BOOT_IMG`](#avb_sign_boot_img) is set.

See [documentation for all rules].

## AVB\_BOOT\_ALGORITHM

```python
kernel_images(avb_boot_algorithm=...)
```

Note: It only has effect when [`AVB_SIGN_BOOT_IMG`](#avb_sign_boot_img) is set.

See [documentation for all rules].

## AVB\_BOOT\_PARTITION\_NAME

```python
kernel_images(avb_boot_partition_name=...)
```

Note: It only has effect when [`AVB_SIGN_BOOT_IMG`](#avb_sign_boot_img) is set.

See [documentation for all rules].

## BUILD\_INITRAMFS

```python
kernel_images(build_initramfs=...)
```

See [documentation for all rules].

## MODULES\_OPTIONS

```python
kernel_images(modules_options=...)
```

See [documentation for all rules].

## MODULES\_ORDER

Not customizable in Bazel.

Reason: The Bazel build already sets the order of loading modules for you, and
`build_utils.sh` uses it generate the `modules.load` files already.

## GKI\_MODULES\_LIST

**Note**: This config has been deprecated.
List is being generated by checking the presence of the signature instead.

Not customizable in Bazel.

Reason: This is set to a fixed value in the `module_implicit_outs` attribute of
`//common:kernel_aarch64`.

See [documentation for all rules].

## VENDOR\_DLKM\_FS\_TYPE

```python
kernel_images(vendor_dlkm_fs_type=[ext4, erofs])
```

See [documentation for all rules].

## VENDOR\_DLKM\_ETC\_FILES

```python
kernel_images(vendor_dlkm_etc_files=[...])
```

See [documentation for all rules].

## VENDOR\_DLKM\_MODULES\_LIST

```python
kernel_images(vendor_dlkm_modules_list=...)
```

See [documentation for all rules].

## VENDOR\_DLKM\_MODULES\_BLOCKLIST

```python
kernel_images(vendor_dlkm_modules_blocklist=...)
```

See [documentation for all rules].

## VENDOR\_DLKM\_PROPS

```python
kernel_images(vendor_dlkm_props=...)
```

See [documentation for all rules].

## SYSTEM\_DLKM\_FS\_TYPE

```python
kernel_images(system_dlkm_fs_types=["ext4", "erofs"])
```

See [documentation for all rules].

## SYSTEM\_DLKM\_MODULES\_LIST

```python
kernel_images(system_dlkm_modules_list=...)
```

See [documentation for all rules].

## SYSTEM\_DLKM\_MODULES\_BLOCKLIST

```python
kernel_images(system_dlkm_modules_blocklist=...)
```

See [documentation for all rules].

## SYSTEM\_DLKM\_PROPS

```python
kernel_images(system_dlkm_props=...)
```

See [documentation for all rules].

## LZ4\_RAMDISK

**NOTE**: Rather than specifying if `lz4` is used or not, kleaf expects you to
 specify the value as one of the strings `lz4` or `gzip`.

* If `ramdisk_compression =` `lz4` or `gzip`, do not specify `LZ4_RAMDISK` in
  build configs, as the value will be ignored.
* If `ramdisk_compression =` `None` or unspecified, the value in build configs
 will be respected.

```python
kernel_image(ramdisk_compression="lz4",...)
```

See [documentation for all rules].

## LZ4\_RAMDISK\_COMPRESS\_ARGS

**NOTE**: Use in combination with [`LZ4_RAMDISK`](#lz4_ramdisk).

```python
kernel_image(ramdisk_compression_args=...)
```

See [documentation for all rules].

## TRIM\_NONLISTED\_KMI

```python
kernel_build(trim_nonlisted_kmi=...)
```

See [documentation for all rules].

See [documentation for ABI monitoring].

## KMI\_SYMBOL\_LIST\_STRICT\_MODE

```python
kernel_build(kmi_symbol_list_strict_mode=...)
```

See [documentation for all rules].

See [documentation for ABI monitoring].

## KMI\_STRICT\_MODE\_OBJECTS

Not customizable in Bazel.

Reason: This is always `vmlinux` (regardless of whether it is in `outs`) and
the list of `module_outs` from the `kernel_build` attribute of the `kernel_abi`
macro invocation.

See [documentation for all rules].

See [documentation for ABI monitoring].

## GKI\_DIST\_DIR

Not used in Bazel. Alternatives:

Mixed builds are supported by

```python
kernel_build(base_build=...)
```

See [documentation for implementing Kleaf].

## GKI\_BUILD\_CONFIG

Not used in Bazel. Alternatives:

Mixed builds are supported by

```python
kernel_build(base_build=...)
```

See [documentation for implementing Kleaf].

## GKI\_PREBUILTS\_DIR

```python
kernel_filegroup()
```

Mixed builds are supported by

```python
kernel_build(base_build=...)
```

You may specify the `kernel_filegroup` target in the `base_build`
attribute of the `kernel_build` macro invocation.

See [documentation for all rules].

See [documentation for implementing Kleaf].

## BUILD\_DTBO\_IMG

```python
kernel_images(build_dtbo=...)
```

See [documentation for all rules].

## DTS\_EXT\_DIR

```python
kernel_dtstree()
kernel_build(dtstree=...)
```

Define `kernel_dtstree()` in `DTS_EXT_DIR`, then set the `dtstree` argument of
the `kernel_build()` macro invocation to the `kernel_dtstree()` target.

See [documentation for all rules].

## BUILD\_GKI\_CERTIFICATION\_TOOLS

Add `//build/kernel:gki_certification_tools` to your `copy_to_dist_dir()` macro
invocation.

See [build/kernel/BUILD.bazel](../../BUILD.bazel).

## BUILD\_VENDOR\_KERNEL\_BOOT

```python
kernel_images(build_vendor_kernel_boot=...)
```

See [documentation for all rules].

## MKBOOTIMG\_PATH

```python
kernel_images(mkbootimg=...)
gki_artifacts(mkbootimg=...)
```

See [documentation for all rules] for `kernel_images`.

**NOTE**: `gki_artifacts` is an implementation detail, and it should only be
invoked by GKI targets.

## BUILD\_GKI\_ARTIFACTS

```python
gki_artifacts()
```

**NOTE**: `gki_artifacts` is an implementation detail, and it should only be
invoked by GKI targets.

For GKI targets, it may be configured via the following:

```python
define_common_kernels(
  target_configs = {
    "kernel_aarch64": {
      "build_gki_artifacts": True,
      "gki_boot_img_sizes": {
        "": "67108864",
        "lz4": "53477376",
      },
    },
  },
)
```

See [documentation for all rules].

## GKI\_KERNEL\_CMDLINE

```python
gki_artifacts(gki_kernel_cmdline=...)
```

**NOTE**: `gki_artifacts` is an implementation detail, and it should only be
invoked by GKI targets.

## KBUILD\_SYMTYPES

If `KBUILD_SYMTYPES=1` is specified in build configs:

```python
kernel_build(kbuild_symtypes="true")
```

See [documentation for all rules].

To specify `KBUILD_SYMTYPES=1` at build time:

```shell
$ bazel build --kbuild_symtypes ...
```

See [symtypes.md](symtypes.md) for details.

## KMI\_SYMBOL\_LIST\_ADD\_ONLY

```python
kernel_abi(kmi_symbol_list_add_only=...)
```

See [documentation for all rules].

See [documentation for ABI monitoring].

[documentation for all rules]: api_reference.md

[documentation for ABI monitoring]: abi.md

[documentation for implementing Kleaf]: impl.md
