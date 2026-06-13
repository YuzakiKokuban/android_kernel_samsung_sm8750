# Build your kernels and drivers with Bazel

**Note**:
You may view the documentation for the following Bazel rules and macros on
Android Continuous Integration. See
[API Reference and Documentation for all rules](api_reference.md).

## Setting up the workspace

* (Recommended) To use Kleaf tooling as a dependent Bazel module, see
  [Setting up DDK workspace](ddk/workspace.md).
* To use Kleaf tooling as the root Bazel module, see
  [Use @kleaf as root module (legacy)](bzlmod.md#use-kleaf-as-root-module-legacy).
* Building without Bzlmod is deprecated and will not be supported in
  Android 16 and above.

## Building a custom kernel

**WARNING**: It is recommended to use the common Android kernel
under `//common` (the so-called "mixed build") instead of building a custom
kernel.

You may define a `kernel_build` target to build a custom kernel. The name of
the `kernel_build` target is usually the name of your device, e.g. `tuna`.

The `outs` attribute of the target should align with the `FILES` variable in
build.config. This may include DTB files and kernel images, e.g. `vmlinux`.

The `module_outs` attribute of the target includes the list of in-tree drivers
that you are building. See section to [build in-tree drivers (Step 1)](#step-1)
below.

```
load("//build/kernel/kleaf:kernel.bzl","kernel_build")
load("//build/kernel/kleaf:common_kernels.bzl", "arm64_outs")
kernel_build(
   name = "tuna",
   srcs = glob(
       ["**"],
       exclude = [
           "**/BUILD.bazel",
           "**/*.bzl",
           ".git/**",
       ],
   ),
   outs = arm64_outs,
   build_config = "build.config.tuna",
)
```

## Building kernel modules and DTB files

### Step 0: (Optional) Create a skeleton `BUILD.bazel` file

This step automates most of the following steps for you.

First, install
[Buildozer](https://github.com/bazelbuild/buildtools/tree/master/buildozer).
Make sure that it is available in `$PATH`, or under `$GOPATH/bin`, or under
`$HOME/go/bin`. See the script below for details on how `buildozer` is searched
for.

Next, execute `build_config_to_bazel.py` script. Set `BUILD_CONFIG` accordingly
if you don't have a top level `build.config` file. Example:

```shell
$ BUILD_CONFIG=common-modules/virtual-device/build.config.virtual_device.x86_64 \
    build/kernel/kleaf/build_config_to_bazel.py
```

Sample output:

```text
fixed /home/elsk/android/kernel/common-modules/virtual-device/BUILD.bazel
```

Then, examine the modified file(s), indicated in the command output.
There may be several `FIXME` comments that requires human intervention.
Go through the steps below to fix them accordingly.

**NOTE**: Human intervention is still required for the generated file.

**NOTE**: The script may modify multiple files. All of them should be examined.

**NOTE**: The file is generated based on a number of heuristics. Even if some
attributes aren't commented with `FIXME`, they may not be 100% correct. Go
through the steps below to fix the file to suit your needs.

### Step 1: (Optional) Define a target to build in-tree drivers and DTB files {#step-1}

If you have a separate kernel tree to build in-tree drivers, define
a `kernel_build` target to build these modules. The name of the `kernel_build`
target is usually the name of your device, e.g. `tuna`.

If you also have external kernel modules to be built, be sure to set visibility
accordingly, so that the targets to build external kernel modules can refer to
this `kernel_build` target.

If you are building a custom kernel, you may reuse the existing `kernel_build`
target, and keep kernel images in `outs`. If you are building against GKI, set
the `base_kernel` attribute accordingly (e.g. to `//common:kernel_aarch64`).

The `build_config` attribute of the target should point to the
main `build.config` file. To use `build.config` files generated on the fly, you
may use the `kernel_build_config` rule. See example for Pixel 2021 below.

The `outs` attribute of the target should align with the `FILES` variable in
build.config. This may include DTB files.

The `module_outs` attribute of the target includes the list of in-tree drivers
that you are building.

* Hint: You may leave the list empty and build the target. If the list is not
  up to date, modify the list according to the error message.

**Note**: It is recommended that kernel modules are moved out of the kernel tree
to be built as external kernel modules. This means keeping the list
of `module_outs` empty or as short as possible. See Step 2 for building external
kernel modules.

For other build configurations defined in the `build.config` file, see
[build_configs.md](build_configs.md).

Example for Pixel 2021 (see the `kernel_build` target named `slider`):

[https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel)

### Step 2: Define targets to build external kernel modules

Define `kernel_module` targets to build external kernel modules. You should
create a `kernel_module` target for each item in `EXT_MODULES` variable
in `build.config`.

The `kernel_build` attribute should be the target to the `kernel_build` you have
previously created in step 1, or  `//common:kernel_aarch64` if you did not do
step 1.

The `outs` attribute should be set to a list of `*.ko` files built by this
external module.

* Hint: You may leave the list empty and build the target. If the list is not
  up to date, modify the list according to the error message.

Be sure to set visibility accordingly, so that these targets are visible to
the `kernel_modules_install` target that will be created in step 3.

If the module depends on other modules, set `kernel_module_deps` accordingly.
See the `bms` and `power/reset` module below for an example.

If the module depends on headers in other locations, add headers to a filegroup,
then add the headers to `srcs`. See the `bms` and `power/reset` module below for
an example.

Minimal example for the `edgetpu` driver of Pixel 2021:

[https://android.googlesource.com/kernel/google-modules/edgetpu/+/refs/heads/android-gs-raviole-mainline/drivers/edgetpu/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/edgetpu/+/refs/heads/android-gs-raviole-mainline/drivers/edgetpu/BUILD.bazel)

Example for the `bms` driver of Pixel 2021:

[https://android.googlesource.com/kernel/google-modules/bms/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/bms/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel)

Example for the `power/reset` driver of Pixel 2021:

[https://android.googlesource.com/kernel/google-modules/power/reset/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/power/reset/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel)

### Step 3: Define a target to run `depmod`

Define a `kernel_modules_install` target that includes all external kernel
modules created in Step 2. This is equivalent to running `make modules_install`,
which runs `depmod`.

The name of the target is usually the name of your device
with `_modules_install` appended to it, e.g. `tuna_modules_install`.

See Step 2 to determine the `kernel_build` attribute of the target.

Example for Pixel 2021 (see the `kernel_modules_install` target
named `slider_modules_install`):

[https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel)

### Step 4: (Optional) Define a target to build all boot images

The `kernel_images` macro produces partition images that are ready to be flashed
and tested immediately on your device. It can build the `initramfs`
image, `boot` image, `vendor_boot` image, `vendor_dlkm` image, `system_dlkm` image, etc.

The name of the target is usually the name of your device with `_images`
appended to it, e.g. `tuna_images`.

If you do not need to build any partition images, skip this step.

Example for Pixel 2021 (see the `kernel_images` target named `slider_images`):

[https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel)

### Step 5: Define a target for distribution {#step-5}

Define a `copy_to_dist_dir` target that includes the targets you want in the
distribution directory. The name of this `copy_to_dist_dir` target is usually
the name of your device with `_dist` appended to it, e.g. `tuna_dist`.

Set `flat = True` so the directory structure within `dist_dir` is flattened.

Set `dist_dir` so there's less typing at build time. For example:

```text
copy_to_dist_dir(
   dist_dir = "out/dist"
)
```

Add the following to the `data` attribute of the `copy_to_dist_dir` target:

* The name of the `kernel_build` you have created in Step 1,
  e.g. `:tuna`. This adds all `outs`
  and `module_outs` to the distribution directory.
  * This usually includes DTB files and in-tree kernel modules.
* The name of the `kernel_modules_install` target you have created in Step 3.
  You may skip the `kernel_modules` targets created in Step 2, because
  the `kernel_modules_install` target includes all `kernel_modules` targets.
  This copies all external kernel modules to the distribution directory.
* The name of the `kernel_images` target you have created in Step 4. This copies
  all partition images to the distribution directory.
* GKI artifacts, including:
  * `//common:kernel_aarch64`
  * `//common:kernel_aarch64_additional_artifacts`
* UAPI headers, e.g. `//common:kernel_aarch64_uapi_headers`
* GKI modules
  * If you are using all GKI modules, add `//common:kernel_aarch64_modules`.
  * If you are using part of the GKI modules, add them individually, e.g.:
    * `//common:kernel_aarch64/zram.ko`
    * `//common:kernel_aarch64/zsmalloc.ko`
  * Modules from the device kernel build with the same name as GKI modules
    (e.g. on android13-5.15, you have `zram.ko` in `kernel_build.module_outs`)
    does not need to be specified, because `module_outs` are added to
    distribution.

Example for Pixel 2021 (see the `copy_to_dist_dir` target named `slider_dist`):

[https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android13-gs-raviole-5.15/BUILD.bazel)

### Step 6: Build, flash and test

```shell
# Optional: prepare the device by flashing a base build.
# During development, you may want to wipe, disable verity and disable verification.
# fastboot update tuna-img.zip -w --disable-verity --disable-verification

# Assuming dist_dir=out/dist
$ tools/bazel run //private/path/to/sources:tuna_dist
# Flash static partitions
$ fastboot flash boot out/dist/boot.img
$ fastboot flash system_dlkm out/dist/system_dlkm.img
$ fastboot flash vendor_boot out/dist/vendor_boot.img
$ fastboot flash dtbo out/dist/dtbo.img
$ fastboot reboot fastboot
# Flash dynamic partitions
$ fastboot flash vendor_dlkm out/dist/vendor_dlkm.img
$ fastboot reboot
```

## Resolving common errors

See [errors.md](errors.md).

## Handling SCM version

See [scmversion.md](scmversion.md).

## Advanced usage

### Disable LTO during development

See [lto.md](lto.md).

### Using configurable build attributes `select()`

See official Bazel documentation for `select()`
here: https://docs.bazel.build/versions/main/configurable-attributes.html

In general, inputs to a target are configurable, while declared outputs are not.
One exception is that the `kernel_build` rule provides limited support
of `select()` in `outs` and `module_outs` attributes. See
[documentations](api_reference.md) of `kernel_build` for details.

### .bazelrc files

By default, the `.bazelrc` (symlink to `build/kernel/kleaf/common.bazelrc`)
tries to import the following two files if they exist:

* `device.bazelrc`: Device-specific bazelrc file (e.g. GKI prebuilt settings)
* `user.bazelrc`: User-specific bazelrc file (e.g. LTO settings)

To add device-specific configurations, you may create a `device.bazelrc`
file in the device kernel tree, then create a symlink at the repo root.

### Notes on hermeticity

Bazel builds are hermetic by default. Hermeticity is ensured by manually
declaring each target to depend on `//build/kernel:hermetic-tools`.

At this time of writing (2022-03-08), the following binaries are still
expected from the environement, or host machine, to build the kernel with
Bazel, in addition to the list of the allowlist of host tools specified in
`//build/kernel:hermetic-tools`. This is because the following usage does
not depend on `//build/kernel:hermetic-tools`.
* `cp` used by [`copy_file`](https://github.com/bazelbuild/bazel-skylib/blob/main/rules/copy_file.bzl)
  in `copy_to_dist_dir` rules
* `echo`, `readlink`, `git` used by `build/kernel/kleaf/workspace_status.sh`
