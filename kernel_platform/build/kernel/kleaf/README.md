# Kleaf - Building Android Kernels with Bazel

## Table of contents

### Getting started

[Introduction to Kleaf](docs/kleaf.md)

[Building your kernels and drivers with Bazel](docs/impl.md)

[Creating distributions](docs/dist.md)

[Driver Development Kit (DDK)](docs/ddk/main.md)

[Cheatsheet](docs/cheatsheet.md)

### Configuring your build

[`build.sh` build configs](docs/build_configs.md)

[Configuring kernel\_build](docs/kernel_config.md)

### ABI monitoring

[Support ABI monitoring (GKI)](docs/abi.md)

[Support ABI monitoring (Device)](docs/abi_device.md)

### Debugging and testing

[Resolving common errors](docs/errors.md)

[Kleaf testing](docs/testing.md)

[Debugging Kleaf](docs/debugging.md)

[Visualizing dependencies](docs/dependencies.md)

### Advanced topics

[Handling SCM version](docs/scmversion.md)

[Building against downloaded prebuilts](docs/download_prebuilt.md)

[Customize workspace](docs/workspace.md)

[Bzlmod support for Kleaf](docs/bzlmod.md)

[Building `compile_commands.json`](docs/compile_commands.md)

[Ensuring hermeticity](docs/hermeticity.md)

[Internet Access](docs/network.md)

[Toolchain resolution](docs/toolchains.md)

[Checkpatch](docs/checkpatch.md)

[Kleaf Development](docs/kleaf_development.md)

[Discover newest Kleaf features in Canary](docs/canary.md)

### Configurations in command line

`--config=fast`: [Make local builds faster](docs/fast.md)

`--config=local`: [Sandboxing](docs/sandbox.md)

`--config=release`: [Release builds](docs/release.md)

`--config=stamp`: [Handling SCM version](docs/scmversion.md)

`--config=canary`: [Kleaf Canary Features](docs/canary.md)

### Flags

For a full list of flags, run

```sh
$ tools/bazel help kleaf
```

`--gcov`: [Keep GCOV files](docs/gcov.md)

`--kasan`: [kasan](docs/kasan.md)

`--kbuild_symtypes`: [KBUILD\_SYMTYPES](docs/symtypes.md)

`--kgdb`: [GDB scripts](docs/kgdb.md)

`--lto`: [Configure LTO during development](docs/lto.md)

`--notrim`: Disables `TRIM_NONLISTED_KMI` globally.

`--btf_debug_info`: [Enable/disable BTF debug information](docs/btf.md)

`--gki_build_config_fragment`:
[Supporting GKI\_BUILD\_CONFIG\_FRAGMENT on Kleaf](docs/gki_build_config_fragment.md)

`--defconfig_fragment`: [Defconfig fragments](docs/kernel_config.md#defconfig-fragments)

Other flags for debugging and disabling integrity checks may be found in the
[Debugging Kleaf](docs/debugging.md) page.

### References

[References to Bazel rules and macros for the Android Kernel](docs/api_reference.md)
