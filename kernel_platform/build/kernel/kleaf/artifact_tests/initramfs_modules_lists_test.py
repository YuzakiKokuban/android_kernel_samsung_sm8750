# Copyright (C) 2023 The Android Open Source Project
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

import argparse
import subprocess
import sys
import tempfile
import unittest
import re
import pathlib

from absl.testing import absltest


def load_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--expected_modules_list")
    parser.add_argument("--expected_modules_recovery_list")
    parser.add_argument("--expected_modules_charger_list")
    parser.add_argument("--build_vendor_boot")
    parser.add_argument("--build_vendor_kernel_boot")
    parser.add_argument("files", nargs="*", default=[])
    return parser.parse_known_args()


arguments = None


class InitramfsModulesLists(unittest.TestCase):
    def _detect_decompression_cmd(self, initramfs):
        """Determines what commands to use for decompressing initramfs.img

        Args:
            initramfs: The path to the initramfs.img to decompress

        Returns:
            The command that should be used to decompress the image.
        """
        magic_to_decompression_command = {
            # GZIP
            b'\x1f\x8b\x08': ["gzip", "-c", "-d"],
            # LZ4
            # The kernel build uses legacy LZ4 compression (i.e. lz4 -l ...),
            # so the legacy LZ4 magic must be used in little-endian format.
            b'\x02\x21\x4c\x18': ["lz4", "-c", "-d", "-l"],
        }
        max_magic_len = max(len(magic) for magic in magic_to_decompression_command)

        with open(initramfs, "rb") as initramfs_file:
            hdr = initramfs_file.read(max_magic_len)

        self.assertIsNotNone(hdr)

        for magic, command in magic_to_decompression_command.items():
            if hdr.startswith(magic):
                return command

        self.fail("No suitable compression method found")

    def _decompress_initramfs(self, initramfs, temp_dir):
        """Decompress initramfs into temp_dir.

        Args:
            initramfs: path to initramfs.img gzip file to be decompressed
            temp_dir: directory in which to decompress initramfs.img into
        """
        decompression_cmd = self._detect_decompression_cmd(initramfs)
        with open(initramfs) as initramfs_file:
            with subprocess.Popen(["cpio", "-i"], cwd=temp_dir,
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE) as cpio_sp:
                with subprocess.Popen(decompression_cmd, stdin=initramfs_file,
                                      stdout=cpio_sp.stdin) as decompress_sp:
                    decompress_sp.communicate()
                    self.assertEqual(0, decompress_sp.returncode)

    def _diff_modules_lists(self, modules_lists_map, modules_dir):
        """Compares generated modules lists against expected modules lists for equality.

        Given a dictionary of modules.load* files as keys, and expected modules
        lists as values, compares each key value pair for equality.

        Args:
            modules_lists_map: dictionary with modules lists to compare
            modules_dir:       directory that contains the modules.load* files
        """
        for modules_load, expected_modules_list_path in modules_lists_map.items():
            modules_load_path = pathlib.Path(modules_dir, modules_load)
            self.assertTrue(modules_load_path.is_file(), f"Can't find {modules_load_path}")

            with open(modules_load_path) as modules_load_file, \
                 open(expected_modules_list_path) as expected_modules_list_file:
                modules_load_lines = [pathlib.Path(line.strip()).name for line in modules_load_file]
                expected_modules_list_lines = [line.strip() for line in expected_modules_list_file]
                self.assertCountEqual(modules_load_lines, expected_modules_list_lines)

    def test_diff(self):
        initramfs_list = [file
                          for file in arguments.files
                          if pathlib.Path(file).name == "initramfs.img"]
        self.assertEqual(len(initramfs_list), 1)
        initramfs = initramfs_list[0]
        modules_lists_map = {}

        if arguments.expected_modules_list:
            modules_lists_map["modules.load"] = arguments.expected_modules_list

        if arguments.expected_modules_recovery_list:
            modules_lists_map["modules.load.recovery"] = arguments.expected_modules_recovery_list

        if arguments.expected_modules_charger_list:
            modules_lists_map["modules.load.charger"] = arguments.expected_modules_charger_list

        with tempfile.TemporaryDirectory() as temp_dir:
            self._decompress_initramfs(initramfs, temp_dir)

            lib_modules = pathlib.Path(temp_dir, "lib/modules")
            self.assertTrue(lib_modules.is_dir())

            for kernel_version in lib_modules.iterdir():
                modules_dir = pathlib.Path(lib_modules, kernel_version)
                self._diff_modules_lists(modules_lists_map, modules_dir)

    def _verify_modules_load_lists(self, modules_list_name, vendor_boot_name, image_files):
        """Given a modules.load* name, ensures that no extraneous such files exist.

        This tests to ensure that if a modules.load* file exists, then no other
        modules.load* file must exist, except for vendor_boot.modules.load* or
        vendor_kernel_boot.modules.load.

        Args:
            modules_list_name: The name of the modules.load list.
            vendor_boot_name: Either vendor_boot or vendor_kernel_boot.
            image_files: The files associated with the kernel_images() target.
        """
        modules_load_lists = [modules_list_name]
        if vendor_boot_name:
            modules_load_lists.append(f"{vendor_boot_name}.{modules_list_name}")

        modules_load_list_re = re.compile(f".*{modules_list_name}$")
        modules_load_list_matches = [pathlib.Path(file).name
                                     for file in image_files
                                     if modules_load_list_re.fullmatch(pathlib.Path(file).name)]

        self.assertCountEqual(modules_load_lists, modules_load_list_matches)

    def test_modules_lists_existence(self):
        vendor_boot_name = None

        if arguments.build_vendor_boot:
            vendor_boot_name = "vendor_boot"
        elif arguments.build_vendor_kernel_boot:
            vendor_boot_name = "vendor_kernel_boot"

        if arguments.expected_modules_list:
            self._verify_modules_load_lists("modules.load", vendor_boot_name, arguments.files)

        if arguments.expected_modules_recovery_list:
            self._verify_modules_load_lists("modules.load.recovery", vendor_boot_name, arguments.files)

        if arguments.expected_modules_charger_list:
            self._verify_modules_load_lists("modules.load.charger", vendor_boot_name, arguments.files)

    def _verify_modules_dep_contains_modules_lists(self, modules_lists, modules_dir):
        """Ensures that modules.dep contains all entries needed for modules.load*.

        Given a list of modules lists, this function ensures that modules.dep
        contains an entry for each module in all of the modules lists.

        Args:
            modules_lists: The list of modules.load* that need to be tested.
            modules_dir: The directory in which the modules reside in.
        """
        modules_dep_path = pathlib.Path(modules_dir, "modules.dep")
        modules_dep_set = set()

        self.assertTrue(modules_dep_path.is_file(), f"Can't find {modules_dep_path}")

        with open(modules_dep_path) as modules_dep_file:
            for line in modules_dep_file:
                # depmod entries have the form:
                # mod_path: dep_path_1 dep_path_2
                mod_name = line.split(":")[0].strip()
                modules_dep_set.add(pathlib.Path(mod_name).name)

        for mod_list in modules_lists:
            mod_list_path = pathlib.Path(modules_dir, mod_list)
            self.assertTrue(mod_list_path.is_file(), f"Can't find {mod_list_path}")
            mod_list_modules = set()

            with open(mod_list_path) as mod_list_file:
                for line in mod_list_file:
                    mod_list_modules.add(pathlib.Path(line.strip()).name)

            self.assertTrue(mod_list_modules.issubset(modules_dep_set),
                            "modules.dep does not contain an entry for each module to be loaded")

    def test_modules_dep_contains_all_modules_lists(self):
        initramfs_list = [file
                          for file in arguments.files
                          if pathlib.Path(file).name == "initramfs.img"]
        self.assertEqual(len(initramfs_list), 1)
        initramfs = initramfs_list[0]
        modules_lists = []

        if arguments.expected_modules_list:
            modules_lists.append("modules.load")

        if arguments.expected_modules_recovery_list:
            modules_lists.append("modules.load.recovery")

        if arguments.expected_modules_charger_list:
            modules_lists.append("modules.load.charger")

        with tempfile.TemporaryDirectory() as temp_dir:
            self._decompress_initramfs(initramfs, temp_dir)

            lib_modules = pathlib.Path(temp_dir, "lib/modules")
            self.assertTrue(lib_modules.is_dir())

            for kernel_version in lib_modules.iterdir():
                modules_dir = pathlib.Path(lib_modules, kernel_version)
                self._verify_modules_dep_contains_modules_lists(modules_lists, modules_dir)

if __name__ == '__main__':
    arguments, unknown = load_arguments()
    sys.argv[1:] = unknown
    absltest.main()
