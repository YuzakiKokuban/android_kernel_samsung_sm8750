#!/usr/bin/env python

# Copyright (c) 2020, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import errno
import logging
import os
import shlex
import shutil
import subprocess
import sys
import tempfile

"""
image_generation_tool wrapper script enables generation of boot and
vbmeta images outside of Android tree. This script helps to create a
consistent interface between upstream tools and the client via versioning
scheme. It also abstracts out the underlying parameters needed to generate
both boot and vbmeta images by providing a simple interface to the client.

usage: python image_generation_tool [-h]
       {version,generate_boot_image,generate_vendor_boot_image,
       generate_vbmeta_image,info_vbmeta_image}

subcommands:
    version                    Prints the version of image_generation_tool
    generate_boot_image        Generates boot image
    generate_vendor_boot_image Generates vendor_boot_image
    generate_vbmeta_image      Generates vbmeta image
    info_vbmeta_image          Shows information about vbmeta image


subcommands usage and parameters:

1) version

usage: python image_generation_tool version [-h]

optional arguments:
  -h, --help  show this help message and exit

2) generate_boot_image

usage: image_generation_tool generate_boot_image [-h]
       --tool_version TOOL_VERSION --vendor_bundle PATH --kernel IMAGE
       --ramdisk IMAGE --output IMAGE

optional arguments:
  -h, --help            show this help message and exit

required arguments:
  --tool_version TOOL_VERSION  Tool version e.g. 1.0
  --vendor_bundle              Path to vendor bundle e.g. foo/bar/boo/
  --kernel IMAGE               Path to kernel zImage e.g. foo/bar/zImage
  --ramdisk IMAGE              Path to ramdisk.img e.g. foo/bar/ramdisk.img
  --output IMAGE               Path to generated boot.img e.g. foo/bar/boot.img

3) generate_vendor_boot_image

usage: image_generation_tool generate_vendor_boot_image [-h]
       --tool_version TOOL_VERSION --vendor_bundle PATH --dtb IMAGE
       --vendor_ramdisk IMAGE --output IMAGE

optional arguments:
  -h, --help            show this help message and exit

required arguments:
  --tool_version TOOL_VERSION  Tool version e.g. 1.2
  --vendor_bundle              Path to vendor bundle e.g. foo/bar/boo/
  --dtb IMAGE                  Path to dtb.img e.g. foo/bar/dtb.img
  --vendor_ramdisk IMAGE       Path to vendor_ramdisk.img e.g. foo/bar/vendor_ramdisk.img
  --output IMAGE               Path to generated boot.img e.g. foo/bar/boot.img

4) generate_vbmeta_image

usage: image_generation_tool generate_vbmeta_image [-h]
        --tool_version TOOL_VERSION --vendor_bundle PATH --boot IMAGE
        --vendor_boot IMAGE --dtbo IMAGE --vendor IMAGE
        --odm IMAGE --output IMAGE

optional arguments:
  -h, --help            show this help message and exit

required arguments:
  --tool_version TOOL_VERSION  Tool version e.g. 1.0
  --vendor_bundle              Path to vendor bundle e.g. foo/bar/boo/
  --boot IMAGE                 Path to boot.img e.g. foo/bar/boot.img
  --vendor_boot IMAGE          Path to vendor_boot.img e.g. foo/bar/vendor_boot.img
  --dtbo IMAGE                 Path to dtbo.img e.g. foo/bar/dtbo.img
  --vendor IMAGE               Path to vendor.img e.g. foo/bar/vendor.img
  --odm IMAGE                  Path to odm.img e.g. foo/bar/odm.img
  --output IMAGE               Path to generated vbmeta.img e.g. foo/bar/vbmeta.img

5) info_vbmeta_image

usage: image_generation_tool info_vbmeta_image [-h]
       --tool_version TOOL_VERSION --vendor_bundle PATH --image IMAGE --output OUTPUT

optional arguments:
  -h, --help            show this help message and exit

required arguments:
  --tool_version TOOL_VERSION    Tool version e.g. 1.0
  --vendor_bundle                Path to vendor bundle e.g. foo/bar/boo/
  --image IMAGE                  vbmeta.img to show information about
                                 e.g. foo/bar/vbmeta.img
  --output OUTPUT                Output file name e.g. foo/bar/info_vbmeta.txt

6) generate_vendor_bundle

usage: python image_generation_tool generate_vendor_bundle [-h]
       --tool_version TOOL_VERSION --product_out PRODUCT_OUT

optional arguments:
  -h, --help            show this help message and exit

required arguments:
  --tool_version TOOL_VERSION  Tool version e.g. 1.0
  --product_out PRODUCT_OUT    The name of the target e.g. taro

This function is used to create a bundle during vendor SI compilation.
image_generation_tool along with the vendor and the system bundle can
be copied outside of Android tree to generate boot and vbmeta images.

Note: This is a function internal to the wrapper script and is not
expected to be used by the client and hence does not show up as a
supported subcommand in tools help.

Bundle location and contents:

    Location:
    out/host/linux-x86/bin

    Folder: vendor_image_gen_tool_bundle

    Files:
    i)   mkbootimg.py and avbtool.py in 'upstream_tools' sub-folder
    ii)  misc_info.txt in 'config' sub-folder
    iii) RSA test key in 'test_key' sub-folder
    iv)  Extracted public key in 'public_key' sub-folder

"""


"""
Script Versioning:

Version 1.0:
    - Provides subcommand 'version' to get tool version.
    - Provides subcommand 'generate_boot_image' to generate boot image.
    - Provides subcommand 'generate_vbmeta_image' to generate vbmeta image.
    - Provides subcommand 'info_vbmeta_image' to show vbmeta image information.

Version 1.1:
    - Provides user the option to provide vendor bundle path using
      following command - "--vendor_bundle PATH".

Version 1.2:
    - Provides subcommand 'generate_vendor_boot_image' to generate
      vendor_boot image.
    - Subcommand generate_vendor_bundle command now requires PRODUCT_OUT to be 
      passed with it.
"""
IMAGE_GEN_TOOL_VERSION_MAJOR = 1
IMAGE_GEN_TOOL_VERSION_MINOR = 2

"""Define Constants"""
BUNDLE_DIR = "vendor_image_gen_tool_bundle/"

class ImageGenError(Exception):
    """Raise exception with error message.

    Attributes:
        message: Error message.

    """

    def __init__(self, message):
      Exception.__init__(self, message)

class Dictionary:
    """Creates a dictionary using misc_info.txt"""

    def __init__():
        pass

    @staticmethod
    def LoadDictionaryFromFile(input_file, vendor_bundle):
        try:
            d = {}
            if vendor_bundle[-1] != '/':
                vendor_bundle += '/'
            with open(input_file, 'r') as misc_info:
                for line in misc_info:
                    line = line.strip()
                    if not line or line.startswith("#"):
                        continue
                    if "=" in line:
                        name, value = line.split("=", 1)
                        d[name] = value

            d['mkbootimg_tool'] = 'system/tools/mkbootimg/mkbootimg.py'
            d['avbtool_tool'] = 'external/avb/avbtool.py'
            d['boot_partition_name'] = 'boot'
            d['vendor_boot_partition_name'] = 'vendor_boot'
            d['dtbo_partition_name'] = 'dtbo'
            d['test_key'] = vendor_bundle + "test_key/" + d.get(
                'avb_vbmeta_key_path').split("/")[-1]
            d['public_key'] = vendor_bundle + "public_key/avb-sFeCqq.avbpubkey"

            return d
        except Exception as e:
            print(e)

class ImageGen(object):
    """Business logic for image generation command-line tool"""

    def generate_vendor_bundle(self, tool_version, product_out):
        """Implements 'generate_vendor_bundle' command"""

        """
        Create a bundle for below if the script is executed in the
        Android tree. Else, use the pre-generated bundle as is to
        execute the image_generation_tool.

            i)   mkbootimg.py, avbtool.py in 'upstream_tools' folder
            ii)  misc_info.txt in 'config' folder
            iii) RSA test key in 'test_key' folder
            iv)  Extracted public key in 'public_key' folder

        """
        try:
            def create_dir(path=None):
                """Creates directory in the path specified.

                Arguments:
                    path: The dir to create

                """

                if os.path.isdir(path) or path == None:
                    return
                try:
                    access_rights = 0o777
                    os.makedirs(path, access_rights)
                except OSError as error:
                    if error.errno != errno.EEXIST:
                        raise

            def copy_file(src, dest):
                """Copies file from source to destination.

                Arguments:
                    src: Source file to copy
                    dest: Destination to copy the file to

                """

                if os.path.isfile(dest):
                    return
                try:
                    shutil.copyfile(src, dest)
                    access_rights = 0o755
                    os.chmod(dest, access_rights)
                except:
                    print("Couldn't copy " + str(src) + " to " + str(dest))
                    raise

            def extract_public_key_from_test_key(upstream_tool_path, test_key,
                public_key):
                """Implements extract_public_key_from_test_key command.

                Arguments:
                    upstream_tool_path: Path to upstream_tools in bundle
                    test_key: Path to a RSA private key file
                    public_key: The file to write extracted public key

                """

                if os.path.isfile(public_key):
                    return

                args = [upstream_tool_path + "avbtool.py",
                        "extract_public_key", "--key", test_key,
                        "--output", public_key]

                p = subprocess.Popen(args,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                (pout, perr) = p.communicate()

                if p.wait() != 0:
                    raise ImageGenError(
                        'Failed to add hash footer: {}'.format(perr))

            def write_board_config_data_into_config_file(dest, croot):
                board_config_file = open(croot + 'device/qcom/' + product_out
                                  + '/BoardConfig.mk', 'r')
                lines = board_config_file.readlines()
                vendor_boot_data = {}
                for line in lines:
                    wordsList = line.split()
                    if len(wordsList) > 1 and wordsList[1] == ':=':
                        if wordsList[0] == 'BOARD_KERNEL_CMDLINE':
                            vendor_boot_data['vendor_boot_cmdline'
                            ] = '--vendor_cmdline ' + " ".join(wordsList[2:])
                            vendor_boot_data['vendor_boot_cmdline'
                            ] += ' buildvariant=' + os.environ['TARGET_BUILD_VARIANT']
                        elif wordsList[0] == 'BOARD_KERNEL_BASE':
                            vendor_boot_data['vendor_boot_base'
                            ] = '--base ' + " ".join(wordsList[2:])
                        elif wordsList[0] == 'BOARD_KERNEL_PAGESIZE':
                            vendor_boot_data['vendor_boot_pagesize'
                            ] = '--pagesize ' + " ".join(wordsList[2:])
                board_config_file.close()
                vendor_bundle_config_file = open(dest, 'a+')
                for d in vendor_boot_data:
                    t_data = d + '=' + vendor_boot_data[d]+ "\n"
                    vendor_bundle_config_file.write(t_data)
                vendor_bundle_config_file.close()


            if (tool_version == '1.0' or
                tool_version == '1.1' or
                tool_version == '1.2'):
                """
                Create dir to store
                    i)   mkbootimg.py, avbtool.py in 'upstream_tools' folder
                    ii)  misc_info.txt in 'config' folder
                    iii) RSA test key in 'test_key' folder
                    iv)  Extracted public key in 'public_key' folder

                """
                product_host_out = os.getcwd() + "/out/host/linux-x86/bin/"
                croot = os.getcwd() + "/"
                tools_dir = {
                    "config_dir": BUNDLE_DIR + "config/",
                    "upstream_tools_dir": BUNDLE_DIR + "upstream_tools/",
                    "test_key_dir": BUNDLE_DIR + "test_key/",
                    "public_key_dir": BUNDLE_DIR + "public_key/"
                }
                for d in tools_dir.values():
                    create_dir(os.path.join(product_host_out, d))

                """
                Copy misc_info.txt from $OUT to 'config' folder and
                create a dictionary

                """
                product_out = product_out.split('/')[-1]
                src = croot + "out/target/product/" + product_out + "/"
                src += "misc_info.txt"
                dest = product_host_out + tools_dir.get('config_dir') + "misc_info.txt"
                copy_file(src, dest)
                write_board_config_data_into_config_file(dest, croot)
                tools_dict = Dictionary.LoadDictionaryFromFile(dest, BUNDLE_DIR)

                """
                Copy mkbootimg.py, avbtool.py to 'upstream_tools' folder.
                Copy RSA test key to 'test_key' folder

                """

                mkbootimg_py_path = croot + tools_dict.get('mkbootimg_tool')
                avbtool_py_path = croot + tools_dict.get('avbtool_tool')
                vbmeta_test_key_path = croot + tools_dict.get(
                                    'avb_vbmeta_key_path')
                upstream_files = {
                    "mkbootimg_py": mkbootimg_py_path.split("/")[-1],
                    "avbtool_py": avbtool_py_path.split("/")[-1],
                    "test_key_pem": vbmeta_test_key_path.split("/")[-1]
                }

                copy_file(mkbootimg_py_path,
                        product_host_out + tools_dir.get('upstream_tools_dir')
                        + upstream_files.get('mkbootimg_py'))
                copy_file(avbtool_py_path,
                        product_host_out + tools_dir.get('upstream_tools_dir')
                        + upstream_files.get('avbtool_py'))
                copy_file(vbmeta_test_key_path,
                        product_host_out + tools_dir.get('test_key_dir')
                        + upstream_files.get('test_key_pem'))

                upstream_tool_path = product_host_out + tools_dir.get(
                    'upstream_tools_dir')
                test_key = product_host_out + tools_dict.get('test_key')
                public_key = product_host_out + tools_dict.get('public_key')

                """Extract public key from test key"""
                extract_public_key_from_test_key(upstream_tool_path,
                                                test_key,
                                                public_key)
            else:
                print("\nversion specified does not match supported tool versions."
                    " Use 'python image_generation_tool version' to"
                    " get the latest tool version\n")
        except Exception as e:
            print(e)

    def get_tool_version(self):
        """Returns the image generation tool version"""
        try:
            return 'image_generation_tool {}.{}'.format(
                IMAGE_GEN_TOOL_VERSION_MAJOR, IMAGE_GEN_TOOL_VERSION_MINOR)
        except Exception as e:
            print(e)

    def generate_boot_image(self, tool_version, vendor_bundle,
                            kernel, ramdisk, output):
        """Implements 'generate_boot_image' command.

        Arguments:
            tool_version: Tool version
            vendor_bundle: Path to vendor bundle
            kernel: Path to kernel zImage
            ramdisk: Path to ramdisk image
            mkbootimg_args: Boot header_version
            mkbootimg_version_args: os_version and os_patch_level
            output: File to write the image to

        """
        try:

            if vendor_bundle[-1] != '/':
                vendor_bundle += '/'

            if tool_version == '1.0' or tool_version == '1.1':
                input_file = vendor_bundle + "config/misc_info.txt"
                config_dict = Dictionary.LoadDictionaryFromFile(input_file,
                                                                vendor_bundle)

                args = [vendor_bundle + "upstream_tools/mkbootimg.py",
                        "--kernel", kernel,
                        "--ramdisk", ramdisk, "--output", output]

                mkbootimg_args = config_dict.get('mkbootimg_args')
                if mkbootimg_args and mkbootimg_args.strip():
                    args.extend(shlex.split(mkbootimg_args))

                mkbootimg_version_args = config_dict.get('mkbootimg_version_args')
                if mkbootimg_version_args and mkbootimg_version_args.strip():
                    args.extend(shlex.split(mkbootimg_version_args))

                p = subprocess.Popen(args,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                (pout, perr) = p.communicate()

                if p.wait() != 0:
                    raise ImageGenError(
                        'Failed to generate boot image: {}'.format(perr))
                else:
                    print("\nGenerated boot.img successfully!\n")
            else:
                print("\nTool version specified does not match supported versions."
                    " Use 'python image_generation_tool version' to"
                    " get the latest tool version\n")
        except Exception as e:
            print(e)

    def generate_vendor_boot_image(self, tool_version, vendor_bundle, dtb,
                                  vendor_ramdisk, output):
        """Implements 'generate_vendor_boot_image' command.

        Arguments:
            tool_version: Tool version
            vendor_bundle: Path to vendor bundle
            dtb: Path to dtb.img
            vendor_ramdisk: Path to vendor_ramdisk.img
            mkbootimg_args: Boot header_version
            mkbootimg_version_args: os_version and os_patch_level
            output: File to write the image to

        """
        try:

            if vendor_bundle[-1] != '/':
                vendor_bundle += '/'

            if tool_version == '1.2':
                input_file = vendor_bundle + "config/misc_info.txt"
                config_dict = Dictionary.LoadDictionaryFromFile(input_file, vendor_bundle)
                args = [vendor_bundle + "upstream_tools/mkbootimg.py", "--dtb", dtb]
                vendor_boot_cmdline = config_dict.get('vendor_boot_cmdline')
                if vendor_boot_cmdline and vendor_boot_cmdline.strip():
                    tempSplitList = vendor_boot_cmdline.split()
                    args.extend([tempSplitList[0]," ".join(tempSplitList[1:])])

                vendor_boot_base = config_dict.get('vendor_boot_base')
                if vendor_boot_base and vendor_boot_base.strip():
                    args.extend(shlex.split(vendor_boot_base))

                vendor_boot_pagesize = config_dict.get('vendor_boot_pagesize')
                if vendor_boot_pagesize and vendor_boot_pagesize.strip():
                    args.extend(shlex.split(vendor_boot_pagesize))

                mkbootimg_args = config_dict.get('mkbootimg_args')
                if mkbootimg_args and mkbootimg_args.strip():
                    args.extend(shlex.split(mkbootimg_args))

                mkbootimg_version_args = config_dict.get('mkbootimg_version_args')
                if mkbootimg_version_args and mkbootimg_version_args.strip():
                    args.extend(shlex.split(mkbootimg_version_args))

                args.extend(["--vendor_ramdisk", vendor_ramdisk, "--vendor_boot", output])
                p = subprocess.Popen(args,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                (pout, perr) = p.communicate()

                if p.wait() != 0:
                    raise ImageGenError(
                        'Failed to generate vendor boot image: {}'.format(perr))
                else:
                    print("\nGenerated vendor_boot.img successfully!\n")
            else:
                print("\nTool version specified does not match supported versions."
                    " Use 'python image_generation_tool version' to"
                    " get the latest tool version\n")
        except Exception as e:
            print(e)

    def generate_vbmeta_image(self, tool_version, vendor_bundle, boot, vendor_boot, dtbo,
        vendor, odm, output):
        """Implements 'generate_vbmeta_image' command.

        Arguments:
            tool_version: Tool version
            vendor_bundle: Path to vendor bundle
            boot: Path to boot.img
            vendor_boot: Path to vendor_boot.img
            dtbo: Path to dtbo.img
            vendor: Path to vendor.img
            odm: Path to odm.img
            test_key: Path to RSA private key file
            public_key: Extracted public key
            avb_vbmeta_algorithm: Name of algorithm to use
            avb_vbmeta_system_rollback_index_location: vbmeta_system
                rollback index location
            output: File to write the image to

        """

        try:
            def add_hash_footer_to_image(image, partition_name, partition_size,
                partition_avb_args):
                """Implements add_hash_footer_to_image command.

                Arguments:
                    image: File to add the footer to
                    partition_name: Name of the partition (without A/B suffix)
                    partition_size: Size of the partition
                    partition_avb_args: Key:Value pairs for build fingerprint,
                        os_version and os_security_patch

                """

                args = [vendor_bundle + "upstream_tools/avbtool.py",
                        "add_hash_footer", "--image", image,
                        "--partition_name", partition_name,
                        "--partition_size", partition_size]

                avb_args = partition_avb_args
                if avb_args and avb_args.strip():
                    args.extend(shlex.split(avb_args))

                p = subprocess.Popen(args,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                (pout, perr) = p.communicate()

                if p.wait() != 0:
                    raise ImageGenError(
                        'Failed to add hash footer: {}'.format(perr))

            def add_hash_footer_to_vbmeta_images(boot, vendor_boot, dtbo,
                conf_dict):
                """Implements add_hash_footer_to_vbmeta_images command.

                Arguments:
                    boot: Path to boot.img
                    vendor_boot: Path to vendor_boot.img
                    dtbo: Path to dtbo.img
                    config_dict: Dictionary with key:value pairs for all configs

                """

                image_list = [boot, vendor_boot, dtbo]

                partition_name_list = [
                    conf_dict.get('boot_partition_name'),
                    conf_dict.get('vendor_boot_partition_name'),
                    conf_dict.get('dtbo_partition_name')]

                partition_size_list = [
                    conf_dict.get('boot_size'),
                    conf_dict.get('vendor_boot_size'),
                    conf_dict.get('dtbo_size')]

                partition_avb_args_list = [
                    conf_dict.get('avb_boot_add_hash_footer_args'),
                    conf_dict.get('avb_vendor_boot_add_hash_footer_args'),
                    conf_dict.get('avb_dtbo_add_hash_footer_args')]

                for index, image in enumerate(image_list):
                    add_hash_footer_to_image(image,
                                            partition_name_list[index],
                                            partition_size_list[index],
                                            partition_avb_args_list[index])

            if vendor_bundle[-1] != '/':
                vendor_bundle += '/'

            if tool_version == '1.0' or tool_version == '1.1':
                input_file = vendor_bundle + "config/misc_info.txt"
                config_dict = Dictionary.LoadDictionaryFromFile(input_file, vendor_bundle)

                """Add hash_footer to boot, vendor_boot and dtbo images"""
                add_hash_footer_to_vbmeta_images(boot, vendor_boot, dtbo, config_dict)

                """Generate vbmeta image"""
                args = [vendor_bundle + "upstream_tools/avbtool.py", "make_vbmeta_image",
                        "--output", output, "--key", config_dict.get('test_key'),
                        "--algorithm", config_dict.get('avb_vbmeta_algorithm')]

                args.extend(["--include_descriptors_from_image", vendor,
                            "--include_descriptors_from_image", vendor_boot])

                chain_partition_args = "{}:{}:{}".format("vbmeta_system",
                    config_dict.get('avb_vbmeta_system_rollback_index_location'),
                    config_dict.get('public_key'))
                args.extend(["--chain_partition", chain_partition_args])

                args.extend(["--include_descriptors_from_image", boot,
                            "--include_descriptors_from_image", odm,
                            "--include_descriptors_from_image", dtbo])

                avb_vbmeta_args = config_dict.get('avb_vbmeta_args')
                if avb_vbmeta_args and avb_vbmeta_args.strip():
                    args.extend(shlex.split(avb_vbmeta_args))

                p = subprocess.Popen(args,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                (pout, perr) = p.communicate()

                if p.wait() != 0:
                    raise ImageGenError(
                        'Failed to generate vbmeta image: {}'.format(perr))
                else:
                    print("\nGenerated vbmeta.img successfully!\n")
            else:
                print("\nTool version specified does not match supported versions."
                    " Use 'python image_generation_tool version' to"
                    " get the latest tool version\n")
        except Exception as e:
            print(e)


    def info_vbmeta_image(self, tool_version, vendor_bundle, image, output):
        """Implements 'info_vbmeta_image' command.

        Arguments:
            tool_version: Tool version
            vendor_bundle: Path to vendor bundle
            image_filename: Image file to get information from
            output: Output file to write human-readable information to

        """
        try:
            if vendor_bundle[-1] != '/':
                vendor_bundle += '/'

            if tool_version == '1.0' or tool_version == '1.1':
                args = [vendor_bundle + "upstream_tools/avbtool.py", "info_image",
                        "--image", image, "--output", output]
                p = subprocess.Popen(args,
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                (pout, perr) = p.communicate()

                if p.wait() != 0:
                    raise ImageGenError(
                        'Failed to get vbmeta image info: {}'.format(perr))
                else:
                    print("\nSuccessfully extracted vbmeta.img information!\n")
            else:
                print("\nTool version specified does not match supported versions."
                    " Use 'python image_generation_tool version' to"
                    " get the latest tool version\n")
        except Exception as e:
            print(e)


class ImageGenTool(object):
    """Object for ImageGenTool command-line tool"""

    def __init__(self):
        """Initializer method"""
        self.image_gen = ImageGen()

    def parse_args_and_init(self, argv):
        try:
            """Parse cmdline and initialize """
            parser = argparse.ArgumentParser()
            subparsers = parser.add_subparsers(title='subcommands',
            metavar='{version, generate_boot_image, generate_vbmeta_image, '
            +'generate_vendor_boot_image, info_vbmeta_image}')

            """Sub command to create vendor SI bundle needed for the tool"""
            sub_parser = subparsers.add_parser('generate_vendor_bundle')
            required_args = sub_parser.add_argument_group('required arguments')
            required_args.add_argument('--tool_version',
                                    help='Tool version e.g. 1.0',
                                    required=True)
            required_args.add_argument('--product_out',
                                    help="The name of the target e.g. taro",
                                    required=True)
            required_args.add_argument('--vendor_bundle',
                                    help="Path to vendor bundle e.g. foo/bar/boo/",
                                    default="vendor_image_gen_tool_bundle/")
            sub_parser.set_defaults(func=self.generate_vendor_bundle)

            """Sub command to query the tool version"""
            sub_parser = subparsers.add_parser('version',
                                            help='Prints the version of'
                                                    ' image_generation_tool')
            sub_parser.set_defaults(func=self.get_tool_version)

            """Sub command to generate boot image"""
            sub_parser = subparsers.add_parser('generate_boot_image',
                                            help='Generates boot image')
            required_args = sub_parser.add_argument_group('required arguments')
            required_args.add_argument('--tool_version',
                                    help='Tool version e.g. 1.0',
                                    required=True)
            required_args.add_argument('--vendor_bundle',
                                    help="Path to vendor bundle e.g. foo/bar/boo/",
                                    default="vendor_image_gen_tool_bundle/")
            required_args.add_argument('--kernel',
                                    help='Path to kernel zImage'
                                            ' e.g. foo/bar/zImage',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--ramdisk',
                                    help='Path to ramdisk.img'
                                            ' e.g. foo/bar/ramdisk.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--output',
                                    help='Path to generated boot.img'
                                            ' e.g. foo/bar/boot.img',
                                    metavar='IMAGE',
                                    required=True)
            sub_parser.set_defaults(func=self.generate_boot_image)

            """Sub command to generate vendor_boot image"""
            sub_parser = subparsers.add_parser('generate_vendor_boot_image',
                                            help='Generates vendor_boot image.'+
                                            'Note: Available only from verion '+
                                            '1.2 onwards.')
            required_args = sub_parser.add_argument_group('required arguments')
            required_args.add_argument('--tool_version',
                                    help='Tool version e.g. 1.2',
                                    required=True)
            required_args.add_argument('--vendor_bundle',
                                    help="Path to vendor bundle e.g. foo/bar/boo/",
                                    default="vendor_image_gen_tool_bundle/")
            required_args.add_argument('--dtb',
                                    help='Path to dtb.img'
                                            ' e.g. foo/bar/dtb',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--vendor_ramdisk',
                                    help='Path to vendor_ramdisk.img'
                                            ' e.g. foo/bar/vendor_ramdisk.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--output',
                                    help='Path to generated vendor_boot.img'
                                            ' e.g. foo/bar/vendor_boot.img',
                                    metavar='IMAGE',
                                    required=True)
            sub_parser.set_defaults(func=self.generate_vendor_boot_image)

            """Sub command to generate vbmeta image"""
            sub_parser = subparsers.add_parser('generate_vbmeta_image',
                                            help='Generates vbmeta image')
            required_args = sub_parser.add_argument_group('required arguments')
            required_args.add_argument('--tool_version',
                                    help='Tool version e.g. 1.0',
                                    required=True)
            required_args.add_argument('--vendor_bundle',
                                    help="Path to vendor bundle e.g. foo/bar/boo/",
                                    default="vendor_image_gen_tool_bundle/")
            required_args.add_argument('--boot',
                                    help='Path to boot.img'
                                            ' e.g. foo/bar/boot.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--vendor_boot',
                                    help='Path to vendor_boot.img'
                                            ' e.g. foo/bar/vendor_boot.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--dtbo',
                                    help='Path to dtbo.img'
                                            ' e.g. foo/bar/dtbo.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--vendor',
                                    help='Path to vendor.img'
                                            ' e.g. foo/bar/vendor.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--odm',
                                    help='Path to odm.img'
                                            ' e.g. foo/bar/odm.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--output',
                                    help='Path to generated vbmeta.img'
                                            ' e.g. foo/bar/vbmeta.img',
                                    metavar='IMAGE',
                                    required=True)
            sub_parser.set_defaults(func=self.generate_vbmeta_image)

            """Sub command to show info about vbmeta image"""
            sub_parser = subparsers.add_parser('info_vbmeta_image',
                                            help='Shows information'
                                                    ' about vbmeta image')
            required_args = sub_parser.add_argument_group('required arguments')
            required_args.add_argument('--tool_version',
                                    help='Tool version e.g. 1.0',
                                    required=True)
            required_args.add_argument('--vendor_bundle',
                                    help="Path to vendor bundle e.g. foo/bar/boo/",
                                    default="vendor_image_gen_tool_bundle/")
            required_args.add_argument('--image',
                                    help='vbmeta.img to show information about'
                                            ' e.g. foo/bar/vbmeta.img',
                                    metavar='IMAGE',
                                    required=True)
            required_args.add_argument('--output',
                                    help='Output file name'
                                            ' e.g. foo/bar/info_vbmeta.txt',
                                    required=True)
            sub_parser.set_defaults(func=self.info_vbmeta_image)

            args = parser.parse_args(argv[1:])

            try:
                args.func(args)
            except ImageGenError as e:
                sys.stderr.write('{}: {}\n'.format(argv[0], str(e)))
                sys.exit(1)
        except Exception as e:
            print(e)


    def generate_vendor_bundle(self, args):
        """Implements 'generate_vendor_bundle' sub-command"""
        self.image_gen.generate_vendor_bundle(args.tool_version,
                                              args.product_out)

    def get_tool_version(self, _):
        """Implements 'version' sub-command"""
        print(self.image_gen.get_tool_version())

    def generate_boot_image(self, args):
        """Implements 'generate_boot_image' sub-command"""
        self.image_gen.generate_boot_image(args.tool_version,
                                           args.vendor_bundle,
                                           args.kernel,
                                           args.ramdisk,
                                           args.output)

    def generate_vendor_boot_image(self, args):
        """Implements 'generate_vendor_boot_image' sub-command"""
        self.image_gen.generate_vendor_boot_image(args.tool_version,
                                                  args.vendor_bundle,
                                                  args.dtb,
                                                  args.vendor_ramdisk,
                                                  args.output)

    def generate_vbmeta_image(self, args):
        """Implements 'generate_vbmeta_image' sub-command"""
        self.image_gen.generate_vbmeta_image(args.tool_version,
                                             args.vendor_bundle,
                                             args.boot, args.vendor_boot,
                                             args.dtbo, args.vendor,
                                             args.odm, args.output)

    def info_vbmeta_image(self, args):
        """Implements 'info_vbmeta_image' sub-command"""
        self.image_gen.info_vbmeta_image(args.tool_version,
                                         args.vendor_bundle,
                                         args.image,
                                         args.output)

if __name__ == '__main__':
    tool = ImageGenTool()
    tool.parse_args_and_init(sys.argv)
