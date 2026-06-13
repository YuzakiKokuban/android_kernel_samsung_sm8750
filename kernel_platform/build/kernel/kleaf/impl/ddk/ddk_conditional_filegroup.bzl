# Copyright (C) 2023 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Sources conditional to a [`ddk_module`](#ddk_module)."""

load(":utils.bzl", "utils")

visibility("//build/kernel/kleaf/...")

_DDK_CONDITIONAL_TRUE = "__kleaf_ddk_conditional_srcs_true_value__"

DdkConditionalFilegroupInfo = provider(
    "Provides attributes for [`ddk_conditional_filegroup`](#ddk_conditional_filegroup)",
    fields = {
        "config": "`ddk_conditional_filegroup.config`",
        "value": """bool or str. `ddk_conditional_filegroup.value`

This may be a special value `True` when it is set to `True` in `ddk_module`.
        """,
    },
)

def _ddk_conditional_filegroup_impl(ctx):
    value = ctx.attr.value
    if value == _DDK_CONDITIONAL_TRUE:
        value = True

    return [
        DefaultInfo(files = depset(transitive = [target.files for target in ctx.attr.srcs])),
        DdkConditionalFilegroupInfo(
            config = ctx.attr.config,
            value = value,
        ),
    ]

_ddk_conditional_filegroup = rule(
    implementation = _ddk_conditional_filegroup_impl,
    doc = """A target that declares sources conditionally included based on configs.

Example (Pseudocode):

```
_ddk_conditional_filegroup(
    name = "srcs_when_foo_is_set",
    config = "CONFIG_FOO",
    value = "y",
    srcs = ["foo_is_set.c"]
)

ddk_module(
    name = "mymodule",
    srcs = [
        ":srcs_when_foo_is_set",
    ],
    ...
)
```

In the above example, `foo_is_set.c` is only included in `mymodule.ko`
if `CONFIG_FOO=y`:

```
ifeq ($(CONFIG_FOO),y)
mymodule-y += foo_is_set.c
endif
```

A special value `_DDK_CONDITIONAL_TRUE` means `y` or `m`. Example:

```
_ddk_conditional_filegroup(
    name = "srcs_when_foo_is_set",
    config = "CONFIG_FOO",
    value = _DDK_CONDITIONAL_TRUE,
    srcs = ["foo_is_set.c"]
)

ddk_module(
    name = "mymodule",
    srcs = [
        ":srcs_when_foo_is_set",
    ],
    ...
)
```

This generates:

```
mymodule-$(CONFIG_FOO) += foo_is_set.c
```

Note that during the analysis phase, `foo_is_set.c` is always an input
to `mymodule`, so any change to `foo_is_set.c` will trigger a rebuild
on `mymodule` regardless of the value of `CONFIG_FOO`. The conditional
is only examined in Kbuild.
    """,
    attrs = {
        "config": attr.string(
            mandatory = True,
            doc = "Name of the config with the `CONFIG_` prefix.",
        ),
        "value": attr.string(
            mandatory = True,
            doc = """Expected value of the config.

If and only if the config matches this value, `srcs` are included.

This should be set to `_DDK_CONDITIONAL_TRUE` when `True` is in
`ddk_modules.conditional_srcs`.
""",
        ),
        "srcs": attr.label_list(
            allow_files = [".c", ".h", ".S", ".rs"],
            doc = "See [`ddk_module.srcs`](#ddk_module-srcs).",
        ),
    },
)

def ddk_conditional_filegroup(
        name,
        config,
        value,
        srcs = None,
        **kwargs):
    """Wrapper macro of _ddk_conditional_filegroup.

    Args:
        name: name of target
        config: Name of the config with the `CONFIG_` prefix.
        value: bool or str. Expected value of the config.

          This value may be:

          - `True`: becomes `obj-$(CONFIG_FOO) += xxx`
          - `False`: maps empty string; becomes `ifeq ($(CONFIG_FOO),)`
          - a string: becomes `ifeq ($(CONFIG_FOO),the_expected_value)`

        srcs: See [`ddk_module.srcs`](#ddk_module-srcs).
        **kwargs: kwargs
    """
    if value == True:
        value = _DDK_CONDITIONAL_TRUE
    elif value == False:
        value = ""

    _ddk_conditional_filegroup(
        name = name,
        config = config,
        value = value,
        srcs = srcs,
        **kwargs
    )

# buildifier: disable=unnamed-macro
def flatten_conditional_srcs(
        module_name,
        conditional_srcs,
        **kwargs):
    """Helper to flatten `conditional_srcs`.

    Args:
        module_name: name of `ddk_module` or `ddk_submodule`
        conditional_srcs: conditional sources
        **kwargs: additional kwargs to internal rules

    Returns:
        A list of targets to be palced in srcs
    """

    if not conditional_srcs:
        return []
    flattened_conditional_srcs = []
    for config, config_srcs_dict in conditional_srcs.items():
        for config_value, config_srcs in config_srcs_dict.items():
            if type(config_value) != "bool":
                fail("{label}: expected value of config {config} must be a bool, but got {config_value} of type {value_type}".format(
                    label = native.package_relative_label(module_name),
                    config_value = config_value,
                    config = config,
                    value_type = type(config_value),
                ))
            fg_name = "{name}_{config}_{value}_srcs".format(
                name = module_name,
                config = config,
                value = utils.normalize(str(config_value)),
            )
            ddk_conditional_filegroup(
                name = fg_name,
                config = config,
                value = config_value,
                srcs = config_srcs,
                **kwargs
            )
            flattened_conditional_srcs.append(fg_name)
    return flattened_conditional_srcs
