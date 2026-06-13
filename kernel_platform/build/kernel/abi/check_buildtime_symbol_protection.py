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
"""Enforce build time GKI modules symbol protection.

Implements a mechanism to ensure that all undefined symbols in unsigned modules
are either listed as part of the Kernel Module Interface (KMI) or defined by
another unsigned module at build time. Failure to meet this requirement will
prevent the module from loading at runtime, even if the build is successful.
This is because the symbol is likely exported by a signed (GKI) module and is
protected from being accessed by unsigned (vendor) modules, providing an early
warning at the build time to avoid a testing iteration.

Usage:

   check_buildtime_symbol_protection --abi-symbol-list ABI_SYMBOL_LIST
   [directory]
"""
import argparse
import itertools
import os
import pathlib
import subprocess
import sys

import symbol_extraction


def main():
  """Ensure undefined symbols in unsigned modules are accounted for.

  For a given directory and a given symbol list, locate all unsigned modules
  and ensure for each of them that all symbols they require (undefined) are:
  - Either listed in the symbol list (GKI public interface)
  - Or exported by another module in the lookup (vendor interface)
  """

  parser = argparse.ArgumentParser()
  parser.add_argument(
      "directory",
      nargs="?",
      default=os.getcwd(),
      help="the directory to search for unsigned modules")

  parser.add_argument(
      "--abi-symbol-list",
      required=True,
      help="ABI symbol list with symbols which are allow listed.")

  parser.add_argument(
      "--print-unsigned-modules",
      action="store_true",
      help="Emit the names of the processed unsigned modules")

  args = parser.parse_args()

  if not os.path.isdir(args.directory):
    print(
        f"Expected a directory to search for unsigned modules, but got {args.directory}",
        file=sys.stderr,
    )
    return 1

  modules = pathlib.Path(args.directory).glob("**/*.ko")

  # Find unsigned modules
  unsigned_modules = [
      module for module in modules
      if not symbol_extraction.is_signature_present(module)
  ]

  if args.print_unsigned_modules:
    print(
        "These modules have been checked for GKI protected symbol violations:")
    for module in sorted(unsigned_modules):
      print(f" {os.path.basename(module)}")

  # Find all undefined symbols from unsigned modules
  undefined_symbol_consumer_lookup = {}
  undefined_symbols = []
  for module in unsigned_modules:
    module_undefined_symbols = symbol_extraction.extract_undefined_symbols(
        module
    )
    undefined_symbols.extend(module_undefined_symbols)
    for symbol in module_undefined_symbols:
      if symbol in undefined_symbol_consumer_lookup:
        undefined_symbol_consumer_lookup[symbol].append(module.name)
      else:
        undefined_symbol_consumer_lookup[symbol] = [module.name]

  # Find all defined symbols from unsigned modules
  defined_symbols = itertools.chain.from_iterable(
      symbol_extraction.extract_exported_symbols(module)
      for module in unsigned_modules)

  # Read ABI symbols in a list
  abi_symbols = symbol_extraction.read_symbol_list(args.abi_symbol_list)

  # Set difference to get elements in undefined but not in defined or symbollist
  missing_symbols = (
      set(undefined_symbols) - set(defined_symbols) - set(abi_symbols)
  )

  if missing_symbols:
    print(
        (
            "\nThese symbols are missing from the symbol list and are not"
            " available at runtime for unsigned modules:"
        ),
        file=sys.stderr,
    )
    for symbol in sorted(missing_symbols):
      print(
          f"  {symbol} required by {undefined_symbol_consumer_lookup[symbol]}",
          file=sys.stderr,
      )
    return 1

  return 0


if __name__ == "__main__":
  sys.exit(main())
