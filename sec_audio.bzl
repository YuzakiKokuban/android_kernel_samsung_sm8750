# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.

__audio_platform_map =  {
    "sun": {
        "perf": [
            # keep sorted & in-tree modules only
            "snd_debug_proc.ko",
            "sec_audio_sysfs.ko",
        ],
        "consolidate": [
            # keep sorted
        ],
    },
}

def sec_audio(target, variant):
    if not target in __audio_platform_map:
        return []

    target_map = __audio_platform_map[target]
    if not variant in target_map:
        return []

    if variant == "consolidate":
        return target_map[variant] + target_map["perf"]

    return target_map[variant]
