# Sandboxing

## TL;DR

To reduce sandboxes and boost build time, build with `--config=local`.

Example:

```shell
$ tools/bazel run --config=local //common:kernel_aarch64 -- --dist_dir=out/dist
```

Or add to `user.bazelrc`:

```text
# user.bazelrc
build --config=local
```

## How does this work?

By default, all [actions](https://bazel.build/reference/glossary#action) runs
within a sandbox. Sandboxes ensures hermeticity, but also introduced extra
overhead at build time:

- Creating the sandbox needs time, especially when there are too many inputs
- Using sandboxes disallows caching of `$OUT_DIR`

To overcome this and boost build time, a few types of actions are executed
without the sandbox when `--config=local`. The exact list of types of actions
are an implementation detail. If other types of actions were executed without
the sandbox, they might interfere with each other when executed in parallel.

When building with `--config=local`, `$OUT_DIR` is cached.

To clean the cache, run

```shell
$ tools/bazel clean
```

**NOTE**: It is recommended to execute `tools/bazel clean` whenever you switch
from and to `--config=fast`. Otherwise, you may get surprising cache hits or
misses because changing `--strategy` does **NOT** trigger rebuilding of an
action.

## Naming

The name of the config `local` comes from the value `local` in `--strategy`. See
Bazel's official documentation on `--strategy`
[here](https://bazel.build/reference/command-line-reference#flag--strategy).

## SCM version

If you do not care about stamping local builds during development, it is
advised that you do not specify `--config=stamp`. In this case, any change to
the SCM version alone (e.g. edit commit message, rebase commits, etc.) does not
trigger a rebuild at all during incremental builds. Rebuilding a `kernel_build`
or `kernel_module` target usually finishes within seconds.

If you do care about stamping local builds, you may `--config=stamp`. In
that case, a change in the SCM version (e.g. edit commit message, rebase
commits, etc.) invalidates caches at the Bazel level, causing `kernel_build`
or `kernel_module`s to rebuild if you execute `tools/bazel build` in
incremental builds. You may combine the flag with `--config=local` to cache
the `$OUT_DIR` and shorten incremental build times.

See [scmversion.md](scmversion.md).

## Local cache dir

The `--config=local` mode makes use of a persistent `$OUT_DIR`
across invocations to cache the rule execution state. The default cache
directory is `$WORKSPACE/out/cache`, but can be overridden by passing
`--cache_dir=/some/fast/disk` in order to make use of a file system that
performs well or better for the kernel build workload. Full example:

```shell
$ tools/bazel run --config=local --cache_dir=/some/fast/disk //common:kernel_aarch64_dist
```

If you have built multiple `kernel_build` before and/or with different
configurations (e.g. LTO), there may be multiple subdirectories under
the cache directory.

Usually, a symlink named `last_build` points to the `COMMON_OUT_DIR` from
building the last `kernel_build`. The destination of the symlink may be
unexpected if:

- There are multiple `kernel_build`'s building in the same `bazel` command
- Bazel cached the build result so the last `bazel` command doesn't actually
  build anything.

Sample directory structure:

```text
out/cache
├── 39c6af8c
├── 5f914ca4
└── last_build -> 5f914ca4
```

To understand what `kernel_build` is built with a given cache directory and
relevant configurations to build it, check the `kleaf_config_tags.json` file
under the subdirectories:

```shell
$ tail -n +1 */kleaf_config_tags.json
```

Sample output snippet:

```text
==> last_build/kleaf_config_tags.json <==
{
  "@//build/kernel/kleaf/impl:force_add_vmlinux": false,
  "@//build/kernel/kleaf/impl:force_ignore_base_kernel": false,
  "@//build/kernel/kleaf/impl:preserve_cmd": false,
  "@//build/kernel/kleaf/impl:force_disable_trim": false,
  "@//build/kernel/kleaf:gcov": false,
  "@//build/kernel/kleaf:kasan": true,
  "@//build/kernel/kleaf:kbuild_symtypes": false,
  "@//build/kernel/kleaf:kmi_symbol_list_strict_mode": true,
  "@//build/kernel/kleaf:lto": "none",
  "_kernel_build": "@//common:kernel_aarch64"
}
```

## Other flags

The flag `--config=local` is also implied by other flags, e.g.:

* `--config=fast`. See [fast.md](fast.md).

## Common issues

It is possible to see `Read-only file system` errors if a previous
`--config=local` build was interrupted, especially when it was
building the defconfig file.

See [errors.md#defconfig-readonly](errors.md#defconfig-readonly) for solutions.

If you see
```text
unterminated call to function 'wildcard': missing ')'.  Stop.
```

This is a known issue. See
[errors.md](errors.md#unterminated-call-to-function-wildcard) for explanation.
