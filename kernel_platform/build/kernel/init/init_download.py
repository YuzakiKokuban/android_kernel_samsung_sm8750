#!/usr/bin/env python3
#
# Copyright (C) 2024 The Android Open Source Project
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

"""Small utility to download files."""

import shutil
import sys
import traceback
import urllib.request

try:
    with urllib.request.urlopen(sys.argv[1]) as input_file, open(
        sys.argv[2], "wb"
    ) as output_file:
        shutil.copyfileobj(input_file, output_file)
except Exception as exc:  # pylint: disable=broad-exception-caught
    formatted_lines = traceback.format_exc().splitlines()
    if formatted_lines:
        print(formatted_lines[-1], file=sys.stderr)
    sys.exit(1)
