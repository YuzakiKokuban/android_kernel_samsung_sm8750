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
import subprocess
import sys
import tempfile
import unittest

from absl.testing import absltest


def load_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--expected")
    parser.add_argument("files", nargs="*", default=[])
    return parser.parse_known_args()


arguments = None


class InitramfsModulesOptions(unittest.TestCase):
    def test_diff(self):
        initramfs_list = [f for f in arguments.files if os.path.basename(f) == "initramfs.img"]
        self.assertEqual(1, len(initramfs_list))
        initramfs = initramfs_list[0]

        with open(arguments.expected) as expected:
            with tempfile.TemporaryDirectory() as temp_dir:
                with open(initramfs) as initramfs_file:
                    with subprocess.Popen(["cpio", "-i"], cwd=temp_dir,
                                          stdin=subprocess.PIPE, stdout=subprocess.PIPE) as cpio_sp:
                        # Assume LZ4_RAMDISK is not set for this target.
                        with subprocess.Popen(["gzip", "-c", "-d"], stdin=initramfs_file,
                                              stdout=cpio_sp.stdin) as gzip_sp:
                            gzip_sp.communicate()
                            self.assertEqual(0, gzip_sp.returncode)

                lib_modules = os.path.join(temp_dir, "lib/modules")
                self.assertTrue(os.path.isdir(lib_modules))

                kernel_versions = os.listdir(lib_modules)
                for v in kernel_versions:
                    modules_options = os.path.join(lib_modules, v, "modules.options")
                    self.assertTrue(os.path.isfile(modules_options), f"Can't find {modules_options}")

                    with open(modules_options) as modules_options_file:
                        expected.seek(0)
                        self.assertEqual(modules_options_file.read(), expected.read())


if __name__ == '__main__':
    arguments, unknown = load_arguments()
    sys.argv[1:] = unknown
    absltest.main()
