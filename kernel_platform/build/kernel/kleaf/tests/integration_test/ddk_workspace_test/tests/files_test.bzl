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

"""Tests that a target has the given files."""

def _single_file_test_impl(ctx):
    files = ctx.files.src
    if len(files) != 1:
        fail("Zero or multiple files encountered: {}".format(files))
    if files[0].basename != ctx.attr.expected_basename:
        fail("Expected: {}, actual: {}".format(ctx.attr.expected_basename, files[0]))

    test_script = ctx.actions.declare_file(ctx.attr.name)
    ctx.actions.write(test_script, "", is_executable = True)
    return DefaultInfo(
        executable = test_script,
        files = depset([test_script]),
    )

single_file_test = rule(
    doc = "Test that the target has a single file",
    implementation = _single_file_test_impl,
    attrs = {
        "src": attr.label(doc = "target under test", allow_files = True),
        "expected_basename": attr.string(doc = "expected file name"),
    },
    test = True,
)

def _files_test_impl(ctx):
    actual_basenames = [file.basename for file in ctx.files.src]
    for expected_basename in ctx.attr.expected_basenames:
        if expected_basename not in actual_basenames:
            fail("Can't find a file named {} in {}".format(expected_basename, ctx.files.src))

    test_script = ctx.actions.declare_file(ctx.attr.name)
    ctx.actions.write(test_script, "", is_executable = True)
    return DefaultInfo(
        executable = test_script,
        files = depset([test_script]),
    )

files_test = rule(
    doc = "Test that the target has the given files. Extra files are okay.",
    implementation = _files_test_impl,
    attrs = {
        "src": attr.label(doc = "target under test", allow_files = True),
        "expected_basenames": attr.string_list(doc = "expected file names"),
    },
    test = True,
)
