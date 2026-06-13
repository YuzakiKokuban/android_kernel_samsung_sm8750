# Bzlmod support for Kleaf

## Migrate to bzlmod

### Use @kleaf as dependent module (recommended)

If you are setting up a new workspace, it is recommended
to use `@kleaf` as a dependent module. See [Setting up DDK workspace](ddk/workspace.md).

### Use @kleaf as root module (legacy)

If you are migrating from non-Bzlmod, `WORKSPACE`-style
setup, this may be the easier option because it resembles the directory
structure of `WORKSPACE`-style setup.

Set up your repo manifest to conform with the following filesystem layout.

```text
<workspace_root>/
    |- WORKSPACE               -> build/kernel/kleaf/bazel.WORKSPACE # Note 1
    |- WORKSPACE.bzlmod        -> build/kernel/kleaf/bzlmod/bazel.WORKSPACE.bzlmod # Note 1
    |- MODULE.bazel            -> build/kernel/kleaf/bzlmod/bazel.MODULE.bazel
    |- build/
    |    `- kernel/
    |- common/
    |    `- build.config.constants              # Note 2
    `- external/
         |- bazelbuild-bazel-central-registry
         `- <other external repositories>       # Note 3
```

**Note 1**: The root `WORKSPACE` and `WORKSPACE.bzlmod` files are present to
support switching between bzlmod and non-bzlmod builds. During migration to
bzlmod, you may have an non-empty `WORKSPACE.bzlmod` file for dependencies
that has not been migrated to bzlmod. After all dependencies and the
root module migrated to Bzlmod, both files may be removed.

See
[hybrid mode for gradual migration](https://bazel.build/external/migration#hybrid-mode)
for details.

**Note 2**: If `build.config.constants` exists elsewhere other than `common/`,
create the symlink `common/build.config.constants` to the file. This may be
done with `<linkfile>` in your repo manifest.

**Note 3**: A list of external repositories are required for bzlmod to work.
For the up-to-date list, refer to the repo manifest of the correspoding ACK
branch.

See example manifests for
[Pixel 6 and Pixel 6 Pro](https://android.googlesource.com/kernel/manifest/+/refs/heads/gs-android-gs-raviole-mainline/default.xml)
and for
[Android Common Kernel and Cloud Android Kernel](https://android.googlesource.com/kernel/manifest/+/refs/heads/common-android-mainline/default.xml).

## Versions of dependent modules

### Cheatsheet

```text
bazel_dep version <= actual version in external/
```

### bazel\_dep version

This refers to the version of a given module declared in `bazel_dep` in
[MODULE.bazel](../bzlmod/bazel.MODULE.bazel).

This is the version that `@kleaf` expects from the dependent module. For
example, if `@kleaf` uses feature A from `rules_cc@1.5`, then the `bazel_dep`
declaration should have at least `rules_cc@1.5`.

In practice, the above version may get outdated. Because `MODULE.bazel`
declares `local_path_override`, the versions in `bazel_dep` are ignored, and
Kleaf will continue to work with the actual version in `external/` as
guaranteed by continuous testing. This may become a problem in the future
when Kleaf is used as a dependency module and the root module does not use the
same `local_path_override`.

### actual version in external/

For a given module, this is the version declared in
`external/<module_name>/MODULE.bazel`, with a few exceptions.

This is the actual version of the dependency. But at build time, Bazel does not
care about the actual version. Because of
[backwards compatibility guarantees](https://bazel.build/external/module#compatibility_level)
when compatibility level is the same, it is okay to use a new version as an
old version.

Usually, the following also holds true:

```text
bazel_dep version == actual version in external/
```

However, if the external Git repository is updated indenpendently, there may
be a period of time where `bazel_dep version < actual version in external/`,
until `MODULE.bazel` is updated.

## See also

[Bazel modules](https://bazel.build/external/module)
