# `GCOV`

When the flag `--gcov` is set, the build is reconfigured to produce (and keep)
`*.gcno` files.

For example:

```shell
$ tools/bazel build --gcov //common:kernel_aarch64
```

You may find the `*.gcno` files under the
`bazel-bin/<package_name>/<target_name>/<target_name>_gcno` directory,
where `<target_name>` is the name of the `kernel_build()`
macro. In the above example, the `.gcno` files can be found at

```
bazel-bin/common/kernel_aarch64/kernel_aarch64_gcno/
```

... or in `dist_dir`:

```shell
$ tools/bazel run --gcov //common:kernel_aarch64_dist
[...]
$ ls bazel-bin/common/kernel_aarch64/kernel_aarch64_gcno/
[...]
```

## Handling path mapping

After you boot up the kernel and [mount debugfs](https://docs.kernel.org/filesystems/debugfs.html):

```shell
$ mount -t debugfs debugfs /sys/kernel/debug
```

You may see gcno files under:

```
/sys/kernel/debug/gcov/<some_host_absolute_path_to_repository>/<some_out_directory>/common/<some_source_file>.gcno
```

To map between these paths to the host, consult the `gcno_mapping.<name>.json`
under `bazel-bin/`.

### GKI

In the above example, the file can be found after a build:

```shell
$ tools/bazel build --gcov //common:kernel_aarch64
[...]
$ cat bazel-bin/common/kernel_aarch64/gcno_mapping.kernel_aarch64.json
[...]
```

You may also find this file under `dist_dir`:

```shell
$ tools/bazel run //common:kernel_aarch64_dist -- --dist_dir=out/kernel_aarch64/dist
[...]
$ cat out/kernel_aarch64/dist/gcno_mapping.kernel_aarch64.json
[...]
```

### Device mixed builds

You need to consult the JSON file for the device `kernel_build`.
Using virtual device as an example, you may find the files under:

```shell
$ tools/bazel build --gcov //common-modules/virtual-device:virtual_device_x86_64
[...]
$ cat bazel-bin/common-modules/virtual-device/virtual_device_x86_64/gcno_mapping.virtual_device_x86_64.json
[...]
```

Or under `dist_dir`:

```shell
$ tools/bazel run --gcov //common-modules/virtual-device:virtual_device_x86_64_dist -- --dist_dir=out/vd/dist
[...]
$ cat out/vd/dist/gcno_mapping.virtual_device_x86_64.json
[...]
```

**Note**: You will also see `gcno_mapping.kernel_x86_64.json` under `dist_dir`. That file is incomplete
as it does not contain mappings for in-tree modules specific for virtual device.

### Sample content of `gcno_mapping.<name>.json`:

Without `--config=local` (see [sandboxing](sandbox.md)):

```json
[
  {
    "from": "/<repository_root>/out/bazel/output_user_root/.../__main__/out.../android-mainline/common",
    "to": "bazel-out/.../kernel_x86_64/gcno"
  }
]
```

With `--config=local` (see [sandboxing](sandbox.md)):

```json
[
  {
    "from": "/mnt/sdc/android/kernel/out/cache/.../common",
    "to": "bazel-out/k8-fastbuild/bin/common/kernel_aarch64/gcno"
  }
]
```

The JSON file contains a list of mappings. Each mapping indicates that the `.gcno` files
located in `<from>` were copied to `<to>`. Hence, `/sys/kernel/debug/<from>`
on the device maps to `<to>` on host.

**Note**: For both `<from>` and `<to>`, absolute paths should be interpreted as-is,
and relative paths should be interpreted as relative to the repository on host. For example:

```json
[
  {
    "from": "/absolute/from",
    "to": "/absolute/to"
  },
  {
    "from": "relative/from",
    "to": "relative/to"
  }
]
```

This means:
* Device `/sys/kernel/debug/absolute/from` maps to host `/absolute/to`
* Device `/sys/kernel/debug/<repositry_root>/relative/from` maps to host `/<repository_root>/relative/to`.
