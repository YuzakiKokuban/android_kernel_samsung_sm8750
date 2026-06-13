#!/usr/bin/env python3

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

"""A script that converts existing build.config to a skeleton Bazel BUILD rules.

The skeleton Bazel BUILD file likely won't build properly. Manual intervention
is required after the skeleton file is created. Most instructions are presented
as "FIXME" comments in the generated file.

Running this script requires buildozer. Install it at
  https://github.com/bazelbuild/buildtools/blob/master/buildozer/README.md
"""

import argparse
import collections
import json
import logging
import os
import re
import subprocess
import sys
import buildozer_command_builder
from typing import Optional, Mapping, Sequence

_BUILD_CONFIG_PREFIX = "build.config."
# See kernel_build.bzl
_DEFAULT_KERNEL_BUILD_SRCS = \
    """glob(["**"],\\ exclude=["**/.*",\\ "**/.*/**",\\ "**/BUILD.bazel",\\ "**/*.bzl",])"""

# Variables that should be conditionally ignored and not shown in BUILD files.
# Keys are variable names. Values are regular expressions.
# - If the value matches the regular expression, it is considered ignored (i.e. the BUILD
#   file is not modified).
# - If the value of the variable in the build config does NOT match the regular expression, the
#   variable is considered unsupported.
#
# Following are variables set by build configs or _setup_env.sh that does not
# need to be translated into BUILD definitions. Ignore these variables.
# ^(.|\n)*$ matches any (multi-line) string.
_IGNORED_BUILD_CONFIGS = dict.fromkeys(
    [
        "_",  # reserved by bash
        "OUT_DIR",
        "MAKE_GOALS",
        "LD",
        "SKIP_MRPROPER",
        "SKIP_DEFCONFIG",
        "SKIP_IF_VERSION_MATCHES",
        "SKIP_EXT_MODULES",
        "SKIP_CP_KERNEL_HDR",
        "SKIP_UNPACKING_RAMDISK",
        "POST_DEFCONFIG_CMDS",
        "IN_KERNEL_MODULES",
        "AVB_SIGN_BOOT_IMG",
        "AVB_BOOT_PARTITION_SIZE",
        "AVB_BOOT_KEY",
        "AVB_BOOT_ALGORITHM",
        "AVB_BOOT_PARTITION_NAME",
        "MODULES_ORDER",
        "GKI_MODULES_LIST",
        "LZ4_RAMDISK",
        "LZ4_RAMDISK_COMPRESS_ARGS",
        "KMI_STRICT_MODE_OBJECTS",
        "GKI_DIST_DIR",
        "BUILD_GKI_ARTIFACTS",
        "GKI_KERNEL_CMDLINE",
        "AR",
        "ARCH",
        "BRANCH",
        "BUILDTOOLS_PREBUILT_BIN",
        "CC",
        "CLANG_PREBUILT_BIN",
        "CLANG_VERSION",
        "COMMON_OUT_DIR",
        "DECOMPRESS_GZIP",
        "DECOMPRESS_LZ4",
        "DEFCONFIG",
        "DEPMOD",
        "DTC",
        "HOSTCC",
        "HOSTCFLAGS",
        "HOSTCXX",
        "HOSTLDFLAGS",
        "KBUILD_BUILD_HOST",
        "KBUILD_BUILD_TIMESTAMP",
        "KBUILD_BUILD_USER",
        "KBUILD_BUILD_VERSION",
        "KCFLAGS",
        "KCPPFLAGS",
        "KMI_GENERATION",
        "LC_ALL",
        "LLVM",
        "MODULES_ARCHIVE",
        "NDK_TRIPLE",
        "NM",
        "OBJCOPY",
        "OBJDUMP",
        "OBJSIZE",
        "PATH",
        "RAMDISK_COMPRESS",
        "RAMDISK_DECOMPRESS",
        "RAMDISK_EXT",
        "READELF",
        "ROOT_DIR",
        "SOURCE_DATE_EPOCH",
        "STRIP",
        "TOOL_ARGS",
        "TZ",
        "UNSTRIPPED_DIR",
        "UNSTRIPPED_MODULES_ARCHIVE",
        "USERCFLAGS",
        "USERLDFLAGS",
        "_SETUP_ENV_SH_INCLUDED",
    ],
    r"^(.|\n)*$"
)

# Conditionally ignored.
# These variables are ignored only if the value matches the condition.
_IGNORED_BUILD_CONFIGS.update(
    {
        "HERMETIC_TOOLCHAIN": r"^1$",  # Ignore iff HERMETIC_TOOLCHAIN=1
    }
)

# Variables not supported by Kleaf. If any of these variables are set to
# a non-empty value, it is considered unsupported.
# Device owners will need to migrate away from these variables.
_IGNORED_BUILD_CONFIGS.update(dict.fromkeys(
    [
        "EXT_MODULES_MAKEFILE",
        "COMPRESS_MODULES",
        "ADDITIONAL_HOST_TOOLS",
        "POST_KERNEL_BUILD_CMDS",
        "TAGS_CONFIG",
        "EXTRA_CMDS",
        "DIST_CMDS",
        "VENDOR_RAMDISK_CMDS",
        "STOP_SHIP_TRACEPRINTK",
    ],
    r"^$"
))


def die(msg):
    logging.error("%s", msg)
    sys.exit(1)


def order_dict_by_key(d: Mapping[str, str]) -> Mapping[str, str]:
    return collections.OrderedDict(sorted(d.items()))


def find_build_config(env: Mapping[str, str]) -> str:
    # Set by either environment or _setup_env.sh
    if env.get("BUILD_CONFIG"):
        real_build_config = os.path.realpath(env["BUILD_CONFIG"])
        real_this = os.path.realpath(".")
        if os.path.commonpath([real_build_config, real_this]) != real_this:
            die(f"realpath $BUILD_CONFIG ({real_build_config}) is not under the repository root")
        return os.path.relpath(real_build_config, real_this)
    die("$BUILD_CONFIG is not set, and top level build.config file is not found.")


def infer_target_name(args, build_config: str) -> str:
    if args.target:
        return args.target
    build_config_base = os.path.basename(build_config)
    if build_config_base.startswith(
            _BUILD_CONFIG_PREFIX) and build_config_base != _BUILD_CONFIG_PREFIX:
        return build_config_base[len(_BUILD_CONFIG_PREFIX):]
    die("Fail to infer target name. Specify with --target.")


class BuildConfigToBazel(buildozer_command_builder.BuildozerCommandBuilder):
    def __init__(self, *init_args, **init_kwargs):
        super().__init__(*init_args, **init_kwargs)

        self.new_env = order_dict_by_key(json.loads(subprocess.check_output(
            "source build/kernel/_setup_env.sh > /dev/null && build/kernel/kleaf/dump_env.py",
            shell=True, stderr=self.stderr, env=self.environ, executable="/bin/bash")))
        logging.info("Captured env: %s", json.dumps(self.new_env, indent=2))

        build_config = find_build_config(self.new_env)
        target_name = infer_target_name(self.args, build_config)

        self.package = os.path.dirname(build_config)
        self.target_name = target_name

        self.pkg = f"//{self.package}:__pkg__"
        self.dist_name = f"{target_name}_dist"
        self.unstripped_modules_name = f"{target_name}_unstripped_modules_archive"
        self.images_name = f"{target_name}_images"
        self.abi_name = f"{target_name}_abi"
        self.dts_name = f"{target_name}_dts"
        self.modules_install_name = f"{target_name}_modules_install"

        # set elsewhere
        self.dist_targets: Optional[set[str]] = None

    def _new(self, kind: str, name: str, package=None, load_from="//build/kernel/kleaf:kernel.bzl",
             add_to_dist=True) -> str:
        if package is None:
            package = self.package
        new_target = super()._new(kind, name, package, load_from=load_from)
        if add_to_dist:
            self.dist_targets.add(new_target)
        return new_target

    def _create_buildozer_commands(self) -> None:
        """Fills in self.out_file."""
        common = self.args.common_kernel_tree

        self.dist_targets = set()

        target = self._new("kernel_build", self.target_name)
        dist = self._new("copy_to_dist_dir", self.dist_name,
                         load_from="//build/bazel_common_rules/dist:dist.bzl", add_to_dist=False)
        self._set_attr(dist, "flat", True)

        images = None
        need_unstripped_modules = False
        abi = None
        modules = []

        target_comment = []

        # List of build configs unknown to this script. They require attention from
        # the developers to be translated properly.
        unknowns = []

        for key, value in self.new_env.items():
            esc_value = value.replace(" ", "\\ ").replace("\n", "\\n")

            if key in _IGNORED_BUILD_CONFIGS:
                if not re.match(_IGNORED_BUILD_CONFIGS[key], value):
                    target_comment.append(f"FIXME: {key}={esc_value} not supported")
                continue
                # else ignore
            elif type(self)._is_bash_func(key):
                continue
            elif key == "BUILD_CONFIG":
                self._set_attr(target, "build_config", os.path.basename(value), quote=True)
            elif key == "BUILD_CONFIG_FRAGMENTS":
                target_comment.append(
                    f"FIXME: {key}={esc_value}: Please manually convert to kernel_build_config")
            elif key == "FAST_BUILD":
                if value:
                    target_comment.append(f"FIXME: {key}: Specify --config=fast in device.bazelrc")
            elif key == "LTO":
                if value:
                    target_comment.append(f"FIXME: {key}: Specify --lto={value} in device.bazelrc")
            elif key == "DIST_DIR":
                rel_dist_dir = os.path.relpath(value)
                self._add_comment(dist, "dist_dir",
                                  f'FIXME: or dist_dir = "{rel_dist_dir}"')
            elif key == "DO_NOT_STRIP_MODULES":
                self._set_attr(target, "strip_modules", value != "1")
            elif key == "FILES":
                for elem in value.split():
                    self._add_attr(target, "outs", elem, quote=True)
            elif key == "EXT_MODULES":
                module_packages = [token.strip() for token in value.split() if token.strip()]
                for module_package in module_packages:
                    module = self._new("kernel_module",
                                       name=os.path.basename(module_package),
                                       package=module_package,
                                       add_to_dist=False)
                    self._set_attr(module, "kernel_build", target, quote=True)
                    # buildozer converts None to ["None"] for outs, so use a different name
                    # then rename.
                    self._add_comment(module, "temp_outs",
                                      f"FIXME: set to the list of external modules in this package. You may "
                                      f"run `tools/bazel build {module}` and follow the instructions "
                                      f"in the error message.",
                                      lambda attr_val: attr_val.is_missing_or_none())
                    self._rename(module, "temp_outs", "outs")
                    modules.append(module)
            elif key == "KERNEL_DIR":
                if value != self.package:
                    if value.removesuffix("/") == common:
                        self._set_attr(target, "srcs", _DEFAULT_KERNEL_BUILD_SRCS, quote=False,
                                       command="set_if_absent")
                        self._add_attr(target, "srcs", f"//{common}:kernel_aarch64_sources",
                                       quote=True)
                    else:
                        self._add_comment(
                            target, "srcs",
                            f"FIXME: add files from KERNEL_DIR {self.new_env['KERNEL_DIR']}")
                # else keep srcs unchanged
            elif key == "KCONFIG_EXT_PREFIX":
                self._set_attr(target, "kconfig_ext", value, quote=True)
            elif key == "UNSTRIPPED_MODULES":
                self._set_attr(target, "collect_unstripped_modules", bool(value))
            elif key == "COMPRESS_UNSTRIPPED_MODULES":
                if value == "1":
                    need_unstripped_modules = True
            elif key == "ABI_DEFINITION":
                abi = self._new("kernel_abi", self.abi_name)
                self._add_comment(abi, "abi_definition",
                                  f"Usually not set in Kleaf. See "
                                  f"build/kernel/kleaf/docs/abi_device.md. Original value: "
                                  f"//{common}:{value}",
                                  lambda attr_val: attr_val.is_missing_or_none())
            elif key in ("KMI_ENFORCED", "KMI_SYMBOL_LIST_ADD_ONLY"):
                abi = self._new("kernel_abi", self.abi_name)
                if value == "1":
                    self._set_attr(abi, key.lower(), True)
            elif key == "KMI_SYMBOL_LIST_MODULE_GROUPING":
                abi = self._new("kernel_abi", self.abi_name)
                if value == "1":
                    self._set_attr(abi, "module_grouping", True)
            elif key == "KMI_SYMBOL_LIST":
                self._set_attr(target, "kmi_symbol_list", f"//{common}:{value}", quote=True)
            elif key == "ADDITIONAL_KMI_SYMBOL_LISTS":
                kmi_symbol_lists = value.split()
                for kmi_symbol_list in kmi_symbol_lists:
                    self._add_attr(target, "additional_kmi_symbol_lists",
                                   f"//{common}:{kmi_symbol_list}", quote=True)

            elif key in (
                    "TRIM_NONLISTED_KMI",
                    "GENERATE_VMLINUX_BTF",
                    "KMI_SYMBOL_LIST_STRICT_MODE",
                    "KBUILD_SYMTYPES",
            ):
                self._set_attr(target, key.lower(), bool(value == "1"))
            elif key == "PRE_DEFCONFIG_CMDS":
                target_comment.append(
                    "FIXME: PRE_DEFCONFIG_CMDS: Don't forget to modify PRE_DEFCONFIG_CMDS "
                    "so it writes to $OUT_DIR, not the source tree: "
                    "https://android.googlesource.com/kernel/build/+/refs/heads/main/kleaf/docs/errors.md#defconfig-readonly")
            elif key in (
                    "BUILD_BOOT_IMG",
                    "BUILD_VENDOR_BOOT_IMG",
                    "BUILD_DTBO_IMG",
                    "BUILD_VENDOR_KERNEL_BOOT",
                    "BUILD_INITRAMFS",
            ):
                images = self._new("kernel_images", self.images_name)
                # bool(value) checks if the string is empty or not
                self._set_attr(images, key.removesuffix("_IMG").lower(), bool(value))
            elif key == "SKIP_VENDOR_BOOT":
                images = self._new("kernel_images", self.images_name)
                self._set_attr(images, "build_vendor_boot", not bool(value))
            elif key == "MKBOOTIMG_PATH":
                images = self._new("kernel_images", self.images_name)
                self._add_comment(images, "mkbootimg",
                                  f"FIXME: set mkbootimg to label of {esc_value}")
            elif key == "MODULES_OPTIONS":
                images = self._new("kernel_images", self.images_name)
                modules_options_filename = f"modules.options.{self.target_name}"
                modules_options_path = os.path.join(self.package, modules_options_filename)
                self._create_extra_file(modules_options_path, value)
                self._set_attr(images, "modules_options",
                               f"//{self.package}:{modules_options_filename}",
                               quote=True)
            elif key in (
                    "MODULES_LIST",
                    "MODULES_BLOCKLIST",
                    "SYSTEM_DLKM_FS_TYPE",
                    "SYSTEM_DLKM_MODULES_LIST",
                    "SYSTEM_DLKM_MODULES_BLOCKLIST",
                    "SYSTEM_DLKM_PROPS",
                    "VENDOR_DLKM_ETC_FILES",
                    "VENDOR_DLKM_FS_TYPE",
                    "VENDOR_DLKM_MODULES_LIST",
                    "VENDOR_DLKM_MODULES_BLOCKLIST",
                    "VENDOR_DLKM_PROPS",
            ):
                images = self._new("kernel_images", self.images_name)
                if os.path.isabs(value):
                    value = os.path.relpath(value)
                if os.path.commonpath((value, self.package)) == self.package:
                    self._set_attr(images, key.lower(), os.path.relpath(value, start=self.package),
                                   quote=True)
                else:
                    self._add_comment(images, key.lower(),
                                      f"FIXME: set {key.lower()} to label of {esc_value}")
            elif key == "GKI_BUILD_CONFIG":
                if value == f"{common}/build.config.gki.aarch64":
                    self._set_attr(target, "base_kernel", f"//{common}:kernel_aarch64", quote=True)
                else:
                    self._add_comment(target, "base_kernel",
                                      f"FIXME: set base_kernel to kernel_build for {esc_value}")
            elif key == "GKI_PREBUILTS_DIR":
                target_comment.append(
                    f"FIXME: {key}={esc_value}: Please manually convert to kernel_filegroup")
            elif key == "DTS_EXT_DIR":
                dts = self._new("kernel_dtstree", self.dts_name, package=value,
                                add_to_dist=False)
                self._set_attr(target, "dtstree", dts, quote=True)
            elif key == "BUILD_GKI_CERTIFICATION_TOOLS":
                if value == "1":
                    self.dist_targets.add("//build/kernel:gki_certification_tools")
            elif key in self.environ:
                if self.environ[key] == value:
                    logging.info(f"Ignoring variable {key} in environment.")
                else:
                    target_comment.append(f"FIXME: Unknown in build config: {key}={esc_value}")
                    unknowns.append(key)
            else:
                target_comment.append(f"FIXME: Unknown in build config: {key}={esc_value}")
                unknowns.append(key)

        for dist_target in self.dist_targets:
            self._add_attr(dist, "data", dist_target, quote=True)

        unstripped_modules = None
        if need_unstripped_modules or abi:
            unstripped_modules = self._new("kernel_unstripped_modules_archive",
                                           self.unstripped_modules_name)
            self._set_attr(unstripped_modules, "kernel_build", target, quote=True)
            for module in modules:
                self._add_attr(unstripped_modules, "kernel_modules", module, quote=True)

        modules_install = None
        need_modules_install = images or modules
        if need_modules_install:
            modules_install = self._new("kernel_modules_install", self.modules_install_name)
            self._set_attr(modules_install, "kernel_build", target, quote=True)
            for module in modules:
                self._add_attr(modules_install, "kernel_modules", module, quote=True)

        if abi:
            for module in modules:
                self._add_attr(abi, "kernel_modules", module, quote=True)
            self._set_attr(abi, "unstripped_modules_archive", unstripped_modules, quote=True)
            self._set_attr(abi, "kernel_build", target, quote=True)

        if images:
            self._set_attr(images, "kernel_build", target, quote=True)
            self._set_attr(images, "kernel_modules_install", modules_install, quote=True)

        self._add_comment(target, "base_kernel",
                          f"FIXME: base_kernel should be migrated to //{common}:kernel_aarch64.",
                          lambda attr_val: attr_val.value not in (
                              f"//{common}:kernel_aarch64", f"//{common}:kernel"))

        self._add_comment(target, "module_outs",
                          f"FIXME: set to the list of in-tree modules. You may run "
                          f"`tools/bazel build {target}` and follow the instructions "
                          f"in the error message.",
                          lambda attr_val: attr_val.is_missing_or_none())

        self._add_target_comment(target, target_comment)

        if unknowns:
            logging.info("Unknown variables:\n%s", ",\n".join(f'"{e}"' for e in unknowns))

        self.out_file.flush()


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-t", "--target",
                        help="Name of target. Otherwise, infer from the name of the "
                             "build.config file.")
    parser.add_argument("-v", "--verbose", help="verbose mode", action="store_true")
    parser.add_argument("-k", "--keep-going",
                        help="buildozer keeps going on errors. Use when targets are already "
                             "defined. There may be duplicated FIXME comments.",
                        action="store_true")
    parser.add_argument("--stdout",
                        help="buildozer writes changed BUILD file to stdout (dry run)",
                        action="store_true")
    parser.add_argument("--common-kernel-tree",
                        help="path to common kernel source tree; default is common.",
                        default="common")
    return parser.parse_args(argv)


def main(argv: Sequence[str]):
    args = parse_args(argv)
    log_level = logging.INFO if args.verbose else logging.WARNING
    logging.basicConfig(level=log_level, format="%(levelname)s: %(message)s")
    BuildConfigToBazel(args=args).run()


if __name__ == "__main__":
    main(sys.argv[1:])
