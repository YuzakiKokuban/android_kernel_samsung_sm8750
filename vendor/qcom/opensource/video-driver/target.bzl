load(":video_modules.bzl", "video_driver_modules")
load(":video_driver_build.bzl", "define_lunch_target_variant_modules")
load("//msm-kernel:target_variants.bzl", "get_all_la_variants")

def define_target_modules():
    for (target, variant) in get_all_la_variants():
        define_lunch_target_variant_modules(
            target = target,
            variant = variant,
            registry = video_driver_modules,
            modules = [
                "msm_video",
                "video",
            ],
        )
