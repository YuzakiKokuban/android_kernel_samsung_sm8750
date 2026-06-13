#!/bin/bash

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

# Example usage:
#   export ROOT_DIR=$(./gettop.sh)

# This script is located at ${ROOT_DIR}/build/{kernel/,}gettop.sh.
# TODO(b/204425264): remove hack once we cut over to build/kernel/ for branches

# This is either ${ROOT_DIR}/build or ${ROOT_DIR}/build/kernel
parent_dir=$(dirname $(readlink -f $0))

if [[ $(basename ${parent_dir}) == "kernel" ]]; then
  echo $(dirname $(dirname ${parent_dir}))
else
  echo $(dirname ${parent_dir})
fi
