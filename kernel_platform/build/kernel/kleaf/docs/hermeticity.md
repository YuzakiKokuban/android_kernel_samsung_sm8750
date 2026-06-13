# Ensuring Hermeticity

Hermetic builds are a key goal of Kleaf. Hermeticity means that all tools,
toolchain, inputs, etc. come from the source tree, not outside of the source
tree from the host machine.

All rules provided by Kleaf are as hermetic as possible (see
[Known violations](#known-violations)). However, this does not guarantee
hermeticity unless you also set up the build targets properly.

Below are some tips to ensure hermeticity for your builds.

## Use hermetic\_genrule

The command of the native
[`genrule`](https://bazel.build/reference/be/general#genrule)
can access the passthrough `PATH`, allowing the `genrule`
to use any tools from the host machine. See
[Genrule Environment](https://bazel.build/reference/be/general#genrule)
for details.

Kleaf provides the `hermetic_genrule` via
`//build/kernel:hermetic_tools.bzl` as a drop-in replacement for `genrule`.
The `hermetic_genrule` sets PATH to the registered hermetic toolchain.

Avoid using absolute paths (e.g. `/bin/ls`) in your `genrule`s or
`hermetic_genrule`s, since this will use tools and resources from your host
machine.

Example:

```python
load("//build/kernel/kleaf:hermetic_tools.bzl", "hermetic_genrule")
hermetic_genrule(
    name = "generated_source",
    srcs = ["in.template"],
    outs = ["generated.c"],
    # cat and grep is from hermetic toolchain
    script = "cat $(location in.template) | grep x y > $@",
)
```

To make the change more transparent, you may use an alias in the `load`
statement:

```python
load("//build/kernel/kleaf:hermetic_tools.bzl", genrule = "hermetic_genrule")
genrule(
    name = "generated_source",
    ...
)
```

### Accessing CC toolchain

Setting `use_cc_toolchain` to `True` in `hermetic_genrule` makes C/C++ tools
and binaries available. For example:

```python
hermetic_genrule(
    name = "readelf_version",
    outs = ["version.txt"],
    # llvm-readelf comes from the resolved CC toolchain.
    cmd = "llvm-readelf --version > $@",
    use_cc_toolchain = True,
)
```

**NOTE**: This is recommended for very simple use cases, for complex ones,
prefer to use [custom rules](#custom-rules).

## Use hermetic\_exec and hermetic\_exec\_test

Similarly, the`script` of `exec` and `exec_test` from
`//build/bazel_common_rules:exec.bzl`
can access the passthrough `PATH`, hence they are not hermetic either.

Kleaf provides the `hermetic_exec` and `hermetic_exec_test` via
`//build/kernel:hermetic_tools.bzl` as drop-in replacements for `exec`
and `exec_test`, respectively.

Avoid using absolute paths (e.g. `/bin/ls`) in your `exec`s, `exec_test`s,
`hermetic_exec`s, or `hermetic_exec_test`s, since this will use tools and
resources from your host machine.

## sh\_* rules

If you use `sh_binary`, `sh_library`, `sh_test` etc. from Bazel, the shell
executable is defined by the shebangs (e.g. `#!/bin/bash`).

There are several other dependencies on `/bin/bash` and `/bin/sh` (see
[Known violations](#known-violations)). Besides them, avoid using other
shell executables in `sh_*` rules.

## Custom rules

If you have custom `rule()`s, make sure to use the hermetic toolchain.

- Add `hermetic_toolchain.type` to `toolchains` of `rule()`.
- Add `hermetic_tools = hermetic_toolchain.get(ctx)` to your rule
    implementation. `hermetic_tools` is a struct with two fields:
    `setup` and `deps`.
- Ensure the following to `ctx.actions.run_shell`:
    - The `command` should start with `hermetic_tools.setup`
    - The `tools` should include the depset `hermetic_tools.deps`.
        If there are other `tools`, chain the `depset`s using the
        [`transitive`](https://bazel.build/rules/lib/globals/bzl.html#depset)
        argument.

If you are using `ctx.actions.run`, usually there are no actions needed, since
Bazel will execute that binary directly without instantiating a shell
environment.

Example:

```python
load("//build/kernel:hermetic_tools.bzl", "hermetic_toolchain")

def _rename_impl(ctx):
    dst = ctx.actions.declare_file("{}/{}".format(ctx.attr.name, ctx.attr.dst))

    # Retrieve the toolchain
    hermetic_tools = hermetic_toolchain.get(ctx)

    # Set up environment (PATH)
    command = hermetic_tools.setup

    command += """
        cp -L {src} {dst}
    """.format(
        src = ctx.file.src.path,
        dst = dst.path,
    )
    ctx.actions.run_shell(
        inputs = [ctx.file.src],
        outputs = [dst],
        # Add hermetic tools to the dependencies of the action.
        tools = hermetic_tools.deps,
        command = command,
    )
    return DefaultInfo(files = depset([dst]))

rename = rule(
    implementation = _rename_impl,
    attrs = {
        "src": attr.label(allow_single_file = True),
        "dst": attr.string(),
    },
    # Declare the list of toolchains that the rule uses.
    toolchains = [
        hermetic_toolchain.type,
    ],
)
```

### Accessing CC toolchain

Accessing the CC tools is possible for custom rules too. The following shows an
example on how to access the `strip` tool from the resolved toolchain.

```python
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")
load("@rules_cc//cc:action_names.bzl", "ACTION_NAMES")
load("//build/kernel/kleaf:hermetic_tools.bzl", "hermetic_toolchain")

def _strip_version_impl(ctx):
    version = ctx.actions.declare_file("{}/strip_version.txt".format(ctx.attr.name))

    # Retrieve the tools toolchain.
    hermetic_tools = hermetic_toolchain.get(ctx)

    # Set up environment (PATH)
    command = hermetic_tools.setup

    # Retrieve default resolved CC toolchain.
    cc_toolchain = find_cpp_toolchain(ctx, mandatory = False)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
    )

    # Customize this according to the tool needed.
    strip_path = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = ACTION_NAMES.strip,
    )
    command += """
        {strip} --version > {version}
    """.format(
        version = version.path,
        strip = strip_path,
    )
    ctx.actions.run_shell(
        tools = [cc_toolchain.all_files, hermetic_tools.deps],
        outputs = [version],
        command = command,
    )
    return DefaultInfo(files = depset([version]))

strip_version = rule(
    implementation = _strip_version_impl,
    attrs = {
        "_cc_toolchain": attr.label(default = "//build/kernel/kleaf/impl:kernel_toolchains"),
    },
    toolchains = [hermetic_toolchain.type] + use_cpp_toolchain(mandatory = False),
    fragments = ["cpp"],
)
```
For more action names mapping to tool names, refer to
`prebuilts/clang/host/linux-x86/kleaf/common.bzl`.

## Known violations

The hermetic toolchain provided by `//build/kernel:hermetic-tools`
still uses a few binaries from the host machine. For the up-to-date list,
see `host_tools` of the target. In particular, `bash` and `sh` are in the list
at the time of this writing.

For bootstraping, some scripts still uses `/bin/bash`. This
includes:

* `tools/bazel` that points to `build/kernel/kleaf/bazel.sh`
* `build/kernel/kleaf/workspace_status.sh`, which uses `git` from the
  host machine.
  * The script may also use `printf` etc. from the host machine if
    `--nokleaf_localversion`. See `scripts/setlocalversion`.

All `ctx.actions.run_shell` uses a shell defined by Bazel, which is usually
`/bin/bash`.

When configuring a kernel via `tools/bazel run //path/to:foo_config`, the
script is not hermetic in order to use `ncurses` from the host machine
for `menuconfig`.

When running a `checkpatch()` target, the execution is not fully hermetic
in order to use `git` from the host machine.

The kernel build may also read from absolute paths outside of the source tree,
e.g. to draw randomness from `/dev/urandom` to create key pairs for signing.

Updating the ABI definition uses the host executables in order to use `git`.

If `--workaround_btrfs_b292212788` is set, `find` comes from the host machine.
[See internal bug b/292212788](http://b/292212788), or
[Bug 217681 - gen_kheaders.sh gets stuck in an infinite loop](https://bugzilla.kernel.org/show_bug.cgi?id=217681)
