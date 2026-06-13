load(":display_modules.bzl", "display_driver_modules")
load(":display_driver_build.bzl", "define_target_variant_modules")
load("//msm-kernel:target_variants.bzl", "get_all_la_variants")

def define_niobe():
    for (t, v) in get_all_la_variants():
        if t == "niobe":
            define_target_variant_modules(
                target = t,
                variant = v,
                registry = display_driver_modules,
                modules = [
                    "msm_drm",
                ],
                config_options = [
                    "CONFIG_DRM_MSM_SDE",
                    "CONFIG_SYNC_FILE",
                    "CONFIG_DRM_MSM_DSI",
                    "CONFIG_DRM_MSM_DP",
                    #"CONFIG_DRM_MSM_DP_MST",
                    "CONFIG_DSI_PARSER",
                    "CONFIG_DRM_SDE_RSC",
                    "CONFIG_DRM_SDE_WB",
                    "CONFIG_DRM_MSM_REGISTER_LOGGING",
                    "CONFIG_QCOM_MDSS_PLL",
                    "CONFIG_HDCP_QSEECOM",
                    "CONFIG_QCOM_FSA4480_I2C",
                    "CONFIG_DRM_SDE_SYSTEM_SLEEP_DISABLE",
                    "CONFIG_QCOM_SPEC_SYNC",
                    "CONFIG_THERMAL_OF",
                    "CONFIG_DRM_SDE_MINIDUMP_DISABLE",
                    "CONFIG_MSM_EXT_DISPLAY",
                    "CONFIG_DRM_SDE_DEV_COREDUMP_DISABLE"
                ],
            )
