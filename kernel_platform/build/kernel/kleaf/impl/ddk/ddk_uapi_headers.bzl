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

"""UAPI headers target for DDK."""

load(":common_providers.bzl", "KernelBuildExtModuleInfo")
load(":debug.bzl", "debug")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

def _ddk_uapi_headers_impl(ctx):
    if not ctx.attr.out.endswith(".tar.gz"):
        fail("{}: out-file name must end with \".tar.gz\"".format(ctx.label.name))

    out_file = ctx.actions.declare_file("{}/{}".format(ctx.label.name, ctx.attr.out))
    ddk_config_env = ctx.attr.kernel_build[KernelBuildExtModuleInfo].ddk_config_env

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = ddk_config_env,
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )

    command += """
         # Make the staging directory
           mkdir -p {kernel_uapi_headers_dir}/usr
         # Make unifdef, required by scripts/headers_install.sh
           make -C ${{KERNEL_DIR}} -f /dev/null scripts/unifdef
         # Install each header individually
           while read -r hdr; do
             out_prefix=$(dirname $(echo ${{hdr}} | sed -e 's|.*include/uapi/||g'))
             mkdir -p {kernel_uapi_headers_dir}/usr/include/${{out_prefix}}
             base=$(basename ${{hdr}})
             (
               cd ${{KERNEL_DIR}}
               ./scripts/headers_install.sh \
                 ${{OLDPWD}}/${{hdr}} ${{OLDPWD}}/{kernel_uapi_headers_dir}/usr/include/${{out_prefix}}/${{base}}
             )
           done < $1
         # Create archive
           tar czf {out_file} --directory={kernel_uapi_headers_dir} usr/
         # Delete kernel_uapi_headers_dir because it is not declared
           rm -rf {kernel_uapi_headers_dir}
    """.format(
        out_file = out_file.path,
        kernel_uapi_headers_dir = out_file.path + "_staging",
    )

    args = ctx.actions.args()
    args.use_param_file("%s", use_always = True)
    args.add_all(depset(transitive = [target.files for target in ctx.attr.srcs]))

    inputs = []
    transitive_inputs = [target.files for target in ctx.attr.srcs]
    transitive_inputs.append(ddk_config_env.inputs)
    tools = ddk_config_env.tools

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "DdkUapiHeaders",
        inputs = depset(inputs, transitive = transitive_inputs),
        tools = tools,
        outputs = [out_file],
        progress_message = "Building DDK UAPI headers %s" % ctx.attr.name,
        command = command,
        arguments = [args],
    )

    return [
        DefaultInfo(files = depset([out_file])),
    ]

ddk_uapi_headers = rule(
    implementation = _ddk_uapi_headers_impl,
    doc = """A rule that generates a sanitized UAPI header tarball.

    Example:

    ```
    ddk_uapi_headers(
       name = "my_headers",
       srcs = glob(["include/uapi/**/*.h"]),
       out = "my_headers.tar.gz",
       kernel_build = "//common:kernel_aarch64",
    )
    ```
    """,
    attrs = {
        "srcs": attr.label_list(
            doc = 'UAPI headers files which can be sanitized by "make headers_install"',
            allow_files = [".h"],
        ),
        "out": attr.string(
            doc = "Name of the output tarball",
            mandatory = True,
        ),
        "kernel_build": attr.label(
            doc = "[`kernel_build`](#kernel_build).",
            providers = [
                KernelBuildExtModuleInfo,
            ],
            mandatory = True,
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
)
