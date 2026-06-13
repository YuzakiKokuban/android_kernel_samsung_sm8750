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

# A test that does nothing.

def _empty_test_impl(ctx):
    exec = ctx.actions.declare_file("{}.sh".format(ctx.label.name))
    ctx.actions.write(exec, "", is_executable = True)
    return DefaultInfo(executable = exec)

empty_test = rule(
    implementation = _empty_test_impl,
    test = True,
)
