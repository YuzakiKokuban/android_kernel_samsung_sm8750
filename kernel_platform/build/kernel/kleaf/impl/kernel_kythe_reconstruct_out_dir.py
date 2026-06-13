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
Construct directory structure in $COMMON_OUT_DIR to look like $ROOT_DIR
This is done by re-creating the directories pointed by each file in
compile_commands.json with $ROOT_DIR. This is needed so the file created
by clang -MMD option in compile_commands.json has the directory.
"""

import pathlib, json, sys

common_out_dir = pathlib.Path(sys.argv[1])
compile_commands_with_vars = pathlib.Path(sys.argv[2])

dirs = set()

with open(compile_commands_with_vars) as f:
    for item in json.load(f):
        rel_file = item["file"].removeprefix("${ROOT_DIR}/")
        dirs.add((common_out_dir / rel_file).parent)

for dir in dirs:
    dir.mkdir(parents = True, exist_ok = True)
