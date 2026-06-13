# BTF debug information

Option `--btf_debug_info` can enable or disable the generation of BTF debug
information:

* `default` - use kernel config value for CONFIG_DEBUG_INFO_BTF.
* `enable` - enable generation of BTF debug information.
* `disable` - disable generation of BTF debug information.

While this information is useful for debugging and loading BPF programs, it
requires significant time to be generated. Currently, there is no runtime
dependency on BTF debug information and for a faster local build one can try
`--btf_debug_info=disable` in addition to `--config=fast`. But there is no
guarantee that future kernels will work properly without CONFIG_DEBUG_INFO_BTF.
