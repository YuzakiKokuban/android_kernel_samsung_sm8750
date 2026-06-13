# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Rules for defining a DDK (Driver Development Kit) module."""

load(":ddk/ddk_conditional_filegroup.bzl", "flatten_conditional_srcs")
load(":ddk/ddk_config.bzl", "ddk_config")
load(":ddk/makefiles.bzl", "makefiles")
load(":kernel_module.bzl", "kernel_module")

visibility("//build/kernel/kleaf/...")

def ddk_module(
        name,
        kernel_build,
        srcs = None,
        deps = None,
        hdrs = None,
        textual_hdrs = None,
        includes = None,
        conditional_srcs = None,
        linux_includes = None,
        out = None,
        local_defines = None,
        copts = None,
        kconfig = None,
        defconfig = None,
        generate_btf = None,
        **kwargs):
    """
    Defines a DDK (Driver Development Kit) module.

    Example:

    ```
    ddk_module(
        name = "my_module",
        srcs = ["my_module.c", "private_header.h"],
        out = "my_module.ko",
        # Exported headers
        hdrs = ["include/my_module_exported.h"],
        textual_hdrs = ["my_template.c"],
        includes = ["include"],
    )
    ```

    Note: Local headers should be specified in one of the following ways:

    - In a `ddk_headers` target in the same package, if you need to auto-generate `-I` ccflags.
      In that case, specify the `ddk_headers` target in `deps`.
    - Otherwise, in `srcs` if you don't need the `-I` ccflags.

    Exported headers should be specified in one of the following ways:

    - In a separate `ddk_headers` target in the same package. Then specify the
      target in `hdrs`. This is recommended if there
      are multiple `ddk_module`s depending on a
      [`glob`](https://bazel.build/reference/be/functions#glob) of headers or a large list
      of headers.
    - Using `hdrs`, `textual_hdrs` and `includes` of this target.

    `hdrs`, `textual_hdrs` and `includes` have the same semantics as [`ddk_headers`](#ddk_headers).
    That is, this target effectively acts as a `ddk_headers` target when specified in the `deps`
    attribute of another `ddk_module`. In other words, the following code snippet:

    ```
    ddk_module(name = "module_A", hdrs = [...], includes = [...], ...)
    ddk_module(name = "module_B", deps = ["module_A"], ...)
    ```

    ... is effectively equivalent to the following:

    ```
    ddk_headers(name = "module_A_hdrs, hdrs = [...], includes = [...], ...)
    ddk_module(name = "module_A", ...)
    ddk_module(name = "module_B", deps = ["module_A", "module_A_hdrs"], ...)
    ```

    **Submodules**

    See [ddk_submodule](#ddk_submodule).

    If `deps` contains a `ddk_submodule` target, the `ddk_module` target must not specify
    anything except:

    - `kernel_build`
    - `linux_includes`

    **Ordering of `includes`**

    **The best practice is to not have conflicting header names and search paths.**
    But if you do, see below for ordering of include directories to be
    searched for header files.

    A [`ddk_module`](#ddk_module) is compiled with the following order of include directories
    (`-I` options):

    1. Traverse depedencies for `linux_includes`:
        1. All `linux_includes` of this target, in the specified order
        2. All `linux_includes` of `deps`, in the specified order (recursively apply #1.3 on each target)
        3. All `linux_includes` of `hdrs`, in the specified order (recursively apply #1.3 on each target)
    2. `LINUXINCLUDE` (See `${KERNEL_DIR}/Makefile`)
    3. Traverse depedencies for `includes`:
        1. All `includes` of this target, in the specified order
        2. All `includes` of `deps`, in the specified order (recursively apply #3.1 and #3.3 on each target)
        3. All `includes` of `hdrs`, in the specified order (recursively apply #3.1 and #3.3 on each target)

    In other words, #1 and #3 uses the `preorder` of
    [depset](https://bazel.build/rules/lib/depset).

    "In the specified order" means that order matters within these lists.
    To prevent buildifier from sorting these lists, use the `# do not sort` magic line.

    To export a target `:x` in `hdrs` before other targets in `deps`
    (that is, if you need #3.3 before #3.2, or #1.2 before #1.1),
    specify `:x` in the `deps` list in the position you want. See example below.

    To export an include directory in `includes` that needs to be included
    after other targets in `hdrs` or `deps` (that is, if you need #3.1 after #3.2
    or #3.3), specify the include directory in a separate `ddk_headers` target,
    then specify this `ddk_headers` target in `hdrs` and/or `deps` based on
    your needs.

    For example:

    ```
    ddk_headers(name = "dep_a", includes = ["dep_a"], linux_includes = ["uapi/dep_a"])
    ddk_headers(name = "dep_b", includes = ["dep_b"])
    ddk_headers(name = "dep_c", includes = ["dep_c"], hdrs = ["dep_a"])
    ddk_headers(name = "hdrs_a", includes = ["hdrs_a"], linux_includes = ["uapi/hdrs_a"])
    ddk_headers(name = "hdrs_b", includes = ["hdrs_b"])
    ddk_headers(name = "x", includes = ["x"])

    ddk_module(
        name = "module",
        deps = [":dep_b", ":x", ":dep_c"],
        hdrs = [":hdrs_a", ":x", ":hdrs_b"],
        linux_includes = ["uapi/module"],
        includes = ["self_1", "self_2"],
    )
    ```

    Then `":module"` is compiled with these flags, in this order:

    ```
    # 1.1 linux_includes
    -Iuapi/module

    # 1.2 deps, linux_includes, recursively
    -Iuapi/dep_a

    # 1.3 hdrs, linux_includes, recursively
    -Iuapi/hdrs_a

    # 2.
    $(LINUXINCLUDE)

    # 3.1 includes
    -Iself_1
    -Iself_2

    # 3.2. deps, recursively
    -Idep_b
    -Ix
    -Idep_a   # :dep_c depends on :dep_a, so include dep_a/ first
    -Idep_c

    # 3.3. hdrs, recursively
    -Ihdrs_a
    # x is already included, skip
    -Ihdrs_b
    ```

    A dependent module automatically gets #1.1, #1.3, #3.1, #3.3, in this order. For example:

    ```
    ddk_module(
        name = "child",
        deps = [":module"],
        # ...
    )
    ```

    Then `":child"` is compiled with these flags, in this order:

    ```
    # 1.2. linux_includes of deps, recursively
    -Iuapi/module
    -Iuapi/hdrs_a

    # 2.
    $(LINUXINCLUDE)

    # 3.2. includes of deps, recursively
    -Iself_1
    -Iself_2
    -Ihdrs_a
    -Ix
    -Ihdrs_b
    ```

    Args:
        name: Name of target. This should usually be name of the output `.ko` file without the
          suffix.
        srcs: sources and local headers.
        deps: A list of dependent targets. Each of them must be one of the following:

            - [`kernel_module`](#kernel_module)
            - [`ddk_module`](#ddk_module)
            - [`ddk_headers`](#ddk_headers).
        hdrs: See [`ddk_headers.hdrs`](#ddk_headers-hdrs)
        textual_hdrs: See [`ddk_headers.textual_hdrs`](#ddk_headers-textual_hdrs)
        includes: See [`ddk_headers.includes`](#ddk_headers-includes)
        linux_includes: See [`ddk_headers.linux_includes`](#ddk_headers-linux_includes)

          Unlike `ddk_headers.linux_includes`, `ddk_module.linux_includes` is **NOT**
          applied to dependent `ddk_module`s.
        kernel_build: [`kernel_build`](#kernel_build)
        conditional_srcs: A dictionary that specifies sources conditionally compiled based on configs.

          Example:

          ```
          conditional_srcs = {
              "CONFIG_FOO": {
                  True: ["foo.c"],
                  False: ["notfoo.c"]
              }
          }
          ```

          In the above example, if `CONFIG_FOO` is `y` or `m`, `foo.c` is compiled.
          Otherwise, `notfoo.c` is compiled instead.

        out: The output module file. This should usually be `"{name}.ko"`.

          This is required if this target does not contain submodules.
        local_defines: List of defines to add to the compile line.

          **Order matters**. To prevent buildifier from sorting the list, use the
          `# do not sort` magic line.

          Each string is prepended with `-D` and added to the compile command
          line for this target, but not to its dependents.

          Unlike
          [`cc_library.local_defines`](https://bazel.build/reference/be/c-cpp#cc_library.local_defines),
          this is not subject to
          ["Make" variable substitution](https://bazel.build/reference/be/make-variables) or
          [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables).

          Each string is treated as a single Bourne shell token. Unlike
          [`cc_library.local_defines`](https://bazel.build/reference/be/c-cpp#cc_library.local_defines),
          this is not subject to
          [Bourne shell tokenization](https://bazel.build/reference/be/common-definitions#sh-tokenization).
          The behavior is similar to `cc_library` with the `no_copts_tokenization`
          [feature](https://bazel.build/reference/be/functions#package.features).
          For details about `no_copts_tokenization`, see
          [`cc_library.copts`](https://bazel.build/reference/be/c-cpp#cc_library.copts).

        copts: Add these options to the compilation command.

          **Order matters**. To prevent buildifier from sorting the list, use the
          `# do not sort` magic line.

          Subject to
          [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables).

          The flags take effect only for compiling this target, not its
          dependencies, so be careful about header files included elsewhere.

          All host paths should be provided via
          [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables).
          See "Implementation detail" section below.

          Each `$(location)` expression should occupy its own token. For example:

          ```
          # Good
          copts = ["-include", "$(location //other:header.h)"]

          # BAD -- DON'T DO THIS!
          copts = ["-include $(location //other:header.h)"]

          # BAD -- DON'T DO THIS!
          copts = ["-include=$(location //other:header.h)"]
          ```

          Unlike
          [`cc_library.local_defines`](https://bazel.build/reference/be/c-cpp#cc_library.local_defines),
          this is not subject to
          ["Make" variable substitution](https://bazel.build/reference/be/make-variables).

          Each string is treated as a single Bourne shell token. Unlike
          [`cc_library.copts`](https://bazel.build/reference/be/c-cpp#cc_library.copts)
          this is not subject to
          [Bourne shell tokenization](https://bazel.build/reference/be/common-definitions#sh-tokenization).
          The behavior is similar to `cc_library` with the `no_copts_tokenization`
          [feature](https://bazel.build/reference/be/functions#package.features).
          For details about `no_copts_tokenization`, see
          [`cc_library.copts`](https://bazel.build/reference/be/c-cpp#cc_library.copts).

          Because each string is treated as a single Bourne shell token, if
          a plural `$(locations)` expression expands to multiple paths, they
          are treated as a single Bourne shell token, which is likely an
          undesirable behavior. To avoid surprising behaviors, use singular
          `$(location)` expressions to ensure that the label only expands to one
          path. For differences between the `$(locations)` and `$(location)`, see
          [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables).

          **Implementation detail**: Unlike usual `$(location)` expansion,
          `$(location)` in `copts` is expanded to a path relative to the current
          package before sending to the compiler.

          For example:

          ```
          # package: //package
          ddk_module(
            name = "my_module",
            copts = ["-include", "$(location //other:header.h)"],
            srcs = ["//other:header.h", "my_module.c"],
          )
          ```
          Then the generated Makefile contains:

          ```
          ccflags-y += -include ../other/header.h
          ```

          The behavior is such because the generated `Makefile` is located in
          `package/Makefile`, and `make` is executed under `package/`. In order
          to find `other/header.h`, its path relative to `package/` is given.

        kconfig: The Kconfig file for this external module.

          See
          [`Documentation/kbuild/kconfig-language.rst`](https://www.kernel.org/doc/html/latest/kbuild/kconfig.html)
          for its format.

          Kconfig is optional for a `ddk_module`. The final Kconfig known by
          this module consists of the following:

          - Kconfig from `kernel_build`
          - Kconfig from dependent modules, if any
          - Kconfig of this module, if any
        defconfig: The `defconfig` file.

          Items must already be declared in `kconfig`. An item not declared
          in Kconfig and inherited Kconfig files is silently dropped.

          An item declared in `kconfig` without a specific value in `defconfig`
          uses default value specified in `kconfig`.
        generate_btf: Allows generation of BTF type information for the module.
          See [kernel_module.generate_btf](#kernel_module-generate_btf)
        **kwargs: Additional attributes to the internal rule.
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """

    ddk_config(
        name = name + "_config",
        defconfig = defconfig,
        kconfig = kconfig,
        kernel_build = kernel_build,
        module_deps = deps,
        module_hdrs = hdrs,
        module_textual_hdrs = textual_hdrs,
        generate_btf = generate_btf,
    )

    kernel_module(
        name = name,
        kernel_build = kernel_build,
        srcs = [],
        # Set it to empty list, not None, so kernel_module() doesn't fallback to {name}.ko.
        # _kernel_module_impl infers the list of outs from internal_ddk_makefiles_dir.
        outs = [],
        generate_btf = generate_btf,
        internal_ddk_makefiles_dir = ":{name}_makefiles".format(name = name),
        # This is used in build_cleaner.
        internal_module_symvers_name = "{name}_Module.symvers".format(name = name),
        internal_drop_modules_order = True,
        internal_exclude_kernel_build_module_srcs = True,
        internal_ddk_config = name + "_config",
        **kwargs
    )

    private_kwargs = dict(kwargs)
    private_kwargs["visibility"] = ["//visibility:private"]

    flattened_conditional_srcs = flatten_conditional_srcs(
        module_name = name,
        conditional_srcs = conditional_srcs,
        **private_kwargs
    )

    makefiles(
        name = name + "_makefiles",
        module_srcs = (srcs or []) + flattened_conditional_srcs,
        module_hdrs = hdrs,
        module_textual_hdrs = textual_hdrs,
        module_includes = includes,
        module_linux_includes = linux_includes,
        module_out = out,
        module_deps = deps,
        module_local_defines = local_defines,
        module_copts = copts,
        top_level_makefile = True,
        **private_kwargs
    )
