# Build faster

## TL;DR

For local developing workflow, build with `--config=fast`.

Example:

```shell
$ tools/bazel run --config=fast //common:kernel_aarch64 -- --dist_dir=out/dist
```

Or add to `user.bazelrc`:

```text
# user.bazelrc
build --config=fast
```

## How does this work?

This config implies:

- `--config=local`. See [sandbox.md](sandbox.md).
