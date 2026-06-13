# Copyright (C) 2019 The Android Open Source Project
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

# This is an implementation detail of Kleaf. Do not source directly as it will
# spoil your shell. You have been warned! If you have a good reason to source
# the result of this file into a shell, please let kernel-team@android.com know
# and we will be happy to help with your use case.

[ -n "$_SETUP_ENV_SH_INCLUDED" ] && return || export _SETUP_ENV_SH_INCLUDED=1

# TODO: Use a $(gettop) style method.
export ROOT_DIR=$(readlink -f $PWD)

export BUILD_CONFIG=${BUILD_CONFIG:-build.config}

# Helper function to let build.config files add command to PRE_DEFCONFIG_CMDS, EXTRA_CMDS, etc.
# Usage: append_cmd PRE_DEFCONFIG_CMDS 'the_cmd'
function append_cmd() {
  if [ ! -z "${!1}" ]; then
    eval "$1=\"${!1} && \$2\""
  else
    eval "$1=\"\$2\""
  fi
}
export -f append_cmd

export KERNEL_DIR
# for case that KERNEL_DIR is not specified in environment
if [ -z "${KERNEL_DIR}" ]; then
    # for the case that KERNEL_DIR is not specified in the BUILD_CONFIG file
    # use the directory of the build config file as KERNEL_DIR
    # for the case that KERNEL_DIR is specified in the BUILD_CONFIG file,
    # or via the config files sourced, the value of KERNEL_DIR
    # set here would be overwritten, and the specified value would be used.
    build_config_path=$(readlink -f ${ROOT_DIR}/${BUILD_CONFIG})
    real_root_dir=${build_config_path%%${BUILD_CONFIG}}
    build_config_dir=$(dirname ${build_config_path})
    build_config_dir=${build_config_dir##${ROOT_DIR}/}
    build_config_dir=${build_config_dir##${real_root_dir}}
    KERNEL_DIR="${build_config_dir}"
fi

set -a
. ${ROOT_DIR}/${BUILD_CONFIG}
for fragment in ${BUILD_CONFIG_FRAGMENTS}; do
  . ${ROOT_DIR}/${fragment}
done
set +a

# For incremental kernel development, it is beneficial to trade certain
# optimizations for faster builds.
if [[ -n "${FAST_BUILD}" ]]; then
  # Decrease lz4 compression level to significantly speed up ramdisk compression.
  : ${LZ4_RAMDISK_COMPRESS_ARGS:="--fast"}
  # Use ThinLTO for fast incremental compiles
  : ${LTO:="thin"}
  # skip installing kernel headers
  : ${SKIP_CP_KERNEL_HDR:="1"}
fi

export COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}})
export OUT_DIR=$(readlink -m ${COMMON_OUT_DIR}/${KERNEL_DIR})
export DIST_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})
export UNSTRIPPED_DIR=${DIST_DIR}/unstripped
export UNSTRIPPED_MODULES_ARCHIVE=unstripped_modules.tar.gz
export MODULES_ARCHIVE=modules.tar.gz

export TZ=UTC
export LC_ALL=C
if [ -z "${SOURCE_DATE_EPOCH}" ]; then
  if [[ -n "${KLEAF_SOURCE_DATE_EPOCHS}" ]]; then
    export SOURCE_DATE_EPOCH=$(extract_git_metadata "${KLEAF_SOURCE_DATE_EPOCHS}" "${KERNEL_DIR}" SOURCE_DATE_EPOCH)
    # Unset KLEAF_SOURCE_DATE_EPOCHS to avoid polluting {kernel_build}_env.sh
    # with unnecessary information (git metadata of unrelated projects)
    unset KLEAF_SOURCE_DATE_EPOCHS
  else
    export SOURCE_DATE_EPOCH=$(git -C ${ROOT_DIR}/${KERNEL_DIR} log -1 --pretty=%ct)
  fi
fi
if [ -z "${SOURCE_DATE_EPOCH}" ]; then
  echo "WARNING: Unable to determine SOURCE_DATE_EPOCH for ${ROOT_DIR}/${KERNEL_DIR}, fallback to 0" >&2
  export SOURCE_DATE_EPOCH=0
fi
# NOTE: Samsung does not use the Git SCM. Forcely set 'SOURCE_DATE_EPOCH' to
# the current date and time.
export SOURCE_DATE_EPOCH=`date "+%s"`
export KBUILD_BUILD_TIMESTAMP="$(date -d @${SOURCE_DATE_EPOCH})"
export KBUILD_BUILD_HOST=build-host
export KBUILD_BUILD_USER=build-user
export KBUILD_BUILD_VERSION=1

# List of prebuilt directories shell variables to incorporate into PATH
prebuilts_paths=(
LINUX_GCC_CROSS_COMPILE_PREBUILTS_BIN
LINUX_GCC_CROSS_COMPILE_ARM32_PREBUILTS_BIN
LINUX_GCC_CROSS_COMPILE_COMPAT_PREBUILTS_BIN
CLANG_PREBUILT_BIN
CLANGTOOLS_PREBUILT_BIN
RUST_PREBUILT_BIN
LZ4_PREBUILTS_BIN
DTC_PREBUILTS_BIN
LIBUFDT_PREBUILTS_BIN
BUILDTOOLS_PREBUILT_BIN
KLEAF_INTERNAL_BUILDTOOLS_PREBUILT_BIN
)

# Have host compiler use LLD and compiler-rt.
LLD_COMPILER_RT="-fuse-ld=lld --rtlib=compiler-rt"
if [[ -n "${NDK_TRIPLE}" ]]; then
  NDK_DIR=${ROOT_DIR}/prebuilts/ndk-r26
  USERCFLAGS="--target=${NDK_TRIPLE} "
  USERCFLAGS+="--sysroot=${NDK_DIR}/toolchains/llvm/prebuilt/linux-x86_64/sysroot "
  # Some kernel headers trigger -Wunused-function for unused static functions
  # with clang; GCC does not warn about unused static inline functions. The
  # kernel sets __attribute__((maybe_unused)) on such functions when W=1 is
  # not set.
  USERCFLAGS+="-Wno-unused-function "
  # To help debug these flags, consider commenting back in the following, and
  # add `echo $@ > /tmp/log.txt` and `2>>/tmp/log.txt` to the invocation of $@
  # in scripts/cc-can-link.sh.
  #USERCFLAGS+=" -Wl,--verbose -v"
  # We need to set -fuse-ld=lld for Android's build env since AOSP LLVM's
  # clang is not configured to use LLD by default, and BFD has been
  # intentionally removed. This way CC_CAN_LINK can properly link the test in
  # scripts/cc-can-link.sh.
  USERLDFLAGS="${LLD_COMPILER_RT} "
  USERLDFLAGS+="--target=${NDK_TRIPLE} "
else
  USERCFLAGS="--sysroot=/dev/null"
fi
export USERCFLAGS USERLDFLAGS

unset LD_LIBRARY_PATH

if [ "${HERMETIC_TOOLCHAIN:-0}" -eq 1 ]; then
  HOST_TOOLS=${OUT_DIR}/host_tools
  [ ! -e "${HOST_TOOLS}" ] && mkdir -p ${HOST_TOOLS}
  for tool in \
      bash \
      git \
      perl \
      rsync \
      sh \
      ${ADDITIONAL_HOST_TOOLS}
  do
      ln -sf $(which $tool) ${HOST_TOOLS}
  done
  PATH=${HOST_TOOLS}

  # set the common sysroot
  sysroot_flags+="--sysroot=${ROOT_DIR}/build/kernel/build-tools/sysroot "

  # add openssl (via boringssl) and other prebuilts into the lookup path
  cflags+="-I${ROOT_DIR}/prebuilts/kernel-build-tools/linux-x86/include "

  # add openssl and further prebuilt libraries into the lookup path
  ldflags+="-L ${ROOT_DIR}/prebuilts/kernel-build-tools/linux-x86/lib64 "
  ldflags+=${LLD_COMPILER_RT}
  export LD_LIBRARY_PATH="${ROOT_DIR}/prebuilts/kernel-build-tools/linux-x86/lib64"

  export HOSTCFLAGS="$sysroot_flags $cflags"
  export HOSTLDFLAGS="$sysroot_flags $ldflags"

fi

for prebuilt_bin in "${prebuilts_paths[@]}"; do
    prebuilt_bin=\${${prebuilt_bin}}
    eval prebuilt_bin="${prebuilt_bin}"
    if [ -n "${prebuilt_bin}" ]; then
        # Mitigate dup paths
        PATH=${PATH//"${ROOT_DIR}\/${prebuilt_bin}:"}
        PATH=${ROOT_DIR}/${prebuilt_bin}:${PATH}
    fi
done
PATH=${COMMON_OUT_DIR}/host/bin:${PATH}
LD_LIBRARY_PATH=${COMMON_OUT_DIR}/host/lib:${LD_LIBRARY_PATH}

export PATH
export LD_LIBRARY_PATH

unset PYTHONPATH
unset PYTHONHOME
unset PYTHONSTARTUP

export HOSTCC HOSTCXX CC LD AR NM OBJCOPY OBJDUMP OBJSIZE READELF STRIP AS

tool_args=()

# LLVM=1 implies what is otherwise set below; it is a more concise way of
# specifying CC=clang LD=ld.lld NM=llvm-nm OBJCOPY=llvm-objcopy <etc>, for
# newer kernel versions.
if [[ -n "${LLVM}" ]]; then
  tool_args+=("LLVM=1")
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
  OBJSIZE=llvm-size
  READELF=llvm-readelf
  STRIP=llvm-strip
else
  if [ -n "${HOSTCC}" ]; then
    tool_args+=("HOSTCC=${HOSTCC}")
  fi

  if [ -n "${CC}" ]; then
    tool_args+=("CC=${CC}")
    if [ -z "${HOSTCC}" ]; then
      tool_args+=("HOSTCC=${CC}")
    fi
  fi

  if [ -n "${LD}" ]; then
    tool_args+=("LD=${LD}" "HOSTLD=${LD}")
  fi

  if [ -n "${NM}" ]; then
    tool_args+=("NM=${NM}")
  fi

  if [ -n "${OBJCOPY}" ]; then
    tool_args+=("OBJCOPY=${OBJCOPY}")
  fi
fi

if [ -n "${LLVM_IAS}" ]; then
  tool_args+=("LLVM_IAS=${LLVM_IAS}")
  # Reset $AS for the same reason that we reset $CC etc above.
  AS=clang
fi

if [ -n "${DEPMOD}" ]; then
  tool_args+=("DEPMOD=${DEPMOD}")
fi

if [ -n "${DTC}" ]; then
  tool_args+=("DTC=${DTC}")
fi

export TOOL_ARGS="${tool_args[@]}"

export DECOMPRESS_GZIP DECOMPRESS_LZ4 RAMDISK_COMPRESS RAMDISK_DECOMPRESS RAMDISK_EXT

DECOMPRESS_GZIP="gzip -c -d"
DECOMPRESS_LZ4="lz4 -c -d -l"
if [ -z "${LZ4_RAMDISK}" ] ; then
  RAMDISK_COMPRESS="gzip -c -f"
  RAMDISK_DECOMPRESS="${DECOMPRESS_GZIP}"
  RAMDISK_EXT="gz"
else
  RAMDISK_COMPRESS="lz4 -c -l ${LZ4_RAMDISK_COMPRESS_ARGS:--12 --favor-decSpeed}"
  RAMDISK_DECOMPRESS="${DECOMPRESS_LZ4}"
  RAMDISK_EXT="lz4"
fi

# verifies that defconfig matches the DEFCONFIG
function check_defconfig() {
    (cd ${OUT_DIR} && \
     make ${TOOL_ARGS} O=${OUT_DIR} savedefconfig)
    [ "$ARCH" = "x86_64" -o "$ARCH" = "i386" ] && local ARCH=x86
    RES=0
    if [[ -f ${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG} ]]; then
      diff -u ${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG} ${OUT_DIR}/defconfig >&2 ||
        RES=$?
    else
      diff -u ${OUT_DIR}/arch/${ARCH}/configs/${DEFCONFIG} ${OUT_DIR}/defconfig >&2 ||
        RES=$?
    fi
    if [ ${RES} -ne 0 ]; then
        echo ERROR: savedefconfig does not match ${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG} >&2
    fi
    return ${RES}
}
export -f check_defconfig
