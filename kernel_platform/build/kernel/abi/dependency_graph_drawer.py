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
"""Utility function to create a visualization graph using dot language."""

import argparse
import hashlib
import json
import logging
import pathlib
import sys


def _create_graphviz(
    adjacency_list: dict,
    output: pathlib.Path,
    colors: bool,
):
    "Creates a diagram to display a graph using DOT language."
    content = ["digraph {"]
    content.extend([
        "\tgraph [rankdir=LR, splines=ortho];",
        "\tnode [color=steelblue, shape=plaintext];",
        "\tedge [arrowhead=odot, color=olive];",
    ])
    leaves = []
    for node in adjacency_list.values():
        # vmlinux is dependency for most of the nodes so skip it.
        if node["name"] == "vmlinux":
            continue
        # Skip nodes without dependents.
        if not node["dependents"]:
            leaves.append(node["name"])
            continue
        edges = []
        for neighbor in node["dependents"]:
            edges.append(f'"{adjacency_list[neighbor]["name"]}"')
        edge_str = ",".join(edges)
        # Customize edge colors.
        edge_color = ""
        if colors:
            h = hashlib.shake_256(edge_str.encode())
            edge_color = f' [color="  # {h.hexdigest(3)}"]'
        content.append(f'\t"{node["name"]}" -> {edge_str}{edge_color};')
    logging.warning("Leaf nodes: [%s]", leaves)
    content.append("}")
    output.write_text("\n".join(content), encoding="utf-8")


def _read_graph(
    adjacency_list_file: str,
):
    try:
        with pathlib.Path(adjacency_list_file).open() as adjacency_list:
            return json.load(adjacency_list)
    except Exception as exc:
        raise argparse.ArgumentError(
            f"{adjacency_list_file}", "Failed to load."
        ) from exc


def main():
    """Creates two maps of dependencies for a directory full of kernel modules."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "adjacency_list",
        type=_read_graph,
        help="File with a graph represented as an adjacency list.",
    )
    parser.add_argument(
        "output", type=pathlib.Path, help="Where to store the output"
    )
    parser.add_argument(
        "--colors",
        action="store_true",
        help=(
            "Edges to dependents of a module share the same color. This is"
            " useful to differentiate dependencies of a module."
        ),
    )

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO,
                        format="%(levelname)s: %(message)s")

    # Create graph visualization.
    _create_graphviz(args.adjacency_list, args.output, args.colors)


if __name__ == "__main__":
    sys.exit(main())
