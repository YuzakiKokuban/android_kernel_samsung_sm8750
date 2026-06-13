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

"""Repository for kernel prebuilts."""

load(
    ":constants.bzl",
    "FILEGROUP_DEF_ARCHIVE_SUFFIX",
    "FILEGROUP_DEF_BUILD_FRAGMENT_NAME",
)

visibility("//build/kernel/kleaf/...")

_BUILD_NUM_ENV_VAR = "KLEAF_DOWNLOAD_BUILD_NUMBER_MAP"
ARTIFACT_URL_FMT = "https://androidbuildinternal.googleapis.com/android/internal/build/v3/builds/{build_number}/{target}/attempts/latest/artifacts/{filename}/url?redirect=true"

def _parse_env(repository_ctx, var_name, expected_key):
    """
    Given that the environment variable named by `var_name` is set to the following:

    ```
    key=value[,key=value,...]
    ```

    Return a list of values, where key matches `expected_key`. If there
    are multiple matches, the first one is returned. If there is no match,
    return `None`.

    For example:
    ```
    MYVAR="myrepo=x,myrepo2=y" bazel ...
    ```

    Then `_parse_env(repository_ctx, "MYVAR", "myrepo")` returns `"x"`
    """
    for pair in repository_ctx.os.environ.get(var_name, "").split(","):
        pair = pair.strip()
        if not pair:
            continue

        tup = pair.split("=", 1)
        if len(tup) != 2:
            fail("Unrecognized token in {}, must be key=value:\n{}".format(var_name, pair))
        key, value = tup
        if key == expected_key:
            return value
    return None

def _get_build_number(repository_ctx):
    """Gets the value of build number, setting defaults if necessary."""
    build_number = _parse_env(repository_ctx, _BUILD_NUM_ENV_VAR, repository_ctx.attr.apparent_name)
    if not build_number:
        build_number = repository_ctx.attr.build_number
    return build_number

def _get_remote_filename(
        repository_ctx,
        build_number,
        remote_filename_fmt,
        fallback_local_filename = None):
    """Returns remote_filename by using the format remote_filename_fmt with build_number.

    Args:
        repository_ctx: repository_ctx
        build_number: build number
        remote_filename_fmt: the format string
        fallback_local_filename: If set, and build number is not set but used,
            fallback to the given local_filename for the local prebuilt case.
    """
    bazel_target_name = repository_ctx.attr.target
    remote_filename = remote_filename_fmt.format(
        build_number = build_number,
        target = bazel_target_name,
    )
    remote_filename_with_fake_build_number = remote_filename_fmt.format(
        build_number = "__FAKE_BUILD_NUMBER_PLACEHOLDER__",
        target = bazel_target_name,
    )
    if not build_number and remote_filename != remote_filename_with_fake_build_number:
        if fallback_local_filename:
            return fallback_local_filename
        fail("ERROR: No build_number specified for @@{}".format(repository_ctx.attr.name))
    return remote_filename

def _get_local_path(repository_ctx, local_filename):
    """Returns a path object where we store the file named local_filename"""
    return repository_ctx.path(_join(local_filename, _basename(local_filename)))

_true_future = struct(wait = lambda: struct(success = True))
_false_future = struct(wait = lambda: struct(success = False))

# buildifier: disable=unused-variable
def _symlink_local_file(repository_ctx, local_filename, remote_filename_fmt, file_mandatory):
    """Creates symlink in local_filename that points to remote_filename.

    Returns:
        a future object, with `wait()` function that returns a struct containing:

        - Either a boolean, `success`, indicating whether the file exists or not.
          If the file does not exist and `file_mandatory == True`,
          either this function or `wait()` throws build error.
        - Or a string, `fail_later`, an error message for an error that should
          be postponed to the analysis phase when the target is requested.
        """

    local_path = _get_local_path(repository_ctx, local_filename)

    artifact_path = repository_ctx.workspace_root.get_child(repository_ctx.attr.local_artifact_path).get_child(local_filename)
    if artifact_path.exists:
        repository_ctx.symlink(artifact_path, local_path)
        return _true_future
    if file_mandatory:
        fail("{}: {} does not exist".format(repository_ctx.attr.name, artifact_path))
    return _false_future

def _download_remote_file(repository_ctx, local_filename, remote_filename_fmt, file_mandatory):
    """Download `remote_filename` to `local_filename`.

    Returns:
        a future object, with `wait()` function that returns a struct containing:

        - Either a boolean, `success`, indicating whether the file is downloaded
          successfully.
          If the file fails to download and `file_mandatory == True`,
          either this function or `wait()` throws build error.
        - Or a string, `fail_later`, an error message for an error that should
          be postponed to the analysis phase when the target is requested.
        """

    local_path = _get_local_path(repository_ctx, local_filename)
    build_number = _get_build_number(repository_ctx)
    remote_filename = _get_remote_filename(repository_ctx, build_number, remote_filename_fmt)

    # This doesn't have to be the same as the Bazel target name, hence
    # we use a separate variable to signify so. If we have the ci_target_name
    # != bazel_target_name in the future, this needs to be adjusted properly.
    ci_target_name = repository_ctx.attr.target

    artifact_url = repository_ctx.attr.artifact_url_fmt.format(
        build_number = build_number,
        target = ci_target_name,
        filename = remote_filename,
    )

    url_with_fake_build_number = repository_ctx.attr.artifact_url_fmt.format(
        build_number = "__FAKE_BUILD_NUMBER_PLACEHOLDER__",
        target = ci_target_name,
        filename = remote_filename,
    )
    if not build_number and artifact_url != url_with_fake_build_number:
        return struct(wait = lambda: struct(
            fail_later = repr("ERROR: No build_number specified for @@{}".format(repository_ctx.attr.name)),
        ))

    return repository_ctx.download(
        url = artifact_url,
        output = local_path,
        allow_fail = not file_mandatory,
        block = False,
    )

def _get_download_configs(repository_ctx):
    if repository_ctx.attr.local_artifact_path:
        path = repository_ctx.workspace_root.get_child(repository_ctx.attr.local_artifact_path).get_child("download_configs.json")
    else:
        _download_remote_file(
            repository_ctx = repository_ctx,
            local_filename = "download_configs.json",
            remote_filename_fmt = "download_configs.json",
            file_mandatory = True,
        ).wait()
        path = _get_local_path(repository_ctx, "download_configs.json")
    content = repository_ctx.read(path)
    return json.decode(content)

def _kernel_prebuilt_repo_impl(repository_ctx):
    download_configs = _get_download_configs(repository_ctx)

    futures = {}
    for local_filename, config in download_configs.items():
        if repository_ctx.attr.local_artifact_path:
            download = _symlink_local_file
        else:
            download = _download_remote_file

        futures[local_filename] = download(
            repository_ctx = repository_ctx,
            local_filename = local_filename,
            remote_filename_fmt = config["remote_filename_fmt"],
            file_mandatory = config["mandatory"],
        )

    download_statuses = {}
    for local_filename, future in futures.items():
        download_statuses[local_filename] = future.wait()

    for local_filename, download_status in download_statuses.items():
        msg_repr = getattr(download_status, "fail_later", None)
        if msg_repr:
            fmt = """\
load("{fail_bzl}", "fail_rule")

fail_rule(
    name = {local_filename_repr},
    message = {msg_repr},
)
"""
        elif download_status.success:
            fmt = """\
exports_files(
    [{local_filename_repr}],
    visibility = ["//visibility:public"],
)
"""
        else:
            fmt = """\
filegroup(
    name = {local_filename_repr},
    srcs = [],
    visibility = ["//visibility:public"],
)
"""
        content = fmt.format(
            local_filename_repr = repr(_basename(local_filename)),
            fail_bzl = Label("//build/kernel/kleaf:fail.bzl"),
            msg_repr = msg_repr,
        )
        repository_ctx.file(_join(local_filename, "BUILD.bazel"), content)

    _create_top_level_files(repository_ctx, download_configs)

def _create_top_level_files(repository_ctx, download_configs):
    bazel_target_name = repository_ctx.attr.target
    repository_ctx.file("""WORKSPACE.bazel""", """\
workspace({})
""".format(repr(repository_ctx.attr.name)))

    filegroup_decl_archives = []
    for local_filename in download_configs:
        if _basename(local_filename).endswith(FILEGROUP_DEF_ARCHIVE_SUFFIX):
            local_path = repository_ctx.path(_join(local_filename, _basename(local_filename)))
            filegroup_decl_archives.append(local_path)

    if not filegroup_decl_archives:
        return
    if len(filegroup_decl_archives) > 1:
        fail("Multiple files with suffix {}: {}".format(
            FILEGROUP_DEF_ARCHIVE_SUFFIX,
            filegroup_decl_archives,
        ))

    filegroup_decl_archive = filegroup_decl_archives[0]
    repository_ctx.extract(
        # If local_artifact_path is set, filegroup_decl_archive is a symlink.
        # The symlink is under the working directory so we can't set
        # watch_archive = "yes".
        # Use realpath (which may point outside the working directory) and
        # watch_archive = "auto" (the default) achieves optimal effect.
        archive = filegroup_decl_archive.realpath,
        output = repository_ctx.path(bazel_target_name),
    )

    template_path = repository_ctx.path(_join(bazel_target_name, FILEGROUP_DEF_BUILD_FRAGMENT_NAME))
    template_content = repository_ctx.read(template_path)

    repository_ctx.file(repository_ctx.path(_join(bazel_target_name, "BUILD.bazel")), """\
load({kernel_bzl_repr}, "kernel_filegroup")
load({extracted_system_dlkm_staging_archive_bzl_repr}, "extracted_system_dlkm_staging_archive")

_CLANG_KLEAF_PKG = {clang_kleaf_pkg}

{template_content}
""".format(
        kernel_bzl_repr = repr(str(Label("//build/kernel/kleaf:kernel.bzl"))),
        extracted_system_dlkm_staging_archive_bzl_repr = repr(str(Label(":extracted_system_dlkm_staging_archive.bzl"))),
        clang_kleaf_pkg = repr(str(Label("//prebuilts/clang/host/linux-x86/kleaf"))),
        template_content = template_content,
    ))

kernel_prebuilt_repo = repository_rule(
    implementation = _kernel_prebuilt_repo_impl,
    attrs = {
        "local_artifact_path": attr.string(
            doc = """Directory to local artifacts.

                If set, `artifact_url_fmt` is ignored.

                Only the root module may call `declare()` with this attribute set.

                If relative, it is interpreted against workspace root.

                If absolute, this is similar to setting `artifact_url_fmt` to
                `file://<absolute local_artifact_path>/{filename}`, but avoids
                using `download()`. Files are symlinked not copied, and
                `--config=internet` is not necessary.
            """,
        ),
        "build_number": attr.string(
            doc = "the default build number to use if the environment variable is not set.",
        ),
        "apparent_name": attr.string(doc = "apparant repo name", mandatory = True),
        "target": attr.string(doc = "Name of target on the download location, e.g. `kernel_aarch64`"),
        "artifact_url_fmt": attr.string(
            doc = """API endpoint for Android CI artifacts.

            The format may include anchors for the following properties:
                * {build_number}
                * {target}
                * {filename}

            Its default value is the API endpoint for http://ci.android.com.
            """,
            default = ARTIFACT_URL_FMT,
        ),
    },
    environ = [
        _BUILD_NUM_ENV_VAR,
    ],
)

# Avoid dependency to paths, since we do not necessary have skylib loaded yet.
# TODO(b/276493276): Use paths once we migrate to bzlmod completely.
def _basename(s):
    return s.split("/")[-1]

def _join(path, *others):
    ret = path

    for other in others:
        if not ret.endswith("/"):
            ret += "/"
        ret += other

    return ret
