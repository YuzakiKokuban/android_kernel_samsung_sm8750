# Resolving common errors when developing with the DDK

## Table of contents

[Generic steps for resolving missing header](#missing-headers)

[Missing include/linux/compiler-version.h](#missing-compiler-version-h)

[Missing include/linux/kconfig.h](#missing-compiler-version-h)

[Incorrect header is included](#incorrect-header)

[modpost symbol undefined](#modpost-symbol-undefined)

[Use out instead of outs](#outs)

[Missing Abseil Python](#missing-abseil-python)

[Appendix](#appendix)

## `<source>.c:<line>:<col>: fatal error: '<header>.h file not found` {#missing-headers}

Resolving errors about missing headers can be tough. In general, debugging these
errors involve the following steps:

1. Check where the requested header is
2. Check all of the include directories of the DDK module
3. Add the requested header and necessary include directories to the module

### Find a certain header with the given name

This step is straightforward with a `find(1)` command. Example: if the error is

```text
#include <linux/i2c.h>
         ^~~~~~~~~~~~~
```

Then you can look for it with

```shell
$ find . -path "*/linux/i2c.h"
./common/include/uapi/linux/i2c.h
[... other results]
```

The above search result indicates that one expected search directory for
`linux/i2c.h` is `common/include/uapi`.

**NOTE**: There might be multiple matches. However, usually you only want to
include a specific one.

### Step 1: Check all of the include directories of the DDK module

There are multiple ways to do this. You may look at the generated `Kbuild` file.

Example: If you are compiling
`//common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_sync`, you may
look at:

```shell
$ grep -rn 'goldfish_sync' bazel-bin/common-modules/virtual-device/
[...]
bazel-bin/common-modules/virtual-device/goldfish_drivers/goldfish_sync_makefiles/makefiles/goldfish_drivers/Kbuild:2:obj-m += goldfish_sync.o
[...]

$ grep 'ccflags-y' bazel-bin/common-modules/virtual-device/x86_64/goldfish_drivers/goldfish_sync_makefiles/makefiles/goldfish_drivers/Kbuild
ccflags-y += '-I$(srctree)/$(src)/../../../common/include/uapi'
[... other ccflags-y]
```

The expression `$(srctree)/$(src)` evaluates to
`<package>/<dirname of output module>`.

* Package is `common-modules/virtual-device`.
* Output module is the `out` attribute of the
  `ddk_module` target, which defaults to `<target name>.ko`. In this case, it
  is `goldfish_drivers/goldfish_sync.ko`.

Hence `$(srctree)/$(src)`
is` common-modules/virtual-device/goldfish_drivers` in this case.

Hence, the above include directory points to

```text
<repository_root>/common-modules/virtual-device/goldfish_drivers/../../../common/include/uapi
```

which is just

```text
<repository_root>/common/include/uapi
```

Check if the expected search directories of the missing header found in the
previous step is in these `-I` options.

Another way to determine is to use the `--debug_annotate_scripts` option.
Example:

```shell
$ tools/bazel build \
  //common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_sync \
  --debug_annotate_scripts > /tmp/out.log 2>&1
$ grep 'goldfish_sync.o' /tmp/out.log
[clang command]
[... other lines]
```

Examine the command and look for `-I` options, and compare it with the expected
search directories found in step 1.

### Step 2: Look for or define the appropriate target with the headers

See [instructions](#query-ddk-headers) to look for a `ddk_headers` target or
`filegroup` target under the package with the requested header, or look for
[`exports_files` declarations](https://bazel.build/reference/be/functions#exports_files)
manually.

If there's none, [define one](main.md#ddk_headers).

### Step 3: Add to `deps` of the `ddk_module` target

See instructions for [ddk_module](main.md#ddk_module).

## `<built-in>:1:10: fatal error: '<path>/include/linux/compiler-version.h' file not found` {#missing-compiler-version-h}

**NOTE**: This error is about `include/linux/compiler-version.h` or
`include/linux/kconfig.h`.

If you see the following error:

```text
<built-in>:1:10: fatal error: '<path>/include/linux/compiler-version.h' file not found
#include "<path>/include/linux/compiler-version.h"
         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1 error generated.
make[3]: *** [<path>/scripts/Makefile.build:286: <module>.o] Error 1
```

The reason is that a module implicitly includes this header (and
`include/linux/kconfig.h`) from the kernel source tree. This is usually listed
in `LINUXINCLUDE` in the `${KERNEL_DIR}`.

To resolve this:

1. Ensure that the `${KERNEL_DIR}` has a `ddk_headers` that exports these
   headers, or a `filegroup` target or
   an [`exports_files` declaration](https://bazel.build/reference/be/functions#exports_files)
   that exports these files.

    * If `${KERNEL_DIR}` points to the Android Common Kernel source tree, there
      should be a `ddk_headers` target named `all_headers`. There may be other
      smaller targets to use. Check the `BUILD.bazel` file under
      the `${KERNEL_DIR}` for the exact declarations.
    * Hint: you may also search all valid `ddk_headers` target with a query
      command; see [instructions](#query-ddk-headers).
    * If `${KERNEL_DIR}` points to a custom kernel source tree that does not
      track the Android Common Kernel source tree, use the `bazel query`
      command above to look for a suitable `ddk_headers` or `filegroup` target,
      or manually look
      for [`exports_files` declarations](https://bazel.build/reference/be/functions#exports_files)
      . If there's none, you can [define one](main.md#ddk_headers). Example:
      ```python
      ddk_headers(
          name = "linuxinclude",
          hdrs = [
              "include/linux/compiler-version.h",
              "include/linux/kconfig.h",
          ],
      )
      ```
2. Add the target found or defined in step 1 to the `deps` attribute of the
   `ddk_modules` target. For example, to add `"//common:all_headers"` to `deps`:
   ```python
   ddk_module(
       name = "foo",
       out = "foo.ko",
       deps = ["//common:all_headers"],
   )
   ```
   For details, see [ddk_module](main.md#ddk_module).

## Incorrect header is included {#incorrect-header}

Because include directories are searched from in a certain order, sometimes an
incorrect header `include/header.h` is included if multiple include directories
of a module `module.ko` includes `include/header.h`. If this is the case, you
may see errors like:

- `use of undeclared identifier`
- `no member named '<member>' in 'struct <struct>'`
- `incomplete definition of type 'struct <struct>'`
- ... and other weird errors unlisted here.

This is especially hard to debug if you focus on one of the `include/header.h`
that is not included, ignoring the one that is actually included with
incompatible definitions.

To resolve such error, the best practice is to **rename the headers**
to avoid confusion for both the build system and humans.

But if you would like to keep the names, go through
[generic steps for resolving missing header](#missing-headers), but focus on the
ordering of `-I` options in the `clang` command or in `ccflags-y`. The first
directory with `include/header.h` is the one used. You can confirm this by
putting an `#error` directive at the top of the suspicious `include/header.h`
and recompiling, for example.

Once the correct ordering is determined, order the dependencies of the
`ddk_modules` target to ensure that the correct `include/header.h` is used. Use
the `# do not sort` magic line to prevent buildifier from sorting the list.

For details about ordering of include directories, see
[documentations of all rules](../api_reference.md) and click `ddk_module` for
details.

## ERROR: modpost: "foo" [.../mod_using_foo.ko] undefined! {#modpost-symbol-undefined}

See [link](../errors.md#modpost-symbol-undefined) for help on resolving this
error for external modules in general.

## Error: kernel\_module() got multiple values for parameter 'outs' {#outs}

You might be using `outs` instead of `out`. For example:

```python
# Correct:
ddk_module(
    name = "foo",
    out = "foo.ko",
)

# Incorrect:
ddk_module(
    name = "foo",
    outs = ["foo.ko"], # WRONG! DO NOT USE
)
```

## Missing Abseil Python

If you encounter the following error:

```
ERROR: /path/to/WORKSPACE:17:23: fetching local_repository rule //external:io_abseil_py: java.io.IOException: The repository's path is "external/python/absl-py" (absolute: ...) but it does not exist or is not a directory.
```

It is because you did not check out the abseil project in your repo manifest.
The Python script that generates `Makefile` / `Kbuild` relies on abseil. Check
it out by adding an entry in your repo manifest, e.g.

```xml
<project path="external/python/absl-py" name="platform/external/python/absl-py" />
```

Then running `repo sync`.

See [change 2127786](http://r.android.com/2127786) for an example.

Refer to
[repo Manifest Format](https://gerrit.googlesource.com/git-repo/+/master/docs/manifest-format.md)
if you have multiple remotes.

## Appendix

### Query appropriate `ddk_headers` targets to use {#query-ddk-headers}

Example: The following query shows all `ddk_headers` targets in `//common` that
includes `common/include/linux/compiler-version.h`, and visible to the
target `//common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_sync`.

```shell
$ tools/bazel query \
  'visible(//common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_sync,
           kind(ddk_headers,
                rdeps(//common:*,
                      //common:include/linux/compiler-version.h)))'
```

See [Bazel query language](https://bazel.build/query/language).
