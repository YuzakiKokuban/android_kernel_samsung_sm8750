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
import subprocess

from absl import flags
from absl.testing import absltest
import pathlib
import os
import subprocess
import unittest

flags.DEFINE_string("dir", None, "Directory of modules")
flags.DEFINE_string("module", None, "name of module to check")
flags.DEFINE_boolean("expect_signature", None, "Whether to expect signature from the module")
flags.mark_flags_as_required(
    ["dir", "module", "expect_signature"]
)

FLAGS = flags.FLAGS


class CheckModuleSignatureTest(unittest.TestCase):
    def test_module_signature(self):
        found = False
        module_parts = pathlib.Path(FLAGS.module).parts
        for root, dirs, files in os.walk(FLAGS.dir):
            root_path = pathlib.Path(root)
            for filename in files:
                file_path = root_path / filename
                file_path_parts = file_path.parts
                if len(file_path_parts) >= len(module_parts) and \
                    file_path_parts[-len(module_parts):] == module_parts:

                    found = True
                    with self.subTest(file_path = file_path):
                        self.assert_signature(file_path)

        self.assertTrue(found, f"{FLAGS.module} is not found under {FLAGS.dir}")


    def assert_signature(self, file_path):
        # TODO(b/250667773): Use signer or signature
        sig_id=subprocess.check_output(["modinfo", "-F", "sig_id", file_path],
                                       text=True).strip()
        expected_sig_id = "PKCS#7" if FLAGS.expect_signature else ""
        self.assertEqual(expected_sig_id, sig_id)


if __name__ == '__main__':
    absltest.main()
