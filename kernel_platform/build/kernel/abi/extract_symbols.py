#!/usr/bin/env python3
#
# Copyright (C) 2019 The Android Open Source Project
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
import collections
import functools
import itertools
import os
import re
import subprocess
import sys

import symbol_extraction

_ALWAYS_INCLUDED = [
    "module_layout",  # is exported even if CONFIG_TRIM_UNUSED_KSYMS is enabled
    "__put_task_struct",  # this allows us to keep `struct task_struct` stable
]
_ABIGAIL_HEADER = "[abi_symbol_list]"

def symbol_sort(symbols):
  # Use a method similar to `LANG=en_US sort`: case insensitive and ignoring
  # underscores, that keeps symbols with related names close to each other.

  def __key(a):
    """Creates a key for comparison of symbols."""
    # if the caller passes None or an empty string something is odd, so assert
    # and ignore if asserts are disabled as we do not need to deal with that
    assert (a)
    if not a:
      return a

    # We want to ignore case and underscores, except that we want to sort
    # underscore-prefixed symbols before others. So use the (unique) symbol name
    # as a tie-breaker.
    return (a.lower().replace("_", ""), a)

  return sorted(set(symbols), key=__key)


def find_binaries(directory):
  """Locates vmlinux and kernel modules (*.ko)."""
  vmlinux = None
  modules = []
  for root, dirs, files in os.walk(directory):
    for file in files:
      if file.endswith(".ko"):
        modules.append(os.path.join(root, file))
      elif file == "vmlinux":
        vmlinux = os.path.join(root, file)

  return vmlinux, modules


def extract_undefined_symbols_multiple(modules):
  """Extracts undefined symbols from a list of module files."""

  # yes, we could pass all of them to nm, but I want to avoid hitting shell
  # limits with long lists of modules
  result = {}
  for module in sorted(modules):
    result[os.path.basename(module)] = symbol_sort(
        symbol_extraction.extract_undefined_symbols(module))

  return result


def extract_generic_exports(vmlinux, modules):
  """Extracts the ksymtab exported symbols from vmlinux and a set of modules."""
  symbols = symbol_extraction.extract_exported_symbols(vmlinux)
  for module in modules:
    symbols.extend(symbol_extraction.extract_exported_symbols(module))
  return symbol_sort(symbols)


def extract_exported_in_modules(modules):
  """Extracts the ksymtab exported symbols for a list of kernel modules."""
  return {
      module: symbol_sort(symbol_extraction.extract_exported_symbols(module))
      for module in modules
  }


def report_missing(module_symbols, exported):
  """Reports missing symbols that are undefined, but not known in any binary."""
  for module, symbols in module_symbols.items():
    for symbol in symbols:
      if symbol not in exported:
        print("Symbol {} required by {} but not provided".format(
            symbol, module))


def add_dependent_symbols(module_symbols, exported):
  """Checks the undefined symbols, and adds more to enforce missing dependencies."""
  for module, symbols in module_symbols.items():
    syms = []
    for symbol in symbols:

      # Tracepoints are exposed in the ABI using their matching struct
      # tracepoint. Sadly this exposes callback functions as void * pointers,
      # which make the ABI tooling ineffective to monitor tracepoint changes.
      # To enable ABI checks covering tracepoint, add the matching __traceiter
      # symbols to the symbol list as they are defined with full types.
      if not symbol.startswith('__tracepoint_'):
        continue
      cur = symbol.replace('__tracepoint_', '__traceiter_')
      if (cur not in exported) or (cur in symbols):
        continue
      syms.append(cur)
    module_symbols[module].extend(syms)


def create_symbol_list(symbol_list, undefined_symbols, exported,
                       emit_module_symbol_lists, module_grouping,
                       additions_only):
  """Creates a libabigail format symbol list."""
  precious_symbols = set()
  if additions_only:
    precious_symbols.update(symbol_extraction.read_symbol_list(symbol_list))

  symbol_counter = collections.Counter(
      itertools.chain.from_iterable(undefined_symbols.values()))

  with open(symbol_list, "w") as wl:

    common_symbols = [
        symbol for symbol, count in symbol_counter.items()
        if (count > 1 or not module_grouping) and symbol in exported
    ] + _ALWAYS_INCLUDED

    # When both --additions-only and --skip-module-grouping are used together,
    # we sort the unused symbols together will all of the other symbols.
    if additions_only and not module_grouping:
        common_symbols.extend(precious_symbols)
        precious_symbols.clear()

    common_wl_section = symbol_sort(common_symbols)

    # write the header
    wl.write(_ABIGAIL_HEADER)
    wl.write("\n")
    if module_grouping:
      wl.write("# commonly used symbols\n")
    wl.write("  ")
    wl.write("\n  ".join(common_wl_section))
    wl.write("\n")
    precious_symbols.difference_update(common_wl_section)

    for module, symbols in undefined_symbols.items():

      if emit_module_symbol_lists:
        mod_wl_file = symbol_list + "_" + os.path.splitext(module)[0]
        with open(mod_wl_file, "w") as mod_wl:
          # write the header
          mod_wl.write(_ABIGAIL_HEADER)
          mod_wl.write("\n  ")
          mod_wl.write("\n  ".join([s for s in symbols if s in exported]))
          mod_wl.write("\n")

      new_wl_section = symbol_sort([
          symbol for symbol in symbols
          if symbol in exported and symbol not in common_wl_section
      ])

      if not new_wl_section:
        continue

      wl.write("\n# required by {}\n  ".format(module))
      wl.write("\n  ".join(new_wl_section))
      wl.write("\n")
      precious_symbols.difference_update(new_wl_section)

    if precious_symbols:
      wl.write("\n# preserved by --additions-only\n  ")
      wl.write("\n  ".join(symbol_sort(precious_symbols)))
      wl.write("\n")


def main():
  """Extracts the required symbols for a directory full of kernel modules."""
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "directory",
      nargs="?",
      default=os.getcwd(),
      help="the directory to search for kernel binaries")

  parser.add_argument(
      "--skip-report-missing",
      action="store_false",
      dest="report_missing",
      help="Do not report symbols required by modules, but missing from vmlinux"
  )

  parser.add_argument(
      "--include-module-exports",
      action="store_true",
      help="Include inter-module symbols")

  parser.add_argument(
      "--full-gki-abi",
      action="store_true",
      help="Assume all vmlinux and GKI module symbols are part of the ABI")

  parser.add_argument(
      "--symbol-list", "--whitelist",
      help="The symbol list to create")

  parser.add_argument(
      "--additions-only",
      action="store_true",
      help="Read the existing symbol list and ensure no symbols get removed")

  parser.add_argument(
      "--print-modules",
      action="store_true",
      help="Emit the names of the processed modules")

  parser.add_argument(
      "--emit-module-symbol-lists", "--emit-module-whitelists",
      action="store_true",
      help="Emit a separate symbol list for each module")

  parser.add_argument(
      "--skip-module-grouping",
      action="store_false",
      dest="module_grouping",
      help="Do not group symbols by module. When coupled with --additions-only, the unused symbols are sorted with all the symbols")

  parser.add_argument(
      "--module-include",
      action="append",
      dest="module_includes",
      help="Only process modules matching the filter. Can be passed multiple times."
  )

  parser.add_argument(
      "--module-exclude",
      action="append",
      dest="module_excludes",
      help="Do not process modules matching the filter. Can be passed multiple times."
  )

  args = parser.parse_args()

  if not os.path.isdir(args.directory):
    print("Expected a directory to search for binaries, but got %s" %
          args.directory)
    return 1

  if args.emit_module_symbol_lists and not args.symbol_list:
    print("Emitting module symbol lists requires the --symbol-list parameter.")
    return 1

  if args.symbol_list is None:
    args.symbol_list = "/dev/stdout"

  # Locate the Kernel Binaries
  vmlinux, modules = find_binaries(args.directory)

  if args.module_includes:
    modules = [
        mod for mod in modules if any(
            [re.search(f, os.path.basename(mod)) for f in args.module_includes])
    ]

  if args.module_excludes:
    modules = [
        mod for mod in modules if not any(
            [re.search(f, os.path.basename(mod)) for f in args.module_excludes])
    ]

  # Partition vendor (unsigned) and GKI modules (signed) in two lists
  t1, t2 = itertools.tee(modules)
  gki_modules = list(filter(symbol_extraction.is_signature_present, t1))
  local_modules = list(
      itertools.filterfalse(symbol_extraction.is_signature_present, t2))

  if vmlinux is None or not os.path.isfile(vmlinux):
    print("Could not find a suitable vmlinux file.")
    return 1

  # Get required symbols of all modules
  gki_undefined_symbols = extract_undefined_symbols_multiple(gki_modules)
  local_undefined_symbols = extract_undefined_symbols_multiple(local_modules)
  undefined_symbols = {}
  undefined_symbols.update(gki_undefined_symbols)
  undefined_symbols.update(local_undefined_symbols)

  # Get the actually defined and exported symbols
  generic_exports = extract_generic_exports(vmlinux, gki_modules)
  local_exports = extract_exported_in_modules(local_modules)

  # Build the list of all exported symbols (generic and local)
  all_exported = list(
      itertools.chain.from_iterable(local_exports.values()))
  all_exported.extend(generic_exports)
  all_exported = set(all_exported)

  add_dependent_symbols(undefined_symbols, all_exported)

  # For sanity, check for inconsistencies between required and exported symbols
  # Do not do this analysis if module_includes or module_excludes are in place as likely
  # inter-module dependencies are broken by this.
  if args.report_missing and not (args.module_includes or args.module_excludes):
    report_missing(undefined_symbols, all_exported)

  # If specified, create the symbol list
  if args.symbol_list:
    create_symbol_list(
        args.symbol_list,
        { "full-gki-abi": generic_exports } if args.full_gki_abi else local_undefined_symbols,
        all_exported if args.include_module_exports else generic_exports,
        args.emit_module_symbol_lists,
        args.module_grouping,
        args.additions_only)

  if args.print_modules:
    if local_modules:
      print("These modules have been considered when creating the symbol list:")
      print("  " +
            "\n  ".join(sorted([os.path.basename(mod) for mod in local_modules])))

    if gki_modules:
      print("These modules have *NOT* been considered when creating the symbol list:")
      print("  " +
            "\n  ".join(sorted([os.path.basename(mod) for mod in gki_modules])))

if __name__ == "__main__":
  sys.exit(main())
