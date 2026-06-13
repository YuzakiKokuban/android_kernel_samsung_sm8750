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

"""Impl of `exec_aspect`."""

visibility("//build/bazel_common_rules/exec/...")

_attrs = ["args", "env", "data", "srcs", "deps"]

ExecAspectInfo = provider(
    doc = "See [`exec_aspect`](#exec_aspect).",
    fields = {attr: attr + " of the target" for attr in _attrs},
)

def _aspect_impl(_target, ctx):
    kwargs = {}
    for attr in _attrs:
        value = getattr(ctx.rule.attr, attr, None)
        kwargs[attr] = value
    return ExecAspectInfo(**kwargs)

exec_aspect = aspect(
    implementation = _aspect_impl,
    doc = "Make arguments available for targets depending on executables.",
    attr_aspects = _attrs,
)
