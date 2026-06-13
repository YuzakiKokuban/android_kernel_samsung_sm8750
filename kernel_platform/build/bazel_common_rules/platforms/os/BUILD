# Standard constraint_setting and constraint_values to be used in platforms.

load("@bazel_skylib//lib:selects.bzl", "selects")

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

alias(
    name = "android",
    actual = "@platforms//os:android",
    deprecation = "Use @platforms//os:android directly.",
)

config_setting(
    name = "android_config_setting",
    constraint_values = [
        ":android",
    ],
)

# Alias to the local_jdk's toolchain constraint to make local_jdk resolve
# correctly with --tool_java_runtime_version=local_jdk and the checked-in JDK.
alias(
    name = "linux",
    actual = "@platforms//os:linux",
    deprecation = "Use @platforms//os:linux directly.",
)

alias(
    name = "linux_glibc",
    actual = "@platforms//os:linux",
    deprecation = "Use @platforms//os:linux directly.",
)

constraint_value(
    name = "linux_musl",
    constraint_setting = "@platforms//os:os",
)

constraint_value(
    name = "linux_bionic",
    constraint_setting = "@platforms//os:os",
)

config_setting(
    name = "linux_bionic_config_setting",
    constraint_values = [
        ":linux_bionic",
    ],
)

alias(
    name = "windows",
    actual = "@platforms//os:windows",
    deprecation = "Use @platforms//os:windows directly.",
)

alias(
    name = "darwin",
    actual = "@platforms//os:macos",
    deprecation = "Use @platforms//os:macos directly.",
)

selects.config_setting_group(
    name = "bionic",
    match_any = [
        ":android_config_setting",
        ":linux_bionic_config_setting",
    ],
)
