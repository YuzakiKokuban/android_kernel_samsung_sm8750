# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.

__ipc_platform_map =  {
    "sun": {
        "perf": [
            "drivers/samsung/ipc/sec_ipc_tiantong.ko",
        ],
        "consolidate": [
            # keep sorted & in-tree modules only
        ],
    },
}

def sec_ipc(target, variant):

    if not target in __ipc_platform_map:
        return []

    target_map = __ipc_platform_map[target]
    if not variant in target_map:
        return []

    if variant == "consolidate":
        return target_map[variant] + target_map["perf"]

    return target_map[variant]