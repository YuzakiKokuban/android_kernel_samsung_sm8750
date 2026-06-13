#!/usr/bin/env python3
#
# Copyright (C) 2021 The Android Open Source Project
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
import sys


_TRACE_POINT = '__tracepoint_'
_TRACE_ITER = '__traceiter_'


def _validate_symbols(symbol_list, symbols):
  """Validates Tracepoints consistenty in a given symbol list."""
  missing = []
  for symbol in symbols:
    if not symbol.startswith((_TRACE_POINT, _TRACE_ITER)):
      continue
    if symbol.startswith(_TRACE_POINT):
      other = symbol.replace(_TRACE_POINT, _TRACE_ITER)
      if other not in symbols:
        missing.append(other)
    if symbol.startswith(_TRACE_ITER):
      other = symbol.replace(_TRACE_ITER, _TRACE_POINT)
      if other not in symbols:
        missing.append(other)
  if missing:
    print(
        'ERROR: Missing symbols: ',
        missing,
        'in ',
        os.path.basename(symbol_list),
        file=sys.stderr,
    )
    sys.exit(1)


def _read_denied_symbols_config(deny_file):
  """Reads denied symbols configuration file."""
  denied_symbols = {}

  with open(deny_file) as file:
    for line in file:
      fields = line.rstrip('\n').split(None, 1)
      if not fields:
        continue
      symbol = fields[0]
      if symbol.startswith('#'):
        continue
      reason = ''
      if len(fields) > 1:
        reason = fields[1]
      if symbol in denied_symbols:
        print(f"symbol '{symbol}' duplicate configuration", file=sys.stderr)
        continue
      denied_symbols[symbol] = reason

  return denied_symbols


def _read_symbol_lists(symbol_lists):
  """Reads libabigail symbol list files as a list of lines."""
  all_lines = []
  for symbol_list in symbol_lists:
    with open(symbol_list) as sl:
      lines = sl.read().splitlines(keepends=True)
    all_lines.extend(lines)
    # Separate files or at least protect against missing final newlines.
    all_lines.append('\n')
    # validate symbols by file
    _validate_symbols(symbol_list, _get_symbols(lines))
  return all_lines


def _get_symbols(lines):
  """Gets symbols from symbol list lines."""
  symbols = set()
  for line in lines:
    stripped = line.strip()
    if stripped and not stripped.startswith(('#', '[')):
      symbols.add(stripped)
  return symbols


def main():
  dir = os.path.dirname(sys.argv[0])
  deny_file = os.path.join(dir, 'symbols.deny')

  parser = argparse.ArgumentParser()
  parser.add_argument(
      'symbol_lists',
      metavar='FILE',
      type=str,
      nargs='+',
      help='a symbol list file',
  )
  parser.add_argument(
      '--in-dir', required=True, help='where to find the symbol list files'
  )
  parser.add_argument(
      '--out-dir',
      required=True,
      help='where to put the combined symbol list and report',
  )
  parser.add_argument(
      '--out-file', required=True, help='combined symbol list file name'
  )
  parser.add_argument(
      '--verbose', action='store_true', help='increase verbosity of the output'
  )

  args = parser.parse_args()

  in_directory = args.in_dir
  out_directory = args.out_dir
  symbol_lists = [os.path.join(in_directory, s) for s in args.symbol_lists]
  out_file = os.path.join(out_directory, args.out_file)

  denied_symbols = _read_denied_symbols_config(deny_file)
  lines = _read_symbol_lists(symbol_lists)
  symbols = _get_symbols(lines)

  if args.verbose:
    print('========================================================')
    print(f'Generating ABI symbol list definition in {out_file}')
  with open(out_file, 'w') as sl:
    sl.writelines(lines)

  exit_status = 0
  if args.verbose:
    print('Checking symbols are not forbidden')
  for symbol in symbols:
    if symbol in denied_symbols:
      reason = denied_symbols[symbol]
      print(f"symbol '{symbol}' is not allowed: {reason}", file=sys.stderr)
      exit_status = 1

  return exit_status


if __name__ == '__main__':
  sys.exit(main())
