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

"""Build kzips for [Kythe](https://kythe.io/)."""

load("@bazel_skylib//lib:shell.bzl", "shell")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load(
    ":common_providers.bzl",
    "KernelBuildInfo",
    "KernelBuildOriginalEnvInfo",
)
load(":srcs_aspect.bzl", "SrcsInfo", "srcs_aspect")
load(":utils.bzl", "utils")

visibility("//build/kernel/kleaf/...")

def _kernel_kythe_transition_impl(_settings, _attr):
    return {
        # Until we resolve OUT_DIR properly like in kernel_build, this needs to be executed in a
        # sandbox.
        # TODO(b/201801372): Consider dropping the sandbox
        "//build/kernel/kleaf:config_local": False,
        "//build/kernel/kleaf/impl:build_compile_commands": True,
    }

_kernel_kythe_transition = transition(
    implementation = _kernel_kythe_transition_impl,
    inputs = [],
    outputs = [
        "//build/kernel/kleaf/impl:build_compile_commands",
        "//build/kernel/kleaf:config_local",
    ],
)

def _create_vnames_mappings(ctx):
    mappings = [
        {
            "pattern": "^(/etc/.*)",
            "vname": {
                "path": "fake_root/@1@",
            },
        },
    ]
    mappings_json = json.encode_indent(mappings, indent = "  ")
    output = ctx.actions.declare_file(ctx.attr.name + "/vnames.json")
    ctx.actions.write(output = output, content = mappings_json)
    return output

def _kernel_kythe_impl(ctx):
    if not ctx.attr.corpus[BuildSettingInfo].value:
        # buildifier: disable=print
        print("WARNING: {}: --{} is not defined!".format(ctx.label, ctx.attr.corpus.label))

    compile_commands_with_vars = ctx.attr.kernel_build[KernelBuildInfo].compile_commands_with_vars
    compile_commands_out_dir = ctx.attr.kernel_build[KernelBuildInfo].compile_commands_out_dir
    all_kzip = ctx.actions.declare_file(ctx.attr.name + "/all.kzip")
    intermediates_dir = utils.intermediates_dir(ctx)
    kzip_dir = intermediates_dir + "/kzip"
    extracted_kzip_dir = intermediates_dir + "/extracted"
    transitive_inputs = [src.files for src in ctx.attr.kernel_build[SrcsInfo].srcs]
    vnames_mappings_json_file = _create_vnames_mappings(ctx)
    inputs = [
        compile_commands_with_vars,
        compile_commands_out_dir,
        vnames_mappings_json_file,
    ]
    tools = [ctx.executable._reconstruct_out_dir]

    # Use KernelBuildOriginalEnvInfo from kernel_env because we don't need anything in $OUT_DIR from
    # kernel_config or kernel_build.
    transitive_inputs.append(ctx.attr.kernel_build[KernelBuildOriginalEnvInfo].env_info.inputs)
    transitive_tools = [ctx.attr.kernel_build[KernelBuildOriginalEnvInfo].env_info.tools]
    command = ctx.attr.kernel_build[KernelBuildOriginalEnvInfo].env_info.setup
    command += """
             # Copy compile_commands.json to root, resolving $ROOT_DIR to the real value,
             # and $OUT_DIR to $ROOT_DIR/$KERNEL_DIR.
               sed -e "s:\\${{OUT_DIR}}:${{OUT_DIR}}:g;s:\\${{ROOT_DIR}}:${{ROOT_DIR}}:g" \\
                    {compile_commands_with_vars} > ${{ROOT_DIR}}/compile_commands.json

             # Prepare directories. Copy from compile_commands_out_dir to $OUT_DIR.
               mkdir -p {kzip_dir} {extracted_kzip_dir} ${{OUT_DIR}}
               rsync -aL --chmod=D+w --chmod=F+w {compile_commands_out_dir}/ ${{OUT_DIR}}/

               {reconstruct_out_dir} ${{COMMON_OUT_DIR}} {compile_commands_with_vars}

             # Define env variables
               export KYTHE_ROOT_DIRECTORY=${{ROOT_DIR}}
               export KYTHE_OUTPUT_DIRECTORY={kzip_dir}
               export KYTHE_CORPUS={quoted_corpus}
               export KYTHE_VNAMES=$(realpath {vnames_mappings_json_file})
               export KYTHE_KZIP_ENCODING=PROTO
               export KYTHE_BUILD_CONFIG={kernel_build_name}
             # Generate kzips
               runextractor compdb -extractor $(which cxx_extractor)

             # Package it all into a single .kzip, ignoring duplicates.
               for zip in $(find {kzip_dir} -name '*.kzip'); do
                   unzip -qn "${{zip}}" -d {extracted_kzip_dir}
               done
               soong_zip -d -C {extracted_kzip_dir} -D {extracted_kzip_dir} -o {all_kzip}
             # Clean up directories
               rm -rf {kzip_dir}
               rm -rf {extracted_kzip_dir}
    """.format(
        compile_commands_with_vars = compile_commands_with_vars.path,
        compile_commands_out_dir = compile_commands_out_dir.path,
        vnames_mappings_json_file = vnames_mappings_json_file.path,
        reconstruct_out_dir = ctx.executable._reconstruct_out_dir.path,
        kzip_dir = kzip_dir,
        extracted_kzip_dir = extracted_kzip_dir,
        quoted_corpus = shell.quote(ctx.attr.corpus[BuildSettingInfo].value),
        all_kzip = all_kzip.path,
        kernel_build_name = ctx.attr.kernel_build.label.name,
    )
    ctx.actions.run_shell(
        mnemonic = "KernelKythe",
        inputs = depset(inputs, transitive = transitive_inputs),
        outputs = [all_kzip],
        tools = depset(tools, transitive = transitive_tools),
        command = command,
        progress_message = "Building Kythe source code index (kzip) {}".format(ctx.label),
    )

    return DefaultInfo(files = depset([
        all_kzip,
    ]))

kernel_kythe = rule(
    implementation = _kernel_kythe_impl,
    doc = """
Extract Kythe source code index (kzip file) from a `kernel_build`.
    """,
    attrs = {
        "kernel_build": attr.label(
            mandatory = True,
            doc = "The `kernel_build` target to extract from.",
            providers = [KernelBuildOriginalEnvInfo, KernelBuildInfo],
            aspects = [srcs_aspect],
        ),
        "corpus": attr.label(
            mandatory = True,
            doc = "A flag containing value of `KYTHE_CORPUS`. See [kythe.io/examples](https://kythe.io/examples).",
        ),
        "_reconstruct_out_dir": attr.label(
            default = "//build/kernel/kleaf/impl:kernel_kythe_reconstruct_out_dir",
            executable = True,
            cfg = "exec",
        ),
        # Allow any package to use kernel_compile_commands because it is a public API.
        # The ACK source tree may be checked out anywhere; it is not necessarily //common
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
    cfg = _kernel_kythe_transition,
)
