# Setting up a DDK Workspace

## Use the bootstrapping script

The bootstrapping script may be found in the
[kernel/build/bootstrap](https://android.googlesource.com/kernel/build/bootstrap)
project. The following one-line command downloads and executes it:

```shell
$ curl https://android.googlesource.com/kernel/build/bootstrap/+/refs/heads/main/init.py?format=TEXT | base64 --decode | python3 - [flags]
```

This command will be referred to as `python3 init.py` below.

### Typical usage

In the examples below, `/path/to/ddk/workspace` is the path to the
DDK workspace the script will populate. Usually, the script generates at
least the following in the directory:
* [`MODULE.bazel`](#root-modulebazel)
* [`device.bazelrc`](#devicebazelrc)
* `tools/bazel` symlink

#### Local sources

Populate DDK workspace against a local, full
checkout of ACK source tree at `/path/to/ddk/workspace/external/kleaf`:

```shell
$ python3 init.py --local \
    --ddk_workspace /path/to/ddk/workspace \
    --kleaf_repo /path/to/ddk/workspace/external/kleaf
```

#### Local sources + prebuilts

Populate DDK workspace against a local checkout of
Kleaf projects at `/path/to/ddk/workspace/external/kleaf` and prebuilts at
`/path/to/ddk/workspace/prebuilts/kernel`:

```shell
$ python3 init.py --local \
    --ddk_workspace /path/to/ddk/workspace \
    --kleaf_repo /path/to/ddk/workspace/external/kleaf \
    --prebuilts_dir /path/to/ddk/workspace/prebuilts/kernel
```

#### Build against prebuilts from a build

Populate DDK workspace `/path/to/ddk/workspace/`, with the following:

* Kleaf projects will be checked out at `/path/to/ddk/workspace/external/kleaf`
* Prebuilts will be checked out at `/path/to/ddk/workspace/prebuilts/kernel`

`/path/to/ddk/workspace` must be a valid repo; run `repo init` to initalize it.

To checkout projects and prebuilts associated with the build ID `123456`
from ci.android.com:

```shell
$ ( cd /path/to/ddk/workspace && repo init ... )
$ python3 init.py --build_id 123456 \
    --ddk_workspace /path/to/ddk/workspace \
    --kleaf_repo /path/to/ddk/workspace/external/kleaf \
    --prebuilts_dir /path/to/ddk/workspace/prebuilts/kernel
```

To checkout projects and prebuilts associated with the tip of tree of the
branch, `common-android15-6.6`:

```shell
$ ( cd /path/to/ddk/workspace && repo init ... )
$ python3 init.py --branch aosp_kernel-common-android15-6.6 \
    --ddk_workspace /path/to/ddk/workspace \
    --kleaf_repo /path/to/ddk/workspace/external/kleaf \
    --prebuilts_dir /path/to/ddk/workspace/prebuilts/kernel
```

`init.py` will run `repo sync` on relevant projects as part of the process. To
skip this so you can run `repo sync` manually, add the `--nosync` flag to
`init.py`.

See [workspace_example.sh](workspace_example.sh) for a complete example.

If you need to refresh the prebuilts and the source tree for Kleaf tooling
to point to the latest build on `common-android15-6.6`, simply re-run the
`init.py` script with the exact same arguments.

If you need to use a different branch or build ID, simply re-run the `init.py`
script with the exact same arguments and modified `--branch` or `--build_id`.

## Root MODULE.bazel

**Note**: The content below is automatically generated if you are using
the [bootstrapping script](#use-the-bootstrapping-script).

### @kleaf dependency

The `MODULE.bazel` file of the root module should declare a dependency
to `@kleaf`. In this example, the module is checked out at
`external/kleaf` relative to the workspace root:

```python
bazel_dep(name = "kleaf")
local_path_override(
    module_name = "kleaf",
    path = "external/kleaf", # or absolute path
)
```

You may now use rules in the `@kleaf` repository. For example, in `BUILD.bazel`:

```python
load("@kleaf//build/kernel/kleaf:kernel.bzl", "ddk_module")
```

If the full kernel source tree exists in `external/kleaf/common`, you may also
use the kernel built from source. For example:

```python
ddk_module(
    name = "mymodule",
    kernel_build = "@kleaf//common:kernel_aarch64",
    deps = [
        "@kleaf//common:all_headers_aarch64",
    ],
    # other attrs
)
```

### Declare prebuilts repository

The `MODULE.bazel` file of the root module may declare a repository containing
kernel prebuilts, if they exist. For example:

```python
kernel_prebuilt_ext = use_extension(
    "@kleaf//build/kernel/kleaf:kernel_prebuilt_ext.bzl",
    "kernel_prebuilt_ext",
)
kernel_prebuilt_ext.declare_kernel_prebuilts(
    name = "gki_prebuilts", # name of your choice
    local_artifact_path = "prebuilts/kernel", # or absolute path
    # other attrs
)
```

See
[kernel_prebuilt_ext.declare_kernel_prebuilts](../api_reference/kernel_prebuilt_ext.md) for its API.
**Note**: the API may undergo changes when this feature is in experiemental
stage.

You may now use `@gki_prebuilts//kernel_aarch64` in the `kernel_build`
attributes of various rules. For example:

```python
ddk_module(
    name = "mymodule",
    kernel_build = "@gki_prebuilts//kernel_aarch64",
    # other attrs
)
```

**Advanced usage**: If you have both `@kleaf//common:kernel_aarch64` and
`@gki_prebuilts//kernel_aarch64`, you may define flags to switch between
them. For example:

```python
# mypackage/BUILD.bazel
label_flag(
    name = "base_kernel",
    build_setting_default = "@gki_prebuilts//kernel_aarch64",
)

ddk_module(
    name = "mymodule",
    kernel_build = ":base_kernel",
    # other attrs
)
```

```shell
# Build against prebuilt kernel
$ tools/bazel build //mypackage:mymodule

# Build against kernel built from sources
$ tools/bazel build --//mypackage:base_kernel=@kleaf//common:kernel_aarch64 \
    //mypackage:mymodule
```

You may add flag aliases and configs to `device.bazelrc`. See
[.bazelrc files](../impl.md#bazelrc-files).

### Transitive dependencies

This refers to the list of dependencies of `@kleaf` that are not
directly needed by the root module, plus the Bazel Central Registry.

#### Download from the Internet

To rely on the Internet for transitive dependencies from the `@kleaf`,
add [`--config=internet` flag](../network.md). You may add the flag to
[`device.bazelrc`](#devicebazelrc).

#### True local builds

To build in an air-gapped environment without Internet access, all
dependencies must be vendored locally.

##### Registry

You may checkout a registry from one of the following:

* [Bazel Central Registry (BCR)](https://bcr.bazel.build/)
* [AOSP mirror of BCR](https://android.googlesource.com/platform/external/bazelbuild-bazel-central-registry).
  This may be slightly outdated.

Assuming that you have checked out the registry somewhere on your disk,
you may specify the `--registry` flag. For example, in
[`device.bazelrc`](#devicebazelrc):

```text
common --registry=file://%workspace%/external/bazelbuild-bazel-central-registry
```

##### Dependent modules

You may collect the list of all dependent modules with the following command:

```shell
$ tools/bazel mod graph --include_builtin
```

For details, see [bazel mod command](https://bazel.build/external/mod-command).

Among them, the list of modules that `@kleaf` module depends on may be
found by looking at `external/kleaf/MODULE.bazel`, assuming that the module is
located at `external/kleaf`. Its content may also be found in
[this file](../../bzlmod/bazel.MODULE.bazel).

Then, declare `local_path_override()` for each dependency and transitive
dependency in your **root `MODULE.bazel`**. You may also use other
[non-registry overrides](https://bazel.build/external/module#non-registry_overrides)
if applicable. For example:

```python
local_path_override(
    module_name = "rules_cc",
    path = "external/bazelbuild-rules_cc",
)
```

**Note**: `local_path_override()` in dependent modules (e.g. the ones
in `@kleaf`) has no effect. `local_path_override()` is only effective
when used at the root MODULE.bazel. See
[documentation for local_path_override()](https://bazel.build/rules/lib/globals/module#local_path_override).

**Note**: If `path` is relative, it is interpreted against the workspace root.
In the above example, `@rules_cc` is found at
`/path/to/ddk/workspace/external/bazelbuild-rules_cc`.

## device.bazelrc

The `device.bazelrc` file in your workspace root may contain the following
lines. See [.bazelrc files](../impl.md#bazelrc-files) for details.

Bazel may fetch dependencies from the Internet if there are no
`local_path_override`
declarations for the dependency. The following
allows Internet access. For details, see [Internet access](../network.md).

```text
common --config=internet
```

Kleaf sets `--registry` by default; see
[bzlmod.bazelrc](../../bazelrc/bzlmod.bazelrc). If the registry is not checked
out at `external/bazelbuild-bazel-central-registry` under the workspace root,
override its value. For example:

```text
common:bzlmod --registry=file:///path/to/bcr
```
