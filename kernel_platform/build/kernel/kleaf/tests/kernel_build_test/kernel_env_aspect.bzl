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

KernelEnvAspectInfo = provider(fields = {
    "kernel_env": "The `kernel_env` target",
})

def _kernel_env_aspect_impl(target, ctx):
    if ctx.rule.kind == "_kernel_build":
        return ctx.rule.attr.config[KernelEnvAspectInfo]
    if ctx.rule.kind == "kernel_config":
        return ctx.rule.attr.env[KernelEnvAspectInfo]
    if ctx.rule.kind == "kernel_env":
        return KernelEnvAspectInfo(kernel_env = target)

    fail("{label}: Unable to get `kernel_env` because {kind} is not supported.".format(
        kind = ctx.rule.kind,
        label = ctx.label,
    ))

kernel_env_aspect = aspect(
    implementation = _kernel_env_aspect_impl,
    doc = "An aspect describing the `kernel_env` of a `_kernel_build`, `kernel_config`, or `kernel_env` rule.",
    attr_aspects = [
        "config",
        "env",
    ],
)
