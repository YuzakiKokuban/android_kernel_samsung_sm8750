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

"""Parses the repo manifest from a build."""

import dataclasses
import pathlib
import re
import xml.dom.minidom
import xml.parsers.expat
from typing import TextIO

from init.init_errors import KleafProjectSetterError

_TOOLS_BAZEL = "tools/bazel"


@dataclasses.dataclass(frozen=True)
class ProjectState:
    # Paths are relative to repo root.

    # Original path in the repo manifest in the build
    original_path: pathlib.Path

    # Fixed up path below the current repo root.
    fixed_path: pathlib.Path


@dataclasses.dataclass
class RepoManifestParser:
    """Parses the repo manifest from a build."""
    manifest: str
    project_prefix: pathlib.Path

    # If None, add all projects. If a set, only add projects that matches
    # any of these groups. If both an empty set, no project is added.

    # list of projects with paths fixed up below project_prefix
    fixup_groups: set[str] | None
    # list of projects with paths untouched
    preserve_groups: set[str] | None

    def write_transformed_dom(self, file: TextIO) \
            -> set[ProjectState]:
        """Transforms manifest from the build and write result to file.

        Returns:
            set of ProjectState objects describing old and new paths.
        """
        try:
            with xml.dom.minidom.parseString(self.manifest) as dom:
                project_states = self._transform_dom(dom)
                dom.writexml(file)
                return project_states
        except xml.parsers.expat.ExpatError as err:
            raise KleafProjectSetterError("Unable to parse repo manifest") \
                from err

    def _transform_dom(self, dom: xml.dom.minidom.Document) \
            -> set[ProjectState]:
        """Transforms manifest from the build.

        - Append project_prefix to each project.
        - Filter out projects of mismatching groups
        - Drop elements that may conflict with the main manifest

        Returns:
            set of ProjectState objects describing old and new paths.
        """
        root: xml.dom.minidom.Element = dom.documentElement
        projects = root.getElementsByTagName("project")
        defaults = self._parse_repo_manifest_defaults(root)
        project_states = set()
        for project in projects:
            category = self._match_group(project)
            if category == "delete":
                root.removeChild(project).unlink()
                continue

            for key, value in defaults.items():
                if not project.hasAttribute(key):
                    project.setAttribute(key, value)

            # https://gerrit.googlesource.com/git-repo/+/master/docs/manifest-format.md#element-project
            orig_path_below_repo = pathlib.Path(project.getAttribute("path") or
                                                project.getAttribute("name"))

            if category == "preserve":
                project_states.add(ProjectState(orig_path_below_repo,
                                                orig_path_below_repo))
                continue

            path_below_repo = self.project_prefix / orig_path_below_repo
            project_states.add(ProjectState(
                orig_path_below_repo, path_below_repo))
            project.setAttribute("path", str(path_below_repo))

            for link in project.getElementsByTagName("linkfile"):
                orig_dest = link.getAttribute("dest")
                # b/355523169 special case which should be in the top directory.
                if orig_dest == _TOOLS_BAZEL:
                    continue
                orig_dest = pathlib.Path(orig_dest)
                link.setAttribute("dest", str(self.project_prefix / orig_dest))

        # Avoid <superproject> and <default> in Kleaf manifest conflicting with
        # the one in main manifest
        for superproject in root.getElementsByTagName("superproject"):
            root.removeChild(superproject).unlink()
        for default_element in root.getElementsByTagName("default"):
            root.removeChild(default_element).unlink()
        return project_states

    def _match_group(self, project: xml.dom.minidom.Element) -> str:
        """Returns category of the groups if project matches any of groups."""
        # preserve_groups has higher priority.
        if self._match_group_internal(project, self.preserve_groups):
            return "preserve"
        if self._match_group_internal(project, self.fixup_groups):
            return "fixup"
        return "delete"

    def _match_group_internal(self, project: xml.dom.minidom.Element,
                              expect_groups: set[str] | None):
        if expect_groups is None:
            return True
        project_groups = re.split(r",| ", project.getAttribute("groups"))
        return bool(set(project_groups) & expect_groups)

    def _parse_repo_manifest_defaults(self, root: xml.dom.minidom.Element):
        """Parses <default> in a repo manifest. """
        ret = dict[str, str]()
        for default_element in root.getElementsByTagName("default"):
            attrs = default_element.attributes
            for index in range(attrs.length):
                attr = attrs.item(index)
                assert isinstance(attr, xml.dom.minidom.Attr)
                ret[attr.name] = attr.value
        return ret
