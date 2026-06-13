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

################################################################################

# create_targets_array: create an array containing a list of all found target build.configs
# The first argument is the name of the array to store the targets in.
#    Each element in array is a valid BUILD_CONFIG value relative to root dir
# The second argument is the name of the array to store the directory where the target build.configs
#    were found. (optional)
function create_targets_array() {
	local target_arr=$1
	local origin_arr="${2:-tmp_origin}"

	eval "$target_arr=()"
	eval "$origin_arr=()"

	while read targetsfile
	do
		# readlink the targetsdir? No use case currently, so don't
		targetsdir=$(dirname "$targetsfile")
		while read buildconfig
		do
			# config files listed in a build.targets file are relative to the directory
			# of the build.targets file, thus make sure to prepend the build.targets'
			# location to the config file listed
			buildconfig="$targetsdir/$buildconfig"
			if ! [ -f "$buildconfig" ]; then
				continue
			fi
			eval "$target_arr+=(\"\$buildconfig\")"
			eval "$origin_arr+=(\"\$targetsdir\")"
		done < <(cat $targetsfile)
	done < <(find -maxdepth 5 -name build.targets | sort)
}

# list_targets: lists all found target build.configs, one per line
function list_targets() {
	local tmp_targets

	create_targets_array tmp_targets
	for target in "${tmp_targets[@]}"
	do
		echo "${target}"
	done
}

################################################################################

# _list_variants: helper function which sources the kernel/build environment and prints all the
#    variants. Runs in subshell so outer environment isn't polluted by kernel/build's configuration
function _list_variants() (
	if [ -n "$1" ]; then
		BUILD_CONFIG=$1
	fi

	source "${ROOT_DIR}/build/_setup_env.sh"

	echo Possible Variants:
	for variant in "${VARIANTS[@]}"
	do
		echo "${variant}"
	done
)

# create_variants_array: create an array containing a list of all found variants for a BUILD_CONFIG.
#    BUILD_CONFIG may be mentioned by environment variable or as the second argument
# The first argument is the name of the array to store the variants in.
# The second argument is the target BUILD_CONFIG to use.
#    Optional if BUILD_CONFIG is set.
function create_variants_array() {
	eval "$1=()"
	# We need to source the build environment and query the variants.
	# However, we also do not want to pollute the current environment with all
	# of kernel/builds configuration. In a subshell, source the environment and
	# print:
	# Possible Variants:
	# varianta
	# variantb
	while read variant; do
		eval "$1+=(\"\$variant\")"
	done < <(_list_variants $2 2> /dev/null | awk '/Possible Variants:/{p=1}p' | tail -n+2)
}

# list_variants: lists all variants for a target build.config, one per line
# The first argument is the target BUILD_CONFIG to use.
#    Optional if BUILD_CONFIG is set.
function list_variants() {
	local tmp_variants

	create_variants_array tmp_variants $1
	for variant in "${tmp_variants[@]}"
	do
		echo "${variant}"
	done
}

################################################################################

function _get_branch() {
	BUILD_CONFIG=${target:-${BUILD_CONFIG}}
	VARIANT=${variant:-${VARIANT}}

	source "${ROOT_DIR}/build/_setup_env.sh"

	echo Branch:
	echo "${BRANCH}"
}

function get_branch() {
	_get_branch 2> /dev/null | awk '/Branch:/{p=1}p' | tail -n+2
}
