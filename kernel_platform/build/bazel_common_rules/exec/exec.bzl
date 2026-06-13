# Copyright (C) 2024 The Android Open Source Project
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

"""Helps embedding `args` of an executable target."""

load(
    "//build/bazel_common_rules/exec/impl:exec.bzl",
    _exec = "exec",
    _exec_rule = "exec_rule",
    _exec_test = "exec_test",
)

visibility("public")

def exec(
        name,
        data = None,
        hashbang = None,
        script = None,
        **kwargs):
    """Runs a script when `bazel run` this target.

    See [documentation] for the `args` attribute.

    **NOTE**: Like [genrule](https://bazel.build/reference/be/general#genrule)s,
    hermeticity is not enforced or guaranteed, especially if `script` accesses PATH.
    See [`Genrule Environment`](https://bazel.build/reference/be/general#genrule-environment)
    for details.

    Args:
        name: name of the target
        data: A list of labels providing runfiles. Labels may be used in `script`.

            Executables in `data` must not have the `args` and `env` attribute. Use
            [`embedded_exec`](#embedded_exec) to wrap the depended target so its env and args
            are preserved.
        hashbang: hashbang of the script, default is `"/bin/bash -e"`.
        script: The script.

            Use `$(rootpath <label>)` to refer to the path of a target specified in `data`. See
            [documentation](https://bazel.build/reference/be/make-variables#predefined_label_variables).

            Use `$@` to refer to the args attribute of this target.

            See `build/bazel_common_rules/exec/tests/BUILD` for examples.
        **kwargs: Additional attributes to the internal rule, e.g.
            [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
            See complete list
            [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).

    Deprecated:
        Use `hermetic_exec` for stronger hermeticity.
    """

    # buildifier: disable=print
    print("WARNING: {}: exec is deprecated. Use `hermetic_exec` instead.".format(
        native.package_relative_label(name),
    ))

    kwargs.setdefault("deprecation", "Use hermetic_exec for stronger hermeticity")

    _exec(
        name = name,
        data = data,
        hashbang = hashbang,
        script = script,
        **kwargs
    )

def exec_test(
        name,
        data = None,
        hashbang = None,
        script = None,
        **kwargs):
    """Runs a script when `bazel test` this target.

    See [documentation] for the `args` attribute.

    **NOTE**: Like [genrule](https://bazel.build/reference/be/general#genrule)s,
    hermeticity is not enforced or guaranteed, especially if `script` accesses PATH.
    See [`Genrule Environment`](https://bazel.build/reference/be/general#genrule-environment)
    for details.

    Args:
        name: name of the target
        data: A list of labels providing runfiles. Labels may be used in `script`.

            Executables in `data` must not have the `args` and `env` attribute. Use
            [`embedded_exec`](#embedded_exec) to wrap the depended target so its env and args
            are preserved.
        hashbang: hashbang of the script, default is `"/bin/bash -e"`.
        script: The script.

            Use `$(rootpath <label>)` to refer to the path of a target specified in `data`. See
            [documentation](https://bazel.build/reference/be/make-variables#predefined_label_variables).

            Use `$@` to refer to the args attribute of this target.

            See `build/bazel_common_rules/exec/tests/BUILD` for examples.
        **kwargs: Additional attributes to the internal rule, e.g.
            [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
            See complete list
            [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).

    Deprecated:
        Use `hermetic_exec` for stronger hermeticity.
    """

    # buildifier: disable=print
    print("WARNING: {}: exec_test is deprecated. Use `hermetic_exec_test` instead.".format(
        native.package_relative_label(name),
    ))

    kwargs.setdefault("deprecation", "Use hermetic_exec_test for stronger hermeticity")

    _exec_test(
        name = name,
        data = data,
        hashbang = hashbang,
        script = script,
        **kwargs
    )

# buildifier: disable=unnamed-macro
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

    # buildifier: disable=print
    print("WARNING: exec_rule is deprecated.")

    _exec_rule(
        cfg = cfg,
        attrs = attrs,
    )
