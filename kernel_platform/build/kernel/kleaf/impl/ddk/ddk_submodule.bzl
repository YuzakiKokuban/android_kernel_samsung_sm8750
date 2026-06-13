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

"""Rules for defining a DDK (Driver Development Kit) submodule."""

load(":ddk/ddk_conditional_filegroup.bzl", "flatten_conditional_srcs")
load(":ddk/makefiles.bzl", "makefiles")

visibility("//build/kernel/kleaf/...")

def ddk_submodule(
        name,
        out,
        srcs = None,
        deps = None,
        hdrs = None,
        includes = None,
        local_defines = None,
        copts = None,
        conditional_srcs = None,
        **kwargs):
    """Declares a DDK (Driver Development Kit) submodule.

    Symbol dependencies between submodules in the same [`ddk_module`](#ddk_module)
    are not specified explicitly. This is convenient when you have multiple module
    files for a subsystem.

    See [Building External Modules](https://www.kernel.org/doc/Documentation/kbuild/modules.rst)
    or `Documentation/kbuild/modules.rst`, section "6.3 Symbols From Another External Module",
    "Use a top-level kbuild file".

    Example:

    ```
    ddk_submodule(
        name = "a",
        out = "a.ko",
        srcs = ["a.c"],
    )

    ddk_submodule(
        name = "b",
        out = "b.ko",
        srcs = ["b_1.c", "b_2.c"],
    )

    ddk_module(
        name = "mymodule",
        kernel_build = ":tuna",
        deps = [":a", ":b"],
    )
    ```

    `linux_includes` must be specified in the top-level `ddk_module`; see
    [`ddk_module.linux_includes`](#ddk_module-linux_includes).

    **Ordering of `includes`**

    See [`ddk_module`](#ddk_module).

    **Caveats**

    As an implementation detail, `ddk_submodule` alone does not build any modules. The
    `ddk_module` target is the one responsible for building all `.ko` files.

    A side effect is that for incremental builds, modules may be rebuilt unexpectedly.
    In the above example,
    if `a.c` is modified, the whole `mymodule` is rebuilt, causing both `a.ko` and `b.ko` to
    be rebuilt. Because `ddk_module` is always built in a sandbox, the object files (`*.o`) for
    `b.ko` is not cached.

    Hence, it is always recommended to use one `ddk_module` per module (`.ko` file). You may
    use `build/kernel/kleaf/build_cleaner.py` to resolve dependencies; see
    `build/kernel/kleaf/docs/build_cleaner.md`.

    The `ddk_submodule` rule should only be used when the dependencies among modules are too
    complicated to be presented in `BUILD.bazel`, and are frequently updated. When the
    dependencies are stable, it is recommended to:

    1. Replace `ddk_submodule` with `ddk_module`;
    2. Specify dependencies in the `deps` attribute explicitly.

    Args:
        name: See [`ddk_module.name`](#ddk_module-name).
        srcs: See [`ddk_module.srcs`](#ddk_module-srcs).
        conditional_srcs: See [`ddk_module.conditional_srcs`](#ddk_module-conditional_srcs).
        out: See [`ddk_module.out`](#ddk_module-out).
        hdrs: See [`ddk_module.hdrs`](#ddk_module-hdrs).

            These are only effective in the current submodule, not other submodules declared in the
            same [`ddk_module.deps`](#ddk_module-deps).

            These are exported to downstream targets that depends on the
            `ddk_module` that includes the current target. Example:

            ```
            ddk_submodule(name = "module_parent_a", hdrs = [...])
            ddk_module(name = "module_parent", deps = [":module_parent_a"])
            ddk_module(name = "module_child", deps = [":module_parent"])
            ```

            `module_child` automatically gets `hdrs` of `module_parent_a`.

        deps: See [`ddk_module.deps`](#ddk_module-deps).

            These are only effective in the current submodule, not other submodules declared in the
            same [`ddk_module.deps`](#ddk_module-deps).

            These are not exported to downstream targets that depends on the
            `ddk_module` that includes the current target.

        includes: See [`ddk_module.includes`](#ddk_module-includes).

            These are only effective in the current submodule, not other submodules declared in the
            same [`ddk_module.deps`](#ddk_module-deps).

            These are exported to downstream targets that depends on the
            `ddk_module` that includes the current target. Example:

            ```
            ddk_submodule(name = "module_parent_a", includes = [...])
            ddk_module(name = "module_parent", deps = [":module_parent_a"])
            ddk_module(name = "module_child", deps = [":module_parent"])
            ```

            `module_child` automatically gets `includes` of `module_parent_a`.

        local_defines: See [`ddk_module.local_defines`](#ddk_module-local_defines).

            These are only effective in the current submodule, not other submodules declared in the
            same [`ddk_module.deps`](#ddk_module-deps).

            These are not exported to downstream targets that depends on the
            `ddk_module` that includes the current target.

        copts: See [`ddk_module.copts`](#ddk_module-copts).

            These are only effective in the current submodule, not other submodules declared in the
            same [`ddk_module.deps`](#ddk_module-deps).

            These are not exported to downstream targets that depends on the
            `ddk_module` that includes the current target.
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """

    private_kwargs = dict(kwargs)
    private_kwargs["visibility"] = ["//visibility:private"]

    flattened_conditional_srcs = flatten_conditional_srcs(
        module_name = name,
        conditional_srcs = conditional_srcs,
        **private_kwargs
    )

    makefiles(
        name = name,
        module_srcs = (srcs or []) + flattened_conditional_srcs,
        module_hdrs = hdrs,
        module_includes = includes,
        module_out = out,
        module_deps = deps,
        module_local_defines = local_defines,
        module_copts = copts,
        top_level_makefile = False,
        **kwargs
    )
