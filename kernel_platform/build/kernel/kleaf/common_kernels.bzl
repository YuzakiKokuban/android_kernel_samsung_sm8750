# Copyright (C) 2021 The Android Open Source Project
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

"""Functions that are useful in the common kernel package (usually `//common`)."""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load("//build/kernel/kleaf/artifact_tests:device_modules_test.bzl", "device_modules_test")
load("//build/kernel/kleaf/artifact_tests:kernel_test.bzl", "initramfs_modules_options_test")
load("//build/kernel/kleaf/impl:gki_artifacts.bzl", "gki_artifacts", "gki_artifacts_prebuilts")
load("//build/kernel/kleaf/impl:kernel_filegroup_declaration.bzl", "kernel_filegroup_declaration")
load(
    "//build/kernel/kleaf/impl:kernel_prebuilt_utils.bzl",
    "CI_TARGET_MAPPING",
)
load("//build/kernel/kleaf/impl:kernel_sbom.bzl", "kernel_sbom")
load("//build/kernel/kleaf/impl:merge_kzip.bzl", "merge_kzip")
load("//build/kernel/kleaf/impl:out_headers_allowlist_archive.bzl", "out_headers_allowlist_archive")
load(
    ":constants.bzl",
    "DEFAULT_GKI_OUTS",
    "X86_64_OUTS",
)
load(
    ":kernel.bzl",
    "kernel_abi",
    "kernel_abi_dist",
    "kernel_build",
    "kernel_build_config",
    "kernel_compile_commands",
    "kernel_images",
    "kernel_kythe",
    "kernel_modules_install",
    "kernel_unstripped_modules_archive",
    "merged_kernel_uapi_headers",
)
load(":print_debug.bzl", "print_debug")

# keys: name of common kernels
# values: list of keys in target_configs to look up
_COMMON_KERNEL_NAMES = {
    "kernel_aarch64": ["kernel_aarch64"],
    "kernel_aarch64_16k": ["kernel_aarch64_16k", "kernel_aarch64"],
    "kernel_aarch64_interceptor": ["kernel_aarch64_interceptor", "kernel_aarch64"],
    "kernel_aarch64_debug": ["kernel_aarch64_debug", "kernel_aarch64"],
    "kernel_riscv64": ["kernel_riscv64"],
    "kernel_x86_64": ["kernel_x86_64"],
    "kernel_x86_64_debug": ["kernel_x86_64_debug", "kernel_x86_64"],
}

# Always collect_unstripped_modules for common kernels.
_COLLECT_UNSTRIPPED_MODULES = True

# Always strip modules for common kernels.
_STRIP_MODULES = True

# Always keep a copy of Module.symvers for common kernels.
_KEEP_MODULE_SYMVERS = True

# This transition is not needed for GKI
_GKI_ADD_VMLINUX = False

# glob() must be executed in a BUILD thread, so this cannot be a global
# variable.
def _default_target_configs():
    """Return the default value of `target_configs` of [`define_common_kernels()`](#define_common_kernels).
    """
    aarch64_kmi_symbol_list = native.glob(["android/abi_gki_aarch64"])
    aarch64_kmi_symbol_list = aarch64_kmi_symbol_list[0] if aarch64_kmi_symbol_list else None
    aarch64_additional_kmi_symbol_lists = native.glob(
        ["android/abi_gki_aarch64*"],
        exclude = ["**/*.xml", "**/*.stg", "android/abi_gki_aarch64"],
    )
    aarch64_protected_exports_list = (native.glob(["android/abi_gki_protected_exports"]) or [None])[0]
    aarch64_protected_modules_list = (native.glob(["android/gki_protected_modules"]) or [None])[0]
    aarch64_trim_and_check = bool(aarch64_kmi_symbol_list) or len(aarch64_additional_kmi_symbol_lists) > 0
    aarch64_abi_definition_stg = native.glob(["android/abi_gki_aarch64.stg"])
    aarch64_abi_definition_stg = aarch64_abi_definition_stg[0] if aarch64_abi_definition_stg else None

    # Common configs for aarch64*
    aarch64_common = {
        "arch": "arm64",
        "build_config": "build.config.gki.aarch64",
        "outs": DEFAULT_GKI_OUTS,
        "gki_system_dlkm_modules": "android/gki_system_dlkm_modules_arm64",
    }

    gki_boot_img_sizes = {
        # Assume BUILD_GKI_BOOT_IMG_SIZE is the following
        "": "67108864",
        # Assume BUILD_GKI_BOOT_IMG_LZ4_SIZE is the following
        "lz4": "53477376",
        # Assume BUILD_GKI_BOOT_IMG_GZ_SIZE is the following
        "gz": "47185920",
    }

    aarch64_abi = {
        # Assume the value for KMI_SYMBOL_LIST, ADDITIONAL_KMI_SYMBOL_LISTS, ABI_DEFINITION, and KMI_ENFORCED
        # for build.config.gki.aarch64
        "kmi_symbol_list": aarch64_kmi_symbol_list,
        "additional_kmi_symbol_lists": aarch64_additional_kmi_symbol_lists,
        "protected_exports_list": aarch64_protected_exports_list,
        "protected_modules_list": aarch64_protected_modules_list,
        "abi_definition_stg": aarch64_abi_definition_stg,
        "kmi_enforced": bool(aarch64_abi_definition_stg),
    }

    # Common configs for riscv64
    riscv64_common = {
        "arch": "riscv64",
        "build_config": "build.config.gki.riscv64",
        "outs": DEFAULT_GKI_OUTS,
        # Assume BUILD_GKI_ARTIFACTS=1
        "build_gki_artifacts": True,
        "gki_boot_img_sizes": gki_boot_img_sizes,
        "gki_system_dlkm_modules": "android/gki_system_dlkm_modules_riscv64",
    }

    # Common configs for x86_64 and x86_64_debug
    x86_64_common = {
        "arch": "x86_64",
        "build_config": "build.config.gki.x86_64",
        "outs": X86_64_OUTS,
        # Assume BUILD_GKI_ARTIFACTS=1
        "build_gki_artifacts": True,
        "gki_boot_img_sizes": {
            # Assume BUILD_GKI_BOOT_IMG_SIZE is the following
            "": "67108864",
        },
        "gki_system_dlkm_modules": "android/gki_system_dlkm_modules_x86_64",
    }

    return {
        "kernel_aarch64": dicts.add(aarch64_common, aarch64_abi, {
            # In build.config.gki.aarch64:
            # - If there are symbol lists: assume TRIM_NONLISTED_KMI=${TRIM_NONLISTED_KMI:-1}
            # - If there aren't:           assume TRIM_NONLISTED_KMI unspecified
            "trim_nonlisted_kmi": aarch64_trim_and_check,
            "kmi_symbol_list_strict_mode": aarch64_trim_and_check,
            # Assume BUILD_GKI_ARTIFACTS=1
            "build_gki_artifacts": True,
            "gki_boot_img_sizes": gki_boot_img_sizes,
        }),
        "kernel_aarch64_16k": dicts.add(aarch64_common, {
            # Assume TRIM_NONLISTED_KMI="" in build.config.gki.aarch64.16k
            "trim_nonlisted_kmi": False,
            "page_size": "16k",
            # Assume BUILD_GKI_ARTIFACTS=1
            "build_gki_artifacts": True,
            "gki_boot_img_sizes": gki_boot_img_sizes,
        }),
        "kernel_aarch64_interceptor": dicts.add(aarch64_common, {
            "enable_interceptor": True,
        }),
        "kernel_aarch64_debug": dicts.add(aarch64_common, aarch64_abi, {
            "trim_nonlisted_kmi": False,
            "kmi_symbol_list_strict_mode": False,
            # Assume BUILD_GKI_ARTIFACTS=1
            "build_gki_artifacts": True,
            "gki_boot_img_sizes": gki_boot_img_sizes,
            "deprecation": """
    Consider building {main_target} with:
        * --notrim to disable trimming, or
        * --debug to enable additional debug options.""".format(
                main_target = native.package_relative_label("kernel_aarch64"),
            ),
        }),
        "kernel_riscv64": dicts.add(riscv64_common, {
            # Assume TRIM_NONLISTED_KMI="" in build.config.gki.riscv64
            "trim_nonlisted_kmi": False,
        }),
        "kernel_x86_64": x86_64_common,
        "kernel_x86_64_debug": dicts.add(x86_64_common, {
            "trim_nonlisted_kmi": False,
            "kmi_symbol_list_strict_mode": False,
            "deprecation": """
    Consider building {main_target} with:
        * --notrim to disable trimming, or
        * --debug to enable additional debug options.""".format(
                main_target = native.package_relative_label("kernel_x86_64"),
            ),
        }),
    }

# buildifier: disable=unnamed-macro
def define_common_kernels(
        branch = None,
        target_configs = None,
        toolchain_version = None,
        visibility = None):
    """Defines common build targets for Android Common Kernels.

    This macro expands to the commonly defined common kernels (such as the GKI
    kernels and their variants. They are defined based on the conventionally
    used `BUILD_CONFIG` file and produce usual output files.

    Targets declared for kernel build (parent list item depends on child list item;
    deprecated targets not listed):
    - `kernel_aarch64_sources`
    - `kernel_aarch64_dist`
      - `kernel_aarch64`
      - `kernel_aarch64_uapi_headers`
      - `kernel_aarch64_additional_artifacts`
      - `kernel_aarch64_modules`
    - `kernel_aarch64_16k_dist`
      - `kernel_aarch64_16k`
      - `kernel_aarch64_modules`
    - `kernel_riscv64_dist`
      - `kernel_riscv64`
    - `kernel_x86_64_sources`
    - `kernel_x86_64_dist`
      - `kernel_x86_64`
      - `kernel_x86_64_uapi_headers`
      - `kernel_x86_64_additional_artifacts`

    `<name>` (aka `kernel_{aarch64,riscv64,x86_64}{_16k,}`) targets build the
    main kernel build artifacts, e.g. `vmlinux`, etc.

    `<name>_sources` are convenience filegroups that refers to all sources required to
    build `<name>` and related targets.

    `<name>_uapi_headers` targets build `kernel-uapi-headers.tar.gz`.

    `<name>_additional_artifacts` contains additional artifacts that may be added to
    a distribution. This includes:
      - Images, including `system_dlkm`, etc.
      - `kernel-headers.tar.gz`

    `<name>_dist` targets can be run to obtain a distribution outside the workspace.

    Aliases are created to refer to the GKI kernel (`kernel_aarch64`) as
    "`kernel`" and the corresponding dist target (`kernel_aarch64_dist`) as
    "`kernel_dist`".

    Targets declared for cross referencing:
    - `kernel_aarch64_kythe_dist`
      - `kernel_aarch64_kythe`

    Targets declared for Bazel rules analysis for debugging purposes:
    - `kernel_aarch64_print_configs`
    - `kernel_riscv64_print_configs`
    - `kernel_x86_64_print_configs`

    **ABI monitoring**
    On branches with ABI monitoring turned on (aka KMI symbol lists are checked
    in; see argument `target_configs`), the following targets are declared:

    - `kernel_aarch64_abi`

    See [`kernel_abi()`](kernel.md#kernel_abi) for details.

    **Target configs**

    The content of `target_configs` should match the following variables in
    `build.config.gki{,-debug}.{aarch64,riscv64,x86_64}`:
    - `KMI_SYMBOL_LIST`
    - `ADDITIONAL_KMI_SYMBOL_LISTS`
    - `TRIM_NONLISTED_KMI`
    - `KMI_SYMBOL_LIST_STRICT_MODE`
    - `GKI_MODULES_LIST` (corresponds to [`kernel_build.module_implicit_outs`](kernel.md#kernel_build-module_implicit_outs))
    - `BUILD_GKI_ARTIFACTS`
    - `BUILD_GKI_BOOT_IMG_SIZE` and `BUILD_GKI_BOOT_IMG_{COMPRESSION}_SIZE`

    The keys of the `target_configs` may be one of the following:
    - `kernel_aarch64`
    - `kernel_aarch64_16k`
    - `kernel_riscv64`
    - `kernel_x86_64`

    The values of the `target_configs` should be a dictionary, where keys
    are one of the following, and values are passed to the corresponding
    argument in [`kernel_build`](kernel.md#kernel_build):
    - `kmi_symbol_list`
    - `additional_kmi_symbol_lists`
    - `trim_nonlisted_kmi`
    - `kmi_symbol_list_strict_mode`
    - `module_implicit_outs` (corresponds to `GKI_MODULES_LIST`)

    In addition, the values of `target_configs` may contain the following keys:
    - `build_gki_artifacts`
    - `gki_boot_img_sizes` (corresponds to `BUILD_GKI_BOOT_IMG_SIZE` and `BUILD_GKI_BOOT_IMG_{COMPRESSION}_SIZE`)
        - This is a dictionary where keys are lower-cased compression algorithm (e.g. `"lz4"`)
        and values are sizes (e.g. `BUILD_GKI_BOOT_IMG_LZ4_SIZE`).
        The empty-string key `""` corresponds to `BUILD_GKI_BOOT_IMG_SIZE`.

    A target is configured as follows. A configuration item for this target
    is determined by the following, in the following order:

    1. `target_configs[target_name][configuration_item]`, if it exists;
    2. `default_target_configs[target_name][configuration_item]`, if it exists, where
        `default_target_configs` contains sensible defaults. See below.
    3. `None`

    For example, to determine the value of `kmi_symbol_list` of `kernel_aarch64`:

    ```
    if "kernel_aarch64" in target_configs and "kmi_symbol_list" in target_configs["kernel_aarch64"]:
        value = target_configs["kernel_aarch64"]["kmi_symbol_list"]
        # Note: if `target_configs["kernel_aarch64"]["kmi_symbol_list"] == None`, it'll be passed
        # as None, regardless of value in default_target_configs
    elif "kernel_aarch64" in default_target_configs and "kmi_symbol_list" in default_target_configs["kernel_aarch64"]:
        value = default_target_configs["kernel_aarch64"]["kmi_symbol_list"]
    else:
        value = None

    kernel_build(..., kmi_symbol_list = value)
    ```

    The `default_target_configs` above contains sensible defaults:
    - `kernel_aarch64`:
        - `kmi_symbol_list = "android/abi_gki_aarch64"` if the file exist, else `None`
        - `additional_kmi_symbol_list = glob(["android/abi_gki_aarch64*"])` excluding `kmi_symbol_list` and XMLs
        - `TRIM_NONLISTED_KMI=${TRIM_NONLISTED_KMI:-1}` in `build.config` if there are symbol lists, else empty
        - `KMI_SYMBOL_LIST_STRICT_MODE=${KMI_SYMBOL_LIST_STRICT_MODE:-1}` in `build.config` if there are symbol lists, else empty
    - `kernel_aarch64_16k`:
        - No `kmi_symbol_list` nor `additional_kmi_symbol_lists`
        - `TRIM_NONLISTED_KMI` is not specified in `build.config`
        - `KMI_SYMBOL_LIST_STRICT_MODE` is not specified in `build.config`
    - `kernel_riscv64`:
        - No `kmi_symbol_list` nor `additional_kmi_symbol_lists`
        - `TRIM_NONLISTED_KMI` is not specified in `build.config`
        - `KMI_SYMBOL_LIST_STRICT_MODE` is not specified in `build.config`
    - `kernel_x86_64`:
        - No `kmi_symbol_list` nor `additional_kmi_symbol_lists`
        - `TRIM_NONLISTED_KMI` is not specified in `build.config`
        - `KMI_SYMBOL_LIST_STRICT_MODE` is not specified in `build.config`

    That is, the default value is:
    ```
    aarch64_kmi_symbol_list = glob(["android/abi_gki_aarch64"])
    aarch64_kmi_symbol_list = aarch64_kmi_symbol_list[0] if aarch64_kmi_symbol_list else None
    aarch64_additional_kmi_symbol_lists = glob(
        ["android/abi_gki_aarch64*"],
        exclude = ["**/*.stg", "android/abi_gki_aarch64"],
    )
    aarch64_protected_exports_list = native.glob(["android/abi_gki_protected_exports"])
    aarch64_protected_exports_list = aarch64_protected_exports_list[0] if aarch64_protected_exports_list else None
    aarch64_protected_modules_list = native.glob(["android/gki_protected_modules"])
    aarch64_protected_modules_list = aarch64_protected_modules_list[0] if aarch64_protected_modules_list else None
    aarch64_trim_and_check = bool(aarch64_kmi_symbol_list) or len(aarch64_additional_kmi_symbol_lists) > 0
    default_target_configs = {
        "kernel_aarch64": {
            "kmi_symbol_list": aarch64_kmi_symbol_list,
            "additional_kmi_symbol_lists": aarch64_additional_kmi_symbol_lists,
            "protected_exports_list": aarch64_protected_exports_list,
            "protected_modules_list": aarch64_protected_modules_list,
            "trim_nonlisted_kmi": aarch64_trim_and_check,
            "kmi_symbol_list_strict_mode": aarch64_trim_and_check,
        },
        "kernel_aarch64_16k": {
        },
        "kernel_riscv64": {
        },
        "kernel_x86_64": {
        },
    }
    ```

    If `target_configs` is not set explicitly in `define_common_kernels()`:

    ```
    |                                   |trim?         |
    |-----------------------------------|--------------|
    |`kernel_aarch64`                   |TRIM          |
    |(with symbol lists)                |              |
    |(`trim_nonlisted_kmi=True`)        |              |
    |-----------------------------------|--------------|
    |`kernel_aarch64`                   |NO TRIM       |
    |(no symbol lists)                  |              |
    |(`trim_nonlisted_kmi=None`)        |              |
    |-----------------------------------|--------------|
    |`kernel_aarch64_16k`               |NO TRIM       |
    |(`trim_nonlisted_kmi=None`)        |              |
    |-----------------------------------|--------------|
    |`kernel_riscv64`                   |NO TRIM       |
    |(`trim_nonlisted_kmi=None`)        |              |
    |-----------------------------------|--------------|
    |`kernel_x86_64`                    |NO TRIM       |
    |(`trim_nonlisted_kmi=None`)        |              |
    ```

    To print the actual configurations for debugging purposes for e.g.
    `//common:kernel_aarch64`:

    ```
    bazel build //common:kernel_aarch64_print_configs
    ```

    **Prebuilts**

    You may set the argument `--use_prebuilt_gki` to a GKI prebuilt build number
    on [ci.android.com](http://ci.android.com) or your custom CI host. The format is:

    ```
    bazel <command> --use_prebuilt_gki=<build_number> <targets>
    ```

    For example, the following downloads GKI artifacts of build number 8077484 (assuming
    the current package is `//common`):

    ```
    bazel build --use_prebuilt_gki=8077484 //common:kernel_aarch64_download_or_build
    ```

    If you leave out the `--use_prebuilt_gki` argument, the command is equivalent to
    `bazel build //common:kernel_aarch64`, which builds kernel from source.

    `<name>_download_or_build` targets builds `<name>` from source if the `use_prebuilt_gki`
    is not set, and downloads artifacts of the build number from
    [ci.android.com](http://ci.android.com) (or your custom CI host) if it is set.

    - `kernel_aarch64_download_or_build`
      - `kernel_aarch64_additional_artifacts_download_or_build`
      - `kernel_aarch64_uapi_headers_download_or_build`

    Note: If a device should build against downloaded prebuilts unconditionally, set
    `--use_prebuilt_gki` and a fixed build number in `device.bazelrc`. For example:
    ```
    # device.bazelrc
    build --use_prebuilt_gki
    build --action_env=KLEAF_DOWNLOAD_BUILD_NUMBER_MAP="gki_prebuilts=8077484"
    ```

    This is equivalent to specifying `--use_prebuilt_gki=8077484` for all Bazel commands.

    You may set `--use_signed_prebuilts` to download the signed boot images instead
    of the unsigned one. This requires `--use_prebuilt_gki` to be set to a signed build.

    Args:
      branch: **Deprecated**. This attribute is ignored.

        This used to be used to calculate the default `--dist_dir`, which was
        `out/{branch}/dist`. This was expected to be
        the value of `BRANCH` in `build.config`. If not set, it was loaded
        from `common/build.config.constants` **in `//{common_kernel_package}`**
        where `common_kernel_package` was supplied to `define_kleaf_workspace()`
        in the `WORKSPACE` file. Usually, `common_kernel_package = "common"`.
        Hence, if `define_common_kernels()` was called in a different package, it
        was required to be supplied.

        Now, the default value of `--dist_dir` is `out/{name}/dist`, so the value
        of `branch` has no effect. Hence, the attribute is ignored.
      target_configs: A dictionary, where keys are target names, and
        values are a dictionary of configurations to override the default
        configuration for this target.

      toolchain_version: If not set, use default value in `kernel_build`.
      visibility: visibility of the `kernel_build` and targets defined for downloaded prebuilts.
        If unspecified, its value is `["//visibility:public"]`.

        See [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
    """

    if branch != None:
        # buildifier: disable=print
        print(("\nWARNING: {package}: define_common_kernels() no longer uses the branch " +
               "attribute. Default value of --dist_dir has been changed to out/{{name}}/dist. " +
               "Please remove the branch attribute from define_common_kernels().").format(
            package = str(native.package_relative_label(":x")).removesuffix(":x"),
        ))

    if visibility == None:
        visibility = ["//visibility:public"]

    # Workaround to set KERNEL_DIR correctly and
    #  avoid using the fallback (directory of the config).
    # TODO(b/338438451): Clean this up with kernel_build.kernel_dir attr.
    set_kernel_dir_cmd = "KERNEL_DIR=\"{kernel_dir}\"".format(
        kernel_dir = paths.join(
            native.package_relative_label(":x").workspace_root,
            native.package_relative_label(":x").package,
        ),
    )
    write_file(
        name = "set_kernel_dir_build_config",
        content = [set_kernel_dir_cmd, ""],
        out = "set_kernel_dir_build_config/build.config",
        visibility = visibility,
    )

    default_target_configs = _default_target_configs()
    new_target_configs = {}
    for name, target_configs_names in _COMMON_KERNEL_NAMES.items():
        new_target_config = _get_target_config(
            name = name,
            target_configs_names = target_configs_names,
            target_configs = target_configs,
            default_target_configs = default_target_configs,
        )

        # On android14-5.15, riscv64 is not supported. However,
        # default_target_configs still contains riscv64 unconditionally.
        # Filter it out.
        if not native.glob([new_target_config["build_config"]]):
            continue
        new_target_configs[name] = new_target_config
    target_configs = new_target_configs

    native.filegroup(
        name = "common_kernel_sources",
        srcs = native.glob(
            ["**"],
            exclude = [
                "BUILD.bazel",
                "**/*.bzl",
                ".git/**",

                # ctag files
                "tags",
                "TAGS",

                # temporary ctag files
                "tags.temp",
                "tags.lock",

                # cscope files
                "cscope.*",
                "ncscope.*",
            ],
        ),
    )

    for name, target_config in target_configs.items():
        _define_common_kernel(
            name = name,
            toolchain_version = toolchain_version,
            visibility = visibility,
            **target_config
        )

    native.alias(
        name = "kernel",
        actual = ":kernel_aarch64",
    )

    native.alias(
        name = "kernel_dist",
        actual = ":kernel_aarch64_dist",
    )

    string_flag(
        name = "kernel_kythe_corpus",
        build_setting_default = "",
    )

    kythe_candidates = [
        "kernel_aarch64",
        "kernel_x86_64",
        "kernel_riscv64",
    ]

    merge_kzip(
        name = "kernel_kythe",
        srcs = [name + "_kythe" for name in kythe_candidates if name in target_configs],
    )

    copy_to_dist_dir(
        name = "kernel_kythe_dist",
        data = [":kernel_kythe"],
        flat = True,
    )

    _define_prebuilts(visibility = visibility)

def _get_target_config(
        name,
        target_configs_names,
        target_configs,
        default_target_configs):
    """Returns arguments to _define_common_kernel for a target."""
    if target_configs == None:
        target_configs = {}
    target_config = {}
    for target_configs_name in target_configs_names:
        if target_configs_name in target_configs:
            target_config = dict(target_configs[target_configs_name])
            break
    default_target_config = default_target_configs.get(name, {})
    for key, default_value in default_target_config.items():
        target_config.setdefault(key, default_value)
    return target_config

def _define_common_kernel(
        name,
        outs,
        arch,
        build_config,
        toolchain_version,
        visibility,
        defconfig_fragments = None,
        enable_interceptor = None,
        kmi_symbol_list = None,
        additional_kmi_symbol_lists = None,
        trim_nonlisted_kmi = None,
        kmi_symbol_list_strict_mode = None,
        kmi_symbol_list_add_only = None,
        module_implicit_outs = None,
        protected_exports_list = None,
        protected_modules_list = None,
        gki_system_dlkm_modules = None,
        make_goals = None,
        abi_definition_stg = None,
        kmi_enforced = None,
        build_gki_artifacts = None,
        gki_boot_img_sizes = None,
        page_size = None,
        deprecation = None,
        ddk_headers_archive = None,
        extra_dist = None):
    json_target_config = dict(
        name = name,
        outs = outs,
        arch = arch,
        build_config = build_config,
        defconfig_fragments = defconfig_fragments,
        toolchain_version = toolchain_version,
        visibility = visibility,
        enable_interceptor = enable_interceptor,
        kmi_symbol_list = kmi_symbol_list,
        additional_kmi_symbol_lists = additional_kmi_symbol_lists,
        trim_nonlisted_kmi = trim_nonlisted_kmi,
        kmi_symbol_list_strict_mode = kmi_symbol_list_strict_mode,
        module_implicit_outs = module_implicit_outs,
        protected_exports_list = protected_exports_list,
        protected_modules_list = protected_modules_list,
        gki_system_dlkm_modules = gki_system_dlkm_modules,
        make_goals = make_goals,
        abi_definition_stg = abi_definition_stg,
        kmi_enforced = kmi_enforced,
        build_gki_artifacts = build_gki_artifacts,
        gki_boot_img_sizes = gki_boot_img_sizes,
        page_size = page_size,
        deprecation = deprecation,
        ddk_headers_archive = ddk_headers_archive,
        extra_dist = extra_dist,
    )
    json_target_config = json.encode_indent(json_target_config, indent = "    ")
    json_target_config = json_target_config.replace("null", "None")

    print_debug(
        name = name + "_print_configs",
        content = "_define_common_kernel(**{})".format(json_target_config),
        tags = ["manual"],
    )

    native.alias(
        name = name + "_sources",
        actual = ":common_kernel_sources",
    )

    all_kmi_symbol_lists = additional_kmi_symbol_lists
    all_kmi_symbol_lists = [] if all_kmi_symbol_lists == None else list(all_kmi_symbol_lists)

    # Add user KMI symbol lists to additional lists
    additional_kmi_symbol_lists = all_kmi_symbol_lists + [
        "//build/kernel/kleaf:user_kmi_symbol_lists",
    ]

    if kmi_symbol_list:
        all_kmi_symbol_lists.append(kmi_symbol_list)

    native.filegroup(
        name = name + "_all_kmi_symbol_lists",
        srcs = all_kmi_symbol_lists,
    )

    if defconfig_fragments:
        defconfig_fragments = list(defconfig_fragments)

    kernel_build_config(
        name = name + "_build_config",
        srcs = [
            # do not sort
            ":set_kernel_dir_build_config",
            build_config,
            Label("//build/kernel/kleaf:gki_build_config_fragment"),
        ],
    )

    kernel_build(
        name = name,
        srcs = [name + "_sources"],
        outs = outs,
        arch = arch,
        implicit_outs = [
            # Kernel build time module signing utility and keys
            # Only available during GKI builds
            # Device fragments need to add: '# CONFIG_MODULE_SIG_ALL is not set'
            "scripts/sign-file",
            "certs/signing_key.pem",
            "certs/signing_key.x509",
        ],
        build_config = name + "_build_config",
        defconfig_fragments = defconfig_fragments,
        enable_interceptor = enable_interceptor,
        visibility = visibility,
        collect_unstripped_modules = _COLLECT_UNSTRIPPED_MODULES,
        strip_modules = _STRIP_MODULES,
        toolchain_version = toolchain_version,
        keep_module_symvers = _KEEP_MODULE_SYMVERS,
        kmi_symbol_list = kmi_symbol_list,
        additional_kmi_symbol_lists = additional_kmi_symbol_lists,
        trim_nonlisted_kmi = trim_nonlisted_kmi,
        kmi_symbol_list_strict_mode = kmi_symbol_list_strict_mode,
        module_implicit_outs = module_implicit_outs,
        protected_exports_list = protected_exports_list,
        protected_modules_list = protected_modules_list,
        make_goals = make_goals,
	page_size = page_size,
        deprecation = deprecation,
        pack_module_env = True,
        ddk_module_defconfig_fragments = [
            Label("//build/kernel/kleaf/impl/defconfig:signing_modules_disabled"),
        ],
    )

    kernel_abi(
        name = name + "_abi",
        kernel_build = name,
        visibility = visibility,
        define_abi_targets = bool(kmi_symbol_list),
        # Sync with KMI_SYMBOL_LIST_MODULE_GROUPING
        module_grouping = None,
        abi_definition_stg = abi_definition_stg,
        kmi_enforced = kmi_enforced,
        kmi_symbol_list_add_only = kmi_symbol_list_add_only,
        deprecation = deprecation,
        enable_add_vmlinux = _GKI_ADD_VMLINUX,
    )

    if enable_interceptor:
        return

    # A subset of headers in OUT_DIR that only contains scripts/. This is useful
    # for DDK headers interpolation.
    out_headers_allowlist_archive(
        name = name + "_script_headers",
        kernel_build = name,
        subdirs = ["scripts"],
    )

    native.filegroup(
        name = name + "_ddk_allowlist_headers",
        srcs = [
            name + "_script_headers",
            name + "_uapi_headers",
        ],
        visibility = [
            Label("//build/kernel/kleaf:__pkg__"),
        ],
    )

    kernel_modules_install(
        name = name + "_modules_install",
        # The GKI target does not have external modules. GKI modules goes
        # into the in-tree kernel module list, aka kernel_build.module_implicit_outs.
        # Hence, this is empty.
        kernel_modules = [],
        kernel_build = name,
    )

    kernel_unstripped_modules_archive(
        name = name + "_unstripped_modules_archive",
        kernel_build = name,
    )

    kernel_images(
        name = name + "_images",
        kernel_build = name,
        kernel_modules_install = name + "_modules_install",
        # Sync with CI_TARGET_MAPPING.*.download_configs.images
        build_system_dlkm = True,
        build_system_dlkm_flatten = True,
        system_dlkm_fs_types = ["erofs", "ext4"],
        # Keep in sync with build.config.gki* MODULES_LIST
        modules_list = gki_system_dlkm_modules,
    )

    if build_gki_artifacts:
        gki_artifacts(
            name = name + "_gki_artifacts",
            kernel_build = name,
            boot_img_sizes = gki_boot_img_sizes,
            arch = arch,
        )
    else:
        native.filegroup(
            name = name + "_gki_artifacts",
            srcs = [],
        )

    # modules_staging_archive from <name>
    native.filegroup(
        name = name + "_modules_staging_archive",
        srcs = [name],
        output_group = "modules_staging_archive",
    )

    # All GKI modules
    native.filegroup(
        name = name + "_modules",
        srcs = [
            "{}/{}".format(name, module)
            for module in (module_implicit_outs or [])
        ],
    )

    # The purpose of this target is to allow device kernel build to include reasonable
    # defaults of artifacts from GKI. Hence, this target includes everything in name + "_dist",
    # excluding the following:
    # - UAPI headers, because device-specific external kernel modules may install different
    #   headers.
    # - DDK; see _ddk_artifacts below.
    native.filegroup(
        name = name + "_additional_artifacts",
        srcs = [
            # Sync with additional_artifacts_items
            name + "_headers",
            name + "_images",
            name + "_kmi_symbol_list",
            name + "_raw_kmi_symbol_list",
            name + "_gki_artifacts",
        ],
    )

    filegroup_extra_deps = [
        name + "_unstripped_modules_archive",
    ]
    kernel_filegroup_declaration(
        name = name + "_filegroup_declaration",
        kernel_build = name,
        extra_deps = filegroup_extra_deps,
        images = name + "_images",
        visibility = ["//visibility:private"],
    )
    target_mapping = CI_TARGET_MAPPING.get(name, {})
    write_file(
        name = name + "_download_configs",
        content = [
            json.encode_indent(target_mapping.get("download_configs", {})),
        ],
        # / is needed to distinguish between variants as 16k (and avoid conflicts).
        out = name + "/download_configs.json",
    )

    # Everything in name + "_dist" for the DDK.
    # These are necessary for driver development. Hence they are also added to
    # kernel_*_dist so they can be downloaded.
    ddk_artifacts = [
        name + "_download_configs",
        name + "_filegroup_declaration",
        name + "_unstripped_modules_archive",
    ]
    if ddk_headers_archive:
        ddk_artifacts.append(ddk_headers_archive)
    native.filegroup(
        name = name + "_ddk_artifacts",
        srcs = ddk_artifacts,
    )

    dist_targets = (extra_dist or []) + [
        name,
        name + "_uapi_headers",
        name + "_additional_artifacts",
        name + "_ddk_artifacts",
        name + "_modules",
        name + "_modules_install",
        # BUILD_GKI_CERTIFICATION_TOOLS=1 for all kernel_build defined here.
        Label("//build/kernel:gki_certification_tools"),
        "build.config.constants",
        Label("//build/kernel:init_ddk_zip"),
    ]

    kernel_sbom(
        name = name + "_sbom",
        srcs = dist_targets,
        kernel_build = name,
    )

    dist_targets.append(name + "_sbom")

    copy_to_dist_dir(
        name = name + "_dist",
        data = dist_targets,
        flat = True,
        dist_dir = "out/{name}/dist".format(name = name),
        log = "info",
    )

    kernel_abi_dist_name = name + "_abi_dist"
    kernel_abi_dist(
        name = kernel_abi_dist_name,
        kernel_abi = name + "_abi",
        kernel_build_add_vmlinux = _GKI_ADD_VMLINUX,
        data = dist_targets,
        flat = True,
        dist_dir = "out_abi/{name}/dist".format(name = name),
        log = "info",
    )

    kernel_abi_dist(
        name = name + "_abi_ignore_diff_dist",
        kernel_abi = name + "_abi",
        kernel_build_add_vmlinux = _GKI_ADD_VMLINUX,
        data = dist_targets,
        flat = True,
        dist_dir = "out_abi/{name}/dist".format(name = name),
        log = "info",
        ignore_diff = True,
        no_ignore_diff_target = kernel_abi_dist_name,
    )

    _define_common_kernels_additional_tests(
        name = name + "_additional_tests",
        kernel_build_name = name,
        kernel_modules_install = name + "_modules_install",
        modules = (module_implicit_outs or []),
        arch = arch,
    )

    native.test_suite(
        name = name + "_tests",
        tests = [
            name + "_additional_tests",
            name + "_test",
            name + "_modules_test",
        ],
    )

    kernel_compile_commands(
        name = name + "_compile_commands",
        kernel_build = name,
    )

    kernel_kythe(
        name = name + "_kythe",
        kernel_build = name,
        corpus = ":kernel_kythe_corpus",
    )

    copy_to_dist_dir(
        name = name + "_kythe_dist",
        data = [
            name + "_kythe",
        ],
        flat = True,
    )

# TODO(b/291918087): Delete once users have migrated to @gki_prebuilts
def _define_prebuilts(**kwargs):
    # Legacy flag for backwards compatibility
    # TODO(https://github.com/bazelbuild/bazel/issues/13463): alias to bool_flag does not
    # work. Hence we use a composite flag here.
    bool_flag(
        name = "use_prebuilt_gki",
        build_setting_default = False,
        # emit a warning if the legacy flag is used.
        deprecation = "Use {} or {} instead, respectively.".format(
            Label("//build/kernel/kleaf:use_prebuilt_gki"),
            Label("//build/kernel/kleaf:use_prebuilt_gki_is_true"),
        ),
    )
    native.config_setting(
        name = "local_use_prebuilt_gki_set",
        flag_values = {
            ":use_prebuilt_gki": "true",
        },
        visibility = ["//visibility:private"],
    )

    # Matches when --use_prebuilt_gki or --//<common_package>:use_prebuilt_gki is set
    selects.config_setting_group(
        name = "use_prebuilt_gki_set",
        match_any = [
            Label("//build/kernel/kleaf:use_prebuilt_gki_is_true"),
            ":local_use_prebuilt_gki_set",
        ],
    )

    for name, value in CI_TARGET_MAPPING.items():
        repo_name = value["repo_name"]
        deprecate_msg = "Use @{}//{} directly".format(repo_name, name)
        not_available_msg = "This will no longer be available. File a bug if you rely on this target."

        native.alias(
            name = name + "_downloaded",
            actual = name + "_files_downloaded",
            deprecation = deprecate_msg,
        )

        # A kernel_filegroup that:
        # - If --use_prebuilt_gki_num is set, use downloaded prebuilt of kernel_aarch64
        # - Otherwise build kernel_aarch64 from sources.
        native.alias(
            name = name + "_download_or_build",
            actual = select({
                ":use_prebuilt_gki_set": "@{}//{}".format(repo_name, name),
                "//conditions:default": name,
            }),
            deprecation = deprecate_msg,
            **kwargs
        )

        gki_artifacts_prebuilts(
            name = name + "_gki_artifacts_downloaded",
            srcs = select({
                Label("//build/kernel/kleaf:use_signed_prebuilts_is_true"): [name + "_boot_img_archive_signed_downloaded"],
                "//conditions:default": [name + "_boot_img_archive_downloaded"],
            }),
            outs = [name + "_gki_prebuilts_outs_downloaded"],
            deprecation = deprecate_msg,
        )

        native.filegroup(
            name = name + "_gki_artifacts_download_or_build",
            srcs = select({
                ":use_prebuilt_gki_set": [name + "_gki_artifacts_downloaded"],
                "//conditions:default": [name + "_gki_artifacts"],
            }),
            deprecation = deprecate_msg,
            **kwargs
        )

        files_by_target_suffix = {}
        for local_filename, config in value["download_configs"].items():
            files_by_target_suffix.setdefault(config["target_suffix"], []).append(local_filename)

        for target_suffix, files in files_by_target_suffix.items():
            native.filegroup(
                name = name + "_" + target_suffix + "_downloaded",
                srcs = ["@{}//{}".format(repo_name, filename) for filename in files],
                tags = ["manual"],
                deprecation = deprecate_msg,
            )

            # A filegroup that:
            # - If --use_prebuilt_gki_num is set, use downloaded prebuilt of kernel_{arch}_{target_suffix}
            # - Otherwise build kernel_{arch}_{target_suffix}
            native.filegroup(
                name = name + "_" + target_suffix + "_download_or_build",
                srcs = select({
                    ":use_prebuilt_gki_set": [":" + name + "_" + target_suffix + "_downloaded"],
                    "//conditions:default": [name + "_" + target_suffix],
                }),
                deprecation = deprecate_msg,
                **kwargs
            )

        additional_artifacts_items = [
            name + "_headers",
            name + "_images",
            name + "_kmi_symbol_list",
            name + "_gki_artifacts",
        ]

        native.filegroup(
            name = name + "_additional_artifacts_downloaded",
            srcs = [item + "_downloaded" for item in additional_artifacts_items],
            deprecation = not_available_msg,
        )

        native.filegroup(
            name = name + "_additional_artifacts_download_or_build",
            srcs = [item + "_download_or_build" for item in additional_artifacts_items],
            deprecation = not_available_msg,
        )

def _define_common_kernels_additional_tests(
        name,
        kernel_build_name,
        kernel_modules_install,
        modules,
        arch):
    fake_modules_options = Label("//build/kernel/kleaf/artifact_tests:fake_modules_options.txt")

    kernel_images(
        name = name + "_fake_images",
        kernel_modules_install = kernel_modules_install,
        build_initramfs = True,
        modules_options = fake_modules_options,
    )

    initramfs_modules_options_test(
        name = name + "_fake",
        kernel_images = name + "_fake_images",
        expected_modules_options = fake_modules_options,
    )

    write_file(
        name = name + "_empty_modules_options",
        out = name + "_empty_modules_options/modules.options",
        content = [],
    )

    kernel_images(
        name = name + "_empty_images",
        kernel_modules_install = kernel_modules_install,
        build_initramfs = True,
        # Not specify module_options
    )

    initramfs_modules_options_test(
        name = name + "_empty",
        kernel_images = name + "_empty_images",
        expected_modules_options = name + "_empty_modules_options",
    )

    device_modules_test(
        name = name + "_device_modules_test",
        srcs = [kernel_build_name + "_sources"],
        base_kernel_label = Label("{}//{}:{}".format(native.repository_name(), native.package_name(), kernel_build_name)),
        base_kernel_module = min(modules) if modules else None,
        arch = arch,
    )

    native.test_suite(
        name = name,
        tests = [
            name + "_empty",
            name + "_fake",
            name + "_device_modules_test",
        ],
    )

def define_db845c(
        name,
        outs,
        build_config = None,
        module_outs = None,
        make_goals = None,
        define_abi_targets = None,
        kmi_symbol_list = None,
        kmi_symbol_list_add_only = None,
        module_grouping = None,
        unstripped_modules_archive = None,
        gki_modules_list = None,
        dist_dir = None):
    """Define target for db845c.

    Note: This is a mixed build.

    Requires [`define_common_kernels`](#define_common_kernels) to be called in the same package.

    **Deprecated**. Use [`kernel_build`](kernel.md#kernel_build) directly.

    Args:
        name: name of target. Usually `"db845c"`.
        build_config: See [kernel_build.build_config](kernel.md#kernel_build-build_config). If `None`,
          default to `"build.config.db845c"`.
        outs: See [kernel_build.outs](kernel.md#kernel_build-outs).
        module_outs: See [kernel_build.module_outs](kernel.md#kernel_build-module_outs). The list of
          in-tree kernel modules.
        make_goals: See [kernel_build.make_goals](kernel.md#kernel_build-make_goals).  A list of strings
          defining targets for the kernel build.
        define_abi_targets: See [kernel_abi.define_abi_targets](kernel.md#kernel_abi-define_abi_targets).
        kmi_symbol_list: See [kernel_build.kmi_symbol_list](kernel.md#kernel_build-kmi_symbol_list).
        kmi_symbol_list_add_only: See [kernel_abi.kmi_symbol_list_add_only](kernel.md#kernel_abi-kmi_symbol_list_add_only).
        module_grouping: See [kernel_abi.module_grouping](kernel.md#kernel_abi-module_grouping).
        unstripped_modules_archive: See [kernel_abi.unstripped_modules_archive](kernel.md#kernel_abi-unstripped_modules_archive).
        gki_modules_list: List of gki modules to be copied to the dist directory.
          If `None`, all gki kernel modules will be copied.
        dist_dir: Argument to `copy_to_dist_dir`. If `None`, default is `"out/{name}/dist"`.

    Deprecated:
        Use [`kernel_build`](kernel.md#kernel_build) directly.
    """

    # buildifier: disable=print
    print("""{}//{}:{}: define_db845c is deprecated.

          Use [`kernel_build`](kernel.md#kernel_build) directly.

          Use https://r.android.com/2634654 and its cherry-picks as a reference
            on how to unfold the macro and use the other rules directly.
    """.format(native.package_relative_label(name), native.package_name(), name))

    if build_config == None:
        build_config = "build.config.db845c"

    if kmi_symbol_list == None:
        kmi_symbol_list = ":android/abi_gki_aarch64_db845c" if define_abi_targets else None

    if kmi_symbol_list_add_only == None:
        kmi_symbol_list_add_only = True if define_abi_targets else None

    if gki_modules_list == None:
        gki_modules_list = [":kernel_aarch64_modules"]

    if dist_dir == None:
        dist_dir = "out/{name}/dist".format(name = name)

    # Also refer to the list of ext modules for ABI monitoring targets
    _kernel_modules = []

    kernel_build(
        name = name,
        outs = outs,
        srcs = [":common_kernel_sources"],
        # List of in-tree kernel modules.
        module_outs = module_outs,
        build_config = build_config,
        # Enable mixed build.
        base_kernel = ":kernel_aarch64",
        kmi_symbol_list = kmi_symbol_list,
        collect_unstripped_modules = _COLLECT_UNSTRIPPED_MODULES,
        strip_modules = True,
        make_goals = make_goals,
    )

    # enable ABI Monitoring
    # based on the instructions here:
    # https://android.googlesource.com/kernel/build/+/refs/heads/main/kleaf/docs/abi_device.md
    # https://android-review.googlesource.com/c/kernel/build/+/2308912
    kernel_abi(
        name = name + "_abi",
        kernel_build = name,
        define_abi_targets = define_abi_targets,
        kernel_modules = _kernel_modules,
        kmi_symbol_list_add_only = kmi_symbol_list_add_only,
        module_grouping = module_grouping,
        unstripped_modules_archive = unstripped_modules_archive,
    )

    kernel_modules_install(
        name = name + "_modules_install",
        kernel_build = name,
        # List of external modules.
        kernel_modules = _kernel_modules,
    )

    merged_kernel_uapi_headers(
        name = name + "_merged_kernel_uapi_headers",
        kernel_build = name,
        kernel_modules = _kernel_modules,
    )

    kernel_images(
        name = name + "_images",
        build_initramfs = True,
        kernel_build = name,
        kernel_modules_install = name + "_modules_install",
    )

    dist_targets = [
        name,
        name + "_images",
        name + "_modules_install",
        # Mixed build: Additional GKI artifacts.
        ":kernel_aarch64",
        ":kernel_aarch64_additional_artifacts",
        name + "_merged_kernel_uapi_headers",
    ]

    copy_to_dist_dir(
        name = name + "_dist",
        data = dist_targets + gki_modules_list,
        dist_dir = dist_dir,
        flat = True,
        log = "info",
    )
