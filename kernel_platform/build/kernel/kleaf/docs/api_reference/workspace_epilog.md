<!-- Generated with Stardoc: http://skydoc.bazel.build -->

Optional epilog macro for defining repositories in a Kleaf workspace.

<a id="define_kleaf_workspace_epilog"></a>

## define_kleaf_workspace_epilog

<pre>
define_kleaf_workspace_epilog()
</pre>

Optional epilog macro for defining repositories in a Kleaf workspace.

**This macro must only be called from `WORKSPACE` or `WORKSPACE.bazel`
files, not `BUILD` or `BUILD.bazel` files!**

The epilog macro is needed if you are running
[Bazel analysis tests](https://bazel.build/rules/testing).

If called, it must be called after
[`define_kleaf_workspace`](workspace.md#define_kleaf_workspace) is called.



