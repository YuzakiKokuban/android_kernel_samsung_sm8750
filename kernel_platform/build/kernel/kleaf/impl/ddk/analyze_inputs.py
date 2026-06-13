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

"""Analyze the inputs from `.cmd` files"""
import argparse
import asyncio
import collections
import dataclasses
import fnmatch
import functools
import json
import logging
import operator
import pathlib
import os
import shlex
import re
import tarfile
from typing import Iterable, Optional, Any

# Regex to parse .cmd files. Each section has the format of:
# dep_foo := \
#   a.h \
#   b.h
_RE = r"^(?P<key>\S*?)\s*:=(?P<values>((\\\n| |\t)+(\S*))*)"


def _make_rel(path: pathlib.Path):
    """Makes a reasonable relative path from path."""
    if not path.is_absolute():
        return path
    return pathlib.Path(*path.parts[1:])


@dataclasses.dataclass
class IncludeData(object):
    include_dirs: set[pathlib.Path] = dataclasses.field(default_factory=set)
    include_files: set[pathlib.Path] = dataclasses.field(default_factory=set)
    unresolved: set[pathlib.Path] = dataclasses.field(default_factory=set)

    def __ior__(self, other):
        self.include_dirs |= other.include_dirs
        self.include_files |= other.include_files
        self.unresolved |= other.unresolved
        return self

    def to_dict(self) -> dict[str, list[str]]:
        dict_pairs = []
        for key, values in vars(self).items():
            dict_pairs.append((key, sorted(str(value) for value in values)))
        return collections.OrderedDict(dict_pairs)


class AnalyzeInputs(object):

    def __init__(self, out: pathlib.Path, dirs: list[pathlib.Path],
                 module_srcs: list[pathlib.Path],
                 include_filters: list[str], exclude_filters: list[str],
                 gen_files_archives: list[tarfile.TarFile], **ignored):
        self._out = out
        self._dirs = dirs
        self._include_filters = include_filters
        self._exclude_filters = exclude_filters
        self._module_srcs = set(module_srcs)
        self._unresolved: set[pathlib.Path] = set()

        self._cmd_parser = argparse.ArgumentParser()
        self._cmd_parser.add_argument("-I", type=pathlib.Path, action="append", default=[])
        self._cmd_parser.add_argument("-include", type=pathlib.Path, action="append", default=[])
        self._cmd_parser.add_argument("--sysroot", type=pathlib.Path)

        self._archived_input_names: set[pathlib.Path] = set()
        for archive in gen_files_archives:
            names = archive.getnames()
            paths = set(pathlib.Path(os.path.normpath(name)) for name in names)
            self._archived_input_names.update(paths)

    async def run(self):
        self._out.mkdir(parents=True, exist_ok=True)
        aws = []
        for dir in self._dirs:
            for root, _, files in os.walk(dir):
                root_path = pathlib.Path(root)
                for filename in files:
                    aws.append(self._write_deps(root_path / filename))

        await asyncio.gather(*aws)

    async def _write_deps(self, path: pathlib.Path):
        deps = self._get_deps(path)
        stem = self._out / _make_rel(path)
        stem.parent.mkdir(parents=True, exist_ok=True)
        with open(stem.with_suffix(".json"), "w") as file:
            json.dump(deps.to_dict(), file, indent=2)


    def _get_deps(self, path: pathlib.Path) -> IncludeData:
        ret = IncludeData()

        deps = dict()
        cmds = dict()
        with open(path) as f:
            for mo in re.finditer(_RE, f.read(), re.MULTILINE):
                key = mo.group("key")
                if key.startswith("deps_"):
                    deps[key.removeprefix("deps_")] = mo.group("values")
                elif key.startswith("cmd_"):
                    cmds[key.removeprefix("cmd_")] = mo.group("values")

            for object, deps_str in deps.items():
                deps_str = deps_str.replace("\\\n", " ")
                one_deps = set(self._filter_deps(deps_str.split()))
                one_parse_data = self._resolve_files(one_deps, cmds.get(object), path)
                ret |= one_parse_data
        return ret

    def _filter_deps(self, dep_strs: Iterable[str]) -> Iterable[pathlib.Path]:
        for dep_str in dep_strs:
            dep_str = dep_str.strip()
            if not dep_str:
                continue
            if dep_str.startswith("$(wildcard") or dep_str.endswith(")"):
                # Ignore wildcards; we don't need them for headers analysis
                continue

            for exclude_filter in self._exclude_filters:
                if fnmatch.fnmatch(dep_str, exclude_filter):
                    continue

            should_include = any(fnmatch.fnmatch(dep_str, i) for i in self._include_filters)
            should_exclude = any(fnmatch.fnmatch(dep_str, i) for i in self._exclude_filters)

            if should_include and not should_exclude:
                yield pathlib.Path(dep_str)

    def _parse_cmd(self, cmd: Optional[str]) -> IncludeData:
        if not cmd:
            return IncludeData()

        ret = IncludeData()
        # Simple cmd parser
        for one_cmd in cmd.split(";"):
            tokens = shlex.split(one_cmd)
            if not tokens or "clang" not in pathlib.Path(tokens[0]).name:
                continue
            known, _ = self._cmd_parser.parse_known_args(tokens[1:])
            ret.include_files |= set(known.include)
            ret.include_dirs |= set(AnalyzeInputs._resolve_path(dir) for dir in known.I)
            if known.sysroot:
                ret.include_dirs.add(AnalyzeInputs._resolve_path(known.sysroot))
        return ret

    def _resolve_files(self, deps: Iterable[pathlib.Path], cmd: Optional[str],
                       cmd_file_path: pathlib.Path) -> IncludeData:
        cmd_parse_data = self._parse_cmd(cmd)

        ret_deps = set()
        unresolved = set()

        for dep_list in (cmd_parse_data.include_files, deps):
            for dep in dep_list:
                # Absolute paths are unrecognized. All paths should already be handled by
                # replacing ${ROOT_DIR} with a fake value.
                if dep.is_absolute():
                    logging.warning("%s: Unknown dep with absolute path %s", cmd_file_path, dep)
                    unresolved.add(dep)
                    continue

                if dep.parts[0] == "${ROOT_DIR}":
                    # Resolve assuming ${ROOT_DIR} is CWD. Resolve because there might be
                    # .. in the path.
                    resolved_dep = AnalyzeInputs._resolve_path(dep)
                    if resolved_dep in self._module_srcs:
                        ret_deps.add(resolved_dep)
                        continue

                # Ignore headers in archives of generated files
                if dep in self._archived_input_names:
                    continue

                logging.warning("%s: Unknown dep %s", cmd_file_path, dep)
                unresolved.add(dep)
        return IncludeData(cmd_parse_data.include_dirs, ret_deps, unresolved)

    @staticmethod
    def _resolve_path(path: pathlib.Path):
        if path.parts[0] == "${ROOT_DIR}":
            path = pathlib.Path(*path.parts[1:]).resolve().relative_to(
                pathlib.Path(".").resolve())
        return path


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=pathlib.Path, required=True)
    parser.add_argument("--dirs", type=pathlib.Path, nargs="*", default=[])
    parser.add_argument("-v", "--verbose", action="store_true", default=False)
    parser.add_argument("--include_filters", nargs="*", default=["*"])
    parser.add_argument("--exclude_filters", nargs="*", default=[])
    parser.add_argument("--gen_files_archives", type=tarfile.open, nargs="*", default=[],
                        help="List of tar of generated files. Generated files are not considered"
                            "as inputs to a target.")
    parser.add_argument("--module_srcs", type=pathlib.Path, nargs="*", default=[])

    args = parser.parse_args()
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(level=log_level, format="%(levelname)s: %(message)s")

    asyncio.run(AnalyzeInputs(**vars(parser.parse_args())).run())
