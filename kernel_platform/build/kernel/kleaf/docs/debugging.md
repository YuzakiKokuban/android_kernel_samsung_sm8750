# Debugging Kleaf

## Debugging Options

This is a non exhaustive list of options to help debugging compilation issues:

*   Customise Kleaf:

    *   `--debug_annotate_scripts`: Runs all script invocations with `set -x`
        and a trap that executes `date` after every command.

    *   `--debug_print_scripts`: Prints the content of the (generated) command
        scripts during rule execution.

    *   `--debug_cache_dir_conflict={detect,resolve}`: Attempt to detect or
        resolve any conflicts if multiple actions uses the same subdirectory
        within `--cache_dir` simultaneously.

*   Customise Kbuild:

    *   `--debug_make_verbosity`: Controls verbosity of `make` executions.
        *   `E` (default): Only print errors (`make -s`)
        *   `I`: print brief description of make targets being built (`make`)
        *   `D`: print full commands (`make V=1`)
        *   `V`: print the reason for the rebuild of each make target
            (`make V=2`)

    *   `--debug_modpost_warn`: Sets
        [`KBUILD_MODPOST_WARN=1`](https://www.kernel.org/doc/html/latest/kbuild/kbuild.html#kbuild-modpost-warn).
        TL; DR. can be set to avoid errors in case of undefined symbols in the
        final module linking stage. It changes such errors into warnings.

*   Customise Bazel:

    *   `--sandbox_debug`: Enables debugging features for the
        [sandboxing feature](https://bazel.build/docs/sandboxing).
    *   `--verbose_failures`: If a command fails, print out the full command
        line.
    *   `--jobs`: This option, which takes an integer argument, specifies a
        limit on the number of jobs that should be executed concurrently during
        the execution phase of the build.
    *   For a complete list see https://bazel.build/docs/user-manual

## Disabling checks

This is a list of options to disable checks in Kleaf due to various reasons. For
example, some checks may be disabled during device bring-up for a quick
development cycle. Usually, these flags should not be set on a release build.

*   `--allow_ddk_unsafe_headers`: Allow DDK modules to also use the unsafe
    header list in the common package.
*   `--allow_undeclared_modules`: Allow modules to be undeclared in
    `kernel_build.module_outs` and `kernel_build.module_implicit_outs`. If
    modules are built but not declared in these lists, Kleaf emits a warning
    unless `--nowarn_undeclared_modules` is set.
*   `--nowarn_undeclared_modules`: Allow modules to be undeclared in
    `kernel_build.module_outs` and `kernel_build.module_implicit_outs`. No
    warnings are generated.
*   `--nokmi_symbol_list_strict_mode`: Disable KMI symbol list check.
*   `--nokmi_symbol_list_violations_check`: Disable KMI symbol list violations
    check.

**NOTE**: In addition to `--nokmi_symbol_list_strict_mode` and
`--nokmi_symbol_list_violations_check` the following list of
[predefined flags](kernel_config.md#other-pre_defined-flags) also skip the
symbol list and symbol list violations checks (`--notrim`, `--debug`, `--gcov`,
`--k*san`, `--kgdb`).

## Debugging incremental build issues

Incremental build issues refers to issues where actions are executed in an
incremental build, but you do not expect them to be executed, or the reverse.

For example, if you are debugging why
`//common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_pipe` is
rebuilt after you change a core kernel file, you may execute the following:

```shell
# Custom flags provided to the build; change accordingly
$ FLAGS="--config=fast"

# Build
$ tools/bazel build "${FLAGS}" //common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_pipe

# Record hashes of all input files to the action
# Note that kernel_module() defines multiple actions, so use mnemonic() to filter out
# the non-interesting ones.
$ build/kernel/kleaf/analysis/inputs.py -- "${FLAGS}" \
  'mnemonic(KernelModule, //common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_pipe)' \
  > out/hash_1.txt

# Change a core kernel file, e.g.
$ echo >> common/kernel/sched/core.c

# Build again
$ tools/bazel build "${FLAGS}" //common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_pipe

# Record hashes of all input files to the action
$ build/kernel/kleaf/analysis/inputs.py -- "${FLAGS}" \
  'mnemonic(KernelModule, //common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_pipe)' \
  > out/hash_2.txt

# Compare hashes, e.g.
$ diff out/hash_1.txt out/hash_2.txt
```

Positional arguments to `build/kernel/kleaf/analysis/inputs.py` are fed directly
to `tools/bazel aquery`. Visit
[Action Graph Query](https://bazel.build/query/aquery) for the query language.

## Debugging dependencies on external repositories

If you see an error like this:

```
ERROR: An error occurred during the fetch of repository 'rules_python':
   Traceback (most recent call last):
        File "<...>/http.bzl", line 132, column 45, in _http_archive_impl
                download_info = ctx.download_and_extract(
[...]
ERROR: <...>:24:22: While resolving toolchains for target <...>: invalid registered toolchain '@bazel_tools//tools/jdk:all': while parsing '@bazel_tools//tools/jdk:all': no such package '@rules_python//python': java.io.IOException: Error downloading <...>
```

In this example, the error message suggests that `@bazel_tools//tools/jdk:all`
has a dependency on `@rules_python`.

If this error is unexpected, you may try these commands to diagnose issues with
external repositories:

```sh
rm -rf /tmp/temp_repo_cache && mkdir -p /tmp/temp_repo_cache
bazel clean --expunge
bazel query @bazel_tools//tools/jdk:all \
  --repository_cache=/tmp/temp_repo_cache \
  --experimental_repository_disable_download
```

## Debugging target `providers`

Inspecting the information exposed by bazel targets via
[providers](https://bazel.build/extending/rules#providers) is possible following
[Defining the output format using Starlark](https://bazel.build/query/cquery#output-format-definition)
docs.

Here is an example used in
[CL:2615849](https://android-review.googlesource.com/c/kernel/build/+/2615849)
to inspect the information exposed by `KernelBuildAbiInfo` from
`//common:kernel_aarch64_download_or_build` target.

```sh
$ tools/bazel cquery //common:kernel_aarch64_download_or_build --use_prebuilt_gki=10283028  --output=starlark --starlark:expr='providers(target)["//build/kernel/kleaf/impl:common_providers.bzl%KernelBuildAbiInfo"]'

...
struct(module_outs_file = <source file file/kernel_aarch64_modules>, modules_staging_archive = <source file file/modules_staging_dir.tar.gz>, src_protected_modules_list = <source file file/gki_aarch64_protected_modules>)
...

```

## Reproducing sandboxed commands

Sometimes you need to reproduce commands that was exectuted in hermetic sandbox
environment, for example, to catch a bug in compiler. Then you may follow these
approximate steps:

1. Run `tools/bazel build --debug_make_verbosity=D --debug_print_scripts
   --sandbox_debug //common:kernel_aarch64`. These options will print out all
   executed commands and preserve sandbox environment.
2. Run `find out -name 'command.log'` and open log in your favorite text editor.
   This log have the same contents as stdout and stderr of Bazel invocation.
3. Find in log for example `drivers/net/usb/usbnet.o` to get corresponding
   compiler invocation.
4. Search *up* for `Run this command to start an interactive shell in an
   identical sandboxed environment`. There may be multiple sandboxed
   environments, you need to find the closest one before the interesting
   command.
5. Search *down* from here for `+ export PATH=`. Plus sign is important, it will
   show an actual path to hermetic toolchain with expanded variables.
6. Run commands from step 4 to get into sandboxed environment, then from step 5
   to have toolchain binaries in your path, and then from step 3 to reproduce
   compiler invocation.
