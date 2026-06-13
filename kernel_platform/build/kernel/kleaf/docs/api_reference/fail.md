<!-- Generated with Stardoc: http://skydoc.bazel.build -->

A rule that fails.

<a id="fail_action"></a>

## fail_action

<pre>
fail_action(<a href="#fail_action-name">name</a>, <a href="#fail_action-message">message</a>)
</pre>

A rule that fails at execution phase

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="fail_action-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="fail_action-message"></a>message |  fail message   | String | optional |  `""`  |


<a id="fail_rule"></a>

## fail_rule

<pre>
fail_rule(<a href="#fail_rule-name">name</a>, <a href="#fail_rule-message">message</a>)
</pre>

A rule that fails at analysis phase

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="fail_rule-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="fail_rule-message"></a>message |  fail message   | String | optional |  `""`  |


