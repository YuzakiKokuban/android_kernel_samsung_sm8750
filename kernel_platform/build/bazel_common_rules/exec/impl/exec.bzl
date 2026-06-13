# Copyright (C) 2022 The Android Open Source Project
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

"""Impl of `exec`."""

load(":exec_aspect.bzl", "ExecAspectInfo", "exec_aspect")

visibility([
    "//build/bazel_common_rules/exec/...",
    "//build/kernel/kleaf/...",
])

_DEFAULT_HASHBANG = "/bin/bash -e"

def _impl(ctx):
    out_file = ctx.actions.declare_file(ctx.label.name)

    for target in ctx.attr.data:
        if ExecAspectInfo not in target:
            continue
        if target[ExecAspectInfo].args:
            fail("{}: {} must not have args. Use embedded_exec to wrap it.".format(ctx.label, target.label))
        if target[ExecAspectInfo].env:
            fail("{}: {} must not have env. Use embedded_exec to wrap it.".format(ctx.label, target.label))

    content = "#!{}\n".format(ctx.attr.hashbang)
    content += ctx.attr.script

    content = ctx.expand_location(content, ctx.attr.data)
    ctx.actions.write(out_file, content, is_executable = True)

    runfiles = ctx.runfiles(files = ctx.files.data + [out_file])
    runfiles = runfiles.merge_all([target[DefaultInfo].default_runfiles for target in ctx.attr.data])

    return DefaultInfo(
        files = depset([out_file]),
        executable = out_file,
        runfiles = runfiles,
    )

exec = rule(
    implementation = _impl,
    doc = """Run a script when `bazel run` this target.

See [documentation] for the `args` attribute.

**NOTE**: Like [genrule](https://bazel.build/reference/be/general#genrule)s,
hermeticity is not enforced or guaranteed, especially if `script` accesses PATH.
See [`Genrule Environment`](https://bazel.build/reference/be/general#genrule-environment)
for details.
""",
    attrs = {
        "data": attr.label_list(aspects = [exec_aspect], allow_files = True, doc = """A list of labels providing runfiles. Labels may be used in `script`.

Executables in `data` must not have the `args` and `env` attribute. Use
[`embedded_exec`](#embedded_exec) to wrap the depended target so its env and args
are preserved.
"""),
        "hashbang": attr.string(default = _DEFAULT_HASHBANG, doc = "Hashbang of the script."),
        "script": attr.string(doc = """The script.

Use `$(rootpath <label>)` to refer to the path of a target specified in `data`. See
[documentation](https://bazel.build/reference/be/make-variables#predefined_label_variables).

Use `$@` to refer to the args attribute of this target.

See `build/bazel_common_rules/exec/tests/BUILD` for examples.
"""),
    },
    executable = True,
)

exec_test = rule(
    implementation = _impl,
    doc = """Run a test script when `bazel test` this target.

See [documentation] for the `args` attribute.

**NOTE**: Like [genrule](https://bazel.build/reference/be/general#genrule)s,
hermeticity is not enforced or guaranteed, especially if `script` accesses PATH.
See [`Genrule Environment`](https://bazel.build/reference/be/general#genrule-environment)
for details.
""",
    attrs = {
        "data": attr.label_list(aspects = [exec_aspect], allow_files = True, doc = """A list of labels providing runfiles. Labels may be used in `script`.

Executables in `data` must not have the `args` and `env` attribute. Use
[`embedded_exec`](#embedded_exec) to wrap the depended target so its env and args
are preserved.
"""),
        "hashbang": attr.string(default = _DEFAULT_HASHBANG, doc = "Hashbang of the script."),
        "script": attr.string(doc = """The script.

Use `$(rootpath <label>)` to refer to the path of a target specified in `data`. See
[documentation](https://bazel.build/reference/be/make-variables#predefined_label_variables).

Use `$@` to refer to the args attribute of this target.

See `build/bazel_common_rules/exec/tests/BUILD` for examples.
"""),
    },
    test = True,
)

def exec_rule(
        cfg = None,
        attrs = None):
    """Returns a rule() that is similar to `exec`, but with the given incoming transition.

    **NOTE**: Like [genrule](https://bazel.build/reference/be/general#genrule)s,
    hermeticity is not enforced or guaranteed for targets of the returned
    rule, especially if a target specifies `script` that accesses PATH.
    See [`Genrule Environment`](https://bazel.build/reference/be/general#genrule-environment)
    for details.

    Args:
        cfg: [Incoming edge transition](https://bazel.build/extending/config#incoming-edge-transitions)
            on the rule
        attrs: Additional attributes to be added to the rule.

            Specify `_allowlist_function_transition` if you need a transition.
    Returns:
        a rule
    """

    fixed_attrs = {
        "data": attr.label_list(aspects = [exec_aspect], allow_files = True),
        "hashbang": attr.string(default = _DEFAULT_HASHBANG),
        "script": attr.string(),
    }

    if attrs == None:
        attrs = {}
    attrs = attrs | fixed_attrs

    return rule(
        implementation = _impl,
        attrs = attrs,
        cfg = cfg,
        executable = True,
    )
