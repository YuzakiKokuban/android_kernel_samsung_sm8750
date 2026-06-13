# Copyright (C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
 Test Ramdisk Options.
"""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//build/kernel/kleaf:kernel.bzl", "kernel_build", "kernel_modules_install")
load("//build/kernel/kleaf/impl:image/kernel_images.bzl", "kernel_images")
load("//build/kernel/kleaf/tests:test_utils.bzl", "test_utils")

# Check effect of ramdisk_options -- compress format and arguments.
def _initramfs_test_impl(ctx):
    env = analysistest.begin(ctx)

    action = test_utils.find_action(env, "Initramfs")
    script = test_utils.get_shell_script(env, action)
    found_compress_args = ctx.attr.expected_compress_args in script

    asserts.equals(
        env,
        actual = found_compress_args,
        expected = True,
        msg = "expected_compress_args = {} not found.".format(
            ctx.attr.expected_compress_args,
        ),
    )
    return analysistest.end(env)

_initramfs_test = analysistest.make(
    impl = _initramfs_test_impl,
    attrs = {
        "expected_compress_args": attr.string(),
    },
)

# Check effect of ramdisk_options -- ramdisk extension in boot image.
def _boot_image_test_impl(ctx):
    env = analysistest.begin(ctx)
    file_expected = "ramdisk.{}".format(ctx.attr.expected_compress_ext)

    action = test_utils.find_action(env, "BootImages")
    script = test_utils.get_shell_script(env, action)
    found_file = file_expected in script

    asserts.equals(
        env,
        actual = found_file,
        expected = True,
        msg = "file_expected = {} not found.".format(file_expected),
    )
    return analysistest.end(env)

_boot_image_test = analysistest.make(
    impl = _boot_image_test_impl,
    attrs = {
        "expected_compress_ext": attr.string(),
    },
)

# Check effect of avb_sign_boot_img and avb_boot_*.
def _avb_sign_test_impl(ctx):
    env = analysistest.begin(ctx)
    action = test_utils.find_action(env, "BootImages")
    script = test_utils.get_shell_script(env, action)
    found_all = all([env_val in script for env_val in ctx.attr.expected_env_values])
    found_all = True
    for env_val in ctx.attr.expected_env_values:
        found_all = found_all and env_val in script
    asserts.equals(
        env,
        actual = found_all,
        expected = True,
        msg = "expected_env_values = {} not found.".format(
            ctx.attr.expected_env_values,
        ),
    )
    return analysistest.end(env)

_avb_sign_test = analysistest.make(
    impl = _avb_sign_test_impl,
    attrs = {
        "expected_env_values": attr.string_list(),
    },
)

def initramfs_test(name):
    """Define tests for `ramdisk_options`.

    Args:
      name: Name of this test suite.
    """

    # Test setup
    kernel_build(
        name = name + "build",
        build_config = "build.config.fake",
        outs = [
            # This is a requirement (for more, see initramfs.bzl).
            "System.map",
        ],
        tags = ["manual"],
    )
    kernel_modules_install(
        name = name + "modules_install",
        kernel_build = name + "build",
        tags = ["manual"],
    )

    tests = []

    # Fallback to config values.
    kernel_images(
        name = name + "fallback_image",
        kernel_modules_install = name + "modules_install",
        build_initramfs = True,
        kernel_build = name + "build",
        build_boot = True,
        tags = ["manual"],
    )
    _initramfs_test(
        name = name + "fallback_test",
        target_under_test = name + "fallback_image_initramfs",
        expected_compress_args = "${RAMDISK_COMPRESS}",
    )
    tests.append(name + "fallback_test")
    _boot_image_test(
        name = name + "fallback_boot_test",
        target_under_test = name + "fallback_image_boot_images",
        expected_compress_ext = "lz4",
    )
    tests.append(name + "fallback_boot_test")

    # Explicitly using GZIP
    kernel_images(
        name = name + "gzip_image",
        kernel_modules_install = name + "modules_install",
        build_initramfs = True,
        kernel_build = name + "build",
        build_boot = True,
        ramdisk_compression = "gzip",
        tags = ["manual"],
    )
    _initramfs_test(
        name = name + "gzip_test",
        target_under_test = name + "gzip_image_initramfs",
        expected_compress_args = "gzip -c -f",
    )
    tests.append(name + "gzip_test")
    _boot_image_test(
        name = name + "gzip_boot_test",
        target_under_test = name + "gzip_image_boot_images",
        expected_compress_ext = "gz",
    )
    tests.append(name + "gzip_boot_test")

    # Explicitly using LZ4 with default arguments
    kernel_images(
        name = name + "lz4_default_image",
        kernel_modules_install = name + "modules_install",
        build_initramfs = True,
        kernel_build = name + "build",
        build_boot = True,
        ramdisk_compression = "lz4",
        tags = ["manual"],
    )
    _initramfs_test(
        name = name + "lz4_default_test",
        target_under_test = name + "lz4_default_image_initramfs",
        expected_compress_args = "-12 --favor-decSpeed",
    )
    tests.append(name + "lz4_default_test")
    _boot_image_test(
        name = name + "lz4_default_boot_test",
        target_under_test = name + "lz4_default_image_boot_images",
        expected_compress_ext = "lz4",
    )
    tests.append(name + "lz4_default_boot_test")

    # Explicitly using LZ4 with custom arguments
    kernel_images(
        name = name + "lz4_custom_image",
        kernel_modules_install = name + "modules_install",
        build_initramfs = True,
        kernel_build = name + "build",
        build_boot = True,
        ramdisk_compression = "lz4",
        ramdisk_compression_args = "-foo --bar",
        tags = ["manual"],
    )
    _initramfs_test(
        name = name + "lz4_custom_test",
        target_under_test = name + "lz4_custom_image_initramfs",
        expected_compress_args = "-foo --bar",
    )
    tests.append(name + "lz4_custom_test")

    # Sign boot image using AVB.
    kernel_images(
        name = name + "sign_avb_image",
        kernel_modules_install = name + "modules_install",
        build_initramfs = True,
        kernel_build = name + "build",
        unpack_ramdisk = False,
        avb_sign_boot_img = True,
        avb_boot_partition_size = 512,
        avb_boot_key = "//tools/mkbootimg:gki/testdata/testkey_rsa4096.pem",
        avb_boot_algorithm = "SHA256_RSA4096",
        avb_boot_partition_name = "boot",
        tags = ["manual"],
    )
    _avb_sign_test(
        name = name + "sign_avb_image_test",
        target_under_test = name + "sign_avb_image_boot_images",
        expected_env_values = [
            "SKIP_UNPACKING_RAMDISK=1",
            "AVB_SIGN_BOOT_IMG=1",
            "AVB_BOOT_PARTITION_SIZE=512",
            "AVB_BOOT_KEY=",
            "AVB_BOOT_ALGORITHM=SHA256_RSA4096",
            "AVB_BOOT_PARTITION_NAME=boot",
        ],
    )
    tests.append(name + "sign_avb_image_test")

    native.test_suite(
        name = name,
        tests = tests,
    )
