
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

"""Helper classes to support `kleaf help`."""

import argparse
import dataclasses
import os
import pathlib
import re
import textwrap

_BAZEL_RC_DIR = "build/kernel/kleaf/bazelrc"
FLAGS_BAZEL_RC = "build/kernel/kleaf/bazelrc/flags.bazelrc"

_FLAG_PATTERN = re.compile(
    r"build --flag_alias=(?P<short_name>[a-z_]+)=(?P<is_negated>no)?(?P<label>[a-zA-Z/:_-]+)$")

_FLAG_COMMENT_PATTERN = re.compile(
    r'(?P<comment>((#\s*.*)\n)*)(?P<rule>[a-z_]+)\(\s*name\s*=\s*"(?P<name>[a-zA-Z-_]+)"',
    flags=re.MULTILINE
)

_CONFIG_PATTERN = re.compile(
    r"^--config=(?P<config>[a-z_]+):\s*(?P<description>.*)$"
)


class KleafHelpPrinter(object):
    def __init__(self):
        pass

    def add_startup_option_to_parser(self, parser: argparse.ArgumentParser):
        """Add startup options to the given ArgumentParser."""
        raise NotImplementedError

    def add_command_args_to_parser(self, parser: argparse.ArgumentParser):
        """Add command arguments to the given ArgumentParser."""
        raise NotImplementedError

    def print_kleaf_help(self, kleaf_repo_dir: pathlib.Path):
        """Print Kleaf help menu to stdout."""
        parser = argparse.ArgumentParser(
            prog="bazel",
            add_help=False,
            usage="bazel [<startup options>] <command> [<args>] [--] [<target patterns>]",
            formatter_class=argparse.RawTextHelpFormatter,
        )

        parser.add_argument_group(
            title="Startup options",
            description=textwrap.dedent("""\
                Consists of "Wrapper flags" and "Native flags".
                """))
        self.add_startup_option_to_parser(parser)
        parser.add_argument_group(
            title="Startup options - Native flags",
            description="$ bazel help startup_options")

        parser.add_argument_group(
            title="Command",
            description="""$ bazel help""",
        )

        self.add_command_args_to_parser(parser)
        bazelrc_parser = FlagsBazelrcParser(
            kleaf_repo_dir / FLAGS_BAZEL_RC)
        bazelrc_parser.add_to(parser, kleaf_repo_dir=kleaf_repo_dir)

        config_group = parser.add_argument_group(
            title="Args - configs"
        )
        for f in os.listdir(kleaf_repo_dir / _BAZEL_RC_DIR):
            bazelrc_parser = ConfigBazelrcParser(
                kleaf_repo_dir / _BAZEL_RC_DIR / f)
            bazelrc_parser.add_to(
                config_group, kleaf_repo_dir=kleaf_repo_dir)

        # Additional helper queries for target discovery.
        kleaf_group = parser.add_argument_group(
            title=textwrap.dedent("""\
                                  Kleaf Help - Query commands.
                                  Usage: bazel help kleaf [<command>]"""),
        )
        kleaf_group.add_argument(
            "targets",
            help="List kernel_build targets under current WORKSPACE",
        )
        kleaf_group.add_argument(
            "abi_targets",
            help="List ABI related targets under current WORKSPACE",
        )

        parser.add_argument_group(
            title="Target patterns",
            description="$ bazel help target-syntax"
        )

        parser.print_help()


class BazelrcSection(object):
    def __init__(self):
        self.comments: list[str] = []

    def add_comment(self, comment_line: str):
        self.comments.append(comment_line)

    def handle_line(self, line):
        pass

    def add_to(self, parser_or_group, kleaf_repo_dir: pathlib.Path):
        raise NotImplementedError


class BazelrcParser(object):
    def __init__(self, path: pathlib.Path):
        with open(path) as f:
            cur_section = self.new_section()
            self.sections = [cur_section]
            for line in f:
                line = line.strip()
                if not line:
                    cur_section = self.new_section()
                    self.sections.append(cur_section)
                    continue
                if line.startswith("#"):
                    cur_section.add_comment(line.removeprefix("#").strip())
                else:
                    cur_section.handle_line(line)

    def new_section(self) -> BazelrcSection:
        raise NotImplementedError

    def add_to(self, parser_or_group, kleaf_repo_dir: pathlib.Path):
        parser_or_group.add_argument_group("Args - Kleaf flags")

        for section in self.sections:
            section.add_to(parser_or_group, kleaf_repo_dir=kleaf_repo_dir)


@dataclasses.dataclass
class FlagAlias(object):
    short_name: str
    label: str
    is_negated: bool

    _build_file_cache = {}

    def read_flag_comment(self, kleaf_repo_dir: pathlib.Path):
        # TODO(b/256052600): Use buildozer
        build_file_rel = self.label.removeprefix('//')
        build_file_rel = build_file_rel[:build_file_rel.index(":")]
        label_name = self.label[self.label.index(":") + 1:]
        build_file = kleaf_repo_dir / build_file_rel / "BUILD.bazel"

        self._rule = None
        self._description = ""

        if build_file not in FlagAlias._build_file_cache:
            with open(build_file) as f:
                FlagAlias._build_file_cache[build_file] = {}
                for mo in _FLAG_COMMENT_PATTERN.finditer(f.read()):
                    FlagAlias._build_file_cache[build_file][mo.group("name")] = {
                        "rule": mo.group("rule"),
                        "comment": mo.group("comment"),
                    }

        parsed_build_file = FlagAlias._build_file_cache[build_file]
        if label_name in parsed_build_file:
            self._rule = parsed_build_file[label_name]["rule"]
            comment = parsed_build_file[label_name]["comment"]
            if self.is_negated:
                self._description = "Negates the following flag: \n"
            else:
                self._description = ""
            self._description += "\n".join(
                line.removeprefix("#").strip() for line in comment.split("\n")
            )

    def add_to_group(self, group, kleaf_repo_dir: pathlib.Path):
        self.read_flag_comment(kleaf_repo_dir=kleaf_repo_dir)

        kwargs = {
            "help": self._description,
        }
        if self._rule == "bool_flag":
            kwargs["action"] = "store_true"
        elif self._rule == "label_flag":
            kwargs["metavar"] = "LABEL"
        else:
            kwargs["metavar"] = "VAL"

        group.add_argument(
            f"--{self.short_name}",
            **kwargs
        )


class FlagsSection(BazelrcSection):
    def __init__(self):
        super().__init__()
        self.flags: list[FlagAlias] = []

    def handle_line(self, line):
        mo = _FLAG_PATTERN.match(line)
        if not mo:
            return
        self.flags.append(FlagAlias(
            short_name=mo.group("short_name"),
            is_negated=mo.group("is_negated") == "no",
            label=mo.group("label")
        ))

    def add_to(self, parser: argparse.ArgumentParser, kleaf_repo_dir: pathlib.Path):
        if not self.flags:
            # Skip for license header
            return

        title = None
        description = None
        if self.comments:
            title = self.comments[0]
            if len(self.comments) > 1:
                description = "\n".join(self.comments[1:])

        title = "Args - " + title

        group = parser.add_argument_group(
            title=title,
            description=description,
        )

        for alias in self.flags:
            try:
                alias.add_to_group(group, kleaf_repo_dir=kleaf_repo_dir)
            except argparse.ArgumentError:
                # For flags like --use_prebuilt_gki, its help is already printed by the wrapper.
                # Skip them.
                pass


class FlagsBazelrcParser(BazelrcParser):
    def new_section(self):
        return FlagsSection()


class ConfigSection(BazelrcSection):
    def add_to(self, group, kleaf_repo_dir: pathlib.Path):
        if not self.comments:
            return
        mo = _CONFIG_PATTERN.match(self.comments[0])
        if not mo:
            return

        config = mo.group("config")
        description_first_line = mo.group("description").strip()
        description = []
        if description_first_line:
            description.append(description_first_line)
        description += self.comments[1:]
        description = "\n".join(description)
        group.add_argument(f"--config={config}",
                           help=description,
                           action="store_true")


class ConfigBazelrcParser(BazelrcParser):
    def new_section(self):
        return ConfigSection()
