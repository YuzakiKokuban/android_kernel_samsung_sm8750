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

"""Tests for `ddk_submodule`."""

load("//build/kernel/kleaf/impl:ddk/ddk_headers.bzl", "ddk_headers")
load("//build/kernel/kleaf/impl:ddk/ddk_module.bzl", "ddk_module")
load("//build/kernel/kleaf/impl:ddk/ddk_submodule.bzl", "ddk_submodule")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/tests:failure_test.bzl", "failure_test")
load(":ddk_module_test.bzl", "ddk_module_test")

def _ddk_module_test_make(
        name,
        expected_inputs = None,
        expected_hdrs = None,
        expected_includes = None,
        **kwargs):
    ddk_module(
        name = name + "_module",
        tags = ["manual"],
        **kwargs
    )

    ddk_module_test(
        name = name,
        target_under_test = name + "_module",
        expected_inputs = expected_inputs,
        expected_hdrs = expected_hdrs,
        expected_includes = expected_includes,
    )

def ddk_submodule_test(name):
    """Tests for `ddk_submodule`.

    Args:
        name: name of the test suite."""
    kernel_build(
        name = name + "_kernel_build",
        build_config = "build.config.fake",
        outs = [],
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_headers",
        includes = ["include"],
        hdrs = ["include/subdir.h"],
        tags = ["manual"],
    )

    tests = []

    ddk_submodule(
        name = name + "_submodule_self",
        out = name + "_submodule_self.ko",
        srcs = ["self.c", "self.h"],
        tags = ["manual"],
    )

    ddk_submodule(
        name = name + "_submodule_dep",
        out = name + "_submodule_dep.ko",
        srcs = ["dep.c"],
        tags = ["manual"],
    )

    # Test module with one submodule

    _ddk_module_test_make(
        name = name + "_one",
        kernel_build = name + "_kernel_build",
        deps = [name + "_submodule_self"],
        expected_inputs = ["self.c", "self.h"],
    )
    tests.append(name + "_one")

    # Test module with two submodules

    _ddk_module_test_make(
        name = name + "_two",
        kernel_build = name + "_kernel_build",
        deps = [
            name + "_submodule_self",
            name + "_submodule_dep",
        ],
        expected_inputs = [
            "dep.c",
            "self.c",
            "self.h",
        ],
    )
    tests.append(name + "_two")

    # Test on locally depending on a ddk_headers target

    ddk_submodule(
        name = name + "_external_headers_submodule",
        out = name + "_external_headers_submodule.ko",
        deps = [name + "_headers"],
        srcs = [],
        tags = ["manual"],
    )

    _ddk_module_test_make(
        name = name + "_external_headers",
        kernel_build = name + "_kernel_build",
        deps = [name + "_external_headers_submodule"],
        expected_inputs = ["include/subdir.h"],
    )
    tests.append(name + "_external_headers")

    # Test on exporting a ddk_headers target

    ddk_submodule(
        name = name + "_export_ddk_headers_submodule",
        out = name + "_export_ddk_headers_submodule.ko",
        hdrs = [name + "_headers"],
        srcs = [],
        tags = ["manual"],
    )

    _ddk_module_test_make(
        name = name + "_export_ddk_headers",
        kernel_build = name + "_kernel_build",
        deps = [name + "_export_ddk_headers_submodule"],
        expected_inputs = ["include/subdir.h"],
        expected_hdrs = ["include/subdir.h"],
        expected_includes = [native.package_name() + "/include"],
    )
    tests.append(name + "_export_ddk_headers")

    # Test on exporting headers + includes
    ddk_submodule(
        name = name + "_export_my_headers_submodule",
        out = name + "_export_my_headers_submodule.ko",
        hdrs = ["include/subdir.h"],
        includes = ["include"],
        srcs = [],
        tags = ["manual"],
    )

    _ddk_module_test_make(
        name = name + "_export_my_headers",
        kernel_build = name + "_kernel_build",
        deps = [name + "_export_my_headers_submodule"],
        expected_inputs = ["include/subdir.h"],
        expected_hdrs = ["include/subdir.h"],
        expected_includes = [native.package_name() + "/include"],
    )
    tests.append(name + "_export_my_headers")

    # Test that a ddk_module with ddk_submodules must not define banned attributes.

    for kwargs in [
        {"srcs": ["dep.c"]},
        {"out": "a.ko"},
        {"hdrs": ["self.h"]},
        {"includes": ["include"]},
        {"local_defines": ["FOO=bar"]},
        {"copts": ["-Werror"]},
    ]:
        attr = kwargs.keys()[0]
        ddk_module(
            name = "{}_module_with_submodule_and_{}".format(name, attr),
            kernel_build = name + "_kernel_build",
            deps = [name + "_submodule_self"],
            tags = ["manual"],
            **kwargs
        )

        failure_test(
            name = "{}_no_{}_with_submodule".format(name, attr),
            target_under_test = "{}_module_with_submodule_and_{}".format(name, attr),
            error_message_substrs = [
                "with submodules, {} should be specified in individual ddk_submodule".format(attr),
            ],
        )
        tests.append("{}_no_{}_with_submodule".format(name, attr))

    native.test_suite(
        name = name,
        tests = tests,
    )
