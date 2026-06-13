#!/usr/bin/env python3

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

"""Kleaf SBOM generator: Generate SBOM for kernel build.

Inputs:
1. --version: The android kernel build version string.
              example: 5.15.110-android14-11-00098-gbdd2312e95c7-ab10365441
2. --dist_dir: Output dir where all the kernel build artifacts are.
              example: out/kernel_aarch64/dist
3. --output_file: File where SBOM should be written.
              example: kernel_sbom.spdx.json

Examples:

    # Generate SBOM after a kernel build with dist.
    build/kernel/kleaf/kernel_sbom.py \
      --version "5.15.110-android14-11-00098-gbdd2312e95c7-ab10365441" \
      --dist_dir "out/kernel_aarch64/dist" \
      --output_file "kernel_sbom.spdx.json"
"""

import argparse
from collections.abc import Iterable
import dataclasses
import datetime
import hashlib
import json
import os
import pathlib
import re
import subprocess
from typing import Any


_SPDX_VERSION = "SPDX-2.3"
_DATA_LICENSE = "CC0-1.0"
_GOOGLE_ORGANIZATION_NAME = "Google"
_LINUX_ORGANIZATION_NAME = "The Linux Kernel Organization"
_LINUX_UPSTREAM_WEBSITE = "https://www.kernel.org"
_NAMESPACE_PREFIX = "https://www.google.com/sbom/spdx/android/kernel/"
_MAIN_PACKAGE_NAME = "kernel"
_SOURCE_CODE_PACKAGE_NAME = "KernelSourceCode"
_SOURCE_CODE_DOWNLOAD_LOCATION = "https://source.android.com"
_LINUX_UPSTREAM_PACKAGE_NAME = "LinuxUpstreamPackage"
_GENERATED_FROM_RELATIONSHIP = "GENERATED_FROM"
_VARIANT_OF_RELATIONSHIP = "VARIANT_OF"
_SPDX_REF = "SPDXRef"


def _spdx_id(identifier: str):
  # the id string is a "unique string containing letters, numbers, . and/or -."
  # https://spdx.github.io/spdx-spec/v2.3/file-information/#82-file-spdx-identifier-field
  sanitized_identifier = re.sub(r"[^0-9a-zA-Z-\.]+", "-", identifier)
  return f"{_SPDX_REF}-{sanitized_identifier}"


@dataclasses.dataclass(order=True)
class File:
  id: str
  name: str
  path: pathlib.Path
  checksum: str
  build_id: str | None


class KernelSbom:

  def __init__(
      self,
      android_kernel_version: str,
      file_list: Iterable[pathlib.Path],
      readelf: pathlib.Path,
  ):
    self._android_kernel_version = android_kernel_version
    self._upstream_kernel_version = android_kernel_version.split("-")[0]
    self._files = sorted(
        [
            File(
                id=_spdx_id(file.name),
                name=file.name,
                path=file,
                checksum=self._checksum(file),
                build_id=self._build_id(file, readelf)
            )
            for file in file_list
        ]
    )
    self._sbom_doc = self._generate_sbom()

  # replacement for 3.11 hashlib.file_digest(), adopted from upstream CPython
  def _file_digest(self, fileobj, algorithm: str):
    digestobj = hashlib.new(algorithm)

    # We only support binary file objects
    buf = bytearray(2**18)  # Reusable buffer to reduce allocations.
    view = memoryview(buf)
    while True:
      size = fileobj.readinto(buf)
      if size == 0:
        break  # EOF
      digestobj.update(view[:size])

    return digestobj

  def _checksum(self, file_path: pathlib.Path) -> str:
    with file_path.open("rb") as f:
      if hasattr(hashlib, "file_digest"):
        digest = hashlib.file_digest(f, "sha1")
      else:
        digest = self._file_digest(f, "sha1")
      return str(digest.hexdigest())

  def _generate_package_verification_code(self, files: list[File]) -> str:
    combined_checksum = hashlib.sha1()
    for checksum in sorted(f.checksum.encode() for f in files):
      combined_checksum.update(checksum)
    return combined_checksum.hexdigest()

  def _generate_doc_headers(self) -> dict[str, Any]:
    timestamp = datetime.datetime.now(tz=datetime.timezone.utc).strftime(
        "%Y-%m-%dT%H:%M:%SZ"
    )
    namespace = os.path.join(_NAMESPACE_PREFIX, self._android_kernel_version)
    headers = {
        "spdxVersion": _SPDX_VERSION,
        "dataLicense": _DATA_LICENSE,
        "SPDXID": _spdx_id("DOCUMENT"),
        "name": self._android_kernel_version,
        "documentNamespace": namespace,
        "creationInfo": {
            "creators": [f"Organization: {_GOOGLE_ORGANIZATION_NAME}"],
            "created": timestamp,
        },
        "documentDescribes": [f"SPDXRef-{_MAIN_PACKAGE_NAME}"],
    }
    return headers

  def _build_id(
      self, file_path: pathlib.Path, readelf: pathlib.Path
  ) -> str | None:
    if file_path.name != "vmlinux" and file_path.suffix != ".ko":
      return None

    out = subprocess.check_output([readelf, "--notes", file_path]).decode()
    build_id = None
    for line in out.splitlines():
      if "Build ID:" in line:
        assert(build_id is None)
        build_id = line.strip()
    return build_id

  def _generate_package_dict(
      self,
      version: str,
      package_name: str,
      file_list: list[File],
      organization: str,
      download_location: str,
  ) -> dict[str, Any]:
    package_dict: dict[str, Any] = {
        "name": package_name,
        "SPDXID": _spdx_id(package_name),
        "downloadLocation": download_location,
        "filesAnalyzed": False,
        "versionInfo": version,
        "supplier": f"Organization: {organization}",
    }
    if file_list:
      package_dict["hasFiles"] = [file.id for file in file_list]
      verification_hash = self._generate_package_verification_code(file_list)
      package_dict["packageVerificationCode"] = {
          "packageVerificationCodeValue": verification_hash
      }
      package_dict["filesAnalyzed"] = True
    return package_dict

  def _generate_file_dict(self, file: File) -> dict[str, Any]:
    result = {
        "fileName": file.name,
        "SPDXID": file.id,
        "checksums": [
            {
                "algorithm": "SHA1",
                "checksumValue": file.checksum,
            },
        ],
    }
    if file.build_id is not None:
      result.update(comment=file.build_id)

    return result

  def _generate_relationship_dict(
      self, element: str, related_element: str, relationship_type: str
  ) -> dict[str, str]:
    return {
        "spdxElementId": element,
        "relatedSpdxElement": related_element,
        "relationshipType": relationship_type,
    }

  def _generate_sbom(self) -> dict[str, Any]:
    sbom = self._generate_doc_headers()
    sbom["packages"] = [
        self._generate_package_dict(
            self._android_kernel_version,
            _MAIN_PACKAGE_NAME,
            self._files,
            _GOOGLE_ORGANIZATION_NAME,
            _SOURCE_CODE_DOWNLOAD_LOCATION,
        ),
        self._generate_package_dict(
            self._android_kernel_version,
            _SOURCE_CODE_PACKAGE_NAME,
            [],
            _GOOGLE_ORGANIZATION_NAME,
            _SOURCE_CODE_DOWNLOAD_LOCATION,
        ),
        self._generate_package_dict(
            self._upstream_kernel_version,
            _LINUX_UPSTREAM_PACKAGE_NAME,
            [],
            _LINUX_ORGANIZATION_NAME,
            _LINUX_UPSTREAM_WEBSITE,
        ),
    ]
    sbom["files"] = [self._generate_file_dict(f) for f in self._files]

    sbom["relationships"] = [
        self._generate_relationship_dict(
            _spdx_id(_MAIN_PACKAGE_NAME),
            _spdx_id(_SOURCE_CODE_PACKAGE_NAME),
            _GENERATED_FROM_RELATIONSHIP,
        ),
        self._generate_relationship_dict(
            _spdx_id(_SOURCE_CODE_PACKAGE_NAME),
            _spdx_id(_LINUX_UPSTREAM_PACKAGE_NAME),
            _VARIANT_OF_RELATIONSHIP,
        ),
    ] + [
        self._generate_relationship_dict(
            f.id,
            _spdx_id(_SOURCE_CODE_PACKAGE_NAME),
            _GENERATED_FROM_RELATIONSHIP,
        )
        for f in self._files
    ]

    return sbom

  def write_sbom_file(self, output_path: pathlib.Path):
    # omit all error handling to fatally fail with stacktrace in that case
    with output_path.open("w") as output_file:
      json.dump(self._sbom_doc, output_file, indent=4)


def get_args():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "--output_file",
      required=True,
      type=pathlib.Path,
      help="The generated SBOM file in SPDX format.",
  )
  dist_group = parser.add_mutually_exclusive_group(required=True)
  dist_group.add_argument(
      "--dist_dir",
      type=pathlib.Path,
      help="Directory containing generated artifacts.",
  )
  dist_group.add_argument(
      "--files",
      nargs="+",
      type=pathlib.Path,
      help="Explicit list of files to consider for SBOM generation.",
  )
  version_group = parser.add_mutually_exclusive_group(required=True)
  version_group.add_argument("--version", help="The android kernel version.")
  version_group.add_argument(
      "--version_file",
      type=pathlib.Path,
      help="path to the kernel.release file",
  )
  parser.add_argument(
      "--readelf",
      required=True,
      type=pathlib.Path, help="The readelf binary to process binaries.",
  )
  return parser.parse_args()


def get_file_list(dist_dir: pathlib.Path) -> Iterable[pathlib.Path]:
  if dist_dir.is_dir():
    return [p for p in pathlib.Path(dist_dir).glob("*") if p.is_file()]
  else:
    raise FileNotFoundError(
        f"Distribution directory '{dist_dir}' is not a directory."
    )


def read_version_from_file(version_file: pathlib.Path):
  with version_file.open() as f:
    return f.read().strip()


def main():
  args = get_args()
  files = args.files or get_file_list(args.dist_dir)
  version = args.version or read_version_from_file(args.version_file)
  sbom = KernelSbom(version, files, args.readelf)
  sbom.write_sbom_file(args.output_file)


if __name__ == "__main__":
  main()
