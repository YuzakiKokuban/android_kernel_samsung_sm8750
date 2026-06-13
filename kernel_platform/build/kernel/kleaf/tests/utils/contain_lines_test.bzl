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

"""Tests for contents of a given file."""

def contain_lines_test(name, expected, actual, order = None):
    """See `contain_lines_test.py` for explanation.

    Args:
        name: name of test
        expected: A label expanding into the expected files.
        actual: A label expanding into the actual files.
        order: If True, also assert ordering.
    """

    args = [
        "--actual",
        "$(locations {})".format(actual),
        "--expected",
        "$(locations {})".format(expected),
    ]
    if order:
        args.append("--order")

    native.py_test(
        name = name,
        python_version = "PY3",
        main = "contain_lines_test.py",
        srcs = ["//build/kernel/kleaf/tests/utils:contain_lines_test.py"],
        data = [expected, actual],
        args = args,
        timeout = "short",
        deps = [
            "@io_abseil_py//absl/testing:absltest",
        ],
    )
