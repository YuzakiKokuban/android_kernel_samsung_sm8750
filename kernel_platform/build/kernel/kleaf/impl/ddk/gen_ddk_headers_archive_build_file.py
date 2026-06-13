# Copyright (C) 2023 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Generate BUILD.bazel for a DDK headers archive
"""

import absl.flags.argparse_flags
import argparse
import logging
import pathlib
import textwrap
from typing import TextIO


# TODO(b/250998477): After importing buildifier, just write a simple list
#   expression and let buildifier handle formatting.
def list_to_repr(lst: list[pathlib.Path], indent=None):
    """Similar to repr(lst), but prettified & use double quotes."""
    result = f"["
    if lst:
        result += "\n"
    result += f"".join(f'{indent}{indent}"{elem}",\n' for elem in sorted(lst))
    if lst:
        result += f"{indent}"
    result += "]"
    return result


def gen_ddk_headers_archive_build_file(
    name: str,
    hdrs: list[pathlib.Path],
    linux_includes: list[pathlib.Path],
    includes: list[pathlib.Path],
    out: TextIO,
):
    indent = "    "
    out.write(textwrap.dedent("""\
        ddk_headers(
            name = "{name}",
            hdrs = {hdrs_repr},
            linux_includes = {linux_includes_repr},
            includes = {includes_repr},
            visibility = ["//visibility:public"],
        )
    """).format(
        name=name,
        hdrs_repr=list_to_repr(hdrs, indent=indent),
        linux_includes_repr=list_to_repr(linux_includes, indent=indent),
        includes_repr=list_to_repr(includes, indent=indent),
    ))


if __name__ == "__main__":
    # argparse_flags.ArgumentParser only accepts --flagfile if there
    # are some DEFINE'd flags
    # https://github.com/abseil/abseil-py/issues/199
    absl.flags.DEFINE_string("flagfile_hack_do_not_use", "", "")

    logging.basicConfig(level=logging.INFO,
                        format="%(levelname)s: %(message)s")

    parser = absl.flags.argparse_flags.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--hdrs",
                        type=pathlib.Path, nargs="*", default=[])
    parser.add_argument("--linux-includes",
                        type=pathlib.Path, nargs="*", default=[])
    parser.add_argument("--includes", type=pathlib.Path,
                        nargs="*", default=[])
    parser.add_argument("--out", type=argparse.FileType("w"), required=True)

    args = parser.parse_args()

    gen_ddk_headers_archive_build_file(**vars(args))
