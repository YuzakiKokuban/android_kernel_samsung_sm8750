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
"""
Tests for artifacts produced by kernel_module.
"""

load("//build/kernel/kleaf/tests:hermetic_test.bzl", "hermetic_test")

visibility("//build/kernel/kleaf/...")

def kernel_module_test(
        name,
        modules = None,
        **kwargs):
    """A test on artifacts produced by [kernel_module](kernel.md#kernel_module).

    Args:
        name: name of test
        modules: The list of `*.ko` kernel modules, or targets that produces
            `*.ko` kernel modules (e.g. [kernel_module](kernel.md#kernel_module)).
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """
    args = []
    data = []
    if modules:
        args.append("--modules")
        args += ["$(rootpaths {})".format(module) for module in modules]
        data += modules

    hermetic_test(
        name = name,
        actual = Label("//build/kernel/kleaf/artifact_tests:kernel_module_test"),
        data = data,
        args = args,
        timeout = "short",
        **kwargs
    )

def kernel_build_test(
        name,
        target = None,
        **kwargs):
    """A test on artifacts produced by [kernel_build](kernel.md#kernel_build).

    Args:
        name: name of test
        target: The [`kernel_build()`](kernel.md#kernel_build).
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """
    args = []
    data = []
    if target:
        args += ["--artifacts", "$(rootpaths {})".format(target)]
        data.append(target)

    hermetic_test(
        name = name,
        actual = Label("//build/kernel/kleaf/artifact_tests:kernel_build_test"),
        use_cc_toolchain = True,
        data = data,
        args = args,
        timeout = "short",
        **kwargs
    )

def initramfs_modules_options_test(
        name,
        kernel_images,
        expected_modules_options,
        **kwargs):
    """Tests that initramfs has modules.options with the given content.

    Args:
        name: name of the test
        kernel_images: name of the `kernel_images` target. It must build initramfs.
        expected_modules_options: file with expected content for `modules.options`
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """
    args = [
        "--expected",
        "$(rootpath {})".format(expected_modules_options),
        "$(rootpaths {})".format(kernel_images),
    ]

    hermetic_test(
        name = name,
        actual = Label("//build/kernel/kleaf/artifact_tests:initramfs_modules_options_test"),
        data = [
            expected_modules_options,
            kernel_images,
        ],
        args = args,
        timeout = "short",
        **kwargs
    )

def initramfs_modules_lists_test(
        name,
        kernel_images,
        expected_modules_list = None,
        expected_modules_recovery_list = None,
        expected_modules_charger_list = None,
        build_vendor_boot = None,
        build_vendor_kernel_boot = None,
        **kwargs):
    """Tests that the initramfs has modules.load* files with the given content.

    Args:
        name: name of the test
        kernel_images: name of the `kernel_images` target. It must build initramfs.
        expected_modules_list: file with the expected content for `modules.load`
        expected_modules_recovery_list: file with the expected content for `modules.load.recovery`
        expected_modules_charger_list: file with the expected content for `modules.load.charger`
        build_vendor_boot: If the `kernel_images` target builds vendor_boot.img
        build_vendor_kernel_boot: If the `kernel_images` target builds vendor_kernel_boot.img
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).
          See complete list
          [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).
    """
    args = []

    if expected_modules_list:
        args += [
            "--expected_modules_list",
            "$(rootpath {})".format(expected_modules_list),
        ]

    if expected_modules_recovery_list:
        args += [
            "--expected_modules_recovery_list",
            "$(rootpath {})".format(expected_modules_recovery_list),
        ]

    if expected_modules_charger_list:
        args += [
            "--expected_modules_charger_list",
            "$(rootpath {})".format(expected_modules_charger_list),
        ]

    if build_vendor_boot:
        args.append("--build_vendor_boot")
    elif build_vendor_kernel_boot:
        args.append("--build_vendor_kernel_boot")

    args.append("$(rootpaths {})".format(kernel_images))

    hermetic_test(
        name = name,
        actual = Label("//build/kernel/kleaf/artifact_tests:initramfs_modules_lists_test"),
        data = [
            expected_modules_list,
            expected_modules_recovery_list,
            expected_modules_charger_list,
            kernel_images,
        ],
        args = args,
        timeout = "short",
        **kwargs
    )
