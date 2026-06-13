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

import dataclasses
import json
import logging
import os
import pathlib
import shutil
import subprocess
import sys
import xml.dom.minidom
import xml.parsers.expat

_FAKE_KERNEL_VERSION = "99.99.99"


@dataclasses.dataclass
class PathCollectible(object):
    """Represents a path and the result of an asynchronous task."""
    path: pathlib.Path

    def collect(self) -> str:
        raise NotImplementedError


@dataclasses.dataclass
class PathPopen(PathCollectible):
    """Consists of a path and the result of a subprocess."""
    popen: subprocess.Popen

    def collect(self) -> str:
        return collect(self.popen)


@dataclasses.dataclass
class PresetResult(PathCollectible):
    """Consists of a path and a pre-defined result."""
    result: str

    def collect(self) -> str:
        return self.result


@dataclasses.dataclass
class LocalversionResult(PathPopen):
    """Consists of results of localversion."""
    removed_prefix: str | None
    suffix: str | None

    def collect(self) -> str:
        ret = super().collect()
        if self.removed_prefix:
            ret = ret.removeprefix(self.removed_prefix)
        if self.suffix:
            ret += self.suffix
        return ret


def get_localversion_from_script(bin: pathlib.Path | None, project: pathlib.Path, *args) \
        -> PathCollectible | None:
    """Call setlocalversion.

    Args:
      bin: path to setlocalversion, or None if it does not exist.
      project: relative path to the project
      args: additional arguments
    Return:
      A PathCollectible object that resolves to the result, or None if bin or
      project does not exist.
    """
    if not project.is_dir():
        return None
    srctree = project.resolve()

    if bin:
        working_dir = "build/kernel/kleaf/workspace_status_dir"
        env = dict(os.environ)
        env["KERNELVERSION"] = _FAKE_KERNEL_VERSION
        env.pop("BUILD_NUMBER", None)
        popen = subprocess.Popen([bin, srctree] + list(args),
                                 text=True,
                                 stdout=subprocess.PIPE,
                                 cwd=working_dir,
                                 env=env)

        suffix = None
        if os.environ.get("BUILD_NUMBER"):
            suffix = "-ab" + os.environ["BUILD_NUMBER"]
        return LocalversionResult(
            path=project,
            popen=popen,
            removed_prefix=_FAKE_KERNEL_VERSION,
            suffix=suffix
        )

    return None


def get_localversion_from_git(project: pathlib.Path) -> PathCollectible | None:
    """Calculate localversion without calling setlocalversion script.

    Args:
      project: relative path to the project
    Return:
      A PathCollectible object that resolves to the result, or None if bin or
      project does not exist.
    """

    if not project.is_dir():
        return None

    # Note: To ensure hermeticity as much as possible, only get git from
    # host, then clear PATH.
    script = """
        GIT=$(command -v git)
        PATH=
        if head=$($GIT rev-parse --verify --short=12 HEAD 2>/dev/null); then
            echo -n -g"$head"
        fi
        if {
            $GIT --no-optional-locks status -uno --porcelain 2>/dev/null ||
            $GIT diff-index --name-only HEAD
        } | read placeholder; then
            echo -n -dirty
        fi
    """
    popen = subprocess.Popen(script, shell=True, text=True,
                             stdout=subprocess.PIPE, cwd=project)
    suffix = None
    if os.environ.get("BUILD_NUMBER"):
        suffix = "-ab" + os.environ["BUILD_NUMBER"]
    return LocalversionResult(
        path=project,
        popen=popen,
        removed_prefix=None,
        suffix=suffix
    )


def _find_repo(curdir: pathlib.Path) -> pathlib.Path | None:
    """Find repo installation."""
    while curdir.parent != curdir:  # is not root
        maybe_dot_repo = curdir / ".repo"
        if maybe_dot_repo.is_dir():
            return curdir
        curdir = curdir.parent
    return None


def list_projects() -> list[pathlib.Path]:
    """Lists projects in the repository.

    Returns:
        a list of Git projects relative to CWD.
    """
    repo_root_s, repo_manifest = os.environ.get("KLEAF_REPO_MANIFEST", ":").split(":")
    if repo_root_s:
        repo_root = pathlib.Path(repo_root_s)
    else:
        repo_root = _find_repo(pathlib.Path(".").resolve())

    if not repo_root:
        logging.warning("Unable to determine repo root. Please specify --repo_manifest.")
        return []

    if repo_manifest:
        with open(repo_manifest) as repo_manifest_file:
            return parse_repo_manifest(repo_root, repo_manifest_file.read())

    try:
        output = subprocess.check_output(["repo", "manifest", "-r"], text=True)
        return parse_repo_manifest(repo_root, output)
    except (subprocess.SubprocessError, FileNotFoundError) as e:
        logging.warning("Unable to execute repo manifest -r: %s", e)
        return []


def parse_repo_manifest(repo_root: pathlib.Path, manifest: str) \
        -> list[pathlib.Path]:
    """Parses a repo manifest file.

    Returns:
        a list of paths to all projects in the repository.
    """
    kleaf_repo_dir = pathlib.Path(".").resolve()
    try:
        dom = xml.dom.minidom.parseString(manifest)
    except xml.parsers.expat.ExpatError as e:
        logging.error("Unable to parse repo manifest: %s", e)
        return []
    projects = dom.documentElement.getElementsByTagName("project")
    ret = list[pathlib.Path]()
    for project in projects:
        # https://gerrit.googlesource.com/git-repo/+/master/docs/manifest-format.md#element-project
        path_below_repo = pathlib.Path(project.getAttribute("path") or
                                       project.getAttribute("name"))
        realpath = repo_root / path_below_repo
        if realpath.is_relative_to(kleaf_repo_dir):
            ret.append(realpath.relative_to(kleaf_repo_dir))
        else:
            logging.warning("Skipping project %s because it is not below %s",
                            realpath, kleaf_repo_dir)
    return ret


def collect(popen_obj: subprocess.Popen) -> str:
    """Collect the result of a Popen object.

    Terminates the program if return code is non-zero.

    Return:
      stdout of the subprocess.
    """
    stdout, _ = popen_obj.communicate()
    if popen_obj.returncode != 0:
        logging.error("return code is %d", popen_obj.returncode)
        sys.exit(1)
    return stdout.strip()


class Stamp(object):

    def __init__(self):
        self.ignore_missing_projects = os.environ.get(
            "KLEAF_IGNORE_MISSING_PROJECTS") == "true"
        self.use_kleaf_localversion = os.environ.get(
            "KLEAF_USE_KLEAF_LOCALVERSION") == "true"

        self.projects = list_projects()
        extra_git_project_env_var = os.environ.get("KLEAF_EXTRA_GIT_PROJECTS")
        if extra_git_project_env_var:
            self.projects.extend(pathlib.Path(value) for value in
                                 extra_git_project_env_var.split(":"))

        self.init_for_dot_source_date_epoch_dir()

    def init_for_dot_source_date_epoch_dir(self) -> None:
        self.kernel_dir = pathlib.Path(".source_date_epoch_dir").resolve()
        if not self.kernel_dir.is_dir():
            self.kernel_dir = None
        if self.kernel_dir:
            self.kernel_rel = self.kernel_dir.relative_to(
                pathlib.Path(".").resolve())

        self.find_setlocalversion()

    def main(self) -> int:
        scmversion_map = self.get_localversion_all()

        source_date_epoch_map = self.async_get_source_date_epoch_all()

        scmversion_result_map = self.collect_map(scmversion_map)

        source_date_epoch_result_map = self.collect_map(source_date_epoch_map)

        self.print_result(
            scmversion_result_map=scmversion_result_map,
            source_date_epoch_result_map=source_date_epoch_result_map,
        )
        return 0

    def find_setlocalversion(self) -> None:
        self.setlocalversion = None

        if self.use_kleaf_localversion:
            return

        all_projects = []
        if self.kernel_dir:
            all_projects.append(self.kernel_rel)
        all_projects.extend(self.projects)

        if self.ignore_missing_projects:
            all_projects = filter(pathlib.Path.is_dir, all_projects)

        for proj in all_projects:
            if not proj.is_dir():
                logging.error(
                    "Project %s in repo manifest does not exist on disk.",
                    proj
                )
                sys.exit(1)

            candidate = proj / "scripts/setlocalversion"
            if os.access(candidate, os.X_OK):
                self.setlocalversion = candidate.resolve()
                return

    def get_localversion_all(self) -> dict[pathlib.Path, PathCollectible]:
        all_projects: set[pathlib.Path] = set()
        if self.kernel_dir:
            all_projects.add(self.kernel_rel)
        all_projects |= set(self.get_ext_modules())
        all_projects |= set(self.projects)

        if self.ignore_missing_projects:
            all_projects = filter(pathlib.Path.is_dir, all_projects)

        scmversion_map = {}
        for project in all_projects:
            if not project.is_dir():
                logging.error(
                    "Project %s in repo manifest does not exist on disk.",
                    project)
                sys.exit(1)

            path_popen = self.get_localversion(project)
            if path_popen:
                scmversion_map[project] = path_popen

        return scmversion_map

    def get_localversion(self, project: pathlib.Path) -> PathCollectible | None:
        if not self.use_kleaf_localversion:
            return get_localversion_from_script(self.setlocalversion, project)

        return get_localversion_from_git(project)

    def get_ext_modules(self) -> list[pathlib.Path]:
        if not self.setlocalversion:
            return []
        try:
            cmd = """
                    source build/build_utils.sh
                    source build/_setup_env.sh
                    echo $EXT_MODULES
                  """
            out = subprocess.check_output(cmd,
                                          shell=True,
                                          text=True,
                                          stderr=subprocess.PIPE,
                                          executable="/bin/bash")
            return [pathlib.Path(path) for path in out.split()]
        except subprocess.CalledProcessError as e:
            logging.warning(
                "Unable to determine EXT_MODULES; scmversion "
                "for external modules may be incorrect. "
                "code=%d, stderr=%s", e.returncode, e.stderr.strip())
        return []

    def async_get_source_date_epoch_all(self) \
            -> dict[str, PathCollectible]:

        all_projects: set[pathlib.Path] = set()
        if self.kernel_dir:
            all_projects.add(self.kernel_rel)
        all_projects |= set(self.projects)

        if self.ignore_missing_projects:
            all_projects = filter(pathlib.Path.is_dir, all_projects)

        return {
            proj: self.async_get_source_date_epoch(proj)
            for proj in filter(os.path.exists, all_projects)
        }

    def async_get_source_date_epoch(self, rel_path: pathlib.Path) -> PathCollectible:
        env_val = os.environ.get("SOURCE_DATE_EPOCH")
        if env_val:
            return PresetResult(rel_path, env_val)
        if shutil.which("git"):
            args = [
                "git", "-C",
                rel_path.resolve(), "log", "-1", "--pretty=%ct"
            ]
            popen = subprocess.Popen(args, text=True, stdout=subprocess.PIPE)
            return PathPopen(rel_path, popen)
        return PresetResult(rel_path, "0")

    def collect_map(
        self,
        legacy_map: dict[pathlib.Path, PathCollectible],
    ) -> dict[pathlib.Path, str]:
        return {
            path: path_popen.collect()
            for path, path_popen in legacy_map.items()
        }

    def print_result(
        self,
        scmversion_result_map: dict[pathlib.Path, str],
        source_date_epoch_result_map: dict[pathlib.Path, str],
    ) -> None:
        stable_source_date_epochs = json.dumps({
            str(key): value for key, value in source_date_epoch_result_map.items()
        }, sort_keys=True)
        print("STABLE_SOURCE_DATE_EPOCHS", stable_source_date_epochs)

        # If the list is empty, this prints "STABLE_SCMVERSIONS", and is
        # filtered by Bazel.
        stable_scmversions = json.dumps({
            str(key): value for key, value in scmversion_result_map.items()
        }, sort_keys=True)
        print("STABLE_SCMVERSIONS", stable_scmversions)


if __name__ == '__main__':
    logging.basicConfig(stream=sys.stderr,
                        level=logging.WARNING,
                        format="%(levelname)s: %(message)s")
    sys.exit(Stamp().main())
