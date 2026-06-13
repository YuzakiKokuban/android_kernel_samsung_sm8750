# Checkpatch

## Run checkpatch for ACK repository

To run checkpatch for the common kernel Git repository
(assuming that the repository is checked out at `common/`):

```sh
$ tools/bazel run //common:checkpatch
```

Like any executable targets, flags to the script itself should be placed
after `--`:

```sh
$ tools/bazel run //common:checkpatch [-- [flags]]
```

Run the following to get an updated list of flags accepted by the script:

```sh
$ tools/bazel run //common:checkpatch -- --help
```

## Declare and run checkpatch for additional Git repositories

To support checkpatch for other Git repositories (e.g. the
Git repositories for external modules), define the following target
at `<root of git repository>/BUILD.bazel` (i.e. right next to `.git`):

```python
# path/to/git/repository/BUILD.bazel

load("//build/kernel/kleaf:kernel.bzl", "checkpatch")

checkpatch(
    name = "checkpatch",
    checkpatch_pl = "//common:scripts/checkpatch.pl",
)
```

After the target is declared, you may run the following
to run checkpatch on `path/to/git/repository`:

```sh
$ tools/bazel run //path/to/git/repository:checkpatch
```

## Presubmit on ci.android.com

Presubmit checks on [ci.android.com](http://ci.android.com) requires
exactly one `checkpatch()` target to be declared for each Git repository that
is eligible for checkpatch.

If you are migrating from
`build/kernel/static_analysis/checkpatch_presubmit.sh`, a `checkpatch` target
must be declared for each directory listed in `EXT_MODULES`.

## Advanced usage

### Run all declared checkpatch targets in the workspace

You may use [Bazel query](https://bazel.build/query/guide) to find all declared
checkpatch targets in the workspace, and run all of them. Example script:

```sh
(
    tools/bazel query '
        kind("^checkpatch rule$", //...:all) except //out/...
    ' -k 2>/dev/null || true
) | \
while read -r target ; do
    tools/bazel run "$target"
done
```

### Run checkpatch on a different Git repository without declaring `checkpatch()`

**NOTE**: This is not recommended because it does not declare that the
given Git repository is eligible for checkpatch. This should only be used
for debugging to validate an arbitrary Git repository.

For example:

```sh
$ tools/bazel run //common:checkpatch -- --dir another/git/repository
```
