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

"""A genrule that uses hermetic tools."""

load("//build/kernel/kleaf/impl:common_providers.bzl", "KernelEnvToolchainsInfo")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")

visibility("//build/kernel/kleaf/...")

def _attrs():
    return {
        "_toolchains": attr.label(
            default = "//build/kernel/kleaf/impl:kernel_toolchains",
            providers = [KernelEnvToolchainsInfo],
            cfg = "exec",
        ),
        "use_cc_toolchain": attr.bool(
            doc = "When set to `True` resolved CC toolchain is accessible from the genrule.",
        ),
    }

def _get_toolchains(ctx):
    return ctx.attr._toolchains[KernelEnvToolchainsInfo]

def _hermetic_genrule_toolchain_setup_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    setup_sh = ctx.actions.declare_file("{}/setup.sh".format(ctx.label.name))
    setup_cmd = hermetic_tools.setup

    if ctx.attr.use_cc_toolchain:
        toolchains = _get_toolchains(ctx)
        setup_cmd += toolchains.setup_env_var_cmd

    ctx.actions.write(
        setup_sh,
        setup_cmd,
        is_executable = True,
    )
    return DefaultInfo(files = depset([setup_sh]))

_hermetic_genrule_toolchain_setup = rule(
    implementation = _hermetic_genrule_toolchain_setup_impl,
    toolchains = [hermetic_toolchain.type],
    attrs = _attrs(),
)

def _hermetic_genrule_toolchain_deps_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    if ctx.attr.use_cc_toolchain:
        toolchains = _get_toolchains(ctx)
        return DefaultInfo(files = depset(
            hermetic_tools.deps.to_list() + toolchains.all_files.to_list(),
        ))
    else:
        return DefaultInfo(files = hermetic_tools.deps)

_hermetic_genrule_toolchain_deps = rule(
    implementation = _hermetic_genrule_toolchain_deps_impl,
    toolchains = [hermetic_toolchain.type],
    attrs = _attrs(),
)

def hermetic_genrule(
        name,
        cmd,
        tools = None,
        use_cc_toolchain = None,
        **kwargs):
    """A genrule that uses hermetic tools.

    Hermetic tools are resolved from toolchain resolution. To replace it,
    register a different hermetic toolchain.

    Only `cmd` is expected and used. `cmd_bash`, `cmd_ps`, `cmd_bat` etc. are
    ignored.

    Args:
        name: name of the target
        cmd: See [genrule.cmd](https://bazel.build/reference/be/general#genrule.cmd)
        tools: See [genrule.tools](https://bazel.build/reference/be/general#genrule.tools)
        use_cc_toolchain: When set to `True` resolved CC toolchain is accessible from the genrule.
        **kwargs: See [genrule](https://bazel.build/reference/be/general#genrule)
    """

    # Not using a global target here because it is hard to be referred to
    # in pre_cmd below, especially when this macro is invoked in another
    # repository.
    _hermetic_genrule_toolchain_setup(
        name = name + "_hermetic_genrule_toolchain_setup",
        use_cc_toolchain = use_cc_toolchain,
    )

    _hermetic_genrule_toolchain_deps(
        name = name + "_hermetic_genrule_toolchain_deps",
        use_cc_toolchain = use_cc_toolchain,
    )

    if tools == None:
        tools = []

    # tools may not be a list (it may be a select()), so use a explicit expr
    tools = tools + [
        name + "_hermetic_genrule_toolchain_setup",
        name + "_hermetic_genrule_toolchain_deps",
    ]

    pre_cmd = """
        . $(execpath {name}_hermetic_genrule_toolchain_setup)
    """.format(name = name)

    native.genrule(
        name = name,
        tools = tools,
        cmd = pre_cmd + cmd,
        **kwargs
    )
