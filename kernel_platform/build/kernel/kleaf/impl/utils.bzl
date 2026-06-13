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
Utilities for kleaf.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//lib:sets.bzl", "sets")
load("@bazel_skylib//lib:shell.bzl", "shell")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load(
    ":common_providers.bzl",
    "DdkConfigInfo",
    "DdkSubmoduleInfo",
    "KernelBuildExtModuleInfo",
    "KernelImagesInfo",
    "KernelModuleDepInfo",
    "KernelModuleInfo",
    "KernelModuleKernelBuildInfo",
    "KernelModuleSetupInfo",
    "KernelSerializedEnvInfo",
    "ModuleSymversInfo",
)
load(":ddk/ddk_headers.bzl", "DdkHeadersInfo")

visibility("//build/kernel/kleaf/...")

def _reverse_dict(d):
    """Reverse a dictionary of {key: [value, ...]}

    Return {value: [key, ...]}.
    """
    ret = {}
    for k, values in d.items():
        for v in values:
            if v not in ret:
                ret[v] = []
            ret[v].append(k)
    return ret

def _getoptattr(thing, attr, default_value = None):
    """Return attribute value if |thing| has attribute named |attr|, otherwise return |default_value|."""
    if hasattr(thing, attr):
        return getattr(thing, attr)
    return default_value

def find_file(name, files, what, required = False):
    """Find a file named |name| in the list of |files|. Expect zero or one match.

    Args:
        name: Name of the file to be searched.
        files: List of files.
        what: Target.
        required: whether to fail if a non exact result is produced.

    Returns:
        A match when found or `None`.
    """
    result = []
    for file in files:
        if file.basename == name:
            result.append(file)
    if len(result) > 1 or (not result and required):
        fail("{what} contains {actual_len} file(s) named {name}, expected {expected_len}{files}".format(
            what = what,
            actual_len = len(result),
            name = name,
            expected_len = "1" if required else "0 or 1",
            files = ":\n  " + ("\n  ".join([e.path for e in result])) if result else "",
        ))
    return result[0] if result else None

def find_files(files, suffix = None):
    """Find files which names end with a given |suffix|.

    Args:
        files: list of files to inspect.
        suffix: Looking for files ending with this given suffix.

    Returns:
        A list of files.
    """
    result = []
    for file in files:
        if suffix != None and file.basename.endswith(suffix):
            result.append(file)
    return result

def _package_bin_dir(ctx):
    """Return the directory for output files in this package.

    This is similar to

    ```
    dirname(ctx.actions.declare_directory("x"))
    ```

    ... but not actually declare any directory, so there's no `File` object
    and no need to add it to the list of outputs of an action.
    """
    return paths.join(
        ctx.bin_dir.path,
        ctx.label.workspace_root,
        ctx.label.package,
    )

def _intermediates_dir(ctx):
    """Return a good directory for intermediates.

    This generally ensures that different targets have their own intermediates
    dir. This is similar to

    ```
    ctx.actions.declare_directory(ctx.attr.name + "_intermediates")
    ```

    ... but not actually declaring the directory, so there's no `File` object
    and no need to add it to the list of outputs of an action. It also won't
    conflict with any other actions that generates outputs of
    `declare_file(ctx.attr.name + "_intermediates/" + file_name)`.

    For sandboxed actions, this means the intermediates dir does not need to be
    cleaned up. However, for local actions, the result of intermediates dir from
    a previous build may remain and affect a later build. Use with caution.
    """
    return paths.join(
        _package_bin_dir(ctx),
        ctx.attr.name + "_intermediates",
    )

def _compare_file_names(files, expected_file_names, what):
    """Check that the list of files matches the given expected list.

    The basenames of files are checked.

    Args:
      files: A list of [File](https://bazel.build/rules/lib/File)s.
      expected_file_names: A list of file names to check files against.
      what: description of the caller that compares the file names.
    """

    actual_file_names = [file.basename for file in files]
    actual_set = sets.make(actual_file_names)
    expected_set = sets.make(expected_file_names)
    if not sets.is_equal(actual_set, expected_set):
        fail("{}: Actual: {}\nExpected: {}".format(
            what,
            actual_file_names,
            expected_file_names,
        ))

def _sanitize_label_as_filename(label):
    """Sanitize a Bazel label so it is safe to be used as a filename."""
    label_text = str(label)
    return _normalize(label_text)

def _normalize(s):
    """Returns a normalized string by replacing non-letters / non-numbers as underscores."""
    return "".join([c if c.isalnum() else "_" for c in s.elems()])

def _hash_hex(x):
    """Returns `hash(x)` in hex format."""
    ret = "%x" % hash(x)
    if len(ret) < 8:
        ret = "0" * (8 - len(ret)) + ret
    return ret

def _get_check_sandbox_cmd():
    """Returns a script that tries to check if we are running in a sandbox.

    Note: This is not always accurate."""

    return """
           if [[ ! $PWD =~ /(sandbox|bazel-working-directory|linux-sandbox|processwrapper-sandbox)/ ]]; then
             echo "FATAL: this action must be executed in a sandbox! Actual: $PWD" >&2
             exit 1
           fi
    """

def _write_depset(ctx, d, out):
    """Writes a depset to a file.

    Requires `_write_depset` in attrs.

    Args:
        ctx: ctx
        d: the depset
        out: name of the output file
    Returns:
        A struct with the following fields:
        - depset_file: the declared output file.
        - depset: a depset that contains `d` and `depset_file`
    """
    out_file = ctx.actions.declare_file("{}/{}".format(ctx.attr.name, out))

    args = ctx.actions.args()
    args.add(out_file)
    args.add_all(d)
    ctx.actions.run(
        executable = ctx.executable._write_depset,
        arguments = [args],
        outputs = [out_file],
        mnemonic = "WriteDepset",
        progress_message = "Dumping depset to {}: {}".format(out, ctx.label),
    )
    return struct(
        depset_file = out_file,
        depset = depset([out_file], transitive = [d]),
    )

# Utilities that applies to all Bazel stuff in general. These functions are
# not Kleaf specific.
utils = struct(
    package_bin_dir = _package_bin_dir,
    intermediates_dir = _intermediates_dir,
    reverse_dict = _reverse_dict,
    getoptattr = _getoptattr,
    find_file = find_file,
    find_files = find_files,
    compare_file_names = _compare_file_names,
    sanitize_label_as_filename = _sanitize_label_as_filename,
    normalize = _normalize,
    hash_hex = _hash_hex,
    get_check_sandbox_cmd = _get_check_sandbox_cmd,
    write_depset = _write_depset,
)

def _filter_module_srcs(files):
    """Filters and categorizes sources for building `kernel_module`."""
    hdrs = []
    scripts = []
    kconfig = []
    for file in files:
        if file.path.endswith(".h"):
            hdrs.append(file)
        if ("Makefile" in file.path or "scripts/" in file.path or
            file.basename == "module.lds.S"):
            scripts.append(file)
        if "Kconfig" in file.basename:
            kconfig.append(file)
    return struct(
        module_scripts = depset(scripts),
        module_hdrs = depset(hdrs),
        module_kconfig = depset(kconfig),
    )

def _transform_kernel_build_outs(name, what, outs):
    """Transform `*outs` attributes for `kernel_build`.

    - If `outs` is a list, return it directly.
    - If `outs` is a dict, return `select(outs)`.
    - Otherwise fail

    The logic should be in par with `_kernel_build_outs_add_vmlinux`.
    """
    if outs == None:
        return None
    if type(outs) == type([]):
        return outs
    elif type(outs) == type({}):
        return select(outs)
    else:
        fail("{}: Invalid type for {}: {}".format(name, what, type(outs)))

def _check_kernel_build(kernel_module_infos, kernel_build_label, this_label):
    """Check that kernel_modules have the same kernel_build as the given one.

    Args:
        kernel_module_infos: list of KernelModuleInfo of kernel module dependencies.
        kernel_build_label: the label of kernel_build.
        this_label: label of the module being checked.
    """

    for kernel_module_info in kernel_module_infos:
        if kernel_build_label == None:
            kernel_build_label = kernel_module_info.kernel_build_infos.label
            continue

        if kernel_module_info.kernel_build_infos.label != \
           kernel_build_label:
            fail((
                "{this_label} refers to kernel_build {kernel_build}, but " +
                "depended kernel_module {dep} refers to kernel_build " +
                "{dep_kernel_build}. They must refer to the same kernel_build."
            ).format(
                this_label = this_label,
                kernel_build = kernel_build_label,
                dep = kernel_module_info.label,
                dep_kernel_build = kernel_module_info.kernel_build_infos.label,
            ))

def _create_kernel_module_kernel_build_info(kernel_build):
    """Creates KernelModuleKernelBuildInfo.

    This info represents information on a kernel_module.kernel_build.

    Args:
        kernel_build: the `kernel_build` Target.
    """
    return KernelModuleKernelBuildInfo(
        label = kernel_build.label,
        ext_module_info = kernel_build[KernelBuildExtModuleInfo],
        # TODO(b/308492731): Implement the following for kernel_filegroup
        #   in order to build images
        serialized_env_info = kernel_build[KernelSerializedEnvInfo] if KernelSerializedEnvInfo in kernel_build else None,
        images_info = kernel_build[KernelImagesInfo] if KernelImagesInfo in kernel_build else None,
    )

def _local_exec_requirements(ctx):
    """Returns the execution requirement for `--config=local`.

    This should only be used on the actions that are proven to be safe to be
    built outside of the sandbox.
    """
    if ctx.attr._config_is_local[BuildSettingInfo].value:
        return {"local": "1"}
    return None

def _split_kernel_module_deps(deps, this_label):
    """Splits `deps` for a `kernel_module` or `ddk_module`.

    Args:
        deps: The list of deps
        this_label: label of the module being checked.
    """

    kernel_module_deps = []
    hdr_deps = []
    submodule_deps = []
    module_symvers_deps = []
    ddk_config_deps = []
    for dep in deps:
        is_valid_dep = False
        if DdkHeadersInfo in dep:
            hdr_deps.append(dep)
            is_valid_dep = True
        if all([info in dep for info in [KernelModuleSetupInfo, KernelModuleInfo, ModuleSymversInfo]]):
            kernel_module_deps.append(dep)
            is_valid_dep = True
        if all([info in dep for info in [DdkHeadersInfo, DdkSubmoduleInfo]]):
            submodule_deps.append(dep)
            is_valid_dep = True
        if ModuleSymversInfo in dep:
            module_symvers_deps.append(dep)
            is_valid_dep = True
        if DdkConfigInfo in dep:
            ddk_config_deps.append(dep)
            is_valid_dep = True
        if not is_valid_dep:
            fail("{}: {} is not a valid item in deps. Only kernel_module, ddk_module, ddk_headers, ddk_submodule are accepted.".format(this_label, dep.label))
    return struct(
        kernel_modules = kernel_module_deps,
        hdrs = hdr_deps,
        submodules = submodule_deps,
        module_symvers_deps = module_symvers_deps,
        ddk_configs = ddk_config_deps,
    )

def _create_kernel_module_dep_info(kernel_module):
    """Creates KernelModuleDepInfo.

    Args:
        kernel_module: A `kernel_module` Target.
    """

    return KernelModuleDepInfo(
        label = kernel_module.label,
        kernel_module_setup_info = kernel_module[KernelModuleSetupInfo],
        kernel_module_info = kernel_module[KernelModuleInfo],
        module_symvers_info = kernel_module[ModuleSymversInfo],
    )

# Cross compiler name is not always the same as the linux arch
# ARCH is not always the same as the architecture dir (b/254348147)
def _set_src_arch_cmd():
    """Returns a script that sets SRCARCH based on ARCH.

    This is where we find DEFCONFIG.

    The logic should be synced with common/Makefile.
    """

    return """
        SRCARCH=${ARCH}
        # Additional ARCH settings for x86
        if [[ ${ARCH} == "i386" ]]; then
                SRCARCH=x86
        fi
        if [[ ${ARCH} == "x86_64" ]]; then
                SRCARCH=x86
        fi
        # Additional ARCH settings for sparc
        if [[ ${ARCH} == "sparc32" ]]; then
               SRCARCH=sparc
        fi
        if [[ ${ARCH} == "sparc64" ]]; then
               SRCARCH=sparc
        fi
        # Additional ARCH settings for parisc
        if [[ ${ARCH} == "parisc64" ]]; then
               SRCARCH=parisc
        fi
    """

def _eval_restore_out_dir_cmd():
    """Returns a command that evaluates `KLEAF_RESTORE_OUT_DIR_CMD`.

    `KLEAF_RESTORE_OUT_DIR_CMD` should be set beforehand to a command line
    that restores the value of `OUT_DIR`. The variable is set by
    `kernel_utils.setup_serialized_env_cmd`.
    """
    return """
        if [[ -z "${KLEAF_RESTORE_OUT_DIR_CMD}" ]]; then
            echo "ERROR: FATAL: KLEAF_RESTORE_OUT_DIR_CMD is not defined!" >&2
            exit 1
        fi
        eval "${KLEAF_RESTORE_OUT_DIR_CMD}"
    """

def _setup_serialized_env_cmd(serialized_env_info, restore_out_dir_cmd):
    """Returns a command that sets up `KernelSerializedEnvInfo`.

    Args:
        serialized_env_info: `KernelSerializedEnvInfo`
        restore_out_dir_cmd: The command to restore value of `OUT_DIR`.
    """

    if not restore_out_dir_cmd:
        restore_out_dir_cmd = ":"

    return """
        KLEAF_RESTORE_OUT_DIR_CMD={quoted_restore_out_dir_cmd}
        . {setup_script}
        unset KLEAF_RESTORE_OUT_DIR_CMD
    """.format(
        quoted_restore_out_dir_cmd = shell.quote(restore_out_dir_cmd),
        setup_script = serialized_env_info.setup_script.path,
    )

kernel_utils = struct(
    filter_module_srcs = _filter_module_srcs,
    transform_kernel_build_outs = _transform_kernel_build_outs,
    check_kernel_build = _check_kernel_build,
    local_exec_requirements = _local_exec_requirements,
    split_kernel_module_deps = _split_kernel_module_deps,
    set_src_arch_cmd = _set_src_arch_cmd,
    create_kernel_module_kernel_build_info = _create_kernel_module_kernel_build_info,
    create_kernel_module_dep_info = _create_kernel_module_dep_info,
    eval_restore_out_dir_cmd = _eval_restore_out_dir_cmd,
    setup_serialized_env_cmd = _setup_serialized_env_cmd,
)
