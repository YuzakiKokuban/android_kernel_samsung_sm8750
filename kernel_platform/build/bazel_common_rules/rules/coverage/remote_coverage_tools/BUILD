# This is a stub BUILD to override remote_coverage_tools.
# See b/201242197 for more information.

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "coverage_report_generator",
    srcs = ["coverage_report_generator.sh"],
)

# TODO(b/201242197): vendor remote_coverage_tools.
#
# Necessary to keep cc_test's implicit dep lookup from complaining.
filegroup(
    name = "lcov_merger",
    srcs = ["stub.sh"],
)
