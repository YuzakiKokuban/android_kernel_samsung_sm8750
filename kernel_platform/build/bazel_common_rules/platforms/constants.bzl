"""Constants related to Bazel platforms."""

# This dict denotes the suffixes for host platforms (keys) and the constraints
# associated with them (values). Used in transitions and tests, in addition to
# here.
host_platforms = {
    "linux_x86": [
        "@platforms//cpu:x86_32",
        "@platforms//os:linux",
    ],
    "linux_x86_64": [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    "linux_musl_x86": [
        "@platforms//cpu:x86_32",
        "@//build/bazel_common_rules/platforms/os:linux_musl",
    ],
    "linux_musl_x86_64": [
        "@platforms//cpu:x86_64",
        "@//build/bazel_common_rules/platforms/os:linux_musl",
    ],
    # linux_bionic is the OS for the Linux kernel plus the Bionic libc runtime,
    # but without the rest of Android.
    "linux_bionic_arm64": [
        "@platforms//cpu:arm64",
        "@//build/bazel_common_rules/platforms/os:linux_bionic",
    ],
    "linux_bionic_x86_64": [
        "@platforms//cpu:x86_64",
        "@//build/bazel_common_rules/platforms/os:linux_bionic",
    ],
    "darwin_arm64": [
        "@platforms//cpu:arm64",
        "@platforms//os:macos",
    ],
    "darwin_x86_64": [
        "@platforms//cpu:x86_64",
        "@platforms//os:macos",
    ],
    "windows_x86": [
        "@platforms//cpu:x86_32",
        "@platforms//os:windows",
    ],
    "windows_x86_64": [
        "@platforms//cpu:x86_64",
        "@platforms//os:windows",
    ],
}
