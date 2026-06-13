load(":eva_modules.bzl", "eva_modules")
load(":eva_module_build.bzl", "define_consolidate_gki_perf_modules")

def define_tuna():
    define_consolidate_gki_perf_modules(
        target = "tuna",
        registry = eva_modules,
        modules = [
            "msm-eva",
        ],
        config_options = [
            #"CONFIG_TARGET_SYNX_ENABLE",
            "TARGET_SYNX_ENABLE",
            "TARGET_DSP_ENABLE",
            "TARGET_MMRM_ENABLE",
            "CONFIG_EVA_SUN",
            #"TARGET_PRESIL_ENABLE",
            "CONFIG_MSM_MMRM"
        ],
    )
