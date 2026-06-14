#!/usr/bin/env bash
#
# build.sh -- build the Samsung SM-S9380V (SM8650/Snapdragon 8 Gen 3) GKI kernel.
#
# Usage:
#   build/build.sh [lkm|resukisu]
#
# Modes:
#   lkm       (default) pure kernel, no KSU/SUSFS (KSU injected at flash
#             time by the manager app patching init_boot)
#   resukisu  built-in ReSukiSU + SUSFS + full feature set
#
# Environment overrides:
#   TOOLCHAIN_DIR  prebuilts dir (default: ../toolchain_samsung_sm8650/prebuilts)
#   JOBS           parallel jobs (default: nproc)
#   PACK           1 to pack an AnyKernel3 zip (needs ANYKERNEL_DIR)
#   ANYKERNEL_DIR  path to an AnyKernel3 tree (default: ../AnyKernel3_sm9380)
#
set -e
MODE="${1:-${MODE:-lkm}}"
[ "$MODE" = "resukisu" ] || [ "$MODE" = "lkm" ] || { echo "mode must be lkm|resukisu"; exit 1; }

KROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$KROOT"
HERE="$KROOT/build"
OUT="$KROOT/out"
JOBS="${JOBS:-$(nproc)}"
TOOLCHAIN_DIR="${TOOLCHAIN_DIR:-$KROOT/../toolchain_samsung_sm8650/prebuilts}"
CLANG_BIN="${TOOLCHAIN_DIR}/clang/host/linux-x86/clang-r487747c/bin"
ANYKERNEL_DIR="${ANYKERNEL_DIR:-$KROOT/../AnyKernel3_sm9380}"
KMI_GEN=8   # android15-6.6 == KMI generation 8

[ -d "$CLANG_BIN" ] || { echo "WARNING: toolchain not found at $CLANG_BIN"
  echo "Set TOOLCHAIN_DIR to your Samsung clang-r487747c prebuilts dir."
  echo "Continuing anyway (toolchain may be in PATH)..." ; }

echo "=== SM-S9380V build: MODE=$MODE  JOBS=$JOBS ==="
echo "    $(grep -m1 '^VERSION' Makefile | tr -d ' ') $(grep -m1 '^PATCHLEVEL' Makefile | tr -d ' ') $(grep -m1 '^SUBLEVEL' Makefile | tr -d ' ')"

# ---- toolchain env ----
if [ -d "$CLANG_BIN" ]; then
  export PATH="$TOOLCHAIN_DIR/build-tools/linux-x86/bin:$TOOLCHAIN_DIR/build-tools/path/linux-x86:$CLANG_BIN:$TOOLCHAIN_DIR/kernel-build-tools/linux-x86/bin:$PATH"
  sysroot="--sysroot=$TOOLCHAIN_DIR/gcc/linux-x86/host/x86_64-linux-glibc2.17-4.8/sysroot"
  export LD_LIBRARY_PATH="$TOOLCHAIN_DIR/kernel-build-tools/linux-x86/lib64"
  export HOSTCFLAGS="$sysroot -I$TOOLCHAIN_DIR/kernel-build-tools/linux-x86/include"
  export HOSTLDFLAGS="$sysroot -L $TOOLCHAIN_DIR/kernel-build-tools/linux-x86/lib64 -fuse-ld=lld --rtlib=compiler-rt"
fi

# ccache (big speedup on rebuilds; no-op if ccache absent). Masquerade a clang
# symlink at the front of PATH so the string-based "CC=clang" routes through
# ccache, which then finds the real clang further down PATH (CLANG_BIN).
# Honours CCACHE_DIR / CCACHE_MAXSIZE exported by the CI (cached across runs).
if command -v ccache >/dev/null 2>&1; then
  export CCACHE_DIR="${CCACHE_DIR:-$KROOT/.ccache}"
  export CCACHE_MAXSIZE="${CCACHE_MAXSIZE:-2G}"
  mkdir -p "$CCACHE_DIR" "$KROOT/.ccache_bin"
  ln -sf "$(command -v ccache)" "$KROOT/.ccache_bin/clang"
  export PATH="$KROOT/.ccache_bin:$PATH"
  echo "    ccache: on (dir=$CCACHE_DIR max=$CCACHE_MAXSIZE)"
fi
MAKE_ARGS="CC=clang ARCH=arm64 LLVM=1 LLVM_IAS=1"

# ---- apply features (mode-aware) ----
MODE="$MODE" CACHE="$KROOT/.build_cache" bash "$HERE/apply_features.sh"

# ---- configure ----
echo "=== configure ==="
make -j"$JOBS" O="$OUT" $MAKE_ARGS defconfig >/dev/null

# common: drop Samsung security stack that conflicts with custom kernels
COMMON_DISABLE="-d UH -d RKP -d KDP -d SECURITY_DEFEX -d INTEGRITY -d FIVE -d TRIM_UNUSED_KSYMS"
# common features (KSU-independent): NTFS3, zram-lz4, FQ+BBR, NTSync, IPv6 NAT, Re:Kernel, sysvipc/mqueue
COMMON_ENABLE="-e NTFS3_FS -e NTFS3_LZX_XPRESS -e ZRAM_DEF_COMP_LZ4 --set-str ZRAM_DEF_COMP lz4 \
  -e NET_SCH_FQ -e TCP_CONG_BBR -e DEFAULT_BBR -e NTSYNC -e IP6_NF_NAT -e REKERNEL \
  -e SYSVIPC -e POSIX_MQUEUE -e IPC_NS -e PID_NS -e DEVTMPFS \
  -e NETFILTER_XT_MATCH_ADDRTYPE -e NETFILTER_XT_MATCH_RECENT \
  -e TMPFS -e TMPFS_POSIX_ACL -e TMPFS_XATTR -e TMPFS_INODE64 \
  -e NETFILTER_XT_TARGET_HL \
  -e IP_SET -e IP_SET_BITMAP_IP -e IP_SET_BITMAP_IPMAC -e IP_SET_BITMAP_PORT \
  -e IP_SET_HASH_IP -e IP_SET_HASH_IPMARK -e IP_SET_HASH_IPPORT -e IP_SET_HASH_IPPORTIP \
  -e IP_SET_HASH_IPPORTNET -e IP_SET_HASH_IPMAC -e IP_SET_HASH_MAC -e IP_SET_HASH_NET \
  -e IP_SET_HASH_NETNET -e IP_SET_HASH_NETPORT -e IP_SET_HASH_NETPORTNET \
  -e IP_SET_HASH_NETIFACE -e IP_SET_LIST_SET"

if [ "$MODE" = "resukisu" ]; then
  MODE_CFG="-e KSU -e KSU_SUSFS"
else
  MODE_CFG="-d KSU -d KSU_SUSFS"
fi

# shellcheck disable=SC2086
./scripts/config --file "$OUT/.config" $COMMON_DISABLE $MODE_CFG $COMMON_ENABLE \
  -d LOCALVERSION_AUTO --set-str LOCALVERSION "-android15-${KMI_GEN}-YuccaA-SM9380-4k"

# baseband_guard LSM
if grep -q '^CONFIG_LSM="' "$OUT/.config" && ! grep -q baseband_guard "$OUT/.config"; then
  sed -i 's/^\(CONFIG_LSM="[^"]*\)"$/\1,baseband_guard"/' "$OUT/.config"
fi
make -j"$JOBS" O="$OUT" $MAKE_ARGS olddefconfig >/dev/null

echo "=== config summary ==="
for c in KSU KSU_SUSFS NTFS3_FS ZRAM_DEF_COMP_LZ4 TCP_CONG_BBR NTSYNC IP6_NF_NAT REKERNEL POSIX_MQUEUE; do
  printf '    %-20s %s\n' "$c" "$(grep -q "^CONFIG_$c=y" "$OUT/.config" && echo y || echo n)"
done
printf '    %-20s %s\n' "baseband_guard" "$(grep -q baseband_guard "$OUT/.config" && echo y || echo n)"

# ---- build (retry once: kbuild fixdep occasionally races on a parallel build) ----
echo "=== build Image ==="
if ! make -j"$JOBS" O="$OUT" $MAKE_ARGS Image; then
  echo "=== first pass failed; retrying incrementally (fixdep race guard) ==="
  make -j"$JOBS" O="$OUT" $MAKE_ARGS Image
fi
IMG="$OUT/arch/arm64/boot/Image"
if [ -f "$IMG" ]; then
  REL="$(cat "$OUT/include/config/kernel.release")"
  echo "    Image: $(stat -c%s "$IMG" 2>/dev/null || stat -f%z "$IMG") bytes   release: $REL"
else
  echo "ERROR: Image not found at $IMG"
  exit 1
fi

# ---- pack (optional) ----
if [ "${PACK:-0}" = "1" ]; then
  [ -d "$ANYKERNEL_DIR" ] || { echo "ANYKERNEL_DIR not found: $ANYKERNEL_DIR"; exit 1; }
  tag="lkm"; [ "$MODE" = "resukisu" ] && tag="resukisu"
  ZIP="$KROOT/../SM9380_${tag}_${REL%%-*}_$(date +%m%d).zip"
  rm -f "$ANYKERNEL_DIR"/*.zip "$ANYKERNEL_DIR"/zImage "$ANYKERNEL_DIR"/Image
  cp -f "$IMG" "$ANYKERNEL_DIR/zImage"
  # SM-S9380V device codename
  ak="$ANYKERNEL_DIR/anykernel.sh"
  if [ -f "$ak" ] && ! grep -qE '^device\.name[0-9]*=psq$' "$ak"; then
    sed -i "/^do.devicecheck=/a device.name4=psq" "$ak"
  fi
  [ -f "$ak" ] && echo "    AnyKernel devices: $(grep -oE '^device.name[0-9]+=[a-z0-9]+' "$ak" | cut -d= -f2 | tr '\n' ' ')"
  rm -f "$ZIP"
  ( cd "$ANYKERNEL_DIR" && zip -r9 "$ZIP" . -x '*.git*' '*.zip' >/dev/null )
  echo "=== packed: $ZIP ($(du -h "$ZIP" | cut -f1)) ==="
fi
echo "=== DONE ($MODE): $REL ==="
