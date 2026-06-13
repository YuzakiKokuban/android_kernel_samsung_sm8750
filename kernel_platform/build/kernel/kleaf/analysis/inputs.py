#!/usr/bin/env python3

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

"""Dumps the sha1sum of all dependent files of an aquery.

This helps you analyze why a specific action needs to be rebuilt when
building incrementally.

Example:

bazel build //common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_pipe
build/kernel/kleaf/analysis/inputs.py -- --config=fast \\
    'mnemonic("KernelModule.*", //common-modules/virtual-device:x86_64/goldfish_drivers/goldfish_pipe)'
# do some change to the code base that you don't expect it will affect this target
# then re-execute these two commands, and look for differences.
"""

import argparse
import dataclasses
import errno
import json
import os
import pathlib
import subprocess
from typing import Any


@dataclasses.dataclass(frozen=True, order=True)
class ArtifactPath(object):
    """Represents the path information of an artifact."""
    path: pathlib.Path
    is_tree_artifact: bool


def analyze_inputs(aquery_args):
    """Main entry point to the program.

    Args:
        aquery_args: arguments to `bazel aquery`
    Returns:
        A dictionary, where keys are file paths, and values are hashes.
    """
    text_result = subprocess.check_output(
        [
            "tools/bazel",
            "aquery",
            "--output=jsonproto"
        ] + aquery_args,
        text=True,
    )
    json_result = json.loads(text_result)

    # https://github.com/bazelbuild/bazel/blob/master/src/main/protobuf/analysis_v2.proto

    actions = json_result["actions"]
    artifacts = id_object_list_to_dict(json_result.get("artifacts", []))
    dep_set_of_files = id_object_list_to_dict(json_result.get("depSetOfFiles", []))
    path_fragments = id_object_list_to_dict(json_result.get("pathFragments", []))

    inputs: set[ArtifactPath] = set()
    for action in actions:
        inputs |= load_inputs(action,
                              dep_set_of_files=dep_set_of_files,
                              artifacts=artifacts,
                              path_fragments=path_fragments)

    inputs = resolve_inputs(inputs)

    return hash_all(inputs)


def id_object_list_to_dict(l: list[dict[str, Any]]) -> dict[int, dict[str, Any]]:
    """Turns a list of objects to a dictionary from IDs to these objects."""
    ret = {}
    for elem in l:
        ret[elem["id"]] = elem
    return ret


def load_inputs(action: dict[str, Any],
                dep_set_of_files: dict[int, dict[str, Any]],
                artifacts: dict[int, dict[str, Any]],
                path_fragments: dict[int, dict[str, Any]],
                ) -> set[ArtifactPath]:
    """Returns the list of input paths to an action.

    Args:
        action: the action to look at.
        dep_set_of_files: global dict of depsets
        artifacts: global dict of artifacts
        path_fragments: global dict of path fragments

    Returns:
        the set of input paths to the given action
    """
    all_inputs_artifact_ids = dep_set_to_artifact_ids(
        dep_set_ids=action.get("inputDepSetIds", []),
        dep_set_of_files=dep_set_of_files,
    )

    return artifacts_to_paths(
        artifact_ids=all_inputs_artifact_ids,
        artifacts=artifacts,
        path_fragments=path_fragments,
    )


# TODO(b/250646733): Ignore visited
def dep_set_to_artifact_ids(
        dep_set_ids: list[int],
        dep_set_of_files: dict[int, dict[str, Any]]
) -> set[int]:
    """Flattens the list of depsets.

    Args:
        dep_set_ids: list of depset IDs to look at
        dep_set_of_files: global dict of depsets

    Returns:
        a set of artifact IDs that these depsets represents.
    """
    ret = set()
    for dep_set_id in dep_set_ids:
        dep_set = dep_set_of_files[dep_set_id]
        ret |= set(dep_set.get("directArtifactIds", []))
        if dep_set.get("transitiveDepSetIds"):
            ret |= dep_set_to_artifact_ids(
                dep_set_ids=dep_set["transitiveDepSetIds"],
                dep_set_of_files=dep_set_of_files)
    return ret


# TODO(b/250646733): cache
def artifacts_to_paths(artifact_ids: set[int],
                       artifacts: dict[int, dict[str, Any]],
                       path_fragments: dict[int, dict[str, Any]]) -> set[ArtifactPath]:
    """Maps lists of artifacts to their paths.

    Args:
        artifact_ids: list of artifact IDs to look at
        artifacts: global dict of artifacts
        path_fragments: global dict of path fragments

    Returns:
        a set of paths of the given artifacts
    """
    ret = set()
    for artifact_id in artifact_ids:
        artifact = artifacts[artifact_id]
        path = ArtifactPath(
            path=pathlib.Path(*get_path(
                path_fragment_id=artifact["pathFragmentId"],
                path_fragments=path_fragments,
            )),
            is_tree_artifact=bool(artifact.get("isTreeArtifact")))
        ret.add(path)
    return ret


def get_path(
        path_fragment_id: int,
        path_fragments: dict[int, dict[str, Any]]
) -> list[str]:
    """Returns the full path that the given path fragment ID represents.

    Args:
        path_fragment_id: the path fragment ID to look at
        path_fragments: global dict of path fragments

    Returns:
        A list of path fragments of the final path.
    """
    path_fragment = path_fragments[path_fragment_id]
    if path_fragment.get("parentId"):
        ret = get_path(
            path_fragment_id=path_fragment["parentId"],
            path_fragments=path_fragments)
    else:
        ret = []
    ret.append(path_fragment["label"])
    return ret


def hash_all(paths: set[ArtifactPath]) -> dict[str, str]:
    """Hashes all the given paths.

    For files, their hashes are recorded.
    For directories, files under them are hashed.
    For non-existing paths, `None` is set in the final value.

    Args:
        paths: a set of paths to look at.
    Returns:
        a dictionary, where the keys are paths to files, and values are the hashes.
    """
    files: set[pathlib.Path] = set()
    for path in paths:
        if path.is_tree_artifact:
            files |= walk_files(path.path)
        else:
            files.add(path.path)

    exists, missing = split_existing_files(files)

    return hash_all_files(list(exists)) | {
        str(file): None for file in missing
    }


def hash_all_files(files: list[pathlib.Path]) -> dict[str, str]:
    """Hashes all the given files.

    For files, their hashes are recorded.
    For non-existing paths, `None` is set in the final value.

    Args:
        files: a set of paths to look at. They are expected to point to a file.
    Returns:
        a dictionary, where the keys are paths to files, and values are the hashes.
    """

    if not files:
        return {}

    try:
        output = subprocess.check_output([
                                             "sha1sum"
                                         ] + list(str(path) for path in files),
                                         text=True).splitlines()
        ret = dict()
        for line in output:
            sha1sum, path = line.split(maxsplit=2)
            ret[path] = sha1sum

        return ret
    except OSError as e:
        if e.errno != errno.E2BIG:
            raise e

        mid = len(files) // 2
        head = files[:mid]
        tail = files[mid:]

        if not head or not tail:
            # A single item is too big already. Continue recursing will
            # cause infinite recursion. Throwing E2BIG correctly reflects that
            # a path is too long.
            raise e

        return hash_all_files(head) | hash_all_files(tail)


def walk_files(path: pathlib.Path):
    """Returns a list of files under the given directory.

    Args:
        path: the directory
    Returns:
        the list of files under the given directory.
    """
    ret = set()
    for root, dir, files in os.walk(path):
        ret |= set(pathlib.Path(root) / file for file in files)
    return ret


def resolve_inputs(inputs: set[ArtifactPath]) -> set[ArtifactPath]:
    """Resolves paths returned by bazel aquery.

    For input files from sub-workspaces, `bazel aquery` returns the following:

        external/<workspace_name>/<label>

    However, such path does not exist starting from the root of the main
    workspace. Hence, resolve the path under execroot.

    Args:
        inputs: set of inputs returned by `bazel aquery`
        actions: list of actions
        targets: global dict of targets

    Returns:
        set of resolved inputs
    """
    resolved_inputs: set[ArtifactPath] = set()
    output_base = get_output_base()
    for input in inputs:
        if input.path.is_relative_to("external"):
            if (output_base / input.path).exists() and \
                    (output_base / input.path).is_dir() == input.is_tree_artifact:
                resolved_inputs.add(ArtifactPath(
                    path=output_base / input.path,
                    is_tree_artifact=input.is_tree_artifact,
                ))
            elif input.path.exists() and \
                    input.path.is_dir() == input.is_tree_artifact:
                resolved_inputs.add(input)
            else:
                raise FileNotFoundError(f"{input.path} ({output_base / input.path})")
        else:
            resolved_inputs.add(input)

    return resolved_inputs


def get_output_base() -> pathlib.Path:
    """Returns the output base.

    Returns:
        path to execroot relative to the current working directory (which should be the
        root of the repository).
    """
    return pathlib.Path(
        subprocess.check_output(["tools/bazel", "info", "output_base"], text=True).strip())


def split_existing_files(files: set[pathlib.Path]):
    """Splits the given list of paths into existing and missing sets.

    Args:
        files: list of paths to look at
    Returns:
        A tuple, where the first element is the set of paths that exists,
        and the second is the set of paths that doesn't exist.
    """
    exists = set()
    missing = set()

    for file in files:
        if file.exists():
            exists.add(file)
        else:
            missing.add(file)
    return exists, missing


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("aquery_args", nargs="+",
                        help="Args to `bazel aquery`.")
    args = parser.parse_args()

    results = analyze_inputs(**vars(args))
    print(json.dumps(results, indent=2, sort_keys=True))
