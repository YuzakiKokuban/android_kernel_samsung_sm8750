# Kleaf Development

This documentation summarizes principles used in Kleaf development.

## Style Guides

* Follow [.bzl style guide](https://bazel.build/rules/bzl-style) for
  [Starlark](https://bazel.build/rules/language) files.
* Follow [BUILD Style Guide](https://bazel.build/build/style-guide) for BUILD
  files.

For Python:
* Use PEP 8 for formatting. Our team uses autopep8.
* Use [.pylintrc](../../.pylintrc) for linting. The file is copied from
  [https://google.github.io/styleguide/pylintrc](https://google.github.io/styleguide/pylintrc)
  with indent size changed to 4 to follow PEP 8.

## Conventions

* Follow these [conventions](https://bazel.build/extending/macros#conventions)
  for Macros, in particular:
  * In most cases, optional parameters should have a default value of `None`.
* Avoid using
  [`unittest.suite`](https://github.com/bazelbuild/bazel-skylib/blob/main/docs/unittest_doc.md#unittestsuite)
  . Reasons:
  * Individual test names are tagged with indices, not descriptive names. This
    makes failures hard to triage.
  * When items are reordered or inserted into the list, the indicies change,
    and thus individual test names change as well. Continuous testing processes
    may be confused and show incomplete test histories because of name changes.

### "DAMP" BUILD.bazel files {#damp}

`BUILD.bazel` files should apply the "DAMP" (descriptive and meaningful
phrases) rule, as opposed to the "DRY" (do not repeat yourself) rule that is
otherwise often applied to production code.

In practice, this rule encourages all targets visible to the user to be
declared in `BUILD.bazel` files instead of being wrapped in macros.

Usually, this means the implementation of a macro should only declare one
target public to the user, named with the same name of the macro
invocation. Private targets should usually be named `name + "_<suffix>"`,
unknown to the user and not guaranteed through the contract of the interface.

Example:

```py
# GOOD

# BUILD.bazel
foo_library(name = "my_target_lib", ...)
foo_binary(name = "my_target", ...)
```

```py
# BAD

# my_target.bzl
def my_rule(name, ...)
    foo_library(name = name + "_lib", ...)
    foo_binary(name = name, ...)

# BUILD.bazel
my_rule(name = "my_target", ...)
```

If you expect users to know about both `my_target` and `my_target_lib`; that
is, a user can do `bazel build :my_target :my_target_lib`, it is
encouraged that both are declared explicitly in the `BUILD.bazel` file. There
might be some repitition in the list of attributes shared between `my_target`
and `my_target_lib`, but the overall maintenance cost is lower in the long run.

A notable exception for macros is when the additional targets are implementation
details and not visible to users. For example:

```py
# GOOD

# my_target.bzl
def my_rule(name, ...)
    foo_library(
        name = name + "_internal_lib",
        visibility = ["//visibility:private"],
        # ...
    )
    foo_binary(
        name = name,
        deps = [name + "_internal_lib"]
        # ...
    )

# BUILD.bazel
my_rule(name = "my_target", ...)
```

In this case, the user is expected to only do `bazel build :my_target`,
not `bazel build :my_target_internal_lib`, because the library is an
implementation detail of `my_rule`.

**NOTE**: There are multiple violations of the "DAMP" rule even within
Kleaf's code base, notably the following. They require some cleanup, but
due to backwards compatibility of the API, they might not be completely
"DAMP" even in the future.

- `kernel_build`
- `kernel_module` (it calls `kernel_module_test`)
- `define_common_kernels`
- etc.

## Performance

* For performance optimizations follow the tips in
  [Optimizing Performance](https://bazel.build/rules/performance).
  * E.g. Use `depsets` instead of `lists`; `depsets` are a tree of objects that
  can be concatenated effortlessly.`lists` are concatenated by copying contents.

### Prefer depset over `ctx.files.X` in rules

In general, in rule implementations, prefer depsets and avoid using `ctx.files.X`.

- If you need a `depset`, prefer
  `depset(transitive = [target.files for target in ctx.attr.X])`. This is more
  memory efficient because it does not retain the `ctx.files.X` list.
  Instead, it only creates the depset tree using existing depsets in memory.
  - If there are too many labels in attribute `X`, you may micro-optimize by
    assingning the depset to a variable instead of computing it every time,
    or using an intermediate `filegroup`. Use Bazel profiles to confirm time
    reduction before doing micro-optimization.
- If you need a `list[File]`, and the list is generally small, `ctx.files.X`
  is okay to use. Note that `ctx.files.X` is lazily computed when you request
  the list. If you keep a large list around, there may be a memory impact.
- If you need a `list[File]` but it is large, consider optimizing your
  implementation so it works with `depset`s.
- `ctx.file.X` is okay to use because there's only a single file.

## Updating external repositories

Inside the kernel tree, run:

```sh
prebuilts/kernel-build-tools/linux-x86/bin/external_updater update <project_path> --no-build
```

Example:

```sh
prebuilts/kernel-build-tools/linux-x86/bin/external_updater update external/python/absl-py --no-build
```

Always prefer tags. For example, if you are prompted with:

```
Current version: v1.4.0
Latest version: v2.1.0
Alternative latest version: fae7e951d46011fdaf62685893ef4efd48544c0a
Out of date!
Would you like to upgrade to sha fae7e951d46011fdaf62685893ef4efd48544c0a instead of tag v2.1.0? (yes/no)
We DO NOT recommend upgrading to sha fae7e951d46011fdaf62685893ef4efd48544c0a.
```

Enter "no" and proceed.
