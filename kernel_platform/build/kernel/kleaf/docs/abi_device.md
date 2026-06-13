# Supporting ABI monitoring with Bazel (device kernel)

## Enable ABI monitoring

ABI monitoring only needs to be configured for core kernel build targets. Mixed
build configurations (ones that define `base_kernel`) that compile directly
with the GKI kernel **only need to add support for tracking the device symbol
list**. The ABI definition **should** be updated using the GKI build.

To set up the device symbol list, update the relevant targets in `BUILD.bazel`.

### Update the `kernel_build` target

Add these attributes to the `kernel_build` target for KMI symbol list support:

- `kmi_symbol_list`: The KMI symbol list for the device, e.g.
  `"//common:android/abi_gki_aarch64_virtual_device"`.

### Define a `kernel_abi` target

Add a new `kernel_abi` target. The target should usually be named `{name}_abi`
by convention, where `{name}` is the name of the `kernel_build` target.

Add these attributes to the `kernel_abi` target for KMI symbol list support:

- `kernel_build`: point to the `kernel_build` target.
- `kernel_modules`: The list of external modules. This should be consistent with
  the `kernel_modules_install` target.

You may optionally define the following:

- `define_abi_targets`
    - This is `True` by default. If `False`, ABI monitoring is disabled.
    - This is useful to minimize the difference in the source tree between
      `android-mainline` and a KMI frozen branch.
- `module_grouping`
- `kmi_symbol_list_add_only`
- `kmi_enforced`

See documentation for explanation of the `kernel_abi` rule and its
attributes: [API Reference and Documentation for all rules](api_reference.md).

### Example for virtual\_device\_aarch64

See `virtual_device_aarch64_abi` in [common-modules/virtual-device/BUILD.bazel](https://android.googlesource.com/kernel/common-modules/virtual-device/+/refs/heads/android14-5.15/BUILD.bazel) for an
example.

## Update the KMI symbol list {#update-symbol-list}

Similar to [updating the KMI symbol list for GKI](abi.md#update-symbol-list),
you may update the `kmi_symbol_list` defined previously with the following.

```shell
$ tools/bazel run //path/to/package:{name}_abi_update_symbol_list
```

In the above example for virtual\_device\_aarch64, the command is

```shell
$ tools/bazel run //common-modules/virtual-device:virtual_device_aarch64_abi_update_symbol_list
```

This updates `common/android/abi_gki_aarch64_virtual_device` file.

## Update the ABI definition

After the [KMI symbol list updated](#update-symbol-list) you may
update the ABI definition at `common/android/abi_gki_aarch64.stg` with the
following command:

```shell
$ tools/bazel run //common:kernel_aarch64_abi_update
```

See [abi.md#update-abi](abi.md#update-abi).

**NOTE**: Do not update the ABI definition via the `virtual_device_aarch64_abi_update` target!
The ABI definition should always be updated via the GKI
`kernel_aarch64_abi_update` target.

