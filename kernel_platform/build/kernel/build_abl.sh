#!/bin/bash
# Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

# Usage:
#   build/build_abl.sh <TARGET_PRODUCT>*
# or:
#   OUT_DIR=<out dir> DIST_DIR=<dist dir> build/build_abl.sh <TARGET_PRODUCT>*
#   To use a custom build config:
#   BUILD_CONFIG_ABL=<path to the build.config> <TARGET_PRODUCT>*
#
# Examples:
#   To define custom out and dist directories:
#     OUT_DIR=output DIST_DIR=dist build/build_abl.sh
#   To use a custom build config:
#     BUILD_CONFIG_ABL=bootable/bootloader/edk2/QcomModulePkg/build.config.msm.kalama build/build_abl.sh
#
# The following environment variables are considered during execution:
#
#   BUILD_CONFIG_ABL
#     Build config file to initialize the build environment from. The location
#     is to be defined relative to the edk2 directory.
#   OUT_DIR
#     Base output directory for the kernel build.
#     Defaults to <REPO_ROOT>/out/<BRANCH>.
#
#   DIST_DIR
#     Base output directory for the kernel distribution.
#     Defaults to <OUT_DIR>/dist
#
#   ABL_OUT_DIR
#     Base output directory for the edk2 build.
#     Defaults to <OUT_DIR>/
#
#   ABL_IMAGE_DIR
#     Base output directory for the edk2 distribution.
#     Defaults to <DIST_DIR>/
#
#   SKIP_SIGN_ABL
#     if set to "1", build a unsigned edk2 image
#
#   SKIP_COMPILE_ABL
#     if set to "1", skip edk2 compilation
#

set -e

function abl_image_generate() {
  PREBUILT_HOST_TOOLS="BUILD_CC=clang BUILD_CXX=clang++ LDPATH=-fuse-ld=lld BUILD_AR=llvm-ar"

  MKABL_ARGS=("-C" "${ROOT_DIR}/${ABL_SRC}")
  MKABL_ARGS+=("BOOTLOADER_OUT=${ABL_OUT_DIR}/obj/ABL_OUT" "all")
  MKABL_ARGS+=("PREBUILT_HOST_TOOLS=${PREBUILT_HOST_TOOLS}")
  MKABL_ARGS+=("${MAKE_FLAGS[@]}")
  MKABL_ARGS+=("CLANG_BIN=${ROOT_DIR}/${CLANG_PREBUILT_BIN}/")

  set -x
  make "${MKABL_ARGS[@]}"
  set +x

  set +e
  ABL_DEBUG_FILE="$(find ${ABL_OUT_DIR} -name LinuxLoader.debug)"
  set -e
  if [ -e "${ABL_DEBUG_FILE}" ]; then
    cp ${ABL_DEBUG_FILE} ${ABL_IMAGE_DIR}/LinuxLoader_${TARGET_BUILD_VARIANT}.debug
    cp ${ABL_OUT_DIR}/unsigned_abl.elf ${ABL_IMAGE_DIR}/unsigned_abl_${TARGET_BUILD_VARIANT}.elf
  fi
}

function sec_abl_image_generate() {
  if [ ! -e "${SECTOOLS}" ]; then
    echo "sectools not found. sectools = ${SECTOOLS}"
    exit 1
  fi
  if [ ! -f "${ABL_OUT_DIR}/unsigned_abl.elf" ]; then
    echo "unsigned_abl.elf not found. Please check the parth=${ABL_OUT_DIR}/unsigned_abl.elf"
    exit 1
  fi
  [ -f "${ABL_IMAGE_DIR}/${ABL_IMAGE_NAME}" ] && rm -rf ${ABL_IMAGE_DIR}/${ABL_IMAGE_NAME}

  cp -rf ${ABL_OUT_DIR}/unsigned_abl.elf ${ABL_IMAGE_DIR}/${ABL_IMAGE_NAME}

  set -x
  "${SECABL_CMD[@]}" > ${ABL_OUT_DIR}/secimage.log 2>&1
  set +x
}

echo "========================================================"
echo " Building abl"

export ROOT_DIR=$(readlink -f $(dirname $0)/..)

source "${ROOT_DIR}/build/_setup_env.sh"

if [ -z "${ABL_SRC}" ]; then
  ABL_SRC=bootable/bootloader/edk2
fi

if [ ! -e "${ROOT_DIR}/${ABL_SRC}" ]; then
  echo "*** STOP *** Please check the edk2 path: ${ROOT_DIR}/${ABL_SRC}"
  exit 1
fi

if [ -n "${1}" ]; then
  MSM_ARCH=${1}
fi

if [ -z "$BUILD_CONFIG_ABL" ]; then
  BUILD_CONFIG_ABL=${ROOT_DIR}/${ABL_SRC}/QcomModulePkg/build.config.msm.${MSM_ARCH}
else
  BUILD_CONFIG_ABL=${ROOT_DIR}/${BUILD_CONFIG_ABL}
fi

if [ ! -f "${BUILD_CONFIG_ABL}" ]; then
  echo "${BUILD_CONFIG_ABL} file not found,\
You should have a target config file to build,\
for ex: build.config.msm.kalama"
  exit 1
fi

[ -z "${ABL_OUT_DIR}" ] && ABL_OUT_DIR=${COMMON_OUT_DIR}

[ -z "${TARGET_BUILD_VARIANT}" ] && TARGET_BUILD_VARIANT=userdebug

ABL_OUT_DIR=${ABL_OUT_DIR}/abl-${TARGET_BUILD_VARIANT}
ABL_IMAGE_NAME=abl_${TARGET_BUILD_VARIANT}.elf

[ -z "${ABL_IMAGE_DIR}" ] && ABL_IMAGE_DIR=${DIST_DIR}
[ -z "${ABL_IMAGE_DIR}" ] && ABL_IMAGE_DIR=${ABL_OUT_DIR}
mkdir -p ${ABL_IMAGE_DIR}

## Include target config file
. ${BUILD_CONFIG_ABL}

# ABL ELF
if [  "${SKIP_COMPILE_ABL}" != "1" ]; then
  abl_image_generate
else
  echo "*** WARN *** Skip bootloader compilation"
fi

#sec-image-generate
if [ -e "${ABL_OUT_DIR}/unsigned_abl.elf" ]; then
  if [ "${SKIP_SIGN_ABL}" != "1" -a -n "${SECTOOLS}" -a -n "${SECTOOLS_SECURITY_PROFILE}" ]; then
    sec_abl_image_generate
    if [ -e "${ABL_IMAGE_DIR}/${ABL_IMAGE_NAME}" ]; then
      ln -sf ${ABL_IMAGE_DIR}/${ABL_IMAGE_NAME} ${ABL_IMAGE_DIR}/abl.elf
    fi
    echo "abl image created at ${ABL_IMAGE_DIR}/abl.elf"
  else
    echo "*** WARN *** Bootloader images are unsigned"
    echo "unsigned abl image created at ${ABL_OUT_DIR}/unsigned_abl.elf"
  fi
fi

