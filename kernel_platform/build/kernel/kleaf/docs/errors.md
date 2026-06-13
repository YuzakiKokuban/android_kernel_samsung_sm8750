# Resolving Common Errors when Writing Bazel Rules

This document explains how to deal with common errors when
[building your kernels and drivers with Kleaf](impl.md).

## /bin/sh: line 1: clang: command not found

This error may happen after the clang version and `CLANG_PREBUILT_BIN` is
updated, but corresponding Bazel rules aren't.

To fix, check clang version. Specifically, check the attribute
`toolchain_version` of `kernel_build` against the value in `CLANG_PREBUILT_BIN`
(usually in `build.config.common` or `build.config.constants`).

Example:
[CL 1918433](https://android-review.googlesource.com/c/kernel/common/+/1918433/3/BUILD.bazel)
updates `toolchain_version` to be `CLANG_VERSION`, which is loaded from
[build.config.constants](https://android-review.googlesource.com/c/kernel/common/+/1918432/3/build.config.constants)
.

## fatal error: '[some\_header\_file].h' file not found {#header-not-found}

This means the header file is not visible to the kernel module build.

If there are multiple matches, first determine which one is needed by examining
the `Makefile` / `Kbuild` file.

Try to find this header file in the kernel module source tree. Example (for
Pixel):

```sh
$ find gs/google-modules -name some_header_file.h
```

If the header file is in the same directory as the kernel module or a
subdirectory, check that `srcs` include the header file. Example:

```bazel
# Adds the file explicitly
srcs = [
  "some_header_file.h"
],

# Adds header files from the same directory as the BUILD.bazel file,
# but NOT subdirectories
srcs = glob(["*.h"]),

# Adds header files from the same directory as the BUILD.bazel file
# INCLUDING matches from subdirectories
srcs = glob(["**/*.h"]),
```

If the header file is in the directory of a different kernel module:

*   Declare a `filegroup` for headers in the other directory, and add this
    module to the `visibility` attribute
*   Add the `filegroup` to `srcs` (as a label)

Example:
[Export headers from BMS](https://android.googlesource.com/kernel/google-modules/bms/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel),
[Use BMS headers for Power Reset module](https://android.googlesource.com/kernel/google-modules/power/reset/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel)

## WARNING: Symbol version dump "[...]/foo/Module.symvers" is missing. {#module-symvers-missing}

This means a kernel module dependency is missing.

**Solution**:

* Try the Kleaf [`build_cleaner`](build_cleaner.md).
* If that doesn't work, manually add the `kernel_module` in `foo` to
  `deps` of the build rule.

Example:
[Power Reset module depends on BMS](https://android.googlesource.com/kernel/google-modules/power/reset/+/refs/heads/android-gs-raviole-mainline/BUILD.bazel).

## ERROR: modpost: "foo" [.../mod_using_foo.ko] undefined! {#modpost-symbol-undefined}

**Solution**:

* First, ensure the `Module.symvers` file from the module defining `foo` is
  present. See [this section](#module-symvers-missing).
* For `kernel_module`s, set `KBUILD_EXTRA_SYMBOLS` accordingly in `Makefile`. Example:
  [Makefile for Power Reset module](https://android.googlesource.com/kernel/google-modules/power/reset/+/refs/heads/android-gs-raviole-mainline/Makefile)
  . This is unnecessary for `ddk_module` because `Makefile` is generated.

## Exception: Unable to find \[some file\] in any of the following directories: ... {#no-files-match}

First, check whether `[some_module].ko` should be an output of the
`kernel_module` rule or not by checking the `Makefile` / `Kbuild` files.

If not, remove its declaration in `BUILD.bazel` in the `outs` attribute of the
`kernel_module` rule.

If it should be an output, it is likely that a previous error prevents it from
being built. Check previous error messages.

If you can't see any errors, run with `--sandbox_debug` to prevent the sandbox
to be teared down.

**Note**: Using `--sandbox_debug` may leave a huge amount of symlinks undeleted,
and prevent sandbox from being unmounted. Repeat use of `--sandbox_debug` may
affect the performance of your host machine. Use sparingly with caution.

Example:

```sh
$ bazel build --verbose_failures --sandbox_debug //gs/google-modules/foo/..
```

With that option, you should see where the sandbox is mounted. Example:

```
<timestamp>: src/main/tools/linux-sandbox-pid1.cc:279: remount rw: <workspace_root>/out/bazel/output_user_root/<hash>/sandbox/linux-sandbox/<sandbox_number>/execroot/__main__
```

Concatenate the sandbox root and the file mentioned in the error message to see
why it is not there. For example, if the error message was

```
Exception: In bazel-out/k8-fastbuild/bin/gs/google-modules/foo/staging/lib/modules/5.10.43/extra, no files match foo_module.ko, expected 1
```

Go to the directory and search for the file:

```sh
$ find \
<workspace_root>/out/bazel/output_user_root/<hash>/sandbox/linux-sandbox/<sandbox_number>/execroot/__main__/bazel-out/k8-fastbuild/bin/private/google-modules/foo/staging/lib/modules/5.10.43/extra \
-name foo_module.ko
```

Check why it is missing.

## Exception: In \[some directory\], multiple files match \[some file\], expected at most 1 {#multiple-files-match}

Same as above section ([no files match](#no-files-match)), except that in the
last step, check which installed files have the given name. For example, you may
see:

```
foo/my_module.ko
bar/my_module.ko
```

In the `outs` attribute, instead of specifying a vague `my_module.ko`, try to
specify the full path so the build system knows which file you are referring to:

```bazel
outs = [
  # This is confusing to the build system. Avoid this.
  # "my_module.ko",

  # Do this instead.
  "for/my_module.ko",
  "bar/my_module.ko",
],
```

## There's an internal problem with your device / Missing SHA in `/proc/version`

If `/proc/version` shows something like

```
5.14.0-mainline
```

without a SHA, the SCM version is not embedded.

This is also the cause of the following dialog on the phone screen with Android
12 userspace:

```
There's an internal problem with your device. Contact your manufacturer for details.
```

The dialog should not show if your device is running Android 13 in userspace
with
[CL 1843574](https://android-review.googlesource.com/c/platform/system/libvintf/+/1843574/)
.

The SCM version is only embedded for `--config=stamp` or any other configs that
inherits from it (e.g. `--config=release`).

The SCM version should be embedded properly on release builds, where
`--config=release` must be specified.

**Solutions**:
- You may hide the dialog by cherry-picking
  [CL 1843574](https://android-review.googlesource.com/c/platform/system/libvintf/+/1843574/)
  if you are running Android 12 in userspace.
- You may embed SCM version in local builds with `--config=stamp`.

See [scmversion.md](scmversion.md) for details.

## error: unable to open output file [...]: 'Operation not permitted' {#operation-not-permitted}

Check what the path of the file is. If it looks like this, continue reading:

```text
/<workspace_root>/out/bazel/output_user_root/<hash>/sandbox/linux-sandbox/<sandbox_number>/execroot/__main__/<some_SOURCE_directory>/<some_output_file>
```

`<some_SOURCE_directory>` means that it **does NOT start with `out/`**. For
example:

*   `gs/`
*   `common/`
*   `private/`
*   etc.

`<some_output_file>` means that it is an output file, not a source file. For
example:

*   `*.ko`
*   `*.o`
*   etc.

This is likely because `make` has been invoked in `<some_SOURCE_directory>`
without the proper `O=` argument, or Kbuild did not respect the `O=` argument
and put output files under the source directory.

However, the Bazel build system treats them as a source file because it is part
of the `glob()`. When the Bazel build system invokes Kbuild, Kbuild tries to
write those files. But within the sandbox, the source tree is readonly,
triggering the permission denial.

You may verify that this is the case by checking the existance of the output
file in the source tree:

```sh
$ ls /<workspace_root>/<some_SOURCE_directory>/<some_output_file>
```

To restore your repository to the normal state in the short term, try one of the
following under `<workspace_root>/<some_SOURCE_directory>`, or a parent
directory:

*   `make clean`
*   `git clean -fdx` (Be careful! This deletes all git ignored files.)

There should not be any output files in the source tree, other than in `out/`.

To properly fix this, investigate the Kbuild definition to see why `O=` is not
respected. Output files should be contained into the directory pointed at `O=`.

## WARNING: Unable to determine EXT_MODULES; scmversion for external modules may be incorrect. [...] <workspace_root>/build.config: No such file or directory

If your device builds external modules, create the top-level `build.config`
symlink to point to the `build.config` file so scmversions for external
modules are inferred correctly.

You may ignore this warning:
- if your device does not build external modules;
- if you are building GKI.

For details, see [scmversion.md](scmversion.md).

## rm: cannot remove 'out/bazel/output_user_root/<hash>/execroot/\_\_main\_\_/bazel-out/k8-fastbuild/bin/<...>

**Note**: `--experimental_writable_outputs` is now enabled by default. If you
still see this error, it may be due to left-over directories from builds before
`--experimental_writable_outputs` is enabled. You may execute
`tools/bazel clean` one last time. Then, you should no longer need to run
`tools/bazel clean` before `rm -rf out/`.

If you try to `rm -rf out/` and get the above message, this is because Bazel
removes the write permission on output directories.

It is not needed to clean the output directory for consistency of build
results.

However, if you need to clean the `out/` directory to
save disk space, you may run `tools/bazel clean`. See
documentation for the `clean` command
[here](https://bazel.build/docs/user-manual#cleaning-build-outputs).

## cp: <workspace\_root>/out/bazel/output\_user\_root/[...]/execroot/\_\_main\_\_/[...]/[...]_defconfig: Read-only file system {#defconfig-readonly}

This is likely because a previous `--config=local` build was interrupted and
did not clean up the `$ROOT_DIR/$KERNEL_DIR/$DEFCONFIG` file.

This may cause `POST_DEFCONFIG_CMDS` to not be executed. Or
`POST_DEFCONFIG_CMDS` is not defined to clean up `$DEFCONFIG`.

To restore the workspace to a build-able state, manually delete the generated
`$DEFCONFIG` file in the source tree.

**HINT**: You may execute the Bazel command with
`--strip_execroot` to get a cleaner path of the file that needs
to be deleted.

To prevent `--config=local` builds from writing `$DEFCONFIG` into
the source tree in the future, you may modify `PRE_DEFCONFIG_CMDS` to
write to `\${OUT_DIR}` instead. Note that because `${OUT_DIR}` is not
defined when `build.config` is loaded, the preceding `$` must be escaped
so `$OUT_DIR` is evaluated properly when `$PRE_DEFCONFIG_CMDS` are executed.

You may also stick with sandboxed builds (i.e. not using `--config=local`)
to prevent this in the future. See [sandbox.md](sandbox.md).

See
[CL:2082199](https://android-review.googlesource.com/2082199) for an example.

## `signing_key.pem` not found

If you see an error like the following:

```text
At main.c:172:
- SSL error:02000002:system library:OPENSSL_internal:No such file or directory: external/boringssl/src/crypto/bio/file.c:98
- SSL error:1100006e:BIO routines:OPENSSL_internal:NO_SUCH_FILE: external/boringssl/src/crypto/bio/file.c:102
sign-file: <execroot>/common/certs/signing_key.pem
```

Add the following line to defconfig, or config fragment:

```text
# CONFIG_MODULE_SIG_ALL is not set
```

See the following change for an example:

[ANDROID: kleaf: convert fips140 to kleaf](https://android-review.googlesource.com/c/kernel/common/+/2212995)

## unterminated call to function 'wildcard': missing ')'.  Stop. {#unterminated-call-to-function-wildcard}

If you see an error like the following when using `--config=local`:

```
ERROR: <...>/BUILD.bazel:5:14: Building external kernel module <...> failed: (Exit 2): bash failed: error executing command (from target <...>) /bin/bash -c ... (remaining 1 argument skipped)
<path>/.<filename>.o.cmd:5: *** unterminated call to function 'wildcard': missing ')'.  Stop.
```

This is a known issue with `--config=local`. The root cause of the issue is
unknown. If you see this error, please file a bug with the following
information:

- Rebuild with
  `--verbose_failures --debug_cache_dir_conflict=detect --profile=/tmp/command.profile.gz`
- Record the full build log
- Provide `/tmp/command.profile.gz`; see
  [JSON trace profile](https://bazel.build/advanced/performance/json-trace-profile)

After filing the bug, you may use one of the methods below to work around the
issue:

- You may run `tools/bazel clean` and try the build again. You may or may not
  see the error again afterwards.
- You may rebuild with `--debug_cache_dir_conflict=resolve`.

## Build hangs on BTRFS

If your tree is on BTRFS, and your Android kernel source tree contains the
following patch, building `kernel_build` may hang.

[FROMLIST: kheaders: dereferences the source tree](http://r.android.com/2626404)

This is a known issue:

[Bug 217681 - gen_kheaders.sh gets stuck in an infinite loop](https://bugzilla.kernel.org/show_bug.cgi?id=217681)

You may update your host machine's kernel to a version that includes the following patch
to fix the problem.

[[PATCH] btrfs: fix infinite directory reads](https://lore.kernel.org/linux-btrfs/c9ceb0e15d92d0634600603b38965d9b6d986b6d.1691923900.git.fdmanana@suse.com/)

You may also use `--workaround_btrfs_b292212788` to work around this issue
temporarily.

## fatal: not a git repository: '[...]/.git' {#not-git}

This is a harmless warning message.

[comment]: <> (Bug 194427140)

## date: invalid date '@' {#invalid-date}

This is a harmless warning message.

[comment]: <> (Bug 194427140)

## date: bad date +%s {#bad-date}

This is a harmless warning message.

[comment]: <> (Bug 194427140)

