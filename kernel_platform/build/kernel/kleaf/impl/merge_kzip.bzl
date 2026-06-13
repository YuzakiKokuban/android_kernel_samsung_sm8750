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

"""Merge kzips for [Kythe](https://kythe.io/)."""

load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":utils.bzl", "utils")

visibility("//build/kernel/kleaf/...")

def _merge_kzip_impl(ctx):
    all_kzip = ctx.actions.declare_file(ctx.attr.name + "/all.kzip")
    intermediates_dir = utils.intermediates_dir(ctx)
    hermetic_tools = hermetic_toolchain.get(ctx)
    srcs = ctx.files.srcs
    transitive_tools = [hermetic_tools.deps]

    command = hermetic_tools.setup + """
               mkdir -p {intermediates_dir}
             # Package it all into a single .kzip, ignoring duplicates.
               for zip in {srcs}; do
                   unzip -qn "${{zip}}" -d {intermediates_dir}
               done
               soong_zip -d -C {intermediates_dir} -D {intermediates_dir} -o {all_kzip}
             # Clean up directories
               rm -rf {intermediates_dir}
    """.format(
        srcs = " ".join([file.path for file in srcs]),
        intermediates_dir = intermediates_dir,
        all_kzip = all_kzip.path,
    )
    ctx.actions.run_shell(
        mnemonic = "MergeKzip",
        inputs = depset(transitive = [target.files for target in ctx.attr.srcs]),
        outputs = [all_kzip],
        tools = depset(transitive = transitive_tools),
        command = command,
        progress_message = "Merging Kythe source code index (kzip) {}".format(ctx.label),
    )

    return DefaultInfo(files = depset([all_kzip]))

merge_kzip = rule(
    implementation = _merge_kzip_impl,
    doc = """Merge .kzip files""",
    attrs = {
        "srcs": attr.label_list(allow_files = True, doc = "kzip files"),
    },
    toolchains = [
        hermetic_toolchain.type,
    ],
)
