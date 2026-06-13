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

# Usage:
#   build/build_module.sh <make options>*
# or:
#   OUT_DIR=<out dir> DIST_DIR=<dist dir> build/build_module.sh <make options>*
#
# Example:
#   OUT_DIR=output DIST_DIR=dist build/build_module.sh -j24 V=1#
#
# The following environment variables are considered during execution:
#
#   BUILD_CONFIG
#     Build config file to initialize the build environment from. The location
#     is to be defined relative to the repo root directory.
#     Defaults to 'build.config'.
#
#   MODULE_CONFIG
#     Build config file for module to initialize the build environment from.
#     The location is to be defined relative to the repo root directory.
#     Defaults to 'module.config'.
#
#   EXT_MODULES
#     Space separated list of external kernel modules to be build.
#
#   UNSTRIPPED_MODULES
#     Space separated list of modules to be copied to <DIST_DIR>/unstripped
#     for debugging purposes.
#
#   MODULE_OUT
#     Location to place compiled module output. When this option is specified,
#     Only one EXT_MODULES may be specified. A symlink is created from the
#     output Kbuild will use to MODULE_OUT.
#
# Environment variables to influence the stages of the kernel build.
#
#   SKIP_MRPROPER
#     if defined, skip `make mrproper`
#
#   INSTALL_MODULE_HEADERS
#     if defined, install uapi headers from the module.
#
#   BUILD_DTBS
#     if defined, install uapi headers from the module.

set -e

# rel_path <to> <from>
# Generate relative directory path to reach directory <to> from <from>
function rel_path() {
  python -c "import os.path; import sys; print(os.path.relpath(sys.argv[1], sys.argv[2]))" "$1" "$2"
}

export ROOT_DIR=$(readlink -f $(dirname $0)/..)

source "${ROOT_DIR}/build/_setup_env.sh"

export MAKE_ARGS="$* ${KBUILD_OPTIONS}"
export MAKEFLAGS="-j$(nproc) ${MAKEFLAGS}"
export MODULES_STAGING_DIR=$(readlink -m ${COMMON_OUT_DIR}/staging)
export MODULES_PRIVATE_DIR=$(readlink -m ${COMMON_OUT_DIR}/private)
export UNSTRIPPED_DIR=${DIST_DIR}/unstripped
export MODULE_UAPI_HEADERS_DIR=$(readlink -m ${COMMON_OUT_DIR}/module_uapi_headers)

# Create & export bazel cache dir within build workspace
DEFAULT_CACHE_DIR=${ROOT_DIR}/bazel-cache
mkdir -p ${DEFAULT_CACHE_DIR}
export TEST_TMPDIR=${DEFAULT_CACHE_DIR}

cd ${ROOT_DIR}

export CLANG_TRIPLE CROSS_COMPILE CROSS_COMPILE_COMPAT CROSS_COMPILE_ARM32 ARCH SUBARCH MAKE_GOALS

# Restore the previously saved CC argument that might have been overridden by
# the BUILD_CONFIG.
[ -n "${CC_ARG}" ] && CC="${CC_ARG}"

# CC=gcc is effectively a fallback to the default gcc including any target
# triplets. An absolute path (e.g., CC=/usr/bin/gcc) must be specified to use a
# custom compiler.
[ "${CC}" == "gcc" ] && unset CC && unset CC_ARG

TOOL_ARGS=()

# LLVM=1 implies what is otherwise set below; it is a more concise way of
# specifying CC=clang LD=ld.lld NM=llvm-nm OBJCOPY=llvm-objcopy <etc>, for
# newer kernel versions.
if [[ -n "${LLVM}" ]]; then
  TOOL_ARGS+=("LLVM=1")
  # Reset a bunch of variables that the kernel's top level Makefile does, just
  # in case someone tries to use these binaries in this script such as in
  # initramfs generation below.
  HOSTCC=clang
  HOSTCXX=clang++
  CC=clang
  LD=ld.lld
  AR=llvm-ar
  NM=llvm-nm
  OBJCOPY=llvm-objcopy
  OBJDUMP=llvm-objdump
  READELF=llvm-readelf
  OBJSIZE=llvm-size
  STRIP=llvm-strip
else
  if [ -n "${HOSTCC}" ]; then
    TOOL_ARGS+=("HOSTCC=${HOSTCC}")
  fi

  if [ -n "${CC}" ]; then
    TOOL_ARGS+=("CC=${CC}")
    if [ -z "${HOSTCC}" ]; then
      TOOL_ARGS+=("HOSTCC=${CC}")
    fi
  fi

  if [ -n "${LD}" ]; then
    TOOL_ARGS+=("LD=${LD}" "HOSTLD=${LD}")
    custom_ld=${LD##*.}
    if [ -n "${custom_ld}" ]; then
      TOOL_ARGS+=("HOSTLDFLAGS=-fuse-ld=${custom_ld}")
    fi
  fi

  if [ -n "${NM}" ]; then
    TOOL_ARGS+=("NM=${NM}")
  fi

  if [ -n "${OBJCOPY}" ]; then
    TOOL_ARGS+=("OBJCOPY=${OBJCOPY}")
  fi
fi

if [ -n "${LLVM_IAS}" ]; then
  TOOL_ARGS+=("LLVM_IAS=${LLVM_IAS}")
  # Reset $AS for the same reason that we reset $CC etc above.
  AS=clang
fi

if [ -n "${DEPMOD}" ]; then
  TOOL_ARGS+=("DEPMOD=${DEPMOD}")
fi

if [ -n "${DTC}" ]; then
  TOOL_ARGS+=("DTC=${DTC}")
fi

# Allow hooks that refer to $CC_LD_ARG to keep working until they can be
# updated.
CC_LD_ARG="${TOOL_ARGS[@]}"

MODULE_CONFIG=${MODULE_CONFIG:-module.config}
if [ -e "${ROOT_DIR}/${MODULE_CONFIG}" ]; then
  source "${ROOT_DIR}/${MODULE_CONFIG}"
fi


# KERNEL_KIT should be explicitly defined, but default it to something sensible
KERNEL_KIT="${KERNEL_KIT:-${COMMON_OUT_DIR}}"

if [ ! -e "${KERNEL_KIT}/.config" ]; then
  # Try a couple reasonable/reliable fallback locations
  if [ -e "${KERNEL_KIT}/dist/.config" ]; then
    KERNEL_KIT="${KERNEL_KIT}/dist"
  elif [ -e "${KERNEL_KIT}/${KERNEL_DIR}/.config" ]; then
    KERNEL_KIT="${KERNEL_KIT}/${KERNEL_DIR}"
  fi
fi
if [ ! -e "${KERNEL_KIT}/.config" ]; then
  echo "ERROR! Could not find prebuilt kernel artifacts in ${KERNEL_KIT}"
  exit 1
fi

if [ ! -e "${OUT_DIR}/Makefile" -o -z "${EXT_MODULES}" ]; then
  echo "========================================================"
  echo " Prepare to compile modules from ${KERNEL_KIT}"

  set -x
  mkdir -p ${OUT_DIR}/
  cp ${KERNEL_KIT}/.config ${KERNEL_KIT}/Module.symvers ${OUT_DIR}/

  if [ -z "${EXT_MODULES}" -a ! ${KERNEL_KIT}/host -ef ${COMMON_OUT_DIR}/host ]; then
    rm -rf ${COMMON_OUT_DIR}/host
  fi
  if [ -e ${KERNEL_KIT}/host -a ! -e ${COMMON_OUT_DIR}/host ]; then
    cp -r ${KERNEL_KIT}/host ${COMMON_OUT_DIR}
  fi

  # Install .config from kernel platform
  (
    cd "${KERNEL_DIR}"
    make O="${OUT_DIR}" "${TOOL_ARGS[@]}" ${MAKE_ARGS} olddefconfig
  )

  # To guard against .config silently diverging from the one kernel platform created,
  # set KCONFIG_NOSILENTUPDATE=1. If doing an incremental build, this also guards against
  # the kernel platform .config changing since autoconf.h and related files would need an update
  # in OUT_DIR. To get around this valid change, do "make olddefconfig", copy the .config again,
  # then do the NOSILENTUPDATE check
  cp ${KERNEL_KIT}/.config ${OUT_DIR}/
  (
    cd "${KERNEL_DIR}"
    make O="${OUT_DIR}" "${TOOL_ARGS[@]}" ${MAKE_ARGS} modules_prepare
    if ! d=$(diff -Naurp ${KERNEL_KIT}/.config ${OUT_DIR}/.config); then
        echo "CONFIG diff between ${KERNEL_KIT}/.config ${OUT_DIR}/.config"
        echo $d
    fi
  )
  set +x
fi
# Set KBUILD_MIXED_TREE in case an out-of-tree Makefile does "make all". This causes
# kbuild to also want to compile vmlinux
MAKE_ARGS+=" KBUILD_MIXED_TREE=${KERNEL_KIT}"

echo "========================================================"
echo " Building external modules and installing them into staging directory"

for EXT_MOD in ${EXT_MODULES}; do
  # The path that we pass in via the variable M needs to be a relative path
  # relative to the kernel source directory. The source files will then be
  # looked for in ${KERNEL_DIR}/${EXT_MOD_REL} and the object files (i.e. .o
  # and .ko) files will be stored in ${OUT_DIR}/${EXT_MOD_REL}. If we
  # instead set M to an absolute path, then object (i.e. .o and .ko) files
  # are stored in the module source directory which is not what we want.
  EXT_MOD_REL=$(rel_path ${ROOT_DIR}/${EXT_MOD} ${KERNEL_DIR})
  # The output directory must exist before we invoke make. Otherwise, the
  # build system behaves horribly wrong.
  set -x
  if [ -n "${MODULE_OUT}" ]; then
    mkdir -p $(dirname ${OUT_DIR}/${EXT_MOD_REL})
    mkdir -p ${MODULE_OUT}
    rm -rf ${OUT_DIR}/${EXT_MOD_REL}
    ln -srT ${MODULE_OUT} ${OUT_DIR}/${EXT_MOD_REL}
  else
    mkdir -p ${OUT_DIR}/${EXT_MOD_REL}
  fi

  module_path="$(echo "$EXT_MOD" | sed -e 's/^[\.\/]*//')"
  top_dir="$(echo "$module_path" | cut -d '/' -f 1)"

  # Create a link to the module's tree within kernel_platform
  (cd "$ROOT_DIR" && ln -fs "../${top_dir}")

  # Search for the module package by looking up from the module_path
  pkg_path="$module_path"
  until [ -f "${pkg_path}/BUILD.bazel" ]; do
    pkg_path="$(dirname "$pkg_path")"

    # If we see a WORKSPACE file, we've gone too far
    if [ -f "${pkg_path}/WORKSPACE.bzlmod" ]; then
      echo "error - no Bazel package associated with $module_path"
      pkg_path=""
      break
    fi
  done

  if [ "$TARGET_BOARD_PLATFORM" = "msmnile" ]; then
     btgt="gen3auto"
  elif [ "$TARGET_BOARD_PLATFORM" = "sm6150" ]; then
     btgt="sdmsteppeauto"
  else
     btgt="$TARGET_BOARD_PLATFORM"
  fi

  filter_regex="${btgt/_/-}_${VARIANT/_/-}_${SUBTARGET_REGEX:-.*}_dist$"

  # Query for a target that matches the pattern for module distribution
  if [ "$ENABLE_DDK_BUILD" = "true" ] \
     && [ -n "$pkg_path" ] \
     && [ -n "$btgt" ] \
     && build_target=$(./tools/bazel query --ui_event_filters=-info --noshow_progress \
          "filter('${filter_regex}', //${pkg_path}/...)") \
     && [ -n "$build_target" ]
  then
    if [ "$(printf "%s\n" "$build_target" | wc -l)" -gt 1 ]; then
      printf "error - multiple targets found matching \"%s\":\n%s\n" \
        "$filter_regex" "$build_target"
      exit 1
    fi

    # Make sure Bazel extensions are linked properly
    if [ ! -f "build/msm_kernel_extensions.bzl" ] \
          && [ -f "msm-kernel/msm_kernel_extensions.bzl" ]; then
      ln -fs "../msm-kernel/msm_kernel_extensions.bzl" "build/msm_kernel_extensions.bzl"
    fi
    if [ ! -f "build/abl_extensions.bzl" ] \
          && [ -f "bootable/bootloader/edk2/abl_extensions.bzl" ]; then
      ln -fs "../bootable/bootloader/edk2/abl_extensions.bzl" "build/abl_extensions.bzl"
    fi

    mapfile -t build_flags < "${KERNEL_KIT}/build_opts.txt"

    if [ "$ALLOW_UNSAFE_DDK_HEADERS" = "true" ]; then
      build_flags+=("--allow_ddk_unsafe_headers")
    fi

    # Samsung KBUILD: camera-kernel ddk build needs EXTRA_DDK_ARGS
    if [[ "x${IS_KBUILD}" == "xtrue" ]] && [[ "$EXT_MOD" == *"camera-kernel"* ]]; then
        EXTRA_DDK_ARGS+=" --//vendor/qcom/opensource/camera-kernel:project_name=${MODEL}"
    fi

    if [ -n "$EXTRA_DDK_ARGS" ]; then
      IFS=" " read -r -a extra_args <<< "$EXTRA_DDK_ARGS"
      build_flags+=("${extra_args[@]}")
    fi

    # Run the dist command passing in the output directory from Android build system
    ./tools/bazel run "${build_flags[@]}" "$build_target" \
      -- --dist_dir="${OUT_DIR}/${EXT_MOD_REL}"

    # The Module.symvers file is named "<target>_<variant>_Modules.symvers, but other modules are
    # looking for just "Module.symvers". Concatenate any of them into one Module.symvers file.
    cat "${OUT_DIR}/${EXT_MOD_REL}/${TARGET_BOARD_PLATFORM}_${VARIANT}"_*_Module.symvers \
      > "${OUT_DIR}/${EXT_MOD_REL}/Module.symvers"

    # Intermediate directories aren't generated automatically, so we need to create them manually
    if [ -n "$INTERMEDIATE_DIR" ]; then
      mkdir -p "$(dirname "$INTERMEDIATE_DIR")"
      rm -rf "${INTERMEDIATE_DIR}/${EXT_MOD_REL}"
      cp -ar "${OUT_DIR}/${EXT_MOD_REL}" "$INTERMEDIATE_DIR"
      for ko in "${OUT_DIR}/${EXT_MOD_REL}"/*.ko; do
        rm -rf "$(dirname "$INTERMEDIATE_DIR")/$(basename "$ko")_intermediates"
        cp -ar "${OUT_DIR}/${EXT_MOD_REL}" \
          "$(dirname "$INTERMEDIATE_DIR")/$(basename "$ko")_intermediates"
      done
    fi

    # We need to manually copy .ko's into subdirectories if they have them
    for ko in $KO_DIRS; do
      if echo "$ko" | grep -q '/'; then
        ko_name="$(basename "$ko")"
        if [ ! -f "${OUT_DIR}/${EXT_MOD_REL}/${ko_name}" ]; then
          continue
        fi
        dir="$(dirname "$ko")"
        mkdir -p "${OUT_DIR}/${EXT_MOD_REL}/${dir}"
        cp -a "${OUT_DIR}/${EXT_MOD_REL}/${ko_name}" "${OUT_DIR}/${EXT_MOD_REL}/${dir}"
      fi
    done
  else
    # Fall back on legacy make if Bazel build is not present
    if [[ ! $EXT_MOD =~ devicetree ]]; then
      echo "warning - building kernel modules with legacy make. Please migrate to DDK."
    fi
    make -C ${EXT_MOD} M=${EXT_MOD_REL} KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}  \
                        O=${OUT_DIR} "${TOOL_ARGS[@]}" ${MAKE_ARGS}
  fi

  if [ -n "${INSTALL_MODULE_HEADERS}" ]; then
    echo "========================================================"
    echo " Installing UAPI module headers:"
    mkdir -p "${KERNEL_UAPI_HEADERS_DIR}/usr"
    make -C ${EXT_MOD} M=${EXT_MOD_REL} KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}  \
                      O=${OUT_DIR} "${TOOL_ARGS[@]}"                         \
                      INSTALL_HDR_PATH="${KERNEL_UAPI_HEADERS_DIR}/usr"      \
                      ${MAKE_ARGS} headers_install
  fi
  set +x
done

