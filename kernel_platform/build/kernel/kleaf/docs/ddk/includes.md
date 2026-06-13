# Handling headers and include directories

## Exported header files and include directories

A `ddk_module` target may export headers that its dependents may use. A
`ddk_headers` target declares a list of headers that can be reused by
other `ddk_headers` or `ddk_module` targets.

You may declare exported header files in the `hdrs` attribute, and exported
include directories in the `includes` attribute.

Example for a module to export headers:

```python
ddk_module(
    name = "graphics",
    out = "graphics.ko",
    srcs = ["graphics.c"],
    hdrs = ["include_graphics/graphics/graphics.h"],
    includes = ["include_graphics"],
)
```

Example of a `ddk_headers` target:

```python
ddk_headers(
    name = "display_common_headers",
    hdrs = ["include_disp/display/display_common.h"],
    includes = ["include_disp"],
)
```

**NOTE**: The `includes` attribute alone has no real effect. It merely adds
`-I` to `Kbuild` of dependents. Without declaring the header files in `hdrs`,
the header files are not visible to dependents.

**NOTE**: Order matters. See `ddk_module` in
[API Reference and Documentation for all rules](../api_reference.md) for
details about order of `includes`.

## Using headers from dependencies

With the above declaration, a target may use the exported headers from
its dependencies.

Depedencies are declared in one of the following attributes:

* `deps`: Exported headers and include directories from the dependency
  are for target use only and not re-exported to dependent targets.
* `hdrs`: Exported headers and include directories from the dependency
  are re-exported.

For example:

```python
ddk_module(
    name = "display_primary",
    out = "display.ko",
    srcs = ["display.c"],
    deps = [
        ":graphics",
        ":display_common_headers",
    ],
)
```

```c
// display.c
#include <display/display_common.h>
#include <graphics/graphics.h>
```

`display.c` is compiled with `-I include_graphics -I include_disp`.

Because `:graphics` and `:display_common_headers` are specified in `deps`,
dependents of `display_primary` do not automatically get the exported headers
and include directories from `:graphics` and `:display_common_headers`. If
you need to let `display_primary` to re-export headers, see
[Re-exporting](#reexport).

## Re-exporting header files and include directories {#reexport}

If a target specify dependencies in `hdrs` instead of `deps`, the exported
headers and include directories from dependencies are re-exported by this
target.

For example:

```python
ddk_module(
    name = "display_secondary_shim",
    out = "display_secondary_shim.ko",
    srcs = ["display_secondary_shim.c"],
    hdrs = [
        ":graphics",
        ":display_common_headers",
    ],
)

ddk_module(
    name = "display_secondary",
    out = "display_secondary.ko",
    srcs = ["display_secondary.c"],
    deps = [
        ":display_secondary_shim",
    ],
)
```

```c
// display_secondary.c
#include <display/display_common.h>
#include <graphics/graphics.h>
```

Because `display_secondary_shim` re-exports headers and include directories
from `:graphics` and `:display_common_headers`, `display_secondary.c` is also
compiled with `-I include_graphics -I include_disp`.

Similarly, you may chain `ddk_headers` targets. For example:

```python
ddk_headers(
    name = "display_external_headers",
    hdrs = [":display_common_headers"],
    # additional hdrs and includes
)
```

Because `display_external_headers` re-exports `:display_common_headers`,
dependents of `display_external_headers` is compiled with `-I include_disp`.

## linux\_includes

The `linux_includes` attribute is like `includes`, but specified in
`LINUXINCLUDE` in the generated `Kbuild` file. Hence, items in
`linux_includes` takes higher precendence.

## Private header files and include directories

A `ddk_module` may use some private headers and include directories during its compilation, without exporting them to dependents.

To do so, declare a separate `ddk_headers` target, and declare the
`ddk_headers` target in `deps`. You may explicitly specify
`//visibility:private` to prevent the `ddk_headers` to be used elsewhere.

Example:

```python
ddk_headers(
    name = "nfc_private_headers",
    hdrs = ["include_nfc/nfc.h"],
    includes = ["include_nfc"],

    # This is the default, but you may explicitly express the intention here
    # to avoid it to be overridden by package(default_visibility=...)
    visibility = ["//visibility:private"],
)

ddk_module(
    name = "nfc",
    out = "nfc.ko",
    srcs = ["nfc.c"],
    deps = [":nfc_private_headers"],
)
```

`include_nfc` are private include directories not visible by other targets.

## Implicitly including a single header file

A `ddk_module` may implicitly include a header file with `copts`. The `copts`
do not affect dependents.

For example:

```python
ddk_module(
    name = "camera",
    out = "camera.ko",
    srcs = ["camera.c", "camera_constants.h"],
    copts = ["-include", "$(location camera_constants.h)"],
)
```

`camera.c` is compiled with `-include <path/to/camera_constants.h>`.

`$(location)` is necessary for Bazel to insert the correct path to the file,
as the compiler is executed in the output directory, not the source directory.
