# Kleaf Cheatsheet

## Building

### Just vmlinux and in-tree modules etc. ( = `make`)

```shell
$ tools/bazel build //common:kernel_aarch64
```

### All GKI artifacts for distribution

```shell
$ tools/bazel run //common:kernel_aarch64_dist
```

### Keep intermediate build artifacts (for example `*.o` files) in `out/cache/`

```shell
$ tools/bazel build --config=local //common:kernel_aarch64
```

## ABI monitoring

### Building all artifacts

```shell
$ tools/bazel run //common:kernel_aarch64_abi_dist
```

### Update symbol list

```shell
$ tools/bazel run //common:kernel_aarch64_abi_update_symbol_list
```

### Update ABI definition

```shell
$ tools/bazel run //common:kernel_aarch64_abi_update
```

## LTO

```text
--lto={none,thin,default,full}
```

```shell
$ bazel run --lto=none //common:kernel_aarch64_dist
```
