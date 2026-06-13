#! /bin/bash
# Copyright (c) 2020, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Changes from Qualcomm Innovation Center are provided under the following license:
# Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

ROOT_DIR=$($(dirname $(readlink -f $0))/../gettop.sh)

set -e

source "${ROOT_DIR}/build/_setup_env.sh"

rm -rf $3
mkdir $3

set -x
$ROOT_DIR/build/android/merge_dtbs.py --base $1 --techpack $2 --out $3
set +x

[[ -n "$(find ${3} -type f -name '*.dtb')" ]] && cat ${3}/*.dtb > ${3}/dtb.img
[[ -n "$(find ${3} -type f -name '*.dtbo')" ]] && mkdtboimg create ${3}/dtbo.img --page_size=${PAGE_SIZE} ${3}/*.dtbo

# SS : make final reverse dt ($3=device/qcom/gki-kernel/dtbs)
FILES=$(find ${3} -type f -name "*.dtb*")

for FILE in ${FILES}; do
  REVERSE_NAME=$(basename ${FILE}).reverse.dts
  dtc -O dts -I dtb -o ${3}/${REVERSE_NAME} ${FILE}
done

exit 0
