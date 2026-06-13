# Using headers from the common kernel

This document briefly explains the distinction between different targets in the
[Android Common Kernel (ACK) source tree](https://android.googlesource.com/kernel/common/)
.

The ACK source tree is usually checked out at the
`//common` [package](https://bazel.build/concepts/build-ref#packages), but it
may be checked out at other places. See [workspace.md](../workspace.md). The ACK
source tree will be referred to as `//common` package throughout this document.

For the most up-to-date information about each target, check out the in-line
comments before the definition of the target in `common/BUILD.bazel`.

Example list of targets on the `android14-5.15` branch:

[https://android.googlesource.com/kernel/common/+/refs/heads/android14-5.15/BUILD.bazel](https://android.googlesource.com/kernel/common/+/refs/heads/android14-5.15/BUILD.bazel)

## //common:all\_headers

This is an alias to `//common:all_headers_aarch64`.

## //common:all\_headers\_aarch64

This is a collection of all headers and include directories that a DDK module
for the arm64 architecture can safely use.

To use it, declare it in the `deps` attribute of a `ddk_module`. Example:

```python
ddk_module(
    name = "mymodule",
    deps = [
        "//common:all_headers",
    ],
)
```

At the time of writing, the `//common:all_headers_aarch64` consists of the
following include directories and header files under them:

- `arch/arm64/include`
- `arch/arm64/include/uapi`
- `include`
- `include/uapi`

**NOTE**: For up-to-date definition, check out `common/BUILD.bazel` in your
source tree directly.

## //common:all_headers\_x86\_64

Same as `//common:all_headers_aarch64` but for the x86 architecture. Include
directories and header files are searched from:

- `arch/x86/include`
- `arch/x86/include/uapi`
- `include`
- `include/uapi`

## --allow\_ddk\_unsafe\_headers: include unsafe list {#unsafe}

If `--allow_ddk_unsafe_headers` is specified in the command line, the
`//common:all_headers_aarch64` and `//common:all_headers_x86_64` targets
additionally includes a list of headers and include directories that are unsafe
to be used for DDK modules, but exported temporarily during the migration to
DDK.

The unsafe list is volatile:

- An item may be removed without notice when all devices using the item are
  cleaned up and the item no longer needs to be exported.
- An item may be moved into the stable allowlist so it may be used without
  the `--allow_ddk_unsafe_headers` flag.

The list of unsafe headers includes a selection of headers under
`common/drivers/` and possibly some others.

Eventually, the list of unsafe headers should either be removed or moved into
the safe allowlist, and `--allow_ddk_unsafe_headers` should have no effect. The
flag is in place for migration from the legacy `kernel_module`
to `ddk_module`.

**NOTE**: For up-to-date definition, check out `common/BUILD.bazel` in your
source tree directly.

## Generating the list of ddk\_headers targets

To generate an initial list of `ddk_headers` targets under `//common`, one may
analyze the inputs and build commands for the existing external modules, which
can either be `ddk_module` or legacy `kernel_module`.

Example CL for `android14-5.15`, using the external modules of Pixel 2021:
[CL:2237490](https://android-review.googlesource.com/c/kernel/common/+/2237490)

## Maintaining the list of ddk\_headers targets

Items may be added from these targets to suit the needs of other devices and
SoCs.

Example CL for `android14-5.15`, using the external modules of virtual device:
[CL:2257886](https://android-review.googlesource.com/c/kernel/common/+/2257886)

Items may be dropped from the unsafe list, or moved from the unsafe list to the
allowlist, under the conditions [above](#unsafe).
