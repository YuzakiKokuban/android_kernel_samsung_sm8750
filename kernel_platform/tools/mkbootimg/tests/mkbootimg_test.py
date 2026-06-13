#!/usr/bin/env python3
#
# Copyright 2020, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests mkbootimg and unpack_bootimg."""

import filecmp
import logging
import os
import random
import shlex
import subprocess
import sys
import tempfile
import unittest

BOOT_ARGS_OFFSET = 64
BOOT_ARGS_SIZE = 512
BOOT_EXTRA_ARGS_OFFSET = 608
BOOT_EXTRA_ARGS_SIZE = 1024
BOOT_V3_ARGS_OFFSET = 44
VENDOR_BOOT_ARGS_OFFSET = 28
VENDOR_BOOT_ARGS_SIZE = 2048

TEST_KERNEL_CMDLINE = (
    'printk.devkmsg=on firmware_class.path=/vendor/etc/ init=/init '
    'kfence.sample_interval=500 loop.max_part=7 bootconfig'
)


def generate_test_file(pathname, size, seed=None):
    """Generates a gibberish-filled test file and returns its pathname."""
    random.seed(os.path.basename(pathname) if seed is None else seed)
    with open(pathname, 'wb') as f:
        f.write(random.randbytes(size))
    return pathname


def subsequence_of(list1, list2):
    """Returns True if list1 is a subsequence of list2.

    >>> subsequence_of([], [1])
    True
    >>> subsequence_of([2, 4], [1, 2, 3, 4])
    True
    >>> subsequence_of([1, 2, 2], [1, 2, 3])
    False
    """
    if len(list1) == 0:
        return True
    if len(list2) == 0:
        return False
    if list1[0] == list2[0]:
        return subsequence_of(list1[1:], list2[1:])
    return subsequence_of(list1, list2[1:])


class MkbootimgTest(unittest.TestCase):
    """Tests the functionalities of mkbootimg and unpack_bootimg."""

    def setUp(self):
        # Saves the test executable directory so that relative path references
        # to test dependencies don't rely on being manually run from the
        # executable directory.
        # With this, we can just open "./tests/data/testkey_rsa2048.pem" in the
        # following tests with subprocess.run(..., cwd=self._exec_dir, ...).
        self._exec_dir = os.path.abspath(os.path.dirname(sys.argv[0]))

        self._avbtool_path = os.path.join(self._exec_dir, 'avbtool')

        # Set self.maxDiff to None to see full diff in assertion.
        # C0103: invalid-name for maxDiff.
        self.maxDiff = None  # pylint: disable=C0103

    def _test_legacy_boot_image_v4_signature(self, avbtool_path):
        """Tests the boot_signature in boot.img v4."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            kernel = generate_test_file(os.path.join(temp_out_dir, 'kernel'),
                                        0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--kernel', kernel,
                '--ramdisk', ramdisk,
                '--cmdline', TEST_KERNEL_CMDLINE,
                '--os_version', '11.0.0',
                '--os_patch_level', '2021-01',
                '--gki_signing_algorithm', 'SHA256_RSA2048',
                '--gki_signing_key', './tests/data/testkey_rsa2048.pem',
                '--gki_signing_signature_args',
                '--prop foo:bar --prop gki:nice',
                '--output', boot_img,
            ]

            if avbtool_path:
                mkbootimg_cmds.extend(
                    ['--gki_signing_avbtool_path', avbtool_path])

            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
            ]

            # cwd=self._exec_dir is required to read
            # ./tests/data/testkey_rsa2048.pem for --gki_signing_key.
            subprocess.run(mkbootimg_cmds, check=True, cwd=self._exec_dir)
            subprocess.run(unpack_bootimg_cmds, check=True)

            # Checks the content of the boot signature.
            expected_boot_signature_info = (
                'Minimum libavb version:   1.0\n'
                'Header Block:             256 bytes\n'
                'Authentication Block:     320 bytes\n'
                'Auxiliary Block:          832 bytes\n'
                'Public key (sha1):        '
                'cdbb77177f731920bbe0a0f94f84d9038ae0617d\n'
                'Algorithm:                SHA256_RSA2048\n'
                'Rollback Index:           0\n'
                'Flags:                    0\n'
                'Rollback Index Location:  0\n'
                "Release String:           'avbtool 1.3.0'\n"
                'Descriptors:\n'
                '    Hash descriptor:\n'
                '      Image Size:            12288 bytes\n'
                '      Hash Algorithm:        sha256\n'
                '      Partition Name:        boot\n'
                '      Salt:                  d00df00d\n'
                '      Digest:                '
                'cf3755630856f23ab70e501900050fee'
                'f30b633b3e82a9085a578617e344f9c7\n'
                '      Flags:                 0\n'
                "    Prop: foo -> 'bar'\n"
                "    Prop: gki -> 'nice'\n"
            )

            avbtool_info_cmds = [
                # use avbtool_path if it is not None.
                avbtool_path or 'avbtool',
                'info_image', '--image',
                os.path.join(temp_out_dir, 'out', 'boot_signature')
            ]
            result = subprocess.run(avbtool_info_cmds, check=True,
                                    capture_output=True, encoding='utf-8')

            self.assertEqual(result.stdout, expected_boot_signature_info)

    def test_legacy_boot_image_v4_signature_without_avbtool_path(self):
        """Boot signature generation without --gki_signing_avbtool_path."""
        self._test_legacy_boot_image_v4_signature(avbtool_path=None)

    def test_legacy_boot_image_v4_signature_with_avbtool_path(self):
        """Boot signature generation with --gki_signing_avbtool_path."""
        self._test_legacy_boot_image_v4_signature(
            avbtool_path=self._avbtool_path)

    def test_legacy_boot_image_v4_signature_exceed_size(self):
        """Tests the boot signature size exceeded in a boot image version 4."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            kernel = generate_test_file(os.path.join(temp_out_dir, 'kernel'),
                                        0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--kernel', kernel,
                '--ramdisk', ramdisk,
                '--cmdline', TEST_KERNEL_CMDLINE,
                '--os_version', '11.0.0',
                '--os_patch_level', '2021-01',
                '--gki_signing_avbtool_path', self._avbtool_path,
                '--gki_signing_algorithm', 'SHA256_RSA2048',
                '--gki_signing_key', './tests/data/testkey_rsa2048.pem',
                '--gki_signing_signature_args',
                # Makes it exceed the signature max size.
                '--prop foo:bar --prop gki:nice ' * 64,
                '--output', boot_img,
            ]

            # cwd=self._exec_dir is required to read
            # ./tests/data/testkey_rsa2048.pem for --gki_signing_key.
            try:
                subprocess.run(mkbootimg_cmds, check=True, capture_output=True,
                               cwd=self._exec_dir, encoding='utf-8')
                self.fail('Exceeding signature size assertion is not raised')
            except subprocess.CalledProcessError as e:
                self.assertIn('ValueError: boot sigature size is > 4096',
                              e.stderr)

    def test_boot_image_v4_signature_empty(self):
        """Tests no boot signature in a boot image version 4."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            kernel = generate_test_file(os.path.join(temp_out_dir, 'kernel'),
                                        0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)

            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--kernel', kernel,
                '--ramdisk', ramdisk,
                '--cmdline', TEST_KERNEL_CMDLINE,
                '--os_version', '11.0.0',
                '--os_patch_level', '2021-01',
                '--output', boot_img,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            subprocess.run(unpack_bootimg_cmds, check=True)

            # The boot signature will be empty if no
            # --gki_signing_[algorithm|key] is provided.
            boot_signature = os.path.join(temp_out_dir, 'out', 'boot_signature')
            self.assertFalse(os.path.exists(boot_signature))

    def test_vendor_boot_v4(self):
        """Tests vendor_boot version 4."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            vendor_boot_img = os.path.join(temp_out_dir, 'vendor_boot.img')
            dtb = generate_test_file(os.path.join(temp_out_dir, 'dtb'), 0x1000)
            ramdisk1 = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk1'), 0x1000)
            ramdisk2 = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk2'), 0x2000)
            bootconfig = generate_test_file(
                os.path.join(temp_out_dir, 'bootconfig'), 0x1000)
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--vendor_boot', vendor_boot_img,
                '--dtb', dtb,
                '--vendor_ramdisk', ramdisk1,
                '--ramdisk_type', 'PLATFORM',
                '--ramdisk_name', 'RAMDISK1',
                '--vendor_ramdisk_fragment', ramdisk1,
                '--ramdisk_type', 'DLKM',
                '--ramdisk_name', 'RAMDISK2',
                '--board_id0', '0xC0FFEE',
                '--board_id15', '0x15151515',
                '--vendor_ramdisk_fragment', ramdisk2,
                '--vendor_cmdline', TEST_KERNEL_CMDLINE,
                '--vendor_bootconfig', bootconfig,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', vendor_boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
            ]
            expected_output = [
                'boot magic: VNDRBOOT',
                'vendor boot image header version: 4',
                'vendor ramdisk total size: 16384',
                f'vendor command line args: {TEST_KERNEL_CMDLINE}',
                'dtb size: 4096',
                'vendor ramdisk table size: 324',
                'size: 4096', 'offset: 0', 'type: 0x1', 'name:',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                'size: 4096', 'offset: 4096', 'type: 0x1', 'name: RAMDISK1',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                'size: 8192', 'offset: 8192', 'type: 0x3', 'name: RAMDISK2',
                '0x00c0ffee, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x00000000,',
                '0x00000000, 0x00000000, 0x00000000, 0x15151515,',
                'vendor bootconfig size: 4096',
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            output = [line.strip() for line in result.stdout.splitlines()]
            if not subsequence_of(expected_output, output):
                msg = '\n'.join([
                    'Unexpected unpack_bootimg output:',
                    'Expected:',
                    ' ' + '\n '.join(expected_output),
                    '',
                    'Actual:',
                    ' ' + '\n '.join(output),
                ])
                self.fail(msg)

    def test_unpack_vendor_boot_image_v4(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            vendor_boot_img = os.path.join(temp_out_dir, 'vendor_boot.img')
            vendor_boot_img_reconstructed = os.path.join(
                temp_out_dir, 'vendor_boot.img.reconstructed')
            dtb = generate_test_file(os.path.join(temp_out_dir, 'dtb'), 0x1000)
            ramdisk1 = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk1'), 0x121212)
            ramdisk2 = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk2'), 0x212121)
            bootconfig = generate_test_file(
                os.path.join(temp_out_dir, 'bootconfig'), 0x1000)

            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--vendor_boot', vendor_boot_img,
                '--dtb', dtb,
                '--vendor_ramdisk', ramdisk1,
                '--ramdisk_type', 'PLATFORM',
                '--ramdisk_name', 'RAMDISK1',
                '--vendor_ramdisk_fragment', ramdisk1,
                '--ramdisk_type', 'DLKM',
                '--ramdisk_name', 'RAMDISK2',
                '--board_id0', '0xC0FFEE',
                '--board_id15', '0x15151515',
                '--vendor_ramdisk_fragment', ramdisk2,
                '--vendor_cmdline', TEST_KERNEL_CMDLINE,
                '--vendor_bootconfig', bootconfig,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', vendor_boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]
            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--vendor_boot', vendor_boot_img_reconstructed,
            ]
            unpack_format_args = shlex.split(result.stdout)
            mkbootimg_cmds.extend(unpack_format_args)

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(vendor_boot_img, vendor_boot_img_reconstructed),
                'reconstructed vendor_boot image differ from the original')

            # Also check that -0, --null are as expected.
            unpack_bootimg_cmds.append('--null')
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            unpack_format_null_args = result.stdout
            self.assertEqual('\0'.join(unpack_format_args) + '\0',
                             unpack_format_null_args)

    def test_unpack_vendor_boot_image_v3(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            vendor_boot_img = os.path.join(temp_out_dir, 'vendor_boot.img')
            vendor_boot_img_reconstructed = os.path.join(
                temp_out_dir, 'vendor_boot.img.reconstructed')
            dtb = generate_test_file(os.path.join(temp_out_dir, 'dtb'), 0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x121212)
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '3',
                '--vendor_boot', vendor_boot_img,
                '--vendor_ramdisk', ramdisk,
                '--dtb', dtb,
                '--vendor_cmdline', TEST_KERNEL_CMDLINE,
                '--board', 'product_name',
                '--base', '0x00000000',
                '--dtb_offset', '0x01f00000',
                '--kernel_offset', '0x00008000',
                '--pagesize', '0x00001000',
                '--ramdisk_offset', '0x01000000',
                '--tags_offset', '0x00000100',
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', vendor_boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]
            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--vendor_boot', vendor_boot_img_reconstructed,
            ]
            mkbootimg_cmds.extend(shlex.split(result.stdout))

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(vendor_boot_img, vendor_boot_img_reconstructed),
                'reconstructed vendor_boot image differ from the original')

    def test_unpack_boot_image_v4(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            boot_img_reconstructed = os.path.join(
                temp_out_dir, 'boot.img.reconstructed')
            kernel = generate_test_file(os.path.join(temp_out_dir, 'kernel'),
                                        0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--kernel', kernel,
                '--ramdisk', ramdisk,
                '--cmdline', TEST_KERNEL_CMDLINE,
                '--output', boot_img,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--out', boot_img_reconstructed,
            ]
            mkbootimg_cmds.extend(shlex.split(result.stdout))

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(boot_img, boot_img_reconstructed),
                'reconstructed boot image differ from the original')

    def test_unpack_boot_image_v3(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            boot_img_reconstructed = os.path.join(
                temp_out_dir, 'boot.img.reconstructed')
            kernel = generate_test_file(os.path.join(temp_out_dir, 'kernel'),
                                        0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '3',
                '--kernel', kernel,
                '--ramdisk', ramdisk,
                '--cmdline', TEST_KERNEL_CMDLINE,
                '--os_version', '11.0.0',
                '--os_patch_level', '2021-01',
                '--output', boot_img,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--out', boot_img_reconstructed,
            ]
            mkbootimg_cmds.extend(shlex.split(result.stdout))

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(boot_img, boot_img_reconstructed),
                'reconstructed boot image differ from the original')

    def test_unpack_boot_image_v2(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            # Output image path.
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            boot_img_reconstructed = os.path.join(
                temp_out_dir, 'boot.img.reconstructed')
            # Creates blank images first.
            kernel = generate_test_file(
                os.path.join(temp_out_dir, 'kernel'), 0x1000)
            ramdisk = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk'), 0x1000)
            second = generate_test_file(
                os.path.join(temp_out_dir, 'second'), 0x1000)
            recovery_dtbo = generate_test_file(
                os.path.join(temp_out_dir, 'recovery_dtbo'), 0x1000)
            dtb = generate_test_file(
                os.path.join(temp_out_dir, 'dtb'), 0x1000)

            cmdline = (BOOT_ARGS_SIZE - 1) * 'x'
            extra_cmdline = (BOOT_EXTRA_ARGS_SIZE - 1) * 'y'

            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '2',
                '--base', '0x00000000',
                '--kernel', kernel,
                '--kernel_offset', '0x00008000',
                '--ramdisk', ramdisk,
                '--ramdisk_offset', '0x01000000',
                '--second', second,
                '--second_offset', '0x40000000',
                '--recovery_dtbo', recovery_dtbo,
                '--dtb', dtb,
                '--dtb_offset', '0x01f00000',
                '--tags_offset', '0x00000100',
                '--pagesize', '0x00001000',
                '--os_version', '11.0.0',
                '--os_patch_level', '2021-03',
                '--board', 'boot_v2',
                '--cmdline', cmdline + extra_cmdline,
                '--output', boot_img,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--out', boot_img_reconstructed,
            ]
            mkbootimg_cmds.extend(shlex.split(result.stdout))

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(boot_img, boot_img_reconstructed),
                'reconstructed boot image differ from the original')

    def test_unpack_boot_image_v1(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            # Output image path.
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            boot_img_reconstructed = os.path.join(
                temp_out_dir, 'boot.img.reconstructed')
            # Creates blank images first.
            kernel = generate_test_file(
                os.path.join(temp_out_dir, 'kernel'), 0x1000)
            ramdisk = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk'), 0x1000)
            recovery_dtbo = generate_test_file(
                os.path.join(temp_out_dir, 'recovery_dtbo'), 0x1000)

            cmdline = (BOOT_ARGS_SIZE - 1) * 'x'
            extra_cmdline = (BOOT_EXTRA_ARGS_SIZE - 1) * 'y'

            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '1',
                '--base', '0x00000000',
                '--kernel', kernel,
                '--kernel_offset', '0x00008000',
                '--ramdisk', ramdisk,
                '--ramdisk_offset', '0x01000000',
                '--recovery_dtbo', recovery_dtbo,
                '--tags_offset', '0x00000100',
                '--pagesize', '0x00001000',
                '--os_version', '11.0.0',
                '--os_patch_level', '2021-03',
                '--board', 'boot_v1',
                '--cmdline', cmdline + extra_cmdline,
                '--output', boot_img,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--out', boot_img_reconstructed,
            ]
            mkbootimg_cmds.extend(shlex.split(result.stdout))

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(boot_img, boot_img_reconstructed),
                'reconstructed boot image differ from the original')

    def test_unpack_boot_image_v0(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            # Output image path.
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            boot_img_reconstructed = os.path.join(
                temp_out_dir, 'boot.img.reconstructed')
            # Creates blank images first.
            kernel = generate_test_file(
                os.path.join(temp_out_dir, 'kernel'), 0x1000)
            ramdisk = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk'), 0x1000)
            second = generate_test_file(
                os.path.join(temp_out_dir, 'second'), 0x1000)

            cmdline = (BOOT_ARGS_SIZE - 1) * 'x'
            extra_cmdline = (BOOT_EXTRA_ARGS_SIZE - 1) * 'y'

            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '0',
                '--base', '0x00000000',
                '--kernel', kernel,
                '--kernel_offset', '0x00008000',
                '--ramdisk', ramdisk,
                '--ramdisk_offset', '0x01000000',
                '--second', second,
                '--second_offset', '0x40000000',
                '--tags_offset', '0x00000100',
                '--pagesize', '0x00001000',
                '--os_version', '11.0.0',
                '--os_patch_level', '2021-03',
                '--board', 'boot_v0',
                '--cmdline', cmdline + extra_cmdline,
                '--output', boot_img,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--out', boot_img_reconstructed,
            ]
            mkbootimg_cmds.extend(shlex.split(result.stdout))

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(boot_img, boot_img_reconstructed),
                'reconstructed boot image differ from the original')

    def test_boot_image_v2_cmdline_null_terminator(self):
        """Tests that kernel commandline is null-terminated."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            dtb = generate_test_file(os.path.join(temp_out_dir, 'dtb'), 0x1000)
            kernel = generate_test_file(os.path.join(temp_out_dir, 'kernel'),
                                        0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)
            cmdline = (BOOT_ARGS_SIZE - 1) * 'x'
            extra_cmdline = (BOOT_EXTRA_ARGS_SIZE - 1) * 'y'
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '2',
                '--dtb', dtb,
                '--kernel', kernel,
                '--ramdisk', ramdisk,
                '--cmdline', cmdline + extra_cmdline,
                '--output', boot_img,
            ]

            subprocess.run(mkbootimg_cmds, check=True)

            with open(boot_img, 'rb') as f:
                raw_boot_img = f.read()
            raw_cmdline = raw_boot_img[BOOT_ARGS_OFFSET:][:BOOT_ARGS_SIZE]
            raw_extra_cmdline = (raw_boot_img[BOOT_EXTRA_ARGS_OFFSET:]
                                 [:BOOT_EXTRA_ARGS_SIZE])
            self.assertEqual(raw_cmdline, cmdline.encode() + b'\x00')
            self.assertEqual(raw_extra_cmdline,
                             extra_cmdline.encode() + b'\x00')

    def test_boot_image_v3_cmdline_null_terminator(self):
        """Tests that kernel commandline is null-terminated."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            kernel = generate_test_file(os.path.join(temp_out_dir, 'kernel'),
                                        0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)
            cmdline = BOOT_ARGS_SIZE * 'x' + (BOOT_EXTRA_ARGS_SIZE - 1) * 'y'
            boot_img = os.path.join(temp_out_dir, 'boot.img')
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '3',
                '--kernel', kernel,
                '--ramdisk', ramdisk,
                '--cmdline', cmdline,
                '--output', boot_img,
            ]

            subprocess.run(mkbootimg_cmds, check=True)

            with open(boot_img, 'rb') as f:
                raw_boot_img = f.read()
            raw_cmdline = (raw_boot_img[BOOT_V3_ARGS_OFFSET:]
                           [:BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE])
            self.assertEqual(raw_cmdline, cmdline.encode() + b'\x00')

    def test_vendor_boot_image_v3_cmdline_null_terminator(self):
        """Tests that kernel commandline is null-terminated."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            dtb = generate_test_file(os.path.join(temp_out_dir, 'dtb'), 0x1000)
            ramdisk = generate_test_file(os.path.join(temp_out_dir, 'ramdisk'),
                                         0x1000)
            vendor_cmdline = (VENDOR_BOOT_ARGS_SIZE - 1) * 'x'
            vendor_boot_img = os.path.join(temp_out_dir, 'vendor_boot.img')
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '3',
                '--dtb', dtb,
                '--vendor_ramdisk', ramdisk,
                '--vendor_cmdline', vendor_cmdline,
                '--vendor_boot', vendor_boot_img,
            ]

            subprocess.run(mkbootimg_cmds, check=True)

            with open(vendor_boot_img, 'rb') as f:
                raw_vendor_boot_img = f.read()
            raw_vendor_cmdline = (raw_vendor_boot_img[VENDOR_BOOT_ARGS_OFFSET:]
                                  [:VENDOR_BOOT_ARGS_SIZE])
            self.assertEqual(raw_vendor_cmdline,
                             vendor_cmdline.encode() + b'\x00')

    def test_vendor_boot_v4_without_dtb(self):
        """Tests building vendor_boot version 4 without dtb image."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            vendor_boot_img = os.path.join(temp_out_dir, 'vendor_boot.img')
            ramdisk = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk'), 0x1000)
            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--vendor_boot', vendor_boot_img,
                '--vendor_ramdisk', ramdisk,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', vendor_boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
            ]
            expected_output = [
                'boot magic: VNDRBOOT',
                'vendor boot image header version: 4',
                'dtb size: 0',
            ]

            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            output = [line.strip() for line in result.stdout.splitlines()]
            if not subsequence_of(expected_output, output):
                msg = '\n'.join([
                    'Unexpected unpack_bootimg output:',
                    'Expected:',
                    ' ' + '\n '.join(expected_output),
                    '',
                    'Actual:',
                    ' ' + '\n '.join(output),
                ])
                self.fail(msg)

    def test_unpack_vendor_boot_image_v4_without_dtb(self):
        """Tests that mkbootimg(unpack_bootimg(image)) is an identity when no dtb image."""
        with tempfile.TemporaryDirectory() as temp_out_dir:
            vendor_boot_img = os.path.join(temp_out_dir, 'vendor_boot.img')
            vendor_boot_img_reconstructed = os.path.join(
                temp_out_dir, 'vendor_boot.img.reconstructed')
            ramdisk = generate_test_file(
                os.path.join(temp_out_dir, 'ramdisk'), 0x121212)

            mkbootimg_cmds = [
                'mkbootimg',
                '--header_version', '4',
                '--vendor_boot', vendor_boot_img,
                '--vendor_ramdisk', ramdisk,
            ]
            unpack_bootimg_cmds = [
                'unpack_bootimg',
                '--boot_img', vendor_boot_img,
                '--out', os.path.join(temp_out_dir, 'out'),
                '--format=mkbootimg',
            ]
            subprocess.run(mkbootimg_cmds, check=True)
            result = subprocess.run(unpack_bootimg_cmds, check=True,
                                    capture_output=True, encoding='utf-8')
            mkbootimg_cmds = [
                'mkbootimg',
                '--vendor_boot', vendor_boot_img_reconstructed,
            ]
            unpack_format_args = shlex.split(result.stdout)
            mkbootimg_cmds.extend(unpack_format_args)

            subprocess.run(mkbootimg_cmds, check=True)
            self.assertTrue(
                filecmp.cmp(vendor_boot_img, vendor_boot_img_reconstructed),
                'reconstructed vendor_boot image differ from the original')


# I don't know how, but we need both the logger configuration and verbosity
# level > 2 to make atest work. And yes this line needs to be at the very top
# level, not even in the "__main__" indentation block.
logging.basicConfig(stream=sys.stdout)

if __name__ == '__main__':
    unittest.main(verbosity=2)
