// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifdef __NEVER_DEFINED__
	/* Rules start after this line */
	{feature_ped_exception,"/system/bin/run-as"},	/* DEFAULT */
	{feature_ped_exception,"/system/bin/dumpstate"},	/* DEFAULT */
	{feature_safeplace_path,"/init"},
	{feature_safeplace_path,"/system/bin/init"},
	{feature_safeplace_path,"/system/bin/app_process32"},
	{feature_safeplace_path,"/system/bin/app_process64"},
	{feature_safeplace_path,"/system/bin/blkid"},
	{feature_safeplace_path,"/system/bin/clatd"},
	{feature_safeplace_path,"/system/bin/cmd"},
	{feature_safeplace_path,"/system/bin/corehelper.sh"},
	{feature_safeplace_path,"/system/bin/crash_dump32"},
	{feature_safeplace_path,"/system/bin/crash_dump64"},
	{feature_safeplace_path,"/system/bin/debuggerd"},
	{feature_safeplace_path,"/system/bin/dnsmasq"},
	{feature_safeplace_path,"/system/bin/dsms"},
	{feature_safeplace_path,"/system/bin/dumpstate"},
	{feature_safeplace_path,"/system/bin/fsck.vfat"},
	{feature_safeplace_path,"/system/bin/fsck.exfat"},
	{feature_safeplace_path,"/system/bin/gatekeeperd"},
	{feature_safeplace_path,"/system/bin/healthd"},
	{feature_safeplace_path,"/system/bin/installd"},
	{feature_safeplace_path,"/system/bin/iod"},
	{feature_safeplace_path,"/system/bin/ip"},
	{feature_safeplace_path,"/system/bin/iptables"},
	{feature_safeplace_path,"/system/bin/iptables-restore"},
	{feature_safeplace_path,"/system/bin/ip6tables"},
	{feature_safeplace_path,"/system/bin/ip6tables-restore"},
	{feature_safeplace_path,"/system/bin/lmkd"},
	{feature_safeplace_path,"/system/bin/lshal"},
	{feature_safeplace_path,"/system/bin/mdf_fota"},
	{feature_safeplace_path,"/system/bin/mkfs.vfat"},
	{feature_safeplace_path,"/system/bin/mkfs.exfat"},
	{feature_safeplace_path,"/system/bin/netd"},
	{feature_safeplace_path,"/system/bin/nst"},
	{feature_safeplace_path,"/system/bin/perfmond"},
	{feature_safeplace_path,"/system/bin/perfprofd"},
	{feature_safeplace_path,"/system/bin/sgdisk"},
	{feature_safeplace_path,"/system/bin/sh"},
	{feature_safeplace_path,"/system/bin/ss"},
	{feature_safeplace_path,"/system/bin/storaged"},
	{feature_safeplace_path,"/system/bin/tc"},
	{feature_safeplace_path,"/system/bin/uncrypt"},
	{feature_safeplace_path,"/system/bin/vold"},
	{feature_safeplace_path,"/system/bin/webview_zygote32"},
	{feature_safeplace_path,"/system/bin/grep"},
	{feature_safeplace_path,"/system/bin/e2fsck"},
	{feature_safeplace_path,"/system/bin/scs"},
	{feature_safeplace_path,"/system/bin/vdc"},
	{feature_safeplace_path,"/system/bin/vaultkeeperd"},
	{feature_safeplace_path,"/system/bin/prepare_param.sh"},
	{feature_safeplace_path,"/system/bin/smdexe"},
	{feature_safeplace_path,"/system/bin/diagexe"},
	{feature_safeplace_path,"/system/bin/ddexe"},
	{feature_safeplace_path,"/system/bin/connfwexe"},
	{feature_safeplace_path,"/system/bin/at_distributor"},
	{feature_safeplace_path,"/system/bin/sdcard"},
	{feature_safeplace_path,"/system/bin/resetreason"},
	{feature_safeplace_path,"/system/bin/lpm"},
	{feature_safeplace_path,"/system/bin/resize2fs"},
	{feature_safeplace_path,"/system/bin/tune2fs"},
	{feature_safeplace_path,"/system/bin/patchoat"},
	{feature_safeplace_path,"/system/bin/knox_changer"},
	{feature_safeplace_path,"/system/bin/knox_changer_recovery"},
	{feature_safeplace_path,"/sbin/sswap"},
	{feature_safeplace_path,"/sbin/cbd"},
	{feature_safeplace_path,"/sbin/adbd"},
	{feature_safeplace_path,"/sbin/recovery"},
	{feature_safeplace_path,"/sbin/mke2fs_static"},
	{feature_safeplace_path,"/vendor/bin/hw/wpa_supplicant"},
	{feature_safeplace_path,"/vendor/bin/hw/macloader"},
	{feature_safeplace_path,"/vendor/bin/hw/mfgloader"},
	{feature_safeplace_path,"/sbin/dm_verity_hash"},
	{feature_safeplace_path,"/sbin/dm_verity_signature_checker"},
	{feature_safeplace_path,"/vendor/bin/qseecomd"},
	{feature_safeplace_path,"/system/bin/vold_prepare_subdirs"},
	{feature_safeplace_path,"/vendor/bin/init.qcom.early_boot.sh"},
	{feature_safeplace_path,"/vendor/bin/toybox_vendor"},
	{feature_safeplace_path,"/vendor/bin/toolbox"},
	{feature_safeplace_path,"/vendor/bin/hw/android.hardware.usb@1.1-service.wahoo"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.iop@2.0-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.perf@1.0-service"},
	{feature_safeplace_path,"/vendor/bin/init.qcom.class_core.sh"},
	{feature_safeplace_path,"/vendor/bin/irsc_util"},
	{feature_safeplace_path,"/vendor/bin/rmt_storage"},
	{feature_safeplace_path,"/system/bin/toybox"},
	{feature_safeplace_path,"/vendor/bin/init.qcom.usb.sh"},
	{feature_safeplace_path,"/vendor/bin/tftp_server"},
	{feature_safeplace_path,"/vendor/bin/init.qcom.sensors.sh"},
	{feature_safeplace_path,"/system/bin/insthk"},
	{feature_safeplace_path,"/vendor/bin/init.class_main.sh"},
	{feature_safeplace_path,"/vendor/bin/time_daemon"},
	{feature_safeplace_path,"/vendor/bin/thermal-engine"},
	{feature_safeplace_path,"/vendor/bin/thermal-engine-v2"},
	{feature_safeplace_path,"/system/bin/sec_diag_uart_log"},
	{feature_safeplace_path,"/vendor/bin/init.qcom.sh"},
	{feature_safeplace_path,"/system/bin/usbd"},
	{feature_safeplace_path,"/vendor/bin/init.qcom.post_boot.sh"},
	{feature_safeplace_path,"/system/bin/adbd"},
	{feature_safeplace_path,"/system/bin/atrace"},
	{feature_safeplace_path,"/system/bin/fsdbg"},
	{feature_safeplace_path,"/system/bin/dumpsys"},
	{feature_safeplace_path,"/system/bin/logcat"},
	{feature_safeplace_path,"/system/bin/toolbox"},
	{feature_safeplace_path,"/system/bin/mke2fs"},
	{feature_safeplace_path,"/vendor/bin/cbd"},
	{feature_safeplace_path,"/vendor/bin/adsprpcd"},
	{feature_safeplace_path,"/sbin/e2fsdroid_static"},
	{feature_safeplace_path,"/system/bin/e2fsdroid"},
	{feature_safeplace_path,"/system/bin/fsck.f2fs"},
	{feature_safeplace_path,"/system/bin/make_f2fs"},
	{feature_safeplace_path,"/system/bin/sload_f2fs"},
	{feature_safeplace_path,"/system/bin/bpfloader"},
	{feature_safeplace_path,"/system/bin/wait_for_keymaster"},
	{feature_safeplace_path,"/system/bin/secdiscard"},
	{feature_safeplace_path,"/system/bin/idledefrag"},
	{feature_safeplace_path,"/vendor/bin/init.mdm.sh"},
	{feature_safeplace_path,"/vendor/bin/mdm_helper"},
	{feature_safeplace_path,"/vendor/bin/ks"},
	{feature_safeplace_path,"/vendor/bin/sh"},
	{feature_safeplace_path,"/system/bin/e4defrag"},
	{feature_safeplace_path,"/sbin/dm_verity_tz_cmd"},
	{feature_safeplace_path,"/sbin/mcDriverDaemon_static"},
	{feature_safeplace_path,"/sbin/qseecomfsd"},
	{feature_safeplace_path,"/sbin/tzdaemon_recovery"},
	{feature_safeplace_path,"/vendor/bin/hvdcp_opti"},
	{feature_safeplace_path,"/sbin/mkfs.f2fs"},
	{feature_safeplace_path,"/sbin/sload.f2fs"},
	{feature_safeplace_path,"/system/bin/secilc"},
	{feature_safeplace_path,"/system/bin/apexd"},
	{feature_safeplace_path,"/system/bin/art_apex_boot_integrity"},
	{feature_safeplace_path,"/system/bin/gsid"},
	{feature_safeplace_path,"/system/bin/idmap2"},
	{feature_safeplace_path,"/system/bin/charger"},
	{feature_safeplace_path,"/system/bin/recovery"},
	{feature_safeplace_path,"/system/bin/watchdogd"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.perf@2.0-service"},
	{feature_safeplace_path,"/system/bin/netutils-wrapper-1.0"},
	{feature_safeplace_path,"/system/bin/bugreport"},
	{feature_safeplace_path,"/system/bin/minadbd"},
	{feature_safeplace_path,"/system/bin/migrate_legacy_obb_data.sh"},
	{feature_safeplace_path,"/vendor/bin/shsusrd"},
	{feature_safeplace_path,"/system/bin/defrag_f2fs"},
	{feature_safeplace_path,"/system/bin/fastbootd"},
	{feature_safeplace_path,"/system/bin/sbm"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.perf@2.1-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.perf@2.2-service"},
	{feature_safeplace_path,"/vendor/bin/grep"},
	{feature_safeplace_path,"/vendor/bin/memlogd"},
	{feature_safeplace_path,"/vendor/bin/init.insmod.sh"},
	{feature_safeplace_path,"/vendor/bin/hw/android.hardware.usb@1.3-service.coral"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.perf-hal-service"},
	{feature_safeplace_path,"/vendor/bin/iod"},
	{feature_safeplace_path,"/vendor/bin/dsmsca"},
	{feature_safeplace_path,"/vendor/bin/hqread"},
	{feature_safeplace_path,"/system/bin/hqcpsnbin"},
	{feature_safeplace_path,"/system/bin/awk"},
	{feature_safeplace_path,"/system/bin/bc"},
	{feature_safeplace_path,"/system/bin/service"},
	{feature_safeplace_path,"/system/bin/fsck_msdos"},
	{feature_safeplace_path,"/vendor/bin/hcgcd"},
	{feature_safeplace_path,"/vendor/bin/qlm-service"},
	{feature_safeplace_path,"/system/bin/rdxd"},
	{feature_safeplace_path,"/system/bin/ztd"},
	{feature_safeplace_path,"/system/bin/fsck.exfat_sec"},
	{feature_safeplace_path,"/system/bin/mkfs.exfat_sec"},
	{feature_safeplace_path,"/system/bin/ksmbd.tools"},
	{feature_safeplace_path,"/system/system_ext/bin/dpmd"},
	{feature_safeplace_path,"/system_ext/bin/dpmd"},
	{feature_safeplace_path,"/vendor/bin/init.qti.dcvs.sh"},
	{feature_safeplace_path,"/vendor/bin/vendor_modprobe.sh"},
	{feature_safeplace_path,"/vendor/bin/init.qti.qcv.sh"},
	{feature_safeplace_path,"/vendor/bin/init.qcom.crashdata.sh"},
	{feature_safeplace_path,"/vendor/bin/energy-awareness"},
	{feature_safeplace_path,"/vendor/bin/qcom-system-daemon"},
	{feature_safeplace_path,"/vendor/bin/init.qti.kernel.sh"},
	{feature_safeplace_path,"/vendor/bin/init.kernel.post_boot.sh"},
	{feature_safeplace_path,"/vendor/bin/init.kernel.post_boot-lahaina.sh"},
	{feature_safeplace_path,"/vendor/bin/init.qti.early_init.sh"},
	{feature_safeplace_path,"/vendor/bin/init.qti.keymaster.sh"},
	{feature_safeplace_path,"/vendor/bin/init.qti.write.sh"},
	{feature_safeplace_path,"/vendor/bin/vmmgr"},
	{feature_safeplace_path,"/product/bin/qvirtmgr"},
	{feature_safeplace_path,"/system_ext/bin/qcrosvm"},
	{feature_safeplace_path,"/vendor/bin/ssr_setup"},
	{feature_safeplace_path,"/product/bin/vendor.qti.qvirt-service_rs"},
	{feature_safeplace_path,"/product/bin/vendor.qti.qvirt-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.debugutils-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.perf2-hal-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.limits@1.1-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.hardware.limits@1.2-service"},
	{feature_safeplace_path,"/vendor/bin/hw/android.hardware.thermal@2.0-service.qti-v2"},
	{feature_safeplace_path,"/vendor/bin/poweropt-service"},
	{feature_safeplace_path,"/vendor/bin/msm_irqbalance"},
	{feature_safeplace_path,"/vendor/bin/qms"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.qti.MemHal-service"},
	{feature_safeplace_path,"/vendor/bin/hw/android.hardware.boot-service.qti"},
	{feature_safeplace_path,"/vendor/bin/cp_diskserver"},
	{feature_safeplace_path,"/vendor/bin/hw/android.hardware.boot-service.exynos"},
	{feature_safeplace_path,"/system_ext/bin/mobile_log_d"},
	{feature_safeplace_path,"/vendor/bin/aee_aedv"},
	{feature_safeplace_path,"/vendor/bin/aee_aedv64"},
	{feature_safeplace_path,"/vendor/bin/dcxosetcap"},
	{feature_safeplace_path,"/vendor/bin/frs"},
	{feature_safeplace_path,"/vendor/bin/meta_tst"},
	{feature_safeplace_path,"/vendor/bin/nvram_daemon"},
	{feature_safeplace_path,"/vendor/bin/thermal_core"},
	{feature_safeplace_path,"/vendor/bin/thermal_manager"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.mediatek.hardware.mtkpower-service.mediatek"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.mediatek.hardware.nvram-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.mediatek.hardware.nvram@1.1-service"},
	{feature_safeplace_path,"/system/bin/oem-iptables-init.sh"},
	{feature_safeplace_path,"/system_ext/bin/aee_aed64_v2"},
	{feature_safeplace_path,"/system_ext/bin/aee_v2"},
	{feature_safeplace_path,"/vendor/bin/aee_aedv64_v2"},
	{feature_safeplace_path,"/vendor/bin/aee_dumpstatev_v2"},
	{feature_safeplace_path,"/vendor/bin/dmabuf_dump"},
	{feature_safeplace_path,"/vendor/bin/mrdump_tool"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.mediatek.hardware.mtkpower@1.0-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.mediatek.hardware.aee@V1-service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.mediatek.hardware.aee@1.1-service"},
	{feature_safeplace_path,"/system/bin/linkerconfig"},
	{feature_safeplace_path,"/system/bin/snapshotctl"},
	{feature_safeplace_path,"/system/bin/boringssl_self_test32"},
	{feature_safeplace_path,"/system/bin/boringssl_self_test64"},
	{feature_safeplace_path,"/vendor/bin/boringssl_self_test32"},
	{feature_safeplace_path,"/vendor/bin/boringssl_self_test64"},
	{feature_safeplace_path,"/apex/com.android.adbd/bin/adbd"},
	{feature_safeplace_path,"/apex/com.android.sdkext/bin/derive_sdk"},
	{feature_safeplace_path,"/apex/com.android.conscrypt/bin/boringssl_self_test32"},
	{feature_safeplace_path,"/apex/com.android.conscrypt/bin/boringssl_self_test64"},
	{feature_safeplace_path,"/system/bin/applypatch"},
	{feature_safeplace_path,"/vendor/bin/applypatch"},
	{feature_safeplace_path,"/system/bin/clean_scratch_files"},
	{feature_safeplace_path,"/system/bin/fsverity"},
	{feature_safeplace_path,"/system/bin/fsverity_init"},
	{feature_safeplace_path,"/system/xbin/librank"},
	{feature_safeplace_path,"/system/xbin/procrank"},
	{feature_safeplace_path,"/system/xbin/showmap"},
	{feature_safeplace_path,"/system/bin/librank"},
	{feature_safeplace_path,"/system/bin/procrank"},
	{feature_safeplace_path,"/system/bin/showmap"},
	{feature_safeplace_path,"/product/bin/dmabuf_dump"},
	{feature_safeplace_path,"/system/bin/dmabuf_dump"},
	{feature_safeplace_path,"/apex/com.android.runtime/bin/spqr"},
	{feature_safeplace_path,"/system/bin/perfetto"},
	{feature_safeplace_path,"/system/bin/update_verifier"},
	{feature_safeplace_path,"/system/bin/bootstrap/linkerconfig"},
	{feature_safeplace_path,"/apex/com.android.runtime/bin/linkerconfig"},
	{feature_safeplace_path,"/system/bin/otapreopt_slot"},
	{feature_safeplace_path,"/apex/com.android.art/bin/dex2oat32"},
	{feature_safeplace_path,"/apex/com.android.art/bin/dex2oat64"},
	{feature_safeplace_path,"/system/bin/incident"},
	{feature_safeplace_path,"/system/bin/odsign"},
	{feature_safeplace_path,"/apex/com.android.art/bin/odrefresh"},
	{feature_safeplace_path,"/apex/com.android.art/bin/artd"},
	{feature_safeplace_path,"/apex/com.android.runtime/bin/crash_dump32"},
	{feature_safeplace_path,"/apex/com.android.runtime/bin/crash_dump64"},
	{feature_safeplace_path,"/system/bin/lpdump"},
	{feature_safeplace_path,"/system/bin/extra_free_kbytes.sh"},
	{feature_safeplace_path,"/system/bin/bpfloader"},
	{feature_safeplace_path,"/system/bin/btfloader"},
	{feature_safeplace_path,"/vendor/bin/system_dlkm_modprobe.sh"},
	{feature_safeplace_path,"/apex/com.android.art/bin/art_boot"},
	{feature_safeplace_path,"/system/bin/update_engine"},
	{feature_safeplace_path,"/system/bin/otapreopt"},
	{feature_safeplace_path,"/system/bin/otapreopt_chroot"},
	{feature_safeplace_path,"/apex/com.android.sdkext/bin/derive_classpath"},
	{feature_safeplace_path,"/apex/com.android.sdkext@340819010.tmp/bin/derive_classpath"},
	{feature_safeplace_path,"/system/bin/mtectrl"},
	{feature_safeplace_path,"/apex/com.android.tethering/bin/netbpfload"},
	{feature_safeplace_path,"/system/bin/misctrl"},
	{feature_safeplace_path,"/system/bin/kcmdlinectrl"},
	{feature_safeplace_path,"/system/bin/appops"},
	{feature_safeplace_path,"/system/bin/content"},
	{feature_safeplace_path,"/system/bin/locksettings"},
	{feature_safeplace_path,"/system/bin/svc"},
	{feature_safeplace_path,"/system/bin/vintf"},
	{feature_safeplace_path,"/system/bin/wm"},
	{feature_safeplace_path,"/system/bin/cplogserver"},
	{feature_safeplace_path,"/system_ext/bin/modemlog_connmgr_service"},
	{feature_safeplace_path,"/vendor/bin/hw/vendor.unisoc.hardware.power-service"},
	{feature_safeplace_path,"/vendor/bin/modem_control"},
	{feature_safeplace_path,"/vendor/bin/refnotify"},
	{feature_safeplace_path,"/vendor/bin/sprdstorageproxyd"},
	{feature_safeplace_path,"/vendor/bin/uniber"},
	{feature_safeplace_path,"/tmp/update_binary"},
	{feature_safeplace_path,"/tmp/update-binary"},
	{feature_safeplace_path,"/postinstall/bin/checkpoint_gc"},
	{feature_safeplace_path,"/postinstall/system/bin/otapreopt_script"},
	{feature_safeplace_path,"/system/bin/install-recovery.sh"},	/* DEFAULT */
	{feature_safeplace_path,"/vendor/bin/install-recovery.sh"},	/* DEFAULT */
	{feature_safeplace_path,"/system/bin/bpfloader"},	/* DEFAULT */
	{feature_safeplace_path,"/system/bin/snapuserd"},	/* DEFAULT */
	{feature_immutable_path_write,"/system/"},	/* DEFAULT */
	{feature_immutable_path_write,"/vendor/"},	/* DEFAULT */
	{feature_immutable_path_open,"/system/bin/app_process64"},
	{feature_immutable_path_open,"/system/bin/"},	/* DEFAULT */
	{feature_immutable_path_open,"/vendor/bin/"},	/* DEFAULT */
	{feature_immutable_root,"/apex/com.android.adbd/bin/adbd:/data/local/tmp/"},	/* DEFAULT */
	{feature_immutable_root,"/apex/com.android.art/bin/odrefresh:/data/misc/apexdata/com.android.art/"},	/* DEFAULT */
	{feature_immutable_root,"/apex/com.android.art/bin/odrefresh:/data/misc/odrefresh/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/apexd:/data/apex/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/apexd:/data/app-staging/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/apexd:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/apexd:/data/misc_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process32:/data/misc/apexdata/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process32:/data/resource-cache/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process32:/data/user/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process32:/data/user_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process64:/data/local/tmp/remote_access.dex"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process64:/data/log/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process64:/data/misc/apexdata/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process64:/data/resource-cache/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process64:/data/user/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/app_process64:/data/user_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/cmd:/data/local/tmp/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/anr/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/log/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/misc/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/system/dropbox/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/system/shutdown-checkpoints"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/system/users/service/data/eRR.p"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/tombstones/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/dumpstate:/data/user_de/0/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/fsdbg:/data/log/fsdbg/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/gatekeeperd:/data/misc/gatekeeper/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/gsid:/data/gsi/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/idmap2:/data/resource-cache/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/bootchart/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/property/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/sec/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/sec_maintenance/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/system_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/unencrypted/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/vendor/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/init:/data/vendor_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/app/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/app-staging/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/dalvik-cache/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/data/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/log/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/media/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/misc/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/misc_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/user/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/installd:/data/user_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/ksmbd.tools:/data/misc/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/netd:/data/misc/net/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/odsign:/data/misc/apexdata/com.android.art/dalvik-cache/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/odsign:/data/misc/odsign/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/perfetto:/data/local/traces/.trace-in-progress_in_perfd.trace"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/profcollectctl:/data/misc/profcollectd/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/rdxd:/data/rdx_dump"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/secdiscard:/data/sec_backup_sys_de/vold/user_keys/ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/secdiscard:/data/sec_backup_sys_de/vold/user_keys/de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/secdiscard:/data/misc/vold/user_keys/ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/secdiscard:/data/misc/vold/user_keys/de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/secdiscard:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/sh:/data/local/tmp/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/sh:/data/local/traces/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/sh:/data/log/traces/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/sh:/data/lost+found/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/storaged:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/anr"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/app/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/data/com.android.cts.install.lib.testapp.A/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/data/com.android.cts.install.lib.testapp.B/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/data/com.google.android.gts.rollback"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/local/tmp/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/local/traces/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/log/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/media/0/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/misc/vold/user_keys/ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/misc/vold/user_keys/de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/misc_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/property/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/sec/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/sec_backup_sys_de/vold/user_keys/ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/sec_backup_sys_de/vold/user_keys/de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/sec_maintenance/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/system/packages.list"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/user_de/0/com.android.cts.install.lib.testapp.A/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/user_de/0/com.android.cts.install.lib.testapp.B/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/user_de/0/com.google.android.gts.rollback"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/vendor_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/toybox:/data/vendor_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/update_engine:/data/fota/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/update_engine:/data/log/update_engine_log/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/update_engine:/data/misc/update_engine/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/update_engine:/data/misc/update_engine_log"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/update_engine:/data/ota_package/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/app/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/data.new"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/incremental"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_maintenance.new"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/data/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/log/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/media/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/misc/vold/user_keys/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/misc_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec.new/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_backup_ce.new/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_backup_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_backup_de.new"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_backup_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_backup_sys_de/vold/user_keys/ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_backup_sys_de/vold/user_keys/de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_backup_unencrypted/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_maintenance/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/sec_pass/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/system/users/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/system_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/system_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/unencrypted/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/user/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/user_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/vendor_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold:/data/vendor_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold_prepare_subdirs:/data/misc/apexdata/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold_prepare_subdirs:/data/misc_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold_prepare_subdirs:/data/misc_de/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold_prepare_subdirs:/data/sec_maintenance/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold_prepare_subdirs:/data/vendor_ce/"},	/* DEFAULT */
	{feature_immutable_root,"/system/bin/vold_prepare_subdirs:/data/vendor_de/"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/hw/vendor.qti.hardware.perf2-hal-service:/data/vendor/"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/hw/vendor.qti.hardware.perf-hal-service:/data/vendor/perfd/"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/iod:/data/vendor/log/strhis.txt"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/qguard:/data/vendor/qguard/"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/qms:/data/vendor/qms_logs/main/"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/sh:/data/vendor/modem_config/"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/sh:/data/vendor/perfd/default_scaling_min_freq"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/sh:/data/vendor/sensors/diag_cfg"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/sh:/data/vendor/sensors/scripts"},	/* DEFAULT */
	{feature_immutable_root,"/vendor/bin/toybox_vendor:/data/vendor/modem_config/ver_info.txt"},	/* DEFAULT */
	{feature_immutable_src_exception,"/system/bin/icd"},
	{feature_immutable_src_exception,"/system/bin/iof"},
	{feature_immutable_src_exception,"/system/bin/sh"},
	{feature_immutable_src_exception,"/system/bin/app_process32"},
	{feature_immutable_src_exception,"/system/bin/app_process64"},
	{feature_immutable_src_exception,"/system/bin/crash_dump32"},
	{feature_immutable_src_exception,"/system/bin/crash_dump64"},
	{feature_immutable_src_exception,"/system/bin/mediaextractor"},
	{feature_immutable_src_exception,"/system/bin/surfaceflinger"},
	{feature_immutable_src_exception,"/vendor/bin/sh"},
	{feature_immutable_src_exception,"/vendor/bin/hw/android.hardware.media.omx@1.0-service"},
	{feature_immutable_src_exception,"/vendor/bin/snap_utility_32"},
	{feature_immutable_src_exception,"/vendor/bin/snap_utility_64"},
	{feature_immutable_src_exception,"/vendor/bin/icd_vendor"},
	{feature_immutable_src_exception,"/vendor/bin/iof_vendor"},
	{feature_immutable_src_exception,"/init"},
	{feature_immutable_src_exception,"/system/bin/init"},
	{feature_immutable_src_exception,"/system/bin/lshal"},
	{feature_immutable_src_exception,"/vendor/bin/hw/vendor.samsung.hardware.camera.provider-service_64"},
	{feature_immutable_src_exception,"/apex/com.android.runtime/bin/crash_dump32"},	/* DEFAULT */
	{feature_immutable_src_exception,"/apex/com.android.runtime/bin/crash_dump64"},	/* DEFAULT */
	{feature_immutable_src_exception,"/data/local/tests/unrestricted/CtsBionicTestCases/arm64/CtsBionicTestCases"},	/* DEFAULT */
	{feature_immutable_src_exception,"/data/local/tests/unrestricted/CtsBionicTestCases/arm/CtsBionicTestCases"},	/* DEFAULT */
	{feature_integrity_check,"/vendor/bin/hw/android.hardware.gatekeeper@1.0-service"},
	{feature_integrity_check,"/vendor/bin/hw/android.hardware.keymaster@4.0-service"},
	{feature_integrity_check,"/vendor/bin/hw/android.hardware.security.keymint-service"},
	{feature_integrity_check,"/vendor/bin/hw/vendor.samsung.hardware.tlc.kg@1.0-service"},
	{feature_integrity_check,"/vendor/bin/hw/vendor.samsung.hardware.security.wsm-service"},
	{feature_integrity_check,"/vendor/bin/vendor.samsung.hardware.security.wsm@1.0-service"},
	{feature_integrity_check,"/vendor/bin/vaultkeeperd"},
	{feature_integrity_check,"/vendor/bin/hw/vendor.samsung.hardware.tlc.kg@1.1-service"},
	/* Rules will be added here */
	/* Never modify the above line. Rules will be added for buildtime */
#endif /* __NEVER_DEFINED__ */
