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

"""Tests all .bzl files in //build/kernel/kleaf/impl package has visibility().

This excludes subpackages like //build/kernel/kleaf/impl/fake_rules_cc/...
"""

import pathlib
import unittest
from absl.testing import absltest


class VisibilityTest(unittest.TestCase):
    def test_visibility(self) -> None:
        for path in pathlib.Path(".").glob("**/*.bzl"):
            with self.subTest(str(path)), open(path) as f:
                has_visibility = any(line.startswith(
                    "visibility(") for line in f)
                self.assertTrue(
                    has_visibility, f"{path} should have visibility()")


if __name__ == "__main__":
    absltest.main()
