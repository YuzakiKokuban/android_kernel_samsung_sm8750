"""Rule to support Bazel in copying its output files to the dist dir outside of
the standard Bazel output user root.
"""

load("@bazel_skylib//rules:copy_file.bzl", "copy_file")
load("//build/bazel_common_rules/exec/impl:embedded_exec.bzl", "embedded_exec")

def _label_list_to_manifest(lst):
    """Convert the outputs of a label list to manifest content."""
    all_dist_files = []
    for f in lst:
        all_dist_files += f[DefaultInfo].files.to_list()
    return all_dist_files, "\n".join([dist_file.short_path for dist_file in all_dist_files])

def _generate_dist_manifest_impl(ctx):
    # Create a manifest of dist files to differentiate them from other runfiles.
    dist_manifest = ctx.actions.declare_file(ctx.attr.name + "_dist_manifest.txt")
    all_dist_files, dist_manifest_content = _label_list_to_manifest(ctx.attr.data)
    ctx.actions.write(
        output = dist_manifest,
        content = dist_manifest_content,
    )

    dist_archives_manifest = ctx.actions.declare_file(ctx.attr.name + "_dist_archives_manifest.txt")
    all_dist_archives, dist_archives_manifest_content = _label_list_to_manifest(ctx.attr.archives)
    ctx.actions.write(
        output = dist_archives_manifest,
        content = dist_archives_manifest_content,
    )

    # Create the runfiles object.
    runfiles = ctx.runfiles(files = all_dist_files + all_dist_archives + [
        dist_manifest,
        dist_archives_manifest,
    ])

    return [DefaultInfo(runfiles = runfiles)]

_generate_dist_manifest = rule(
    implementation = _generate_dist_manifest_impl,
    doc = """Generate a manifest of files to be dist to a directory.""",
    attrs = {
        "data": attr.label_list(
            allow_files = True,
            doc = """Files or targets to copy to the dist dir.

In the case of targets, the rule copies the list of `files` from the target's DefaultInfo provider.
""",
        ),
        "archives": attr.label_list(
            allow_files = [".tar.gz", ".tar"],
            doc = """Files or targets to be extracted to the dist dir.

In the case of targets, the rule copies the list of `files` from the target's DefaultInfo provider.
""",
        ),
    },
)

def copy_to_dist_dir(
        name,
        data = None,
        archives = None,
        flat = None,
        prefix = None,
        strip_components = 0,
        archive_prefix = None,
        dist_dir = None,
        wipe_dist_dir = None,
        allow_duplicate_filenames = None,
        mode_overrides = None,
        log = None,
        testonly = False,
        **kwargs):
    """A dist rule to copy files out of Bazel's output directory into a custom location.

    Example:
    ```
    bazel run //path/to/my:dist_target -- --dist_dir=/tmp/dist
    ```

    Run `bazel run //path/to/my:dist_target -- --help` for explanations of
    options.

    Args:
        name: name of this rule
        data: A list of labels, whose outputs are copied to `--dist_dir`.
        archives: A list of labels, whose outputs are treated as tarballs and
          extracted to `--dist_dir`.
        flat: If true, `--flat` is provided to the script by default. Flatten the distribution
          directory.
        strip_components: If specified, `--strip_components <prefix>` is provided to the script. Strip
          leading components from the existing copied file paths before applying --prefix
          (if specified).
        prefix: If specified, `--prefix <prefix>` is provided to the script by default. Path prefix
          to apply within dist_dir for copied files.
        archive_prefix: If specified, `--archive_prefix <prefix>` is provided to the script by
          default. Path prefix to apply within dist_dir for extracted archives.
        dist_dir: If specified, `--dist_dir <dist_dir>` is provided to the script by default.

          In particular, if this is a relative path, it is interpreted as a relative path
          under workspace root when the target is executed with `bazel run`.

          By default, the script will overwrite any files of the same name in `dist_dir`, but preserve
          any other contents there. This can be overridden with `wipe_dist_dir`.

          See details by running the target with `--help`.
        wipe_dist_dir: If true, and `dist_dir` already exists, `dist_dir` will be removed prior to
          copying.
        allow_duplicate_filenames: If true, duplicate filenames from different sources will be allowed to
          be copied to the same `dist_dir` (with subsequent sources overwriting previous sources).

          With this option enabled, order matters. The final source of the file listed in `data` will be the
          final version copied.

          Use of this option is discouraged. Preferably, the input `data` targets would not include labels
          which produce a duplicate filename. This option is available as a last resort.
        mode_overrides: Map of glob patterns to octal permissions. If the file path being copied matches the
          glob pattern, the corresponding permissions will be set in `dist_dir`. Full file paths are used for
          matching even if `flat = True`. Paths are relative to the workspace root.

          Order matters; the overrides will be stepped through in the order given for each file. To prevent
          buildifier from sorting the list, use the `# do not sort` magic line. For example:
          ```
          mode_overrides = {
              # do not sort
              "**/*.sh": 755,
              "**/hello_world": 755,
              "restricted_dir/**": 600,
              "common/kernel_aarch64/vmlinux": 755,
              "**/*": 644,
          },
          ```

          If no `mode_overrides` are provided, the default Bazel output permissions are preserved.
        log: If specified, `--log <log>` is provided to the script by default. This sets the
          default log level of the script.

        testonly: If enabled, testonly will also be set on the internal targets
          for Bazel analysis to succeed due to the nature of testonly enforcement
          on reverse dependencies.

          See `dist.py` for allowed values and the default value.
        **kwargs: Additional attributes to the internal rule, e.g.
          [`visibility`](https://docs.bazel.build/versions/main/visibility.html).

          These additional attributes are only passed to the underlying embedded_exec rule.
    """

    default_args = []
    if flat:
        default_args.append("--flat")
    if strip_components != None:
        if strip_components < 0:
            fail("strip_components must greater than 0, but is %s" % strip_components)
        default_args += ["--strip_components", str(strip_components)]
    if prefix != None:
        default_args += ["--prefix", prefix]
    if archive_prefix != None:
        default_args += ["--archive_prefix", archive_prefix]
    if dist_dir != None:
        default_args += ["--dist_dir", dist_dir]
    if wipe_dist_dir:
        default_args.append("--wipe_dist_dir")
    if allow_duplicate_filenames:
        default_args.append("--allow_duplicate_filenames")
    if mode_overrides != None:
        for (pattern, mode) in mode_overrides.items():
            default_args += ["--mode_override", pattern, str(mode)]
    if log != None:
        default_args += ["--log", log]

    _generate_dist_manifest(
        name = name + "_dist_manifest",
        data = data,
        archives = archives,
        testonly = testonly,
    )

    copy_file(
        name = name + "_dist_tool",
        src = "//build/bazel_common_rules/dist:dist.py",
        out = name + "_dist.py",
    )

    # The dist py_binary tool must be colocated in the same package as the
    # dist_manifest so that the runfiles directory is the same, and that the
    # dist_manifest is in the data runfiles of the dist tool.
    native.py_binary(
        name = name + "_internal",
        main = name + "_dist.py",
        srcs = [name + "_dist.py"],
        python_version = "PY3",
        visibility = ["//visibility:public"],
        data = [name + "_dist_manifest"],
        testonly = testonly,
        args = default_args,
    )

    embedded_exec(
        name = name,
        actual = name + "_internal",
        testonly = testonly,
        **kwargs
    )
