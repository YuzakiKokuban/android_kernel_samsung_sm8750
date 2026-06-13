#!/usr/bin/env python3
#
# Copyright (C) 2024 The Android Open Source Project
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
"""Utility function to provide a best effort dependency graph."""

import argparse
import json
import logging
import pathlib
import sys

import symbol_extraction


def find_binaries(
    directory: pathlib.Path,
) -> (pathlib.Path | None, list[pathlib.Path]):
    """Locates vmlinux and kernel modules (*.ko)."""
    vmlinux = list(directory.glob("**/vmlinux"))
    modules = list(directory.glob("**/*.ko"))
    if not vmlinux:
        return None, modules
    if len(vmlinux) > 1:
        logging.error("More than one vmlinux found in %s", directory)
        sys.exit(1)
    return vmlinux[0], modules


class SetEncoder(json.JSONEncoder):
    # Needed to serialize set()
    def default(self, o):
        if isinstance(o, set):
            return list(o)
        return super().default(o)


def create_graph(
    undefined_symbols_by_module: dict[str, list[str]],
    exported_symbols_by_module: dict[str, list[str]],
    output: pathlib.Path,
):
    "Creates a best effort dependency graph from symbol relationships."
    ids = dict()
    symbol_to_module = dict()
    # Schema for the list (this uses numeric id's to reduce the output size).
    # {id: {name: str, dependents: list()}}
    adjacency_list = dict()

    for module, exported in exported_symbols_by_module.items():
        if not module in ids:
            mod_id = str(len(ids))
            ids[module] = mod_id
            adjacency_list[mod_id] = {
                "name": module,
                "dependents": set(),
            }
        exporter = ids.get(module)
        for symbol in exported:
            symbol_to_module[symbol] = exporter

    # Update the adjacency_list based on the links created by the undefined symbols.
    for module, symbols in undefined_symbols_by_module.items():
        to_id = ids.get(module)
        for symbol in symbols:
            if symbol not in symbol_to_module:
                logging.warning("%s symbol not found in any binary.", symbol)
                continue
            from_id = symbol_to_module[symbol]
            adjacency_list[from_id]["dependents"].add(to_id)

    # Print the graph.
    output.write_text(
        json.dumps(adjacency_list, cls=SetEncoder), encoding="utf-8"
    )


def main():
    """Extracts the required symbols for a directory full of kernel modules."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "directory",
        type=pathlib.Path,
        help="the directory to search for kernel binaries",
    )
    parser.add_argument(
        "output",
        type=pathlib.Path,
        help="Path for storing the output",
    )
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO,
                        format="%(levelname)s: %(message)s")
    if not args.directory.is_dir():
        logging.error(
            "Expected a directory with binaries, but got %s", args.directory
        )
        return 1

    # Locate the Kernel Binaries.
    vmlinux, modules = find_binaries(args.directory)

    # Extract undefined symbols and exported modules.
    undefined_symbols_by_module = {
        pathlib.Path(module).name: symbol_extraction.extract_undefined_symbols(
            module
        )
        for module in modules
    }
    exported_symbols_by_module = {
        pathlib.Path(blob).name: symbol_extraction.extract_exported_symbols(
            blob
        )
        for blob in [vmlinux] + modules
    }

    # Create a dependency graph.
    create_graph(
        undefined_symbols_by_module, exported_symbols_by_module, args.output
    )


if __name__ == "__main__":
    sys.exit(main())
