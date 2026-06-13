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

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")

def _failure_test_impl(ctx):
    env = analysistest.begin(ctx)
    for substr in ctx.attr.error_message_substrs:
        asserts.expect_failure(env, substr)
    return analysistest.end(env)

failure_test = analysistest.make(
    doc = """An analysis test that expects failure on the given target with certain messages.

Example:

```
failure_test(
    name = "my_test",
    target_under_test = ":my_target_under_test",
    error_message_substrs = [
        "foo must be bar",
        "not valid format",
    ],
)
```

This expects that, when running the analysis phase on `my_target_under_test`,
an error must be generated with a message containing the substring
`foo must be bar` and the substring `not valid format`.

""",
    impl = _failure_test_impl,
    attrs = {
        "error_message_substrs": attr.string_list(doc = "A list of substrings to be expected in the error message."),
    },
    expect_failure = True,
)
