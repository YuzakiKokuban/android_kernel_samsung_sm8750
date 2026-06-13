# Testing Kleaf

Some basic tests can be performed with `bazel test` command. For details of
Bazel testing, please visit

[https://bazel.build/rules/testing](https://bazel.build/rules/testing)

## `kernel_build`

### GKI

For the GKI `kernel_build()` `kernel_aarch64`, the following targets are
created.

```shell
$ bazel test kernel_aarch64_test
```

This command checks the following on the GKI binary:

- scmversion

```shell
$ bazel test kernel_aarch64_module_test
```

This command checks the following on the in-tree GKI modules:

- scmversion
- vermagic

### Device kernel

For your `kernel_build()` named `tuna`, the following targets are
created.

```shell
$ bazel test tuna_test
```

This command checks the following on the kernel binary, if it exists:

- scmversion

```shell
$ bazel test tuna_module_test
```

This command checks the following on the in-tree modules:

- scmversion
- vermagic

## External `kernel_module`

For a `kernel_module()` named `nfc`, the following targets are created.

```shell
$ bazel test nfc_test
```

This command checks the following on the external module:

- scmversion
- vermagic
