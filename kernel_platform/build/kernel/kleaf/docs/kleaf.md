# Kleaf - Building Android Kernels with Bazel

### Prerequisites

There are no additional host dependencies. The Bazel toolchain and environment
are provided through `repo sync`. The corresponding entries are in the kernel
manifests.

As a convenience, installing a `bazel` host package allows the use of the
`bazel` command from anywhere in the tree (as opposed to using `tools/bazel`
from the top of the workspace) while still ensuring the correct version of the
toolchain drives the build. That works because Bazel binaries by convention
search for an executable at `$(dirname WORKSPACE)/tools/bazel` and defer
execution to that if existing. (Similarly, `tools/` could be added to `PATH`,
but this creates a dependency on that particular checked out workspace if used
across workspaces and also adds additional executables from `tools/` to `PATH`.
Hence, this method is not recommended.)

### Running a build

Android Common Kernels define at least a 'kernel' rule as part of the build
definition in the `common/` subdirectory. Building just a kernel is therefore as
simple as

```shell
$ tools/bazel build //common:kernel
```

Installing a host version of Bazel allows using that from `PATH` as a
convenience wrapper without affecting hermeticity. For example, you may install
`bazel` with `apt`:

```shell
$ apt-get install bazel
```

With a `bazel` in `PATH`, this reduces to

```shell
$ bazel build //common:kernel
```

and this command can be executed from any subdirectory below the top level
workspace directory.

`//common:kernel` refers by convention to the default kernel target and in the
case of the Android Common Kernels
([GKI](https://preview.source.android.com/devices/architecture/kernel/generic-kernel-image)),
this will usually be an alias for `kernel_aarch64`. Further targets can be
discovered via bazel's `query` subcommand:

```shell
$ bazel query "kind('py_binary', //common:*)"
```

### Distribution

Copy build artifacts to `DIST_DIR` for distribution by running the following
command.

```shell
$ tools/bazel run //common:kernel_dist
```

You may override the destination of distribution directory in the command line
via the `--dist_dir` argument. The `--dist_dir` is an
argument to the `copy_to_dist_dir` script, not to Bazel. Hence, put them after 
the `--` delimiter.

```shell
$ tools/bazel run //common:kernel_dist -- --dist_dir=out/dist
```

#### Cloud Android

```shell
$ tools/bazel run //common-modules/virtual-device:virtual_device_x86_64_dist
```

## Build definitions

The `kernel_build()` macro provided by this package is to be used in
`BUILD.bazel` build files to define kernel build targets. The simplest example
is (defining the GKI build):

```
load("//build/kernel/kleaf:kernel.bzl", "kernel_build")

kernel_build(
    name = "kernel",
    outs = ["vmlinux"],
    build_config = "common/build.config.gki.aarch64",
    srcs = glob(["**"]),
)
```

The `kernel_module()` macro defines a kernel module target. Example:

```
load("//build/kernel/kleaf:kernel.bzl", "kernel_module")

kernel_module(
    name = "nfc",
    srcs = glob(["**"]),
    outs = [
        "nfc.ko",
    ],
    kernel_build = "//common:kernel",
)
```

### Building your kernels and drivers

See instructions to build your own kernels and drivers with Bazel in
[Build your kernels and drivers with Bazel](impl.md).

### Documentation

See [API Reference and Documentation for all rules](api_reference.md)

## Availability

*Kleaf* is available for Android 13 and later kernels.

## FAQ

**Question:** How can I try it out?

With a recent `repo` checkout of `common-android-mainline`, the simplest
invocation is `tools/bazel build //common:kernel`.

**Question:** Are `BUILD_CONFIG` files still a thing?

Yes! `build.config` files still describe the build environment. Though they get
treated as hermetic input. Further, some features might not be supported yet or
never will be as they do not make sense in a Bazel-based build (e.g.
`SKIP_MRPROPER` is implicit).

**Question:** Why the name "*Kleaf*"?

The Android Platform Build with Bazel is sometimes referred to as Roboleaf
(Robo=Android, Bazel...Basil...Leaf). The kernel variant of that is *Kleaf*.

## Addtional Links

[Bazel](http://bazel.build)
