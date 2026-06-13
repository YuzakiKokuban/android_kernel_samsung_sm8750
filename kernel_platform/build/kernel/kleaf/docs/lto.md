# Configure LTO during development

**WARNING**: LTO is disabled by default on GKI (`gki_defconfig`) on
android14-6.1 and above. This
should only be modified when the trade-offs are fully understood.

Building with link-time optimization (LTO) may take a very long time that brings
little benefit during development. You may disable LTO to shorten the build time
for development purposes.

## Confirming the value of LTO

The default value for LTO is set per branch (i.e. `mainline`'s default might
differ from the `android14-5.15` one, etc).

You may examine the defconfig of a `kernel_build` to see the default LTO
setting. For GKI, this is `gki_defconfig` in

```text
common/arch/arm64/configs/gki_defconfig
common/arch/x86/configs/gki_defconfig
common/arch/riscv/configs/gki_defconfig
```

## Option 1: Explicitly disabling LTO in command line

For example:

```shell
$ tools/bazel build --lto=none //private/path/to/sources:tuna_dist
```

The `--lto` option is applied to the build, not the `copy_to_dist_dir` step.
Hence, put it before the `--` delimiter when running a `*_dist` target. For
example:

```shell
$ tools/bazel run --lto=none //private/path/to/sources:tuna_dist -- --dist_dir=out/dist
```

## Option 2: Disable LTO for this workspace

You only need to **do this once** per workspace.

```shell
# Do this at workspace root next to the file WORKSPACE
$ test -f WORKSPACE && echo 'build --lto=none' >> user.bazelrc
# Future builds in this workspace always disables LTO.
$ tools/bazel build //private/path/to/sources:tuna_dist
```

## Known issues

As of 2024-04-30, there may be incremental build issues with
LTO due to caching. See

[Issue 2021: With LTO and LTO cache, .incbin in assembly is not handled properly during incremental builds](https://github.com/ClangBuiltLinux/linux/issues/2021).

## See also

[kasan](kasan.md)
