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

"""Utility functions for supporting kgdb."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load(":scripts_config_arg_builder.bzl", _config = "scripts_config_arg_builder")

visibility("//build/kernel/kleaf/...")

def _kgdb_config_settings_raw():
    """Attributes of rules that supports kgdb."""
    return {
        "_kgdb": "//build/kernel/kleaf:kgdb",
    }

def _get_grab_gdb_scripts_step(ctx):
    """Returns a step for grabbing gdb scripts.

    Args:
        ctx: kernel_build ctx

    Returns:
        A struct with these fields:
        * inputs
        * tools
        * outputs
        * cmd
    """

    outputs = []
    cmd = ""
    if ctx.attr._kgdb[BuildSettingInfo].value:
        kgdb = ctx.actions.declare_directory("{name}/gdb_scripts".format(name = ctx.label.name))
        outputs.append(kgdb)
        cmd = """
            (
                kgdb_real=$(realpath {kgdb})
                cd ${{OUT_DIR}}
                cp --parents -aL -t ${{kgdb_real}} vmlinux-gdb.py scripts/gdb/linux/*.py
            )
        """.format(kgdb = kgdb.path)

    return struct(
        inputs = [],
        tools = [],
        cmd = cmd,
        outputs = outputs,
    )

def _additional_make_goals(ctx):
    """Returns a list of additional `MAKE_GOALS`.

    Args:
        ctx: ctx
    """
    if ctx.attr._kgdb[BuildSettingInfo].value:
        return ["scripts_gdb"]
    return []

def _get_scripts_config_args(ctx):
    """Returns arguments to `scripts/config` for --kgdb.

    Args:
        ctx: ctx
    Returns:
        a list of arguments to `scripts/config`
    """
    if not ctx.attr._kgdb[BuildSettingInfo].value:
        return []
    return [
        _config.enable("GDB_SCRIPTS"),
        _config.enable("KGDB"),
        _config.enable("KGDB_KDB"),
        _config.disable("RANDOMIZE_BASE"),
        _config.disable("STRICT_KERNEL_RWX"),
        _config.enable("VT"),
        _config.disable("VT_CONSOLE"),
        _config.disable("WATCHDOG"),
        _config.enable_if("KGDB_LOW_LEVEL_TRAP", condition = "X86"),
    ]

kgdb = struct(
    get_grab_gdb_scripts_step = _get_grab_gdb_scripts_step,
    config_settings_raw = _kgdb_config_settings_raw,
    additional_make_goals = _additional_make_goals,
    get_scripts_config_args = _get_scripts_config_args,
)
