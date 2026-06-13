load("//build/kernel/kleaf:kernel.bzl", "ddk_module")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")
load("//msm-kernel:target_variants.bzl", "get_all_variants")
load(":project_defconfig.bzl", "get_project_defconfig")

def _define_module(target, variant):
    tv = "{}_{}".format(target, variant)
    deps = [
        ":camera_headers",
        ":camera_banner",
        "//msm-kernel:all_headers",
    ]

    # Generate the defconfig file dynamically
    native.genrule(
        name = "{}_defconfig_generated".format(tv),
        srcs = [
            # Use the base target/variant defconfig to start
            # and concatenate and project-specific config
            #"{}_defconfig".format(tv),
            "{}_defconfig".format(target),
            get_project_defconfig(target, variant),
        ],
        outs = ["{}_defconfig.generated".format(tv)],
        cmd = "cat $(SRCS) > $@",
    )

    if target == "pineapple":
        deps.extend([
            "//vendor/qcom/opensource/synx-kernel:synx_headers",
            "//vendor/qcom/opensource/synx-kernel:{}_modules".format(tv),
            "//vendor/qcom/opensource/securemsm-kernel:smcinvoke_kernel_headers",
            "//vendor/qcom/opensource/securemsm-kernel:smmu_proxy_headers",
            "//vendor/qcom/opensource/securemsm-kernel:{}_smcinvoke_dlkm".format(tv),
            "//vendor/qcom/opensource/securemsm-kernel:{}_smmu_proxy_dlkm".format(tv),
            "//vendor/qcom/opensource/mmrm-driver:{}_mmrm_driver".format(tv),
        ])
    if target == "sun":
        deps.extend([
            "//vendor/qcom/opensource/synx-kernel:synx_headers",
            "//vendor/qcom/opensource/synx-kernel:{}_modules".format(tv),
            "//vendor/qcom/opensource/securemsm-kernel:smcinvoke_kernel_headers",
            "//vendor/qcom/opensource/securemsm-kernel:smmu_proxy_headers",
            "//vendor/qcom/opensource/securemsm-kernel:{}_smcinvoke_dlkm".format(tv),
            "//vendor/qcom/opensource/securemsm-kernel:{}_smmu_proxy_dlkm".format(tv),
            "//vendor/qcom/opensource/mmrm-driver:{}_mmrm_driver".format(tv),
        ])
    ddk_module(
        name = "{}_camera".format(tv),
        out = "camera.ko",
        srcs = [
            "drivers/cam_req_mgr/cam_req_mgr_core.c",
            "drivers/cam_req_mgr/cam_req_mgr_dev.c",
            "drivers/cam_req_mgr/cam_req_mgr_util.c",
            "drivers/cam_req_mgr/cam_mem_mgr.c",
            "drivers/cam_req_mgr/cam_req_mgr_workq.c",
            "drivers/cam_req_mgr/cam_req_mgr_timer.c",
            "drivers/cam_req_mgr/cam_req_mgr_debug.c",
            "drivers/cam_utils/cam_soc_util.c",
            "drivers/cam_utils/cam_packet_util.c",
            "drivers/cam_utils/cam_debug_util.c",
            "drivers/cam_utils/cam_trace.c",
            "drivers/cam_utils/cam_common_util.c",
            "drivers/cam_utils/cam_compat.c",
            "drivers/cam_core/cam_context.c",
            "drivers/cam_core/cam_context_utils.c",
            "drivers/cam_core/cam_node.c",
            "drivers/cam_core/cam_subdev.c",
            "drivers/cam_smmu/cam_smmu_api.c",
            "drivers/cam_sync/cam_sync.c",
            "drivers/cam_sync/cam_sync_util.c",
            "drivers/cam_sync/cam_sync_dma_fence.c",
            "drivers/cam_cpas/cpas_top/cam_cpastop_hw.c",
            "drivers/cam_cpas/camss_top/cam_camsstop_hw.c",
            "drivers/cam_cpas/cam_cpas_soc.c",
            "drivers/cam_cpas/cam_cpas_intf.c",
            "drivers/cam_cpas/cam_cpas_hw.c",
            "drivers/cam_cdm/cam_cdm_soc.c",
            "drivers/cam_cdm/cam_cdm_util.c",
            "drivers/cam_cdm/cam_cdm_intf.c",
            "drivers/cam_cdm/cam_cdm_core_common.c",
            "drivers/cam_cdm/cam_cdm_virtual_core.c",
            "drivers/cam_cdm/cam_cdm_hw_core.c",
            "drivers/cam_utils/cam_soc_icc.c",
            "drivers/cam_vmrm/cam_vmrm_interface.c",
            "drivers/camera_main.c",
        ],
        conditional_srcs = {
            "CONFIG_TARGET_SYNX_ENABLE": {
                True: ["drivers/cam_sync/cam_sync_synx.c"],
            },
            "CONFIG_QCOM_CX_IPEAK": {
                True: ["drivers/cam_utils/cam_cx_ipeak.c"],
            },
            "CONFIG_INTERCONNECT_QCOM": {
                True: ["drivers/cam_utils/cam_soc_icc.c"],
            },
            "CONFIG_SPECTRA_ISP": {
                True: [
                    "drivers/cam_isp/isp_hw_mgr/hw_utils/cam_tasklet_util.c",
                    "drivers/cam_isp/isp_hw_mgr/hw_utils/cam_isp_packet_parser.c",
                    "drivers/cam_isp/isp_hw_mgr/hw_utils/irq_controller/cam_irq_controller.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_dev.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_soc.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_common.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_hw_ver1.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_hw_ver2.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_mod.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/ife_csid_hw/cam_ife_csid_lite_mod.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/cam_sfe_soc.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/cam_sfe_dev.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/cam_sfe_core.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_top/cam_sfe_top.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_bus/cam_sfe_bus.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_bus/cam_sfe_bus_rd.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/sfe_hw/sfe_bus/cam_sfe_bus_wr.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_soc.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_dev.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/cam_vfe_core.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_ver2.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_rd_ver1.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_bus/cam_vfe_bus_ver3.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_lite_ver2.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_common.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_ver4.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_ver3.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_top_ver2.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_ver2.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_ver3.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_rdi.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_fe_ver1.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe_top/cam_vfe_camif_lite_ver3.c",
                    "drivers/cam_isp/isp_hw_mgr/isp_hw/vfe_hw/vfe17x/cam_vfe.c",
                    "drivers/cam_isp/isp_hw_mgr/cam_isp_hw_mgr.c",
                    "drivers/cam_isp/isp_hw_mgr/cam_ife_hw_mgr.c",
                    "drivers/cam_isp/cam_isp_dev.c",
                    "drivers/cam_isp/cam_isp_context.c",
                ],
            },
            "CONFIG_SPECTRA_ICP": {
                True: [
                    "drivers/cam_icp/icp_hw/icp_hw_mgr/cam_icp_hw_mgr.c",
                    "drivers/cam_icp/icp_hw/ipe_hw/ipe_dev.c",
                    "drivers/cam_icp/icp_hw/ipe_hw/ipe_core.c",
                    "drivers/cam_icp/icp_hw/ipe_hw/ipe_soc.c",
                    "drivers/cam_icp/icp_hw/icp_proc/icp_v1_hw/cam_icp_v1_dev.c",
                    "drivers/cam_icp/icp_hw/icp_proc/icp_v1_hw/cam_icp_v1_core.c",
                    "drivers/cam_icp/icp_hw/icp_proc/icp_v2_hw/cam_icp_v2_dev.c",
                    "drivers/cam_icp/icp_hw/icp_proc/icp_v2_hw/cam_icp_v2_core.c",
                    "drivers/cam_icp/icp_hw/icp_proc/icp_common/cam_icp_proc_common.c",
                    "drivers/cam_icp/icp_hw/icp_proc/icp_common/cam_icp_soc_common.c",
                    "drivers/cam_icp/icp_hw/icp_proc/cam_icp_proc.c",
                    "drivers/cam_icp/icp_hw/bps_hw/bps_dev.c",
                    "drivers/cam_icp/icp_hw/bps_hw/bps_core.c",
                    "drivers/cam_icp/icp_hw/bps_hw/bps_soc.c",
                    "drivers/cam_icp/icp_hw/ofe_hw/ofe_dev.c",
                    "drivers/cam_icp/icp_hw/ofe_hw/ofe_core.c",
                    "drivers/cam_icp/icp_hw/ofe_hw/ofe_soc.c",
                    "drivers/cam_icp/cam_icp_subdev.c",
                    "drivers/cam_icp/cam_icp_context.c",
                    "drivers/cam_icp/hfi.c",
                ],
            },
            "CONFIG_SPECTRA_JPEG": {
                True: [
                    "drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_dev.c",
                    "drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_core.c",
                    "drivers/cam_jpeg/jpeg_hw/jpeg_enc_hw/jpeg_enc_soc.c",
                    "drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_dev.c",
                    "drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_core.c",
                    "drivers/cam_jpeg/jpeg_hw/jpeg_dma_hw/jpeg_dma_soc.c",
                    "drivers/cam_jpeg/jpeg_hw/cam_jpeg_hw_mgr.c",
                    "drivers/cam_jpeg/cam_jpeg_dev.c",
                    "drivers/cam_jpeg/cam_jpeg_context.c",
                ],
            },
            "CONFIG_SPECTRA_SENSOR": {
                True: [
                    "drivers/cam_sensor_module/cam_actuator/cam_actuator_dev.c",
                    "drivers/cam_sensor_module/cam_actuator/cam_actuator_core.c",
                    "drivers/cam_sensor_module/cam_actuator/cam_actuator_soc.c",
                    "drivers/cam_sensor_module/cam_cci/cam_cci_dev.c",
                    "drivers/cam_sensor_module/cam_cci/cam_cci_core.c",
                    "drivers/cam_sensor_module/cam_cci/cam_cci_soc.c",
                    "drivers/cam_sensor_module/cam_tpg/cam_tpg_dev.c",
                    "drivers/cam_sensor_module/cam_tpg/cam_tpg_core.c",
                    "drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw.c",
                    "drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_common.c",
                    "drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_0/tpg_hw_v_1_0.c",
                    "drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_2/tpg_hw_v_1_2.c",
                    "drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_3/tpg_hw_v_1_3.c",
                    "drivers/cam_sensor_module/cam_tpg/tpg_hw/tpg_hw_v_1_4/tpg_hw_v_1_4.c",
                    "drivers/cam_sensor_module/cam_csiphy/cam_csiphy_soc.c",
                    "drivers/cam_sensor_module/cam_csiphy/cam_csiphy_dev.c",
                    "drivers/cam_sensor_module/cam_csiphy/cam_csiphy_core.c",
                    "drivers/cam_sensor_module/cam_eeprom/cam_eeprom_dev.c",
                    "drivers/cam_sensor_module/cam_eeprom/cam_eeprom_core.c",
                    "drivers/cam_sensor_module/cam_eeprom/cam_eeprom_soc.c",
                    "drivers/cam_sensor_module/cam_ois/cam_ois_dev.c",
                    "drivers/cam_sensor_module/cam_ois/cam_ois_core.c",
                    "drivers/cam_sensor_module/cam_ois/cam_ois_soc.c",
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_dev.c",
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_core.c",
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_soc.c",
                    "drivers/cam_sensor_module/cam_sensor_io/cam_sensor_io.c",
                    "drivers/cam_sensor_module/cam_sensor_io/cam_sensor_cci_i2c.c",
                    "drivers/cam_sensor_module/cam_sensor_io/cam_sensor_qup_i2c.c",
                    "drivers/cam_sensor_module/cam_sensor_io/cam_sensor_qup_i3c.c",
                    "drivers/cam_sensor_module/cam_sensor_io/cam_sensor_spi.c",
                    "drivers/cam_sensor_module/cam_sensor_utils/cam_sensor_util.c",
                    "drivers/cam_sensor_module/cam_res_mgr/cam_res_mgr.c",
                    "drivers/cam_sensor_module/cam_flash/cam_flash_dev.c",
                    "drivers/cam_sensor_module/cam_flash/cam_flash_core.c",
                    "drivers/cam_sensor_module/cam_flash/cam_flash_soc.c",
                    "drivers/cam_sensor_module/cam_sensor_module_debug.c",
                ],
            },
            "CONFIG_SPECTRA_CUSTOM": {
                True: [
                    "drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw1/cam_custom_sub_mod_soc.c",
                    "drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw1/cam_custom_sub_mod_dev.c",
                    "drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw1/cam_custom_sub_mod_core.c",
                    "drivers/cam_cust/cam_custom_hw_mgr/cam_custom_csid/cam_custom_csid_dev.c",
                    "drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw_mgr.c",
                    "drivers/cam_cust/cam_custom_dev.c",
                    "drivers/cam_cust/cam_custom_context.c",
                ],
            },
            "CONFIG_QCOM_BUS_SCALING": {
                True: ["drivers/cam_utils/cam_soc_bus.c"],
            },
            "CONFIG_CAM_PRESIL": {
                # Sources need to be available to specify
                # True: [
                #     "drivers/cam_presil/presil/cam_presil_io_util.c",
                #     "drivers/cam_presil/presil/cam_presil_hw_access.c",
                #     "drivers/cam_presil/presil_framework_dev/cam_presil_framework_dev.c",
                # ],
                False: [
                    "drivers/cam_presil/stub/cam_presil_hw_access_stub.c",
                    "drivers/cam_utils/cam_io_util.c",
                ],
            },
            "CONFIG_SPECTRA_VMRM": {
                True: ["drivers/cam_vmrm/qrtr/cam_qrtr_comms.c"],
            },
            "CONFIG_CAMERA_FRAME_CNT_DBG": {
                True: ["drivers/cam_sensor_module/cam_sensor/cam_sensor_thread.c"],
            },
            "CONFIG_CAMERA_SYSFS_V2": {
                True: ["drivers/cam_sensor_module/cam_sensor_utils/cam_sysfs_init.c",
                       "drivers/cam_sensor_module/cam_eeprom/cam_sec_eeprom_core.c",
                       "drivers/cam_sensor_module/cam_actuator/cam_sec_actuator_core.c", ],
            },
            "CONFIG_SENSOR_RETENTION": {
                True: [
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_retention.c",
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_s5khp2.c",
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_s5kgn3.c",
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_imx854.c",
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_s5kjn3.c"
                ],
            },
            "CONFIG_USE_CAMERA_HW_BIG_DATA": {
                True: ["drivers/cam_sensor_module/cam_sensor_utils/cam_hw_bigdata.c"],
            },
            "CONFIG_CAMERA_CDR_TEST": {
                True: ["drivers/cam_sensor_module/cam_sensor_utils/cam_clock_data_recovery.c"],
            },
            "CONFIG_SAMSUNG_WACOM_NOTIFIER": {
                True: ["drivers/cam_utils/cam_notifier.c"],
            },
            "CONFIG_SEC_KUNIT": {
                True: [
                    "drivers/cam_sensor_module/cam_sensor_utils/kunit_test/cam_hw_bigdata_test.c",
                    "drivers/cam_sensor_module/cam_sensor_utils/kunit_test/cam_clock_data_recovery_test.c",
                    "drivers/cam_sensor_module/cam_sensor_utils/kunit_test/cam_sensor_util_unit_test.c",
                    "drivers/cam_sensor_module/cam_sensor_utils/kunit_test/cam_sec_eeprom_core_test.c",
                    "drivers/kunit_test/camera_kunit_main.c"],
            },
            "CONFIG_CAMERA_ADAPTIVE_MIPI": {
                True: [
                    "drivers/cam_sensor_module/cam_sensor/cam_sensor_mipi.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_s5kgn3.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_s5khp2.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_imx754.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_imx854.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_s5k3lu.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_imx564.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_s5k3k1.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_s5kjn3.c",
                    "drivers/cam_sensor_module/cam_sensor/adaptive_mipi/cam_sensor_adaptive_mipi_imx874.c",
                ],
            },
            "CONFIG_SAMSUNG_LPAI_OIS": {
                True: ["drivers/cam_sensor_module/cam_sensor_utils/cam_sysfs_lpai_ois.c"],
            },
        },
        copts = ["-include", "$(location :camera_banner)"],
        deps = deps,
        kconfig = "Kconfig",
        defconfig = "{}_defconfig_generated".format(tv),
        kernel_build = "//msm-kernel:{}".format(tv),
    )

    copy_to_dist_dir(
        name = "{}_camera_dist".format(tv),
        data = [":{}_camera".format(tv)],
        dist_dir = "out/target/product/{}/dlkm/lib/modules/".format(target),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
        mode_overrides = {"**/*": "644"},
    )

def define_camera_module():
    for (t, v) in get_all_variants():
        _define_module(t, v)
