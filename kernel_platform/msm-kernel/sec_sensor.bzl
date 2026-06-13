# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.

__sensor_platform_map =  {
    "sun": {
        "perf": [
            # keep sorted & in-tree modules only
            "drivers/adsp_factory/adsp_factory_module.ko",
            "drivers/sensors/sensors_core.ko",
        ],
        "consolidate": [
            # keep sorted
        ],
    },
}

def sec_sensor(target, variant):
    if not target in __sensor_platform_map:
        return []

    target_map = __sensor_platform_map[target]
    if not variant in target_map:
        return []

    if variant == "consolidate":
        return target_map[variant] + target_map["perf"]

    return target_map[variant]
