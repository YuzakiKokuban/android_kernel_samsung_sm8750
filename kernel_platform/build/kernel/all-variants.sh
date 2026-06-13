#!/bin/bash

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

set -e

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
	echo """
$0 <command>

Loop through all build variants

If no <command> is given, then the variant environment variables are printed
If <command> is given, then the variant environment variables are set when calling <command>

If BUILD_CONFIG is set, then all-variants.sh loops over all variants of that BUILD_CONFIG
If BUILD_CONFIG is not set, then all-variants.sh loops over all found target configs (those listed in build.targets)

The following environment variables are provided:
   BUILD_CONFIG - the path to the build.config file for the target
   VARIANT - a build.config variant
   BRANCH - when not specifying an output directory, output is put in out/${BRANCH}.

Example 1:
./build/all-variants.sh \"./build/build.sh\"
	Invokes ./build/build.sh for each BUILD_CONFIG/VARIANT combo

Example 2:
function do_build() {
	OUT_DIR=./output BUILD_CONFIG=\${BUILD_CONFIG} VARIANT=\${VARIANT} ./build/build.sh 2>&1 | tee ${BRANCH}.log
	if [ \$? -ne \"0\" ]; then
		echo ${BRANCH} build failed!
	fi
	rm -rf ./output
}
./build/all-variants.sh do_build
	Invokes do_build function for each BUILD_CONFIG/VARIANT combo, which compiles each kernel
	in a temporary folder. Build logs go in \${BRANCH}.log
"""
	exit
fi

function do_list_variants() (
	local ROOT_DIR=$(readlink -f $(dirname $0)/..)

	source "${ROOT_DIR}/build/_wrapper_common.sh"

	if [ -n "${BUILD_CONFIG}" ]; then
		BUILD_CONFIGS=("${BUILD_CONFIG}")
	else
		create_targets_array BUILD_CONFIGS
	fi

	for target in "${BUILD_CONFIGS[@]}"
	do
		variants=()
		create_variants_array variants "${target}"

		if [ "${#variants[@]}" -eq 0 ]; then
			echo "BUILD_CONFIG=${target} BRANCH=`get_branch`"
		fi

		for variant in "${variants[@]}"
		do
			echo "BUILD_CONFIG=${target} VARIANT=${variant} BRANCH=`get_branch`"
		done
	done
)

if [ -n "$*" ]; then
	while read variant; do
		unset do_list_variants
		echo "${variant}"
		# Now, export those variables to the requested command
		${SHELL} -c "set -a; ${variant// /;}; set +a; $*"
	done < <(do_list_variants)
else
	do_list_variants
fi
