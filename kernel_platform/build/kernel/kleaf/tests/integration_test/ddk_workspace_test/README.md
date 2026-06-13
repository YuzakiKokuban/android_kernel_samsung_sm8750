# Experimental: DDKv2 tests

This directory contains a list of tests for setting up the DDK workspace.
For details, see [Setting up DDK workspace](../../../docs/ddk/workspace.md).

**Note**: Some tests uses hacks to work around kinks. These hacks
may be replaced by a better API in the future. **DO NOT** use these tests as a
reference to set up your DDK workspace ... yet.

## Running the test

```shell
$ tools/bazel run //build/kernel/kleaf/tests/integration_test \
    -- KleafIntegrationTestShard2.test_ddk_workspace_setup
```
