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

import argparse
import sys
import unittest
import zipfile

from absl.testing import absltest


def load_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--zip_file", help="zip file to test")
    return parser.parse_known_args()


arguments = None


class ZipruleTest(unittest.TestCase):

  def test_zip_content(self):
    with zipfile.ZipFile(arguments.zip_file, 'r') as zip_ref:
      self.assertEqual(
          [
              'a.txt',
              'all_srcs/',
              'all_srcs/a.txt',
              'all_srcs/b.txt',
              'all_srcs/c.txt',
              'all_srcs/d.txt',
              'all_srcs/e.txt',
              'all_srcs/hello_world.c',
              'b.txt',
              'bin/',
              'bin/hello_world',
              'new_dir1/',
              'new_dir1/new_dir2/',
              'new_dir1/new_dir2/dir3/',
              'new_dir1/new_dir2/dir3/d.txt',
              'new_dir1/new_dir2/dir3/e.txt',
          ],
          sorted(zip_ref.namelist()),
      )


if __name__ == '__main__':
    arguments, unknown = load_arguments()
    sys.argv[1:] = unknown
    absltest.main()
