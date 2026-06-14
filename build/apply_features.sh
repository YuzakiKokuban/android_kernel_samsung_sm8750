#!/usr/bin/env bash
#
# apply_features.sh -- apply the LKM feature set onto the clean SM-S9380V base.
#
# Runs from the kernel source root. Driven by environment:
#   MODE        resukisu (default) | lkm
#   CACHE       directory for fetched upstream patch sources
#   SUSFS_PIN   susfs4ksu commit to use (default 8c6da84)
#   WILD_PIN    WildKernels/kernel_patches commit (default 5a5d5d8)
#
# resukisu : KSU built-in + SUSFS + everything else
# lkm      : pure kernel (no KSU/SUSFS); KSU is injected at flash time
#            by the KSU manager app patching init_boot. SUSFS must be OFF
#            because fs/susfs.c references ksu_* symbols that only link with
#            CONFIG_KSU=y.
set +e
KROOT="$(pwd)"
MODE="${MODE:-resukisu}"
CACHE="${CACHE:-$KROOT/.build_cache}"
SUSFS_PIN="${SUSFS_PIN:-8c6da8443a622c8e5dfdea621d306b5143b40d01}"  # susfs4ksu gki-android15-6.6 tip
WILD_PIN="${WILD_PIN:-5a5d5d8}"
SUSFS_URL=https://github.com/ShirkNeko/susfs4ksu.git
WILD_URL=https://github.com/WildKernels/kernel_patches.git
RESUKISU_SETUP=https://raw.githubusercontent.com/ReSukiSU/ReSukiSU/main/kernel/setup.sh
BBG_SETUP=https://github.com/vc-teahouse/Baseband-guard/raw/main/setup.sh
HERE="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$CACHE"

log(){ echo "[features:$MODE] $*"; }

clone_pin(){ # url pin destdir
  local url="$1" pin="$2" dst="$3"
  if [ ! -d "$dst/.git" ]; then git clone --quiet "$url" "$dst" || return 1; fi
  git -C "$dst" fetch --quiet origin "$pin" 2>/dev/null
  git -C "$dst" checkout --quiet "$pin" 2>/dev/null || git -C "$dst" checkout --quiet "$pin"
}

try_patch(){ # patchfile label
  [ -f "$1" ] || { log "?miss $2"; return; }
  if /usr/bin/patch -p1 -R --dry-run -s -f --no-backup-if-mismatch <"$1" >/dev/null 2>&1; then log "=already $2"
  elif /usr/bin/patch -p1 --forward -F3 -s --no-backup-if-mismatch <"$1" >/dev/null 2>&1; then log "+ok $2"
  else log "!SKIP $2"; fi
  find . -name '*.rej' -delete 2>/dev/null; find . -name '*.orig' -delete 2>/dev/null; }

############################################################
log "1/7 ReSukiSU sources (KSU symlink must resolve at Kconfig time, even in lkm)"
curl -LSs "$RESUKISU_SETUP" | bash -s main >/dev/null 2>&1
grep -q 'drivers/kernelsu/Kconfig' drivers/Kconfig && log "  KSU sources present"

if [ "$MODE" = "resukisu" ]; then
  log "2/7 SUSFS @ $SUSFS_PIN"
  clone_pin "$SUSFS_URL" "$SUSFS_PIN" "$CACHE/susfs"
  cp -rf "$CACHE/susfs/kernel_patches/fs/." fs/
  cp -rf "$CACHE/susfs/kernel_patches/include/linux/." include/linux/
  /usr/bin/patch -p1 --forward --fuzz=3 < "$CACHE/susfs/kernel_patches/50_add_susfs_in_gki-android15-6.6.patch" >/dev/null 2>&1
  # namespace.c include-area skew: ensure the susfs_def.h include is present (idempotent).
  if ! grep -q "linux/susfs_def.h" fs/namespace.c; then
    perl -0pi -e 's{#include <linux/mnt_idmapping.h>\n}{#include <linux/mnt_idmapping.h>\n#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT\n#include <linux/susfs_def.h>\n#endif\n}' fs/namespace.c
  fi
  # selinuxfs: neutralise the fake-selinux-status spoof symbols ReSukiSU dropped
  perl -0pi -e 's/&& ksu_selinux_hide_enabled\)/&& 0)/g; s/data = fake_status;/data = NULL;/g; s/static_branch_unlikely\(&fake_status_initialize_key\) && !ret && !fake_status/0/g; s/initialize_fake_status\(\);/(void)0;/g; s/!ksu_selinux_hide_running/1/g' security/selinux/selinuxfs.c
  log "  susfs rejects: $(find . -name '*.rej'|wc -l); setresuid hook: $(grep -c ksu_handle_setresuid kernel/sys.c)"
  find . -name '*.rej' -delete 2>/dev/null; find . -name '*.orig' -delete 2>/dev/null
else
  log "2/7 SUSFS skipped (lkm: pure kernel)"
fi

log "3/7 Baseband-guard"
( export PATH="/usr/bin:/bin:$PATH"; curl -fsSL "$BBG_SETUP" | bash >/dev/null 2>&1 ) && log "  bbg ok"

log "4/7 Wild perf patches @ $WILD_PIN"
clone_pin "$WILD_URL" "$WILD_PIN" "$CACHE/wild"
W="$CACHE/wild/common"
for p in silence_irq_cpu_logspam f2fs_enlarge_min_fsync_blocks f2fs_reduce_congestion reduce_gc_thread_sleep_time \
  increase_ext4_default_commit_age clear_page_16bytes_align file_struct_8bytes_align disable_cache_hot_buddy \
  reduce_cache_pressure mem_opt_prefetch optimized_mem_operations int_sqrt add_timeout_wakelocks_globally \
  avoid_extra_s2idle_wake_attempts minimise_wakeup_time reduce_freeze_timeout reduce_pci_pme_wakeups \
  increase_sk_mem_packets force_tcp_nodelay; do try_patch "$W/$p.patch" "$p"; done

log "5/7 unicode + droidspaces + NTSync"
try_patch "$W/unicode_bypass_fix_6.1+.patch" unicode
try_patch "$W/droidspaces/fix_sysvipc_kabi_6_7_8.patch" droidspaces_kabi
# NTSync: vendored upstream driver (build/features/ntsync) -- no external fetch.
cp -f "$HERE/features/ntsync/ntsync.c" drivers/misc/ntsync.c
cp -f "$HERE/features/ntsync/ntsync.h" include/uapi/linux/ntsync.h
python3 - <<'PY'
import pathlib
kc = pathlib.Path('drivers/misc/Kconfig'); s = kc.read_text()
if 'config NTSYNC' not in s:
    stanza = ('config NTSYNC\n\ttristate "NT synchronization primitive emulation"\n'
              '\tdefault\tm\n\thelp\n'
              '\t  Kernel support for Windows NT synchronization primitive\n'
              '\t  emulation (Wine/Proton). If unsure, say N.\n\n')
    i = s.rfind('\nendmenu')
    kc.write_text((s + '\n' + stanza) if i < 0 else (s[:i+1] + stanza + s[i+1:]))
    print('  ntsync: Kconfig stanza inserted')
mk = pathlib.Path('drivers/misc/Makefile'); m = mk.read_text()
if 'CONFIG_NTSYNC)' not in m:
    mk.write_text(m + '\nobj-$(CONFIG_NTSYNC)\t+= ntsync.o\n')
    print('  ntsync: Makefile wired')
PY
log "  ntsync.c (vendored): $([ -f drivers/misc/ntsync.c ] && echo yes || echo no)"

log "6/7 Re:Kernel (vendored)"
mkdir -p drivers/rekernel
cp -f "$HERE/features/rekernel/." drivers/rekernel/ 2>/dev/null
cp -f "$HERE/features/rekernel/"* drivers/rekernel/
grep -q 'drivers/rekernel/Kconfig' drivers/Kconfig || sed -i '/^endmenu/i source "drivers/rekernel/Kconfig"' drivers/Kconfig
grep -q 'rekernel/' drivers/Makefile || printf '\nobj-$(CONFIG_REKERNEL) += rekernel/\n' >> drivers/Makefile
log "  rekernel wired: $(ls drivers/rekernel | tr '\n' ' ')"

log "7/7 IPv6 NAT hide (config_data scrubs CONFIG_IP6_NF_NAT=y so /proc/config.gz hides it)"
python3 - <<'PY'
import pathlib
p=pathlib.Path('kernel/Makefile'); s=p.read_text()
if 'IP6_NF_NAT_FIX_MARKER' not in s:
    blk='\n# IP6_NF_NAT_FIX_MARKER\ndefine config_fix\n\tif grep -q \'^CONFIG_IP6_NF_NAT=y\' $@; then sed -i \'s/^CONFIG_IP6_NF_NAT=y$$/CONFIG_IP6_NF_NAT=n/\' $@; fi\nendef\n'
    n='filechk_cat = cat $<\n'; i=s.find(n)
    if i>=0:
        s=s[:i+len(n)]+blk+s[i+len(n):]
        r='$(obj)/config_data: arch/arm64/configs/stock_gki_defconfig FORCE\n\t$(call filechk,cat)\n'
        if r in s: s=s.replace(r, r+'\t$(Q)$(config_fix)\n',1)
        p.write_text(s); print('[features] ipv6 hide applied')
PY
log "feature application complete"
