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
"""Integration tests for Kleaf.

The rest of the arguments are passed to absltest.

Example:

    tools/bazel run //build/kernel/kleaf/tests/integration_test

    tools/bazel run //build/kernel/kleaf/tests/integration_test \\
      -- --bazel-arg=--verbose_failures --bazel-arg=--announce_rc

    tools/bazel run //build/kernel/kleaf/tests/integration_test \\
      -- QuickIntegrationTest.test_menuconfig_merge

    tools/bazel run //build/kernel/kleaf/tests/integration_test \\
      -- --bazel-arg=--verbose_failures --bazel-arg=--announce_rc \\
         QuickIntegrationTest.test_menuconfig_merge \\
         --verbosity=2

    tools/bazel run //build/kernel/kleaf/tests/integration_test \\
      -- --bazel-arg=--verbose_failures --include-abi-tests \\
      KleafIntegrationTestAbiTest.test_non_exported_symbol_fails
"""

import argparse
import collections
import contextlib
import dataclasses
import hashlib
import io
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import pathlib
import tempfile
import textwrap
import unittest
import xml.dom.minidom
from typing import Any, Callable, Iterable, TextIO

from absl.testing import absltest
from build.kernel.kleaf.analysis.inputs import analyze_inputs

_BAZEL = pathlib.Path("tools/bazel")

# See local.bazelrc
_LOCAL = ["--//build/kernel/kleaf:config_local"]

_LTO_NONE = [
    "--lto=none",
    "--nokmi_symbol_list_strict_mode",
]

# Handy arguments to build as fast as possible.
_FASTEST = _LOCAL + _LTO_NONE


def load_arguments():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--bazel-arg",
                        action="append",
                        dest="bazel_args",
                        default=[],
                        help="arg to bazel build calls")
    parser.add_argument("--bazel-wrapper-arg",
                        action="append",
                        dest="bazel_wrapper_args",
                        default=[],
                        help="arg to bazel.py wrapper")
    parser.add_argument("--include-abi-tests",
                        action="store_true",
                        dest="include_abi_tests",
                        help="Include ABI Monitoring related tests." +
                        "NOTE: It requires a branch with ABI monitoring enabled.")
    parser.add_argument("--mount-spec",
                        type=_deserialize_mount_spec,
                        help="""A JSON dictionary specifying bind mounts.

                            If not set, some tests will re-run itself
                            in an unshare-d namespace with the flag set.""",
                        default=MountSpec())
    parser.add_argument("--link-spec",
                        type=_deserialize_link_spec,
                        help="""A JSON dictionary specifying symlinks.

                            If not set, some tests will re-run itself
                            with the flag set.""",
                        default=LinkSpec())
    group = parser.add_argument_group("CI", "flags for ci.android.com")
    group.add_argument("--test_result_dir",
                       type=_require_absolute_path,
                       help="""Directory to store test results to be used in :reporter.

                            If set, this script always has exit code 0.
                       """)
    return parser.parse_known_args()


arguments = None


def _require_absolute_path(p: str) -> pathlib.Path:
    path = pathlib.Path(p)
    if not path.is_absolute():
        raise ValueError(f"{p} is not absolute")
    return path


MountSpec = collections.OrderedDict[pathlib.Path, pathlib.Path]


def _serialize_mount_spec(val: MountSpec) -> str:
    return json.dumps([[str(key), str(value)] for key, value in val.items()])


def _deserialize_mount_spec(s: str) -> MountSpec:
    return MountSpec((pathlib.Path(key), pathlib.Path(value))
                     for key, value in json.loads(s))


@dataclasses.dataclass
class Link:
    # Value in repo manifest. Relative against repo root.
    dest: pathlib.Path

    # Unlike the value in repo manifest, this is relative against repo root
    # not project path.
    src: pathlib.Path

    @classmethod
    def from_element(cls, element: xml.dom.minidom.Element,
                     project_path: pathlib.Path) -> "Link":
        return cls(dest=pathlib.Path(element.getAttribute("dest")),
                   src=project_path / element.getAttribute("src"))


LinkSpec = list[Link]


def _serialize_link_spec(links: LinkSpec) -> str:
    return json.dumps([{"dest": str(link.dest), "src": str(link.src)}
                       for link in links])


def _deserialize_link_spec(s: str) -> LinkSpec:
    return [Link(dest=pathlib.Path(obj["dest"]), src=pathlib.Path(obj["src"]))
            for obj in json.loads(s)]


@dataclasses.dataclass
class RepoProject:
    # Project path
    path: pathlib.Path

    # List of symlinks to create
    links: list[Link] = dataclasses.field(default_factory=list)

    # List of groups
    groups: list[str] = dataclasses.field(default_factory=list)

    @classmethod
    def from_element(cls, element: xml.dom.minidom.Element) -> "RepoProject":
        path = pathlib.Path(
                element.getAttribute("path") or element.getAttribute("name"))
        project = cls(path=path)
        for link_element in element.getElementsByTagName("linkfile"):
            project.links.append(Link.from_element(link_element, path))
        project.groups = re.split(r",| ", element.getAttribute("groups"))
        return project


class Exec(object):

    @staticmethod
    def check_call(args: list[str], **kwargs) -> None:
        """Executes a shell command."""
        kwargs.setdefault("text", True)
        sys.stderr.write(f"+ {' '.join(args)}\n")
        subprocess.check_call(args, **kwargs)

    @staticmethod
    def call(args: list[str], **kwargs) -> None:
        """Executes a shell command."""
        kwargs.setdefault("text", True)
        sys.stderr.write(f"+ {' '.join(args)}\n")
        subprocess.call(args, **kwargs)

    @staticmethod
    def check_output(args: list[str], **kwargs) -> str:
        """Returns output of a shell command"""
        kwargs.setdefault("text", True)
        sys.stderr.write(f"+ {' '.join(args)}\n")
        return subprocess.check_output(args, **kwargs)

    @staticmethod
    def popen(args: list[str], **kwargs) -> subprocess.Popen:
        """Executes a shell command.

        Returns:
            the Popen object
        """
        kwargs.setdefault("text", True)
        sys.stderr.write(f"+ {' '.join(args)}\n")
        popen = subprocess.Popen(args, **kwargs)
        return popen

    @staticmethod
    def check_errors(args: list[str], **kwargs) -> str:
        """Returns errors of a shell command"""
        kwargs.setdefault("text", True)
        sys.stderr.write(f"+ {' '.join(args)}\n")
        return subprocess.run(
            args, check=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs).stdout


class KleafIntegrationTestBase(unittest.TestCase):

    def _build_subprocess_args(
        self,
        command: str,
        command_args: Iterable[str] = (),
        use_bazelrc=True,
        startup_options=(),
        use_wrapper_args=True,
        **kwargs,
    ) -> tuple[list[str], dict[str, Any]]:
        """Builds subprocess arguments."""
        subprocess_args = [str(_BAZEL)]
        subprocess_args.extend(startup_options)
        if use_bazelrc:
            subprocess_args.append(f"--bazelrc={self._bazel_rc.name}")
        subprocess_args.append(command)

        if use_wrapper_args:
            subprocess_args.extend(arguments.bazel_wrapper_args)
        subprocess_args.extend(command_args)

        # kwargs has known arguments filtered out.
        return subprocess_args, kwargs

    def _check_call(self, *args, **kwargs) -> None:
        """Executes a bazel command."""
        subprocess_args, kwargs = self._build_subprocess_args(*args, **kwargs)
        Exec.check_call(subprocess_args, **kwargs)

    def _build(self, *args, **kwargs) -> None:
        """Executes a bazel build command."""
        self._check_call("build", *args, **kwargs)

    def _check_output(self, *args, **kwargs) -> str:
        """Returns output of a bazel command."""
        subprocess_args, kwargs = self._build_subprocess_args(*args, **kwargs)
        return Exec.check_output(subprocess_args, **kwargs)

    def _check_errors(self, *args, **kwargs) -> str:
        """Returns errors of a bazel command."""
        subprocess_args, kwargs = self._build_subprocess_args(*args, **kwargs)
        return Exec.check_errors(subprocess_args, **kwargs)

    def _popen(self, *args, **kwargs) -> subprocess.Popen:
        """Executes a bazel command, returning the Popen object."""
        subprocess_args, kwargs = self._build_subprocess_args(*args, **kwargs)
        return Exec.popen(subprocess_args, **kwargs)

    def setUp(self) -> None:
        self.assertTrue(os.environ.get("BUILD_WORKSPACE_DIRECTORY"),
                        "BUILD_WORKSPACE_DIRECTORY is not set. " +
                        "Did you use `tools/bazel test` instead of `tools/bazel run`?")
        os.chdir(os.environ["BUILD_WORKSPACE_DIRECTORY"])
        sys.stderr.write(
            f"BUILD_WORKSPACE_DIRECTORY={os.environ['BUILD_WORKSPACE_DIRECTORY']}\n"
        )

        self.assertTrue(_BAZEL.is_file())

        self._bazel_rc = tempfile.NamedTemporaryFile()
        self.addCleanup(self._bazel_rc.close)
        with open(self._bazel_rc.name, "w") as f:
            for arg in arguments.bazel_args:
                f.write(f"build {shlex.quote(arg)}\n")

    def restore_file_after_test(self, path: pathlib.Path | str):
        with open(path) as file:
            old_content = file.read()

        def cleanup():
            with open(path, "w") as new_file:
                new_file.write(old_content)

        self.addCleanup(cleanup)
        return cleanup

    def filter_lines(
        self,
        path: pathlib.Path | str,
        pred: Callable[[str], bool],
    ):
        """Filters lines in a file."""
        output_file_obj = tempfile.NamedTemporaryFile(mode="w", delete=False)
        with open(path) as input_file:
            with output_file_obj as output_file:
                for line in input_file:
                    if pred(line):
                        output_file.write(line)
        shutil.move(output_file.name, path)

    def replace_lines(
        self,
        path: pathlib.Path | str,
        pred: Callable[[str], bool],
        replacements: Iterable[str],
    ):
        """Replaces lines in a file."""
        output_file_obj = tempfile.NamedTemporaryFile(mode="w", delete=False)
        it = iter(replacements)
        with open(path) as input_file:
            with output_file_obj as output_file:
                for line in input_file:
                    if pred(line):
                        replaced_line = next(it)
                        output_file.write(replaced_line)
                        if not replaced_line.endswith("\n"):
                            output_file.write("\n")
                    else:
                        output_file.write(line)
        shutil.move(output_file.name, path)

    def _sha256(self, path: pathlib.Path | str) -> str:
        """Gets the hash for a file."""
        hash = hashlib.sha256()
        with open(path, "rb") as file:
            chunk = None
            while chunk != b'':
                chunk = file.read(4096)
                hash.update(chunk)
        return hash.hexdigest()

    def _touch(self, path: pathlib.Path | str, append_text="\n") -> None:
        """Modifies a file so it (may) trigger a rebuild for certain targets."""
        self.restore_file_after_test(path)

        with open(path, "a") as file:
            file.write(append_text)

    def _touch_core_kernel_file(self):
        """Modifies a core kernel file."""
        self._touch(f"{self._common()}/kernel/sched/core.c")

    def _common(self) -> str:
        """Returns the common package."""
        return "common"

    def _mount(self, kleaf_repo: pathlib.Path):
        """Mount according to --mount-spec"""
        for from_path, to_path in arguments.mount_spec.items():
            to_path.mkdir(parents=True, exist_ok=True)
            Exec.check_call([shutil.which("mount"), "--bind", "-o", "ro",
                            str(from_path), str(to_path)])
            self.addCleanup(Exec.call,
                            [shutil.which("umount"), str(to_path)])

        for link in arguments.link_spec:
            real_dest = kleaf_repo / link.dest
            real_src = kleaf_repo / link.src
            relative_src = self._force_relative_to(real_src, real_dest.parent)
            real_dest.parent.mkdir(parents=True, exist_ok=True)
            # Ignore symlinks inside projects that already exist in the tree
            # e.g. common/patches
            if not real_dest.is_symlink():
                real_dest.symlink_to(relative_src)

    @staticmethod
    def _force_relative_to(path: pathlib.Path, other: pathlib.Path):
        """Naive implementation of pathlib.Path.relative_to(walk_up)"""
        if sys.version_info[0] == 3 and sys.version_info[1] >= 12:
            return path.relative_to(other, walk_up=True)

        path = path.resolve()
        other = other.resolve()

        if path.is_relative_to(other):
            return path.relative_to(other)

        path_parts = collections.deque(path.parts)
        other_parts = collections.deque(other.parts)
        while path_parts and other_parts and path_parts[0] == other_parts[0]:
            path_parts.popleft()
            other_parts.popleft()
        parts = [".."] * len(other_parts) + list(path_parts)
        return pathlib.Path(*parts)

    def _check_repo_manifest(self, value: str) \
            -> tuple[pathlib.Path | None, pathlib.Path | None]:
        tokens = value.split(":")
        match len(tokens):
            case 0: return (None, None)
            case 1:
                return (pathlib.Path(".").resolve(),
                        _require_absolute_path(value))
            case 2:
                repo_root, repo_manifest = tokens
                return (_require_absolute_path(repo_root),
                        _require_absolute_path(repo_manifest))
        raise argparse.ArgumentTypeError(
            "Must be <REPO_MANIFEST> or <REPO_ROOT>:<REPO_MANIFEST>"
        )

    def _get_repo_manifest(self, revision=False) -> xml.dom.minidom.Document:
        parser = argparse.ArgumentParser()
        parser.add_argument("--repo_manifest",
                            type=self._check_repo_manifest,
                            default=(None, None))
        known, _ = parser.parse_known_args(arguments.bazel_wrapper_args)
        _, repo_manifest = known.repo_manifest
        if repo_manifest:
            with open(repo_manifest) as file:
                return xml.dom.minidom.parse(file)
        args = ["repo", "manifest"]
        if revision:
            args.append("-r")
        manifest_content = Exec.check_output(args)
        return xml.dom.minidom.parseString(manifest_content)

    def _get_projects(self) -> list[RepoProject]:
        manifest_element = self._get_repo_manifest().documentElement
        project_elements = manifest_element.getElementsByTagName("project")
        return [RepoProject.from_element(element)
                for element in project_elements]

    def _get_project_mount_link_spec(self, mount_root: pathlib.Path,
                                     groups: list[str]) \
        -> tuple[MountSpec, LinkSpec]:
        """Returns MountSpec / LinkSpec for projects that matches any group.

        Args:
            groups: List of groups to match projects. Projects with the `groups`
                attribute matching any of the given |groups| are included.
                If empty, all projects are included.
            mount_root: The root of the mount point where projects will be
                mounted below.
        """

        projects = self._get_projects()

        relevant_projects = list[RepoProject]()
        for project in projects:
            if not groups or any(group in project.groups for group in groups):
                relevant_projects.append(project)

        src_mount_root = pathlib.Path(".").resolve()
        mount_spec = MountSpec()
        link_spec = LinkSpec()
        for project in relevant_projects:
            mount_spec[src_mount_root / project.path] = \
                mount_root / project.path
            link_spec.extend(project.links)

        return mount_spec, link_spec

    def _unshare_mount_run(self, mount_spec: MountSpec, link_spec: list[Link]):
        """Reruns the test in an unshare-d namespace with --mount-spec set."""
        args = [shutil.which("unshare"), "--mount", "--map-root-user"]

        # toybox unshare -R does not imply -U, so explicitly say so.
        args.append("--user")

        args.extend([shutil.which("bash"), "-c"])

        test_args = [sys.executable, __file__]
        test_args.extend(f"--bazel-arg={arg}" for arg in arguments.bazel_args)
        test_args.extend(f"--bazel-wrapper-arg={arg}"
                         for arg in arguments.bazel_wrapper_args)
        test_args.append(f"--mount-spec={_serialize_mount_spec(mount_spec)}")
        test_args.append(f"--link-spec={_serialize_link_spec(link_spec)}")
        test_args.append(self.id().removeprefix("__main__."))
        args.append(" ".join(shlex.quote(str(test_arg))
                             for test_arg in test_args))

        Exec.check_call(args)


# NOTE: It requires a branch with ABI monitoring enabled.
#   Include these using the flag --include-abi-tests
class KleafIntegrationTestAbiTest(KleafIntegrationTestBase):

    def test_non_exported_symbol_fails(self):
        """Tests the following:

        - Validates a non-exported symbol makes the build fail.
          For this particular example use db845c mixed build.

        This test requires a branch with ABI monitoring enabled.
        """

        if not arguments.include_abi_tests:
            self.skipTest("--include-abi-tests is not set.")

        # Select an arbitrary driver and unexport a symbols.
        self.driver_file = f"{self._common()}/drivers/i2c/i2c-core-base.c"
        self.restore_file_after_test(self.driver_file)
        self.replace_lines(self.driver_file,
                           lambda x: re.search(
                               "EXPORT_SYMBOL_GPL\(i2c_adapter_type\);", x),
                           [""])

        # Check for errors in the logs.
        output = self._check_errors(
            "build", [f"//{self._common()}:db845c", "--config=fast"])

        def matching_line(line): return re.match(
            r"^ERROR: modpost: \"i2c_adapter_type\" \[.*\] undefined!$",
            line)
        self.assertTrue(
            any([matching_line(line) for line in output.splitlines()]))


# Slow integration tests belong to their own shard.
class KleafIntegrationTestShard1(KleafIntegrationTestBase):

    def test_incremental_switch_local_and_lto(self):
        """Tests the following:

        - switching from non-local to local and back works
        - with --config=local, changing from --lto=none to --lto=thin and back works

        See b/257288175."""
        self._build([f"//{self._common()}:kernel_dist"] + _LTO_NONE + _LOCAL)
        self._build([f"//{self._common()}:kernel_dist"] + _LTO_NONE)
        self._build([f"//{self._common()}:kernel_dist"] + _LTO_NONE + _LOCAL)
        self._build([f"//{self._common()}:kernel_dist"] +
                    ["--lto=thin"] + _LOCAL)
        self._build([f"//{self._common()}:kernel_dist"] + _LTO_NONE + _LOCAL)

    def test_config_sync(self):
        """Test that, with --config=local, .config is reflected in vmlinux.

        See b/312268956.
        """

        gki_defconfig_path = (
            f"{self._common()}/arch/arm64/configs/gki_defconfig")
        restore_defconfig = self.restore_file_after_test(gki_defconfig_path)
        extract_ikconfig = f"{self._common()}/scripts/extract-ikconfig"

        with open(gki_defconfig_path, encoding="utf-8") as f:
            self.assertIn("CONFIG_UAPI_HEADER_TEST=y\n", f)

        self._build([f"//{self._common()}:kernel_aarch64", "--config=fast"])
        vmlinux = pathlib.Path(
            f"bazel-bin/{self._common()}/kernel_aarch64/vmlinux")

        output = subprocess.check_output([extract_ikconfig, vmlinux], text=True)
        self.assertIn("CONFIG_UAPI_HEADER_TEST=y", output.splitlines())

        self.filter_lines(gki_defconfig_path,
                          lambda x: "CONFIG_UAPI_HEADER_TEST" not in x)
        self._build([f"//{self._common()}:kernel_aarch64", "--config=fast"])

        output = subprocess.check_output([extract_ikconfig, vmlinux], text=True)
        self.assertIn("# CONFIG_UAPI_HEADER_TEST is not set",
                      output.splitlines())

        restore_defconfig()
        self._build([f"//{self._common()}:kernel_aarch64", "--config=fast"])

        output = subprocess.check_output([extract_ikconfig, vmlinux], text=True)
        self.assertIn("CONFIG_UAPI_HEADER_TEST=y", output.splitlines())


class KleafIntegrationTestShard2(KleafIntegrationTestBase):

    def test_user_clang_toolchain(self):
        """Test --user_clang_toolchain option."""

        clang_version = None
        build_config_constants = f"{self._common()}/build.config.constants"
        with open(build_config_constants) as f:
            for line in f.read().splitlines():
                if line.startswith("CLANG_VERSION="):
                    clang_version = line.strip().split("=", 2)[1]
        self.assertIsNotNone(clang_version)
        clang_dir = f"prebuilts/clang/host/linux-x86/clang-{clang_version}"
        clang_dir = os.path.realpath(clang_dir)

        # Do not use --config=local to ensure the toolchain dependency is
        # correct.
        args = [
            f"--user_clang_toolchain={clang_dir}",
            f"//{self._common()}:kernel",
        ] + _LTO_NONE
        self._build(args)


class DdkWorkspaceSetupTest(KleafIntegrationTestBase):
    """Tests setting up a DDK workspace with @kleaf as dependency."""

    def setUp(self):
        super().setUp()

        self.real_kleaf_repo = pathlib.Path(".").resolve()
        self.ddk_workspace = (pathlib.Path(__file__).resolve().parent /
                              "ddk_workspace_test")

    def test_ddk_workspace_below_kleaf_module(self):
        """Tests that DDK workspace is below @kleaf"""
        self._run_ddk_workspace_setup_test(
            self.real_kleaf_repo, self.ddk_workspace)

    def test_kleaf_module_below_ddk_workspace(self):
        """Tests that @kelaf is below DDK workspace"""
        kleaf_repo = self.ddk_workspace / "external/kleaf"
        if not arguments.mount_spec:
            mount_spec = {
                self.real_kleaf_repo: kleaf_repo
            }
            self._unshare_mount_run(mount_spec=mount_spec, link_spec=LinkSpec())
            return
        self._run_ddk_workspace_setup_test(kleaf_repo, self.ddk_workspace)

    def test_setup_with_local_prebuilts(self):
        """Tests that init_ddk --prebuilts_dir & --local works."""
        if not arguments.mount_spec:
            with tempfile.TemporaryDirectory() as ddk_workspace_tmp:
                ddk_workspace = pathlib.Path(ddk_workspace_tmp)
                kleaf_repo = ddk_workspace / "external/kleaf"
                prebuilts_dir = ddk_workspace / "gki_prebuilts"
                mount_spec, link_spec = self._get_project_mount_link_spec(
                    kleaf_repo, groups=[],
                )
                mount_spec = MountSpec({
                    self.ddk_workspace: ddk_workspace,
                    self.real_kleaf_repo / "out/kernel_aarch64/dist":
                        prebuilts_dir,
                }) | mount_spec

                self._unshare_mount_run(mount_spec=mount_spec,
                                        link_spec=link_spec)
                return

        self._check_call("run", [f"//{self._common()}:kernel_aarch64_dist"])

        # Restore value of ddk_workspace in child process, which is a tmp dir
        ddk_workspace = arguments.mount_spec[self.ddk_workspace]
        kleaf_repo = ddk_workspace / "external/kleaf"
        prebuilts_dir = ddk_workspace / "gki_prebuilts"

        self._run_ddk_workspace_setup_test(
            kleaf_repo,
            ddk_workspace=ddk_workspace,
            prebuilts_dir=prebuilts_dir,
            local=True)

    def test_setup_with_downloaded_prebuilts(self):
        """Tests that init_ddk --prebuilts_dir & --local=false works."""
        if not arguments.mount_spec:
            with tempfile.TemporaryDirectory() as ddk_workspace_tmp:
                ddk_workspace = pathlib.Path(ddk_workspace_tmp)
                kleaf_repo = ddk_workspace / "external/kleaf"

                # Mount the projects ourselves to simulate --sync.
                mount_spec_ddk, link_spec_ddk = (
                    self._get_project_mount_link_spec(kleaf_repo, ["ddk"]))
                mount_spec_external, link_spec_external = (
                    self._get_project_mount_link_spec(ddk_workspace,
                                                      ["ddk-external"]))
                mount_spec = MountSpec({
                    self.ddk_workspace: ddk_workspace
                }) | mount_spec_ddk | mount_spec_external
                link_spec = link_spec_ddk + link_spec_external

                self._unshare_mount_run(mount_spec=mount_spec,
                                        link_spec=link_spec)
                return

        self._check_call("run", [f"//{self._common()}:kernel_aarch64_dist"])
        real_prebuilts_dir = self.real_kleaf_repo / "out/kernel_aarch64/dist"
        build_id = "123456"

        with open(real_prebuilts_dir / f"manifest_{build_id}.xml", "w") as f:
            self._get_repo_manifest(revision=True).writexml(f)

        # Restore value of ddk_workspace in child process, which is a tmp dir
        ddk_workspace = arguments.mount_spec[self.ddk_workspace]
        kleaf_repo = ddk_workspace / "external/kleaf"
        prebuilts_dir = ddk_workspace / "gki_prebuilts"

        self._run_ddk_workspace_setup_test(
            kleaf_repo,
            ddk_workspace=ddk_workspace,
            prebuilts_dir=prebuilts_dir,
            local=False,
            url_fmt=f"file://{real_prebuilts_dir}/{{filename}}",
            build_id=build_id,
            # build bots have no repo, so we cannot check if `repo sync`
            # actually works. Skip sync and rely on the mount point.
            sync=False)

    def _run_ddk_workspace_setup_test(self,
                                      kleaf_repo: pathlib.Path,
                                      ddk_workspace: pathlib.Path,
                                      prebuilts_dir: pathlib.Path | None = None,
                                      local: bool = True,
                                      url_fmt: str | None = None,
                                      build_id: str | None = None,
                                      sync: bool | None = None):
        # kleaf_repo relative to ddk_workspace
        kleaf_repo_rel = self._force_relative_to(
            kleaf_repo, ddk_workspace)

        git_clean_args = [shutil.which("git"), "clean", "-fdx"]
        if kleaf_repo.is_relative_to(ddk_workspace):
            git_clean_args.extend([
                "-e",
                str(kleaf_repo.relative_to(ddk_workspace)),
            ])

        # git clean is executed in the self.ddk_workspace, not the mounted
        # ddk_workspace, because the latter may be out of Git's version control.
        Exec.check_call(git_clean_args, cwd=self.ddk_workspace)

        # Delete generated files at the end
        self.addCleanup(Exec.check_call, git_clean_args,
                        cwd=self.ddk_workspace)

        self._mount(kleaf_repo)

        # Fake `repo init` executed by user
        default_manifest = ddk_workspace / ".repo/manifests/default.xml"
        default_manifest.parent.mkdir(parents=True, exist_ok=True)
        default_manifest.write_text("""<?xml version="1.0" ?><manifest />""")

        args = [
            "//build/kernel:init_ddk",
            "--",
            f"--kleaf_repo={kleaf_repo}",
            f"--ddk_workspace={ddk_workspace}",
        ]
        if local:
            args.append("--local")
        if prebuilts_dir:
            args.append(f"--prebuilts_dir={prebuilts_dir}")
        if url_fmt:
            args.append(f"--url_fmt={url_fmt}")
        if build_id:
            args.append(f"--build_id={build_id}")
        if sync is not None:
            args.append("--sync" if sync else "--nosync")
        self._check_call("run", args)
        Exec.check_call([
            sys.executable,
            str(ddk_workspace / "extra_setup.py"),
            f"--kleaf_repo_rel={kleaf_repo_rel}",
            f"--ddk_workspace={ddk_workspace}",
        ])

        self._check_call("clean", ["--expunge"], cwd=ddk_workspace)

        args = []
        # Switch base kernel when using prebuilts
        if prebuilts_dir:
            args.append("--//tests:kernel=@gki_prebuilts//kernel_aarch64")
        args.append("//tests")
        self._check_call("test", args, cwd=ddk_workspace)

        # Delete generated files
        self._check_call("clean", ["--expunge"], cwd=ddk_workspace)


# Quick integration tests. Each test case should finish within 1 minute.
# The whole test suite should finish within 5 minutes. If the whole test suite
# takes too long, consider sharding QuickIntegrationTest too.


class QuickIntegrationTest(KleafIntegrationTestBase):

    def test_change_to_core_kernel_does_not_affect_modules_prepare(self):
        """Tests that, with a small change to the core kernel, modules_prepare does not change.

        See b/254357038, b/267385639, b/263415662.
        """
        modules_prepare_archive = \
            f"bazel-bin/{self._common()}/kernel_aarch64_modules_prepare/modules_prepare_outdir.tar.gz"

        # This also tests that fixdep is not needed.
        self._build([f"//{self._common()}:kernel_aarch64_modules_prepare"] +
                    _FASTEST)
        first_hash = self._sha256(modules_prepare_archive)

        old_modules_archive = tempfile.NamedTemporaryFile(delete=False)
        shutil.copyfile(modules_prepare_archive, old_modules_archive.name)

        self._touch_core_kernel_file()

        self._build([f"//{self._common()}:kernel_aarch64_modules_prepare"] +
                    _FASTEST)
        second_hash = self._sha256(modules_prepare_archive)

        if first_hash == second_hash:
            os.unlink(old_modules_archive.name)

        self.assertEqual(
            first_hash, second_hash,
            textwrap.dedent(f"""\
                             Check their content here:
                             old: {old_modules_archive.name}
                             new: {modules_prepare_archive}"""))

    def test_module_does_not_depend_on_vmlinux(self):
        """Tests that, the inputs for building a module does not include vmlinux and System.map.

        See b/254357038."""
        vd_modules = self._check_output("query", [
            'kind("^_kernel_module rule$", //common-modules/virtual-device/...)'
        ]).splitlines()
        self.assertTrue(vd_modules)

        print(
            f"+ build/kernel/kleaf/analysis/inputs.py 'mnemonic(\"KernelModule.*\", {vd_modules[0]})'"
        )
        input_to_module = analyze_inputs(
            aquery_args=[f'mnemonic("KernelModule.*", {vd_modules[0]})'] +
            _FASTEST).keys()
        self.assertFalse([
            path
            for path in input_to_module if pathlib.Path(path).name == "vmlinux"
        ], "An external module must not depend on vmlinux")
        self.assertFalse([
            path for path in input_to_module
            if pathlib.Path(path).name == "System.map"
        ], "An external module must not depend on System.map")

    def test_override_javatmp(self):
        """Tests that out/bazel/javatmp can be overridden.

        See b/267580482."""
        default_java_tmp = pathlib.Path("out/bazel/javatmp")
        new_java_tmp = tempfile.TemporaryDirectory()
        self.addCleanup(new_java_tmp.cleanup)
        try:
            shutil.rmtree(default_java_tmp)
        except FileNotFoundError:
            pass
        self._check_call(startup_options=[
            f"--host_jvm_args=-Djava.io.tmpdir={new_java_tmp.name}"
        ],
            command="build",
            command_args=["//build/kernel/kleaf:empty_test"] +
            _FASTEST)
        self.assertFalse(default_java_tmp.exists())

    def test_override_absolute_out_dir(self):
        """Tests that out/ can be overridden.

        See b/267580482."""
        new_out1 = tempfile.TemporaryDirectory()
        new_out2 = tempfile.TemporaryDirectory()
        self.addCleanup(new_out1.cleanup)
        self.addCleanup(new_out2.cleanup)
        shutil.rmtree(new_out1.name)
        shutil.rmtree(new_out2.name)

        self._check_call(startup_options=[f"--output_root={new_out1.name}"],
                         command="build",
                         command_args=["//build/kernel/kleaf:empty_test"] +
                         _FASTEST)
        self.assertTrue(pathlib.Path(new_out1.name).exists())
        self.assertFalse(pathlib.Path(new_out2.name).exists())
        shutil.rmtree(new_out1.name)
        self._check_call(startup_options=[f"--output_root={new_out2.name}"],
                         command="build",
                         command_args=["//build/kernel/kleaf:empty_test"] +
                         _FASTEST)
        self.assertFalse(pathlib.Path(new_out1.name).exists())
        self.assertTrue(pathlib.Path(new_out2.name).exists())

    def test_config_uapi_header_test(self):
        """Tests that CONFIG_UAPI_HEADER_TEST is not deleted.

        To keep CONFIG_UAPI_HEADER_TEST, USERCFLAGS needs to set --sysroot and
        --target properly, and USERLDFLAGS needs to set --sysroot.

        See b/270996321 and b/190019968."""

        archs = [
            ("aarch64", "arm64"),
            ("x86_64", "x86"),
            # TODO(b/271919464): Need NDK_TRIPLE for riscv so --sysroot is properly set
            # ("riscv64", "riscv"),
        ]

        for arch, srcarch in archs:
            with self.subTest(arch=arch, srcarch=srcarch):
                gki_defconfig = f"{self._common()}/arch/{srcarch}/configs/gki_defconfig"
                self.restore_file_after_test(gki_defconfig)

                self._check_call("run", [
                    f"//{self._common()}:kernel_{arch}_config", "--",
                    "olddefconfig"
                ] + _FASTEST)

                with open(gki_defconfig) as f:
                    new_gki_defconfig_content = f.read()
                self.assertTrue(
                    "CONFIG_UAPI_HEADER_TEST=y"
                    in new_gki_defconfig_content.splitlines(),
                    f"gki_defconfig should still have CONFIG_UAPI_HEADER_TEST=y after "
                    f"`bazel run //{self._common()}:kernel_aarch64_config "
                    f"-- olddefconfig`, but got\n{new_gki_defconfig_content}")

                # It should be fine to call the same command subsequently.
                self._check_call("run", [
                    f"//{self._common()}:kernel_{arch}_config", "--",
                    "olddefconfig"
                ] + _FASTEST)

    def test_menuconfig_merge(self):
        """Test that menuconfig works with a raw merge_config.sh in PRE_DEFCONFIG_CMDS.

        See `menuconfig_merge_test/` for details.

        See b/276889737 and b/274878805."""

        args = [
            "//build/kernel/kleaf/tests/integration_test/menuconfig_merge_test:menuconfig_merge_test_config",
        ] + _FASTEST

        output = self._check_output("run", args)

        def matching_line(line): return re.match(
            r"^Updating .*common/arch/arm64/configs/menuconfig_test_defconfig$",
            line)
        self.assertTrue(
            any([matching_line(line) for line in output.splitlines()]))

        # It should be fine to call the same command subsequently.
        self._check_call("run", args)

    def test_menuconfig_fragment(self):
        """Test that menuconfig works with a FRAGMENT_CONFIG defined.

        See `menuconfig_fragment_test/` for details.

        See b/276889737 and b/274878805."""

        args = [
            "//build/kernel/kleaf/tests/integration_test/menuconfig_fragment_test:menuconfig_fragment_test_config",
        ] + _FASTEST

        output = self._check_output("run", args)

        expected_line = f"Updated {os.environ['BUILD_WORKSPACE_DIRECTORY']}/build/kernel/kleaf/tests/integration_test/menuconfig_fragment_test/defconfig.fragment"
        self.assertTrue(expected_line, output.splitlines())

        # It should be fine to call the same command subsequently.
        self._check_call("run", args)

    def test_ddk_defconfig_must_present(self):
        """Test that for ddk_module, items in defconfig must be in the final .config.

        See b/279105294.
        """

        args = [
            "//build/kernel/kleaf/tests/integration_test/ddk_negative_test:defconfig_must_present_test_module_config",
        ] + _FASTEST
        popen = self._popen("build",
                            args,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
        _, stderr = popen.communicate()
        self.assertNotEqual(popen.returncode, 0)
        self.assertIn(
            "CONFIG_DELETED_SET: actual '', expected 'CONFIG_DELETED_SET=y' from build/kernel/kleaf/tests/integration_test/ddk_negative_test/defconfig.",
            stderr)
        self.assertNotIn("DECLARED_SET", stderr)
        self.assertNotIn("DECLARED_UNSET", stderr)

    def test_dash_dash_help(self):
        """Test that `bazel --help` works."""
        self._check_output("--help", use_bazelrc=False, use_wrapper_args=False)

    def test_help(self):
        """Test that `bazel help` works."""
        self._check_output("help")

    def test_help_kleaf(self):
        """Test that `bazel help kleaf` works."""
        self._check_output("help", ["kleaf"])

    def test_strip_execroot(self):
        """Test that --strip_execroot works."""
        self._check_output("build", ["--nobuild", "--strip_execroot",
                                     "//build/kernel:hermetic-tools"])

    def test_strip_execroot_error(self):
        """Tests that if cmd with --strip_execroot fails, exit code is set."""
        popen = self._popen("what",
                            ["--strip_execroot"],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
        popen.communicate()
        self.assertNotEqual(popen.returncode, 0)

    def test_no_unexpected_output_error_with_proper_flags(self):
        """With --config=silent, no errors emitted on unexpected lines."""

        # Do a build with `--config=local` to force analysis cache to be invalid
        # in the next run, which triggers
        # WARNING: Build options [...] have changed, discarding analysis cache
        self._build(
            ["//build/kernel:hermetic-tools", "--config=local"],
        )

        allowlist = pathlib.Path("build/kernel/kleaf/spotless_log_regex.txt")

        startup_options = [
            f"--stdout_stderr_regex_allowlist={allowlist.resolve()}",
        ]
        self._build(["--config=silent", "//build/kernel:hermetic-tools"],
                    startup_options=startup_options)

    def test_detect_unexpected_output_error(self):
        """Without --config=silent, there are errors on unexpected lines."""
        allowlist = pathlib.Path("build/kernel/kleaf/spotless_log_regex.txt")

        startup_options = [
            f"--stdout_stderr_regex_allowlist={allowlist.resolve()}",
        ]
        stderr = self._check_errors(
            "build",
            ["//build/kernel:hermetic-tools"],
            startup_options=startup_options,
        )
        self.assertIn("unexpected lines", stderr)

    def test_no_unexpected_output_error_if_process_exits_abnormally(self):
        """If the bazel command fails, no errors emitted on unexpected lines."""
        allowlist = pathlib.Path("build/kernel/kleaf/spotless_log_regex.txt")

        startup_options = [
            f"--stdout_stderr_regex_allowlist={allowlist.resolve()}",
        ]
        stderr = self._check_errors(
            "build",
            ["//does_not_exist"],
            startup_options=startup_options,
        )
        self.assertNotIn("unexpected lines", stderr)

    def test_hermetic_tools_double_compilation(self):
        """Test that hermetic tools always has exec transition."""

        target = "//build/kernel/kleaf/tests/integration_test/hermetic_tools_test:mytest"
        output = self._check_output(
            "cquery",
            [
                "--gzip_is_pigz",
                # Use @.*pigz because under bzlmod, the repo name for extensions
                # are mangled.
                f'filter("@.*pigz//:pigz$", deps({target}))',
                # Suppress INFO level logs. We are only interested in error logs
                # and the cquery output.
                "--ui_event_filters=,+error,+fail,+stderr,+stdout",
                "--noshow_progress",
            ]
        )
        self.assertEqual(1, output.count("@pigz//:pigz"))

class ScmversionIntegrationTest(KleafIntegrationTestBase):

    def setUp(self) -> None:
        super().setUp()

        self.strings = shutil.which("llvm-strings")
        self.uname_pattern_prefix = re.compile(
            r"^Linux version [0-9]+[.][0-9]+[.][0-9]+(\S*)")

        self.build_config_common_path = f"{self._common()}/build.config.common"
        self.restore_file_after_test(self.build_config_common_path)

        self.gki_defconfig_path = f"{self._common()}/arch/arm64/configs/gki_defconfig"
        self.restore_file_after_test(self.gki_defconfig_path)

        self.makefile_path = f"{self._common()}/Makefile"
        self.restore_file_after_test(self.makefile_path)

    def _setup_mainline(self):
        with open(self.build_config_common_path, "a") as f:
            f.write("BRANCH=android-mainline\n")
            f.write("unset KMI_GENERATION\n")

        # Writing to defconfig directly requires us to disable check_defconfig,
        # because the ordering is not correct.
        self.build_config_gki_aarch64_path = f"{self._common()}/build.config.gki.aarch64"
        self.restore_file_after_test(self.build_config_gki_aarch64_path)
        with open(self.build_config_gki_aarch64_path, "a") as f:
            f.write("POST_DEFCONFIG_CMDS=true\n")

        extraversion_pattern = re.compile(r"^EXTRAVERSION\s*=")
        self.replace_lines(self.makefile_path,
                           lambda x: re.search(extraversion_pattern, x),
                           ["EXTRAVERSION = -rc999"])

    def _setup_release_branch(self):
        with open(self.build_config_common_path, "a") as f:
            f.write(
                textwrap.dedent("""\
                BRANCH=android99-100.110
                KMI_GENERATION=56
            """))

        localversion_pattern = re.compile(r"^CONFIG_LOCALVERSION=")
        self.filter_lines(self.gki_defconfig_path,
                          lambda x: not re.search(localversion_pattern, x))
        extraversion_pattern = re.compile(r"^EXTRAVERSION\s*=")
        self.replace_lines(self.makefile_path,
                           lambda x: re.search(extraversion_pattern, x),
                           ["EXTRAVERSION ="])

    def _get_vmlinux_scmversion(self, workspace_root=pathlib.Path("."),
                                package : str | pathlib.Path | None = None):
        if not package:
            package = self._common()
        strings_output = Exec.check_output([
            self.strings,
            str(workspace_root / f"bazel-bin/{package}/kernel_aarch64/vmlinux")
        ])
        ret = []
        for line in strings_output.splitlines():
            mo = re.search(self.uname_pattern_prefix, line)
            if mo:
                ret.append(mo.group(1))
        self.assertTrue(ret)
        print(f"scmversion = {ret}")
        return ret

    @staticmethod
    def _env_without_build_number():
        env = dict(os.environ)
        env.pop("BUILD_NUMBER", None)
        # Fix this error to execute `repo` properly:
        #  ModuleNotFoundError: No module named 'color'
        env.pop("PYTHONSAFEPATH", None)
        return env

    @staticmethod
    def _env_with_build_number(build_number):
        env = dict(os.environ)
        env["BUILD_NUMBER"] = str(build_number)
        # Fix this error to execute `repo` properly:
        #  ModuleNotFoundError: No module named 'color'
        env.pop("PYTHONSAFEPATH", None)
        return env

    def test_mainline_no_stamp(self):
        self._setup_mainline()
        self._check_call(
            "build",
            _FASTEST + [
                "--config=local",
                f"//{self._common()}:kernel_aarch64",
            ],
            env=ScmversionIntegrationTest._env_without_build_number())
        for scmversion in self._get_vmlinux_scmversion():
            self.assertEqual("-rc999-mainline-maybe-dirty-4k", scmversion)

    def test_mainline_stamp(self):
        self._setup_mainline()
        self._check_call(
            "build",
            _FASTEST + [
                "--config=stamp",
                "--config=local",
                f"//{self._common()}:kernel_aarch64",
            ],
            env=ScmversionIntegrationTest._env_without_build_number())
        scmversion_pat = re.compile(
            r"^-rc999-mainline(-[0-9]{5,})?-g[0-9a-f]{12,40}(-dirty)?(-4k)?$")
        for scmversion in self._get_vmlinux_scmversion():
            self.assertRegexpMatches(scmversion, scmversion_pat)

    def test_mainline_ab(self):
        self._setup_mainline()
        self._check_call(
            "build",
            _FASTEST + [
                "--config=stamp",
                "--config=local",
                f"//{self._common()}:kernel_aarch64",
            ],
            env=ScmversionIntegrationTest._env_with_build_number("123456"))
        scmversion_pat = re.compile(
            r"^-rc999-mainline(-[0-9]{5,})?-g[0-9a-f]{12,40}(-dirty)?-ab123456(-4k)?$"
        )
        for scmversion in self._get_vmlinux_scmversion():
            self.assertRegexpMatches(scmversion, scmversion_pat)

    def test_release_branch_no_stamp(self):
        self._setup_release_branch()
        self._check_call(
            "build",
            _FASTEST + [
                "--config=local",
                f"//{self._common()}:kernel_aarch64",
            ],
            env=ScmversionIntegrationTest._env_without_build_number())
        for scmversion in self._get_vmlinux_scmversion():
            self.assertEqual("-android99-56-maybe-dirty", scmversion)

    def test_release_branch_stamp(self):
        self._setup_release_branch()
        self._check_call(
            "build",
            _FASTEST + [
                "--config=stamp",
                "--config=local",
                f"//{self._common()}:kernel_aarch64",
            ],
            env=ScmversionIntegrationTest._env_without_build_number())
        scmversion_pat = re.compile(
            r"^-android99-56(-[0-9]{5,})?-g[0-9a-f]{12,40}(-dirty)?$")
        for scmversion in self._get_vmlinux_scmversion():
            self.assertRegexpMatches(scmversion, scmversion_pat)

    def test_release_branch_ab(self):
        self._setup_release_branch()
        self._check_call(
            "build",
            _FASTEST + [
                "--config=stamp",
                "--config=local",
                f"//{self._common()}:kernel_aarch64",
            ],
            env=ScmversionIntegrationTest._env_with_build_number("123456"))
        scmversion_pat = re.compile(
            r"^-android99-56(-[0-9]{5,})?-g[0-9a-f]{12,40}(-dirty)?-ab123456$")
        for scmversion in self._get_vmlinux_scmversion():
            self.assertRegexpMatches(scmversion, scmversion_pat)

    def test_stamp_repo_root_is_not_workspace_root(self):
        """Tests that --config=stamp works when repo root is not Bazel workspace root."""

        self._setup_mainline()

        real_workspace_root = pathlib.Path(".").resolve()
        repo_root = (pathlib.Path(__file__).resolve().parent /
                              "fake_repo_root")
        workspace_root = repo_root / "fake_workspace_root"

        if not arguments.mount_spec:
            mount_spec = {
                real_workspace_root : workspace_root
            }

            self._unshare_mount_run(mount_spec=mount_spec, link_spec=LinkSpec())
            return

        self._mount(workspace_root)

        manifest = self._get_repo_manifest()
        for project in manifest.documentElement.getElementsByTagName("project"):
            path = project.getAttribute("path") or project.getAttribute("name")
            project.setAttribute(
                "path", str(pathlib.Path("fake_workspace_root") / path))

        new_manifest_temp_file = tempfile.NamedTemporaryFile(
            mode="w+", delete=False)
        new_manifest = pathlib.Path(new_manifest_temp_file.name)
        self.addCleanup(new_manifest.unlink)

        with new_manifest_temp_file as file_handle:
            manifest.writexml(file_handle)

        # KI: For this build, git commands on certain projects (e.g.
        # prebuilts/ndk-r26, prebuilts/clang) are slow because it needs to
        # refresh index.
        self._check_call(
            "build",
            _FASTEST + [
                "--config=stamp",
                f"--repo_manifest={repo_root}:{new_manifest}",
                f"//{self._common()}:kernel_aarch64",
            ],
            cwd=workspace_root,
            env=ScmversionIntegrationTest._env_without_build_number(),
        )

        scmversion_pat = re.compile(
            r"^-rc999-mainline(-[0-9]{5,})?-g[0-9a-f]{12,40}(-dirty)?(-4k)?$")
        for scmversion in self._get_vmlinux_scmversion(workspace_root):
            self.assertRegexpMatches(scmversion, scmversion_pat)

    def test_stamp_if_kernel_dir_is_symlink(self):
        """Tests that --stamp works if KERNEL_DIR is a symlink."""
        self._setup_mainline()

        new_kernel_dir = pathlib.Path("test_symlink")
        with open(self.build_config_gki_aarch64_path, "a") as f:
            f.write(f"KERNEL_DIR={new_kernel_dir}\n")

        if not new_kernel_dir.is_symlink():
            new_kernel_dir.symlink_to(self._common(), True)
        self.addCleanup(new_kernel_dir.unlink)

        self._check_call(
            "build",
            _FASTEST + [
                "--config=stamp",
                "--config=local",
                f"//{new_kernel_dir}:kernel_aarch64",
                f"--extra_git_project={new_kernel_dir}"
            ],
            env=ScmversionIntegrationTest._env_without_build_number())
        scmversion_pat = re.compile(
            r"^-rc999-mainline(-[0-9]{5,})?-g[0-9a-f]{12,40}(-dirty)?(-4k)?$")
        for scmversion in self._get_vmlinux_scmversion(package=new_kernel_dir):
            self.assertRegexpMatches(scmversion, scmversion_pat)


# Class that mimics tee(1)
class Tee(object):
    def __init__(self, stream: TextIO, path: pathlib.Path):
        self._stream = stream
        self._path = path

    def __getattr__(self, name: str) -> Any:
        return getattr(self._stream, name)

    def write(self, *args, **kwargs) -> int:
        # Ignore short write to console
        self._stream.write(*args, **kwargs)
        return self._file.write(*args, **kwargs)

    def __enter__(self) -> "Tee":
        self._file = open(self._path, "w")
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._file.close()


def _get_exit_code(exc: SystemExit):
    if exc.code is None:
        return 0
    # absltest calls sys.exit() with a boolean value.
    if type(exc.code) == bool:
        return int(exc.code)
    if type(exc.code) == int:
        return exc.code
    print(
        f"ERROR: Unknown exit code: {e.code}, exiting with code 1",
        file=sys.stderr)
    return 1


if __name__ == "__main__":
    arguments, unknown = load_arguments()

    if not arguments.test_result_dir:
        sys.argv[1:] = unknown
        absltest.main()
        sys.exit(0)

    # If --test_result_dir is set, also set --xml_output_file for :reporter.
    unknown += [
        "--xml_output_file",
        str(arguments.test_result_dir / "output.xml")
    ]
    sys.argv[1:] = unknown

    os.makedirs(arguments.test_result_dir, exist_ok=True)
    stdout_path = arguments.test_result_dir / "stdout.txt"
    stderr_path = arguments.test_result_dir / "stderr.txt"
    with Tee(sys.__stdout__, stdout_path) as stdout_tee, \
            Tee(sys.__stderr__, stderr_path) as stderr_tee, \
            contextlib.redirect_stdout(stdout_tee), \
            contextlib.redirect_stderr(stderr_tee):
        try:
            absltest.main()
            exit_code = 0
        except SystemExit as e:
            exit_code = _get_exit_code(e)

    exit_code_path = arguments.test_result_dir / "exitcode.txt"
    with open(exit_code_path, "w") as exit_code_file:
        exit_code_file.write(f"{exit_code}\n")
