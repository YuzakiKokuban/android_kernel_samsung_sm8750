# Copyright (C) 2023 The Android Open Source Project
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

import os
import pathlib
import shutil
import sys
import time

with open(pathlib.Path(__file__).parent / "raw_test_result_dir_value") as f:
    raw_test_result_dir = pathlib.Path(f.read().strip())

with open(raw_test_result_dir / "stdout.txt") as f:
    shutil.copyfileobj(f, sys.stdout)
with open(raw_test_result_dir / "stderr.txt") as f:
    shutil.copyfileobj(f, sys.stderr)
with open(raw_test_result_dir / "exitcode.txt") as f:
    exit_code = int(f.read().strip())
shutil.copyfile(raw_test_result_dir / "output.xml", os.environ["XML_OUTPUT_FILE"])

print(f"XML_OUTPUT_FILE={os.environ['XML_OUTPUT_FILE']}")

# TODO(b/272135682): Build bot fails to report results properly when a
# bazel test command executes < 10s.
time.sleep(10)

sys.exit(exit_code)
