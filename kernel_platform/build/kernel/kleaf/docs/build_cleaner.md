# Kleaf `build_cleaner`

**NOTE**: Kleaf `build_cleaner` is experimental.

Kleaf `build_cleaner` is a tool to clean up the `BUILD.bazel` files to fix
dependencies.

Invoke `build_cleaner` with the following command:

```shell
$ build/kernel/kleaf/build_cleaner.py <label_to_dist_target>
```

Currently, Kleaf `build_cleaner` has a limited scope of applications. In
particular, the tool works best if all targets are specified directly
in `BUILD.bazel` files. For more and up-to-date information about what is
supported, run the following:

```shell
$ build/kernel/kleaf/build_cleaner.py -h
```

Or inspect its [source code](../build_cleaner.py).
