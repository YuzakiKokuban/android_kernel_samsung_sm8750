<!-- Generated with Stardoc: http://skydoc.bazel.build -->

When a directory created dy ctx.actions.declare_directory is referred to
in a sandbox, if it is empty, or a subdirectory of it is empty, the empty
directory won't be created in the sandbox.
These functions resolve the problem by also recording the directory structure
in a text file.

<a id="directory_with_structure.files"></a>

## directory_with_structure.files

<pre>
directory_with_structure.files(<a href="#directory_with_structure.files-directory_with_structure">directory_with_structure</a>)
</pre>

Return the list of declared [File](https://bazel.build/rules/lib/File) objects in a `directory_with_structure`.

**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="directory_with_structure.files-directory_with_structure"></a>directory_with_structure |  <p align="center"> - </p>   |  none |


<a id="directory_with_structure.isinstance"></a>

## directory_with_structure.isinstance

<pre>
directory_with_structure.isinstance(<a href="#directory_with_structure.isinstance-obj">obj</a>)
</pre>



**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="directory_with_structure.isinstance-obj"></a>obj |  <p align="center"> - </p>   |  none |


<a id="directory_with_structure.make"></a>

## directory_with_structure.make

<pre>
directory_with_structure.make(<a href="#directory_with_structure.make-ctx">ctx</a>, <a href="#directory_with_structure.make-filename">filename</a>)
</pre>

The replacement of [ctx.actions.declare_directory](https://bazel.build/rules/lib/actions#declare_directory) that also preserves empty directories.

Return a struct with the following fields:
    - `directory`: A [File](https://bazel.build/rules/lib/File) object from
      [ctx.actions.declare_directory](https://bazel.build/rules/lib/actions#declare_directory).
    - `structure_file`: A [File](https://bazel.build/rules/lib/File) object that will the
      directory structure.


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="directory_with_structure.make-ctx"></a>ctx |  ctx   |  none |
| <a id="directory_with_structure.make-filename"></a>filename |  See [ctx.actions.declare_directory](https://bazel.build/rules/lib/actions#declare_directory).   |  none |


<a id="directory_with_structure.record"></a>

## directory_with_structure.record

<pre>
directory_with_structure.record(<a href="#directory_with_structure.record-directory_with_structure">directory_with_structure</a>)
</pre>

Return a command that records the directory structure to the `structure_file`.

It is expected that the shell has properly set up [hermetic tools](hermetic_tools.md#hermetic_tools).


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="directory_with_structure.record-directory_with_structure"></a>directory_with_structure |  struct returned by [`directory_with_structure.declare`](#directory_with_structuredeclare).   |  none |


<a id="directory_with_structure.restore"></a>

## directory_with_structure.restore

<pre>
directory_with_structure.restore(<a href="#directory_with_structure.restore-directory_with_structure">directory_with_structure</a>, <a href="#directory_with_structure.restore-dst">dst</a>, <a href="#directory_with_structure.restore-options">options</a>)
</pre>

Return a command that restores a `directory_with_structure`.

It is expected that the shell has properly set up [hermetic tools](hermetic_tools.md#hermetic_tools).


**PARAMETERS**


| Name  | Description | Default Value |
| :------------- | :------------- | :------------- |
| <a id="directory_with_structure.restore-directory_with_structure"></a>directory_with_structure |  struct returned by `declare_directory_with_structure`.   |  none |
| <a id="directory_with_structure.restore-dst"></a>dst |  a string containing the path to the destination directory.   |  none |
| <a id="directory_with_structure.restore-options"></a>options |  a string containing options to `rsync`. If `None`, default to `"-a"`.   |  `None` |


