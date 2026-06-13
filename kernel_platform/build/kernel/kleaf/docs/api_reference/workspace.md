<!-- Generated with Stardoc: http://skydoc.bazel.build -->

Defines repositories in a Kleaf workspace.

<a id="define_kleaf_workspace"></a>

## define_kleaf_workspace

<pre>
define_kleaf_workspace(<a href="#define_kleaf_workspace-common_kernel_package">common_kernel_package</a>, <a href="#define_kleaf_workspace-include_remote_java_tools_repo">include_remote_java_tools_repo</a>, <a href="#define_kleaf_workspace-artifact_url_fmt">artifact_url_fmt</a>)
</pre>

Common macro for defining repositories in a Kleaf workspace.

**This macro must only be called from `WORKSPACE` or `WORKSPACE.bazel`
files, not `BUILD` or `BUILD.bazel` files!**

If [`define_kleaf_workspace_epilog`](workspace_epilog.md#define_kleaf_workspace_epilog) is
called, it must be called after `define_kleaf_workspace` is called.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="define_kleaf_workspace-common_kernel_package"></a>common_kernel_package |  Default is `"@//common"`. The package to the common kernel source tree.<br><br>As a legacy behavior, if the provided string does not start with `@` or `//`, it is prepended with `@//`.<br><br>Do not provide the trailing `/`.   |  `None` |
| <a id="define_kleaf_workspace-include_remote_java_tools_repo"></a>include_remote_java_tools_repo |  Default is `False`. Whether to vendor two extra repositories: remote_java_tools and remote_java_tools_linux.<br><br>These respositories should exist under `//prebuilts/bazel/`   |  `False` |
| <a id="define_kleaf_workspace-artifact_url_fmt"></a>artifact_url_fmt |  API endpoint for Android CI artifacts. The format may include anchors for the following properties:   * {build_number}   * {target}   * {filename}   |  `None` |

**DEPRECATED**

The use of legacy WORKSPACE is deprecated. Please migrate to Bazel modules.
See [bzlmod.md](../bzlmod.md).


