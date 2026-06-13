#!/bin/bash
# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -o errexit
set -o pipefail

readonly DEFAULT_TARGET=pineapple
readonly DEFAULT_BRANCH=android14-6.1
readonly ACK_PROJECT=kernel/common
readonly ACK_REPO="https://android.googlesource.com/${ACK_PROJECT}"
readonly SYMBOL_LIST=android/abi_gki_aarch64_qcom

ROOT_DIR="$(readlink -f "$(dirname "$0")/../..")"
readonly ROOT_DIR

print_usage() {
	name=$(basename "$0")
	cat << EOF
$name - update the msm-kernel ABI symbol list

This script will pull down the latest symbol list from upstream ACK, update
the list with the latest symbols from the msm-kernel build, then create a
commit suitable for pushing back to ACK.

Usage: $name [-s <SHORT DESCRIPTION>] [-l <LONG DESCRIPTION>] [-b <BUG>]
             [-B <BRANCH>] [-t <TARGET>] [-a SYMBOLS] [-p]

Options:
    -s SHORT_DESC  Short change description for use in commit message
    -l LONG_DESC   Long change description for use in commit message
    -b BUG         Bug number for use in commit message
    -B BRANCH      ACK branch to target (default $DEFAULT_BRANCH)
    -t TARGET      Target to build for (default $DEFAULT_TARGET)
    -a SYMBOLS     Manually add SYMBOLS to the list (comma-separated)
    -p             Push the commit to ACK automatically

Note: if -s, -l, or -b are omitted, the commit will be done in interactive mode.
EOF
}

tree_has_changes() {
	if git diff --quiet; then
		return 1
	else
		return 0
	fi
}

# Save the current git tree state, stashing if needed
save_tree_state() {
	printf "Saving current tree state... "
	current_ref="$(git rev-parse HEAD)"
	readonly current_ref
	if tree_has_changes; then
		unstash="true"
		git stash push --quiet
	fi
	printf "OK\n"
}

# Restore the git tree state, unstashing if needed
restore_tree_state() {
	if [ -z "$current_ref" ]; then
		return 0
	fi

	printf "Restoring current tree state... "
	git checkout --quiet "$current_ref"
	if [ "$unstash" = "true" ]; then
		git stash pop --quiet
		unstash="false"
	fi
	printf "OK\n"
}

main() {
	while getopts "hs:l:b:p:t:B:a:" opt; do
		case $opt in
		h)
			print_usage
			exit 0
			;;
		s)
			short_desc="$OPTARG"
			;;
		l)
			long_desc="$OPTARG"
			;;
		b)
			bug="$OPTARG"
			;;
		p)
			push="true"
			;;
		B)
			branch="$OPTARG"
			;;
		t)
			target="$OPTARG"
			;;
		a)
			manual_additions="$OPTARG"
			;;
		*)
			print_usage
			exit 1
			;;
		esac
	done

	cd "${ROOT_DIR}/msm-kernel"

	# Get the remote name or create the remote if it's not already there
	if ! remote=$(git remote -v | awk "\$2==\"$ACK_REPO\" {print \$1; exit}") \
		|| [ -z "$remote" ]
	then
		remote="ack"
		git remote add "$remote" "$ACK_REPO"
	fi

	printf "Fetching references from upstream ACK...\n"
	git fetch "$remote" -a

	save_tree_state
	trap 'restore_tree_state' EXIT

	ack_branch="${branch:-$DEFAULT_BRANCH}"

	# Update our symbol list to the ACK tip
	if ! git show "${remote}/${ack_branch}:${SYMBOL_LIST}" > "$SYMBOL_LIST"; then
		git checkout "$SYMBOL_LIST"
		exit 1
	fi

	if tree_has_changes; then
		git commit --quiet -m 'Temporary ACK sync' "$SYMBOL_LIST"
	fi

	# Add any additional symbols passed in by the user (sorting/dedup will
	# be taken care of later by Bazel)
	for s in $(echo "$manual_additions" | tr ',' ' '); do
		printf "  %s\n" "$s" >> android/abi_gki_aarch64_qcom
	done

	# Add new symbols from our build
	(
		cd ..
		./tools/bazel run \
			"//msm-kernel:${target:-$DEFAULT_TARGET}_gki_abi_update_symbol_list"
	)

	if ! tree_has_changes; then
		printf "No new ABI symbols to add to ACK!\n"
		exit 0
	fi

	# Commit our changes
	local -r commit_msg="$(mktemp)"
	cat << EOF > "$commit_msg"
ANDROID: $(basename "$SYMBOL_LIST"): ${short_desc:-<SHORT DESCRIPTION>}

$(printf "%s\n" "${long_desc:-<DESCRIPTION>}" | fmt)

Symbols added:
$(git diff "$SYMBOL_LIST" | grep '^\+ ' | tr '+' ' ')

Bug: ${bug:-<BUG>}
EOF
	if [ -z "$short_desc" ] || [ -z "$long_desc" ] || [ -z "$bug" ]; then
		git commit -e -a -s -F "$commit_msg"
	else
		git commit -a -s -F "$commit_msg"
	fi

	rm -f "$commit_msg"

	# Store a reference to the sync
	local -r sync_commit="$(git rev-parse HEAD)"

	# Check out the latest from ACK
	git checkout "${remote}/${ack_branch}"

	# Cherry-pick the commit which updated the list
	git cherry-pick "$sync_commit"
	if [ "$push" = "true" ]; then
		git push "$remote" "HEAD:refs/for/${ack_branch}"
		exit 0
	fi

	local -r commit_to_push="$(git rev-parse HEAD)"

	printf "\n\n===========================\n"
	printf "Commit ready to push: %s\n" "$commit_to_push"
	printf "Please double-check it and run the following to push to ACK:\n"
	printf "  git push %s %s:refs/for/%s\n" "$remote" "$commit_to_push" "$ack_branch"
	printf "===========================\n"
}

main "$@"
