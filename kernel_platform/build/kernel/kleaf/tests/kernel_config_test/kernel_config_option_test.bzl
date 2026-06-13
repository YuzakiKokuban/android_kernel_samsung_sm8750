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

"""
Test kernel_config against options (e.g. lto).
Require //common package.

Deprecated:
    See kernel_defconfig_fragments_test
"""

load("@bazel_skylib//lib:dicts.bzl", "dicts")
load("//build/kernel/kleaf:constants.bzl", "LTO_VALUES")
load("//build/kernel/kleaf:kernel.bzl", "kernel_build")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")
load("//build/kernel/kleaf/impl:utils.bzl", "utils")
load("//build/kernel/kleaf/tests/utils:contain_lines_test.bzl", "contain_lines_test")
load(":kernel_config_aspect.bzl", "KernelConfigAspectInfo", "kernel_config_aspect")

# Helper functions and rules.

_KASAN_FLAG = "//build/kernel/kleaf:kasan"
_KCSAN_FLAG = "//build/kernel/kleaf:kcsan"
_KGDB_FLAG = "//build/kernel/kleaf:kgdb"
_LTO_FLAG = "//build/kernel/kleaf:lto"
_ARCHS = ("aarch64", "x86_64")

def _get_config_file(ctx, kernel_build, filename):
    """Gets the `.config` file of the `kernel_build` to a file with file name `{filename}`.

    The config file is compared with `data/{filename}` later by the caller.

    Return:
        The file with name `{prefix}_config`, which points to the `.config` of the kernel.
    """
    kernel_config = kernel_build[KernelConfigAspectInfo].kernel_config
    out_dir = utils.find_file(
        name = "out_dir",
        files = kernel_config.files.to_list(),
        what = "{}: kernel_config outputs".format(kernel_build.label),
    )

    # Create symlink so that the Python test script compares with the correct expected file.
    out = ctx.actions.declare_file("{}/{}".format(ctx.label.name, filename))

    hermetic_tools = hermetic_toolchain.get(ctx)
    command = hermetic_tools.setup + """
        cp -pL {out_dir}/.config {out}
    """.format(
        out_dir = out_dir.path,
        out = out.path,
    )

    ctx.actions.run_shell(
        inputs = [out_dir],
        outputs = [out],
        command = command,
        tools = hermetic_tools.deps,
        mnemonic = "GetConfigFile",
        progress_message = "Getting .config {}".format(ctx.label),
    )

    return out

def _get_config_attrs_common(transition):
    """Common attrs for rules to get `.config` of the given `kernel_build` with the given transition.

    Args:
      transition: The transition. May be `None` if no transition is needed.
    """
    attrs = {
        "kernel_build": attr.label(cfg = transition, aspects = [kernel_config_aspect], mandatory = True),
    }
    if transition != None:
        attrs.update({
            "_allowlist_function_transition": attr.label(
                default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
            ),
        })
    return attrs

def _get_transitioned_config_impl(ctx):
    """Common impl for getting `.config` of the given `kernel_build` with the given transition.

    Helper for testing a flag.
    """
    files = [
        _get_config_file(ctx, kernel_build, key + "_config")
        for key, kernel_build in ctx.split_attr.kernel_build.items()
    ]
    return DefaultInfo(files = depset(files), runfiles = ctx.runfiles(files = files))

def _transition_test(name, kernel_build, test_data_rule, expected, **test_data_rule_kwargs):
    """Test the effect of a flag on `kernel_config`.

    Helper for testing a flag.

    Args:
        name: name of test
        test_data_rule: `rule()` to get the actual `.config` of a kernel.
        expected: A list of expected files.
        kernel_build: target under test
        **test_data_rule_kwargs: kwargs to `test_data_rule`.
    """
    test_data_rule(
        name = name + "_actual",
        kernel_build = kernel_build,
        **test_data_rule_kwargs
    )
    native.filegroup(
        name = name + "_expected",
        srcs = expected,
    )
    contain_lines_test(
        name = name,
        expected = name + "_expected",
        actual = name + "_actual",
    )

def _get_config_impl(ctx):
    symlink = _get_config_file(ctx, ctx.attr.kernel_build, ctx.attr.prefix + "_config")
    return DefaultInfo(files = depset([symlink]), runfiles = ctx.runfiles(files = [symlink]))

_get_config = rule(
    implementation = _get_config_impl,
    doc = "Generic way to get `.config` for a kernel without transition. Helper for testing attributes.",
    attrs = dicts.add(_get_config_attrs_common(None), {
        "prefix": attr.string(doc = "prefix of output file name"),
    }),
    toolchains = [hermetic_toolchain.type],
)

# Tests

## Tests on --lto

def _lto_transition_impl(_settings, _attr):
    return {value: {_LTO_FLAG: value} for value in LTO_VALUES}

_lto_transition = transition(
    implementation = _lto_transition_impl,
    inputs = [],
    outputs = [_LTO_FLAG],
)

_lto_test_data = rule(
    implementation = _get_transitioned_config_impl,
    doc = "Get `.config` for a kernel with the LTO transition.",
    attrs = _get_config_attrs_common(_lto_transition),
    toolchains = [hermetic_toolchain.type],
)

def _lto_test(name, kernel_build):
    """Test the effect of a `--lto` on `kernel_config`."""
    _transition_test(
        name = name,
        kernel_build = kernel_build,
        test_data_rule = _lto_test_data,
        expected = ["data/{}_config".format(lto) for lto in LTO_VALUES],
    )

## Tests on --kasan
def _kasan_str(kasan):
    return "kasan" if kasan else "nokasan"

def _kasan_transition_impl(_settings, _attr):
    return {_kasan_str(kasan): {_KASAN_FLAG: kasan} for kasan in (True, False)}

_kasan_transition = transition(
    implementation = _kasan_transition_impl,
    inputs = [],
    outputs = [_KASAN_FLAG],
)

# Kasan|Kcsan requires LTO=none to run, otherwise it fails.
def _no_lto_impl(settings, attr):
    _ignore = (settings, attr)  # @unused
    return {_LTO_FLAG: "default"}

_force_no_lto_transition = transition(
    implementation = _no_lto_impl,
    inputs = [],
    outputs = [_LTO_FLAG],
)

_kasan_test_data = rule(
    implementation = _get_transitioned_config_impl,
    doc = "Get `.config` for a kernel with the KASAN transition.",
    attrs = _get_config_attrs_common(_kasan_transition),
    cfg = _force_no_lto_transition,
    toolchains = [hermetic_toolchain.type],
)

def _kasan_test(name, kernel_build):
    """Test the effect of a `--kasan` on `kernel_config`."""
    _transition_test(
        name = name,
        kernel_build = kernel_build,
        test_data_rule = _kasan_test_data,
        expected = ["data/{}_config".format(_kasan_str(kasan)) for kasan in (True, False)],
    )

## Tests on --kcsan
def _kcsan_str(kcsan):
    return "kcsan" if kcsan else "nokcsan"

def _kcsan_transition_impl(_settings, _attr):
    return {_kcsan_str(kcsan): {_KCSAN_FLAG: kcsan} for kcsan in (True, False)}

_kcsan_transition = transition(
    implementation = _kcsan_transition_impl,
    inputs = [],
    outputs = [_KCSAN_FLAG],
)

_kcsan_test_data = rule(
    implementation = _get_transitioned_config_impl,
    doc = "Get `.config` for a kernel with the KCSAN transition.",
    attrs = _get_config_attrs_common(_kcsan_transition),
    cfg = _force_no_lto_transition,
    toolchains = [hermetic_toolchain.type],
)

def _kcsan_test(name, kernel_build):
    """Test the effect of a `--kcsan` on `kernel_config`."""
    _transition_test(
        name = name,
        kernel_build = kernel_build,
        test_data_rule = _kcsan_test_data,
        expected = ["data/{}_config".format(_kcsan_str(kcsan)) for kcsan in (True, False)],
    )

## Tests on --kgdb
def _kgdb_str(kgdb, arch):
    return ("kgdb" if kgdb else "nokgdb") + "_" + arch

def _kgdb_transition_impl(_settings, attr):
    return {_kgdb_str(kgdb, attr.arch): {_KGDB_FLAG: kgdb} for kgdb in (True, False)}

_kgdb_transition = transition(
    implementation = _kgdb_transition_impl,
    inputs = [],
    outputs = [_KGDB_FLAG],
)

_kgdb_test_data = rule(
    implementation = _get_transitioned_config_impl,
    doc = "Get `.config` for a kernel with the kgdb transition.",
    attrs = dicts.add(_get_config_attrs_common(_kgdb_transition), {
        "arch": attr.string(),
    }),
    toolchains = [hermetic_toolchain.type],
)

def _kgdb_test(name, arch, kernel_build):
    """Test the effect of a `--kgdb` on `kernel_config`."""

    _transition_test(
        name = name,
        kernel_build = kernel_build,
        test_data_rule = _kgdb_test_data,
        expected = ["data/{}_config".format(_kgdb_str(kgdb, arch)) for kgdb in (True, False)],
        arch = arch,
    )

## Tests on `trim_nonlisted_kmi`

def _trim_str(trim):
    return "trim" if trim else "notrim"

def _trim_test(name, kernels):
    """Test the effect of `trim_nonlisted_kmi` on `kernel_config`.

    Args:
        name: name of test
        kernels: a dict, where key is a struct, and value is
          the label to the target under test (`kernel_build`).

          The key struct contains:
          - trim: whether trimming is enabled
          - arch: architecture
    """
    tests = []
    for key, kernel_build in kernels.items():
        trim_str = _trim_str(key.trim)
        prefix = key.arch + "_" + trim_str
        test_name = "{name}_{prefix}".format(name = name, prefix = prefix)

        _get_config(
            name = test_name + "_config",
            prefix = trim_str,
            kernel_build = kernel_build,
        )
        contain_lines_test(
            name = test_name,
            expected = "data/{}_config".format(trim_str),
            actual = test_name + "_config",
        )
        tests.append(test_name)
    native.test_suite(
        name = name,
        tests = tests,
    )

## Tests on all combinations.

def _combined_test_combinations(key):
    ret = {}
    for lto in LTO_VALUES:
        for kasan in (True, False):
            for kcsan in (True, False):
                for kgdb in (True, False):
                    if kasan and lto not in ("default", "none"):
                        continue
                    if kcsan and lto not in ("default", "none"):
                        continue

                    test_name = "_".join([
                        key.arch,
                        _trim_str(key.trim),
                        lto,
                        _kasan_str(kasan),
                        _kgdb_str(kgdb, key.arch),
                    ])
                    ret_key = {
                        "lto": lto,
                        "kasan": kasan,
                        "kcsan": kcsan,
                        "kgdb": kgdb,
                    }
                    ret[test_name] = ret_key
    return ret

def _combined_test_expected_impl(ctx):
    expected_file_names = [
        ctx.attr.lto + "_config",
        _kasan_str(ctx.attr.kasan) + "_config",
        _kcsan_str(ctx.attr.kcsan) + "_config",
        _kgdb_str(ctx.attr.kgdb, ctx.attr.arch) + "_config",
        _trim_str(ctx.attr.trim) + "_config",
    ]
    files = []
    for f in ctx.files.srcs:
        if f.basename in expected_file_names:
            files.append(f)
    if not len(files) == len(expected_file_names):
        fail("{}: Can't find all expected files: {}, but expected {}".format(
            ctx.label,
            files,
            expected_file_names,
        ))
    hermetic_tools = hermetic_toolchain.get(ctx)
    command = hermetic_tools.setup
    for input_file in files:
        command += """
            cat {input} >> {out}.tmp
        """.format(
            input = input_file.path,
            out = ctx.outputs.out.path,
        )
    command += """
        cat {out}.tmp | sort | uniq > {out}
        rm {out}.tmp
    """.format(out = ctx.outputs.out.path)
    ctx.actions.run_shell(
        inputs = files,
        outputs = [ctx.outputs.out],
        tools = hermetic_tools.deps,
        command = command,
        progress_message = "Creating expected config for {}".format(ctx.label),
    )
    return DefaultInfo(
        files = depset([ctx.outputs.out]),
        runfiles = ctx.runfiles(files = [ctx.outputs.out]),
    )

_combined_test_expected = rule(
    implementation = _combined_test_expected_impl,
    doc = "Test on a given combination of flags and attributes on `kernel_config`",
    attrs = {
        "trim": attr.bool(),
        "lto": attr.string(values = LTO_VALUES),
        "kasan": attr.bool(),
        "kcsan": attr.bool(),
        "kgdb": attr.bool(),
        "arch": attr.string(),
        "srcs": attr.label_list(allow_files = True),
        "out": attr.output(),
    },
    toolchains = [hermetic_toolchain.type],
)

def _combined_test_actual_transition_impl(_settings, attr):
    return {
        _LTO_FLAG: attr.lto,
        _KASAN_FLAG: attr.kasan,
        _KCSAN_FLAG: attr.kcsan,
        _KGDB_FLAG: attr.kgdb,
    }

_combined_test_actual_transition = transition(
    implementation = _combined_test_actual_transition_impl,
    inputs = [],
    outputs = [_KASAN_FLAG, _KCSAN_FLAG, _KGDB_FLAG, _LTO_FLAG],
)

def _combined_test_actual_impl(ctx):
    if len(ctx.attr.kernel_build) != 1:
        fail("FATAL: Expected 1:1 transition on ctx.attr.kernel_build")
    symlink = _get_config_file(ctx, ctx.attr.kernel_build[0], ctx.attr.prefix + "_config")
    return DefaultInfo(files = depset([symlink]), runfiles = ctx.runfiles(files = [symlink]))

_combined_test_actual = rule(
    implementation = _combined_test_actual_impl,
    doc = "Test on a given combination of flags and attributes on `kernel_config`",
    attrs = dicts.add(_get_config_attrs_common(_combined_test_actual_transition), {
        "lto": attr.string(values = LTO_VALUES),
        "kasan": attr.bool(),
        "kcsan": attr.bool(),
        "kgdb": attr.bool(),
        "prefix": attr.string(),
    }),
    toolchains = [hermetic_toolchain.type],
)

def _combined_option_test(name, kernels):
    """Test the effect of all possible combinations of flags on `kernel_config`:

    Args:
        name: name of test
        kernels: a dict, where key is a struct, and value is
          the label to the target under test (`kernel_build`).

          The key struct contains:
          - trim: whether trimming is enabled
          - arch: architecture
    """
    tests = []
    for key, kernel_build in kernels.items():
        for test_name, combination in _combined_test_combinations(key).items():
            test_name = name + "_" + test_name
            out_prefix = test_name

            # key.trim is the value of trim_nonlisted_kmi declared in kernel_build macro.
            # expected_trim is the expected value of CONFIG_TRIM_UNUSED_KSYMS, affected by kasan.
            expected_trim = key.trim
            if combination["kasan"]:
                expected_trim = False
            if combination["kcsan"]:
                expected_trim = False
            if combination["kgdb"]:
                expected_trim = False

            _combined_test_expected(
                name = test_name + "_expected",
                out = out_prefix + "_config",
                srcs = ["data/{}_config".format(lto) for lto in LTO_VALUES] +
                       ["data/{}_config".format(_kasan_str(kasan)) for kasan in (True, False)] +
                       ["data/{}_config".format(_kcsan_str(kcsan)) for kcsan in (True, False)] +
                       ["data/{}_config".format(_kgdb_str(kgdb, key.arch)) for kgdb in (True, False)] +
                       ["data/{}_config".format(_trim_str(trim)) for trim in (True, False)],
                arch = key.arch,
                trim = expected_trim,
                **combination
            )

            _combined_test_actual(
                name = test_name + "_actual",
                prefix = out_prefix,
                kernel_build = kernel_build,
                **combination
            )

            contain_lines_test(
                name = test_name,
                actual = test_name + "_actual",
                expected = test_name + "_expected",
            )
            tests.append(test_name)

    native.test_suite(
        name = name,
        tests = tests,
    )

## Exported test suite.

def kernel_config_option_test_suite(name):
    """Tests for various flags on `kernel_config`.

    Args:
        name: name of the test.
    """
    for arch in _ARCHS:
        kernel_build_arch = arch
        if kernel_build_arch == "aarch64":
            kernel_build_arch = "arm64"

        kernel_build(
            name = name + "_kernel_{}".format(arch),
            srcs = ["//common:kernel_{}_sources".format(arch)],
            arch = kernel_build_arch,
            build_config = "//common:build.config.gki.{}".format(arch),
            make_goals = ["FAKE_MAKE_GOALS"],
            outs = [],
            tags = ["manual"],
        )

        kernel_build(
            name = name + "_kernel_{}_trim".format(arch),
            srcs = ["//common:kernel_{}_sources".format(arch)],
            arch = kernel_build_arch,
            build_config = "//common:build.config.gki.{}".format(arch),
            trim_nonlisted_kmi = True,
            kmi_symbol_list = "data/fake_kmi_symbol_list",
            make_goals = ["FAKE_MAKE_GOALS"],
            outs = [],
            tags = ["manual"],
        )

        kernel_build(
            name = name + "_kernel_{}_notrim".format(arch),
            srcs = ["//common:kernel_{}_sources".format(arch)],
            arch = kernel_build_arch,
            build_config = "//common:build.config.gki.{}".format(arch),
            trim_nonlisted_kmi = False,
            kmi_symbol_list = "data/fake_kmi_symbol_list",
            make_goals = ["FAKE_MAKE_GOALS"],
            outs = [],
            tags = ["manual"],
        )

    trim_kernels = {}
    for arch in _ARCHS:
        for trim in (True, False):
            trim_kernels[struct(trim = trim, arch = arch)] = \
                name + "_kernel_{}_{}".format(arch, _trim_str(trim))

    tests = []

    for arch in _ARCHS:
        _lto_test(
            name = name + "_lto_{}_test".format(arch),
            kernel_build = name + "_kernel_{}".format(arch),
        )
        tests.append(name + "_lto_{}_test".format(arch))

        _kasan_test(
            name = name + "_kasan_{}_test".format(arch),
            kernel_build = name + "_kernel_{}".format(arch),
        )
        tests.append(name + "_kasan_{}_test".format(arch))
        _kcsan_test(
            name = name + "_kcsan_{}_test".format(arch),
            kernel_build = name + "_kernel_{}".format(arch),
        )
        tests.append(name + "_kcsan_{}_test".format(arch))

        _kgdb_test(
            name = name + "_kgdb_{}_test".format(arch),
            kernel_build = name + "_kernel_{}".format(arch),
            arch = arch,
        )
        tests.append(name + "_kgdb_{}_test".format(arch))

    _trim_test(
        name = name + "_trim_test",
        kernels = trim_kernels,
    )
    tests.append(name + "_trim_test")

    _combined_option_test(
        name = name + "_combined_option_test",
        kernels = trim_kernels,
    )
    tests.append(name + "_combined_option_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
