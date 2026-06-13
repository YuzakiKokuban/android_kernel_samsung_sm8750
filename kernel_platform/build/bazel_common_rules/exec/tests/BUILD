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

# BUILD
load("//build/bazel_common_rules/exec/impl:embedded_exec.bzl", "embedded_exec")
load("//build/bazel_common_rules/exec/impl:exec.bzl", "exec")

exec(
    name = "script_a",
    args = [
        "--argsA=valueA",
        "--args_expanded=$(rootpath data.txt)",
    ],
    data = [":data.txt"],
    script = "echo script_a $@",
)

sh_binary(
    name = "script_b",
    srcs = ["script_b.sh"],
    args = [
        "--script_b_arg=value",
        "--args_expanded=$(rootpath data.txt)",
    ],
    data = [":data.txt"],
    env = {
        "SCRIPT_B_ENV": "env_value",
        "SCRIPT_B_ENV_EXPANDED": "$(rootpath data.txt)",
    },
)

exec(
    name = "cat_data",
    data = [":data.txt"],
    script = "cat $(rootpath :data.txt)",
)

embedded_exec(
    name = "script_a_embedded",
    actual = "script_a",
)

embedded_exec(
    name = "script_b_embedded",
    actual = "script_b",
)

exec(
    name = "combined",
    args = ["--script_a_path=$(rootpath :script_a_embedded)"],
    data = [
        ":cat_data",
        ":script_a_embedded",
        ":script_b_embedded",
    ],
    script = """
        echo combined_args=$@
        $(rootpath :script_a_embedded)
        $(rootpath :script_b_embedded)
        $(rootpath :cat_data)
    """,
)

py_test(
    name = "exec_test",
    srcs = ["exec_test.py"],
    args = ["$(location :combined)"],
    data = [":combined"],
    deps = [
        "@io_abseil_py//absl/testing:absltest",
    ],
)

test_suite(
    name = "tests",
    tests = [
        ":exec_test",
    ],
)
