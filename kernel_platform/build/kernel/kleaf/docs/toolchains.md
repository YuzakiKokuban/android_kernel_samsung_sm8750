# Toolchain resolution in Kleaf

## C toolchains

The C toolchains are registered with the standard type
`@bazel_tools//tools/cpp:toolchain_type`. For details about C toolchains
registered by default, refer to `prebuilts/clang/host/linux-x86/kleaf`.
For details about Bazel's C toolchain registration, see
[this tutorial](https://bazel.build/tutorials/ccp-toolchain-config).

For details about C toolchain resolution in Kleaf, see
`prebuilts/clang/host/linux-x86/kleaf/README.md`.

## cc\_* rules

When `--config=hermetic_cc` is set (this is the default),
for `cc_binary`, `cc_library` etc., the build system builds them against
the [execution platform](https://bazel.build/extending/platforms) (usually
Linux x86_64) with the toolchain version defined by `@kernel_toolchain_info`.

To build binaries for an Android device, build the targets against the
[target platform](https://bazel.build/extending/platforms) by using
[`--config=android*`](../bazelrc/platforms.bazelrc)
or wrapping the target with an `android_filegroup` rule. See
[build/kernel/kleaf/tests/cc_testing/BUILD.bazel](../tests/cc_testing/BUILD.bazel)
for examples.

## kernel\_* rules

Kleaf uses Bazel's toolchain resolution to determine the C toolchain
for `kernel_build`, etc.

Kleaf always uses the toolchain files (clang, ld, ar, etc.) from the
resolved toolchain. These files are usually consistent across all platforms
and architectures.

When `--incompatible_kernel_use_resolved_toolchains` is set, Kleaf uses
flags from the resolved toolchain to determine `USERCFLAGS`, `USERLDFLAGS`,
`HOSTCFLAGS`, `HOSTLDFLAGS` etc. for building the kernel. This requires
`kernel_build.arch` to be set properly, because the flags are different
for different architecture.

For implementation details, see [kernel_toolchains](../impl/kernel_toolchains.bzl).

## Hermetic toolchain

A hermetic toolchain is registered with the type `hermetic_toolchain.type`
(internally `//build/kernel:hermetic_tools_toolchain_type`).

All rules provided by Kleaf, including `hermetic_genrule` and `hermetic_exec`,
uses this registered toolchain.

Any custom rules that needs the hermetic toolchain should find it from toolchain
resolution. For details, see
[Hermeticity: custom rules](hermeticity.md#custom-rules).
