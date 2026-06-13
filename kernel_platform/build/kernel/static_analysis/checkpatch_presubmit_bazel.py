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

"""Runs necessary checkpatch targets for a build on ci.android.com.

usage:

tools/bazel run //build/kernel/static_analysis:checkpatch_presubmit -- \\
    --dist_dir <DIST_DIR> \\
    [<other flags to checkpatch>] \\
"""

import argparse
import collections
import logging
import os
import pathlib
import shlex
import subprocess
import sys
from typing import Any

_LOG_LEVEL = logging.INFO
# _LOG_LEVEL = logging.DEBUG

_BAZEL = pathlib.Path("tools/bazel")
_SILENT_ARGS = [
    "--ui_event_filters=-info",
    "--noshow_progress",
]

# TODO: Find a better way to handle this exceptions;
_PATH_PREFIX_DENY_LIST = (
    "external/",
)

def load_arguments() -> dict[str, Any]:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument(
        "--dist_dir",
        type=_resolve_against_workspace_root,
        required=True,
        help="DIST_DIR. If relative, resolve against workspace root.",
    )
    parser.add_argument(
        "--bid",
        help="Build ID. If specified, it is used to skip the check on post-submit.",
    )
    return parser.parse_known_args()


def _resolve_against_workspace_root(value: str) -> pathlib.Path:
    path = pathlib.Path(value)
    if path.is_absolute():
        return path
    return pathlib.Path(os.environ["BUILD_WORKSPACE_DIRECTORY"]) / path


def _log_command(args):
    quoted = [shlex.quote(str(arg)) for arg in args]
    logging.debug("Running command line: %s", " ".join(quoted))


def _find_checkpatch_targets(path: pathlib.Path) -> list[str]:
    if str(path).startswith(_PATH_PREFIX_DENY_LIST):
        logging.info("Skipped //%s path in deny list", path)
        return []

    if not _resolve_against_workspace_root(path / "BUILD.bazel").is_file() and \
        not _resolve_against_workspace_root(path / "BUILD").is_file():
        logging.info("//%s is not a package; no BUILD file is found", path)
        return []

    args = [_BAZEL, "query"]
    args += _SILENT_ARGS
    args.append(f'kind("^checkpatch rule$", //{path}:all)')
    _log_command(args)
    lines = subprocess.check_output(
        args,
        text=True,
        cwd=_resolve_against_workspace_root("."),
    ).splitlines()
    return [line.strip() for line in lines if line.strip()]


def _run_checkpatch(
    target: str,
    git_sha1: str,
    log: pathlib.Path,
    checkpatch_args: list[str],
    silent: bool = False,
) -> int:
    args = [_BAZEL, "run", "--show_result=0"]
    args += _SILENT_ARGS
    args += [target, "--"]
    args += checkpatch_args
    args += ["--log", log]
    args += ["--git_sha1", git_sha1]
    _log_command(args)
    return subprocess.run(
        args,
        text=True,
        cwd=_resolve_against_workspace_root("."),
        stdout=subprocess.DEVNULL if silent else None,
        stderr=subprocess.STDOUT if silent else None,
    ).returncode


def main(
        checkpatch_args: list[str],
        dist_dir: pathlib.Path,
        bid: str | None,
) -> int:
    if bid:
        # Skip checkpatch for postsubmit (b/35390488).
        if not bid.startswith("P"):
            logging.info("Did not identify a presubmit build. Exiting.")
            return 0
    applied_prop = dist_dir / "applied.prop"
    applied_prop_dict: dict[pathlib.Path, list[str]] = \
        collections.defaultdict(list)
    with open(applied_prop) as applied_prop_file:
        for line in applied_prop_file:
            line = line.strip()
            if not line:
                continue
            path, git_sha1 = line.split(maxsplit=2)
            applied_prop_dict[pathlib.Path(path)].append(git_sha1)

    targets: list[(list[str], str)] = []
    for path, git_sha1_list in applied_prop_dict.items():
        if len(git_sha1_list) > 1:
            logging.error("Multiple git sha1 found in %s for %s",
                          applied_prop, path)
            return 1
        path_targets = _find_checkpatch_targets(path)
        if not path_targets:
            logging.info(
                "Skipping %s because no checkpatch() target is found.", path)
            continue
        targets.append((path_targets, git_sha1_list[0]))

    checkpatch_log = dist_dir / "checkpatch.log"
    checkpatch_full_log = dist_dir / "checkpatch_full.log"
    if checkpatch_log.exists():
        os.unlink(checkpatch_log)
    if checkpatch_full_log.exists():
        os.unlink(checkpatch_full_log)
    return_codes = []
    for path_targets, git_sha1 in targets:
        for target in path_targets:
            return_codes.append(_run_checkpatch(
                target=target,
                git_sha1=git_sha1,
                log=checkpatch_log,
                checkpatch_args=checkpatch_args,
            ))
            _run_checkpatch(
                target=target,
                git_sha1=git_sha1,
                log=checkpatch_full_log,
                checkpatch_args=checkpatch_args + ["--ignored_checks", ""],
                silent=True,
            )

    success = sum(return_codes) == 0

    if not success:
        logging.info("See %s for complete output.", checkpatch_log.name)

    return 0 if success else 1


if __name__ == "__main__":
    logging.basicConfig(level=_LOG_LEVEL,
                        format="%(levelname)s: %(message)s")
    known, checkpatch_args = load_arguments()
    sys.exit(main(checkpatch_args=checkpatch_args, **vars(known)))
