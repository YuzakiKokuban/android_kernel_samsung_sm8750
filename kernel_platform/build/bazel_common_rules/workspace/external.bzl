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

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def import_external_repositories(
        workspace_root = None,
        bazel_skylib = None,
        io_abseil_py = None,
        io_bazel_stardoc = None):
    """Import repositories in `{root}/external/` that are common to Bazel builds for Android.

    In particular, these external projects are shared by Android platform
    repo manifest and Android kernel repo manifest.

    Caller of this function (in the `WORKSPACE` file) should manage a list
    of repositories imported by providing them in the arguments.

    Args:
        workspace_root: Root under which the `external/` directory may be found, relative
            to the main workspace.

            When calling `import_external_repositories` in the main workspace's
            `WORKSPACE` file, leave `root = None`.
        bazel_skylib: If `True`, load `bazel_skylib`.
        io_abseil_py: If `True`, load `io_abseil_py`.
        io_bazel_stardoc: If `True`, load `io_bazel_stardoc`.
    """
    workspace_prefix = workspace_root or ""
    if workspace_prefix:
        workspace_prefix += "/"

    if bazel_skylib:
        maybe(
            repo_rule = native.local_repository,
            name = "bazel_skylib",
            path = "{}external/bazel-skylib".format(workspace_prefix),
        )

    if io_abseil_py:
        maybe(
            repo_rule = native.local_repository,
            name = "io_abseil_py",
            path = "{}external/python/absl-py".format(workspace_prefix),
        )

    if io_bazel_stardoc:
        maybe(
            repo_rule = native.local_repository,
            name = "io_bazel_stardoc",
            path = "{}external/stardoc".format(workspace_prefix),
        )
