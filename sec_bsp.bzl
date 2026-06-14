# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.

__module_platform_map =  {
    "sun": {
        "perf": [
            # keep sorted & in-tree modules only
            "drivers/i2c/algos/i2c-algo-bit.ko",
            "drivers/i2c/busses/i2c-gpio.ko",
            "drivers/watchdog/softdog.ko",
            "drivers/samsung/bsp/blk_helper/sec_blk_helper.ko",
            "drivers/samsung/bsp/class/sec_class.ko",
            "drivers/samsung/bsp/key_notifier/sec_key_notifier.ko",
            "drivers/samsung/bsp/param/sec_param.ko",
            "drivers/samsung/bsp/qcom/param/sec_qc_param.ko",
            "drivers/samsung/bsp/reloc_gpio/sec_reloc_gpio.ko",
            "drivers/samsung/debug/arm64/ap_context/sec_arm64_ap_context.ko",
            "drivers/samsung/debug/arm64/debug/sec_arm64_debug.ko",
            "drivers/samsung/debug/arm64/fsimd_debug/sec_arm64_fsimd_debug.ko",
            "drivers/samsung/debug/boot_stat/sec_boot_stat.ko",
            "drivers/samsung/debug/common/sec_debug.ko",
            "drivers/samsung/debug/crashkey_long/sec_crashkey_long.ko",
            "drivers/samsung/debug/crashkey/sec_crashkey.ko",
            "drivers/samsung/debug/debug_region/sec_debug_region.ko",
            "drivers/samsung/debug/log_buf/sec_log_buf.ko",
            "drivers/samsung/debug/pmsg/sec_pmsg.ko",
            "drivers/samsung/debug/qcom/dbg_partition/sec_qc_dbg_partition.ko",
            "drivers/samsung/debug/qcom/debug/sec_qc_debug.ko",
            "drivers/samsung/debug/qcom/hw_param/sec_qc_hw_param.ko",
            "drivers/samsung/debug/qcom/logger/sec_qc_logger.ko",
            "drivers/samsung/debug/qcom/reboot_cmd/sec_qc_rbcmd.ko",
            "drivers/samsung/debug/qcom/reboot_reason/sec_qc_qcom_reboot_reason.ko",
            "drivers/samsung/debug/qcom/rst_exinfo/sec_qc_rst_exinfo.ko",
            "drivers/samsung/debug/qcom/smem/sec_qc_smem.ko",
            "drivers/samsung/debug/qcom/soc_id/sec_qc_soc_id.ko",
            "drivers/samsung/debug/qcom/summary/sec_qc_summary.ko",
            "drivers/samsung/debug/qcom/upload_cause/sec_qc_upload_cause.ko",
            "drivers/samsung/debug/qcom/user_reset/sec_qc_user_reset.ko",
            "drivers/samsung/debug/qcom/wdt_core/sec_qc_qcom_wdt_core.ko",
            "drivers/samsung/debug/rdx_bootdev/sec_rdx_bootdev.ko",
            "drivers/samsung/debug/reboot_cmd/sec_reboot_cmd.ko",
            "drivers/samsung/debug/upload_cause/sec_upload_cause.ko",
        ],
        "consolidate": [
            # keep sorted
        ],
    },
}

def sec_bsp(target, variant):
    if not target in __module_platform_map:
        return []

    target_map = __module_platform_map[target]
    if not variant in target_map:
        return []

    if variant == "consolidate":
        return target_map[variant] + target_map["perf"]

    return target_map[variant]
