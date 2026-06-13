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

"""Provides utility functions for tests."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")

def _find_action(env, mnemonic, actions = None):
    """Finds an action with the given mnemonic.

    Args:
        env: env
        actions: a list of actions. If None, use actions from env
        mnemonic: expected mnemonic.
    """
    if actions == None:
        actions = analysistest.target_actions(env)

    mnemonics = []
    for action in actions:
        if action.mnemonic == mnemonic:
            return action
        mnemonics.append(action.mnemonic)

    asserts.true(env, False, "No matching action with mnemonic {} found in {}".format(mnemonic, mnemonics))
    return None

def _find_output(action, basename):
    """Finds the output with the given basename from the given action.

    Args:
        action: The action that expects to produce the output
        basename: The expected basename of the output

    Returns:
        The output file, or None if not found.
    """
    for output in action.outputs.to_list():
        if output.basename == basename:
            return output

    return None

def _get_shell_script(env, action):
    """Assuming the action is a `run_shell`, returns the script.

    Args:
        env: env
        action: the action.
    """
    argv = action.argv
    asserts.equals(env, 3, len(argv), "run_shell action should contain 3 args")
    return argv[2]

test_utils = struct(
    find_action = _find_action,
    find_output = _find_output,
    get_shell_script = _get_shell_script,
)
