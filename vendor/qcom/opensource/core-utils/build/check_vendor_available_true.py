# Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
#
# Changes from Qualcomm Innovation Center are provided under the following license:
# Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import io
import os
import subprocess
import re
import sys

vendor_available_true_list = set()
sys_abi_lib_list = set()
sys_abi_new_lib_list = set()
cnt_module = 0

def print_new_libs():
    if len(sys_abi_new_lib_list) > 0:
        print ("Count of newly added modules are: %s\n" %len(sys_abi_new_lib_list))
        for file_name in sys_abi_new_lib_list:
            print("    %s" % file_name)
    else:
        print ("Count of newly added modules are: %s" %len(sys_abi_new_lib_list))
    print()
    return None

def add_libs_to_sys_abi_lib_list(file_name):
    if os.path.isfile(file_name):
       os.chmod(file_name , 0o777)
       bp_file = open(file_name, 'r')
       while True:
             line1 = bp_file.readline()
             if re.search('name:', line1):
                line1 = line1.strip()
                line1 = line1.rstrip("\n")
                line1 = line1.rstrip(",")
                line1 = line1.rstrip("\"")
                line1 = line1.lstrip("name:")
                line1 = line1.strip()
                line1 = line1.lstrip("\"")
                line1 = line1+'.so'
                line2 = bp_file.readline()
                if re.search('vendor_available: true', line2):
                   sys_abi_lib_list.add(line1)
                if not line2:
                   break
             if not line1:
                break

       bp_file.close()
    return None

def check_vendor_available_true(line,file_name):
    global vendor_available_true_list
    if re.search(r'vendor_available: true',line):
        vendor_available_true_list.add(file_name)
        add_libs_to_sys_abi_lib_list(file_name)

def scan_files(file_list):
    global cnt_module

    # Scan each file
    for f in file_list:
        try:
            with io.open(ANDROID_BUILD_TOP+f, errors='ignore') as o_file:
                lines_itr = iter(o_file.readlines())
                for line in lines_itr:
                    line = line.strip()
                    if not line.startswith('#'):

                        # Take care of backslash (\) continuation
                        while line.endswith('\\'):
                            try:
                                line = line[:-1] + next(lines_itr).strip()
                            except StopIteration:
                                line = line[:-1]

                        if re.match(r'.*/Android.bp', f):
                            check_vendor_available_true(line, f)

                cnt_module = 0

        except IOError:
            print("Error opening file %s" % f)


def print_vendor_available_true_modules():

    found_errors = False

    if len(vendor_available_true_list) > 0:
        print("count of vendor_available_true_list:  %s" % len(vendor_available_true_list))
        print("\n\nprint_vendor_available_true_modules are:.")
        print()
        for file_name in vendor_available_true_list:
            print("    %s" % file_name)
        found_errors = True

    return found_errors

def print_changes(android_top_dir, prev_lib64_dir, new_lib64_dir):
    ABIGAIL_DIR = android_top_dir + '..' + '/' + 'QIIFA_abigail/QIIFA-fwk/plugins/qiifa_api_management' + '/'
    QIIFA_TOOLS_DIR = android_top_dir  + '..' + '/' + 'QIIFA_abigail/QIIFA-tools' + '/'
    ABIGAIL_DIFFER = ABIGAIL_DIR + 'abigail_core.py'

    for lib_name in sys_abi_lib_list:
        COMPARE_PREV_LIB = prev_lib64_dir + lib_name
        COMPARE_NEW_LIB  = new_lib64_dir  + lib_name
        if os.path.isfile(COMPARE_PREV_LIB):
            if os.path.isfile(COMPARE_NEW_LIB):
               print("\nABI diffs for: %s" %COMPARE_NEW_LIB)
               cmd = 'exec ' + ABIGAIL_DIFFER + ' ' +  '--qiifa_tool' + ' ' + QIIFA_TOOLS_DIR \
                          + ' ' + '--diff' + ' ' + COMPARE_PREV_LIB + ' ' + COMPARE_NEW_LIB
               print("abigail differ cmd: %s\n\n" % cmd)
               print(); print()
               os.system(cmd)
               print()
        else:
            if os.path.isfile(COMPARE_NEW_LIB):
               sys_abi_new_lib_list.add(COMPARE_NEW_LIB)

    return None

def print_help_message():
    print("Usage:")
    print("source build/envsetup.sh && lunch qssi_64-userdebug \npython3 vendor/qcom/opensource/core-utils/build/check_vendor_available_true.py \
           <PREV_VER_DIR_NAME> trunk-fs-release ks-qcom-main 2>&1 | tee compare_log.txt")
    print("where <PREV_VER_DIR_NAME> is sibling directory name of previous version to compare.")
    # For example
    #print("source build/envsetup.sh && lunch qssi_64-userdebug \npython3 vendor/qcom/opensource/core-utils/build/check_vendor_available_true.py \
    #       lanaib trunk-fs-release ks-qcom-main 2>&1 | tee compare_log.txt")

    return None

def check_for_help():
    if (len(sys.argv) < 4):
        print_help_message()
        exit(1)
    if (sys.argv[1] == "-h"):
        print_help_message()
        exit(1)
    if (sys.argv[1] == "--h"):
        print_help_message()
        exit(1)

    return None

def get_clang_version(top_dir):

    file_name = top_dir + 'soong/cc/config/global.go'
    #file_name = top_dir + 'build/soong/cc/config/global.go'
    if os.path.isfile(file_name):
       os.chmod(file_name , 0o777)
       clang_file = open(file_name, 'r')
       notfound = True
       while (notfound == True):
             line = clang_file.readline()
             if re.search('ClangDefaultVersion', line):
                notfound = False
                line = line.strip()
                line = line.lstrip("ClangDefaultVersion")
                line = line.strip()
                line = line.rstrip("\n")
                line = line.rstrip("\"")
                line = line.lstrip("=")
                line = line.strip()
                line = line.lstrip("\"")
                break
             if not line:
                line = ""
                break

       clang_file.close()

    return line

def main():
    print()

    global ANDROID_BUILD_TOP
    global PREV_ANDROID_BUILD_TOP
    global NEW_LIB64_DIR
    global PREV_LIB64_DIR
    global PREV_ANDROID_BUILD_DIR
    global KEYSTONE_TOP_DIR
    global TRUNK_FS_TOP_DIR

    ANDROID_BUILD_TOP = os.environ.get('ANDROID_BUILD_TOP') + '/'
    check_for_help()

    PREV_ANDROID_BUILD_DIR = sys.argv[1]
    TRUNK_FS_TOP_DIR       = sys.argv[2]
    KEYSTONE_TOP_DIR       = sys.argv[3]
    PREV_ANDROID_BUILD_TOP = ANDROID_BUILD_TOP + '..' + '/' + PREV_ANDROID_BUILD_DIR + '/'
    TRUNK_FS_TOP_DIR = ANDROID_BUILD_TOP + '..' + '/' + TRUNK_FS_TOP_DIR + '/'
    KEYSTONE_TOP_DIR = ANDROID_BUILD_TOP + '..' + '/' + KEYSTONE_TOP_DIR + '/'

    trunk_fs_clang_version = get_clang_version(TRUNK_FS_TOP_DIR)
    keystone_clang_version = get_clang_version(KEYSTONE_TOP_DIR)
    current_clang_version = get_clang_version(ANDROID_BUILD_TOP+'build'+'/')
    prev_clang_version    = get_clang_version(PREV_ANDROID_BUILD_TOP+'build'+'/')
    print(); print()
    print("CLANG Versions are:")
    print("Trunk-Fs CLANG Version:  %s" % trunk_fs_clang_version)
    print("Keystone CLANG Version:  %s" % keystone_clang_version)
    print("Current  CLANG Version:  %s" % current_clang_version)
    print("Previous CLANG Version:  %s" % prev_clang_version)
    print()

    subdirs = os.listdir()
    #subdirs = ["frameworks"]
    subdir_abspath_str = " ".join([ANDROID_BUILD_TOP+i for i in subdirs])

    print("TOP DIR: %s" % ANDROID_BUILD_TOP)
    print(); print()
    NEW_LIB64_DIR = ANDROID_BUILD_TOP + 'out/target/product/qssi_64/system/lib64' + '/'
    PREV_LIB64_DIR = PREV_ANDROID_BUILD_TOP + 'out/target/product/qssi_64/system/lib64' + '/'

    # Find all files and convert to relative path
    with open(os.devnull, 'w') as dev_null:
        files = subprocess.check_output(
            """find %s -type f \( -iname '*.bp' \); :;""" % subdir_abspath_str, shell=True, stderr=dev_null)
        files = files.decode().strip()
        files = files.replace(ANDROID_BUILD_TOP, '')
        files = files.split('\n')
    scan_files(files)

    found_errors = print_vendor_available_true_modules()
    print(); print(); print()
    print("Start ABI Diffs: ")
    print_changes(ANDROID_BUILD_TOP, PREV_LIB64_DIR, NEW_LIB64_DIR)
    print_new_libs()

    if found_errors:
        exit(1)
    else:
        exit(0)


if __name__ == '__main__':
    main()
