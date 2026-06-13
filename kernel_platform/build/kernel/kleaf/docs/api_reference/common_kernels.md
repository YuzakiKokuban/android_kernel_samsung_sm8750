<!-- Generated with Stardoc: http://skydoc.bazel.build -->

Functions that are useful in the common kernel package (usually `//common`).

<a id="define_common_kernels"></a>

## define_common_kernels

<pre>
define_common_kernels(<a href="#define_common_kernels-branch">branch</a>, <a href="#define_common_kernels-target_configs">target_configs</a>, <a href="#define_common_kernels-toolchain_version">toolchain_version</a>, <a href="#define_common_kernels-visibility">visibility</a>)
</pre>

Defines common build targets for Android Common Kernels.

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


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="define_common_kernels-branch"></a>branch |  **Deprecated**. This attribute is ignored.<br><br>This used to be used to calculate the default `--dist_dir`, which was `out/{branch}/dist`. This was expected to be the value of `BRANCH` in `build.config`. If not set, it was loaded from `common/build.config.constants` **in `//{common_kernel_package}`** where `common_kernel_package` was supplied to `define_kleaf_workspace()` in the `WORKSPACE` file. Usually, `common_kernel_package = "common"`. Hence, if `define_common_kernels()` was called in a different package, it was required to be supplied.<br><br>Now, the default value of `--dist_dir` is `out/{name}/dist`, so the value of `branch` has no effect. Hence, the attribute is ignored.   |  `None` |
| <a id="define_common_kernels-target_configs"></a>target_configs |  A dictionary, where keys are target names, and values are a dictionary of configurations to override the default configuration for this target.   |  `None` |
| <a id="define_common_kernels-toolchain_version"></a>toolchain_version |  If not set, use default value in `kernel_build`.   |  `None` |
| <a id="define_common_kernels-visibility"></a>visibility |  visibility of the `kernel_build` and targets defined for downloaded prebuilts. If unspecified, its value is `["//visibility:public"]`.<br><br>See [`visibility`](https://docs.bazel.build/versions/main/visibility.html).   |  `None` |


<a id="define_db845c"></a>

## define_db845c

<pre>
define_db845c(<a href="#define_db845c-name">name</a>, <a href="#define_db845c-outs">outs</a>, <a href="#define_db845c-build_config">build_config</a>, <a href="#define_db845c-module_outs">module_outs</a>, <a href="#define_db845c-make_goals">make_goals</a>, <a href="#define_db845c-define_abi_targets">define_abi_targets</a>,
              <a href="#define_db845c-kmi_symbol_list">kmi_symbol_list</a>, <a href="#define_db845c-kmi_symbol_list_add_only">kmi_symbol_list_add_only</a>, <a href="#define_db845c-module_grouping">module_grouping</a>, <a href="#define_db845c-unstripped_modules_archive">unstripped_modules_archive</a>,
              <a href="#define_db845c-gki_modules_list">gki_modules_list</a>, <a href="#define_db845c-dist_dir">dist_dir</a>)
</pre>

Define target for db845c.

Note: This is a mixed build.

Requires [`define_common_kernels`](#define_common_kernels) to be called in the same package.

**Deprecated**. Use [`kernel_build`](kernel.md#kernel_build) directly.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="define_db845c-name"></a>name |  name of target. Usually `"db845c"`.   |  none |
| <a id="define_db845c-outs"></a>outs |  See [kernel_build.outs](kernel.md#kernel_build-outs).   |  none |
| <a id="define_db845c-build_config"></a>build_config |  See [kernel_build.build_config](kernel.md#kernel_build-build_config). If `None`, default to `"build.config.db845c"`.   |  `None` |
| <a id="define_db845c-module_outs"></a>module_outs |  See [kernel_build.module_outs](kernel.md#kernel_build-module_outs). The list of in-tree kernel modules.   |  `None` |
| <a id="define_db845c-make_goals"></a>make_goals |  See [kernel_build.make_goals](kernel.md#kernel_build-make_goals).  A list of strings defining targets for the kernel build.   |  `None` |
| <a id="define_db845c-define_abi_targets"></a>define_abi_targets |  See [kernel_abi.define_abi_targets](kernel.md#kernel_abi-define_abi_targets).   |  `None` |
| <a id="define_db845c-kmi_symbol_list"></a>kmi_symbol_list |  See [kernel_build.kmi_symbol_list](kernel.md#kernel_build-kmi_symbol_list).   |  `None` |
| <a id="define_db845c-kmi_symbol_list_add_only"></a>kmi_symbol_list_add_only |  See [kernel_abi.kmi_symbol_list_add_only](kernel.md#kernel_abi-kmi_symbol_list_add_only).   |  `None` |
| <a id="define_db845c-module_grouping"></a>module_grouping |  See [kernel_abi.module_grouping](kernel.md#kernel_abi-module_grouping).   |  `None` |
| <a id="define_db845c-unstripped_modules_archive"></a>unstripped_modules_archive |  See [kernel_abi.unstripped_modules_archive](kernel.md#kernel_abi-unstripped_modules_archive).   |  `None` |
| <a id="define_db845c-gki_modules_list"></a>gki_modules_list |  List of gki modules to be copied to the dist directory. If `None`, all gki kernel modules will be copied.   |  `None` |
| <a id="define_db845c-dist_dir"></a>dist_dir |  Argument to `copy_to_dist_dir`. If `None`, default is `"out/{name}/dist"`.   |  `None` |

**DEPRECATED**

Use [`kernel_build`](kernel.md#kernel_build) directly.


