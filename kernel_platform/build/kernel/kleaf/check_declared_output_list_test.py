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

import unittest

from absl.testing import absltest
from check_declared_output_list import check


class MyTestCase(unittest.TestCase):
  def test_empty(self):
    self.assertFalse(check([], []))

  def test_simple(self):
    self.assertFalse(check(["foo"], ["foo"]))

  def test_simple_remain(self):
    self.assertEqual(check(declared=[], actual=["foo"]), ["foo"])

  def test_path_remain(self):
    self.assertEqual(check(declared=[], actual=["foo/bar"]), ["foo/bar"])

  def test_path_simple(self):
    self.assertFalse(check(["some/path/for/foo"], ["some/path/for/foo"]))

  def test_path_ok(self):
    self.assertFalse(check(declared=["foo"], actual=["some/path/for/foo"]))

  def test_non_matching_path(self):
    self.assertEqual(
        check(declared=["some/path/for/foo"], actual=["foo"]), ["foo"])

  def test_non_matching_path2(self):
    self.assertEqual(
        check(declared=["some/path/for/foo"], actual=["other/path/for/foo"]),
        ["other/path/for/foo"])


if __name__ == '__main__':
  absltest.main()
