# `get_kernel_output`

The `OUT_DIR` is hidden inside Kleaf, and we cannot get the path of `OUT_DIR`
before build system starting. Unfortunately, some of automatically debugging and
analyzing tools which need static `OUT_DIR` path would be broken.

`--preserve_kbuild_output` is for compatibility with Linux build which get `O`,
a.k.a. `OUT_DIR` in Kleaf, easily and get everything unconditionally.

This is only for debugging or analyzing which would NOT affect any sandbox or caching
mechanism (e.g. config=local) in Bazel.

When the flag `--preserve_kbuild_output` is set, the `OUT_DIR` would be rsynced
to bazel output.

For example:

```shell
$ bazel build --preserve_kbuild_output //common:kernel_aarch64
```

You may find the `OUT_DIR` directory under the
`bazel-bin/<package_name>/<target_name>/kbuild_output` directory,
where `<target_name>` is the name of the `kernel_build()`
macro. In the above example, the `OUT_DIR` files can be found at

```
bazel-bin/common/kernel_aarch64/kbuild_output/
```
