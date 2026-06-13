# Rules and macros

DDK provides the following rules and macros to define DDK modules.

* [`ddk_headers`](#ddk_headers) exposes headers from the kernel or from a module
  to other modules.
* [`ddk_module`](#ddk_module) defines the kernel module build.
* [`ddk_submodule`](#ddk_submodule) may be used along with `ddk_module` if there
  are multiple module output files (`*.ko`) within the same `ddk_module`.

## ddk\_headers

A `ddk_headers` target consists of the following:

- A list of header `.h` files, that `ddk_module`s depending on it can use.
- A list of textual header files, that `ddk_module`s depending on it can use.
- A list of include directories that `ddk_module`s depending on it
  will add to their include lookup path (corresponding to the `-I` option).

`ddk_headers` can be chained. That is, a `ddk_headers` target may re-export the
header files and include directories of another `ddk_headers` target.

You may define a `ddk_headers` target to include a collection of header files
and include directories to search from. You may want to do this because:

- You have a separate kernel source tree to build the kernel modules that does
  not track
  the [Android Common Kernel (ACK)](https://android.googlesource.com/kernel/common/)
  source tree.
- You want to define one or more sets of exported headers for a DDK module to
  suit the needs of the dependent modules.
- Or any reason unlisted here.

For up-to-date information about `ddk_headers`, its API, and examples, see
[documentation for all rules](../api_reference.md) and click on
the `ddk_headers` rule.

For `ddk_headers` target in the Android Common Kernel source tree, see
[using headers from the common kernel](common_headers.md).

## ddk\_module

A `ddk_module` target is a rule that defines a kernel module build using the
Kbuild mechanism. Necessary `Makefile`s etc. are generated automatically.

A `ddk_module` target may depend on a set of `ddk_headers` targets to use the
header files and include directories that the `ddk_headers` targets export.

A `ddk_module` target may re-export header files, include directories, and
`ddk_headers` targets.

A `ddk_module` target may depend on other `ddk_module` targets to use the header
files and include directories that the dependent `ddk_headers` target exports.

Example of a module with a single source file, no dependant modules, and
some private and exported headers:

```python
ddk_module(
    name = "my_module",
    srcs = ["my_module.c", "private_header.h"],
    out = "my_module.ko",
    # Exported headers
    hdrs = ["include/my_module_exported.h"],
    # Exported include directory
    includes = ["include"],
)
```

For up-to-date information about `ddk_module`, its API, and more examples, see
[documentation for all rules](../api_reference.md) and click on the `ddk_module`
rule.

### Kconfig / defconfig

A `ddk_module` target may optionally provide a `Kconfig` and/or a `defconfig`
file, via the `kconfig` or `defconfig` attribute, respectively.

See [configuring DDK module](config.md) for details.

## ddk\_submodule

**NOTE**: Using `ddk_submodule` is discouraged because of the unclear module
dependency. Check the [caveats](#caveats-for-ddk_submodule) section.

The `ddk_submodule` rule provides a way to specify multiple module definitions
(`*.ko`) within the same `ddk_module`. A `ddk_submoule` describes the inputs and
outputs to build a kernel module without specifying complete kernel submodule
dependencies among the submodules defined within a `ddk_module`. Symbol
dependencies are looked up from other `ddk_submodule` within the same
`ddk_module`.

### Caveats for ddk\_submodule

One must understand the following caveats before using `ddk_submodule`:

- Defining `ddk_submodule` alone has virtually no effect. A separate
  `ddk_module` must be defined to include the `ddk_submodule`.
- Building `ddk_submodule` alone does not build any modules. Build the
  `ddk_module` instead.
- Incremental builds may be slower than using one `ddk_module` per module
  (`.ko` output file). If the inputs of a `ddk_submodule` has changed, the
  entire build rule is invalidated and all modules are built unconditionally.

For up-to-date information about `ddk_module`, its API, examples, and caveats,
see [documentation for all rules](../api_reference.md) and click on the
`ddk_submodule` rule.
