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
"""Verify every symbol in symbol list is exported in ksymtab.

Verifies that all symbols specified in the vendor symbol list,
which is supplied using the --raw-kmi-symbol-list argument,
are included in the generated ksymtab. The ksymtab is produced
using the --symvers-file argument and a list of objects, specified
with the --objects argument (with vmlinux set as the default),
which are considered for generating the ksymtab.

Conducting this validation during the build time provides an early
warning of a possible runtime failure. This step guarantees that
the vendor (unsigned) module does not use any symbols that are not
included in the KMI symbol list.
"""

import argparse
import os
import sys

import symbol_extraction


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "--raw-kmi-symbol-list",
      required=True,
      help="KMI symbol list",
  )

  parser.add_argument(
      "--symvers-file",
      required=True,
      help="symvers file to extract ksymtab information (e.g. Module.symvers)",
  )

  parser.add_argument(
      "--objects",
      nargs="*",
      default=["vmlinux"],
      help="Kernel binaries to consider for ksymtab verification",
  )

  args = parser.parse_args()

  # Parse Module.symvers, and ignore non-exported and vendor-specific symbols
  ksymtab_symbols = []
  with open(args.symvers_file) as symvers_file:
    for line in symvers_file:
      _, symbol, object, export_type = line.strip().split("\t", maxsplit=3)
      if export_type.startswith("EXPORT_SYMBOL") and object in args.objects:
        ksymtab_symbols.append(symbol)

  # List of symbols defined in the raw_kmi_symbol_list
  kmi_symbols = symbol_extraction.read_symbol_list(args.raw_kmi_symbol_list)

  # Set difference to get elements in symbol list but not in ksymtab
  missing_ksymtab_symbols = set(kmi_symbols) - set(ksymtab_symbols)
  if missing_ksymtab_symbols:
    print("Symbols missing from the ksymtab:", file=sys.stderr)
    for symbol in sorted(missing_ksymtab_symbols):
      print(f"  {symbol}", file=sys.stderr)
    return 1

  return 0


if __name__ == "__main__":
  sys.exit(main())
