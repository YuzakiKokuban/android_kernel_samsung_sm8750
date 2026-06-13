load("//build/kernel/kleaf:kernel.bzl", "ddk_module", "ddk_submodule")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")

def _register_module_to_map(module_map, name, path, config_option, srcs, config_srcs, deps, config_deps):
    processed_config_srcs = {}
    nested_config = {}
    processed_config_deps = {}

    for config_src_name in config_srcs:
        config_src = config_srcs[config_src_name]

        if type(config_src) == "list":
            processed_config_srcs[config_src_name] = {True: config_src}
        else:
            processed_config_srcs[config_src_name] = config_src
        if type(config_src) == "dict":
            nested_config = config_src

            for nested_src, nest_name in nested_config.items():
                if nested_src == "True":
                    for nest_src in nest_name:
                        final_srcs = nest_name[nest_src]
                        processed_config_srcs[nest_src] = final_srcs

    for config_deps_name in config_deps:
        config_dep = config_deps[config_deps_name]
        if type(config_dep) == "list":
            processed_config_deps[config_deps_name] = {True: config_dep}
        else:
            processed_config_deps[config_deps_name] = config_dep

    module = struct(
        name = name,
        path = path,
        srcs = srcs,
        config_srcs = processed_config_srcs,
        config_option = config_option,
        deps = deps,
        config_deps = processed_config_deps,
    )

    module_map[name] = module

def _get_config_choices(map, options):
    choices = []
    for option in map:
        choices.extend(map[option].get(option in options,[]))
    return choices

def _get_kernel_build_options(modules, config_options):
    all_options = {option: True for option in config_options}
    all_options = all_options | {module.config_option: True for module in modules if module.config_option}
    return all_options

def _get_kernel_build_module_srcs(module, options, formatter):
    srcs = module.srcs + _get_config_choices(module.config_srcs, options)
    module_path = "{}/".format(module.path) if module.path else ""
    return ["{}{}".format(module_path, formatter(src)) for src in srcs]

def _get_kernel_build_module_deps(module, options, formatter):
    deps = module.deps + _get_config_choices(module.config_deps, options)
    deps = [formatter(dep) for dep in deps]
    return deps

def display_module_entry(hdrs = []):
    module_map = {}

    def register(name, path = None, config_option = None, srcs = [], config_srcs = {}, deps = [], config_deps = {}):
        _register_module_to_map(module_map, name, path, config_option, srcs, config_srcs, deps, config_deps)
    return struct(
        register = register,
        get = module_map.get,
        hdrs = hdrs,
        module_map = module_map
    )

def define_target_variant_modules(target, variant, registry, modules, config_options = []):
    kernel_build = "{}_{}".format(target, variant)
    kernel_build_label = "//msm-kernel:{}".format(kernel_build)
    modules = [registry.get(module_name) for module_name in modules]
    options = _get_kernel_build_options(modules, config_options)
    build_print = lambda message : print("{}: {}".format(kernel_build, message))
    formatter = lambda s : s.replace("%b", kernel_build).replace("%t", target)
    headers = ["//msm-kernel:all_headers"] + registry.hdrs
    all_module_rules = []

    for module in modules:
        rule_name = "{}_{}".format(kernel_build, module.name)
        module_srcs = _get_kernel_build_module_srcs(module, options, formatter)
        print(rule_name)
        if not module_srcs:
            continue

        ddk_submodule(
            name = rule_name,
            srcs = module_srcs,
            out = "{}.ko".format(module.name),

# CONFIG_DISPLAY_SAMSUNG start
            conditional_srcs =  {
                "CONFIG_PANEL_PBA_FHD_DSI1": {
                    True: [
                        "msm/samsung/PBA_BOOTING_DSI1/ss_dsi_panel_PBA_BOOTING_fhd_dsi1.c",
                    ]
                },
                "CONFIG_PANEL_PA1_S6E3FAE_AMB616FL03_FHD": {
                    True: [
                        "msm/samsung/PA1_S6E3FAE_AMB616FL03/PA1_S6E3FAE_AMB616FL03_panel.c",
                    ]
                },
                "CONFIG_PANEL_PA1_S6E3FAE_AMB616FL03_VHM_FHD": {
                    True: [
                        "msm/samsung/PA1_S6E3FAE_AMB616FL03_VHM/PA1_S6E3FAE_AMB616FL03_VHM_panel.c",
                    ]
                },
                "CONFIG_PANEL_PA2_S6E3HAF_AMB666FM03_WQHD": {
                    True: [
                        "msm/samsung/PA2_S6E3HAF_AMB666FM03/PA2_S6E3HAF_AMB666FM03_panel.c",
                    ]
                },
                "CONFIG_PANEL_PA2_S6E3HAF_AMB666FM03_VHM_WQHD": {
                    True: [
                        "msm/samsung/PA2_S6E3HAF_AMB666FM03_VHM/PA2_S6E3HAF_AMB666FM03_VHM_panel.c",
                    ]
                },
                "CONFIG_PANEL_PA2_S6E3HAF_AMB666FM01_WQHD": {
                    True: [
                        "msm/samsung/PA2_S6E3HAF_AMB666FM01/PA2_S6E3HAF_AMB666FM01_panel.c",
                    ]
                },
                "CONFIG_PANEL_PA3_S6E3HAF_AMB679FN01_WQHD": {
                    True: [
                        "msm/samsung/PA3_S6E3HAF_AMB679FN01/PA3_S6E3HAF_AMB679FN01_panel.c",
                    ]
                },
                "CONFIG_PANEL_PA3_S6E3HAF_AMB686HX01_WQHD": {
                    True: [
                        "msm/samsung/PA3_S6E3HAF_AMB686HX01/PA3_S6E3HAF_AMB686HX01_panel.c",
                    ]
                },
                "CONFIG_PANEL_PA3_S6E3HAF_AMB686HX01_VHM_WQHD": {
                    True: [
                        "msm/samsung/PA3_S6E3HAF_AMB686HX01_VHM/PA3_S6E3HAF_AMB686HX01_VHM_panel.c",
                    ]
                },
                "CONFIG_PANEL_Q7M_ANA38407_AMSA10FA01_WQXGA": {
                    True: [
                        "msm/samsung/Q7M_ANA38407_AMSA10FA01/Q7M_ANA38407_AMSA10FA01_panel.c",
                        "msm/samsung/PMIC/ss_boost_max77816_i2c.c",
                    ]
                },
                "CONFIG_PANEL_Q7M_S6E3FAE_AMB649GY01_HD": {
                    True: [
                        "msm/samsung/Q7M_S6E3FAE_AMB649GY01/Q7M_S6E3FAE_AMB649GY01_panel.c",
                    ]
                },
                "CONFIG_PANEL_Q7_S6E3XA5_AMF800GX01_QXGA": {
                    True: [
                        "msm/samsung/Q7_S6E3XA5_AMF800GX01/Q7_S6E3XA5_AMF800GX01_panel.c",
                    ]
                },
                "CONFIG_PANEL_Q7_S6E3FAE_AMB649GY01_HD": {
                    True: [
                        "msm/samsung/Q7_S6E3FAE_AMB649GY01/Q7_S6E3FAE_AMB649GY01_panel.c",
                    ]
                },
                "CONFIG_PANEL_PS_S6E3HAF_AMB666LH01_WQHD": {
                    True: [
                        "msm/samsung/PS_S6E3HAF_AMB666LH01/PS_S6E3HAF_AMB666LH01_panel.c",
                    ]
                },
                "CONFIG_PANEL_PS_S6E3HAF_AMB666LH01_VHM_WQHD": {
                    True: [
                        "msm/samsung/PS_S6E3HAF_AMB666LH01_VHM/PS_S6E3HAF_AMB666LH01_VHM_panel.c",
                    ]
                },
                "CONFIG_DISPLAY_SAMSUNG": {
                    True: [
                        "msm/samsung/PBA_BOOTING/ss_dsi_panel_PBA_BOOTING_fhd.c",
                        "msm/samsung/ss_dsi_panel_sysfs.c",
                        "msm/samsung/ss_dsi_panel_debug.c",
                        "msm/samsung/ss_dsi_panel_common.c",
                        "msm/samsung/ss_dsi_mdnie_lite_common.c",
                        "msm/samsung/ss_dpui_common.c",
                        "msm/samsung/ss_copr_common.c",
                        "msm/samsung/ss_wrapper_common.c",
                        "msm/samsung/ss_panel_parse.c",
                        "msm/samsung/ss_panel_power.c",
                        "msm/samsung/SELF_DISPLAY/self_display.c",
                        "msm/samsung/MAFPC/ss_dsi_mafpc.c",
                    ]
                },
                "CONFIG_SECDP" : {
                    True: [
                         "msm/dp/secdp_sysfs.c",
                         "msm/dp/secdp_unit_test.c",
                    ],
                },
                "CONFIG_SECDP_LOGGER" : {
                    True: [
                         "msm/dp/secdp_logger.c",
                    ],
                },
                "CONFIG_SECDP_BIGDATA" : {
                    True: [
                         "msm/dp/secdp_bigdata.c",
                    ],
                },
            },
# CONFIG_DISPLAY_SAMSUNG end

            deps = headers + _get_kernel_build_module_deps(module, options, formatter),
            local_defines = options.keys(),
        )
        all_module_rules.append(rule_name)

    ddk_module(
        name = "{}_display_drivers".format(kernel_build),
        kernel_build = kernel_build_label,
# CONFIG_DISPLAY_SAMSUNG start
        kconfig = "sec_display_Kconfig",
        defconfig = "sec_display_defconfig",
# CONFIG_DISPLAY_SAMSUNG end
        deps = all_module_rules,
    )
    copy_to_dist_dir(
        name = "{}_display_drivers_dist".format(kernel_build),
        data = [":{}_display_drivers".format(kernel_build)],
        dist_dir = "out/target/product/{}/dlkm/lib/modules/".format(target),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
        mode_overrides = {"**/*": "644"},
        log = "info",
    )
