# Visualizing dependencies

This is a non exhaustive list of options to help understanding dependencies.

## Understanding kernel modules dependencies

Use the `dependency_graph` macro to create a diagram of dependencies between
 kernel modules. The dependencies are calculated via the exported symbols from
 each modules and are represented in DOT language so they can be rendered
 using a [Graphviz Server](https://graphviz.org/about/#viewers) or using the
[CLI](https://graphviz.org/doc/info/command.html) as follows.

<!-- TODO: After the change adding these targets land, reference the code here. -->
Provide the `kernel_build` and a list of `kernel_module`'s to analyze:

For example:
```python
dependency_graph(
    name = "virtual_device_x86_64_dependency_graph",
    colorful = True,
    kernel_build = ":virtual_device_x86_64",
    kernel_modules = [
        ":virtual_device_x86_64_external_modules",
    ],
)
```

Build the diagram by running:
```shell
$ tools/bazel build //common-modules/virtual-device:virtual_device_x86_64_dependency_graph
```

The generated file can be found at:
```
bazel-bin/common-modules/virtual-device/virtual_device_x86_64_dependency_graph_drawer/dependency_graph.dot
```

You can use this file to render the graph in `.png` or `.svg` format.

For example:
```shell
$ dot -Tsvg bazel-bin/common-modules/virtual-device/virtual_device_x86_64_dependency_graph_drawer/dependency_graph.dot
```

Optionally to reduce the graph density by eliminating transitive dependencies use the [tred]() utility:
```shell
$ tred bazel-bin/common-modules/virtual-device/virtual_device_x86_64_dependency_graph_drawer/dependency_graph.dot > reduced_graph.dot
```


<!-- TODO: Add section for Bazel dependencies. -->