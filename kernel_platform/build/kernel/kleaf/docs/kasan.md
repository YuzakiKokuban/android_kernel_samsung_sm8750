# The Kernel Address Sanitizer (KASAN)

To build with [KASAN](https://docs.kernel.org/dev-tools/kasan.html) enabled, add
the `--kasan` flag. Example:

```shell
$ tools/bazel build --kasan //common:kernel_aarch64
```

Alternatively, use the `sanitizers` attribute of `kernel_build`:

```python
kernel_build(
    name = "tuna_kasan",
    sanitizers = ["kasan_any_mode"],
    ...
)
```

Kasan builds automatically disable LTO by setting its value to `none`.
If `--lto` is explicitly specified, it must be set to `none`. Otherwise the
build fails.

## Interaction with `--config=fast`

`--config=fast` specifies `--lto=thin`, which is not allowed by `--kasan`. If
you want to disable some sandboxes to make incremental build faster,
use `--config=local` instead. For example:

```shell
$ tools/bazel build --kasan --config=local //common:kernel_aarch64
```

## Confirming the value of `--kasan`

You may build the following to confirm the value of kasan setting:

```shell
$ tools/bazel build [flags] //build/kernel/kleaf:print_flags
```

Note: the value of `--lto` reflects the value specified on the command line,
which may show `default` or `none`. However, if `--kasan` is specified,
`--lto` is coerced into `none`, no matter if it is specified on the command
line.

## See also

[LTO](lto.md)

[Sandboxing](sandbox.md)
