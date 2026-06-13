# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.

__block_platform_map =  {
    "sun": {
        "perf": [
            # keep sorted & in-tree modules only
            "block/blk-sec-common.ko",
            "block/blk-sec-stats.ko",
            "block/blk-sec-wb.ko",
            "block/ssg.ko",
        ],
        "consolidate": [
            # keep sorted
        ],
    },
}

def sec_block(target, variant):
    if not target in __block_platform_map:
        return []

    target_map = __block_platform_map[target]
    if not variant in target_map:
        return []

    if variant == "consolidate":
        return target_map[variant] + target_map["perf"]

    return target_map[variant]
