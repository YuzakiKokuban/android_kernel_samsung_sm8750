#!/bin/bash -e

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

# This script is used as a test after
#  `tools/bazel //common:kernel_aarch64_abi_dist``
# to check if the build reported ABI differences. It exits with non-zero error
# code if ABI report file is missing or when it is not empty.

# Example:
#   tools/bazel //common:kernel_aarch64_abi_dist
#   build/kernel/abi_compliance.sh out_abi/kernel_aarch64/dist

exit_code=0
for dist_dir in "$@"
do
    abi_report_path="${dist_dir}/abi_stgdiff/abi.report.short"
    abi_report=$(cat "${abi_report_path}")

    if [ -n "${abi_report}" ]; then
        echo 'ERROR: ABI DIFFERENCES HAVE BEEN DETECTED!' >&2
        echo "ERROR: From ${abi_report_path}:" >&2
        echo >&2
        cat "${abi_report_path}" >&2
        exit_code=1
    else
        echo "INFO: no ABI differences reported in ${abi_report_path}."
    fi
done
exit ${exit_code}
