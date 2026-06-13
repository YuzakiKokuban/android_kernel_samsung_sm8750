# Copyright (C) 2023 The Android Open Source Project
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

"""Archive [`ddk_headers`](#ddk_headers) for distribution."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("//build/kernel/kleaf/impl:hermetic_toolchain.bzl", "hermetic_toolchain")
load(":ddk/ddk_headers.bzl", "DdkHeadersInfo")

visibility("//build/kernel/kleaf/...")

def _drop_package(x, package):
    if type(x) == "File":
        x = x.path
    return paths.relativize(x, package)

def _create_build_frag_for_src(ctx, src):
    """Create a single BUILD.bazel fragment for an item in srcs.

    Args:
        ctx: ctx
        src: an item in ctx.attr.srcs

    Returns:
        The created BUILD.bazel fragment
    """
    if src.label.workspace_name != ctx.label.workspace_name:
        fail("ddk_headers_archive {this_label} can only include srcs within the same workspace, but got {src}".format(
            this_label = ctx.label,
            src = src.label,
        ))

    src_package = str(src.label.package)
    drop_src_package = lambda x: _drop_package(x, src_package)

    build_file = ctx.actions.declare_file("{name}/{src_package}/{src_name}/gen_BUILD.bazel".format(
        name = ctx.attr.name,
        src_package = src.label.package,
        src_name = src.label.name,
    ))
    args = ctx.actions.args()
    args.set_param_file_format("multiline")
    args.use_param_file("--flagfile=%s")
    args.add_all(
        "--hdrs",
        src[DdkHeadersInfo].files,
        uniquify = True,
        map_each = drop_src_package,
        allow_closure = True,
    )
    args.add_all(
        "--includes",
        src[DdkHeadersInfo].includes,
        uniquify = True,
        map_each = drop_src_package,
        allow_closure = True,
    )
    args.add_all(
        "--linux-includes",
        src[DdkHeadersInfo].linux_includes,
        uniquify = True,
        map_each = drop_src_package,
        allow_closure = True,
    )
    args.add("--out", build_file)
    args.add("--name", src.label.name)
    ctx.actions.run(
        inputs = [],
        outputs = [build_file],
        executable = ctx.executable._gen_ddk_headers_archive_build_file,
        arguments = [args],
        mnemonic = "DdkHeadersArchiveSrcBuildFile",
        progress_message = "Generating BUILD.bazel for {} {}".format(src.label.name, ctx.label),
    )
    return build_file

def _create_build_file_for_packages(ctx):
    """Create a BUILD.bazel for each package in srcs.

    Mutliple BUILD.bazel fragments for srcs in the same package are concantenated.

    Args:
        ctx: ctx

    Returns:
        A dictionary where keys are unique packages of `srcs`, and values is a struct with
        these fields:
        - build_file: a single BUILD.bazel file fragment.
    """
    hermetic_tools = hermetic_toolchain.get(ctx)
    build_file_fragments = {}
    srcs_by_package = {}
    for src in ctx.attr.srcs:
        if src.label.package not in build_file_fragments:
            build_file_fragments[src.label.package] = []
        if src.label.package not in srcs_by_package:
            srcs_by_package[src.label.package] = []
        build_file_fragments[src.label.package].append(_create_build_frag_for_src(ctx, src))
        srcs_by_package[src.label.package].append(src)

    package_files = {}
    for package, package_build_file_fragments in build_file_fragments.items():
        common_header_content = """\
# Generated file. DO NOT EDIT.

\"""Generated package of DDK headers.

ddk_headers_archive: {this_label}
Original package: //{package}
\"""

# Extension label is relative to kernel_prebuilt_repo.bzl
load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")

""".format(
            this_label = ctx.label,
            package = package,
        )
        common_header = ctx.actions.declare_file("{name}/{package}/common_header.txt".format(
            name = ctx.attr.name,
            package = package,
        ))
        ctx.actions.write(output = common_header, content = common_header_content)
        package_build_file = ctx.actions.declare_file("{name}/{package}/gen_BUILD.bazel".format(
            name = ctx.attr.name,
            package = package,
        ))

        args = ctx.actions.args()
        args.add(common_header)
        args.add_all(package_build_file_fragments)

        cmd = hermetic_tools.setup + """
            cat "$@" >> {}
        """.format(package_build_file.path)

        ctx.actions.run_shell(
            inputs = package_build_file_fragments + [common_header],
            outputs = [package_build_file],
            tools = hermetic_tools.deps,
            command = cmd,
            arguments = [args],
            mnemonic = "DdkHeadersArchiveBuildFile",
            progress_message = "Generating BUILD.bazel fragment for {} {}".format(ctx.label, package),
        )

        package_files[package] = struct(
            build_file = package_build_file,
        )

    return package_files

def _create_archive(ctx, package_files):
    hermetic_tools = hermetic_toolchain.get(ctx)

    all_srcs_files = depset(transitive = [src[DdkHeadersInfo].files for src in ctx.attr.srcs])

    out = ctx.actions.declare_file("{name}/{name}.tar.gz".format(
        name = ctx.attr.name,
    ))

    extra_files_in_archive = []
    inputs = []

    cmd = hermetic_tools.setup
    for package, files_for_package in package_files.items():
        package_build_file_dest = "{package}/BUILD.bazel".format(package = package)
        extra_files_in_archive.append(package_build_file_dest)
        inputs.append(files_for_package.build_file)
        cmd += """
            mkdir -p {package}
            cp {package_build_file} {package_build_file_dest}
        """.format(
            package = package,
            package_build_file = files_for_package.build_file.path,
            package_build_file_dest = package_build_file_dest,
        )

    cmd += """
        tar czf {out} --dereference -T "$@"
    """.format(
        out = out.path,
        package = ctx.label.package,
    )
    args = ctx.actions.args()
    args.set_param_file_format("multiline")
    args.use_param_file("%s", use_always = True)
    args.add_all(all_srcs_files)
    args.add_all(extra_files_in_archive)
    ctx.actions.run_shell(
        inputs = depset(inputs, transitive = [all_srcs_files]),
        outputs = [out],
        arguments = [args],
        tools = hermetic_tools.deps,
        command = cmd,
        mnemonic = "DdkHeadersArchive",
        progress_message = "Creating archive {}".format(ctx.label),
    )
    return out

def _ddk_headers_archive_impl(ctx):
    package_build_files = _create_build_file_for_packages(ctx)
    archive = _create_archive(ctx, package_build_files)
    return DefaultInfo(files = depset([archive]))

ddk_headers_archive = rule(
    doc = """An archive of [`ddk_headers`](#ddk_headers).

        The archive includes all headers, as well as a `BUILD` file that is
        semantically identical to the original `ddk_headers` definition.
    """,
    implementation = _ddk_headers_archive_impl,
    attrs = {
        "srcs": attr.label_list(
            providers = [DdkHeadersInfo],
        ),
        "_gen_ddk_headers_archive_build_file": attr.label(
            default = "//build/kernel/kleaf/impl:ddk/gen_ddk_headers_archive_build_file",
            cfg = "exec",
            executable = True,
        ),
    },
    toolchains = [hermetic_toolchain.type],
)
