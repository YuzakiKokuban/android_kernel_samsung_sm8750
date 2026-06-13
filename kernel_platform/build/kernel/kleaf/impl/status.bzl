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

"""
Utility functions to get variables from stable-status.txt and volatile-status.txt.
See https://bazel.build/docs/user-manual#workspace-status-command
"""

visibility("//build/kernel/kleaf/...")

def _get_status_cmd(_ctx, status_file, var):
    return """ (while read -r name value; do if [ "$name" = "{var}" ]; then echo "$value"; fi; done < {status}) """.format(
        status = status_file.path,
        var = var,
    )

def _get_stable_status_cmd(ctx, var):
    """Return the command line that gets a variable `var` from `stable-status.txt`.

    Require the action with this cmd to:
    - add `ctx.info_file` to inputs
    - Set up hermetic tools before calling this command.

    Args:
        ctx: [ctx](https://bazel.build/rules/lib/ctx)
        var: name of variable, prefixed with `STABLE`.
    """
    return _get_status_cmd(ctx, ctx.info_file, var)

def _get_volatile_status_cmd(ctx, var):
    """Return the command line that gets a variable `var` from `volatile-status.txt`.

    Require the action with this cmd to:
    - add `ctx.version_file` to inputs
    - Set up hermetic tools before calling this command.

    Args:
        ctx: [ctx](https://bazel.build/rules/lib/ctx)
        var: name of variable, not prefixed with `STABLE`.
    """
    return _get_status_cmd(ctx, ctx.version_file, var)

status = struct(
    get_stable_status_cmd = _get_stable_status_cmd,
    get_volatile_status_cmd = _get_volatile_status_cmd,
)
