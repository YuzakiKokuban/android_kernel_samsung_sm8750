"""Rules for defining a native cc_library based on a kernel's UAPI headers."""

load("//build/kernel/kleaf/impl:common_providers.bzl", "KernelBuildUapiInfo")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")

visibility("//build/kernel/kleaf/...")

def _kernel_unarchived_uapi_headers_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    uapi_headers = ctx.attr.kernel_build[KernelBuildUapiInfo].kernel_uapi_headers.to_list()

    if not uapi_headers:
        fail("ERROR: no UAPI headers found in kernel_build")

    # If using a mixed build, the main tree's UAPI header's will be last in the list. If
    # not a mixed build, there will be only one element. Take the last one either way.
    input_tar = uapi_headers[-1]
    out_dir = ctx.actions.declare_directory(ctx.label.name)

    command = ""
    command += hermetic_tools.setup
    command += """
      # Create output dir
      mkdir -p "{out_dir}"
      # Unpack headers (stripping /usr/include)
      tar --strip-components=2 -C "{out_dir}" -xzf "{tar_file}"
    """.format(
        tar_file = input_tar.path,
        out_dir = out_dir.path,
    )

    ctx.actions.run_shell(
        mnemonic = "KernelUnarchivedUapiHeaders",
        inputs = [input_tar],
        outputs = [out_dir],
        tools = hermetic_tools.deps,
        progress_message = "Unpacking UAPI headers {}".format(ctx.label),
        command = command,
    )

    return [
        DefaultInfo(files = depset([out_dir])),
    ]

_kernel_unarchived_uapi_headers = rule(
    implementation = _kernel_unarchived_uapi_headers_impl,
    doc = """Unpack `kernel_build`'s `kernel-uapi-headers.tar.gz` (stripping usr/include)""",
    attrs = {
        "kernel_build": attr.label(
            providers = [KernelBuildUapiInfo],
            mandatory = True,
            doc = "the `kernel_build` whose UAPI headers to unarchive",
        ),
    },
    toolchains = [hermetic_toolchain.type],
)

def kernel_uapi_headers_cc_library(name, kernel_build):
    """Defines a native cc_library based on a kernel's UAPI headers.

    Example:

    ```
    kernel_uapi_headers_cc_library(
        name = "uapi_header_lib",
        kernel_build = "//common:kernel_aarch64",
    )

    cc_binary(
        name = "foo",
        srcs = ["foo.c"],
        deps = [":uapi_header_lib"],
    )
    ```

    Internally, the `kernel_build`'s UAPI header output tarball is unpacked. Then, a header-only
    [`cc_library`](https://bazel.build/reference/be/c-cpp#cc_library) is generated. This allows
    other native Bazel C/C++ rules to add the kernel's UAPI headers as a dependency.

    The library will automatically include the header directory in the dependent build, so source
    files are free to simply include the UAPI headers they need.

    Note: the `kernel_build`'s output UAPI header tarball includes the `usr/include` prefix. The
    prefix is stripped while creating this library. To include the file `usr/include/linux/time.h`
    from the tarball, a source file would `#include <linux/time.h>`.

    Args:
        name: Name of target.
        kernel_build: [`kernel_build`](#kernel_build)
    """

    unarchived_headers_rule = name + "_unarchived_uapi_headers"
    _kernel_unarchived_uapi_headers(
        name = unarchived_headers_rule,
        kernel_build = kernel_build,
    )

    # Header-only library build will not invoke any toolchain
    native.cc_library(
        name = name,
        hdrs = [":" + unarchived_headers_rule],
        includes = [unarchived_headers_rule],
    )
