<!-- Generated with Stardoc: http://skydoc.bazel.build -->



<a id="print_debug"></a>

## print_debug

<pre>
print_debug(<a href="#print_debug-name">name</a>, <a href="#print_debug-content">content</a>, <a href="#print_debug-prefix_label">prefix_label</a>)
</pre>

A rule that prints a debug string when built. No outputs are generated.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="print_debug-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="print_debug-content"></a>content |  The string to print.   | String | optional |  `""`  |
| <a id="print_debug-prefix_label"></a>prefix_label |  Prefix with the label of this target.   | Boolean | optional |  `True`  |


