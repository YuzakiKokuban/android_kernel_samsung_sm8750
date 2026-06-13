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

"""Create a build.config file by concatenating build config fragments."""

load(":common_providers.bzl", "KernelBuildConfigInfo")
load(":debug.bzl", "debug")
load(":hermetic_toolchain.bzl", "hermetic_toolchain")

visibility("//build/kernel/kleaf/...")

def _kernel_build_config_impl(ctx):
    out_file = ctx.actions.declare_file(ctx.attr.name + ".generated")
    hermetic_tools = hermetic_toolchain.get(ctx)
    command = hermetic_tools.setup + """
        cat {srcs} > {out_file}
    """.format(
        srcs = " ".join([src.path for src in ctx.files.srcs]),
        out_file = out_file.path,
    )
    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = "KernelBuildConfig",
        inputs = ctx.files.srcs,
        tools = hermetic_tools.deps,
        outputs = [out_file],
        command = command,
        progress_message = "Generating build config {}".format(ctx.label),
    )

    deps_depset = depset(transitive = [target.files for target in ctx.attr.deps])

    return [
        DefaultInfo(files = depset([out_file])),
        KernelBuildConfigInfo(deps = deps_depset),
    ]

kernel_build_config = rule(
    implementation = _kernel_build_config_impl,
    doc = "Create a build.config file by concatenating build config fragments.",
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            doc = """List of build config fragments.

Order matters. To prevent buildifier from sorting the list, use the
`# do not sort` magic line. For example:

```
kernel_build_config(
    name = "build.config.foo.mixed",
    srcs = [
        # do not sort
        "build.config.mixed",
        "build.config.foo",
    ],
)
```

""",
        ),
        "deps": attr.label_list(
            allow_files = True,
            doc = """Additional build config dependencies.

These include build configs that are indirectly `source`d by items
in `srcs`. Unlike `srcs`, they are not be emitted in the output.
""",
        ),
        "_debug_print_scripts": attr.label(default = "//build/kernel/kleaf:debug_print_scripts"),
    },
    toolchains = [hermetic_toolchain.type],
)
