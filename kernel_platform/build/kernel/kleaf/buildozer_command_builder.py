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
import dataclasses
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Callable, Optional, TextIO, Iterable, Mapping

_BUILDOZER_RETURN_CODE_NO_CHANGES_MADE = 3


def die(*args, **kwargs):
    logging.error(*args, **kwargs)
    sys.exit(1)


def isinstance_or_die(obj, clazz):
    if not isinstance(obj, clazz):
        die("Object %s is not an instance of %s", obj, clazz)
    return obj


def ensure_build_file(package: str, cwd: pathlib.Path):
    if os.path.isabs(package):
        die("%s is not a relative path.", package)
    abs_package = cwd / package
    if not (abs_package / "BUILD.bazel").is_file() and \
            not (abs_package / "BUILD").is_file():
        build_file = abs_package / "BUILD.bazel"
        logging.info(f"Creating {build_file}")
        with open(build_file, "w"):
            pass


@dataclasses.dataclass(frozen=True)
class InfoKey(object):
    """The key of the dictionary storing information for existing BUILD files."""

    # Full label of the target.
    target: str


class TargetKey(InfoKey):
    pass


@dataclasses.dataclass(frozen=True)
class AttributeKey(InfoKey):
    """The key of the dictionary storing information for existing BUILD files."""

    # Name of the attribute.
    attribute: Optional[str]


class InfoValue(object):
    """The value of the dictionary storing information for existing BUILD files."""

    # Attribute value is None.
    NONE = "None"

    # Attribute value is not set, or target does not exist.
    MISSING = None


@dataclasses.dataclass
class AttributeValue(InfoValue):
    # String-representation of the attribute value.
    # - If attribute value is None, this is the string "None" (InfoValue.NONE).
    # - If attribute value is not set, this is the value None (InfoValue.MISSING)
    value: Optional[str | list[str]] = InfoValue.MISSING

    # String that contains the comment.
    # If comment is not found, this is the value None.
    comment: Optional[str] = InfoValue.MISSING

    def is_missing(self):
        return self.value is InfoValue.MISSING

    def is_none_value(self):
        return self.value == InfoValue.NONE

    def is_missing_or_none(self):
        return self.is_missing() or self.is_none_value()


@dataclasses.dataclass
class TargetValue(InfoValue):
    # Kind of the declaration (e.g. kernel_build)
    kind: Optional[str] = InfoValue.MISSING


class BuildozerCommandBuilder(object):
    def __init__(self, args: argparse.Namespace, stdout: Optional[TextIO] = None,
                 stderr: Optional[TextIO] = None,
                 environ: Mapping[str, str] = None):
        """
        Args:
             args: Namespace containing command-line arguments
             stdout: Override stdout stream for subprocesses
             stderr: Override stderr stream for subprocesses
             environ: Override environment variables for subprocesses
        """

        self.stdout = stdout or sys.stdout
        self.stderr = stderr or sys.stderr
        self.environ = environ or os.environ
        self.args = args

        # Add full label as a comment to name for testing purposes.
        self._add_package_comment_for_test = False

        self.buildozer = self._find_buildozer()

        # set in context manager
        self.out_file: Optional[TextIO] = None

        # set in run
        self.existing: Optional[dict[InfoKey, InfoValue]] = None

    def __enter__(self):
        self.out_file = tempfile.NamedTemporaryFile("w+")
        return self

    def __exit__(self, exc, value, tb):
        self.out_file.close()
        self.out_file = None

    def _find_buildozer(self) -> str:
        buildozer = shutil.which("buildozer")
        if buildozer:
            return buildozer

        gopath = self.environ.get("GOPATH", os.path.join(self.environ["HOME"], "go"))
        buildozer = os.path.join(gopath, "bin", "buildozer")
        if os.path.isfile(buildozer):
            return buildozer

        die("Can't find buildozer. Install with instructions at "
            "https://github.com/bazelbuild/buildtools/blob/master/buildozer/README.md")

    def _get_all_info(self, keys: Iterable[InfoKey]) -> dict[InfoKey, InfoValue]:
        """Gets all interesting information of existing BUILD files.

        Args:
            keys: The list of interesting information to get.
        """
        ret = dict()
        for key in keys:
            tup = None
            if isinstance(key, TargetKey):
                tup = self._get_target(key.target)
            elif isinstance(key, AttributeKey):
                tup = self._get_attr(key)
            ret[tup[0]] = tup[1]
        return ret

    def _buildozer_print(self, target, print_command, attribute,
                         parse_list=False) -> Optional[str | list[str]]:
        """Executes a buildozer print command."""
        value = InfoValue.MISSING

        try:
            value = subprocess.check_output(
                [self.buildozer, f"{print_command} {attribute}", target],
                text=True, stderr=subprocess.PIPE, env=self.environ,
                cwd=self._workspace_root()).strip()
        except subprocess.CalledProcessError:
            pass

        if value is not InfoValue.MISSING and parse_list:
            list_value = type(self)._parse_label_list(value)
            if list_value is not None:
                return list_value

        return value

    @staticmethod
    def _parse_label_list(value: str) -> Optional[list[str]]:
        # https://bazel.build/concepts/labels#target-names
        label_re = r"([a-zA-Z0-9@^_\"#$&'()*+,;<=>?\[\]{\|}~/.-]+)"
        label_list_re = r"^\[(%s( %s)*)?\]$" % (label_re, label_re)
        if not re.match(label_list_re, value):
            return None
        return value.removeprefix("[").removesuffix("]").split(" ")

    def _get_target(self, target: str) -> tuple[InfoKey, InfoValue]:
        """Gets information about a single target from existing BUILD files.

        Args:
            target: full label of target.
        """
        kind = self._buildozer_print(target, "print", "kind")
        return TargetKey(target), TargetValue(kind)

    def _get_attr(self, key: AttributeKey) -> tuple[InfoKey, InfoValue]:
        """Gets a single attribute of existing BUILD files.

        Args:
            key: the InfoKey.
        """
        value = self._buildozer_print(key.target, "print", key.attribute, parse_list=True)
        comment = self._buildozer_print(key.target, "print_comment", key.attribute)
        return key, AttributeValue(value=value, comment=comment)

    @staticmethod
    def _is_bash_func(build_config: str) -> bool:
        return build_config.startswith("BASH_FUNC_") and build_config.endswith("%%")

    def _new(self, kind: str, name: str, package: str, load_from="//build/kernel/kleaf:kernel.bzl") \
            -> str:
        """Writes a buildozer command that creates a target.

        Returns:
            the new target
        """
        if package is None:
            die("No package specified in _new()")
        ensure_build_file(package, self._workspace_root())
        new_target_pkg = f"//{package}:__pkg__"
        new_target = f"//{package}:{name}"
        key = TargetKey(new_target)

        existing_kind = InfoValue.MISSING
        if key in self.existing:
            existing_kind = isinstance_or_die(self.existing[key], TargetValue).kind

        if load_from:
            self.out_file.write(f"""
                fix movePackageToTop|{new_target_pkg}
                new_load {load_from} {kind}|{new_target_pkg}
            """)

        if existing_kind is InfoValue.MISSING:
            self.out_file.write(f"""
                new {kind} {name}|{new_target_pkg}
            """)
            self.existing[key] = TargetValue(kind=kind)
        elif existing_kind != kind:
            logging.warning(f"Forcefully setting {new_target} from {existing_kind} to {kind}")
            self.out_file.write(f"""
                set kind {kind}|{new_target}
            """)
            self.existing[key] = TargetValue(kind=kind)

        if self._add_package_comment_for_test:
            self._add_comment(new_target, "name", new_target)

        return new_target

    def _set_kind(self, target: str, kind: str):
        """Writes a buildozer command that sets the kind of a target."""
        self.out_file.write(f"""set kind {kind}|{target}\n""")
        self.existing[TargetKey(target)] = TargetValue(kind=kind)

    def _add_comment(self, target: str, attribute: str, expected_comment: str,
                     should_set_comment_pred: Callable[[AttributeValue], bool] = lambda e: True):
        """Adds comment to attribute of the given target.

        If the attribute does not exist (assuming that it is queried
        with _get_all_info), it is set to None.
        """
        # comments can only be set to existing attributes. Set it to None if the
        # attribute does not already exist.
        self._set_attr(target, attribute, InfoValue.NONE, command="set_if_absent")

        attr_val = self._lookup_existing_attribute(target, attribute)
        if should_set_comment_pred(attr_val):
            logging.info(f"pred passes: {attr_val.comment}")
            if attr_val.comment is InfoValue.MISSING or \
                    expected_comment not in attr_val.comment:
                esc_comment = expected_comment.replace(" ", "\\ ")
                self.out_file.write(f"""comment {attribute} {esc_comment}|{target}\n""")
                attr_val.comment = expected_comment

    def _add_target_comment(self, target: str, comment_lines: Iterable[str]):
        """Adds comment to a given target."""

        # "comment" command on targets will override existing comments,
        # so there is no need to check existing comments.
        content = "\\n".join(comment_lines)
        content = content.replace(" ", "\\ ")
        if content:
            self.out_file.write(f"""comment {content}|{target}\n""")

    def _set_attr(self, target: str, attribute: str, value: Optional[bool | str],
                  quote: bool = False,
                  command: str = "set"):
        """Writes a buildozer command that sets an attribute.

        Args:
            target: full label of target
            attribute: attribute name
            value: value of attribute
            quote: whether value should be quoted in the buildozer command. By default, False.
            command: buildozer command. Either "set" or "set_if_absent". By default, "set".
        """
        if command not in ("set", "set_if_absent"):
            die(f"Unknown command {command} for _set_attr")

        command_value = f'"{value}"' if quote else str(value)
        self.out_file.write(f"""{command} {attribute} {command_value}|{target}\n""")

        # set value in self.existing
        key = AttributeKey(target, attribute)
        if key not in self.existing:
            self.existing[key] = AttributeValue()
        attr_val: AttributeValue = isinstance_or_die(self.existing[key], AttributeValue)
        if command == "set" or (command == "set_if_absent" and attr_val.value is InfoValue.MISSING):
            attr_val.value = str(value)

    def _add_attr(self, target: str, attribute: str, value: str, quote=False):
        """Writes a buildozer command that adds to an attribute.

        Args:
            target: full label of target
            attribute: attribute name
            value: value of attribute
            quote: if value should be quoted in the buildozer command
        """
        command_value = f'"{value}"' if quote else value
        self.out_file.write(f"""add {attribute} {command_value}|{target}\n""")

        # set value in self.existing
        key = AttributeKey(target, attribute)
        if key not in self.existing:
            self.existing[key] = AttributeValue()
        attr_val: AttributeValue = isinstance_or_die(self.existing[key], AttributeValue)
        if attr_val.value is InfoValue.MISSING:
            attr_val.value = [command_value]
        else:
            if isinstance(attr_val.value, list):
                attr_val.value.append(command_value)
            else:
                # This may be an expression that we can't parse. Just do something naive.
                attr_val.value += f" + [{command_value}]"

    def _rename(self, target: str, old_attr: str, new_attr: str):
        """Writes a buildozer command that renames an attribute.

        Args:
            target: full label of target
            old_attr: old attribute name
            new_attr: new attribute name
        """
        self.out_file.write(f"rename {old_attr} {new_attr}|{target}\n")

        old_key = AttributeKey(target, old_attr)
        new_key = AttributeKey(target, new_attr)

        # move value in self.existing
        if old_key not in self.existing:
            # This will fail when executing buildozer, but let buildozer
            # provide a detailed error message. Don't fail here.
            self.existing[old_key] = AttributeValue()
        self.existing[new_key] = self.existing[old_key]
        self.existing[old_key] = AttributeValue()

    def _create_extra_file(self, path: str, content: str):
        """Creates an extra file in the filesystem."""
        if self.args.stdout:
            logging.info(f"Dry-run: skipped creating file at {path}")
            return
        logging.info(f"Creating file at {path}")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(self._workspace_root() / path, "w") as f:
            f.write(content)

    def _run_buildozer(self) -> None:
        self.out_file.seek(0)
        logging.info("Executing buildozer with the following commands:\n%s", self.out_file.read())

        buildozer_args = [
            self.buildozer,
            "-shorten_labels",
            "-f",
            self.out_file.name,
        ]
        if self.args.keep_going:
            buildozer_args.append("-k")
        if self.args.stdout:
            buildozer_args.append("-stdout")
        try:
            subprocess.check_call(buildozer_args, stdout=self.stdout, stderr=self.stderr,
                                  env=self.environ, cwd=self._workspace_root())
        except subprocess.CalledProcessError as e:
            if e.returncode == _BUILDOZER_RETURN_CODE_NO_CHANGES_MADE:
                logging.info("No files were changed.")
            else:
                raise

    def _lookup_existing_target(self, target: str) -> TargetValue:
        return isinstance_or_die(self.existing[TargetKey(target)], TargetValue)

    def _lookup_existing_attribute(self, target: str, attribute: str) -> AttributeValue:
        return isinstance_or_die(self.existing[AttributeKey(target, attribute)], AttributeValue)

    def run(self):
        # Dry run to see what attributes / targets will be added
        self.existing = dict()
        with self:
            # This modifies self.existing
            self._create_buildozer_commands()
            # The buildozer command file is deleted.

        # self.existing.keys() = things we would change.
        # Get the existing information of these things in BUILD files
        self.existing = self._get_all_info(self.existing.keys())

        # Create another buildozer command file. This time, actually run buildozer with it.
        with self:
            self._create_buildozer_commands()
            self._run_buildozer()

    def _create_buildozer_commands(self):
        raise AttributeError

    def _workspace_root(self) -> pathlib.Path:
        return pathlib.Path(self.environ.get("BUILD_WORKSPACE_DIRECTORY", os.getcwd()))
