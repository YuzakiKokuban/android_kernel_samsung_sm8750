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

"""Extracts the string representing the KMI from the kernel release string.

$ python3 get_kmi_string.py 5.15.123-android14-6-something
5.15-android14-6

$ python3 get_kmi_string.py --keep_sublevel 5.15.123-android14-6-something
5.15.123-android14-6

$ python3 get_kmi_string.py 6.1.55-mainline
6.1-mainline
"""

import argparse
import logging
import re
import sys


def get_kmi_string(kernel_release: str, keep_sublevel: bool) -> str:
    """Extracts the string representing the KMI from the kernel release string.

    Check versioning scheme here:

    https://source.android.com/docs/core/architecture/kernel/gki-versioning

    Args:
        kernel_release: the kernel release string to parse
        keep_sublevel: whether sublevel is kept in the output

    Returns:
        A string representing the KMI.

    >>> get_kmi_string("5.15.123", False)
    '5.15'

    >>> get_kmi_string("5.15.123", True)
    '5.15.123'

    >>> get_kmi_string("5.15.123-android14-6", True)
    '5.15.123-android14-6'

    >>> get_kmi_string("5.15.123-android14-6-something", True)
    '5.15.123-android14-6'

    >>> get_kmi_string("5.15.123-android14-6", False)
    '5.15-android14-6'

    >>> get_kmi_string("5.15.123-android14-6-something", False)
    '5.15-android14-6'

    >>> get_kmi_string("6.1.55-mainline", False)
    '6.1-mainline'

    >>> get_kmi_string("6.1.55-mainline-something", False)
    '6.1-mainline'

    >>> get_kmi_string("6.1.55-mainline-something", True)
    '6.1.55-mainline'
    """

    ver_pat = re.compile(
        r"^(?P<version>\d+)\.(?P<patch>\d+)\.(?P<sublevel>\d+).*")
    ver_mo = ver_pat.match(kernel_release)
    if not ver_mo:
        logging.error("Unrecognized kernel release %s. This is not a valid GKI version. See "
                      "https://source.android.com/docs/core/architecture/kernel/gki-versioning."
                      "Check early warnings in the build log for details.",
                      kernel_release)
        sys.exit(1)

    version = ver_mo.group("version")
    patch_level = ver_mo.group("patch")
    sublevel = ver_mo.group("sublevel")

    ver_string = f"{version}.{patch_level}"
    if keep_sublevel:
        ver_string += f".{sublevel}"

    if "mainline" in kernel_release.split("-"):
        return f"{ver_string}-mainline"

    kmi_pat = re.compile(
        r"^(\d+)\.(\d+)\.(\d+)-(?P<release>android\d+)-(?P<gen>\d+)(?:-.*)?$")
    kmi_mo = kmi_pat.match(kernel_release)
    if not kmi_mo:
        logging.warning("Unrecognized kernel release %s. This is not a valid GKI version. See "
                        "https://source.android.com/docs/core/architecture/kernel/gki-versioning."
                        "Check early warnings in the build log for details.",
                        kernel_release)
        return ver_string

    android_release = kmi_mo.group("release")
    kmi_generation = kmi_mo.group("gen")
    return f"{ver_string}-{android_release}-{kmi_generation}"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--keep_sublevel", action="store_true")
    parser.add_argument("kernel_release")
    logging.basicConfig(stream=sys.stderr,
                        level=logging.WARNING,
                        format="%(levelname)s: %(message)s")
    result = get_kmi_string(**vars(parser.parse_args()))
    if result:
        print(result)
