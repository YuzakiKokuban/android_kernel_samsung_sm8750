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

"""Utility functions to build arguments to `scripts/config`."""

visibility("//build/kernel/kleaf/...")

def _enable(config):
    return "--enable {}".format(config)

def _disable(config):
    return "--disable {}".format(config)

def _set_str(config, value):
    return "--set-str {} {}".format(config, value)

def _set_val(config, value):
    return "--set-val {} {}".format(config, value)

def _do_action_if(config, condition, action):
    """Returns an argument to `scripts/config` that does `action` to the config if the conditional config is enabled."""
    return """$(
        if [[ "$(${{KERNEL_DIR}}/scripts/config --file ${{OUT_DIR}}/.config --state {condition})" == "y" ]]; then
            echo "--{action} {config}"
        fi
    )""".format(config = config, condition = condition, action = action)

def _enable_if(config, condition):
    return _do_action_if(config, condition, "enable")

def _disable_if(config, condition):
    return _do_action_if(config, condition, "disable")

scripts_config_arg_builder = struct(
    disable = _disable,
    enable = _enable,
    set_str = _set_str,
    set_val = _set_val,
    enable_if = _enable_if,
    disable_if = _disable_if,
)
