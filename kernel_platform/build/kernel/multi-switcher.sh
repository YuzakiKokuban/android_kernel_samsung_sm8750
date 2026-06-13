#!/bin/bash
#
# Copyright (C) 2020 The Android Open Source Project
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

BASE=$(dirname $(dirname $(readlink -f $0)))

BRANCH=$1

pushd $BASE > /dev/null

  if [[ ! ( -d common-${BRANCH} || -d common${BRANCH} )  \
        || ${BRANCH} == ""                               \
        || ${BRANCH} == "modules" ]]; then
    echo "usage: $0 <branch>"
    echo
    echo "Branches available: "
    ls -d common-* | sed 's/common-/\t/g' | grep -v modules
    ls -d common1?-* | sed 's/common/\t/g'
    exit 1
  fi

  echo "Switching to $BRANCH"

  for dir in common common-modules/virtual-device; do
    if [ -L ${dir} ]; then
      rm ${dir}
    fi

    for candidate in ${dir}-${BRANCH} ${dir}${BRANCH}; do
      if [ -d ${candidate} ]; then
          (
            cd $(dirname $candidate)
            ln -vs $(basename ${candidate}) $(basename ${dir})
          )
      fi
    done
  done

  # now switch the build tools between trunk and legacy version
  case "${BRANCH}" in
    4.4|4.9|4.14-stable|4.19-stable|11-5.4|12-5.4|12-5.10)
      suffix="legacy"
      ;;
    *)
      suffix="trunk"
      ;;
  esac

  for dir in "build" "kernel" "prebuilts" "tools"; do
    ln -vsnf "${dir}-${suffix}" "${dir}"
  done

popd > /dev/null
