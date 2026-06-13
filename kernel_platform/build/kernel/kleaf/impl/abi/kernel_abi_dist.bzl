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

"""Dist rules for devices with ABI monitoring enabled."""

load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")

# TODO(b/329305827): Move exec.bzl to //build/kernel
load("//build/bazel_common_rules/exec/impl:exec.bzl", "exec_rule")
load(":abi/abi_stgdiff.bzl", "STGDIFF_CHANGE_CODE")
load(":abi/abi_transitions.bzl", "abi_common_attrs", "with_vmlinux_transition")
load(":hermetic_exec.bzl", "hermetic_exec", "hermetic_exec_target")

visibility("//build/kernel/kleaf/...")

_kernel_abi_dist_exec = exec_rule(
    cfg = with_vmlinux_transition,
    attrs = {
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    } | abi_common_attrs(),
)

def _hermetic_kernel_abi_dist_exec(**kwargs):
    return hermetic_exec_target(
        rule = _kernel_abi_dist_exec,
        **kwargs
    )

def kernel_abi_dist(
        name,
        kernel_abi,
        kernel_build_add_vmlinux = None,
        ignore_diff = None,
        no_ignore_diff_target = None,
        **kwargs):
    """A wrapper over `copy_to_dist_dir` for [`kernel_abi`](#kernel_abi).

    After copying all files to dist dir, return the exit code from `diff_abi`.

    **Implementation notes**:

    `with_vmlinux_transition` is applied on all targets by default. In
    particular, the `kernel_build` targets in `data` automatically builds
    `vmlinux` regardless of whether `vmlinux` is specified in `outs`.

    Args:
      name: name of the dist target
      kernel_abi: name of the [`kernel_abi`](#kernel_abi) invocation.
      kernel_build_add_vmlinux: If `True`, all `kernel_build` targets depended
        on by this change automatically applies a
        [transition](https://bazel.build/extending/config#user-defined-transitions)
        that always builds `vmlinux`. For
        up-to-date implementation details, look for `with_vmlinux_transition`
        in `build/kernel/kleaf/impl/abi`.

        If there are multiple `kernel_build` targets in `data`, only keep the
        one for device build. Otherwise, the build may break. For example:

        ```
        kernel_build(
            name = "tuna",
            base_kernel = "//common:kernel_aarch64"
            ...
        )

        kernel_abi(...)
        kernel_abi_dist(
            name = "tuna_abi_dist",
            data = [
                ":tuna",
                # "//common:kernel_aarch64", # remove GKI
            ],
            kernel_build_add_vmlinux = True,
        )
        ```

        Enabling this option ensures that `tuna_abi_dist` doesn't build
        `//common:kernel_aarch64` and `:tuna` twice, once with the transition
        and once without. Enabling this ensures that `//common:kernel_aarch64`
        and `:tuna` always built with the transition.

        **Note**: Its value will be `True` by default in the future.
        During the migration period, this is `False` by default. Once all
        devices have been fixed, this attribute will be set to `True` by default.
      ignore_diff: [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes).
        If `True` and the return code of `stgdiff` signals the ABI difference,
        then the result is ignored.
      no_ignore_diff_target: [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes).
        If `ignore_diff` is `True`, this need to be set to a name of the target
        that doesn't have `ignore_diff`. This target will be recommended as an
        alternative to a user. If `no_ignore_diff_target` is None, there will
        be no alternative recommended.
      **kwargs: Additional attributes to the internal rule, e.g.
        [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
        See complete list
        [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """

    # TODO(b/231647455): Clean up hard-coded name "_abi_diff_executable".
    # TODO(b/264710236): Set kernel_build_add_vmlinux by default
    # TODO(b/343698081): Simplify "ignore_diff" targets

    if kwargs.get("data") == None:
        kwargs["data"] = []

    # Use explicit + to prevent modifying the original list.
    kwargs["data"] = kwargs["data"] + [kernel_abi]

    copy_to_dist_dir(
        name = name + "_copy_to_dist_dir",
        **kwargs
    )

    if kernel_build_add_vmlinux:
        exec_macro = _hermetic_kernel_abi_dist_exec
    else:
        exec_macro = hermetic_exec

    exec_macro_script = """
          # Copy to dist dir
            $(rootpath {copy_to_dist_dir}) $@
    """.format(copy_to_dist_dir = name + "_copy_to_dist_dir")

    diff_stg = kernel_abi + "_diff_executable"

    if not ignore_diff:
        exec_macro_script += """
          # Check return code of diff_abi and kmi_enforced
            $(rootpath {diff_stg})
        """.format(diff_stg = diff_stg)
    else:
        no_ignore_diff_target_script = ""
        if no_ignore_diff_target != None:
            no_ignore_diff_target_script = """
                echo "WARNING: Use 'tools/bazel run {label}' to fail on ABI difference." >&2
            """.format(
                label = native.package_relative_label(no_ignore_diff_target),
            )
        exec_macro_script += """
          # Store return code of diff_abi and ignore if diff was found
            rc=0
            $(rootpath {diff_stg}) || rc=$?

            if [[ $rc -eq {change_code} ]]; then
                echo "WARNING: difference above is ignored." >&2
                {no_ignore_diff_target_script}
            else
                exit $rc
            fi
        """.format(
            diff_stg = diff_stg,
            change_code = STGDIFF_CHANGE_CODE,
            no_ignore_diff_target_script = no_ignore_diff_target_script,
        )

    exec_macro(
        name = name,
        data = [
            name + "_copy_to_dist_dir",
            kernel_abi + "_diff_executable",
        ],
        script = exec_macro_script,
    )
