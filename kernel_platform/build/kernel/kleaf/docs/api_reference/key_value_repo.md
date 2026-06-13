<!-- Generated with Stardoc: http://skydoc.bazel.build -->

Turn a simple build.config into a Bazel extension.

<a id="key_value_repo"></a>

## key_value_repo

<pre>
key_value_repo(<a href="#key_value_repo-name">name</a>, <a href="#key_value_repo-srcs">srcs</a>, <a href="#key_value_repo-additional_values">additional_values</a>, <a href="#key_value_repo-repo_mapping">repo_mapping</a>)
</pre>

Exposes a Bazel repository with key value pairs defined from srcs.

Configuration files shall contain a single pair of key and value separated
by '='. Keys and values are stripped, hence whitespace characters around the
separator are allowed.

Example:
Given a file `common/build.config.constants` with content
```
    CLANG_VERSION=r433403
```

The workspace file can instantiate a repository rule with
```
load("//build/kernel/kleaf:key_value_repo.bzl", "key_value_repo")

key_value_repo(
    name = "kernel_toolchain_info",
    srcs = ["//common:build.config.constants"],
)
```

and users of the repository can refer to the values with
```
load("@kernel_toolchain_info//:dict.bzl", "CLANG_VERSION")
```

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="key_value_repo-name"></a>name |  A unique name for this repository.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="key_value_repo-srcs"></a>srcs |  Configuration files storing 'key=value' pairs.   | <a href="https://bazel.build/concepts/labels">List of labels</a> | required |  |
| <a id="key_value_repo-additional_values"></a>additional_values |  Additional values in `dict.bzl`   | <a href="https://bazel.build/rules/lib/dict">Dictionary: String -> String</a> | optional |  `{}`  |
| <a id="key_value_repo-repo_mapping"></a>repo_mapping |  In `WORKSPACE` context only: a dictionary from local repository name to global repository name. This allows controls over workspace dependency resolution for dependencies of this repository.<br><br>For example, an entry `"@foo": "@bar"` declares that, for any time this repository depends on `@foo` (such as a dependency on `@foo//some:target`, it should actually resolve that dependency within globally-declared `@bar` (`@bar//some:target`).<br><br>This attribute is _not_ supported in `MODULE.bazel` context (when invoking a repository rule inside a module extension's implementation function).   | <a href="https://bazel.build/rules/lib/dict">Dictionary: String -> String</a> | optional |  |


