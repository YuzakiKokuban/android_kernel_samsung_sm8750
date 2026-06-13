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
Common utilities for working with kernel images.
"""

load("@bazel_skylib//lib:shell.bzl", "shell")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("//build/kernel/kleaf:directory_with_structure.bzl", dws = "directory_with_structure")
load(
    ":common_providers.bzl",
    "KernelModuleInfo",
)
load(":debug.bzl", "debug")
load(":utils.bzl", "kernel_utils", "utils")

visibility("//build/kernel/kleaf/...")

SYSTEM_DLKM_STAGING_ARCHIVE_NAME = "system_dlkm_staging_archive.tar.gz"
SYSTEM_DLKM_MODULES_LOAD_NAME = "system_dlkm.modules.load"

VENDOR_DLKM_STAGING_ARCHIVE_NAME = "vendor_dlkm_staging_archive.tar.gz"

def _build_modules_image_impl_common(
        ctx,
        what,
        outputs,
        build_command,
        modules_staging_dir,
        restore_modules_install = None,
        set_ext_modules = None,
        implicit_outputs = None,
        additional_inputs = None,
        mnemonic = None):
    """Command implementation for building images that directly contain modules.

    Args:
        ctx: ctx.
        what: what is being built, for logging.
        outputs: list of `ctx.actions.declare_file`
        build_command: the command to build `outputs` and `implicit_outputs`.
        modules_staging_dir: a staging directory for module installation.
        restore_modules_install: If `True`, restore `ctx.attr.kernel_modules_install`.
         Default is `True`.
        set_ext_modules: If `True`, set variable `EXT_MODULES` before invoking script
          in `build_utils.sh`
        implicit_outputs: like `outputs`, but not installed to `DIST_DIR` (not
         returned in `DefaultInfo`).
        additional_inputs: Additional files to be included.
        mnemonic: string to reference the build operation.
    """

    if restore_modules_install == None:
        restore_modules_install = True

    kernel_build_infos = ctx.attr.kernel_modules_install[KernelModuleInfo].kernel_build_infos
    kernel_build_outs = depset(
        transitive = [
            # Prefer device kernel_build, then base kernel_build
            kernel_build_infos.images_info.outs,
            kernel_build_infos.images_info.base_kernel_files,
        ],
        order = "preorder",
    )

    # depset.to_list() required for find_file.
    # TODO(b/256688440): providers should provide System.map directly
    kernel_build_outs = kernel_build_outs.to_list()
    system_map = utils.find_file(
        name = "System.map",
        files = kernel_build_outs,
        required = True,
        what = "{}: outs of dependent kernel_build {}".format(ctx.label, kernel_build_infos.label),
    )

    modules_install_staging_dws = None
    if restore_modules_install:
        modules_install_staging_dws_list = ctx.attr.kernel_modules_install[KernelModuleInfo].modules_staging_dws_depset.to_list()
        if len(modules_install_staging_dws_list) != 1:
            fail("{}: {} is not a `kernel_modules_install`.".format(
                ctx.label,
                ctx.attr.kernel_modules_install.label,
            ))
        modules_install_staging_dws = modules_install_staging_dws_list[0]

    inputs = []
    if additional_inputs != None:
        inputs += additional_inputs
    inputs.append(system_map)
    if restore_modules_install:
        inputs += dws.files(modules_install_staging_dws)
    inputs += ctx.files.deps
    transitive_inputs = [kernel_build_infos.serialized_env_info.inputs]
    tools = kernel_build_infos.serialized_env_info.tools

    command_outputs = []
    command_outputs += outputs
    if implicit_outputs != None:
        command_outputs += implicit_outputs

    command = kernel_utils.setup_serialized_env_cmd(
        serialized_env_info = kernel_build_infos.serialized_env_info,
        restore_out_dir_cmd = utils.get_check_sandbox_cmd(),
    )

    for attr_name in (
        "modules_list",
        "modules_recovery_list",
        "modules_charger_list",
        "modules_blocklist",
        "vendor_dlkm_fs_type",
        "vendor_dlkm_modules_list",
        "vendor_dlkm_modules_blocklist",
        "vendor_dlkm_props",
        "system_dlkm_fs_type",
        "system_dlkm_fs_types",
        "system_dlkm_modules_list",
        "system_dlkm_modules_blocklist",
        "system_dlkm_props",
    ):
        # Checks if attr_name is a valid attribute name in the current rule.
        # If not, do not touch its value.
        if not hasattr(ctx.file, attr_name):
            continue

        # If it is a valid attribute name, set environment variable to the path if the argument is
        # supplied, otherwise set environment variable to empty.
        file = getattr(ctx.file, attr_name)
        path = ""
        if file != None:
            path = file.path
            inputs.append(file)
        command += """
            {name}={path}
        """.format(
            name = attr_name.upper(),
            path = path,
        )

    # Allow writing to files because create_modules_staging wants to overwrite modules.order.
    if restore_modules_install:
        command += dws.restore(
            modules_install_staging_dws,
            dst = modules_staging_dir,
            options = "-aL --chmod=F+w --exclude=source --exclude=build",
        )

        # source/ and build/ are symlinks to the source tree and $OUT_DIR, respectively,
        # so they are copied as links.
        command += dws.restore(
            modules_install_staging_dws,
            dst = modules_staging_dir,
            options = "-al --chmod=F+w --include=source --include=build --exclude='*'",
        )

    modules_order_cmd = ""
    if ctx.attr.create_modules_order:
        modules_order_depset = ctx.attr.kernel_modules_install[KernelModuleInfo].modules_order
        modules_order_depset_list = modules_order_depset.to_list()
        inputs += modules_order_depset_list
        modules_order_cmd = """
            cat {modules_order} > kleaf_modules.order
            KLEAF_MODULES_ORDER=kleaf_modules.order
        """.format(
            modules_order = " ".join([modules_order.path for modules_order in modules_order_depset_list]),
        )

    if set_ext_modules and ctx.attr._set_ext_modules[BuildSettingInfo].value:
        ext_modules = ctx.attr.kernel_modules_install[KernelModuleInfo].packages.to_list()
        command += """EXT_MODULES={quoted_ext_modules}""".format(
            quoted_ext_modules = shell.quote(" ".join(ext_modules)),
        )

    if not ctx.attr._set_ext_modules[BuildSettingInfo].value:
        # buildifier: disable=print
        print("""\nWARNING: This is a temporary flag to mitigate issues on migrating away from
setting EXT_MODULES in build.config. If you need --noset_ext_modules, please
file a bug.""")

    command += """
             # Restore System.map to DIST_DIR for run_depmod in create_modules_staging
               mkdir -p ${{DIST_DIR}}
               cp {system_map} ${{DIST_DIR}}/System.map

               {modules_order_cmd}
               {build_command}
    """.format(
        system_map = system_map.path,
        modules_order_cmd = modules_order_cmd,
        build_command = build_command,
    )

    debug.print_scripts(ctx, command)
    ctx.actions.run_shell(
        mnemonic = mnemonic,
        inputs = depset(inputs, transitive = transitive_inputs),
        tools = tools,
        outputs = command_outputs,
        progress_message = "Building {} {}".format(what, ctx.label),
        command = command,
    )
    return DefaultInfo(files = depset(outputs))

def _build_modules_image_attrs_common(additional = None):
    """Common attrs for rules that builds images that directly contain modules."""
    ret = {
        "kernel_modules_install": attr.label(
            mandatory = True,
            providers = [KernelModuleInfo],
        ),
        "deps": attr.label_list(
            allow_files = True,
        ),
        "_debug_print_scripts": attr.label(
            default = "//build/kernel/kleaf:debug_print_scripts",
        ),
        "_set_ext_modules": attr.label(
            default = "//build/kernel/kleaf:set_ext_modules",
        ),
        "create_modules_order": attr.bool(
            default = True,
            doc = """Whether to create and keep a modules.order file generated
                by a postorder traversal of the `kernel_modules_install` sources.
                It defaults to `True`.""",
        ),
    }
    if additional != None:
        ret.update(additional)
    return ret

def _ramdisk_options(ramdisk_compression, ramdisk_compression_args):
    """Options for how to treat ramdisk images.

    Args:
        ramdisk_compression: If provided it specfies the format used for any ramdisks generated.
         If not provided a fallback value from build.config is used.
         Possible values are `lz4`, `gzip`, None.
        ramdisk_compression_args: Command line arguments passed to lz4 command
         to control compression level (defaults to `-12 --favor-decSpeed`).
         For iterative kernel development where faster compression is more
         desirable than a high compression ratio, it can be useful to control
         the compression ratio.
    """

    # Initially fallback to values from build.config.* files.
    _ramdisk_compress = "${RAMDISK_COMPRESS}"
    _ramdisk_decompress = "${RAMDISK_DECOMPRESS}"
    _ramdisk_ext = "lz4"

    if ramdisk_compression == "lz4":
        _ramdisk_compress = "lz4 -c -l "
        if ramdisk_compression_args:
            _ramdisk_compress += ramdisk_compression_args
        else:
            _ramdisk_compress += "-12 --favor-decSpeed"
        _ramdisk_decompress = "lz4 -c -d -l"
    if ramdisk_compression == "gzip":
        _ramdisk_compress = "gzip -c -f"
        _ramdisk_decompress = "gzip -c -d"
        _ramdisk_ext = "gz"

    return struct(
        ramdisk_compress = _ramdisk_compress,
        ramdisk_decompress = _ramdisk_decompress,
        ramdisk_ext = _ramdisk_ext,
    )

image_utils = struct(
    build_modules_image_impl_common = _build_modules_image_impl_common,
    build_modules_image_attrs_common = _build_modules_image_attrs_common,
    ramdisk_options = _ramdisk_options,
)
