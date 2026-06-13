<!-- Generated with Stardoc: http://skydoc.bazel.build -->

Provide tools for a hermetic build.

<a id="hermetic_exec"></a>

## hermetic_exec

<pre>
hermetic_exec(<a href="#hermetic_exec-name">name</a>, <a href="#hermetic_exec-script">script</a>, <a href="#hermetic_exec-data">data</a>, <a href="#hermetic_exec-kwargs">kwargs</a>)
</pre>

A exec that uses hermetic tools.

Hermetic tools are resolved from toolchain resolution. To replace it,
register a different hermetic toolchain.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="hermetic_exec-name"></a>name |  name of the target   |  none |
| <a id="hermetic_exec-script"></a>script |  See [exec.script]   |  none |
| <a id="hermetic_exec-data"></a>data |  See [exec.data]   |  `None` |
| <a id="hermetic_exec-kwargs"></a>kwargs |  See [exec]   |  none |


<a id="hermetic_exec_test"></a>

## hermetic_exec_test

<pre>
hermetic_exec_test(<a href="#hermetic_exec_test-name">name</a>, <a href="#hermetic_exec_test-script">script</a>, <a href="#hermetic_exec_test-data">data</a>, <a href="#hermetic_exec_test-kwargs">kwargs</a>)
</pre>

A exec_test that uses hermetic tools.

Hermetic tools are resolved from toolchain resolution. To replace it,
register a different hermetic toolchain.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="hermetic_exec_test-name"></a>name |  name of the target   |  none |
| <a id="hermetic_exec_test-script"></a>script |  See [exec_test.script]   |  none |
| <a id="hermetic_exec_test-data"></a>data |  See [exec_test.data]   |  `None` |
| <a id="hermetic_exec_test-kwargs"></a>kwargs |  See [exec_test]   |  none |


<a id="hermetic_genrule"></a>

## hermetic_genrule

<pre>
hermetic_genrule(<a href="#hermetic_genrule-name">name</a>, <a href="#hermetic_genrule-cmd">cmd</a>, <a href="#hermetic_genrule-tools">tools</a>, <a href="#hermetic_genrule-use_cc_toolchain">use_cc_toolchain</a>, <a href="#hermetic_genrule-kwargs">kwargs</a>)
</pre>

A genrule that uses hermetic tools.

Hermetic tools are resolved from toolchain resolution. To replace it,
register a different hermetic toolchain.

Only `cmd` is expected and used. `cmd_bash`, `cmd_ps`, `cmd_bat` etc. are
ignored.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="hermetic_genrule-name"></a>name |  name of the target   |  none |
| <a id="hermetic_genrule-cmd"></a>cmd |  See [genrule.cmd](https://bazel.build/reference/be/general#genrule.cmd)   |  none |
| <a id="hermetic_genrule-tools"></a>tools |  See [genrule.tools](https://bazel.build/reference/be/general#genrule.tools)   |  `None` |
| <a id="hermetic_genrule-use_cc_toolchain"></a>use_cc_toolchain |  When set to `True` resolved CC toolchain is accessible from the genrule.   |  `None` |
| <a id="hermetic_genrule-kwargs"></a>kwargs |  See [genrule](https://bazel.build/reference/be/general#genrule)   |  none |


<a id="hermetic_toolchain.get"></a>

## hermetic_toolchain.get

<pre>
hermetic_toolchain.get(<a href="#hermetic_toolchain.get-ctx">ctx</a>)
</pre>

Returns the resolved toolchain information.

**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="hermetic_toolchain.get-ctx"></a>ctx |  ctx. The rule must contain <pre><code>toolchains = [&#10;    hermetic_toolchain.type,&#10;]</code></pre>   |  none |

**RETURNS**

_HermeticToolchainInfo (see hermetic_tools.bzl).


<a id="hermetic_tools"></a>

## hermetic_tools

<pre>
hermetic_tools(<a href="#hermetic_tools-name">name</a>, <a href="#hermetic_tools-deps">deps</a>, <a href="#hermetic_tools-symlinks">symlinks</a>, <a href="#hermetic_tools-extra_args">extra_args</a>, <a href="#hermetic_tools-kwargs">kwargs</a>)
</pre>

Provide tools for a hermetic build.

**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="hermetic_tools-name"></a>name |  name of the target   |  none |
| <a id="hermetic_tools-deps"></a>deps |  additional dependencies. These aren't added to the `PATH`.   |  `None` |
| <a id="hermetic_tools-symlinks"></a>symlinks |  A dictionary, where keys are labels to an executable, and values are names to the tool, separated with `:`. e.g.<br><br><pre><code>{"//label/to:toybox": "cp:realpath"}</code></pre>   |  `None` |
| <a id="hermetic_tools-extra_args"></a>extra_args |  Keys are names to the tool (see `symlinks`). Values are extra arguments added to the tool at the end.   |  `None` |
| <a id="hermetic_tools-kwargs"></a>kwargs |  Additional attributes to the internal rule, e.g. [`visibility`](https://docs.bazel.build/versions/main/visibility.html). See complete list [here](https://docs.bazel.build/versions/main/be/common-definitions.html#common-attributes).   |  none |


