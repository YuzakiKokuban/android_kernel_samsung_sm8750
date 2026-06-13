load(":dist.bzl", "copy_to_dist_dir")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

exports_files(["dist.py"])

bzl_library(
    name = "dist",
    srcs = ["dist.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "//build/bazel_common_rules/exec:embedded_exec",
        "@bazel_skylib//rules:copy_file",
    ],
)

# bazel run --package_path=out/soong/workspace //build/bazel_common_rules/dist:dist_bionic_example -- --dist_dir=/tmp/dist
copy_to_dist_dir(
    name = "dist_bionic_example",
    data = [
        "//bionic/libc",
        "//bionic/libdl",
    ],
)

# For testing that `testonly` deps can be dist'd.
genrule(
    name = "testonly_sample",
    testonly = True,
    outs = ["testonly.txt"],
    cmd = "touch $@",
)

copy_to_dist_dir(
    name = "dist_testonly_sample",
    testonly = True,
    data = [":testonly_sample"],
)
