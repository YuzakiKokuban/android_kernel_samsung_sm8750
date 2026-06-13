# API Reference and Documentation for all rules

## For the current branch

You may find the documentation for the aforementioned Bazel rules and macros in
the [api_reference](api_reference) directory.

Use your favorite Markdown renderer to view the files locally.

* If you are using VS Code, see
  [instructions](https://code.visualstudio.com/docs/languages/markdown#_markdown-preview)
  for previewing Markdown files.

## View online

You may also view the API reference for `android-mainline` online by using
the links below.

**Note**: Due to known bugs in Markdown rendering, links to anchors may
or may not work.

<!-- Ref: b/327647132 -->

- To view the API reference on Code Search, visit
  [https://cs.android.com/android/kernel/superproject/+/common-android-mainline:build/kernel/kleaf/docs/api_reference/](https://cs.android.com/android/kernel/superproject/+/common-android-mainline:build/kernel/kleaf/docs/api_reference/)
- To view the API reference on Gitiles, visit
  [https://android.googlesource.com/kernel/build/+/refs/heads/main/kleaf/docs/api_reference](https://android.googlesource.com/kernel/build/+/refs/heads/main/kleaf/docs/api_reference).

## Updating docs

```sh
tools/bazel run --config=internet //build/kernel/kleaf/docs:docs_dist
```
