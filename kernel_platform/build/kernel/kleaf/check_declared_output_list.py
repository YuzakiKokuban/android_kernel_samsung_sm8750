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

import argparse
import os


def check(declared, actual):
  declared = set(declared)
  actual = set(actual)
  remaining = actual.difference(declared)
  remaining = [e for e in remaining if os.path.basename(e) not in declared]
  return remaining


def main(**kwargs):
  """Work together with search_and_cp_output.py.

  Check that search_and_cp_output.py would copy all interesting output files to
  the destination directory."""

  remaining = check(**kwargs)
  for path in remaining:
    print(path)


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description=main.__doc__)

  parser.add_argument("--declared", nargs="*",
                      help="Declared output list that would be passed as positional arguments to search_and_cp_output.py")
  parser.add_argument("--actual", nargs="*",
                      help="Actual list of interesting outputs.")
  args = parser.parse_args()
  main(**vars(args))
