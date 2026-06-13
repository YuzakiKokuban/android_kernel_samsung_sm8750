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

"""An `exec` that uses hermetic tools."""

load("//build/bazel_common_rules/exec/impl:exec.bzl", "exec", "exec_test")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")

visibility("//build/kernel/kleaf/...")

def _hermetic_exec_toolchain_run_setup_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    run_setup_sh = ctx.actions.declare_file("{}/run_setup.sh".format(ctx.label.name))
    ctx.actions.write(run_setup_sh, hermetic_tools.run_setup, is_executable = True)
    return DefaultInfo(files = depset([run_setup_sh]))

_hermetic_exec_toolchain_run_setup = rule(
    implementation = _hermetic_exec_toolchain_run_setup_impl,
    toolchains = [hermetic_toolchain.type],
)

def _hermetic_exec_toolchain_deps_impl(ctx):
    hermetic_tools = hermetic_toolchain.get(ctx)
    return DefaultInfo(files = hermetic_tools.deps)

_hermetic_exec_toolchain_deps = rule(
    implementation = _hermetic_exec_toolchain_deps_impl,
    toolchains = [hermetic_toolchain.type],
)

def hermetic_exec_target(
        rule,
        name,
        script,
        data = None,
        **kwargs):
    """Instantiates an exec / exec_test / rule from exec_rule as a target.

    Hermetic tools are resolved from toolchain resolution. To replace it,
    register a different hermetic toolchain.

    Args:
        rule: The `rule()`, e.g. `exec`.
        name: name of the target
        script: Pass to `rule`, e.g. see [exec.script]
        data: Pass to `rule`, e.g. see [exec.data]
        **kwargs: Pass to `rule`, e.g. see [exec]
    """

    # Not using a global target here because it is hard to be referred to
    # in pre_script below, especially when this macro is invoked in another
    # repository.
    _hermetic_exec_toolchain_run_setup(
        name = name + "_hermetic_exec_toolchain_run_setup",
    )

    _hermetic_exec_toolchain_deps(
        name = name + "_hermetic_exec_toolchain_deps",
    )

    if data == None:
        data = []

    # data may not be a list (it may be a select()), so use a explicit expr
    data = data + [
        name + "_hermetic_exec_toolchain_run_setup",
        name + "_hermetic_exec_toolchain_deps",
    ]

    pre_script = """
        . $(rootpath {name}_hermetic_exec_toolchain_run_setup)
    """.format(name = name)

    rule(
        name = name,
        data = data,
        script = pre_script + script,
        **kwargs
    )

def hermetic_exec(
        name,
        script,
        data = None,
        **kwargs):
    """A exec that uses hermetic tools.

    Hermetic tools are resolved from toolchain resolution. To replace it,
    register a different hermetic toolchain.

    Args:
        name: name of the target
        script: See [exec.script]
        data: See [exec.data]
        **kwargs: See [exec]
    """
    hermetic_exec_target(
        rule = exec,
        name = name,
        script = script,
        data = data,
        **kwargs
    )

def hermetic_exec_test(
        name,
        script,
        data = None,
        **kwargs):
    """A exec_test that uses hermetic tools.

    Hermetic tools are resolved from toolchain resolution. To replace it,
    register a different hermetic toolchain.

    Args:
        name: name of the target
        script: See [exec_test.script]
        data: See [exec_test.data]
        **kwargs: See [exec_test]
    """
    hermetic_exec_target(
        rule = exec_test,
        name = name,
        script = script,
        data = data,
        **kwargs
    )
