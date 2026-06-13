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

"""Utility functions for debugging Kleaf."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

visibility("//build/kernel/kleaf/...")

def _print_scripts(ctx, command, what = None):
    """Print scripts at analysis phase.

    Requires `_debug_print_scripts` to be in `attrs`.

    Args:
        ctx: [ctx](https://bazel.build/rules/lib/ctx)
        command: The command passed to `ctx.actions.run_shell`
        what: an optional text to distinguish actions within a target.
    """
    if ctx.attr._debug_print_scripts[BuildSettingInfo].value:
        # buildifier: disable=print
        print("""
        # Script that runs %s%s:%s""" % (ctx.label, (" " + what if what else ""), command))

def _trap():
    """Return a shell script that prints a date before each command afterwards.
    """
    return """set -x
              trap '>&2 /bin/date' DEBUG"""

def _modpost_warn(ctx):
    """Returns useful script snippets and files for --debug_modpost_warn."""

    if not ctx.attr._debug_modpost_warn[BuildSettingInfo].value:
        return struct(
            cmd = "",
            outputs = [],
            make_redirect = "",
        )

    cmd = """
          export KBUILD_MODPOST_WARN=1
          """

    # This file is used in build_cleaner.
    make_stderr = ctx.actions.declare_file("{}/make_stderr.txt".format(ctx.attr.name))
    make_redirect = " 2> >(tee {} >&2)".format(make_stderr.path)

    return struct(
        cmd = cmd,
        outputs = [make_stderr],
        make_redirect = make_redirect,
    )

debug = struct(
    print_scripts = _print_scripts,
    trap = _trap,
    modpost_warn = _modpost_warn,
)
