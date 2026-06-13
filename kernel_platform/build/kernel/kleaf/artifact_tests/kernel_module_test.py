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
import subprocess
import sys
import unittest

from absl.testing import absltest


def load_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--modules", nargs="*", default=[])
    return parser.parse_known_args()


arguments = None


class ScmVersionTestCase(unittest.TestCase):
    def test_contains_scmversion(self):
        """Test that all ko files have scmversion."""
        for module in arguments.modules:
            with self.subTest(module=module):
                self._assert_contains_scmversion(module)

    _scmversion_pattern = re.compile(r"^g[0-9a-f]{12,40}(-dirty)?$")

    def _assert_contains_scmversion(self, module):
        basename = os.path.basename(module)
        if os.path.splitext(basename)[1] != ".ko":
            self.skipTest("{} is not a kernel module".format(basename))
        try:
            scmversion = subprocess.check_output(
                ["modinfo", module, "-F", "scmversion"],
                text=True, stderr=subprocess.PIPE).strip()
        except subprocess.CalledProcessError as e:
            self.fail("modinfo returns {}: {}".format(e.returncode, e.stderr))

        self.assertRegex(scmversion, ScmVersionTestCase._scmversion_pattern,
                         "no matching scmversion")


if __name__ == '__main__':
    arguments, unknown = load_arguments()
    sys.argv[1:] = unknown
    absltest.main()
