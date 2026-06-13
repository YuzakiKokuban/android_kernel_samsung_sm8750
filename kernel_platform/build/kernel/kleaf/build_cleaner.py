#!/usr/bin/env python3

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

"""Kleaf build_cleaner: Fixes dependencies in BUILD files.

Given a list of target patterns [Note 1], the script:

1. Finds all dependencies of the targets matching the given target
   patterns [Note 1]. All targets matching the target patterns plus all
   dependencies forms a closure.
2. For all targets in the closure, fix BUILD files [Note 2]

Currently, the script supports fixing the following:

- `kernel_module.deps` [Note 3]
- `ddk_module.deps`    [Note 3]

Notes:
    1. https://bazel.build/run/build#specifying-build-targets

    2. The script requires buildozer to be installed on the host machine.
       In addition, the script only works if the target is specified directly in
       BUILD or BUILD.bazel files. It does not work if the target is wrapped in
       a macro. See documentations for buildozer for details:
       https://github.com/bazelbuild/buildtools/tree/master/buildozer

    3. Fixing kernel_module.deps / ddk_module.deps only works when all modules
       of the device are in the closure, e.g. by specifying the `dist` target
       which depends on the `kernel_modules_install` target which includes all
       known modules.
       See example below.

Examples:

    # Fix all rules for the tuna device.
    build/kernel/kleaf/build_cleaner.py //path/to/package:tuna_dist
"""

import argparse
import buildozer_command_builder
import collections
import dataclasses
import logging
import re
import subprocess
import sys
import pathlib
from typing import Sequence

_MODULE_SYMBOL_PATTERN = r'^0x[0-9a-f]+\s+([_a-zA-Z][_a-zA-Z0-9]*)\s+(\S+)\s+EXPORT_SYMBOL\s*$'
_MODPOST_ERROR_PATTERN = r'modpost: "([_a-zA-Z][_a-zA-Z0-9]*)" \[(\S*)] undefined!'


class BuildCleanerError(Exception):
    pass


class Label(object):
    def __init__(self, s: str):
        # We don't support subworkspaces yet.
        mo = re.match(r"@?//([^:]*):(.*)", s)
        if not mo:
            raise ValueError(
                "{} is not a label known to build_cleaner".format(s))
        self.package = mo.group(1)
        self.name = mo.group(2)

    def bazel_bin_path(self) -> pathlib.Path:
        return pathlib.Path("bazel-bin") / self.package / self.name

    def make_stderr_path(self) -> pathlib.Path:
        """Hack to infer the location of make_stderr.txt for a target label.

        Refer to `debug.bzl`, _modpost_warn.
        """
        return self.bazel_bin_path() / "make_stderr.txt"

    def module_symvers_path(self) -> pathlib.Path:
        """Hack to get Module.symvers for a target.

        Refer to `ddk_module.bzl` and `kernel_module.bzl`, internal_module_symvers_name
        """
        path = self.bazel_bin_path() / "Module.symvers"
        if path.is_file():
            return path
        path = self.bazel_bin_path() / (self.name + "_Module.symvers")
        if path.is_file():
            return path

        raise FileNotFoundError("Module.symvers for {}".format(self))

    def __str__(self):
        return "//{}:{}".format(self.package, self.name)

    def __repr__(self):
        return "Label('{}')".format(self)


@dataclasses.dataclass
class SymbolLocation(object):
    target: Label
    module_file: str

    def __str__(self):
        return '{} [{}]'.format(self.target, self.module_file)


class SingleCleaner(object):
    def __init__(self, cleaner: "BuildCleaner"):
        self._workspace_root = cleaner.workspace_root()
        self.stderr = cleaner.stderr
        self.stdout = cleaner.stdout
        self.environ = cleaner.environ
        self._args = cleaner.args
        self._color = (cleaner.stdout.isatty() or cleaner.stderr.isatty())


class DdkCleaner(SingleCleaner):
    def __init__(self, cleaner: "BuildCleaner"):
        super().__init__(cleaner)

        self.deps: dict[Label, list[Label]] = collections.defaultdict(list)
        self._calc()

    def _bazel(self) -> str:
        return str(self._workspace_root / "tools" / "bazel")

    def _calc(self):
        """Calculates the missing dependencies."""

        # Find all dependencies of kind kernel_module
        query_args = [
            self._bazel(),
            "query",
            'kind("kernel_module rule", deps({}))'.format(
                " union ".join(self._args.targets))
        ]
        if self._color:
            query_args.append("--color=yes")
        try:
            query_out: str = subprocess.check_output(query_args,
                                                     text=True,
                                                     stderr=self.stderr,
                                                     env=self.environ)
        except subprocess.CalledProcessError:
            raise BuildCleanerError(
                "Unable to query kernel_module deps for %s" % self._args.targets)

        kernel_module_target_strs = query_out.splitlines()

        # Build all these kernel_module's with --debug_modpost_warn
        try:
            subprocess.check_call([
                self._bazel(),
                "build",
                "--debug_modpost_warn",
            ] + kernel_module_target_strs,
                stderr=self.stderr, env=self.environ,
                stdout=self.stdout)
        except subprocess.CalledProcessError:
            raise BuildCleanerError("Unable to build the following with --debug_modpost_warn: %s" %
                                    kernel_module_target_strs)

        kernel_module_targets = [Label(target)
                                 for target in kernel_module_target_strs]

        symbols: dict[str, list[SymbolLocation]
                      ] = collections.defaultdict(list)

        for target in kernel_module_targets:
            logging.info("Looking up symbols for %s", target)
            with open(target.module_symvers_path()) as f:
                for line in f.readlines():
                    for mo in re.finditer(_MODULE_SYMBOL_PATTERN, line):
                        symbol = mo.group(1)
                        symbols[symbol].append(SymbolLocation(
                            target=target,
                            module_file=mo.group(2),
                        ))

        errors = []

        for target in kernel_module_targets:
            logging.info("Checking missing deps for %s", target)
            with open(target.make_stderr_path()) as f:
                for mo in re.finditer(_MODPOST_ERROR_PATTERN, f.read()):
                    symbol = mo.group(1)
                    module_file = mo.group(2)

                    if symbol not in symbols:
                        errors.append(
                            '{}: "{}" [{}] undefined!'.format(target, symbol, module_file))
                        continue

                    if len(symbols[symbol]) > 1:
                        errors.append('{}: "{}" [{}] found in multiple locations:\n  {}'.format(
                            target, symbol, module_file,
                            "\n  ".join(str(loc) for loc in symbols[symbol])
                        ))

                    self.deps[target] += [loc.target for loc in symbols[symbol]]

        if errors:
            if self._args.keep_going:
                for error in errors:
                    logging.error(error)
            else:
                raise BuildCleanerError("\n".join(errors))


class BuildCleaner(buildozer_command_builder.BuildozerCommandBuilder):
    def __init__(self, *init_args, **init_kwargs):
        super().__init__(*init_args, **init_kwargs)
        self._ddk_cleaner = DdkCleaner(self)

    def _bazel(self) -> str:
        return str(self._workspace_root() / "tools" / "bazel")

    def _create_buildozer_commands(self):
        for target, deps in self._ddk_cleaner.deps.items():
            for dep in deps:
                self._add_attr(str(target), "deps", str(dep), quote=True)

    def workspace_root(self):
        return self._workspace_root()


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("-v", "--verbose",
                        help="verbose mode", action="store_true")
    parser.add_argument("-k", "--keep-going",
                        help="Keeps going on errors. Use when targets are already "
                             "defined. There may be duplicated FIXME comments.",
                        action="store_true")
    parser.add_argument("--stdout",
                        help="buildozer writes changed BUILD file to stdout (dry run)",
                        action="store_true")
    parser.add_argument("targets", nargs="+",
                        help="List of target patterns, of which rules for all"
                             "dependencies are fixed.")

    return parser.parse_args(argv)


def main(argv: Sequence[str]):
    args = parse_args(argv)
    log_level = logging.INFO if args.verbose else logging.WARNING
    logging.basicConfig(level=log_level, format="%(levelname)s: %(message)s")
    BuildCleaner(args=args).run()


if __name__ == "__main__":
    try:
        main(sys.argv[1:])
    except BuildCleanerError as e:
        logging.error("%s", e)
        sys.exit(1)
