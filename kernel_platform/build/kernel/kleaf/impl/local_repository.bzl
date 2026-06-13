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

"""
Drop-in replacement for `{new_,}local_repository` such that
paths are resolved against Kleaf sub-repository.
"""

visibility([
    "//build/kernel/kleaf/...",
    "//",  # for root MODULE.bazel
])

def _common_attrs():
    return {
        "path": attr.string(doc = "the path relative to Kleaf repository"),
        "path_candidates": attr.string_list(doc = """Candidate `path`s to use, relative to Kleaf repository.

            Order matters. These paths are tested one by one for existence. The
            first directory that exists is used.

            `path` and `path_candidates` cannot be set simultaneously.
        """),
    }

def _get_kleaf_repo_dir(repository_ctx):
    """Returns the root dir of the repository that contains Kleaf tools.

    That is, where this extension should exist."""
    mylabel = Label(":local_repository.bzl")
    mypath = str(repository_ctx.path(mylabel))
    package_path = mypath.removesuffix(mylabel.name).removesuffix("/")
    kleaf_repo_dir = package_path.removesuffix(mylabel.package).removesuffix("/")
    return repository_ctx.path(kleaf_repo_dir)

def _kleaf_local_repository_impl(repository_ctx):
    if repository_ctx.attr.path and repository_ctx.attr.path_candidates:
        fail("@{}: path and path_candidates cannot be set simultaneously".format(repository_ctx.name))

    kleaf_repo_dir = _get_kleaf_repo_dir(repository_ctx)

    target = None
    if repository_ctx.attr.path:
        target = kleaf_repo_dir.get_child(repository_ctx.attr.path)
    else:
        target_candidates = [kleaf_repo_dir.get_child(path_candidate) for path_candidate in repository_ctx.attr.path_candidates]
        for target_candidate in target_candidates:
            if target_candidate.exists:
                target = target_candidate
                break
        if not target:
            fail("@{}: None of path_candidates exists: {}".format(repository_ctx.name, target_candidates))

    for child in target.readdir():
        repository_ctx.symlink(child, repository_ctx.path(child.basename))

kleaf_local_repository = repository_rule(
    attrs = _common_attrs(),
    implementation = _kleaf_local_repository_impl,
)

def _new_kleaf_local_repository_impl(repository_ctx):
    _kleaf_local_repository_impl(repository_ctx)

    kleaf_repo_dir = _get_kleaf_repo_dir(repository_ctx)

    if repository_ctx.attr.build_file:
        repository_ctx.symlink(
            kleaf_repo_dir.get_child(repository_ctx.attr.build_file),
            repository_ctx.path("BUILD.bazel"),
        )
    repository_ctx.file(repository_ctx.path("WORKSPACE.bazel"), """\
workspace({name_repr})
""".format(name_repr = repr(repository_ctx.attr.name)))

new_kleaf_local_repository = repository_rule(
    attrs = _common_attrs() | {
        "build_file": attr.string(doc = "build file. Path is calculated with `repository_ctx.path(build_file)`"),
    },
    implementation = _new_kleaf_local_repository_impl,
)
