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

"""Optional epilog macro for defining repositories in a Kleaf workspace."""

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

def define_kleaf_workspace_epilog():
    """Optional epilog macro for defining repositories in a Kleaf workspace.

    **This macro must only be called from `WORKSPACE` or `WORKSPACE.bazel`
    files, not `BUILD` or `BUILD.bazel` files!**

    The epilog macro is needed if you are running
    [Bazel analysis tests](https://bazel.build/rules/testing).

    If called, it must be called after
    [`define_kleaf_workspace`](workspace.md#define_kleaf_workspace) is called.
    """
    bazel_skylib_workspace()
