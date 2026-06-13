#!/bin/bash -e
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

# Script used as --workspace_status_command with --config=stamp.
# https://docs.bazel.build/versions/main/command-line-reference.html#flag--workspace_status_command

KLEAF_REPO_DIR=$($(dirname $(dirname $(readlink -f "$0")))/gettop.sh)

${KLEAF_REPO_DIR}/build/kernel/kleaf/workspace_status_common.sh
${KLEAF_REPO_DIR}/prebuilts/build-tools/path/linux-x86/python3 ${KLEAF_REPO_DIR}/build/kernel/kleaf/workspace_status_stamp.py
