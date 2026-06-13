# GDB scripts

To enable `CONFIG_GDB_SCRIPTS` and collect the scripts, use the `--kgdb`
flag.

The scripts may be found under `bazel-bin/<package>/<target_name>/gdb_scripts`.

## Example

Example command for virtual devices:

```shell
tools/bazel build //common-modules/virtual-device:virtual_device_x86_64 --kgdb
```

You may find the scripts under
`bazel-bin/common-modules/virtual-device/virtual_device_x86_64/gdb_scripts`
.
