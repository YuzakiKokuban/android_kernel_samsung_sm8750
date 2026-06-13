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

"""Helper to kernel_prebuilt_ext to define kernel prebuilt repo."""

load(":kernel_prebuilt_repo.bzl", "kernel_prebuilt_repo")

visibility("//build/kernel/kleaf/...")

_tag_class = tag_class(
    doc = "Declares a repo that contains kernel prebuilts",
    attrs = {
        "name": attr.string(
            doc = "name of repository",
            mandatory = True,
        ),
        "local_artifact_path": attr.string(
            doc = """Directory to local artifacts.

                If set, `artifact_url_fmt` is ignored.

                Only the root module may call `declare()` with this attribute set.

                If relative, it is interpreted against workspace root.

                If absolute, this is similar to setting `artifact_url_fmt` to
                `file://<absolute local_artifact_path>/{filename}`, but avoids
                using `download()`. Files are symlinked not copied, and
                `--config=internet` is not necessary.
            """,
        ),
        "target": attr.string(
            doc = """Name of the build target as identified by the remote build server.

                This attribute has two effects:

                * Replaces the `{target}` anchor in `artifact_url_fmt`.
                    If `artifact_url_fmt` does not have the `{target}` anchor,
                    this has no effect.

                * If `auto_download_config` is `True`, `download_config`
                    and `mandatory` is inferred from a
                    list of known configs keyed on `target`.
            """,
            default = "kernel_aarch64",
        ),
    },
)

def _declare_repos(module_ctx, tag_name):
    for module in module_ctx.modules:
        for module_tag in getattr(module.tags, tag_name):
            kernel_prebuilt_repo(
                name = module_tag.name,
                apparent_name = module_tag.name,
                local_artifact_path = module_tag.local_artifact_path,
                target = module_tag.target,
            )

declare_kernel_prebuilts = struct(
    declare_repos = _declare_repos,
    tag_class = _tag_class,
)
