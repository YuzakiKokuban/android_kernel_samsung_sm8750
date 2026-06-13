# Driver Development Kit (DDK)

## Objective

The Driver Development Kit (DDK) shall provide an easy way to develop Kernel
modules for GKI kernels. It is suitable for new modules as well as for
existing modules and regardless of their location within the source tree.

## Benefits of using DDK

* DDK generates Makefiles that can be used during upstream contribution to the
  Linux project.
* DDK supports kernel module definitions for GKI kernels, possibly with
  reasonable migration steps.
* DDK ensures correct toolchain use (compilers, linkers, flags, etc.).
* DDK ensures correct visibility of resources provided by the GKI kernels
  (such as headers, Makefiles)
* DDK allows simple definition of kernel modules in one-place (avoiding separate
  definition locations)
* DDK avoids unnecessary boilerplate (such as similarly looking Makefiles)
  generated during the make process.

## Example on the Virtual Device

The virtual device serves as a reference implementation for DDK modules. See
[`BUILD.bazel` for virtual devices](https://android.googlesource.com/kernel/common-modules/virtual-device/+/refs/heads/android-mainline/BUILD.bazel)
.

## Read more

[Setting up workspace](workspace.md)

[Rules and macros](rules.md)

[Using headers from the common kernel](common_headers.md)

[Handling include directories](includes.md)

[Configuring DDK module](config.md)

[Resolving common errors](errors.md)
