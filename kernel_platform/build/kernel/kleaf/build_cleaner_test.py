#!/usr/bin/env python3

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

"""Tests for build_cleaner."""

# This test requires buildozer installed in $HOME, which is not accessible
# via `bazel test`. Hence, execute this test with
#   build/kernel/kleaf/build_cleaner_test.py
# TODO(b/257176147): Move this to bazel py_test, then use:
#   absl.testing.parameterized
#   absltest.main
# TODO(b/257176147): Add this test to kernel_aarch64_additional_tests

import os
import tempfile
import unittest

import build_cleaner

_TEST_DATA = "build/kernel/kleaf/tests/build_cleaner_test_data"


class BuildCleanerTest(unittest.TestCase):
    def setUp(self) -> None:
        self.environ = os.environ.copy()

        self.stdout = tempfile.TemporaryFile('w+')
        self.addCleanup(self.stdout.close)

        self.stderr = tempfile.TemporaryFile('w+')
        self.addCleanup(self.stderr.close)

    def _run_cleaner(self, argv):
        argv = ["--stdout"] + argv
        args = build_cleaner.parse_args(argv)
        cleaner = build_cleaner.BuildCleaner(
            args=args,
            stdout=self.stdout,
            stderr=self.stderr,
            environ=self.environ
        )
        cleaner.run()

    def _read_stdout(self):
        self.stdout.seek(0)
        return self.stdout.read()


class DdkModuleDepTest(BuildCleanerTest):
    def test_ddk_module_dep_good(self):
        self._run_cleaner([
            f"//{_TEST_DATA}/ddk_module_dep/good:modules_install"
        ])
        self.assertIn('deps = [":parent"],', self._read_stdout())

    def test_ddk_module_dep_unresolved(self):
        with self.assertRaises(build_cleaner.BuildCleanerError) as cm:
            self._run_cleaner([f"//{_TEST_DATA}/ddk_module_dep/unresolved:child"])

        self.assertEquals(
            f'//{_TEST_DATA}/ddk_module_dep/unresolved:child: "parent_func" '
            f'[../{_TEST_DATA}/ddk_module_dep/unresolved/child.ko] undefined!',
            str(cm.exception))


if __name__ == '__main__':
    unittest.main(verbosity=2)
