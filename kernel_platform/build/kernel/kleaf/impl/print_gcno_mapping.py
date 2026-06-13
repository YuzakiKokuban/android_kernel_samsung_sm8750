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

"""Print gcno/mapping.json"""

import argparse
import json
import pathlib


def main(file_mappings: list[pathlib.Path], mappings: list[str]):
    mappings_dict = {}
    for file_mapping in file_mappings:
        with file_mapping.open() as file_mapping_content:
            for mapping in json.load(file_mapping_content):
                mappings_dict[mapping["from"]] = mapping["to"]

    for mapping in mappings:
        from_val, to_val = mapping.split(":")
        mappings_dict[from_val] = to_val

    result = [
        {"from": from_val, "to": to_val}
        for from_val, to_val in sorted(mappings_dict.items())
    ]

    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--file_mappings", type=pathlib.Path,
                        nargs="*", default=[])
    parser.add_argument("--mappings", nargs="*", metavar="FROM:TO", default=[])
    main(**vars(parser.parse_args()))
