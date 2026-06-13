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

"""Utilities to get files from Python toolchain."""

visibility("//build/kernel/kleaf/...")

_PY_TOOLCHAIN_TYPE = "@bazel_tools//tools/python:toolchain_type"

def _python_interpreter_file_impl(ctx):
    return DefaultInfo(files = depset([ctx.toolchains[_PY_TOOLCHAIN_TYPE].py3_runtime.interpreter]))

python_interpreter_file = rule(
    doc = "Resolves to the Python interpreter from resolved Python toolchain.",
    implementation = _python_interpreter_file_impl,
    toolchains = [config_common.toolchain_type(_PY_TOOLCHAIN_TYPE, mandatory = True)],
)

def _python_runtime_files_impl(ctx):
    return DefaultInfo(files = ctx.toolchains[_PY_TOOLCHAIN_TYPE].py3_runtime.files)

python_runtime_files = rule(
    doc = "Resolves to the Python runtime files from resolved Python toolchain.",
    implementation = _python_runtime_files_impl,
    toolchains = [config_common.toolchain_type(_PY_TOOLCHAIN_TYPE, mandatory = True)],
)
