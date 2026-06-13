#!/usr/bin/env python3
#
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
#

import argparse
import os
import sys

import symbol_extraction


def update_gki_protected_exports(directory, gki_protected_modules_list,
                                 protected_exports_list):
  """Updates the protected_exports_list with exports from modules in gki_protected_modules_list file"""

  with open(gki_protected_modules_list) as f:
    protected_module_names = [line.strip() for line in f if line.strip()]

  protected_gki_modules = []
  for protected_module in protected_module_names:
    full_path = os.path.join(directory, protected_module)
    if not os.path.isfile(full_path):
      print(f"Warning: Couldn't find module {full_path}")
      continue

    protected_gki_modules.append(full_path)

  gki_protected_exports = []
  for module in protected_gki_modules:
    gki_protected_exports.extend(
        symbol_extraction.extract_exported_symbols(module))

  with open(protected_exports_list, "w") as protected_exports_symbol_list:
    protected_exports_symbol_list.write("\n".join(
        sorted(set(gki_protected_exports))))


def main():
  """Extracts the required symbols for a directory full of kernel modules."""
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "directory",
      nargs="?",
      default=os.getcwd(),
      help="the directory to search for kernel binaries")

  parser.add_argument(
      "--protected-exports-list",
      required=True,
      help="The symbol list to create with protected exports (e.g. common/android/abi_gki_protected_exports)"
  )

  parser.add_argument(
      "--gki-protected-modules-list",
      required=True,
      help="A file with list of GKI protected modules (e.g. common/android/gki_protected_modules)"
  )

  args = parser.parse_args()

  if not os.path.isdir(args.directory):
    print("Expected a directory to search for binaries, but got %s" %
          args.directory)
    return 1

  update_gki_protected_exports(args.directory, args.gki_protected_modules_list,
                               args.protected_exports_list)

  return 0


if __name__ == "__main__":
  sys.exit(main())
