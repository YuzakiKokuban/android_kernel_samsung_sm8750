# Handling SCM version

## What is SCM version?

SCM version refers to the result of the `scripts/setlocalversion` script.

- For the kernel binary `vmlinux`, SCM version can be found in the kernel
  release string, after the kernel version.
- For kernel modules `*.ko`, SCM version can be found using `modinfo(8)`.

## The `stamp` config

Embedding the SCM version:
- Introduces extra overhead for every `bazel` command
- Unnecessarily triggers rebuilds when unrelated code changes

The SCM version is only embedded when `--config=stamp` is set.

On a developer's machine, the configuration is not set by default.

### Other flags

The flag `--config=stamp` is also implied by other flags, e.g.:

* `--config=release`. See [release.md](release.md).

## Handling SCM version and `SOURCE_DATE_EPOCH` in `kernel_build`

For `kernel_build()` that produces `vmlinux`, the following is required to embed
SCM version and `SOURCE_DATE_EPOCH` properly.

### `repo` requirements

The following is required for the build system to infer the list of git projects
in the repository.

Either:

- repo is installed on the host machine
- The git repository defining the `kernel_build()` is below a `repo` reository.

Or:

- A `manifest.xml` file is generated in advance with
  `repo manifest -r`, then provided to Bazel with
  `tools/bazel build --repo_manifest=$(realpath <repo root>):$(realpath <manifest.xml>)`.

The `<repo root>` should be the root of the `repo` repository where `.repo`
resides.

The `manifest.xml` file is needed by the build system when `repo` is not available
in the build environment. This is uncommon.

Extra git projects not listed in the repo manifest may be provided via
`--extra_git_project`. Multiple uses are accumulated. This may be useful if
you have symlinks to Git projects in your repository that does not show up
as a `<project>` element in the repo manifest.

### `setlocalversion` requirements

By default, `--kleaf_localversion` is set, so `scripts/setlocalversion` does
not need to exist.

If `--nokleaf_localversion`,
`scripts/setlocalversion` needs to exist in some git repository managed by the
repo manifest. It is not necessary that the git repository is the same as the
one containing the `kernel_build`. Usually, the file can be found in
`common/scripts/setlocalversion` in the workspace if you check out the core
kernel source tree under `common/`. (This requirement may not be necessary in
the future.)

### --kleaf_localversion flag

If `--kleaf_localversion` is set, Kleaf uses an embedded script to determine
localversion instead of calling `scripts/setlocalversion`. The script is
not branch-specific and does not include the 5-digit number of patches beyond
the tag.

### Testing

To ensure the artifact `vmlinux` contains SCM version properly, you may check
the following.

- Run `{name of kernel_build}_test`. For example, you may check the GKI vmlinux
  with `bazel test //common:kernel_aarch64_test`. For details, see
  [testing.md#gki](testing.md#gki).
- `strings vmlinux | grep "Linux version"`
- Boot the device kernel, and call `uname -r`.

To ensure the in-tree kernel modules contains SCM version properly, you may
check the following.

- Run `{name of kernel_build}_modules_test`. For example, for Pixel 2021
  mainline, you may check the in-tree kernel modules with
  `bazel test //gs/google-modules/soc-modules:slider_modules_test`. For details,
  see [testing.md#device-kernel](testing.md#device-kernel).
- `modinfo -F scmversion <modulename>.ko`
- Boot the device, and check `/sys/module/<MODULENAME>/scmversion`.

## Testing SCM version for external `kernel_module`s

To ensure the external kernel modules contains SCM version properly, you may
check the following.

- Run `{name of kernel_module}_test`. For example, for Pixel 2021 mainline, you
  may check the NFC kernel modules with
  `bazel test //gs/google-modules/nfc:nfc.slider_test`. For details,
  see [testing.md#external-kernel_module](testing.md#external-kernel_module).
- `modinfo -F scmversion <modulename>.ko`
- Boot the device, and check `/sys/module/<MODULENAME>/scmversion`.

## Examples

### Simple Example

Suppose you have the following directory structure, with no symlinks to Git
projects, and the repo root is the Bazel workspace root:

```
repo_root
  |- .repo
  |- tools/bazel
  |- MODULE.bazel
  |- common
  |  `- BUILD.bazel (defines :kernel_aarch64)
  `- mydevice
     `- BUILD.bazel (defines :mydevice_dist that depends on //common:kernel_aarch64)
```

If `repo` is installed in the build environment, run

```
cd repo_root && \
tools/bazel run --config=stamp //mydevice:mydevice_dist
```

If `repo` is not installed in the isolated build environment:
1.  Outside the isolated build environment, prepare repo manifest by running
    ```
    cd repo_root && repo manifest -r > out/manifest.xml
    ```
2.  Inside the isolated build environment, run
    ```
    cd repo_root && \
    tools/bazel run \
        --repo_manifest $(realpath .):$(realpath out/manifest.xml) \
        --config=stamp //mydevice:mydevice_dist
    ```

### Advanced Example

Suppose you have the following directory structure, with symlinks to Git
projects, and the repo root is not the same as the Bazel workspace root:

```
repo_root
  |- .repo
  `- kernel_srctree
      |- tools/bazel
      |- MODULE.bazel
      |- kernel-next
      |  `- BUILD.bazel (defines :kernel_aarch64)
      |- common (symlink to kernel-next)
      |- mydevice-next
      |  `- BUILD.bazel (defines :mydevice_dist that depends on //common:kernel_aarch64)
      `- mydevice (symlink to mydevice-next)
```

If `repo` is installed in the build environment, run

```
cd repo_root/kernel_srctree && \
tools/bazel run \
    --extra_git_project common \
    --extra_git_project mydevice \
    --config=stamp //mydevice:mydevice_dist
```

If `repo` is not installed in the isolated build environment:
1.  Outside the isolated build environment, prepare repo manifest by running
    ```
    cd repo_root && repo manifest -r > out/manifest.xml
    ```
2.  Inside the isolated build environment, run
    ```
    cd repo_root/kernel_srctree && \
    tools/bazel run \
        --extra_git_project common \
        --extra_git_project mydevice \
        --repo_manifest $(realpath .):$(realpath out/manifest.xml) \
        --config=stamp //mydevice:mydevice_dist
    ```

## Deprecated: `.source_date_epoch_dir` and `build.config`

It was required previously (specifically, in `master-kernel-build-2022` for
`android13-*` branches and early commits in `android14-*` branches) that
the top level symlinks `.source_date_epoch_dir` and `build.config` was required
to set `SOURCE_DATE_EPOCH` and scmversion correctly.

The two symlinks are not needed any more. In `android14-*` branches and later
(including `android-mainline`), you are advised to delete these symlinks.
