# Configuring kernel\_build

## Modify defconfig

To run `make *config` or `config.sh` in Kleaf, follow the following steps.

### Step 1: Run the following Kleaf command

Run

```shell
$ tools/bazel run <name_of_kernel_build>_config [-- [menuconfig|nconfig|savedefconfig...]]
```

... where `<name_of_kernel_build>` is the name of the `kernel_build` target with
the requested build config.

The menu command (`menuconfig`, `xconfig`, etc.) must be provided to the
underlying executable, so they need to be provided after `--`. See
[Running executables](https://bazel.build/docs/user-manual#running-executables).
If nothing is provided, the default is `menuconfig`.

Example:

```shell
$ tools/bazel run //common:kernel_aarch64_config

$ tools/bazel run //common:kernel_x86_64_config -- nconfig
```

## Defconfig fragments

When building a `kernel_build` target, the following list of defconfig
fragments are applied on `.config`.

*   `kernel_build.defconfig_fragments`
*   `--defconfig_fragment`
*   defconfig fragments from other command line flags and other `kernel_build`
    attributes

See `kernel_build` in [documentation for all rules](api_reference.md) for
details.

The order does not matter. After `.config` is built, `.config` is checked
against each defconfig fragment to ensure that all defconfig fragments are
properly applied.

### kernel\_build.defconfig\_fragments

The convention is that the files should be named `X_defconfig`, where
`X` describes what the defconfig fragment does.

Example:

```python
# path/to/tuna/BUILD.bazel
kernel_build(
    name = "tuna",
    defconfig_fragments = ["tuna_defconfig"],
    ...
)
```
```shell
# path/to/tuna/tuna_defconfig

# Precondition:
#   CONFIG_TUNA_GRAPHICS must already be declared in kernel_build.kconfig_ext
CONFIG_TUNA_GRAPHICS=y
```

### --defconfig_fragment flag

You may specify a **single** target in the `--defconfig_fragment` flag to
add defconfig fragment(s) via the command line. To refer to a file in the
source tree, the file must already be exported via
[exports_files](https://bazel.build/reference/be/functions#exports_files)
or included in a
[filegroup](https://bazel.build/reference/be/general#filegroup).

**NOTE**: If multiple `--defconfig_fragment` are supplied, only the last
one takes effect.

The convention is that the files should be named `X_defconfig`, where
`X` describes what the defconfig fragment does.

Example:

```python
# path/to/tuna/BUILD.bazel
exports_files([
    "kasan_hw_tags_defconfig",
])
kernel_build(name = "tuna", ...)
```
```shell
# kasan_hw_tags_defconfig
CONFIG_KASAN=y
CONFIG_KASAN_HW_TAGS=y
# CONFIG_KASAN_SW_TAGS is not set
# etc. Add your configs!
```
```shell
$ tools/bazel build \
    --defconfig_fragment=//path/to/tuna:kasan_hw_tags_defconfig \
    //path/to/tuna:tuna
```

To specify multiple fragments in the flag, use a
[filegroup](https://bazel.build/reference/be/general#filegroup).

Example:

```python
# path/to/tuna/BUILD.bazel
filegroup(
    name = "all_kasan_defconfigs",
    srcs = ["kasan_defconfig", "lto_none_defconfig"]
)
kernel_build(name = "tuna", ...)
```
```shell
$ tools/bazel build \
    --defconfig_fragment=//path/to/tuna:all_kasan_defconfigs \
    //path/to/tuna:tuna
```

### Other pre-defined flags

There are a few pre-defined command-line flags and attributes on `kernel_build`
that are commonly used. When these flags and/or attributes are set, additional
defconfig fragments are applied on `.config`, and checked after `.config` is
built. It is recommended to use these common flags instead of defining your
own defconfig fragments to avoid fragmentation in the ecosystem (pun intended).

*   `--btf_debug_info`
*   `--debug`
*   `--gcov`
*   `--kasan`
*   `--kasan_sw_tags`
*   `--kasan_generic`
*   `--kcsan`
*   `--page_size`
*   `--rust` / `--norust`

**NOTE**: w.r.t. to KMI, the following flags will disable both `TRIM_UNUSED_KSYMS`
(by not setting it) and `MODULE_SIG_PROTECT`(by explicitly turning it off):
(`--notrim`, `--debug`, `--gcov`, `--k*san`, `--kgdb`).

### User-defined flags

To control `kernel_build.defconfig_fragments` with command line flags,
you may use
[configurable build attributes](https://bazel.build/docs/configurable-attributes)
(sometimes referred to as `select()`).

Example:

```python
bool_flag(
    name = "khwasan",
    build_setting_default = False,
)

config_setting(
    name = "khwasan_is_set",
    flag_values = {":khwasan": "true"},
)

kernel_build(
    name = "tuna",
    defconfig_fragments = select({
        ":khwasan_is_set": ["khwasan_defconfig"],
        "//conditions:default": []
    }) + [...],
    ...
)
```
```shell
$ tools/bazel build --//path/to/tuna:khwasan //path/to/tuna:tuna
```

Use [device.bazelrc](impl.md#bazelrc-files) to shorten flags:

```text
# device.bazelrc
build --flag_alias=khwasan=--//path/to/tuna:khwasan
```

```shell
$ tools/bazel build --khwasan //path/to/tuna:tuna
```

To shorten `--defconfig_fragment` flags, you may use
[`--config`](https://bazel.build/run/bazelrc#config) in `device.bazelrc`:

```text
# device.bazelrc
build:kasan_hw_tags --defconfig_fragment=//path/to/tuna:kasan_hw_tags_defconfig
```
```shell
$ tools/bazel build --config=kasan_hw_tags //path/to/tuna:tuna
```
