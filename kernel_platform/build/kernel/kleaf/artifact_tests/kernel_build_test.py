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

import argparse
import os
import re
import sys
import subprocess

from absl.testing import absltest
from absl.testing import parameterized


def load_arguments():
  parser = argparse.ArgumentParser()
  parser.add_argument("--artifacts", nargs="*", default=[])
  return parser.parse_known_args()


arguments = None

_VERSION_PREFIX = r"Linux version [0-9]+\.[0-9]+\.[0-9]+(-rc[0-9]+)?"
_VERSION_SUFFIX = r"(-[0-9]+)?(-g[0-9a-f]{12,40})?(-ab[A-Z]?\d+)?"


class ScmVersionTestCase(parameterized.TestCase):
  _scmversion_patterns = [
      re.compile(_VERSION_PREFIX + r) for r in [
          r"-android[0-9]+-[0-9]+" + _VERSION_SUFFIX,
          r"-mainline" + _VERSION_SUFFIX,
          r"-maybe-dirty",
      ]
  ]

  def matches_any_pattern(self, input):
    return any(
        pattern.search(input) is not None
        for pattern in self._scmversion_patterns)

  @parameterized.parameters(
      [
          "Linux version 5.4.42-android12-0-00544-ged21d463f856",
          "Linux version 5.4.42-mainline-00544-ged21d463f856",
          "Linux version 5.18.0-rc3-mainline-19648-g9d2f688e65db",
          "Linux version 6.2.0-rc7-mainline-abP49455452",
          "Linux version 6.1.10-maybe-dirty",
      ],)
  def test_versions(self, input):
    self.assertTrue(self.matches_any_pattern(input))

  def test_vmlinux_contains_scmversion(self):
    """Test that vmlinux (if exists) has scmversion."""
    for artifact in arguments.artifacts:
      if os.path.basename(artifact) != "vmlinux":
        continue
      strings = subprocess.check_output(["llvm-strings", artifact],
                                        text=True).strip().splitlines()
      matches = any(self.matches_any_pattern(s) for s in strings)
      msg = "scmversion not found for vmlinux, found {}".format(
          [s for s in strings if "Linux version" in s])
      self.assertTrue(matches, msg)


if __name__ == "__main__":
  arguments, unknown = load_arguments()
  sys.argv[1:] = unknown
  absltest.main()
