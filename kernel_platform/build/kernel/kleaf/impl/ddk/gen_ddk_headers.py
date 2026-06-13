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

import argparse
import collections
import dataclasses
import json
import logging
import os
import pathlib
import re
import sys
from typing import Sequence, Optional, Iterable, Any, NoReturn, Callable

from build.kernel.kleaf import buildozer_command_builder

Paths = set[pathlib.Path]
PathsWithCompUnitType = dict[pathlib.Path, Paths]
PathsWithCompUnit = lambda: collections.defaultdict(set)


def merge_paths_with_sources(self: PathsWithCompUnitType, other: PathsWithCompUnitType) \
        -> PathsWithCompUnitType:
    for k, v in other.items():
        self[k] |= v
    return self


@dataclasses.dataclass
class IncludeDataWithSource(object):
    """Holds outputs from analyze_inputs, tracking the compilation unit for each source file."""
    include_dirs: PathsWithCompUnitType = dataclasses.field(default_factory=PathsWithCompUnit)
    include_files: PathsWithCompUnitType = dataclasses.field(default_factory=PathsWithCompUnit)
    unresolved: PathsWithCompUnitType = dataclasses.field(default_factory=PathsWithCompUnit)

    def __ior__(self, other):
        merge_paths_with_sources(self.include_dirs, other.include_dirs)
        merge_paths_with_sources(self.include_files, other.include_files)
        merge_paths_with_sources(self.unresolved, other.unresolved)
        return self

    @staticmethod
    def from_dict(d, source):
        ret = IncludeDataWithSource()
        ret.include_dirs = {pathlib.Path(item): {source} for item in d["include_dirs"]}
        ret.include_files = {pathlib.Path(item): {source} for item in d["include_files"]}
        ret.unresolved = {pathlib.Path(item): {source} for item in d["unresolved"]}
        return ret


def die(*args, **kwargs) -> NoReturn:
    logging.error(*args, **kwargs)
    sys.exit(1)


def jsonify(obj):
    """Make obj valid for json.dumps."""
    if isinstance(obj, list) or isinstance(obj, set):
        return sorted([jsonify(item) for item in obj])
    if isinstance(obj, dict):
        return collections.OrderedDict(
            sorted((str(key), jsonify(value)) for key, value in obj.items()))
    return str(obj)


def endswith(a: pathlib.Path, b: pathlib.Path) -> bool:
    return len(a.parts) >= len(b.parts) and a.parts[-len(b.parts):] == b.parts


def suffix_of(a: pathlib.Path, b: pathlib.Path) -> pathlib.Path:
    if not endswith(a, b):
        die("%s does not end with %s", a, b)
    return pathlib.Path(*a.parts[:-len(b.parts)])


class Numfiles(object):
    """Lazily evaluates to the number of files """

    def __init__(self, path: pathlib.Path):
        self._path = path

    def __int__(self):
        return sum([len(files) for _, _, files in os.walk(self._path)])


class GenDdkHeaders(buildozer_command_builder.BuildozerCommandBuilder):
    def __init__(self, include_data: IncludeDataWithSource,
                 *init_args, **init_kwargs):
        super().__init__(*init_args, **init_kwargs)
        self._debug_dump = dict()
        self._include_data = include_data

        self._calc()

    def _sanitize_includes(self, d: dict[pathlib.Path, Any]) -> dict[pathlib.Path, Any]:
        """For results generated analyze_inputs, check that the given source file is available.

        If not, add to _outside or _missing respectively.
        """
        ret = dict()
        for dep, value in d.items():
            if dep.is_absolute():
                logging.debug("Unknown dep outside workspace: %s", dep)
                self._outside[dep] |= value
                continue

            try:
                dep = (self._workspace_root() / dep).resolve(strict=True).relative_to(self._workspace_root())
            except FileNotFoundError:
                logging.debug("Missing dep: %s", dep)
                self._missing[dep] |= value
                continue

            ret[dep] = value
        return ret

    def _calc(self):
        """Computes necessary data before running buildozer.

        This includes the following steps:

        * Step 0: The input from analyze_inputs
        * Step 1: Sanitize output from step 0, filtering out source files that does
          not exist in the workspace
        * Step 2: Categorize the outputs from step 0 by their package. The package
          is guessed heuristically.
        """
        for k, v in vars(self._include_data).items():
            self._dump_debug(pathlib.Path("0_input", k).with_suffix(".json"), jsonify(v))

        self._outside: PathsWithCompUnitType = PathsWithCompUnit()
        self._missing: PathsWithCompUnitType = PathsWithCompUnit()

        self._handle_unresolved()

        self._sanitized_include_data = IncludeDataWithSource(
            include_files=self._sanitize_includes(self._include_data.include_files),
            include_dirs=self._sanitize_includes(self._include_data.include_dirs),
        )
        self._dump_debug("1_sanitized/input_sanitized.json",
                         jsonify(vars(self._sanitized_include_data)))
        self._dump_debug("1_sanitized/outside.json", jsonify(self._outside))
        self._dump_debug("1_sanitized/missing.json", jsonify(self._missing))

        if self._outside:
            strs = sorted(str(path) for path in self._outside)
            logging.error("The following are outside of repo: \n%s", "\n".join(strs))

        if self._missing:
            strs = sorted(str(path) for path in self._missing)
            logging.error("The following are missing: \n%s", "\n".join(strs))

        if (self._outside or self._missing) and not self.args.keep_going:
            die("Exiting.")

        self._unknown_package: PathsWithCompUnitType = PathsWithCompUnit()

        # package -> files
        self._package_files: PathsWithCompUnitType = PathsWithCompUnit()
        for file, source_compile_units in self._sanitized_include_data.include_files.items():
            package = self._get_package(file)
            if not package:
                self._unknown_package[file] |= source_compile_units
                continue
            self._package_files[package].add(file)

        self._dump_debug("2_calc/package_files.json", jsonify(self._package_files))

        # package -> include dirs
        self._package_includes: PathsWithCompUnitType = PathsWithCompUnit()
        for include, source_compile_units in self._sanitized_include_data.include_dirs.items():
            package = self._get_package(include)
            if not package:
                self._unknown_package[include] |= source_compile_units
                continue
            self._package_includes[package].add(include)

        self._dump_debug("2_calc/package_includes.json", jsonify(self._package_includes))

        self._dump_debug("2_calc/unknown_package.json", jsonify(self._unknown_package))
        if self._unknown_package:
            strs = sorted(str(path) for path in self._unknown_package)
            logging.info("The following paths have unknown packages: \n%s", "\n".join(strs))

    def _handle_unresolved(self):
        """Prints the list of unresolved files from analyze_inputs.

        There should not be any. If there are, We don't find a good way to handle
        these yet, so let's output an error message and asks for manual intervention."""
        if self._include_data.unresolved:
            logging.error("Found unresolved includes. Run with `--dump` to trace the sources.")

        for included in self._include_data.unresolved:
            logging.error("Unresolved: %s", included)

        if self._include_data.unresolved and not self.args.keep_going:
            die("Exiting.")

    def _create_buildozer_commands(self):
        """Called by BuildozerCommandBuilder to create the actual commands to buildozer."""
        sorted_package_files = sorted(
            (package, sorted(files)) for package, files in self._package_files.items())

        # List of all known include directories, relative to workspace root
        for package, files in sorted_package_files:
            self._generate_target(package,
                                  f"all_headers_allowlist_{self.args.arch}", files,
                                  "linux_includes",
                                  self._package_includes[package],
                                  self._is_allowed)
            self._generate_target(package, "all_headers_unsafe", files,
                                  "includes",
                                  self._package_includes[package],
                                  lambda x: not self._is_allowed(x))

    def _get_package(self, path: pathlib.Path) -> Optional[pathlib.Path]:
        """Guess the package of a given path based on a list of known `--package`'s."""
        dir_parts = path.parts
        for package in self.args.package:
            if dir_parts[:len(package.parts)] == package.parts:
                return package
        return None  # ignore

    def _generate_target(self, package: pathlib.Path, name: str,
                         files: Iterable[pathlib.Path],
                         includes_attr: str,
                         include_dirs: Iterable[pathlib.Path],
                         should_include: Callable[[pathlib.Path], bool]):
        """Generates buildozer commands that puts data in ddk_headers target.

        Args:
            package: the package where the ddk_headers should be placed
            name: name of the ddk_headers target
            files: List of files to be included in the given ddk_headers target as `hdrs`
            includes_attr: Name of the attribute for includes directories. Can be `includes`
              or `linux_includes`. See doc for ddk_headers for details.
            include_dirs: List of include directories to put in `includes_attr`
            should_include: A callable that, given a path to a file or directory relative
              to the package, returns True if the file or directory should be placed in this
              ddk_headers target, or False otherwise.
        """
        target = self._new("ddk_headers", name, str(package))
        glob_dirs: PathsWithCompUnitType = PathsWithCompUnit()

        for file in sorted(files):
            rel_file = file.relative_to(package)

            if self._is_excluded(rel_file) or not should_include(rel_file):
                continue

            glob_dir = self._get_glob_dir_or_none(rel_file)
            if glob_dir:
                glob_dirs[glob_dir].add(rel_file)
            else:
                self._add_attr(target, "hdrs", str(rel_file), quote=True)

        for directory in include_dirs:
            rel_dir = directory.relative_to(package)
            if self._is_excluded(rel_dir) or not should_include(rel_dir):
                continue
            self._add_attr(target, includes_attr, rel_dir, quote=True)

        if glob_dirs:
            glob_target = self._new("filegroup", name + "_globs", str(package), load_from=None)
            # Technically an incremental run of the script on different device targets may
            # delete items inside the Bazel's glob() invocation. But we don't have a good parser for
            # Bazel's glob(), so we'll just need human-intervention here; the git diff of a change
            # immediately raises a red flag if a line is deleted.
            self._set_attr(glob_target, "srcs", """glob([{}])""".format(
                ",\\ ".join([repr(f"{d}/**/*.h") for d in glob_dirs])))
            self._add_attr(target, "hdrs", glob_target, quote=True)

            for glob_dir, files in glob_dirs.items():
                logging.info("%s has %d files, globbing %d files",
                             glob_dir, len(files),
                             Numfiles(self._workspace_root() / package / glob_dir))

    def _dump_debug(self, rel_path: pathlib.Path, obj: Any):
        """Dumps an object to a file in JSON.

        Args:
            rel_path: directory under `--dump` to place the output JSON file
            obj: the object to dump
        """
        if not self.args.dump:
            return
        path = self.args.dump / rel_path
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w") as fp:
            json.dump(obj, fp, indent=4)

    def _get_glob_dir_or_none(self, rel_file) -> Optional[pathlib.Path]:
        """If --glob already contains the given rel_file, return the glob directory. Otherwise return None."""
        rel_file_parts = rel_file.parts
        for glob_dir in self.args.glob:
            if rel_file_parts[:len(glob_dir.parts)] == glob_dir.parts:
                return glob_dir
        return None

    def _is_excluded(self, item: pathlib.Path):
        return any(re.search(pattern, str(item)) for pattern in self.args.exclude_regex)

    def _is_allowed(self, item: pathlib.Path):
        return any(item.is_relative_to(allowed) for allowed in self.args.allowed)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-i", "--input", help="Input directory or file from analyze_inputs",
                        type=pathlib.Path)
    parser.add_argument("-v", "--verbose", help="verbose mode", action="store_true")
    parser.add_argument("-k", "--keep-going",
                        help="Keeps going on errors. This includes buildozer and this script.",
                        action="store_true")
    parser.add_argument("--stdout",
                        help="buildozer writes changed BUILD file to stdout (dry run)",
                        action="store_true")
    parser.add_argument("--package", nargs="*", type=pathlib.Path,
                        help="""List of known packages. If an input file is found in the known
                                package, subpackage will not be created. Only input files
                                in known packages are considered; others are silently ignored
                                unless you turn on -v (print to stdout) or --dump (print to
                                dump directory).""",
                        default=[pathlib.Path("common")])
    parser.add_argument("--allowed", nargs="*", type=pathlib.Path,
                        help="""List of paths under --package that are known to be in the allowlist
                                of ddk_headers. Others are placed in the unsafe list.
                                """,
                        default=[pathlib.Path(e) for e in [
                            "include",
                            "arch/arm64/include",
                            "arch/x86/include",
                        ]])
    parser.add_argument("--glob", nargs="*", type=pathlib.Path,
                        help="""List of paths under --package that should be globbed instead
                                of listing individual files.""",
                        default=[pathlib.Path(e) for e in [
                            "include",
                            "arch/arm64/include",
                            "arch/x86/include",
                        ]])
    parser.add_argument("--exclude_regex", nargs="*",
                        default=[
                            r"(^|/)arch/(?!(arm64|x86))",
                            r"^tools(/|$)",
                            r"^security(/|$)",
                            r"^net(/|$)",
                            r"^scripts(/|$)",
                        ],
                        help="""List of regex patterns that should not be added to the generated
                                Bazel targets.""")
    parser.add_argument("--dump", type=pathlib.Path,
                        help="""Directory that stores debug info. This is useful to track
                                why a certain header / include directory is in the ddk_headers list
                                when it is not expected to be there.""")
    parser.add_argument("--arch",
                        default="aarch64",
                        choices=("aarch64", "x86_64"),
                        help="""Architecture of the target. This controls the name of the generated
                                ddk_headers target.""")
    return parser.parse_args(argv)


def get_all_files_and_includes(path: pathlib.Path) -> IncludeDataWithSource:
    """Merge all from args.input, tracking the source too. Return values are un-sanitized."""
    if path.is_file():
        with open(path) as f:
            return IncludeDataWithSource.from_dict(json.load(f), path)
    if path.is_dir():
        ret = IncludeDataWithSource()
        for root, _, files in os.walk(path):
            for file in files:
                with open(pathlib.Path(root, file)) as f:
                    ret |= IncludeDataWithSource.from_dict(json.load(f), pathlib.Path(root, file))
        return ret

    die("Unknown file %s", path)


def main(argv: Sequence[str]):
    args = parse_args(argv)
    log_level = logging.INFO if args.verbose else logging.WARNING
    logging.basicConfig(level=log_level, format="%(levelname)s: %(message)s")
    include_data = get_all_files_and_includes(args.input)
    GenDdkHeaders(args=args, include_data=include_data).run()


if __name__ == "__main__":
    main(sys.argv[1:])
