# Build against downloaded prebuilt GKI (deprecated)

**WARNING**: Contents in this page are deprecated, including
the `--use_prebuilt_gki` flag and `*_download_or_build` targets.
To build against downloaded prebuilt GKI with bzlmod, see
[Setting up DDK workspace](ddk/workspace.md#declare-prebuilts-repository).

## Step 1: Replace reference to GKI targets with downloaded targets

Replace all references to `//common:kernel_aarch64` with
`//common:kernel_aarch64_download_or_build`.

Replace all references to `//common:kernel_aarch64_additional_artifacts` with
`//common:kernel_aarch64_additional_artifacts_download_or_build`.

In particular, look out for these places:

* `kernel_build()`: `base_kernel`
* Any target: `kernel_build`
* `copy_to_dist_dir`: `data`
* `kernel_abi_dist`: `data`

See Pixel 2021 mainline for an example (search for `//common:kernel_aarch64`):

[https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel](https://android.googlesource.com/kernel/google-modules/raviole-device/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel)

## Step 2: Build with `--use_prebuilt_gki=<BUILD_ID>`

In the build command, specify `--use_prebuilt_gki=<BUILD_ID>` to build against
downloaded prebuilts.

For android13, the build ID must have a build target named `kernel_kleaf`.

Starting from android14 (including android-mainline), the build ID must have a build target named
`kernel_aarch64` with artifacts built with Kleaf. A build number >= 9359436 will
work.

It is recommended to select the latest build ID from the branch.

Sample branches:

- [aosp_kernel-common-android-mainline](https://ci.android.com/builds/branches/aosp_kernel-common-android-mainline/grid)
- [aosp_kernel-common-android14-6.1](https://ci.android.com/builds/branches/aosp_kernel-common-android14-6.1/grid)
- [aosp_kernel-common-android14-5.15](https://ci.android.com/builds/branches/aosp_kernel-common-android14-5.15/grid)
- [aosp_kernel-common-android13-5.15](https://ci.android.com/builds/branches/aosp_kernel-common-android13-5.15/grid)
- [aosp_kernel-common-android13-5.10](https://ci.android.com/builds/branches/aosp_kernel-common-android13-5.10/grid)

Other unspecified branches with a build target named `kernel_aarch64` may also
work if it is built with Kleaf. You may check whether a build is built with
`Kleaf` by checking the build command in `logs/build.log` or the existence of a
file with suffix `_modules`, e.g. `kernel_aarch64_modules`.

Sample command to build `raviole-android13-5.15` against prebuilts from
`android13-5.15`:

```shell
# On raviole-5.15 branch, build against prebuilts from android13-5.15.
# Build with --use_prebuilt_gki=<build_ID>. Example:
$ tools/bazel run --use_prebuilt_gki=8728678 //gs/google-modules/soc-modules:slider_dist
```

### Downloading the signed boot images

If you want to download the signed boot images instead of the unsigned one, you may
specify `--use_signed_prebuilts`. This requires the build number in `--use_prebuilt_gki`.
Hypothetical example: (this does not work because 8728678 is unsigned):

```shell
$ tools/bazel run --use_prebuilt_gki=8728678 --use_signed_prebuilts //gs/google-modules/soc-modules:slider_dist
```

## Common errors

You may see an error about failing to download a file because it does not
exist ("404 Not Found"). For example:

```text
ERROR: An error occurred during the fetch of repository '<filename>':
   Traceback (most recent call last):
        File "/mnt/sdc/android/raviole-mainline/build/kernel/kleaf/download_repo.bzl", line 128, column 48, in _download_artifact_repo_impl
                download_info = repository_ctx.download(
Error in download: java.io.IOException: Error downloading [<url>] to <path>: GET returned 404 Not Found
```

To resolve this, try using the latest build ID from the branch.

If you are still unable to resolve the issue, you may:
- contact [owners](../OWNERS) or [kernel-team@android.com](mailto:kernel-team@android.com)
- contact your Technical Account Manager to file a bug

### Using a custom CI host

It is possible to specify a different endpoint to download prebuilt artifacts, by customizing the
[workspace](https://bazel.build/concepts/build-ref#workspace) setup similar to what is done for
[kleaf-docs branch](https://android.googlesource.com/kernel/manifest/+/5ea7995b7c75cb30f42224b0273a1516627075c6/default.xml#10).

  * Provide a correct value for `artifact_url_fmt`.
  ```shell
  ...
  load("//build/kernel/kleaf:workspace.bzl", "define_kleaf_workspace")

  define_kleaf_workspace(artifact_url_fmt = "https://ci.android.com/builds/submitted/{build_number}/{target}/latest/raw/{filename}")
  ...
  ```

Note: The format may include anchors for the following properties: build_number, target, filename.

