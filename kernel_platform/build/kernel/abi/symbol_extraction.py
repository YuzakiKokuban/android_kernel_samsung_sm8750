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
"""Common APIs related to symbol extractions.

extract_exported_symbols(): Extracts the ksymtab exported symbols from an ELF
binary.
extract_undefined_symbols(): Extracts the undefined symbols from an ELF file at
binary_path.
is_signature_present(): Checks whether a kernel module file has a PKCS#7
signature appended.
read_symbol_list(): Reads a previously created libabigail format symbol list
into a list of symbols.
"""

import subprocess


def extract_exported_symbols(binary):
  """Extracts the ksymtab exported symbols from an ELF binary."""
  symbols = []
  out = subprocess.check_output(["llvm-nm", "--defined-only", binary],
                                stderr=subprocess.DEVNULL).decode("ascii")
  for line in out.splitlines():
    pos = line.find(" __ksymtab_")
    if pos != -1:
      symbols.append(line[pos + len(" __ksymtab_"):])

  return symbols


def extract_undefined_symbols(binary_path):
  """Extracts the undefined symbols from an ELF file at  binary_path."""
  symbols = []
  out = subprocess.check_output(["llvm-nm", "--undefined-only", binary_path],
                                stderr=subprocess.DEVNULL).decode("ascii")
  for line in out.splitlines():
    symbols.append(line.strip().split()[1])

  return symbols


def is_signature_present(module):
  """Checks whether module has a signature appended (GKI) or not (vendor)"""
  out = subprocess.check_output(["modinfo", "-F", "sig_id", module],
                                stderr=subprocess.STDOUT).decode("ascii")
  return out == "PKCS#7\n"


def read_symbol_list(symbol_list):
  """Reads a previously created libabigail symbol symbol list."""
  symbols = []
  with open(symbol_list) as symbol_list_file:
    for line in [l.strip() for l in symbol_list_file]:
      if not line or line.startswith("#") or line.startswith("["):
        continue
      symbols.append(line)
  return symbols
