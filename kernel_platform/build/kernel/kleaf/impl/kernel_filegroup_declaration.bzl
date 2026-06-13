# Copyright (C) 2024 The Android Open Source Project
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

"""Given a kernel_build, generates corresponding kernel_filegroup target declaration."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load(":common_providers.bzl", "KernelBuildFilegroupDeclInfo")
load(
    ":constants.bzl",
    "FILEGROUP_DEF_ARCHIVE_SUFFIX",
    "FILEGROUP_DEF_BUILD_FRAGMENT_NAME",
)
load(":hermetic_toolchain.bzl", "hermetic_toolchain")
load(":utils.bzl", "utils")

visibility("//build/kernel/kleaf/...")

def _kernel_filegroup_declaration_impl(ctx):
    info = ctx.attr.kernel_build[KernelBuildFilegroupDeclInfo]

    # Not allowed because mod_full_env & mod_inst_env for kernel_build needs
    # kbuild_mixed_tree_ret.outputs, but it cannot be vendored by
    # kernel_filegroup.
    if info.has_base_kernel:
        fail("""{}: {} has base_kernel. kernel_filegroup_declaration on
            device kernel build is not supported.""".format(ctx.label, ctx.attr.kernel_build.label))

    # ddk_artifacts
    deps_files = [
        # _modules_prepare
        info.modules_prepare_archive,
        info.modules_staging_archive,
    ]

    # Get the only file from the depset, so using to_list() here is fast.
    kernel_uapi_headers_lst = info.kernel_uapi_headers.to_list()
    if not kernel_uapi_headers_lst:
        fail("{}: {} does not have kernel_uapi_headers.".format(ctx.label, ctx.attr.kernel_build.label))
    if len(kernel_uapi_headers_lst) > 1:
        fail("{}: kernel_filegroup_declaration on kernel_build {} with base_kernel is not supported yet.".format(
            ctx.label,
            ctx.attr.kernel_build.label,
        ))
    kernel_uapi_headers = kernel_uapi_headers_lst[0]

    system_dlkm_staging_archive = utils.find_file(
        name = "system_dlkm_staging_archive.tar.gz",
        files = ctx.files.images,
        what = "{}: images".format(ctx.label),
        required = False,
    )

    template_file = _write_template_file(
        ctx = ctx,
        has_system_dlkm_staging_archive = bool(system_dlkm_staging_archive),
    )
    filegroup_decl_file = _write_filegroup_decl_file(
        ctx = ctx,
        info = info,
        deps_files = deps_files,
        kernel_uapi_headers = kernel_uapi_headers,
        system_dlkm_staging_archive = system_dlkm_staging_archive,
        template_file = template_file,
    )
    filegroup_decl_archive = _create_archive(
        ctx = ctx,
        info = info,
        deps_files = deps_files,
        kernel_uapi_headers = kernel_uapi_headers,
        filegroup_decl_file = filegroup_decl_file,
    )
    return DefaultInfo(files = depset([filegroup_decl_archive]))

def _write_template_file(
        ctx,
        has_system_dlkm_staging_archive):
    template_content = """\
_ALL_MODULE_NAMES = {all_module_names_repr}

platform(
    name = {target_platform_repr},
    constraint_values = [
        "@platforms//os:android",
        "@platforms//cpu:{arch}",
        # @kleaf//prebuilts/clang/host/linux-x86/kleaf:{toolchain_version}
        package_relative_label(_CLANG_KLEAF_PKG).same_package_label({toolchain_version_repr}),
    ],
    visibility = ["//visibility:private"],
)

platform(
    name = {exec_platform_repr},
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
        # @kleaf//prebuilts/clang/host/linux-x86/kleaf:{toolchain_version}
        package_relative_label(_CLANG_KLEAF_PKG).same_package_label({toolchain_version_repr}),
    ],
    visibility = ["//visibility:private"],
)

kernel_filegroup(
    name = {name_repr},
    srcs = {srcs_repr},
    deps = {deps_repr} + {extra_deps_repr},
    kernel_uapi_headers = {uapi_headers_repr},
    collect_unstripped_modules = {collect_unstripped_modules_repr},
    strip_modules = {strip_modules_repr},
    all_module_names = _ALL_MODULE_NAMES,
    kernel_release = {kernel_release_repr},
    protected_modules_list = {protected_modules_repr},
    ddk_module_defconfig_fragments = {ddk_module_defconfig_fragments_repr},
    config_out_dir_files = glob([{config_out_dir_repr} + "/**"]),
    config_out_dir = {config_out_dir_repr},
    env_setup_script = {env_setup_script_repr},
    modules_prepare_archive = {modules_prepare_archive_repr},
    module_env_archive = {module_env_archive_repr},
    outs = {outs_repr},
    internal_outs = {internal_outs_repr},
    target_platform = {target_platform_repr},
    exec_platform = {exec_platform_repr},
    visibility = ["//visibility:public"],
)
"""

    if has_system_dlkm_staging_archive:
        template_content += """\

extracted_system_dlkm_staging_archive(
    name = {modules_repr},
    src = {system_dlkm_staging_archive_repr},
    outs = _ALL_MODULE_NAMES,
    visibility = ["//visibility:public"],
)

[filegroup(
    name = "{}/{}".format({name_repr}, module_name),
    srcs = [{modules_repr}],
    output_group = module_name,
    visibility = ["//visibility:public"],
) for module_name in _ALL_MODULE_NAMES]
"""
    template_file = ctx.actions.declare_file("{}/{}_template.txt".format(
        ctx.attr.kernel_build.label.name,
        FILEGROUP_DEF_BUILD_FRAGMENT_NAME.removesuffix(".txt"),
    ))
    ctx.actions.write(output = template_file, content = template_content)
    return template_file

def _write_filegroup_decl_file(
        ctx,
        info,
        deps_files,
        kernel_uapi_headers,
        system_dlkm_staging_archive,
        template_file):
    ## Reused kwargs for TemplateDict: https://bazel.build/rules/lib/builtins/TemplateDict
    # For a list of files, represented in a list
    # Intentionally not adding comma for the last item so it works for the empty case.
    join = dict(join_with = ",\n        ", format_joined = "[\n        %s\n    ]")

    # For a single file. Use add_joined so we can use map_each and delay calculation.
    one = dict(join_with = "")

    # For extra downloaded files, prefixed with "//"
    extra = dict(
        allow_closure = True,
        map_each = lambda file: repr("//{}".format(file.basename) if file else None),
    )

    # For local files in this package. Files do not have any prefixes.
    pkg = dict(
        allow_closure = True,
        map_each = lambda file: repr("{}".format(file.path) if file else None),
    )

    sub = ctx.actions.template_dict()
    sub.add("{name_repr}", repr(ctx.attr.kernel_build.label.name))
    sub.add_joined("{srcs_repr}", info.filegroup_srcs, **(join | extra))
    sub.add_joined("{deps_repr}", depset(deps_files), **(join | pkg))
    sub.add_joined(
        "{extra_deps_repr}",
        depset(transitive = [target.files for target in ctx.attr.extra_deps]),
        **(join | extra)
    )
    sub.add_joined("{uapi_headers_repr}", depset([kernel_uapi_headers]), **(one | extra))
    sub.add("{collect_unstripped_modules_repr}", repr(info.collect_unstripped_modules))
    sub.add("{strip_modules_repr}", repr(info.strip_modules))
    sub.add_joined(
        "{all_module_names_repr}",
        depset(info.all_module_names),
        map_each = repr,
        **join
    )
    sub.add_joined("{kernel_release_repr}", depset([info.kernel_release]), **(one | pkg))
    sub.add_joined(
        "{protected_modules_repr}",
        depset([info.src_protected_modules_list]),
        **(one | pkg)
    )
    sub.add_joined(
        "{ddk_module_defconfig_fragments_repr}",
        info.ddk_module_defconfig_fragments,
        **(join | pkg)
    )
    sub.add_joined("{config_out_dir_repr}", depset([info.config_out_dir]), **(one | pkg))
    sub.add_joined("{env_setup_script_repr}", depset([info.env_setup_script]), **(one | pkg))
    sub.add_joined(
        "{modules_prepare_archive_repr}",
        depset([info.modules_prepare_archive]),
        **(one | pkg)
    )
    sub.add_joined("{module_env_archive_repr}", depset([info.module_env_archive]), **(one | pkg))

    # {"//vmlinux": "vmlinux", ...}
    sub.add_joined(
        "{outs_repr}",
        info.outs,
        allow_closure = True,
        map_each = lambda file: "{key}: {value}".format(
            key = repr("//{}".format(file.basename) if file else None),
            value = repr(paths.relativize(file.path, info.ruledir)),
        ),
        join_with = ",\n        ",
        format_joined = "{\n        %s\n    }",
    )

    # {":bazel-out/k8-fastbuild/bin/common/kernel_aarch64/Module.symvers": "Module.symvers", ...}
    sub.add_joined(
        "{internal_outs_repr}",
        info.internal_outs,
        allow_closure = True,
        map_each = lambda file: "{key}: {value}".format(
            key = repr(file.path if file else None),
            value = repr(paths.relativize(file.path, info.ruledir)),
        ),
        join_with = ",\n        ",
        format_joined = "{\n        %s\n    }",
    )

    sub.add("{toolchain_version}", info.toolchain_version)
    sub.add("{toolchain_version_repr}", repr(info.toolchain_version))
    sub.add("{target_platform_repr}", repr(ctx.attr.kernel_build.label.name + "_platform_target"))
    sub.add("{exec_platform_repr}", repr(ctx.attr.kernel_build.label.name + "_platform_exec"))
    sub.add("{arch}", info.arch)

    if system_dlkm_staging_archive:
        sub.add("{modules_repr}", repr(ctx.attr.kernel_build.label.name + "_modules"))
        sub.add_joined(
            "{system_dlkm_staging_archive_repr}",
            depset([system_dlkm_staging_archive]),
            **(one | extra)
        )

    filegroup_decl_file = ctx.actions.declare_file("{}/{}".format(
        ctx.attr.kernel_build.label.name,
        FILEGROUP_DEF_BUILD_FRAGMENT_NAME,
    ))
    ctx.actions.expand_template(
        template = template_file,
        output = filegroup_decl_file,
        computed_substitutions = sub,
    )
    return filegroup_decl_file

def _create_archive(ctx, info, deps_files, kernel_uapi_headers, filegroup_decl_file):
    hermetic_tools = hermetic_toolchain.get(ctx)

    filegroup_decl_archive = ctx.actions.declare_file("{name}/{name}{suffix}".format(
        name = ctx.attr.kernel_build.label.name,
        suffix = FILEGROUP_DEF_ARCHIVE_SUFFIX,
    ))
    direct_inputs = deps_files + [
        filegroup_decl_file,
        info.kernel_release,
        kernel_uapi_headers,
        info.config_out_dir,
        info.env_setup_script,
        info.modules_prepare_archive,
        info.module_env_archive,
    ]
    if info.src_protected_modules_list:
        direct_inputs.append(info.src_protected_modules_list)
    transitive_inputs = [
        info.ddk_module_defconfig_fragments,
        info.internal_outs,
    ]
    inputs = depset(
        direct_inputs,
        transitive = transitive_inputs,
    )

    # FILEGROUP_DEF_BUILD_FRAGMENT_NAME stays at root so that
    # kernel_prebuilt_repo can find it.
    command = hermetic_tools.setup + """
        tar cf {archive} --dereference \\
            --transform 's:.*/{fragment}:{fragment}:g' \\
            "$@"
    """.format(
        archive = filegroup_decl_archive.path,
        fragment = FILEGROUP_DEF_BUILD_FRAGMENT_NAME,
    )
    args = ctx.actions.args()
    args.add_all(inputs)
    ctx.actions.run_shell(
        inputs = inputs,
        tools = hermetic_tools.deps,
        outputs = [filegroup_decl_archive],
        command = command,
        arguments = [args],
        progress_message = "Creating archive of kernel_filegroup declaration {}".format(ctx.label),
        mnemonic = "KernelfilegroupDeclaration",
    )
    return filegroup_decl_archive

kernel_filegroup_declaration = rule(
    implementation = _kernel_filegroup_declaration_impl,
    doc = "Given a kernel_build, generates corresponding kernel_filegroup target declaration.",
    attrs = {
        "kernel_build": attr.label(
            mandatory = True,
            providers = [KernelBuildFilegroupDeclInfo],
        ),
        "extra_deps": attr.label_list(
            doc = """Extra files to be placed in the `deps` of the generated `kernel_filegroup`.

                These files are downloaded separately by `kernel_prebuilt_repo`.

                These files are not included in the generated archive.
            """,
            allow_files = True,
        ),
        "images": attr.label(
            doc = "Labels to look up system_dlkm_staging_archive.tar.gz",
            allow_files = True,
        ),
    },
    toolchains = [hermetic_toolchain.type],
)
