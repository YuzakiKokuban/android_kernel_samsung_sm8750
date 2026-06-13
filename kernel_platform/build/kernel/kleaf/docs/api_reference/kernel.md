<!-- Generated with Stardoc: http://skydoc.bazel.build -->

All public rules and macros to build the kernel.

<a id="android_filegroup"></a>

## android_filegroup

<pre>
android_filegroup(<a href="#android_filegroup-name">name</a>, <a href="#android_filegroup-srcs">srcs</a>, <a href="#android_filegroup-cpu">cpu</a>)
</pre>

Like filegroup, but applies transitions to Android.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="android_filegroup-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="android_filegroup-srcs"></a>srcs |  Sources of the filegroup.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="android_filegroup-cpu"></a>cpu |  Architecture.   | String | optional |  `"arm64"`  |


<a id="checkpatch"></a>

## checkpatch

<pre>
checkpatch(<a href="#checkpatch-name">name</a>, <a href="#checkpatch-checkpatch_pl">checkpatch_pl</a>, <a href="#checkpatch-ignorelist">ignorelist</a>)
</pre>

Run `checkpatch.sh` at the root of this package.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="checkpatch-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="checkpatch-checkpatch_pl"></a>checkpatch_pl |  Label to `checkpatch.pl`.<br><br>This is usually `//<common_package>:scripts/checkpatch.pl`.   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |
| <a id="checkpatch-ignorelist"></a>ignorelist |  checkpatch ignorelist   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `"@kleaf//build/kernel/static_analysis:checkpatch_ignorelist"`  |


<a id="ddk_headers"></a>

## ddk_headers

<pre>
ddk_headers(<a href="#ddk_headers-name">name</a>, <a href="#ddk_headers-hdrs">hdrs</a>, <a href="#ddk_headers-defconfigs">defconfigs</a>, <a href="#ddk_headers-includes">includes</a>, <a href="#ddk_headers-kconfigs">kconfigs</a>, <a href="#ddk_headers-linux_includes">linux_includes</a>, <a href="#ddk_headers-textual_hdrs">textual_hdrs</a>)
</pre>

A rule that exports a list of header files to be used in DDK.

Example:

```
ddk_headers(
   name = "headers",
   hdrs = ["include/module.h"],
   textual_hdrs = ["template.c"],
   includes = ["include"],
)
```

`ddk_headers` can be chained; that is, a `ddk_headers` target can re-export
another `ddk_headers` target. For example:

```
ddk_headers(
   name = "foo",
   hdrs = ["include_foo/foo.h"],
   includes = ["include_foo"],
)
ddk_headers(
   name = "headers",
   hdrs = [":foo", "include/module.h"],
   includes = ["include"],
)
```

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="ddk_headers-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="ddk_headers-hdrs"></a>hdrs |  One of the following:<br><br>- Local header files to be exported. You may also need to set the `includes` attribute. - Other `ddk_headers` targets to be re-exported.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="ddk_headers-defconfigs"></a>defconfigs |  `defconfig` files.<br><br>Items must already be declared in `kconfigs`. An item not declared in Kconfig and inherited Kconfig files is silently dropped.<br><br>An item declared in `kconfigs` without a specific value in `defconfigs` uses default value specified in `kconfigs`.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="ddk_headers-includes"></a>includes |  A list of directories, relative to the current package, that are re-exported as include directories.<br><br>[`ddk_module`](#ddk_module) with `deps` including this target automatically adds the given include directory in the generated `Kbuild` files.<br><br>You still need to add the actual header files to `hdrs`.   | List of strings | optional |  `[]`  |
| <a id="ddk_headers-kconfigs"></a>kconfigs |  Kconfig files.<br><br>See [`Documentation/kbuild/kconfig-language.rst`](https://www.kernel.org/doc/html/latest/kbuild/kconfig.html) for its format.<br><br>Kconfig is optional for a `ddk_module`. The final Kconfig known by this module consists of the following:<br><br>- Kconfig from `kernel_build` - Kconfig from dependent modules, if any - Kconfig of this module, if any   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="ddk_headers-linux_includes"></a>linux_includes |  Like `includes` but specified in `LINUXINCLUDES` instead.<br><br>Setting this attribute allows you to override headers from `${KERNEL_DIR}`. See "Order of includes" in [`ddk_module`](#ddk_module) for details.<br><br>Like `includes`, `linux_includes` is applied to dependent `ddk_module`s.   | List of strings | optional |  `[]`  |
| <a id="ddk_headers-textual_hdrs"></a>textual_hdrs |  The list of header files to be textually included by sources.<br><br>This is the location for declaring header files that cannot be compiled on their own; that is, they always need to be textually included by other source files to build valid code.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |


<a id="ddk_headers_archive"></a>

## ddk_headers_archive

<pre>
ddk_headers_archive(<a href="#ddk_headers_archive-name">name</a>, <a href="#ddk_headers_archive-srcs">srcs</a>)
</pre>

An archive of [`ddk_headers`](#ddk_headers).

The archive includes all headers, as well as a `BUILD` file that is
semantically identical to the original `ddk_headers` definition.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="ddk_headers_archive-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="ddk_headers_archive-srcs"></a>srcs |  -   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |


<a id="ddk_uapi_headers"></a>

## ddk_uapi_headers

<pre>
ddk_uapi_headers(<a href="#ddk_uapi_headers-name">name</a>, <a href="#ddk_uapi_headers-srcs">srcs</a>, <a href="#ddk_uapi_headers-out">out</a>, <a href="#ddk_uapi_headers-kernel_build">kernel_build</a>)
</pre>

A rule that generates a sanitized UAPI header tarball.

Example:

```
ddk_uapi_headers(
   name = "my_headers",
   srcs = glob(["include/uapi/**/*.h"]),
   out = "my_headers.tar.gz",
   kernel_build = "//common:kernel_aarch64",
)
```

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="ddk_uapi_headers-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="ddk_uapi_headers-srcs"></a>srcs |  UAPI headers files which can be sanitized by "make headers_install"   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="ddk_uapi_headers-out"></a>out |  Name of the output tarball   | String | required |  |
| <a id="ddk_uapi_headers-kernel_build"></a>kernel_build |  [`kernel_build`](#kernel_build).   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |


<a id="dependency_graph_drawer"></a>

## dependency_graph_drawer

<pre>
dependency_graph_drawer(<a href="#dependency_graph_drawer-name">name</a>, <a href="#dependency_graph_drawer-adjacency_list">adjacency_list</a>, <a href="#dependency_graph_drawer-colorful">colorful</a>)
</pre>

A rule that creates a [Graphviz](https://graphviz.org/) diagram file.

* Inputs:
  A json file describing a graph as an adjacency list.

* Outputs:
  A `dependency_graph.dot` file containing the diagram representation.

* NOTE: For further simplification of the resulting diagram
  [tred utility](https://graphviz.org/docs/cli/tred/) from the CLI can
  be used as in the following example:
  ```
  tred dependency_graph.dot > simplified.dot
  ```

* Example:
  ```
  dependency_graph_drawer(
      name = "db845c_dependency_graph",
      adjacency_list = ":db845c_dependencies",
  )
  ```

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="dependency_graph_drawer-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="dependency_graph_drawer-adjacency_list"></a>adjacency_list |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |
| <a id="dependency_graph_drawer-colorful"></a>colorful |  Whether outgoing edges from every node are colored.   | Boolean | optional |  `False`  |


<a id="dependency_graph_extractor"></a>

## dependency_graph_extractor

<pre>
dependency_graph_extractor(<a href="#dependency_graph_extractor-name">name</a>, <a href="#dependency_graph_extractor-enable_add_vmlinux">enable_add_vmlinux</a>, <a href="#dependency_graph_extractor-exclude_base_kernel_modules">exclude_base_kernel_modules</a>, <a href="#dependency_graph_extractor-kernel_build">kernel_build</a>,
                           <a href="#dependency_graph_extractor-kernel_modules">kernel_modules</a>)
</pre>

A rule that extracts a symbol dependency graph from a kernel build and modules.

It works by matching undefined symbols from one module with exported symbols from other.

* Inputs:
  It receives a Kernel build target, where the analysis will run (vmlinux + in-tree modules),
   aditionally a list of external modules can be accepted.

* Outputs:
  A `dependency_graph.json` file describing the graph as an adjacency list.

* Example:
  ```
  dependency_graph_extractor(
      name = "db845c_dependencies",
      kernel_build = ":db845c",
      # kernel_modules = [],
  )
  ```

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="dependency_graph_extractor-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="dependency_graph_extractor-enable_add_vmlinux"></a>enable_add_vmlinux |  If `True` enables `kernel_build_add_vmlinux` transition.   | Boolean | optional |  `True`  |
| <a id="dependency_graph_extractor-exclude_base_kernel_modules"></a>exclude_base_kernel_modules |  Whether the analysis should made for only external modules.   | Boolean | optional |  `False`  |
| <a id="dependency_graph_extractor-kernel_build"></a>kernel_build |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="dependency_graph_extractor-kernel_modules"></a>kernel_modules |  -   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |


<a id="extract_symbols"></a>

## extract_symbols

<pre>
extract_symbols(<a href="#extract_symbols-name">name</a>, <a href="#extract_symbols-src">src</a>, <a href="#extract_symbols-enable_add_vmlinux">enable_add_vmlinux</a>, <a href="#extract_symbols-kernel_build">kernel_build</a>, <a href="#extract_symbols-kernel_modules">kernel_modules</a>,
                <a href="#extract_symbols-kernel_modules_exclude_list">kernel_modules_exclude_list</a>, <a href="#extract_symbols-kmi_symbol_list_add_only">kmi_symbol_list_add_only</a>, <a href="#extract_symbols-module_grouping">module_grouping</a>)
</pre>



**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="extract_symbols-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="extract_symbols-src"></a>src |  Source `abi_gki_*` file. Used when `kmi_symbol_list_add_only`.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="extract_symbols-enable_add_vmlinux"></a>enable_add_vmlinux |  If `True` enables `kernel_build_add_vmlinux` transition.   | Boolean | optional |  `True`  |
| <a id="extract_symbols-kernel_build"></a>kernel_build |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="extract_symbols-kernel_modules"></a>kernel_modules |  -   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="extract_symbols-kernel_modules_exclude_list"></a>kernel_modules_exclude_list |  Base name list of kernel modules to exclude from.   | List of strings | optional |  `[]`  |
| <a id="extract_symbols-kmi_symbol_list_add_only"></a>kmi_symbol_list_add_only |  -   | Boolean | optional |  `False`  |
| <a id="extract_symbols-module_grouping"></a>module_grouping |  -   | Boolean | optional |  `True`  |


<a id="gki_artifacts"></a>

## gki_artifacts

<pre>
gki_artifacts(<a href="#gki_artifacts-name">name</a>, <a href="#gki_artifacts-arch">arch</a>, <a href="#gki_artifacts-boot_img_sizes">boot_img_sizes</a>, <a href="#gki_artifacts-gki_kernel_cmdline">gki_kernel_cmdline</a>, <a href="#gki_artifacts-kernel_build">kernel_build</a>, <a href="#gki_artifacts-mkbootimg">mkbootimg</a>)
</pre>

`BUILD_GKI_ARTIFACTS`. Build boot images and optionally `boot-img.tar.gz` as default outputs.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="gki_artifacts-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="gki_artifacts-arch"></a>arch |  `ARCH`.   | String | required |  |
| <a id="gki_artifacts-boot_img_sizes"></a>boot_img_sizes |  A dictionary, with key is the compression algorithm, and value is the size of the boot image.<br><br>For example: <pre><code>{&#10;    "":    str(64 * 1024 * 1024), # For Image and boot.img&#10;    "lz4": str(64 * 1024 * 1024), # For Image.lz4 and boot-lz4.img&#10;}</code></pre>   | <a href="https://bazel.build/rules/lib/dict">Dictionary: String -> String</a> | optional |  `{}`  |
| <a id="gki_artifacts-gki_kernel_cmdline"></a>gki_kernel_cmdline |  `GKI_KERNEL_CMDLINE`.   | String | optional |  `""`  |
| <a id="gki_artifacts-kernel_build"></a>kernel_build |  The [`kernel_build`](kernel.md#kernel_build) that provides all `Image` and `Image.*`.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="gki_artifacts-mkbootimg"></a>mkbootimg |  path to the `mkbootimg.py` script; `MKBOOTIMG_PATH`.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `"@kleaf//tools/mkbootimg:mkbootimg.py"`  |


<a id="gki_artifacts_prebuilts"></a>

## gki_artifacts_prebuilts

<pre>
gki_artifacts_prebuilts(<a href="#gki_artifacts_prebuilts-name">name</a>, <a href="#gki_artifacts_prebuilts-srcs">srcs</a>, <a href="#gki_artifacts_prebuilts-outs">outs</a>)
</pre>



**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="gki_artifacts_prebuilts-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="gki_artifacts_prebuilts-srcs"></a>srcs |  -   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="gki_artifacts_prebuilts-outs"></a>outs |  -   | List of strings | optional |  `[]`  |


<a id="kernel_build_config"></a>

## kernel_build_config

<pre>
kernel_build_config(<a href="#kernel_build_config-name">name</a>, <a href="#kernel_build_config-deps">deps</a>, <a href="#kernel_build_config-srcs">srcs</a>)
</pre>

Create a build.config file by concatenating build config fragments.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="kernel_build_config-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="kernel_build_config-deps"></a>deps |  Additional build config dependencies.<br><br>These include build configs that are indirectly `source`d by items in `srcs`. Unlike `srcs`, they are not be emitted in the output.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="kernel_build_config-srcs"></a>srcs |  List of build config fragments.<br><br>Order matters. To prevent buildifier from sorting the list, use the `# do not sort` magic line. For example:<br><br><pre><code>kernel_build_config(&#10;    name = "build.config.foo.mixed",&#10;    srcs = [&#10;        # do not sort&#10;        "build.config.mixed",&#10;        "build.config.foo",&#10;    ],&#10;)</code></pre>   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |


<a id="kernel_compile_commands"></a>

## kernel_compile_commands

<pre>
kernel_compile_commands(<a href="#kernel_compile_commands-name">name</a>, <a href="#kernel_compile_commands-kernel_build">kernel_build</a>)
</pre>

Define an executable that creates `compile_commands.json` from a `kernel_build`.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="kernel_compile_commands-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="kernel_compile_commands-kernel_build"></a>kernel_build |  The `kernel_build` rule to extract from.   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |


<a id="kernel_filegroup"></a>

## kernel_filegroup

<pre>
kernel_filegroup(<a href="#kernel_filegroup-name">name</a>, <a href="#kernel_filegroup-deps">deps</a>, <a href="#kernel_filegroup-srcs">srcs</a>, <a href="#kernel_filegroup-outs">outs</a>, <a href="#kernel_filegroup-all_module_names">all_module_names</a>, <a href="#kernel_filegroup-collect_unstripped_modules">collect_unstripped_modules</a>,
                 <a href="#kernel_filegroup-config_out_dir">config_out_dir</a>, <a href="#kernel_filegroup-config_out_dir_files">config_out_dir_files</a>, <a href="#kernel_filegroup-ddk_module_defconfig_fragments">ddk_module_defconfig_fragments</a>, <a href="#kernel_filegroup-debug">debug</a>,
                 <a href="#kernel_filegroup-env_setup_script">env_setup_script</a>, <a href="#kernel_filegroup-exec_platform">exec_platform</a>, <a href="#kernel_filegroup-gki_artifacts">gki_artifacts</a>, <a href="#kernel_filegroup-images">images</a>, <a href="#kernel_filegroup-internal_outs">internal_outs</a>, <a href="#kernel_filegroup-kasan">kasan</a>,
                 <a href="#kernel_filegroup-kasan_generic">kasan_generic</a>, <a href="#kernel_filegroup-kasan_sw_tags">kasan_sw_tags</a>, <a href="#kernel_filegroup-kcsan">kcsan</a>, <a href="#kernel_filegroup-kernel_release">kernel_release</a>, <a href="#kernel_filegroup-kernel_uapi_headers">kernel_uapi_headers</a>, <a href="#kernel_filegroup-lto">lto</a>,
                 <a href="#kernel_filegroup-module_env_archive">module_env_archive</a>, <a href="#kernel_filegroup-modules_prepare_archive">modules_prepare_archive</a>, <a href="#kernel_filegroup-protected_modules_list">protected_modules_list</a>, <a href="#kernel_filegroup-strip_modules">strip_modules</a>,
                 <a href="#kernel_filegroup-target_platform">target_platform</a>, <a href="#kernel_filegroup-trim_nonlisted_kmi">trim_nonlisted_kmi</a>)
</pre>

**EXPERIMENTAL.** The API of `kernel_filegroup` rapidly changes and
is not backwards compatible with older builds. The usage of `kernel_filegroup`
is limited to the implementation detail of Kleaf (in particular,
[`define_common_kernels`](#define_common_kernels)). Do not use
`kernel_filegroup` directly. See `download_prebuilt.md` for details.

Specify a list of kernel prebuilts.

This is similar to [`filegroup`](https://docs.bazel.build/versions/main/be/general.html#filegroup)
that gives a convenient name to a collection of targets, which can be referenced from other rules.

It can be used in the `base_kernel` attribute of a [`kernel_build`](#kernel_build).

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="kernel_filegroup-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="kernel_filegroup-deps"></a>deps |  A list of additional labels that participates in implementing the providers.<br><br>This usually contains a list of prebuilts.<br><br>Unlike srcs, these labels are NOT added to the [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html)   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="kernel_filegroup-srcs"></a>srcs |  The list of labels that are members of this file group.<br><br>This usually contains a list of prebuilts, e.g. `vmlinux`, `Image.lz4`, `kernel-headers.tar.gz`, etc.<br><br>Not to be confused with [`kernel_srcs`](#kernel_filegroup-kernel_srcs).   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="kernel_filegroup-outs"></a>outs |  Keys: from `_kernel_build.outs`. Values: path under `$OUT_DIR`.   | <a href="https://bazel.build/rules/lib/dict">Dictionary: Label -> String</a> | optional |  `{}`  |
| <a id="kernel_filegroup-all_module_names"></a>all_module_names |  `module_outs` and `module_implicit_outs` of the original [`kernel_build`](#kernel_build) target.   | List of strings | optional |  `[]`  |
| <a id="kernel_filegroup-collect_unstripped_modules"></a>collect_unstripped_modules |  See [`kernel_build.collect_unstripped_modules`](#kernel_build-collect_unstripped_modules).<br><br>Unlike `kernel_build`, this has default value `True` because [`kernel_abi`](#kernel_abi) sets [`define_abi_targets`](#kernel_abi-define_abi_targets) to `True` by default, which in turn sets `collect_unstripped_modules` to `True` by default.   | Boolean | optional |  `True`  |
| <a id="kernel_filegroup-config_out_dir"></a>config_out_dir |  Directory to support `kernel_config`   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-config_out_dir_files"></a>config_out_dir_files |  Files in `config_out_dir`   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="kernel_filegroup-ddk_module_defconfig_fragments"></a>ddk_module_defconfig_fragments |  Additional defconfig fragments for dependant DDK modules.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |
| <a id="kernel_filegroup-debug"></a>debug |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `"@kleaf//build/kernel/kleaf:debug"`  |
| <a id="kernel_filegroup-env_setup_script"></a>env_setup_script |  Setup script from `kernel_env`   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-exec_platform"></a>exec_platform |  Execution platform, where the build is executed.<br><br>See https://bazel.build/extending/platforms.   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |
| <a id="kernel_filegroup-gki_artifacts"></a>gki_artifacts |  A list of files that were built from the [`gki_artifacts`](#gki_artifacts) target. The `gki-info.txt` file should be part of that list.<br><br>If `kernel_release` is set, this attribute has no effect.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-images"></a>images |  A label providing files similar to a [`kernel_images`](#kernel_images) target.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-internal_outs"></a>internal_outs |  Keys: from `_kernel_build.internal_outs`. Values: path under `$OUT_DIR`.   | <a href="https://bazel.build/rules/lib/dict">Dictionary: Label -> String</a> | optional |  `{}`  |
| <a id="kernel_filegroup-kasan"></a>kasan |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `"@kleaf//build/kernel/kleaf:kasan"`  |
| <a id="kernel_filegroup-kasan_generic"></a>kasan_generic |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `"@kleaf//build/kernel/kleaf:kasan_generic"`  |
| <a id="kernel_filegroup-kasan_sw_tags"></a>kasan_sw_tags |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `"@kleaf//build/kernel/kleaf:kasan_sw_tags"`  |
| <a id="kernel_filegroup-kcsan"></a>kcsan |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `"@kleaf//build/kernel/kleaf:kcsan"`  |
| <a id="kernel_filegroup-kernel_release"></a>kernel_release |  A file providing the kernel release string. This is preferred over `gki_artifacts`.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-kernel_uapi_headers"></a>kernel_uapi_headers |  The label pointing to `kernel-uapi-headers.tar.gz`.<br><br>This attribute should be set to the `kernel-uapi-headers.tar.gz` artifact built by the [`kernel_build`](#kernel_build) macro if the `kernel_filegroup` rule were a `kernel_build`.<br><br>Setting this attribute allows [`merged_kernel_uapi_headers`](#merged_kernel_uapi_headers) to work properly when this `kernel_filegroup` is set to the `base_kernel`.<br><br>For example: <pre><code>kernel_filegroup(&#10;    name = "kernel_aarch64_prebuilts",&#10;    srcs = [&#10;        "vmlinux",&#10;        # ...&#10;    ],&#10;    kernel_uapi_headers = "kernel-uapi-headers.tar.gz",&#10;)&#10;&#10;kernel_build(&#10;    name = "tuna",&#10;    base_kernel = ":kernel_aarch64_prebuilts",&#10;    # ...&#10;)&#10;&#10;merged_kernel_uapi_headers(&#10;    name = "tuna_merged_kernel_uapi_headers",&#10;    kernel_build = "tuna",&#10;    # ...&#10;)</code></pre>   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-lto"></a>lto |  -   | String | optional |  `"default"`  |
| <a id="kernel_filegroup-module_env_archive"></a>module_env_archive |  Archive from `kernel_build.pack_module_env` that contains necessary files to build external modules.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-modules_prepare_archive"></a>modules_prepare_archive |  Archive from `modules_prepare`   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-protected_modules_list"></a>protected_modules_list |  -   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_filegroup-strip_modules"></a>strip_modules |  See [`kernel_build.strip_modules`](#kernel_build-strip_modules).   | Boolean | optional |  `False`  |
| <a id="kernel_filegroup-target_platform"></a>target_platform |  Target platform that describes characteristics of the target device.<br><br>See https://bazel.build/extending/platforms.   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |
| <a id="kernel_filegroup-trim_nonlisted_kmi"></a>trim_nonlisted_kmi |  -   | Boolean | optional |  `False`  |


<a id="kernel_kythe"></a>

## kernel_kythe

<pre>
kernel_kythe(<a href="#kernel_kythe-name">name</a>, <a href="#kernel_kythe-corpus">corpus</a>, <a href="#kernel_kythe-kernel_build">kernel_build</a>)
</pre>

Extract Kythe source code index (kzip file) from a `kernel_build`.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="kernel_kythe-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="kernel_kythe-corpus"></a>corpus |  A flag containing value of `KYTHE_CORPUS`. See [kythe.io/examples](https://kythe.io/examples).   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |
| <a id="kernel_kythe-kernel_build"></a>kernel_build |  The `kernel_build` target to extract from.   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |


<a id="kernel_module_group"></a>

## kernel_module_group

<pre>
kernel_module_group(<a href="#kernel_module_group-name">name</a>, <a href="#kernel_module_group-srcs">srcs</a>)
</pre>

Like filegroup but for [`kernel_module`](#kernel_module)s or [`ddk_module`](#ddk_module)s.

Unlike filegroup, `srcs` must not be empty.

Example:

```
# //package/my_subsystem

# Hide a.ko and b.ko because they are implementation details of my_subsystem
ddk_module(
    name = "a",
    visibility = ["//visibility:private"],
    ...
)

ddk_module(
    name = "b",
    visibility = ["//visibility:private"],
    ...
)

# my_subsystem is the public target that the device should depend on.
kernel_module_group(
    name = "my_subsystem",
    srcs = [":a", ":b"],
    visibility = ["//package/my_device:__subpackages__"],
)

# //package/my_device
kernel_modules_install(
    name = "my_device_modules_install",
    kernel_modules = [
        "//package/my_subsystem:my_subsystem", # This is equivalent to specifying a and b.
    ],
)
```

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="kernel_module_group-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="kernel_module_group-srcs"></a>srcs |  List of [`kernel_module`](#kernel_module)s or [`ddk_module`](#ddk_module)s.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | required |  |


<a id="kernel_modules_install"></a>

## kernel_modules_install

<pre>
kernel_modules_install(<a href="#kernel_modules_install-name">name</a>, <a href="#kernel_modules_install-outs">outs</a>, <a href="#kernel_modules_install-kernel_build">kernel_build</a>, <a href="#kernel_modules_install-kernel_modules">kernel_modules</a>)
</pre>

Generates a rule that runs depmod in the module installation directory.

When including this rule to the `data` attribute of a `copy_to_dist_dir` rule,
all external kernel modules specified in `kernel_modules` are included in
distribution.  This excludes `module_outs` in `kernel_build` to avoid conflicts.

Example:
```
kernel_modules_install(
    name = "foo_modules_install",
    kernel_modules = [               # kernel_module rules
        "//path/to/nfc:nfc_module",
    ],
)
kernel_build(
    name = "foo",
    outs = ["vmlinux"],
    module_outs = ["core_module.ko"],
)
copy_to_dist_dir(
    name = "foo_dist",
    data = [
        ":foo",                      # Includes core_module.ko and vmlinux
        ":foo_modules_install",      # Includes nfc_module
    ],
)
```
In `foo_dist`, specifying `foo_modules_install` in `data` won't include
`core_module.ko`, because it is already included in `foo` in `data`.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="kernel_modules_install-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="kernel_modules_install-outs"></a>outs |  A list of additional outputs from `make modules_install`.<br><br>Since external modules are returned by default, it can be used to obtain modules.* related files (results of depmod). Only files with allowed names can be added to outs. (`_OUT_ALLOWLIST`) <pre><code>_OUT_ALLOWLIST = ["modules.dep", "modules.alias", "modules.builtin", "modules.symbols", "modules.softdep"]</code></pre> Example: <pre><code>kernel_modules_install(&#10;    name = "foo_modules_install",&#10;    kernel_modules = [":foo_module_list"],&#10;    outs = [&#10;        "modules.dep",&#10;        "modules.alias",&#10;    ],&#10;)</code></pre>   | List of strings | optional |  `[]`  |
| <a id="kernel_modules_install-kernel_build"></a>kernel_build |  Label referring to the `kernel_build` module. Otherwise, it is inferred from `kernel_modules`.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_modules_install-kernel_modules"></a>kernel_modules |  A list of labels referring to `kernel_module`s to install.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |


<a id="kernel_unstripped_modules_archive"></a>

## kernel_unstripped_modules_archive

<pre>
kernel_unstripped_modules_archive(<a href="#kernel_unstripped_modules_archive-name">name</a>, <a href="#kernel_unstripped_modules_archive-kernel_build">kernel_build</a>, <a href="#kernel_unstripped_modules_archive-kernel_modules">kernel_modules</a>)
</pre>

Compress the unstripped modules into a tarball.

Add this target to a `copy_to_dist_dir` rule to copy it to the distribution
directory, or `DIST_DIR`.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="kernel_unstripped_modules_archive-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="kernel_unstripped_modules_archive-kernel_build"></a>kernel_build |  A [`kernel_build`](#kernel_build) to retrieve unstripped in-tree modules from.<br><br>It requires `collect_unstripped_modules = True`. If the `kernel_build` has a `base_kernel`, the rule also retrieves unstripped in-tree modules from the `base_kernel`, and requires the `base_kernel` has `collect_unstripped_modules = True`.   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="kernel_unstripped_modules_archive-kernel_modules"></a>kernel_modules |  A list of external [`kernel_module`](#kernel_module)s to retrieve unstripped external modules from.<br><br>It requires that the base `kernel_build` has `collect_unstripped_modules = True`.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |


<a id="merged_kernel_uapi_headers"></a>

## merged_kernel_uapi_headers

<pre>
merged_kernel_uapi_headers(<a href="#merged_kernel_uapi_headers-name">name</a>, <a href="#merged_kernel_uapi_headers-kernel_build">kernel_build</a>, <a href="#merged_kernel_uapi_headers-kernel_modules">kernel_modules</a>)
</pre>

Merge `kernel-uapi-headers.tar.gz`.

On certain devices, kernel modules install additional UAPI headers. Use this
rule to add these module UAPI headers to the final `kernel-uapi-headers.tar.gz`.

If there are conflicts of file names in the source tarballs, files higher in
the list have higher priority:
1. UAPI headers from the `base_kernel` of the `kernel_build` (ususally the GKI build)
2. UAPI headers from the `kernel_build` (usually the device build)
3. UAPI headers from ``kernel_modules`. Order among the modules are undetermined.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="merged_kernel_uapi_headers-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="merged_kernel_uapi_headers-kernel_build"></a>kernel_build |  The `kernel_build`   | <a href="https://bazel.build/concepts/labels">Label</a> | required |  |
| <a id="merged_kernel_uapi_headers-kernel_modules"></a>kernel_modules |  A list of external `kernel_module`s to merge `kernel-uapi-headers.tar.gz`   | <a href="https://bazel.build/concepts/labels">List of labels</a> | optional |  `[]`  |


<a id="super_image"></a>

## super_image

<pre>
super_image(<a href="#super_image-name">name</a>, <a href="#super_image-out">out</a>, <a href="#super_image-super_img_size">super_img_size</a>, <a href="#super_image-system_dlkm_image">system_dlkm_image</a>, <a href="#super_image-vendor_dlkm_image">vendor_dlkm_image</a>)
</pre>

Build super image.

Optionally takes in a "system_dlkm" and "vendor_dlkm".

When included in a `copy_to_dist_dir` rule, this rule copies a `super.img` to `DIST_DIR`.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="super_image-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="super_image-out"></a>out |  Image file name   | String | optional |  `"super.img"`  |
| <a id="super_image-super_img_size"></a>super_img_size |  Size of super.img   | Integer | optional |  `268435456`  |
| <a id="super_image-system_dlkm_image"></a>system_dlkm_image |  `system_dlkm_image` to include in super.img   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="super_image-vendor_dlkm_image"></a>vendor_dlkm_image |  `vendor_dlkm_image` to include in super.img   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |


<a id="unsparsed_image"></a>

## unsparsed_image

<pre>
unsparsed_image(<a href="#unsparsed_image-name">name</a>, <a href="#unsparsed_image-src">src</a>, <a href="#unsparsed_image-out">out</a>)
</pre>

Build an unsparsed image.

Takes in a .img file and unsparses it.

When included in a `copy_to_dist_dir` rule, this rule copies a `super_unsparsed.img` to `DIST_DIR`.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="unsparsed_image-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="unsparsed_image-src"></a>src |  image to unsparse   | <a href="https://bazel.build/concepts/labels">Label</a> | optional |  `None`  |
| <a id="unsparsed_image-out"></a>out |  -   | String | required |  |


<a id="ddk_module"></a>

## ddk_module

<pre>
ddk_module(<a href="#ddk_module-name">name</a>, <a href="#ddk_module-kernel_build">kernel_build</a>, <a href="#ddk_module-srcs">srcs</a>, <a href="#ddk_module-deps">deps</a>, <a href="#ddk_module-hdrs">hdrs</a>, <a href="#ddk_module-textual_hdrs">textual_hdrs</a>, <a href="#ddk_module-includes">includes</a>, <a href="#ddk_module-conditional_srcs">conditional_srcs</a>,
           <a href="#ddk_module-linux_includes">linux_includes</a>, <a href="#ddk_module-out">out</a>, <a href="#ddk_module-local_defines">local_defines</a>, <a href="#ddk_module-copts">copts</a>, <a href="#ddk_module-kconfig">kconfig</a>, <a href="#ddk_module-defconfig">defconfig</a>, <a href="#ddk_module-generate_btf">generate_btf</a>, <a href="#ddk_module-kwargs">kwargs</a>)
</pre>

Defines a DDK (Driver Development Kit) module.

Example:

```
ddk_module(
    name = "my_module",
    srcs = ["my_module.c", "private_header.h"],
    out = "my_module.ko",
    # Exported headers
    hdrs = ["include/my_module_exported.h"],
    textual_hdrs = ["my_template.c"],
    includes = ["include"],
)
```

Note: Local headers should be specified in one of the following ways:

- In a `ddk_headers` target in the same package, if you need to auto-generate `-I` ccflags.
  In that case, specify the `ddk_headers` target in `deps`.
- Otherwise, in `srcs` if you don't need the `-I` ccflags.

Exported headers should be specified in one of the following ways:

- In a separate `ddk_headers` target in the same package. Then specify the
  target in `hdrs`. This is recommended if there
  are multiple `ddk_module`s depending on a
  [`glob`](https://bazel.build/reference/be/functions#glob) of headers or a large list
  of headers.
- Using `hdrs`, `textual_hdrs` and `includes` of this target.

`hdrs`, `textual_hdrs` and `includes` have the same semantics as [`ddk_headers`](#ddk_headers).
That is, this target effectively acts as a `ddk_headers` target when specified in the `deps`
attribute of another `ddk_module`. In other words, the following code snippet:

```
ddk_module(name = "module_A", hdrs = [...], includes = [...], ...)
ddk_module(name = "module_B", deps = ["module_A"], ...)
```

... is effectively equivalent to the following:

```
ddk_headers(name = "module_A_hdrs, hdrs = [...], includes = [...], ...)
ddk_module(name = "module_A", ...)
ddk_module(name = "module_B", deps = ["module_A", "module_A_hdrs"], ...)
```

**Submodules**

See [ddk_submodule](#ddk_submodule).

If `deps` contains a `ddk_submodule` target, the `ddk_module` target must not specify
anything except:

- `kernel_build`
- `linux_includes`

**Ordering of `includes`**

**The best practice is to not have conflicting header names and search paths.**
But if you do, see below for ordering of include directories to be
searched for header files.

A [`ddk_module`](#ddk_module) is compiled with the following order of include directories
(`-I` options):

1. Traverse depedencies for `linux_includes`:
    1. All `linux_includes` of this target, in the specified order
    2. All `linux_includes` of `deps`, in the specified order (recursively apply #1.3 on each target)
    3. All `linux_includes` of `hdrs`, in the specified order (recursively apply #1.3 on each target)
2. `LINUXINCLUDE` (See `${KERNEL_DIR}/Makefile`)
3. Traverse depedencies for `includes`:
    1. All `includes` of this target, in the specified order
    2. All `includes` of `deps`, in the specified order (recursively apply #3.1 and #3.3 on each target)
    3. All `includes` of `hdrs`, in the specified order (recursively apply #3.1 and #3.3 on each target)

In other words, #1 and #3 uses the `preorder` of
[depset](https://bazel.build/rules/lib/depset).

"In the specified order" means that order matters within these lists.
To prevent buildifier from sorting these lists, use the `# do not sort` magic line.

To export a target `:x` in `hdrs` before other targets in `deps`
(that is, if you need #3.3 before #3.2, or #1.2 before #1.1),
specify `:x` in the `deps` list in the position you want. See example below.

To export an include directory in `includes` that needs to be included
after other targets in `hdrs` or `deps` (that is, if you need #3.1 after #3.2
or #3.3), specify the include directory in a separate `ddk_headers` target,
then specify this `ddk_headers` target in `hdrs` and/or `deps` based on
your needs.

For example:

```
ddk_headers(name = "dep_a", includes = ["dep_a"], linux_includes = ["uapi/dep_a"])
ddk_headers(name = "dep_b", includes = ["dep_b"])
ddk_headers(name = "dep_c", includes = ["dep_c"], hdrs = ["dep_a"])
ddk_headers(name = "hdrs_a", includes = ["hdrs_a"], linux_includes = ["uapi/hdrs_a"])
ddk_headers(name = "hdrs_b", includes = ["hdrs_b"])
ddk_headers(name = "x", includes = ["x"])

ddk_module(
    name = "module",
    deps = [":dep_b", ":x", ":dep_c"],
    hdrs = [":hdrs_a", ":x", ":hdrs_b"],
    linux_includes = ["uapi/module"],
    includes = ["self_1", "self_2"],
)
```

Then `":module"` is compiled with these flags, in this order:

```
# 1.1 linux_includes
-Iuapi/module

# 1.2 deps, linux_includes, recursively
-Iuapi/dep_a

# 1.3 hdrs, linux_includes, recursively
-Iuapi/hdrs_a

# 2.
$(LINUXINCLUDE)

# 3.1 includes
-Iself_1
-Iself_2

# 3.2. deps, recursively
-Idep_b
-Ix
-Idep_a   # :dep_c depends on :dep_a, so include dep_a/ first
-Idep_c

# 3.3. hdrs, recursively
-Ihdrs_a
# x is already included, skip
-Ihdrs_b
```

A dependent module automatically gets #1.1, #1.3, #3.1, #3.3, in this order. For example:

```
ddk_module(
    name = "child",
    deps = [":module"],
    # ...
)
```

Then `":child"` is compiled with these flags, in this order:

```
# 1.2. linux_includes of deps, recursively
-Iuapi/module
-Iuapi/hdrs_a

# 2.
$(LINUXINCLUDE)

# 3.2. includes of deps, recursively
-Iself_1
-Iself_2
-Ihdrs_a
-Ix
-Ihdrs_b
```


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="ddk_module-name"></a>name |  Name of target. This should usually be name of the output `.ko` file without the suffix.   |  none |
| <a id="ddk_module-kernel_build"></a>kernel_build |  [`kernel_build`](#kernel_build)   |  none |
| <a id="ddk_module-srcs"></a>srcs |  sources and local headers.   |  `None` |
| <a id="ddk_module-deps"></a>deps |  A list of dependent targets. Each of them must be one of the following:<br><br>- [`kernel_module`](#kernel_module) - [`ddk_module`](#ddk_module) - [`ddk_headers`](#ddk_headers).   |  `None` |
| <a id="ddk_module-hdrs"></a>hdrs |  See [`ddk_headers.hdrs`](#ddk_headers-hdrs)   |  `None` |
| <a id="ddk_module-textual_hdrs"></a>textual_hdrs |  See [`ddk_headers.textual_hdrs`](#ddk_headers-textual_hdrs)   |  `None` |
| <a id="ddk_module-includes"></a>includes |  See [`ddk_headers.includes`](#ddk_headers-includes)   |  `None` |
| <a id="ddk_module-conditional_srcs"></a>conditional_srcs |  A dictionary that specifies sources conditionally compiled based on configs.<br><br>Example:<br><br><pre><code>conditional_srcs = {&#10;    "CONFIG_FOO": {&#10;        True: ["foo.c"],&#10;        False: ["notfoo.c"]&#10;    }&#10;}</code></pre><br><br>In the above example, if `CONFIG_FOO` is `y` or `m`, `foo.c` is compiled. Otherwise, `notfoo.c` is compiled instead.   |  `None` |
| <a id="ddk_module-linux_includes"></a>linux_includes |  See [`ddk_headers.linux_includes`](#ddk_headers-linux_includes)<br><br>Unlike `ddk_headers.linux_includes`, `ddk_module.linux_includes` is **NOT** applied to dependent `ddk_module`s.   |  `None` |
| <a id="ddk_module-out"></a>out |  The output module file. This should usually be `"{name}.ko"`.<br><br>This is required if this target does not contain submodules.   |  `None` |
| <a id="ddk_module-local_defines"></a>local_defines |  List of defines to add to the compile line.<br><br>**Order matters**. To prevent buildifier from sorting the list, use the `# do not sort` magic line.<br><br>Each string is prepended with `-D` and added to the compile command line for this target, but not to its dependents.<br><br>Unlike [`cc_library.local_defines`](https://bazel.build/reference/be/c-cpp#cc_library.local_defines), this is not subject to ["Make" variable substitution](https://bazel.build/reference/be/make-variables) or [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables).<br><br>Each string is treated as a single Bourne shell token. Unlike [`cc_library.local_defines`](https://bazel.build/reference/be/c-cpp#cc_library.local_defines), this is not subject to [Bourne shell tokenization](https://bazel.build/reference/be/common-definitions#sh-tokenization). The behavior is similar to `cc_library` with the `no_copts_tokenization` [feature](https://bazel.build/reference/be/functions#package.features). For details about `no_copts_tokenization`, see [`cc_library.copts`](https://bazel.build/reference/be/c-cpp#cc_library.copts).   |  `None` |
| <a id="ddk_module-copts"></a>copts |  Add these options to the compilation command.<br><br>**Order matters**. To prevent buildifier from sorting the list, use the `# do not sort` magic line.<br><br>Subject to [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables).<br><br>The flags take effect only for compiling this target, not its dependencies, so be careful about header files included elsewhere.<br><br>All host paths should be provided via [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables). See "Implementation detail" section below.<br><br>Each `$(location)` expression should occupy its own token. For example:<br><br><pre><code># Good&#10;copts = ["-include", "$(location //other:header.h)"]&#10;&#10;# BAD -- DON'T DO THIS!&#10;copts = ["-include $(location //other:header.h)"]&#10;&#10;# BAD -- DON'T DO THIS!&#10;copts = ["-include=$(location //other:header.h)"]</code></pre><br><br>Unlike [`cc_library.local_defines`](https://bazel.build/reference/be/c-cpp#cc_library.local_defines), this is not subject to ["Make" variable substitution](https://bazel.build/reference/be/make-variables).<br><br>Each string is treated as a single Bourne shell token. Unlike [`cc_library.copts`](https://bazel.build/reference/be/c-cpp#cc_library.copts) this is not subject to [Bourne shell tokenization](https://bazel.build/reference/be/common-definitions#sh-tokenization). The behavior is similar to `cc_library` with the `no_copts_tokenization` [feature](https://bazel.build/reference/be/functions#package.features). For details about `no_copts_tokenization`, see [`cc_library.copts`](https://bazel.build/reference/be/c-cpp#cc_library.copts).<br><br>Because each string is treated as a single Bourne shell token, if a plural `$(locations)` expression expands to multiple paths, they are treated as a single Bourne shell token, which is likely an undesirable behavior. To avoid surprising behaviors, use singular `$(location)` expressions to ensure that the label only expands to one path. For differences between the `$(locations)` and `$(location)`, see [`$(location)` substitution](https://bazel.build/reference/be/make-variables#predefined_label_variables).<br><br>**Implementation detail**: Unlike usual `$(location)` expansion, `$(location)` in `copts` is expanded to a path relative to the current package before sending to the compiler.<br><br>For example:<br><br><pre><code># package: //package&#10;ddk_module(&#10;  name = "my_module",&#10;  copts = ["-include", "$(location //other:header.h)"],&#10;  srcs = ["//other:header.h", "my_module.c"],&#10;)</code></pre> Then the generated Makefile contains:<br><br><pre><code>ccflags-y += -include ../other/header.h</code></pre><br><br>The behavior is such because the generated `Makefile` is located in `package/Makefile`, and `make` is executed under `package/`. In order to find `other/header.h`, its path relative to `package/` is given.   |  `None` |
| <a id="ddk_module-kconfig"></a>kconfig |  The Kconfig file for this external module.<br><br>See [`Documentation/kbuild/kconfig-language.rst`](https://www.kernel.org/doc/html/latest/kbuild/kconfig.html) for its format.<br><br>Kconfig is optional for a `ddk_module`. The final Kconfig known by this module consists of the following:<br><br>- Kconfig from `kernel_build` - Kconfig from dependent modules, if any - Kconfig of this module, if any   |  `None` |
| <a id="ddk_module-defconfig"></a>defconfig |  The `defconfig` file.<br><br>Items must already be declared in `kconfig`. An item not declared in Kconfig and inherited Kconfig files is silently dropped.<br><br>An item declared in `kconfig` without a specific value in `defconfig` uses default value specified in `kconfig`.   |  `None` |
| <a id="ddk_module-generate_btf"></a>generate_btf |  Allows generation of BTF type information for the module. See [kernel_module.generate_btf](#kernel_module-generate_btf)   |  `None` |
| <a id="ddk_module-kwargs"></a>kwargs |  Additional attributes to the internal rule. See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="ddk_submodule"></a>

## ddk_submodule

<pre>
ddk_submodule(<a href="#ddk_submodule-name">name</a>, <a href="#ddk_submodule-out">out</a>, <a href="#ddk_submodule-srcs">srcs</a>, <a href="#ddk_submodule-deps">deps</a>, <a href="#ddk_submodule-hdrs">hdrs</a>, <a href="#ddk_submodule-includes">includes</a>, <a href="#ddk_submodule-local_defines">local_defines</a>, <a href="#ddk_submodule-copts">copts</a>, <a href="#ddk_submodule-conditional_srcs">conditional_srcs</a>, <a href="#ddk_submodule-kwargs">kwargs</a>)
</pre>

Declares a DDK (Driver Development Kit) submodule.

Symbol dependencies between submodules in the same [`ddk_module`](#ddk_module)
are not specified explicitly. This is convenient when you have multiple module
files for a subsystem.

See [Building External Modules](https://www.kernel.org/doc/Documentation/kbuild/modules.rst)
or `Documentation/kbuild/modules.rst`, section "6.3 Symbols From Another External Module",
"Use a top-level kbuild file".

Example:

```
ddk_submodule(
    name = "a",
    out = "a.ko",
    srcs = ["a.c"],
)

ddk_submodule(
    name = "b",
    out = "b.ko",
    srcs = ["b_1.c", "b_2.c"],
)

ddk_module(
    name = "mymodule",
    kernel_build = ":tuna",
    deps = [":a", ":b"],
)
```

`linux_includes` must be specified in the top-level `ddk_module`; see
[`ddk_module.linux_includes`](#ddk_module-linux_includes).

**Ordering of `includes`**

See [`ddk_module`](#ddk_module).

**Caveats**

As an implementation detail, `ddk_submodule` alone does not build any modules. The
`ddk_module` target is the one responsible for building all `.ko` files.

A side effect is that for incremental builds, modules may be rebuilt unexpectedly.
In the above example,
if `a.c` is modified, the whole `mymodule` is rebuilt, causing both `a.ko` and `b.ko` to
be rebuilt. Because `ddk_module` is always built in a sandbox, the object files (`*.o`) for
`b.ko` is not cached.

Hence, it is always recommended to use one `ddk_module` per module (`.ko` file). You may
use `build/kernel/kleaf/build_cleaner.py` to resolve dependencies; see
`build/kernel/kleaf/docs/build_cleaner.md`.

The `ddk_submodule` rule should only be used when the dependencies among modules are too
complicated to be presented in `BUILD.bazel`, and are frequently updated. When the
dependencies are stable, it is recommended to:

1. Replace `ddk_submodule` with `ddk_module`;
2. Specify dependencies in the `deps` attribute explicitly.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="ddk_submodule-name"></a>name |  See [`ddk_module.name`](#ddk_module-name).   |  none |
| <a id="ddk_submodule-out"></a>out |  See [`ddk_module.out`](#ddk_module-out).   |  none |
| <a id="ddk_submodule-srcs"></a>srcs |  See [`ddk_module.srcs`](#ddk_module-srcs).   |  `None` |
| <a id="ddk_submodule-deps"></a>deps |  See [`ddk_module.deps`](#ddk_module-deps).<br><br>These are only effective in the current submodule, not other submodules declared in the same [`ddk_module.deps`](#ddk_module-deps).<br><br>These are not exported to downstream targets that depends on the `ddk_module` that includes the current target.   |  `None` |
| <a id="ddk_submodule-hdrs"></a>hdrs |  See [`ddk_module.hdrs`](#ddk_module-hdrs).<br><br>These are only effective in the current submodule, not other submodules declared in the same [`ddk_module.deps`](#ddk_module-deps).<br><br>These are exported to downstream targets that depends on the `ddk_module` that includes the current target. Example:<br><br><pre><code>ddk_submodule(name = "module_parent_a", hdrs = [...])&#10;ddk_module(name = "module_parent", deps = [":module_parent_a"])&#10;ddk_module(name = "module_child", deps = [":module_parent"])</code></pre><br><br>`module_child` automatically gets `hdrs` of `module_parent_a`.   |  `None` |
| <a id="ddk_submodule-includes"></a>includes |  See [`ddk_module.includes`](#ddk_module-includes).<br><br>These are only effective in the current submodule, not other submodules declared in the same [`ddk_module.deps`](#ddk_module-deps).<br><br>These are exported to downstream targets that depends on the `ddk_module` that includes the current target. Example:<br><br><pre><code>ddk_submodule(name = "module_parent_a", includes = [...])&#10;ddk_module(name = "module_parent", deps = [":module_parent_a"])&#10;ddk_module(name = "module_child", deps = [":module_parent"])</code></pre><br><br>`module_child` automatically gets `includes` of `module_parent_a`.   |  `None` |
| <a id="ddk_submodule-local_defines"></a>local_defines |  See [`ddk_module.local_defines`](#ddk_module-local_defines).<br><br>These are only effective in the current submodule, not other submodules declared in the same [`ddk_module.deps`](#ddk_module-deps).<br><br>These are not exported to downstream targets that depends on the `ddk_module` that includes the current target.   |  `None` |
| <a id="ddk_submodule-copts"></a>copts |  See [`ddk_module.copts`](#ddk_module-copts).<br><br>These are only effective in the current submodule, not other submodules declared in the same [`ddk_module.deps`](#ddk_module-deps).<br><br>These are not exported to downstream targets that depends on the `ddk_module` that includes the current target.   |  `None` |
| <a id="ddk_submodule-conditional_srcs"></a>conditional_srcs |  See [`ddk_module.conditional_srcs`](#ddk_module-conditional_srcs).   |  `None` |
| <a id="ddk_submodule-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="dependency_graph"></a>

## dependency_graph

<pre>
dependency_graph(<a href="#dependency_graph-name">name</a>, <a href="#dependency_graph-kernel_build">kernel_build</a>, <a href="#dependency_graph-kernel_modules">kernel_modules</a>, <a href="#dependency_graph-colorful">colorful</a>, <a href="#dependency_graph-exclude_base_kernel_modules">exclude_base_kernel_modules</a>, <a href="#dependency_graph-kwargs">kwargs</a>)
</pre>

Declare targets for dependency graph visualization.

Output:
    File with a diagram representing a graph in DOT language.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="dependency_graph-name"></a>name |  Name of this target.   |  none |
| <a id="dependency_graph-kernel_build"></a>kernel_build |  The [`kernel_build`](#kernel_build).   |  none |
| <a id="dependency_graph-kernel_modules"></a>kernel_modules |  A list of external [`kernel_module()`](#kernel_module)s.   |  none |
| <a id="dependency_graph-colorful"></a>colorful |  When set to True, outgoing edges from every node are colored differently.   |  `None` |
| <a id="dependency_graph-exclude_base_kernel_modules"></a>exclude_base_kernel_modules |  Whether the analysis should made for only external modules.   |  `None` |
| <a id="dependency_graph-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="initramfs_modules_lists_test"></a>

## initramfs_modules_lists_test

<pre>
initramfs_modules_lists_test(<a href="#initramfs_modules_lists_test-name">name</a>, <a href="#initramfs_modules_lists_test-kernel_images">kernel_images</a>, <a href="#initramfs_modules_lists_test-expected_modules_list">expected_modules_list</a>,
                             <a href="#initramfs_modules_lists_test-expected_modules_recovery_list">expected_modules_recovery_list</a>, <a href="#initramfs_modules_lists_test-expected_modules_charger_list">expected_modules_charger_list</a>,
                             <a href="#initramfs_modules_lists_test-build_vendor_boot">build_vendor_boot</a>, <a href="#initramfs_modules_lists_test-build_vendor_kernel_boot">build_vendor_kernel_boot</a>, <a href="#initramfs_modules_lists_test-kwargs">kwargs</a>)
</pre>

Tests that the initramfs has modules.load* files with the given content.

**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="initramfs_modules_lists_test-name"></a>name |  name of the test   |  none |
| <a id="initramfs_modules_lists_test-kernel_images"></a>kernel_images |  name of the `kernel_images` target. It must build initramfs.   |  none |
| <a id="initramfs_modules_lists_test-expected_modules_list"></a>expected_modules_list |  file with the expected content for `modules.load`   |  `None` |
| <a id="initramfs_modules_lists_test-expected_modules_recovery_list"></a>expected_modules_recovery_list |  file with the expected content for `modules.load.recovery`   |  `None` |
| <a id="initramfs_modules_lists_test-expected_modules_charger_list"></a>expected_modules_charger_list |  file with the expected content for `modules.load.charger`   |  `None` |
| <a id="initramfs_modules_lists_test-build_vendor_boot"></a>build_vendor_boot |  If the `kernel_images` target builds vendor_boot.img   |  `None` |
| <a id="initramfs_modules_lists_test-build_vendor_kernel_boot"></a>build_vendor_kernel_boot |  If the `kernel_images` target builds vendor_kernel_boot.img   |  `None` |
| <a id="initramfs_modules_lists_test-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_abi"></a>

## kernel_abi

<pre>
kernel_abi(<a href="#kernel_abi-name">name</a>, <a href="#kernel_abi-kernel_build">kernel_build</a>, <a href="#kernel_abi-define_abi_targets">define_abi_targets</a>, <a href="#kernel_abi-kernel_modules">kernel_modules</a>, <a href="#kernel_abi-module_grouping">module_grouping</a>,
           <a href="#kernel_abi-abi_definition_stg">abi_definition_stg</a>, <a href="#kernel_abi-kmi_enforced">kmi_enforced</a>, <a href="#kernel_abi-unstripped_modules_archive">unstripped_modules_archive</a>, <a href="#kernel_abi-kmi_symbol_list_add_only">kmi_symbol_list_add_only</a>,
           <a href="#kernel_abi-kernel_modules_exclude_list">kernel_modules_exclude_list</a>, <a href="#kernel_abi-enable_add_vmlinux">enable_add_vmlinux</a>, <a href="#kernel_abi-kwargs">kwargs</a>)
</pre>

Declare multiple targets to support ABI monitoring.

This macro is meant to be used alongside [`kernel_build`](#kernel_build)
macro.

For example, you may have the following declaration. (For actual definition
of `kernel_aarch64`, see
[`define_common_kernels()`](#define_common_kernels).

```
kernel_build(name = "kernel_aarch64", ...)
kernel_abi(
    name = "kernel_aarch64_abi",
    kernel_build = ":kernel_aarch64",
    ...
)
_dist_targets = ["kernel_aarch64", ...]
copy_to_dist_dir(name = "kernel_aarch64_dist", data = _dist_targets)
kernel_abi_dist(
    name = "kernel_aarch64_abi_dist",
    kernel_abi = "kernel_aarch64_abi",
    data = _dist_targets,
)
```

The `kernel_abi` invocation above defines the following targets:
- `kernel_aarch64_abi_dump`
  - Building this target extracts the ABI.
  - Include this target in a [`kernel_abi_dist`](#kernel_abi_dist)
    target to copy ABI dump to `--dist-dir`.
- `kernel_aarch64_abi`
  - A filegroup that contains `kernel_aarch64_abi_dump`. It also contains other targets
    if `define_abi_targets = True`; see below.

In addition, the following targets are defined if `define_abi_targets = True`:
- `kernel_aarch64_abi_update_symbol_list`
  - Running this target updates `kmi_symbol_list`.
- `kernel_aarch64_abi_update_protected_exports`
  - Running this target updates `protected_exports_list`.
- `kernel_aarch64_abi_update`
  - Running this target updates `abi_definition`.
- `kernel_aarch64_abi_dump`
  - Building this target extracts the ABI.
  - Include this target in a [`kernel_abi_dist`](#kernel_abi_dist)
    target to copy ABI dump to `--dist-dir`.

See build/kernel/kleaf/abi.md for a conversion chart from `build_abi.sh`
commands to Bazel commands.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_abi-name"></a>name |  Name of this target.   |  none |
| <a id="kernel_abi-kernel_build"></a>kernel_build |  The [`kernel_build`](#kernel_build).   |  none |
| <a id="kernel_abi-define_abi_targets"></a>define_abi_targets |  Whether the target contains other files to support ABI monitoring. If `None`, defaults to `True`.<br><br>If `False`, this macro is equivalent to just calling <pre><code>kernel_build(name = name, **kwargs)&#10;filegroup(name = name + "_abi", data = [name, abi_dump_target])</code></pre><br><br>If `True`, implies `collect_unstripped_modules = True`. See [`kernel_build.collect_unstripped_modules`](#kernel_build-collect_unstripped_modules).   |  `None` |
| <a id="kernel_abi-kernel_modules"></a>kernel_modules |  A list of external [`kernel_module()`](#kernel_module)s to extract symbols from.   |  `None` |
| <a id="kernel_abi-module_grouping"></a>module_grouping |  If unspecified or `None`, it is `True` by default. If `True`, then the symbol list will group symbols based on the kernel modules that reference the symbol. Otherwise the symbol list will simply be a sorted list of symbols used by all the kernel modules.   |  `None` |
| <a id="kernel_abi-abi_definition_stg"></a>abi_definition_stg |  Location of the ABI definition in STG format.   |  `None` |
| <a id="kernel_abi-kmi_enforced"></a>kmi_enforced |  This is an indicative option to signal that KMI is enforced. If set to `True`, KMI checking tools respects it and reacts to it by failing if KMI differences are detected.   |  `None` |
| <a id="kernel_abi-unstripped_modules_archive"></a>unstripped_modules_archive |  A [`kernel_unstripped_modules_archive`](#kernel_unstripped_modules_archive) which name is specified in `abi.prop`. DEPRECATED.   |  `None` |
| <a id="kernel_abi-kmi_symbol_list_add_only"></a>kmi_symbol_list_add_only |  If unspecified or `None`, it is `False` by default. If `True`, then any symbols in the symbol list that would have been removed are preserved (at the end of the file). Symbol list update will fail if there is no pre-existing symbol list file to read from. This property is intended to prevent unintentional shrinkage of a stable ABI.<br><br>This should be set to `True` if `KMI_SYMBOL_LIST_ADD_ONLY=1`.   |  `None` |
| <a id="kernel_abi-kernel_modules_exclude_list"></a>kernel_modules_exclude_list |  List of base names for in-tree kernel modules to exclude from. i.e. This is the modules built in `kernel_build`, not the `kernel_modules` mentioned above.   |  `None` |
| <a id="kernel_abi-enable_add_vmlinux"></a>enable_add_vmlinux |  If unspecified or `None`, it is `True` by default. If `True`, enable the `kernel_build_add_vmlinux` [transition](https://bazel.build/extending/config#user-defined-transitions) from all targets instantiated by this macro (e.g. produced by abi_dump, extracted_symbols, etc).   |  `None` |
| <a id="kernel_abi-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_abi_dist"></a>

## kernel_abi_dist

<pre>
kernel_abi_dist(<a href="#kernel_abi_dist-name">name</a>, <a href="#kernel_abi_dist-kernel_abi">kernel_abi</a>, <a href="#kernel_abi_dist-kernel_build_add_vmlinux">kernel_build_add_vmlinux</a>, <a href="#kernel_abi_dist-ignore_diff">ignore_diff</a>, <a href="#kernel_abi_dist-no_ignore_diff_target">no_ignore_diff_target</a>,
                <a href="#kernel_abi_dist-kwargs">kwargs</a>)
</pre>

A wrapper over `copy_to_dist_dir` for [`kernel_abi`](#kernel_abi).

After copying all files to dist dir, return the exit code from `diff_abi`.

**Implementation notes**:

`with_vmlinux_transition` is applied on all targets by default. In
particular, the `kernel_build` targets in `data` automatically builds
`vmlinux` regardless of whether `vmlinux` is specified in `outs`.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_abi_dist-name"></a>name |  name of the dist target   |  none |
| <a id="kernel_abi_dist-kernel_abi"></a>kernel_abi |  name of the [`kernel_abi`](#kernel_abi) invocation.   |  none |
| <a id="kernel_abi_dist-kernel_build_add_vmlinux"></a>kernel_build_add_vmlinux |  If `True`, all `kernel_build` targets depended on by this change automatically applies a [transition](https://bazel.build/extending/config#user-defined-transitions) that always builds `vmlinux`. For up-to-date implementation details, look for `with_vmlinux_transition` in `build/kernel/kleaf/impl/abi`.<br><br>If there are multiple `kernel_build` targets in `data`, only keep the one for device build. Otherwise, the build may break. For example:<br><br><pre><code>kernel_build(&#10;    name = "tuna",&#10;    base_kernel = "//common:kernel_aarch64"&#10;    ...&#10;)&#10;&#10;kernel_abi(...)&#10;kernel_abi_dist(&#10;    name = "tuna_abi_dist",&#10;    data = [&#10;        ":tuna",&#10;        # "//common:kernel_aarch64", # remove GKI&#10;    ],&#10;    kernel_build_add_vmlinux = True,&#10;)</code></pre><br><br>Enabling this option ensures that `tuna_abi_dist` doesn't build `//common:kernel_aarch64` and `:tuna` twice, once with the transition and once without. Enabling this ensures that `//common:kernel_aarch64` and `:tuna` always built with the transition.<br><br>**Note**: Its value will be `True` by default in the future. During the migration period, this is `False` by default. Once all devices have been fixed, this attribute will be set to `True` by default.   |  `None` |
| <a id="kernel_abi_dist-ignore_diff"></a>ignore_diff |  [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes). If `True` and the return code of `stgdiff` signals the ABI difference, then the result is ignored.   |  `None` |
| <a id="kernel_abi_dist-no_ignore_diff_target"></a>no_ignore_diff_target |  [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes). If `ignore_diff` is `True`, this need to be set to a name of the target that doesn't have `ignore_diff`. This target will be recommended as an alternative to a user. If `no_ignore_diff_target` is None, there will be no alternative recommended.   |  `None` |
| <a id="kernel_abi_dist-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_build"></a>

## kernel_build

<pre>
kernel_build(<a href="#kernel_build-name">name</a>, <a href="#kernel_build-build_config">build_config</a>, <a href="#kernel_build-outs">outs</a>, <a href="#kernel_build-keep_module_symvers">keep_module_symvers</a>, <a href="#kernel_build-srcs">srcs</a>, <a href="#kernel_build-module_outs">module_outs</a>, <a href="#kernel_build-implicit_outs">implicit_outs</a>,
             <a href="#kernel_build-module_implicit_outs">module_implicit_outs</a>, <a href="#kernel_build-generate_vmlinux_btf">generate_vmlinux_btf</a>, <a href="#kernel_build-deps">deps</a>, <a href="#kernel_build-arch">arch</a>, <a href="#kernel_build-base_kernel">base_kernel</a>, <a href="#kernel_build-make_goals">make_goals</a>,
             <a href="#kernel_build-kconfig_ext">kconfig_ext</a>, <a href="#kernel_build-dtstree">dtstree</a>, <a href="#kernel_build-kmi_symbol_list">kmi_symbol_list</a>, <a href="#kernel_build-protected_exports_list">protected_exports_list</a>, <a href="#kernel_build-protected_modules_list">protected_modules_list</a>,
             <a href="#kernel_build-additional_kmi_symbol_lists">additional_kmi_symbol_lists</a>, <a href="#kernel_build-trim_nonlisted_kmi">trim_nonlisted_kmi</a>, <a href="#kernel_build-kmi_symbol_list_strict_mode">kmi_symbol_list_strict_mode</a>,
             <a href="#kernel_build-collect_unstripped_modules">collect_unstripped_modules</a>, <a href="#kernel_build-enable_interceptor">enable_interceptor</a>, <a href="#kernel_build-kbuild_symtypes">kbuild_symtypes</a>, <a href="#kernel_build-toolchain_version">toolchain_version</a>,
             <a href="#kernel_build-strip_modules">strip_modules</a>, <a href="#kernel_build-module_signing_key">module_signing_key</a>, <a href="#kernel_build-system_trusted_key">system_trusted_key</a>,
             <a href="#kernel_build-modules_prepare_force_generate_headers">modules_prepare_force_generate_headers</a>, <a href="#kernel_build-defconfig_fragments">defconfig_fragments</a>, <a href="#kernel_build-page_size">page_size</a>, <a href="#kernel_build-pack_module_env">pack_module_env</a>,
             <a href="#kernel_build-sanitizers">sanitizers</a>, <a href="#kernel_build-ddk_module_defconfig_fragments">ddk_module_defconfig_fragments</a>, <a href="#kernel_build-kwargs">kwargs</a>)
</pre>

Defines a kernel build target with all dependent targets.

It uses a `build_config` to construct a deterministic build environment (e.g.
`common/build.config.gki.aarch64`). The kernel sources need to be declared
via srcs (using a `glob()`). outs declares the output files that are surviving
the build. The effective output file names will be
`$(name)/$(output_file)`. Any other artifact is not guaranteed to be
accessible after the rule has run. The default `toolchain_version` is defined
with the value in `common/build.config.constants`, but can be overriden.

A few additional labels are generated.
For example, if name is `"kernel_aarch64"`:
- `kernel_aarch64_uapi_headers` provides the UAPI kernel headers.
- `kernel_aarch64_headers` provides the kernel headers.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_build-name"></a>name |  The final kernel target name, e.g. `"kernel_aarch64"`.   |  none |
| <a id="kernel_build-build_config"></a>build_config |  Label of the build.config file, e.g. `"build.config.gki.aarch64"`.   |  none |
| <a id="kernel_build-outs"></a>outs |  The expected output files.<br><br>Note: in-tree modules should be specified in `module_outs` instead.<br><br>This attribute must be either a `dict` or a `list`. If it is a `list`, for each item in `out`:<br><br>- If `out` does not contain a slash, the build rule   automatically finds a file with name `out` in the kernel   build output directory `${OUT_DIR}`. <pre><code>  find ${OUT_DIR} -name {out}</code></pre>   There must be exactly one match.   The file is copied to the following in the output directory   `{name}/{out}`<br><br>  Example: <pre><code>  kernel_build(name = "kernel_aarch64", outs = ["vmlinux"])</code></pre>   The bulid system copies `${OUT_DIR}/[<optional subdirectory>/]vmlinux`   to `kernel_aarch64/vmlinux`.   `kernel_aarch64/vmlinux` is the label to the file.<br><br>- If `out` contains a slash, the build rule locates the file in the   kernel build output directory `${OUT_DIR}` with path `out`   The file is copied to the following in the output directory     1. `{name}/{out}`     2. `{name}/$(basename {out})`<br><br>  Example: <pre><code>  kernel_build(&#10;    name = "kernel_aarch64",&#10;    outs = ["arch/arm64/boot/vmlinux"])</code></pre>   The bulid system copies     `${OUT_DIR}/arch/arm64/boot/vmlinux`   to:     - `kernel_aarch64/arch/arm64/boot/vmlinux`     - `kernel_aarch64/vmlinux`   They are also the labels to the output files, respectively.<br><br>  See `search_and_cp_output.py` for details.<br><br>Files in `outs` are part of the [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html) that this `kernel_build` returns. For example: <pre><code>kernel_build(name = "kernel", outs = ["vmlinux"], ...)&#10;copy_to_dist_dir(name = "kernel_dist", data = [":kernel"])</code></pre> `vmlinux` will be included in the distribution.<br><br>If it is a `dict`, it is wrapped in [`select()`](https://docs.bazel.build/versions/main/configurable-attributes.html).<br><br>Example: <pre><code>kernel_build(&#10;  name = "kernel_aarch64",&#10;  outs = {"config_foo": ["vmlinux"]})</code></pre> If conditions in `config_foo` is met, the rule is equivalent to <pre><code>kernel_build(&#10;  name = "kernel_aarch64",&#10;  outs = ["vmlinux"])</code></pre> As explained above, the bulid system copies `${OUT_DIR}/[<optional subdirectory>/]vmlinux` to `kernel_aarch64/vmlinux`. `kernel_aarch64/vmlinux` is the label to the file.<br><br>Note that a `select()` may not be passed into `kernel_build()` because [`select()` cannot be evaluated in macros](https://docs.bazel.build/versions/main/configurable-attributes.html#why-doesnt-select-work-in-macros). Hence: - [combining `select()`s](https://docs.bazel.build/versions/main/configurable-attributes.html#combining-selects)   is not allowed. Instead, expand the cartesian product. - To use   [`AND` chaining](https://docs.bazel.build/versions/main/configurable-attributes.html#or-chaining)   or   [`OR` chaining](https://docs.bazel.build/versions/main/configurable-attributes.html#selectsconfig_setting_group),   use `selects.config_setting_group()`.   |  none |
| <a id="kernel_build-keep_module_symvers"></a>keep_module_symvers |  If set to True, a copy of the default output `Module.symvers` is kept. * To avoid collisions in mixed build distribution packages, the file is renamed   as `$(name)_Module.symvers`. * Default is False.   |  `None` |
| <a id="kernel_build-srcs"></a>srcs |  The kernel sources (a `glob()`). If unspecified or `None`, it is the following: <pre><code>glob(&#10;    ["**"],&#10;    exclude = [&#10;        "**/.*",          # Hidden files&#10;        "**/.*/**",       # Files in hidden directories&#10;        "**/BUILD.bazel", # build files&#10;        "**/*.bzl",       # build files&#10;    ],&#10;)</code></pre>   |  `None` |
| <a id="kernel_build-module_outs"></a>module_outs |  A list of in-tree drivers. Similar to `outs`, but for `*.ko` files.<br><br>If a `*.ko` kernel module should not be copied to `${DIST_DIR}`, it must be included `implicit_outs` instead of `module_outs`. The list `implicit_outs + module_outs` must include **all** `*.ko` files in `${OUT_DIR}`. If not, a build error is raised.<br><br>Like `outs`, `module_outs` are part of the [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html) that this `kernel_build` returns. For example: <pre><code>kernel_build(name = "kernel", module_outs = ["foo.ko"], ...)&#10;copy_to_dist_dir(name = "kernel_dist", data = [":kernel"])</code></pre> `foo.ko` will be included in the distribution.<br><br>Like `outs`, this may be a `dict`. If so, it is wrapped in [`select()`](https://docs.bazel.build/versions/main/configurable-attributes.html). See documentation for `outs` for more details.   |  `None` |
| <a id="kernel_build-implicit_outs"></a>implicit_outs |  Like `outs`, but not copied to the distribution directory.<br><br>Labels are created for each item in `implicit_outs` as in `outs`.   |  `None` |
| <a id="kernel_build-module_implicit_outs"></a>module_implicit_outs |  like `module_outs`, but not copied to the distribution directory.<br><br>Labels are created for each item in `module_implicit_outs` as in `outs`.   |  `None` |
| <a id="kernel_build-generate_vmlinux_btf"></a>generate_vmlinux_btf |  If `True`, generates `vmlinux.btf` that is stripped of any debug symbols, but contains type and symbol information within a .BTF section. This is suitable for ABI analysis through BTF.<br><br>Requires that `"vmlinux"` is in `outs`.   |  `None` |
| <a id="kernel_build-deps"></a>deps |  Additional dependencies to build this kernel.   |  `None` |
| <a id="kernel_build-arch"></a>arch |  [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes). Target architecture. Default is `arm64`.<br><br>Value should be one of: * `arm64` * `x86_64` * `riscv64` * `arm` (for 32-bit, uncommon) * `i386` (for 32-bit, uncommon)<br><br>This must be consistent to `ARCH` in build configs if the latter is specified. Otherwise, a warning / error may be raised.   |  `None` |
| <a id="kernel_build-base_kernel"></a>base_kernel |  A label referring the base kernel build.<br><br>If set, the list of files specified in the `DefaultInfo` of the rule specified in `base_kernel` is copied to a directory, and `KBUILD_MIXED_TREE` is set to the directory. Setting `KBUILD_MIXED_TREE` effectively enables mixed build.<br><br>To set additional flags for mixed build, change `build_config` to a `kernel_build_config` rule, with a build config fragment that contains the additional flags.<br><br>The label specified by `base_kernel` must produce a list of files similar to what a `kernel_build` rule does. Usually, this points to one of the following: - `//common:kernel_{arch}` - A `kernel_filegroup` rule, e.g. <pre><code>  load("//build/kernel/kleaf:constants.bzl, "DEFAULT_GKI_OUTS")&#10;  kernel_filegroup(&#10;    name = "my_kernel_filegroup",&#10;    srcs = DEFAULT_GKI_OUTS,&#10;  )</code></pre>   |  `None` |
| <a id="kernel_build-make_goals"></a>make_goals |  A list of strings defining targets for the kernel build. This overrides `MAKE_GOALS` from build config if provided.   |  `None` |
| <a id="kernel_build-kconfig_ext"></a>kconfig_ext |  Label of an external Kconfig.ext file sourced by the GKI kernel.   |  `None` |
| <a id="kernel_build-dtstree"></a>dtstree |  Device tree support.   |  `None` |
| <a id="kernel_build-kmi_symbol_list"></a>kmi_symbol_list |  A label referring to the main KMI symbol list file. See `additional_kmi_symbol_lists`.<br><br>This is the Bazel equivalent of `ADDITIONAL_KMI_SYMBOL_LISTS`.   |  `None` |
| <a id="kernel_build-protected_exports_list"></a>protected_exports_list |  A file containing list of protected exports. For example: <pre><code>protected_exports_list = "//common:android/abi_gki_protected_exports"</code></pre>   |  `None` |
| <a id="kernel_build-protected_modules_list"></a>protected_modules_list |  A file containing list of protected modules, For example: <pre><code>protected_modules_list = "//common:android/gki_protected_modules"</code></pre>   |  `None` |
| <a id="kernel_build-additional_kmi_symbol_lists"></a>additional_kmi_symbol_lists |  A list of labels referring to additional KMI symbol list files.<br><br>This is the Bazel equivalent of `ADDITIONAL_KMI_SYMBOL_LISTS`.<br><br>Let <pre><code>all_kmi_symbol_lists = [kmi_symbol_list] + additional_kmi_symbol_list</code></pre><br><br>If `all_kmi_symbol_lists` is a non-empty list, `abi_symbollist` and `abi_symbollist.report` are created and added to the [`DefaultInfo`](https://docs.bazel.build/versions/main/skylark/lib/DefaultInfo.html), and copied to `DIST_DIR` during distribution.<br><br>If `all_kmi_symbol_lists` is `None` or an empty list, `abi_symbollist` and `abi_symbollist.report` are not created.<br><br>It is possible to use a `glob()` to determine whether `abi_symbollist` and `abi_symbollist.report` should be generated at build time. For example: <pre><code>kmi_symbol_list = "android/abi_gki_aarch64",&#10;additional_kmi_symbol_lists = glob(["android/abi_gki_aarch64*"], exclude = ["android/abi_gki_aarch64"]),</code></pre>   |  `None` |
| <a id="kernel_build-trim_nonlisted_kmi"></a>trim_nonlisted_kmi |  If `True`, trim symbols not listed in `kmi_symbol_list` and `additional_kmi_symbol_lists`. This is the Bazel equivalent of `TRIM_NONLISTED_KMI`.<br><br>Requires `all_kmi_symbol_lists` to be non-empty. If `kmi_symbol_list` or `additional_kmi_symbol_lists` is a `glob()`, it is possible to set `trim_nonlisted_kmi` to be a value based on that `glob()`. For example: <pre><code>trim_nonlisted_kmi = len(glob(["android/abi_gki_aarch64*"])) &gt; 0</code></pre>   |  `None` |
| <a id="kernel_build-kmi_symbol_list_strict_mode"></a>kmi_symbol_list_strict_mode |  If `True`, add a build-time check between `[kmi_symbol_list] + additional_kmi_symbol_lists` and the KMI resulting from the build, to ensure they match 1-1.   |  `None` |
| <a id="kernel_build-collect_unstripped_modules"></a>collect_unstripped_modules |  If `True`, provide all unstripped in-tree.   |  `None` |
| <a id="kernel_build-enable_interceptor"></a>enable_interceptor |  If set to `True`, enable interceptor so it can be used in [`kernel_compile_commands`](#kernel_compile_commands).   |  `None` |
| <a id="kernel_build-kbuild_symtypes"></a>kbuild_symtypes |  The value of `KBUILD_SYMTYPES`.<br><br>This can be set to one of the following:<br><br>- `"true"` - `"false"` - `"auto"` - `None`, which defaults to `"auto"`<br><br>If the value is `"auto"`, it is determined by the `--kbuild_symtypes` flag.<br><br>If the value is `"true"`; or the value is `"auto"` and `--kbuild_symtypes` is specified, then `KBUILD_SYMTYPES=1`. **Note**: kernel build time can be significantly longer.<br><br>If the value is `"false"`; or the value is `"auto"` and `--kbuild_symtypes` is not specified, then `KBUILD_SYMTYPES=`.   |  `None` |
| <a id="kernel_build-toolchain_version"></a>toolchain_version |  [Nonconfigurable](https://bazel.build/reference/be/common-definitions#configurable-attributes). The toolchain version to depend on.   |  `None` |
| <a id="kernel_build-strip_modules"></a>strip_modules |  If `None` or not specified, default is `False`. If set to `True`, debug information for distributed modules is stripped.<br><br>This corresponds to negated value of `DO_NOT_STRIP_MODULES` in `build.config`.   |  `None` |
| <a id="kernel_build-module_signing_key"></a>module_signing_key |  A label referring to a module signing key.<br><br>This is to allow for dynamic setting of `CONFIG_MODULE_SIG_KEY` from Bazel.   |  `None` |
| <a id="kernel_build-system_trusted_key"></a>system_trusted_key |  A label referring to a trusted system key.<br><br>This is to allow for dynamic setting of `CONFIG_SYSTEM_TRUSTED_KEY` from Bazel.   |  `None` |
| <a id="kernel_build-modules_prepare_force_generate_headers"></a>modules_prepare_force_generate_headers |  If `True` it forces generation of additional headers as part of modules_prepare.   |  `None` |
| <a id="kernel_build-defconfig_fragments"></a>defconfig_fragments |  A list of targets that are applied to the defconfig.<br><br>As a convention, files should usually be named `<prop>_defconfig` (e.g. `kasan_defconfig`) or `<prop>_<value>_defconfig` (e.g. `lto_none_defconfig`) to provide human-readable hints during the build. The prefix should describe what the defconfig does. However, this is not a requirement. These configs are also applied to external modules, including `kernel_module`s and `ddk_module`s.<br><br>**NOTE**: `defconfig_fragments` are applied **after** `make defconfig`, similar to `POST_DEFCONFIG_CMDS`. If you migrate from `PRE_DEFCONFIG_CMDS` to `defconfig_fragments`, certain values may change; double check by building the `<target_name>_config` target and examining the generated `.config` file.   |  `None` |
| <a id="kernel_build-page_size"></a>page_size |  Default is `"default"`. Page size of the kernel build.<br><br>Value may be one of `"default"`, `"4k"`, `"16k"` or `"64k"`. If `"default"`, the defconfig is left as-is.<br><br>16k / 64k page size is only supported on `arch = "arm64"`.   |  `None` |
| <a id="kernel_build-pack_module_env"></a>pack_module_env |  If `True`, create `{name}_module_env.tar.gz` and other archives as part of the default output of this target.<br><br>These archives contains necessary files to build external modules.   |  `None` |
| <a id="kernel_build-sanitizers"></a>sanitizers |  **non-configurable**. A list of sanitizer configurations. By default, no sanitizers are explicity configured; values in defconfig are respected. Possible values are:   - `["kasan_any_mode"]`   - `["kasan_sw_tags"]`   - `["kasan_generic"]`   - `["kcsan"]`   |  `None` |
| <a id="kernel_build-ddk_module_defconfig_fragments"></a>ddk_module_defconfig_fragments |  A list of additional defconfigs, to be used in `ddk_module`s building against this kernel. Unlike `defconfig_fragments`, `ddk_module_defconfig_fragments` is not applied to this `kernel_build` target, nor dependent legacy `kernel_module`s.   |  `None` |
| <a id="kernel_build-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_dtstree"></a>

## kernel_dtstree

<pre>
kernel_dtstree(<a href="#kernel_dtstree-name">name</a>, <a href="#kernel_dtstree-srcs">srcs</a>, <a href="#kernel_dtstree-makefile">makefile</a>, <a href="#kernel_dtstree-kwargs">kwargs</a>)
</pre>

Specify a kernel DTS tree.

**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_dtstree-name"></a>name |  name of the module   |  none |
| <a id="kernel_dtstree-srcs"></a>srcs |  sources of the DTS tree. Default is<br><br><pre><code>glob(["**"], exclude = [&#10;    "**/.*",&#10;    "**/.*/**",&#10;    "**/BUILD.bazel",&#10;    "**/*.bzl",&#10;])</code></pre>   |  `None` |
| <a id="kernel_dtstree-makefile"></a>makefile |  Makefile of the DTS tree. Default is `:Makefile`, i.e. the `Makefile` at the root of the package.   |  `None` |
| <a id="kernel_dtstree-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_images"></a>

## kernel_images

<pre>
kernel_images(<a href="#kernel_images-name">name</a>, <a href="#kernel_images-kernel_modules_install">kernel_modules_install</a>, <a href="#kernel_images-kernel_build">kernel_build</a>, <a href="#kernel_images-base_kernel_images">base_kernel_images</a>, <a href="#kernel_images-build_initramfs">build_initramfs</a>,
              <a href="#kernel_images-build_vendor_dlkm">build_vendor_dlkm</a>, <a href="#kernel_images-build_boot">build_boot</a>, <a href="#kernel_images-build_vendor_boot">build_vendor_boot</a>, <a href="#kernel_images-build_vendor_kernel_boot">build_vendor_kernel_boot</a>,
              <a href="#kernel_images-build_system_dlkm">build_system_dlkm</a>, <a href="#kernel_images-build_system_dlkm_flatten">build_system_dlkm_flatten</a>, <a href="#kernel_images-build_dtbo">build_dtbo</a>, <a href="#kernel_images-dtbo_srcs">dtbo_srcs</a>, <a href="#kernel_images-dtbo_config">dtbo_config</a>,
              <a href="#kernel_images-mkbootimg">mkbootimg</a>, <a href="#kernel_images-deps">deps</a>, <a href="#kernel_images-boot_image_outs">boot_image_outs</a>, <a href="#kernel_images-modules_list">modules_list</a>, <a href="#kernel_images-modules_recovery_list">modules_recovery_list</a>,
              <a href="#kernel_images-modules_charger_list">modules_charger_list</a>, <a href="#kernel_images-modules_blocklist">modules_blocklist</a>, <a href="#kernel_images-modules_options">modules_options</a>, <a href="#kernel_images-vendor_ramdisk_binaries">vendor_ramdisk_binaries</a>,
              <a href="#kernel_images-vendor_ramdisk_dev_nodes">vendor_ramdisk_dev_nodes</a>, <a href="#kernel_images-system_dlkm_fs_type">system_dlkm_fs_type</a>, <a href="#kernel_images-system_dlkm_fs_types">system_dlkm_fs_types</a>,
              <a href="#kernel_images-system_dlkm_modules_list">system_dlkm_modules_list</a>, <a href="#kernel_images-system_dlkm_modules_blocklist">system_dlkm_modules_blocklist</a>, <a href="#kernel_images-system_dlkm_props">system_dlkm_props</a>,
              <a href="#kernel_images-vendor_dlkm_archive">vendor_dlkm_archive</a>, <a href="#kernel_images-vendor_dlkm_etc_files">vendor_dlkm_etc_files</a>, <a href="#kernel_images-vendor_dlkm_fs_type">vendor_dlkm_fs_type</a>,
              <a href="#kernel_images-vendor_dlkm_modules_list">vendor_dlkm_modules_list</a>, <a href="#kernel_images-vendor_dlkm_modules_blocklist">vendor_dlkm_modules_blocklist</a>, <a href="#kernel_images-vendor_dlkm_props">vendor_dlkm_props</a>,
              <a href="#kernel_images-ramdisk_compression">ramdisk_compression</a>, <a href="#kernel_images-ramdisk_compression_args">ramdisk_compression_args</a>, <a href="#kernel_images-unpack_ramdisk">unpack_ramdisk</a>, <a href="#kernel_images-avb_sign_boot_img">avb_sign_boot_img</a>,
              <a href="#kernel_images-avb_boot_partition_size">avb_boot_partition_size</a>, <a href="#kernel_images-avb_boot_key">avb_boot_key</a>, <a href="#kernel_images-avb_boot_algorithm">avb_boot_algorithm</a>, <a href="#kernel_images-avb_boot_partition_name">avb_boot_partition_name</a>,
              <a href="#kernel_images-dedup_dlkm_modules">dedup_dlkm_modules</a>, <a href="#kernel_images-create_modules_order">create_modules_order</a>, <a href="#kernel_images-kwargs">kwargs</a>)
</pre>

Build multiple kernel images.

You may use `filegroup.output_group` to request certain files. Example:

```
kernel_images(
    name = "my_images",
    build_vendor_dlkm = True,
)
filegroup(
    name = "my_vendor_dlkm",
    srcs = [":my_images"],
    output_group = "vendor_dlkm.img",
)
```

Allowed strings in `filegroup.output_group`:
* `vendor_dlkm.img`, if `build_vendor_dlkm` is set
* `system_dlkm.img`, if `build_system_dlkm` and `system_dlkm_fs_type` is set
* `system_dlkm.<type>.img` for each of `system_dlkm_fs_types`, if
    `build_system_dlkm` is set and `system_dlkm_fs_types` is not empty.
* `system_dlkm.flatten.<type>.img` for each of `sytem_dlkm_fs_types, if
    `build_system_dlkm_flatten` is set and `system_dlkm_fs_types` is not empty.

If no output files are found, the filegroup resolves to an empty one.
You may also read `OutputGroupInfo` on the `kernel_images` rule directly
in your rule implementation.

For details, see
[Requesting output files](https://bazel.build/extending/rules#requesting_output_files).


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_images-name"></a>name |  name of this rule, e.g. `kernel_images`,   |  none |
| <a id="kernel_images-kernel_modules_install"></a>kernel_modules_install |  A `kernel_modules_install` rule.<br><br>The main kernel build is inferred from the `kernel_build` attribute of the specified `kernel_modules_install` rule. The main kernel build must contain `System.map` in `outs` (which is included if you use `DEFAULT_GKI_OUTS` or `X86_64_OUTS` from `common_kernels.bzl`).   |  none |
| <a id="kernel_images-kernel_build"></a>kernel_build |  A `kernel_build` rule. Must specify if `build_boot`.   |  `None` |
| <a id="kernel_images-base_kernel_images"></a>base_kernel_images |  The `kernel_images()` corresponding to the `base_kernel` of the `kernel_build`. This is required for building a device-specific `system_dlkm` image. For example, if `base_kernel` of `kernel_build()` is `//common:kernel_aarch64`, then `base_kernel_images` is `//common:kernel_aarch64_images`.<br><br>This is also required if `dedup_dlkm_modules and not build_system_dlkm`.   |  `None` |
| <a id="kernel_images-build_initramfs"></a>build_initramfs |  Whether to build initramfs. Keep in sync with `BUILD_INITRAMFS`.   |  `None` |
| <a id="kernel_images-build_vendor_dlkm"></a>build_vendor_dlkm |  Whether to build `vendor_dlkm` image. It must be set if `vendor_dlkm_modules_list` is set.<br><br>Note: at the time of writing (Jan 2022), `vendor_dlkm.modules.blocklist` is **always** created regardless of the value of `VENDOR_DLKM_MODULES_BLOCKLIST`. If `build_vendor_dlkm()` in `build_utils.sh` does not generate `vendor_dlkm.modules.blocklist`, an empty file is created.   |  `None` |
| <a id="kernel_images-build_boot"></a>build_boot |  Whether to build boot image. It must be set if either `BUILD_BOOT_IMG` or `BUILD_VENDOR_BOOT_IMG` is set.<br><br>This depends on `kernel_build`. Hence, if this is set to `True`, `kernel_build` must be set.<br><br>If `True`, adds `boot.img` to `boot_image_outs` if not already in the list.   |  `None` |
| <a id="kernel_images-build_vendor_boot"></a>build_vendor_boot |  Whether to build `vendor_boot.img`. It must be set if either `BUILD_BOOT_IMG` or `BUILD_VENDOR_BOOT_IMG` is set, and `SKIP_VENDOR_BOOT` is not set, and `BUILD_VENDOR_KERNEL_BOOT` is not set.<br><br>At most **one** of `build_vendor_boot` and `build_vendor_kernel_boot` may be set to `True`.<br><br>If `True`, adds `vendor_boot.img` to `boot_image_outs` if not already in the list.   |  `None` |
| <a id="kernel_images-build_vendor_kernel_boot"></a>build_vendor_kernel_boot |  Whether to build `vendor_kernel_boot.img`. It must be set if either `BUILD_BOOT_IMG` or `BUILD_VENDOR_BOOT_IMG` is set, and `SKIP_VENDOR_BOOT` is not set, and `BUILD_VENDOR_KERNEL_BOOT` is set.<br><br>At most **one** of `build_vendor_boot` and `build_vendor_kernel_boot` may be set to `True`.<br><br>If `True`, adds `vendor_kernel_boot.img` to `boot_image_outs` if not already in the list.   |  `None` |
| <a id="kernel_images-build_system_dlkm"></a>build_system_dlkm |  Whether to build system_dlkm.img an image with GKI modules.   |  `None` |
| <a id="kernel_images-build_system_dlkm_flatten"></a>build_system_dlkm_flatten |  Whether to build system_dlkm.flatten.<fs>.img. This image have directory structure as `/lib/modules/*.ko` i.e. no `uname -r` in the path.   |  `None` |
| <a id="kernel_images-build_dtbo"></a>build_dtbo |  Whether to build dtbo image. Keep this in sync with `BUILD_DTBO_IMG`.<br><br>If `dtbo_srcs` is non-empty, `build_dtbo` is `True` by default. Otherwise it is `False` by default.   |  `None` |
| <a id="kernel_images-dtbo_srcs"></a>dtbo_srcs |  list of `*.dtbo` files used to package the `dtbo.img`. Keep this in sync with `MKDTIMG_DTBOS`; see example below.<br><br>If `dtbo_srcs` is non-empty, `build_dtbo` must not be explicitly set to `False`.<br><br>Example: <pre><code>kernel_build(&#10;    name = "tuna_kernel",&#10;    outs = [&#10;        "path/to/foo.dtbo",&#10;        "path/to/bar.dtbo",&#10;    ],&#10;)&#10;kernel_images(&#10;    name = "tuna_images",&#10;    kernel_build = ":tuna_kernel",&#10;    dtbo_srcs = [&#10;        ":tuna_kernel/path/to/foo.dtbo",&#10;        ":tuna_kernel/path/to/bar.dtbo",&#10;    ]&#10;)</code></pre>   |  `None` |
| <a id="kernel_images-dtbo_config"></a>dtbo_config |  a config file to create dtbo image by cfg_create command.   |  `None` |
| <a id="kernel_images-mkbootimg"></a>mkbootimg |  Path to the mkbootimg.py script which builds boot.img. Only used if `build_boot`. If `None`, default to `//tools/mkbootimg:mkbootimg.py`. NOTE: This overrides `MKBOOTIMG_PATH`.   |  `None` |
| <a id="kernel_images-deps"></a>deps |  Additional dependencies to build images.<br><br>This must include the following: - For `initramfs`:   - The file specified by `MODULES_LIST`   - The file specified by `MODULES_BLOCKLIST`, if `MODULES_BLOCKLIST` is set   - The file containing the list of modules needed for booting into recovery.   - The file containing the list of modules needed for booting into charger mode. - For `vendor_dlkm` image:   - The file specified by `VENDOR_DLKM_MODULES_LIST`   - The file specified by `VENDOR_DLKM_MODULES_BLOCKLIST`, if set   - The file specified by `VENDOR_DLKM_PROPS`, if set   - The file specified by `selinux_fc` in `VENDOR_DLKM_PROPS`, if set   |  `None` |
| <a id="kernel_images-boot_image_outs"></a>boot_image_outs |  A list of output files that will be installed to `DIST_DIR` when `build_boot_images` in `build/kernel/build_utils.sh` is executed.<br><br>You may leave out `vendor_boot.img` from the list. It is automatically added when `build_vendor_boot = True`.<br><br>If `build_boot` is equal to `False`, the default is empty.<br><br>If `build_boot` is equal to `True`, the default list assumes the following: - `BOOT_IMAGE_FILENAME` is not set (which takes default value `boot.img`), or is set to   `"boot.img"` - `vendor_boot.img` if `build_vendor_boot` - `RAMDISK_EXT=lz4`. Is used when `ramdisk_compression`(see below) is not specified. - `BOOT_IMAGE_HEADER_VERSION >= 4`, which creates `vendor-bootconfig.img` to contain   `VENDOR_BOOTCONFIG if `build_vendor_boot`. - The list contains `dtb.img`   |  `None` |
| <a id="kernel_images-modules_list"></a>modules_list |  A file containing list of modules to use for `vendor_boot.modules.load`.   |  `None` |
| <a id="kernel_images-modules_recovery_list"></a>modules_recovery_list |  A file containing a list of modules to load when booting into recovery.   |  `None` |
| <a id="kernel_images-modules_charger_list"></a>modules_charger_list |  A file containing a list of modules to load when booting into charger mode.   |  `None` |
| <a id="kernel_images-modules_blocklist"></a>modules_blocklist |  A file containing a list of modules which are blocked from being loaded.<br><br>This file is copied directly to staging directory, and should be in the format: <pre><code>blocklist module_name</code></pre>   |  `None` |
| <a id="kernel_images-modules_options"></a>modules_options |  Label to a file copied to `/lib/modules/<kernel_version>/modules.options` on the ramdisk.<br><br>Lines in the file should be of the form: <pre><code>options &lt;modulename&gt; &lt;param1&gt;=&lt;val&gt; &lt;param2&gt;=&lt;val&gt; ...</code></pre>   |  `None` |
| <a id="kernel_images-vendor_ramdisk_binaries"></a>vendor_ramdisk_binaries |  List of vendor ramdisk binaries which includes the device-specific components of ramdisk like the fstab file and the device-specific rc files. If specifying multiple vendor ramdisks and identical file paths exist in the ramdisks, the file from last ramdisk is used.<br><br>Note: **order matters**. To prevent buildifier from sorting the list, add the following: <pre><code># do not sort</code></pre>   |  `None` |
| <a id="kernel_images-vendor_ramdisk_dev_nodes"></a>vendor_ramdisk_dev_nodes |  List of dev nodes description files which describes special device files to be added to the vendor ramdisk. File format is as accepted by mkbootfs.   |  `None` |
| <a id="kernel_images-system_dlkm_fs_type"></a>system_dlkm_fs_type |  Deprecated. Use `system_dlkm_fs_types` instead.<br><br>Supported filesystems for `system_dlkm` image are `ext4` and `erofs`. Defaults to `ext4` if not specified.   |  `None` |
| <a id="kernel_images-system_dlkm_fs_types"></a>system_dlkm_fs_types |  List of file systems type for `system_dlkm` images.<br><br>Supported filesystems for `system_dlkm` image are `ext4` and `erofs`. If not specified, builds `system_dlkm.img` with ext4 else builds `system_dlkm.<fs>.img` for each file system type in the list.   |  `None` |
| <a id="kernel_images-system_dlkm_modules_list"></a>system_dlkm_modules_list |  location of an optional file containing the list of kernel modules which shall be copied into a system_dlkm partition image.   |  `None` |
| <a id="kernel_images-system_dlkm_modules_blocklist"></a>system_dlkm_modules_blocklist |  location of an optional file containing a list of modules which are blocked from being loaded.<br><br>This file is copied directly to the staging directory and should be in the format: <pre><code>blocklist module_name</code></pre>   |  `None` |
| <a id="kernel_images-system_dlkm_props"></a>system_dlkm_props |  location of a text file containing the properties to be used for creation of a `system_dlkm` image (filesystem, partition size, etc). If this is not set (and `build_system_dlkm` is), a default set of properties will be used which assumes an ext4 filesystem and a dynamic partition.   |  `None` |
| <a id="kernel_images-vendor_dlkm_archive"></a>vendor_dlkm_archive |  If set, enable archiving the vendor_dlkm staging directory.   |  `None` |
| <a id="kernel_images-vendor_dlkm_etc_files"></a>vendor_dlkm_etc_files |  Files that need to be copied to `vendor_dlkm.img` etc/ directory.   |  `None` |
| <a id="kernel_images-vendor_dlkm_fs_type"></a>vendor_dlkm_fs_type |  Supported filesystems for `vendor_dlkm.img` are `ext4` and `erofs`. Defaults to `ext4` if not specified.   |  `None` |
| <a id="kernel_images-vendor_dlkm_modules_list"></a>vendor_dlkm_modules_list |  location of an optional file containing the list of kernel modules which shall be copied into a `vendor_dlkm` partition image. Any modules passed into `MODULES_LIST` which become part of the `vendor_boot.modules.load` will be trimmed from the `vendor_dlkm.modules.load`.   |  `None` |
| <a id="kernel_images-vendor_dlkm_modules_blocklist"></a>vendor_dlkm_modules_blocklist |  location of an optional file containing a list of modules which are blocked from being loaded.<br><br>This file is copied directly to the staging directory and should be in the format: <pre><code>blocklist module_name</code></pre>   |  `None` |
| <a id="kernel_images-vendor_dlkm_props"></a>vendor_dlkm_props |  location of a text file containing the properties to be used for creation of a `vendor_dlkm` image (filesystem, partition size, etc). If this is not set (and `build_vendor_dlkm` is), a default set of properties will be used which assumes an ext4 filesystem and a dynamic partition.   |  `None` |
| <a id="kernel_images-ramdisk_compression"></a>ramdisk_compression |  If provided it specfies the format used for any ramdisks generated. If not provided a fallback value from build.config is used. Possible values are `lz4`, `gzip`, None.   |  `None` |
| <a id="kernel_images-ramdisk_compression_args"></a>ramdisk_compression_args |  Command line arguments passed only to lz4 command to control compression level. It only has effect when used with `ramdisk_compression` equal to "lz4".   |  `None` |
| <a id="kernel_images-unpack_ramdisk"></a>unpack_ramdisk |  When set to `False`, skips unpacking the vendor ramdisk and copy it as is, without modifications, into the boot image. Also skips the mkbootfs step.   |  `None` |
| <a id="kernel_images-avb_sign_boot_img"></a>avb_sign_boot_img |  If set to `True` signs the boot image using the avb_boot_key. The kernel prebuilt tool `avbtool` is used for signing.   |  `None` |
| <a id="kernel_images-avb_boot_partition_size"></a>avb_boot_partition_size |  Size of the boot partition in bytes. Used when `avb_sign_boot_img` is True.   |  `None` |
| <a id="kernel_images-avb_boot_key"></a>avb_boot_key |  Path to the key used for signing. Used when `avb_sign_boot_img` is True.   |  `None` |
| <a id="kernel_images-avb_boot_algorithm"></a>avb_boot_algorithm |  `avb_boot_key` algorithm used e.g. SHA256_RSA2048. Used when `avb_sign_boot_img` is True.   |  `None` |
| <a id="kernel_images-avb_boot_partition_name"></a>avb_boot_partition_name |  = Name of the boot partition. Used when `avb_sign_boot_img` is True.   |  `None` |
| <a id="kernel_images-dedup_dlkm_modules"></a>dedup_dlkm_modules |  If set, modules already in `system_dlkm` is excluded in `vendor_dlkm.modules.load`. Modules in `vendor_dlkm` is allowed to link to modules in `system_dlkm`.<br><br>The `system_dlkm` image is defined by the following:<br><br>- If `build_system_dlkm` is set, the `system_dlkm` image built by   this rule. - If `build_system_dlkm` is not set, the `system_dlkm` image in   `base_kernel_images`. If `base_kernel_images` is not set, build   fails.<br><br>If set, **additional changes in the userspace is required** so that `system_dlkm` modules are loaded before `vendor_dlkm` modules.   |  `None` |
| <a id="kernel_images-create_modules_order"></a>create_modules_order |  Whether to create and keep a modules.order file generated by a postorder traversal of the `kernel_modules_install` sources. It applies to building `initramfs` and `vendor_dlkm`.   |  `None` |
| <a id="kernel_images-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_module"></a>

## kernel_module

<pre>
kernel_module(<a href="#kernel_module-name">name</a>, <a href="#kernel_module-kernel_build">kernel_build</a>, <a href="#kernel_module-outs">outs</a>, <a href="#kernel_module-srcs">srcs</a>, <a href="#kernel_module-deps">deps</a>, <a href="#kernel_module-makefile">makefile</a>, <a href="#kernel_module-generate_btf">generate_btf</a>, <a href="#kernel_module-kwargs">kwargs</a>)
</pre>

Generates a rule that builds an external kernel module.

Example:
```
kernel_module(
    name = "nfc",
    srcs = glob([
        "**/*.c",
        "**/*.h",

        # If there are Kbuild files, add them
        "**/Kbuild",
        # If there are additional makefiles in subdirectories, add them
        "**/Makefile",
    ]),
    outs = ["nfc.ko"],
    kernel_build = "//common:kernel_aarch64",
)
```


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_module-name"></a>name |  Name of this kernel module.   |  none |
| <a id="kernel_module-kernel_build"></a>kernel_build |  Label referring to the kernel_build module.   |  none |
| <a id="kernel_module-outs"></a>outs |  The expected output files. If unspecified or value is `None`, it is `["{name}.ko"]` by default.<br><br>For each token `out`, the build rule automatically finds a file named `out` in the legacy kernel modules staging directory. The file is copied to the output directory of this package, with the label `name/out`.<br><br>- If `out` doesn't contain a slash, subdirectories are searched.<br><br>  Example: <pre><code>  kernel_module(name = "nfc", outs = ["nfc.ko"])</code></pre><br><br>  The build system copies <pre><code>  &lt;legacy modules staging dir&gt;/lib/modules/*/extra/&lt;some subdir&gt;/nfc.ko</code></pre>   to <pre><code>  &lt;package output dir&gt;/nfc.ko</code></pre><br><br>  `nfc/nfc.ko` is the label to the file.<br><br>- If `out` contains slashes, its value is used. The file is   also copied to the top of package output directory.<br><br>  For example: <pre><code>  kernel_module(name = "nfc", outs = ["foo/nfc.ko"])</code></pre><br><br>  The build system copies <pre><code>  &lt;legacy modules staging dir&gt;/lib/modules/*/extra/foo/nfc.ko</code></pre>   to <pre><code>  foo/nfc.ko</code></pre><br><br>  `nfc/foo/nfc.ko` is the label to the file.<br><br>  The file is also copied to `<package output dir>/nfc.ko`.<br><br>  `nfc/nfc.ko` is the label to the file.<br><br>  See `search_and_cp_output.py` for details.   |  `None` |
| <a id="kernel_module-srcs"></a>srcs |  Source files to build this kernel module. If unspecified or value is `None`, it is by default the list in the above example: <pre><code>glob([&#10;  "**/*.c",&#10;  "**/*.h",&#10;  "**/Kbuild",&#10;  "**/Makefile",&#10;])</code></pre>   |  `None` |
| <a id="kernel_module-deps"></a>deps |  A list of other `kernel_module` or `ddk_module` dependencies.<br><br>Before building this target, `Modules.symvers` from the targets in `deps` are restored, so this target can be built against them.<br><br>It is an undefined behavior to put targets of other types to this list (e.g. `ddk_headers`).   |  `None` |
| <a id="kernel_module-makefile"></a>makefile |  `Makefile` for the module. By default, this is `Makefile` in the current package.<br><br>This file determines where `make modules` is executed.<br><br>This is useful when the Makefile is located in a different package or in a subdirectory.   |  `None` |
| <a id="kernel_module-generate_btf"></a>generate_btf |  Allows generation of BTF type information for the module. If enabled, passes `vmlinux` image to module build, which is required by BTF generator makefile scripts.<br><br>Disabled by default.<br><br>Requires `CONFIG_DEBUG_INFO_BTF` enabled in base kernel.<br><br>Requires rebuild of module if `vmlinux` changed, which may lead to an increase of incremental build time.<br><br>BTF type information increases size of module binary.   |  `None` |
| <a id="kernel_module-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_module_test"></a>

## kernel_module_test

<pre>
kernel_module_test(<a href="#kernel_module_test-name">name</a>, <a href="#kernel_module_test-modules">modules</a>, <a href="#kernel_module_test-kwargs">kwargs</a>)
</pre>

A test on artifacts produced by [kernel_module](kernel.md#kernel_module).

**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_module_test-name"></a>name |  name of test   |  none |
| <a id="kernel_module_test-modules"></a>modules |  The list of `*.ko` kernel modules, or targets that produces `*.ko` kernel modules (e.g. [kernel_module](kernel.md#kernel_module)).   |  `None` |
| <a id="kernel_module_test-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


<a id="kernel_uapi_headers_cc_library"></a>

## kernel_uapi_headers_cc_library

<pre>
kernel_uapi_headers_cc_library(<a href="#kernel_uapi_headers_cc_library-name">name</a>, <a href="#kernel_uapi_headers_cc_library-kernel_build">kernel_build</a>)
</pre>

Defines a native cc_library based on a kernel's UAPI headers.

Example:

```
kernel_uapi_headers_cc_library(
    name = "uapi_header_lib",
    kernel_build = "//common:kernel_aarch64",
)

cc_binary(
    name = "foo",
    srcs = ["foo.c"],
    deps = [":uapi_header_lib"],
)
```

Internally, the `kernel_build`'s UAPI header output tarball is unpacked. Then, a header-only
[`cc_library`](https://bazel.build/reference/be/c-cpp#cc_library) is generated. This allows
other native Bazel C/C++ rules to add the kernel's UAPI headers as a dependency.

The library will automatically include the header directory in the dependent build, so source
files are free to simply include the UAPI headers they need.

Note: the `kernel_build`'s output UAPI header tarball includes the `usr/include` prefix. The
prefix is stripped while creating this library. To include the file `usr/include/linux/time.h`
from the tarball, a source file would `#include <linux/time.h>`.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="kernel_uapi_headers_cc_library-name"></a>name |  Name of target.   |  none |
| <a id="kernel_uapi_headers_cc_library-kernel_build"></a>kernel_build |  [`kernel_build`](#kernel_build)   |  none |


