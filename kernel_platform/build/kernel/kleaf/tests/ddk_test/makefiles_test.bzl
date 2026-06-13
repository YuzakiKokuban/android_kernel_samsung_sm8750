# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for ddk/makefiles.bzl."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//lib:sets.bzl", "sets")
load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("@bazel_skylib//rules:build_test.bzl", "build_test")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("//build/kernel/kleaf/impl:common_providers.bzl", "ModuleSymversInfo")
load("//build/kernel/kleaf/impl:ddk/ddk_conditional_filegroup.bzl", "ddk_conditional_filegroup")
load("//build/kernel/kleaf/impl:ddk/ddk_headers.bzl", "ddk_headers")
load("//build/kernel/kleaf/impl:ddk/ddk_module.bzl", "ddk_module")
load("//build/kernel/kleaf/impl:ddk/makefiles.bzl", "makefiles")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")
load("//build/kernel/kleaf/impl:kernel_build.bzl", "kernel_build")
load("//build/kernel/kleaf/tests:failure_test.bzl", "failure_test")
load("//build/kernel/kleaf/tests:test_utils.bzl", "test_utils")
load("//build/kernel/kleaf/tests/utils:contain_lines_test.bzl", "contain_lines_test")

def _argv_to_dict(argv):
    """A naive algorithm that transforms argv to a dictionary.

    E.g.:

    ```
    _argv_to_dict(["--foo", "bar", "baz", "--qux", "quux"])
    ```

    produces

    ```
    {
        "--foo": ["bar", "baz"],
        "--qux": ["quux"]
    }
    ```
    """

    ret = dict()
    key = None

    for item in argv:
        if item.startswith("-"):
            key = item
            if key not in ret:
                ret[key] = []
        else:
            ret[key].append(item)

    return ret

def _makefiles_test_impl(ctx):
    env = analysistest.begin(ctx)
    action = test_utils.find_action(env, "DdkMakefiles")

    argv_dict = _argv_to_dict(action.argv[1:])

    actual_module_out = argv_dict.get("--kernel-module-out")
    if actual_module_out:
        asserts.equals(env, 1, len(actual_module_out), "more than 1 --kernel-module-out")
        actual_module_out = actual_module_out[0]

    asserts.equals(
        env,
        ctx.attr.expected_module_out or None,
        actual_module_out,
        "--kernel-module-out mismatch",
    )

    expected_module_symvers = []
    for dep in ctx.attr.expected_deps:
        if ModuleSymversInfo in dep:
            expected_module_symvers += dep[ModuleSymversInfo].restore_paths.to_list()
    asserts.set_equals(
        env,
        sets.make(expected_module_symvers),
        sets.make(argv_dict.get("--module-symvers-list", [])),
        "--module-symvers-list mismatch",
    )

    # Check content + ordering of include dirs, so do list comparison.
    asserts.equals(
        env,
        ctx.attr.expected_includes,
        argv_dict.get("--include-dirs", []),
        "--include-dirs mismatch",
    )

    return analysistest.end(env)

_makefiles_test = analysistest.make(
    impl = _makefiles_test_impl,
    attrs = {
        "expected_module_srcs": attr.label_list(allow_files = True),
        "expected_module_out": attr.string(),
        "expected_includes": attr.string_list(),
        "expected_deps": attr.label_list(),
    },
)

def _makefiles_test_make(
        name,
        expected_includes = None,
        **kwargs):
    makefiles(
        name = name + "_makefiles",
        tags = ["manual"],
        **kwargs
    )

    _makefiles_test(
        name = name,
        target_under_test = name + "_makefiles",
        expected_module_srcs = kwargs.get("module_srcs"),
        expected_module_out = kwargs.get("module_out"),
        expected_includes = expected_includes,
        expected_deps = kwargs.get("module_deps"),
    )

def _bad_test_make(
        name,
        error_message,
        **kwargs):
    makefiles(
        name = name + "_makefiles",
        tags = ["manual"],
        **kwargs
    )
    failure_test(
        name = name,
        target_under_test = name + "_makefiles",
        error_message_substrs = [error_message],
    )

def _get_top_level_file_impl(ctx):
    out = ctx.actions.declare_file(paths.join(ctx.attr.name, ctx.attr.filename))
    src = paths.join(ctx.file.target.path, ctx.attr.filename)
    hermetic_tools = hermetic_toolchain.get(ctx)
    command = hermetic_tools.setup + """
        if [[ -f {src} ]]; then
            cp -pL {src} {out}
        else
            : > {out}
        fi
    """.format(
        src = src,
        out = out.path,
    )
    ctx.actions.run_shell(
        outputs = [out],
        inputs = [ctx.file.target],
        tools = hermetic_tools.deps,
        command = command,
    )
    return DefaultInfo(
        files = depset([out]),
        runfiles = ctx.runfiles(files = [out]),
    )

get_top_level_file = rule(
    implementation = _get_top_level_file_impl,
    doc = "Gets the top level file from a `makefiles` rule.",
    attrs = {
        "target": attr.label(allow_single_file = True),
        "filename": attr.string(),
    },
    toolchains = [hermetic_toolchain.type],
)

def _create_makefiles_artifact_test(
        name,
        out = None,
        srcs = None,
        local_defines = None,
        copts = None,
        linux_includes = None,
        includes = None,
        deps = None,
        hdrs = None,
        top_level_makefile = None,
        cflags_file_name = None,
        expected_lines = None,
        expected_makefile_lines = None,
        expected_cflags_lines = None):
    """Creates a test on the `Kbuild` file generated by `makefiles`."""

    makefiles(
        name = name + "_module_makefiles",
        module_out = out,
        module_srcs = srcs,
        module_local_defines = local_defines,
        module_copts = copts,
        module_linux_includes = linux_includes,
        module_includes = includes,
        module_hdrs = hdrs,
        module_deps = deps,
        top_level_makefile = top_level_makefile,
        tags = ["manual"],
    )

    tests = []

    write_file(
        name = name + "_expected",
        out = name + "_expected/Kbuild",
        content = expected_lines,
    )

    get_top_level_file(
        name = name + "_kbuild",
        filename = "Kbuild",
        target = name + "_module_makefiles",
        tags = ["manual"],
    )

    contain_lines_test(
        name = name + "_kbuild_test",
        expected = name + "_expected",
        actual = name + "_kbuild",
        order = True,
    )
    tests.append(name + "_kbuild_test")

    write_file(
        name = name + "_expected_makefile",
        out = name + "_expected_makefile/Makefile",
        content = expected_makefile_lines,
    )

    get_top_level_file(
        name = name + "_makefile",
        filename = "Makefile",
        target = name + "_module_makefiles",
        tags = ["manual"],
    )

    contain_lines_test(
        name = name + "_makefile_test",
        expected = name + "_expected_makefile",
        actual = name + "_makefile",
        order = True,
    )
    tests.append(name + "_makefile_test")

    # License header tests.
    write_file(
        name = name + "_kbuild_license_header",
        out = name + "_expected_license/Kbuild",
        content = ["# SPDX-License-Identifier: GPL-2.0"],
    )
    contain_lines_test(
        name = name + "_kbuild_license_test",
        expected = name + "_kbuild_license_header",
        actual = name + "_kbuild",
    )
    tests.append(name + "_kbuild_license_test")
    if top_level_makefile:
        write_file(
            name = name + "_makefile_license_header",
            out = name + "_expected_license/Makefile",
            content = ["# SPDX-License-Identifier: GPL-2.0"],
        )
        contain_lines_test(
            name = name + "_makefile_license_test",
            expected = name + "_makefile_license_header",
            actual = name + "_makefile",
        )
        tests.append(name + "_makefile_license_test")

    if expected_cflags_lines:
        # Assume no submodules. For submodules, out == None
        cflags_file_name = out.removesuffix(".ko") + ".cflags"
        write_file(
            name = name + "_expected_cflags",
            out = name + "_expected_cflags/{}".format(cflags_file_name),
            content = expected_cflags_lines,
        )

        get_top_level_file(
            name = name + "_cflags",
            filename = cflags_file_name,
            target = name + "_module_makefiles",
        )

        contain_lines_test(
            name = name + "_cflags_test",
            expected = name + "_expected_cflags",
            actual = name + "_cflags",
            order = True,
        )
        tests.append(name + "_cflags_test")

    native.test_suite(
        name = name,
        tests = tests,
    )

def _makefiles_subdir_test(name):
    """Define build tests for `makefiles`"""
    tests = []

    _makefile_top_module_uses_subdir_source_test(name = name + "_top_module_uses_subdir_source")
    tests.append(name + "_top_module_uses_subdir_source")

    _makefile_subdir_source_same_name_test(name = name + "_subdir_sources_same_name")
    tests.append(name + "_subdir_sources_same_name")

    _makefile_subdir_source_different_name_test(name = name + "_subdir_sources_different_name")
    tests.append(name + "_subdir_sources_different_name")

    _makefile_subdir_module_uses_top_source_test(name = name + "_subdir_module_uses_top_source")
    tests.append(name + "_subdir_module_uses_top_source")

    _makefile_source_as_module_name_and_other_sources_test(name = name + "_source_as_module_name_and_other_sources_test")
    tests.append(name + "_source_as_module_name_and_other_sources_test")

    native.test_suite(
        name = name,
        tests = tests,
    )

def _makefile_top_module_uses_subdir_source_test(name):
    """Tests makefiles when bar.ko is from subdir/foo.c.

    Args:
        name: name of the test.
    """
    _create_makefiles_artifact_test(
        name = name,
        out = "bar.ko",
        srcs = ["subdir/foo.c"],
        expected_lines = ["bar-y += subdir/foo.o"],
    )

def _makefile_subdir_source_same_name_test(name):
    """Tests makefiles when subdir/foo.ko is from subdir/foo.c.

    Args:
        name: name of the test.
    """

    tests = []

    makefiles(
        name = name + "_module_makefiles",
        module_out = "subdir/foo.ko",
        module_srcs = ["subdir/foo.c"],
        tags = ["manual"],
    )

    get_top_level_file(
        name = name + "_subdir_kbuild",
        filename = "subdir/Kbuild",
        target = name + "_module_makefiles",
    )
    write_file(
        name = name + "_expected_subdir_kbuild",
        out = name + "_exepcted/subdir/Kbuild",
        content = ["# The module subdir/foo.ko has a source file subdir/foo.c"],
    )
    contain_lines_test(
        name = name + "_subdir_kbuild_test",
        expected = name + "_expected_subdir_kbuild",
        actual = name + "_subdir_kbuild",
    )
    tests.append(name + "_subdir_kbuild_test")

    get_top_level_file(
        name = name + "_kbuild",
        filename = "Kbuild",
        target = name + "_module_makefiles",
    )
    write_file(
        name = name + "_expected_kbuild",
        out = name + "_expected/Kbuild",
        content = ["obj-y += subdir/"],
    )
    contain_lines_test(
        name = name + "_kbuild_test",
        expected = name + "_expected_kbuild",
        actual = name + "_kbuild",
    )
    tests.append(name + "_kbuild_test")

    native.test_suite(
        name = name,
        tests = tests,
    )

def _makefile_subdir_source_different_name_test(name):
    """Tests makefiles when subdir/bar.ko is from subdir/foo.c.

    Args:
        name: name of the test.
    """
    tests = []

    makefiles(
        name = name + "_module_makefiles",
        module_out = "subdir/bar.ko",
        module_srcs = ["subdir/foo.c"],
        tags = ["manual"],
    )

    get_top_level_file(
        name = name + "_subdir_kbuild",
        filename = "subdir/Kbuild",
        target = name + "_module_makefiles",
    )
    write_file(
        name = name + "_expected_subdir_kbuild",
        out = name + "_exepcted/subdir/Kbuild",
        content = ["bar-y += foo.o"],
    )
    contain_lines_test(
        name = name + "_subdir_kbuild_test",
        expected = name + "_expected_subdir_kbuild",
        actual = name + "_subdir_kbuild",
    )
    tests.append(name + "_subdir_kbuild_test")

    get_top_level_file(
        name = name + "_kbuild",
        filename = "Kbuild",
        target = name + "_module_makefiles",
    )
    write_file(
        name = name + "_expected_kbuild",
        out = name + "_expected/Kbuild",
        content = ["obj-y += subdir/"],
    )
    contain_lines_test(
        name = name + "_kbuild_test",
        expected = name + "_expected_kbuild",
        actual = name + "_kbuild",
    )
    tests.append(name + "_kbuild_test")

    native.test_suite(
        name = name,
        tests = tests,
    )

def _makefile_subdir_module_uses_top_source_test(name):
    """Tests makefiles when subdir/bar.ko uses foo.c. This should fail.

    Args:
        name: name of the test.
    """
    makefiles(
        name = name + "_module_makefiles",
        module_out = "subdir/bar.ko",
        module_srcs = ["foo.c"],
        internal_target_fail_message =
            "foo.c is not a valid source because it is not under subdir",
        tags = ["manual"],
    )
    build_test(
        name = name,
        targets = [name + "_module_makefiles"],
    )

def _makefile_source_as_module_name_and_other_sources_test(name):
    """Tests that, to build foo.ko, if foo.c exists, it must be the only source file.

    Args:
        name: name of the test
    """

    makefiles(
        name = name + "_module_makefiles",
        module_out = "foo.ko",
        module_srcs = ["foo.c", "bar.c"],
        internal_target_fail_message =
            "Source files ['foo.c'] are not allowed to build foo.ko when multiple " +
            "source files exist. Please change the name of the output file.",
        tags = ["manual"],
    )
    build_test(
        name = name,
        targets = [name + "_module_makefiles"],
    )

def _makefiles_include_ordering_artifacts_test(name):
    """Defines tests on include ordering by actually examining the generated Kbuild file."""

    tests = []

    ddk_headers(
        name = name + "_dep_a_headers",
        includes = ["include/dep_a"],
        linux_includes = ["linux_include/dep_a"],
        hdrs = ["self.h"],  # suppress b/256248232
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_dep_b_headers",
        includes = ["include/dep_b"],
        linux_includes = ["linux_include/dep_b"],
        hdrs = ["self.h"],  # suppress b/256248232
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_dep_c_headers",
        includes = ["include/dep_c"],
        linux_includes = ["linux_include/dep_c"],
        hdrs = [name + "_dep_a_headers"],
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_hdrs_a_headers",
        includes = ["include/hdrs_a"],
        linux_includes = ["linux_include/hdrs_a"],
        hdrs = ["self.h"],  # suppress b/256248232
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_hdrs_b_headers",
        includes = ["include/hdrs_b"],
        linux_includes = ["linux_include/hdrs_b"],
        hdrs = ["self.h"],  # suppress b/256248232
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_hdrs_c_headers",
        includes = ["include/hdrs_c"],
        linux_includes = ["linux_include/hdrs_c"],
        hdrs = [name + "_hdrs_a_headers"],
        tags = ["manual"],
    )

    prefix = "$(ROOT_DIR)/{}".format(
        native.package_name(),
    )

    _create_makefiles_artifact_test(
        name = name + "_include_ordering",
        srcs = ["base.c"],
        out = name + "_base.ko",
        includes = [
            # do not sort
            "local_include/B",
            "local_include/A",
            "local_include/C",
        ],
        linux_includes = [
            # do not sort
            "local_linux_include/B",
            "local_linux_include/A",
            "local_linux_include/C",
        ],
        deps = [
            # do not sort
            name + "_dep_c_headers",
            name + "_dep_b_headers",
            name + "_dep_a_headers",
        ],
        hdrs = [
            # do not sort
            name + "_hdrs_c_headers",
            name + "_hdrs_b_headers",
            name + "_hdrs_a_headers",
        ],
        expected_lines = [
            # do not sort
            # LINUXINCLUDE
            "LINUXINCLUDE := \\",
            # local "linux_includes"
            "-I{}/local_linux_include/B \\".format(prefix),
            "-I{}/local_linux_include/A \\".format(prefix),
            "-I{}/local_linux_include/C \\".format(prefix),
            # linux_includes of deps
            "-I{}/linux_include/dep_c \\".format(prefix),
            "-I{}/linux_include/dep_a \\".format(prefix),  # c includes a
            "-I{}/linux_include/dep_b \\".format(prefix),
            # linux_include/dep_a is already specified, so dropping
            # linux_includes of hdrs
            "-I{}/linux_include/hdrs_c \\".format(prefix),
            "-I{}/linux_include/hdrs_a \\".format(prefix),  # c includes a
            "-I{}/linux_include/hdrs_b \\".format(prefix),
            # linux_include/hdrs_a is already specified, so dropping
            "$(LINUXINCLUDE)",
            "CFLAGS_base.o += @{}/{}_base.cflags".format(prefix, name),
        ],
        expected_cflags_lines = [
            # local "includes"
            "-I{}/local_include/B".format(prefix),
            "-I{}/local_include/A".format(prefix),
            "-I{}/local_include/C".format(prefix),
            # deps, recursively
            "-I{}/include/dep_c".format(prefix),
            "-I{}/include/dep_a".format(prefix),  # c includes a
            "-I{}/include/dep_b".format(prefix),
            # dep_a is already specified, so dropping
            # hdrs, recursively
            "-I{}/include/hdrs_c".format(prefix),
            "-I{}/include/hdrs_a".format(prefix),  # c includes a
            "-I{}/include/hdrs_b".format(prefix),
            # hdrs_a is already specified, so dropping
        ],
    )
    tests.append(name + "_include_ordering")

    native.test_suite(
        name = name,
        tests = tests,
    )

def _makefiles_submodule_symvers_test(
        name,
        kernel_build):
    """Test on Module.symvers of deps of submodules.

    That is, if ddk_module A -> ddk_submodule B -> ddk_module C, then the Makefile
    of A contains Module.symvers from C.
    """

    ddk_module(
        name = name + "_C",
        kernel_build = kernel_build,
        out = name + "_C.ko",
        srcs = ["base.c"],
        tags = ["manual"],
    )

    makefiles(
        name = name + "_B",
        module_srcs = ["dep.c"],
        module_deps = [name + "_C"],
        module_out = name + "_B.ko",
        top_level_makefile = False,
        tags = ["manual"],
    )

    _create_makefiles_artifact_test(
        name = name,
        deps = [name + "_B"],
        top_level_makefile = True,
        expected_makefile_lines = [
            "EXTRA_SYMBOLS += $(COMMON_OUT_DIR)/{}/{}_C_Module.symvers".format(
                native.package_name(),
                name,
            ),
        ],
    )

def _makefiles_cond_srcs_test(name):
    """Test on makefiles depending on ddk_conditional_filegroup."""

    ddk_conditional_filegroup(
        name = name + "_a_y_srcs",
        config = "CONFIG_A",
        value = "y",
        srcs = ["cond_srcs/a_y.c"],
    )
    ddk_conditional_filegroup(
        name = name + "_a_n_srcs",
        config = "CONFIG_A",
        value = "",
        srcs = ["cond_srcs/a_n.c"],
    )

    tests = []
    _create_makefiles_artifact_test(
        name = name + "_simple",
        srcs = [
            name + "_a_y_srcs",
            name + "_a_n_srcs",
        ],
        out = name + "_simple.ko",
        expected_lines = [
            "ifeq ($(CONFIG_A),y)",
            "{}_simple-y += cond_srcs/a_y.o".format(name),
            "endif # ifeq ($(CONFIG_A),y)",
            "ifeq ($(CONFIG_A),)",
            "{}_simple-y += cond_srcs/a_n.o".format(name),
            "endif # ifeq ($(CONFIG_A),)",
        ],
    )
    tests.append(name + "_simple")

    native.test_suite(
        name = name,
        tests = tests,
    )

def makefiles_test_suite(name):
    """Defines tests for `makefiles`.

    Args:
        name: name of the test suite
    """
    tests = []

    _makefiles_test_make(
        name = name + "_simple",
        module_srcs = ["dep.c"],
        module_out = "dep.ko",
    )
    tests.append(name + "_simple")

    _makefiles_test_make(
        name = name + "_multiple_sources",
        module_srcs = ["self.c", "dep.c"],
        module_out = "foo.ko",
    )
    tests.append(name + "_multiple_sources")

    ddk_headers(
        name = name + "_self_headers",
        hdrs = ["self.h"],
        includes = ["."],
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_include_headers",
        hdrs = ["include/subdir.h"],
        includes = ["include"],
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_base_headers",
        hdrs = ["include/base/base.h"],
        includes = ["include/base"],
        tags = ["manual"],
    )

    ddk_headers(
        name = name + "_foo_headers",
        hdrs = ["foo.h"],
        includes = ["include/foo"],
        tags = ["manual"],
    )

    _makefiles_test_make(
        name = name + "_dep_on_headers",
        module_srcs = ["dep.c"],
        module_out = "dep.ko",
        module_deps = [name + "_self_headers"],
        expected_includes = [native.package_name()],
    )
    tests.append(name + "_dep_on_headers")

    native.filegroup(
        name = name + "_empty_filegroup",
        srcs = [],
        tags = ["manual"],
    )
    _bad_test_make(
        name = name + "_bad_dep",
        error_message = "is not a valid item in deps",
        module_srcs = ["dep.c"],
        module_out = "dep.ko",
        module_deps = [name + "_empty_filegroup"],
    )
    tests.append(name + "_bad_dep")

    _makefiles_test_make(
        name = name + "_export_other_headers",
        module_srcs = ["dep.c"],
        module_out = "dep.ko",
        module_hdrs = [name + "_self_headers"],
        expected_includes = [native.package_name()],
    )
    tests.append(name + "_export_other_headers")

    _makefiles_test_make(
        name = name + "_export_local_headers",
        module_srcs = ["dep.c"],
        module_out = "dep.ko",
        module_hdrs = ["self.h"],
        module_includes = ["."],
        expected_includes = [native.package_name()],
    )
    tests.append(name + "_export_local_headers")

    _makefiles_subdir_test(name = name + "_subdir_test")
    tests.append(name + "_subdir_test")

    _bad_test_make(
        name = name + "_bad_copt_location_not_one_token",
        error_message = "An $(location) expression must be its own item",
        module_srcs = ["dep.h"],
        module_out = "dep.ko",
        module_copts = ["-include $(location dep.h)"],
    )
    tests.append(name + "_bad_copt_location_not_one_token")

    _bad_test_make(
        name = name + "_bad_copt_location_not_its_own_token",
        error_message = "An $(location) expression must be its own item",
        module_srcs = ["dep.h"],
        module_out = "dep.ko",
        module_copts = ["-include=$(location dep.h)"],
    )
    tests.append(name + "_bad_copt_location_not_its_own_token")

    _bad_test_make(
        name = name + "_bad_copt_multiple_location_in_one_token",
        error_message = "An $(location) expression must be its own item",
        module_srcs = ["dep.h"],
        module_out = "dep.ko",
        module_copts = ["$(location dep.h) $(location dep.h)"],
    )
    tests.append(name + "_bad_copt_multiple_location_in_one_token")

    _makefiles_test_make(
        name = name + "_include_ordering",
        module_srcs = ["dep.c"],
        module_out = "dep.ko",
        module_includes = [
            # do not sort
            "include/transitive",
            "subdir",
        ],
        module_deps = [
            # do not sort
            name + "_self_headers",
            name + "_include_headers",
        ],
        module_hdrs = [
            # do not sort
            name + "_foo_headers",
            name + "_base_headers",
        ],
        expected_includes = [
            # do not sort
            # First, includes
            "{}/include/transitive".format(native.package_name()),
            "{}/subdir".format(native.package_name()),
            # Then, deps
            native.package_name(),
            "{}/include".format(native.package_name()),
            # Then, hdrs
            "{}/include/foo".format(native.package_name()),
            "{}/include/base".format(native.package_name()),
        ],
    )
    tests.append(name + "_include_ordering")

    # Test that to include hdrs before deps, one must duplicate the hdrs targets in deps
    _makefiles_test_make(
        name = name + "_include_hdrs_before_deps",
        module_out = name + "_include_hdrs_before_deps.ko",
        module_deps = [
            # do not sort
            name + "_include_headers",
            name + "_self_headers",
        ],
        module_hdrs = [
            name + "_base_headers",
            name + "_include_headers",
        ],
        expected_includes = [
            # do not sort
            # deps
            "{}/include".format(native.package_name()),
            native.package_name(),
            # hdrs
            "{}/include/base".format(native.package_name()),
            # skip _include_headers
        ],
    )
    tests.append(name + "_include_hdrs_before_deps")

    kernel_build(
        name = name + "_kernel_build",
        build_config = "build.config.fake",
        outs = [],
        tags = ["manual"],
    )
    ddk_module(
        name = name + "_parent_include_hdrs_before_deps",
        out = name + "_parent_include_hdrs_before_deps.ko",
        deps = [
            # do not sort
            name + "_include_headers",
            name + "_self_headers",
        ],
        hdrs = [
            name + "_base_headers",
            name + "_include_headers",
        ],
        kernel_build = name + "_kernel_build",
        srcs = [],
        tags = ["manual"],
    )

    # Children of _include_hdrs_before_deps still gets
    _makefiles_test_make(
        name = name + "_child_include_hdrs_before_deps",
        top_level_makefile = True,
        module_out = name + "_child_include_hdrs_before_deps.ko",
        module_deps = [
            name + "_parent_include_hdrs_before_deps",
        ],
        expected_includes = [
            # do not sort
            # in _include_hdrs_before_deps, in hdrs, _base_headers comes before _include_headers
            "{}/include/base".format(native.package_name()),
            "{}/include".format(native.package_name()),
        ],
    )
    tests.append(name + "_child_include_hdrs_before_deps")

    _bad_test_make(
        name = name + "_ddk_headers_in_srcs",
        error_message = "is a ddk_headers or ddk_module but specified in srcs. Specify it in deps instead.",
        module_srcs = [name + "_self_headers"],
        module_out = "dep.ko",
    )
    tests.append(name + "_ddk_headers_in_srcs")

    _makefiles_include_ordering_artifacts_test(name = name + "_include_ordering_artifacts_test")
    tests.append(name + "_include_ordering_artifacts_test")

    _makefiles_submodule_symvers_test(
        name = name + "_submodule_symvers_test",
        kernel_build = name + "_kernel_build",
    )
    tests.append(name + "_submodule_symvers_test")

    _makefiles_cond_srcs_test(
        name = name + "_cond_srcs_test",
    )
    tests.append(name + "_cond_srcs_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
