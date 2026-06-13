# Creating distributions

## Creating a distribution for a kernel build

To create a distribution with the kernel image, modules,
partition images, etc., see [impl.md#step-5](impl.md#step-5).

# Creating a distribution for a single module

To create a distribution for a single `kernel_module` or `ddk_module`,
define a separate `copy_to_dist_dir` rule that contains just the target:

```py
kernel_module(
    name = "foo",
    # ...
)

copy_to_dist_dir(
    name = "foo_dist",
    data = [":foo"],
    # ...
)
```

**NOTE**: It is not recommended to do the following because it contradicts
the ["DAMP" rule](kleaf_development.md#damp). All targets that are visible to
users should be defined in `BUILD.bazel` files, not wrapped in macros.

Yet sometimes, you may want to be less repetitive on `BUILD.bazel` files. For that
reason, you may define a small macro that glues the two targets together:

```py
# NOTE: This is discouraged.
# kernel_module_dist.bzl

def kernel_module_dist(
    name,
    **kwargs
):
    kernel_module(
        name = name,
        **kwargs
    )

    copy_to_dist_dir(
        name = name + "_dist",
        data = [name],
        flat = True,
    )
```

```py
# BUILD.bazel

load(":kernel_module_dist.bzl", "kernel_module_dist")

kernel_module_dist(
    name = foo,
    # ...
)
```
