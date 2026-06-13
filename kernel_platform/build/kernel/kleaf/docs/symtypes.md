# `KBUILD_SYMTYPES`

Build symtypes files can be enabled with the `--kbuild_symtypes` flag.
For example:

```shell
$ bazel build --kbuild_symtypes //common:kernel_aarch64
```

```shell
$ bazel run --kbuild_symtypes //common:kernel_aarch64_dist
```

You may find the `*.symtypes` files under the
`bazel-bin/<package_name>/<target_name>/symtypes` directory,
where `<target_name>` is the name of the `kernel_build()`
macro. In the above example, the symtypes file can be found at

```
bazel-bin/common/kernel_aarch64/symtypes/
```

## Confirming the value of `--kbuild_symtypes`

You may build the following to confirm the value of `--kbuild_symtypes`:

```shell
$ tools/bazel build [flags] //build/kernel/kleaf:print_flags
```

**Note**: This only prints whether the flag is set or not for `kernel_build()`
with `kbuild_symtypes="auto"`. If a `kernel_build()` macro has
`kbuild_symtypes="true"` or `"false"`, the value of `KBUILD_SYMTYPES` is not
affected by the `--kbuild_symtypes` flag.
