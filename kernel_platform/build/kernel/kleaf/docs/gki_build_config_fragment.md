# Supporting GKI\_BUILD\_CONFIG\_FRAGMENT on Kleaf

**WARNING:** Build configs are being deprecated. The support for this will stop
once the migration of all [configs](build_configs.md) is completed. Before
trying this approach make sure your use case is not covered by
[flags](README.md#flags) or by customizing [.bazelrc files](impl.md#bazelrc-files).

**NOTE:** If you find a use case for a new flag, please email
kernel-team@android.com so we can discuss its addition to Kleaf.

The **debug** option `--gki_build_config_fragment` allow developers to use a
build config fragment to modify/override the GKI build config for debugging
purposes.

The following is an example of how to use this debug option for the
virtual\_device\aarch64 build (from
[common-modules/virtual-device](https://android.googlesource.com/kernel/common-modules/virtual-device/+/refs/heads/android-mainline)).

A developer will need to provide the target containing the fragment(s) to be
used. For example if the fragment is `build.config.gki.sample.fragment` , the
following [filegroup](https://bazel.build/reference/be/general#filegroup) can be
used:

```shell
filegroup(
    name = "sample_gki_config_fragment",
    srcs = [
        "build.config.gki.sample.fragment",
    ],
)
```

This target can now be used like this:

```shell
tools/bazel build //common-modules/virtual-device:virtual_device_aarch64 --gki_build_config_fragment=//common-modules/virtual-device:sample_gki_config_fragment
```
