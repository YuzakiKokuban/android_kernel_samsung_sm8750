# Supporting ABI monitoring with Bazel

## ABI monitoring for GKI builds

### Build kernel and ABI artifacts

```shell
$ tools/bazel run //common:kernel_aarch64_abi_dist -- --dist_dir=out/dist
```

This compares the current ABI (`abi_definition_stg` of `//common:kernel_aarch64`,
which is `common/android/abi_gki_aarch64.stg`) and the freshly-generated ABI
from the built kernel image and modules, and generates a diff report. This also
builds all ABI-related artifacts for distribution, and copies them to
`out/dist` (or `out_abi/kernel_aarch64/dist` if `--dist_dir` is not specified).
The exit code reflects whether an ABI change is detected in the
comparison, just like `build_abi.sh`.

### Update symbol list

```shell
$ tools/bazel run //common:kernel_aarch64_abi_update_symbol_list
```

This updates `kmi_symbol_list` of `//common:kernel_aarch64`, which is
`common/android/abi_gki_aarch64`.

### Update the protected exports list {#update-protected-exports}

Similar to [updating the KMI symbol list for GKI](abi.md#update-symbol-list),
you may update the `protected_exports_list` defined previously with the
following.

```shell
$ tools/bazel run //path/to/package:{name}_abi_update_protected_exports
```

In the above example for kernel\_aarch64, the command is

```shell
$ tools/bazel run //common:kernel_aarch64_abi_update_protected_exports
```

This updates `common/android/abi_gki_protected_exports`.

### Extracting the ABI

```shell
$ tools/bazel build //common:kernel_aarch64_abi_dump
```

### Update the ABI definition {#update-abi}

**Note**: You must [update the symbol list](#update-symbol-list) and
[update the protected exports list](#update-protected-exports) before
updating the ABI definition. The Bazel command below does not also update
the source symbol list, unlike the `build_abi.sh` command.

If ABI definition doesn't exists i.e. if this is the first time it is being
generated then first and empty symbol file needs to be created and the symbol
list needs to be generated using the `nodiff_update` target as below:

```shell
touch common/android/abi_gki_aarch64.stg
$ tools/bazel run //common:kernel_aarch64_abi_nodiff_update
```

Second time onwards you can use the `//common:kernel_aarch64_abi_update` target
as below:

```shell
$ tools/bazel run //common:kernel_aarch64_abi_update
```

This compares the ABIs, then updates the `abi_definition`
of `//common:kernel_aarch64`, which is `common/android/abi_gki_aarch64.stg`. The
exit code reflects whether an ABI change is detected in the comparison, just
like `build_abi.sh --update`.

Running the script with `--print_git_commands` prints git commands you
may run to create a commit with pre-filled message. For example:

```shell
# -- is needed before --print_git_commands to pass the argument to the script.
$ tools/bazel run //common:kernel_aarch64_abi_update -- --print_git_commands
```

Example output:

```
$ tools/bazel run //common:kernel_aarch64_abi_update -- --print_git_commands
[...]
INFO: Running command line: bazel-bin/common/kernel_aarch64_abi_update.sh --print_git_commands
  git -C /path/to/common/android commit \
    -F /path/to/bazel-out/k8-fastbuild/bin/common/kernel_aarch64_abi_diff/git_message.txt \
    --signoff --edit -- abi_gki_aarch64.stg
```

**NOTE**: The printed command (especially the path to git_message.txt) may
occasionally change. Always run with `--print_git_commands` to get the current
command for your source tree.

The command brings up your pre-configured text editor for git to edit the
commit message. You may edit the subject line, add additional message, and add
a bug number.

If you do not wish to compare the ABIs before the update, you may execute the
following instead:

```shell
$ tools/bazel run //common:kernel_aarch64_abi_nodiff_update
```

### Convert from `build_abi.sh`

Here's a table for converting `build_abi.sh`
into Bazel commands, assuming `BUILD_CONFIG=common/build.config.gki.aarch64`
for `build_abi.sh`.

**NOTE**: It is recommended to run these commands with `--config=local` so
`$OUT_DIR` is cached, similar to how `build_abi.sh` sets `SKIP_MRPROPER`. See
[sandbox.md](sandbox.md) for more details.

**NOTE**: `build_abi.sh` will try to provide an equivalent Bazel command
according to the arguments it's given. So you don't have to look it up here.

```shell
# build_abi.sh --update_symbol_list
# Update symbol list [1]
$ tools/bazel run kernel_aarch64_abi_update_symbol_list

# build_abi.sh --nodiff
# Extract the ABI (but do not compare it) [2]
$ tools/bazel build kernel_aarch64_abi_dump

# build_abi.sh --nodiff --update
# Update symbol list, [1][3]
$ tools/bazel run kernel_aarch64_abi_update_symbol_list &&
# Extract the ABI (but do not compare it), then update `abi_definition` [2][3]
> tools/bazel run kernel_aarch64_abi_nodiff_update

# build_abi.sh --update
# Update symbol list, [1][3]
$ tools/bazel run kernel_aarch64_abi_update_symbol_list &&
# Extract the ABI and compare it, then update `abi_definition` [2][3]
> tools/bazel run kernel_aarch64_abi_update

# build_abi.sh
# Extract the ABI and compare it, then copy artifacts to distribution directory
$ tools/bazel run kernel_aarch64_abi_dist
```

Notes:

1. The command updates `kmi_symbol_list` but it does not update
   `$DIST_DIR/abi_symbollist`, unlike the `build_abi.sh --update-symbol-list`
   command.
2. The Bazel command extracts the ABI and/or compares the ABI like the
   `build_abi.sh` command, but it does not copy the ABI dump and/or the diff
   report to `$DIST_DIR` like the `build_abi.sh` command. You may find the ABI
   dump in Bazel's output directory under `bazel-bin/`.
3. Order matters, and the commands cannot run in parallel. This is because
   updating the ABI definition requires the **source**
   `kmi_symbol_list` to be updated first.
