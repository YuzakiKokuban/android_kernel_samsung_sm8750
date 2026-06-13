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
# Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
import io
import os
import subprocess
import re
import sys
import glob


# Dynamically get whitelists from device/qcom/<target>/
ANDROID_BUILD_TOP = os.environ.get('ANDROID_BUILD_TOP') + '/'
TARGET_PRODUCT = os.environ.get('TARGET_PRODUCT')
QCPATH = os.environ.get('QCPATH')
TARGET_BOARD_PLATFORM = TARGET_PRODUCT
sys.path.insert(1, "%sdevice/qcom/%s" % (ANDROID_BUILD_TOP, TARGET_PRODUCT))
sys.path.insert(1, "%sdevice/qcom/qssi_64" % ANDROID_BUILD_TOP)
board_config_files = []
product_config_files = []
inherited_files_product = []
inherited_files_board = []


#Hard coded path macros
SRC_TARGET_DIR = "build/make/target"
QCPATH = "vendor/qcom/proprietary"
QC_PROP_ROOT = "vendor/qcom/proprietary"
target_base_product = TARGET_PRODUCT
ANDROID_PARTNER_GMS_HOME = "vendor/partner_gms"
OVERLAY_PATH_VND = "vendor/qcom/proprietary/resource-overlay"
TARGET_VENDOR = "qcom"
PREBUILT_BOARD_PLATFORM_DIR = TARGET_PRODUCT


try:
    if TARGET_PRODUCT == "qssi":
        print("Using legacy target whitelist for legacy qssi builds.")
        from makefile_whitelist import *
    else:
        from qssi_makefile_whitelist import *
        if "qssi" not in TARGET_PRODUCT:
            from target_makefile_whitelist import *
        print("Using target specific whitelist")
except ImportError:
    # Fall back to legacy
    print("Using legacy target whitelist.")
    from makefile_whitelist import *


# Enforcement sets for Android make files
kernel_errors = set()
shell_errors = set()
recursive_errors = set()
rm_errors = set()
datetime_errors = set()
local_copy_headers_errors = set()

# Scan for *.mk , *.sh, *.py
target_product_errors = set()
is_product_in_list_errors = set()
ro_build_product_errors = set()

# Enforcement sets for Product and  board config make files
foreach_errors = set()
is_macro_errors = set()
override_errors = set()
soong_namespace_errors = set()

# a set to maintain any new failed file opening
failed_filepaths = set()

# Variables for the kernel check
cur_file_name = str()
cnt_module = 0
fnd_c_include = False
fnd_add_dep = False

# Default Android.mk enforcement variables
BUILD_REQUIRES_KERNEL_DEPS = True
BUILD_BROKEN_USES_SHELL = False
BUILD_BROKEN_USES_RECURSIVE_VARS = True
BUILD_BROKEN_USES_RM_OUT = False
BUILD_BROKEN_USES_DATETIME = False
BUILD_BROKEN_USES_LOCAL_COPY_HEADERS = False
BUILD_BROKEN_USES_TARGET_PRODUCT = False
# Default product and board config enforcement variables
BUILD_BROKEN_USES_FOREACH = False
BUILD_BROKEN_USES_DEPRICATED_MACROS = False
BUILD_BROKEN_USES_SOONG_NAMESPACE = False
BUILD_BROKEN_USES_OVERRIDE = False


# Set whitelist variables if not defined
whitelist_vars = ["SHELL_WHITELIST", "RM_WHITELIST", "LOCAL_COPY_HEADERS_WHITELIST",
                  "DATETIME_WHITELIST", "TARGET_PRODUCT_WHITELIST", "RECURSIVE_WHITELIST", "KERNEL_WHITELIST",
                  "FAILED_FILEPATHS_WHITELIST","FOREACH_WHITELIST","MACRO_WHITELIST","OVERRIDE_WHITELIST",
                  "SOONG_WHITELIST"]
for w in whitelist_vars:
    if w not in globals():
        exec("%s = set()" % w, globals())
    if "VENDOR_" + w not in globals():
        exec("VENDOR_%s = set()" % w, globals())

def check_kernel_deps(line, file_name):
    global cur_file_name
    global cnt_module
    global fnd_c_include
    global fnd_add_dep

    if file_name != cur_file_name:
        cur_file_name = file_name
        cnt_module = 0
        fnd_c_include = False
        fnd_add_dep = False

    if re.match(r'LOCAL_C_INCLUDES.*KERNEL_OBJ/usr.*', line):
        fnd_c_include = True
    elif re.match(r'LOCAL_ADDITIONAL_DEPENDENCIES.*KERNEL_OBJ/usr.*', line):
        fnd_add_dep = True
    elif re.match(r'.*CLEAR_VARS.*', line):
        if cnt_module > 0:
            if fnd_c_include and not fnd_add_dep:
                kernel_errors.add(str(cnt_module)+'::'+file_name)
        fnd_c_include = False
        fnd_add_dep = False
        cnt_module += 1


def check_foreach(line,file_name):
    global foreach_errors
    if re.search(r'\$\(foreach\s+',line):
        foreach_errors.add(file_name)

def check_macro(line,file_name):
    global is_macro_errors

    if re.search(r'^[^#]*call(\s+)+is-board-platform',line):
        #Allow new macros accept for file conversions
        if re.search(r'^[^#]*call(\s+)+is-board-platform2',line):
            pass
        elif re.search(r'^[^#]*call(\s+)+is-board-platform-in-list2',line):
            pass
        else:
            is_macro_errors.add(file_name)

    if re.search(r'^[^#]*call(\s+)+is-vendor-board-platform',line):
        is_macro_errors.add(file_name)


def check_override(line,file_name):
    global override_errors

    line = line.strip()
    if re.search(r'override(\s+)',line):
        override_errors.add(file_name)


def check_soong_namespace(line,file_name):
    global soong_namespace_errors
    if re.search(r'SOONG_CONFIG_',line):
        soong_namespace_errors.add(file_name)


def check_shell(line, file_name):
    global shell_errors
    if re.match(r'.*\$\(shell.*', line):
        shell_errors.add(file_name)


def check_recursive(line, file_name):
    global recursive_errors
    if line.startswith('$'):
        pass
    elif re.match(r'.*[:+?=\>\<]=.*', line):
        pass
    elif re.match(r'.*=.*', line):
        recursive_errors.add(file_name)


def check_rm(line, file_name):
    global rm_errors
    if re.match(r'.*([\s]+|^|@)rm[\s]+.*', line):
        rm_errors.add(file_name)


def check_datetime(line, file_name):
    global datetime_errors
    if re.match(r'.*-Wno-error=date-time.*', line) or re.match(r'.*-Wno-date-time.*', line):
        datetime_errors.add(file_name)


def check_local_copy_headers(line, file_name):
    global local_copy_headers_errors
    if re.match(r'.*LOCAL_COPY_HEADERS.*', line):
        local_copy_headers_errors.add(file_name)


def check_target_product_related(line, file_name):
    global target_product_errors
    global is_product_in_list_errors
    global ro_build_product_errors
    if re.match(r'.*\$\(TARGET_PRODUCT\).*', line) or \
            re.match(r'.*\$\{TARGET_PRODUCT\}.*', line) or \
            re.match(r'.*\'TARGET_PRODUCT\'.*', line) or \
            re.match(r'.*\"TARGET_PRODUCT\".*', line) or \
            re.match(r'.*\$TARGET_PRODUCT.*', line):
        target_product_errors.add(file_name)

    if re.match(r'.*is-product-in-list.*', line):
        is_product_in_list_errors.add(file_name)

    if re.match(r'.*ro.build.product.*', line):
        ro_build_product_errors.add(file_name)


def resolve_symlink(file_path):
    file_path = file_path.strip()
    #Find a symbolic link for the file parsed as input, if any exists
    res_filepath = subprocess.check_output(
           """find %s -exec readlink -f {} \\; """ %file_path, shell=True).decode()
    if re.search(ANDROID_BUILD_TOP,res_filepath):
        res_filepath = re.sub(ANDROID_BUILD_TOP, '', res_filepath)
        res_filepath = res_filepath.strip()

    return res_filepath




def parse_makefile_for_includes(file_path , config_files, inherited_files):
    try:
        with io.open(ANDROID_BUILD_TOP+file_path, errors = 'ignore') as o_file:
            config_lines = iter(o_file.readlines())
            #Get absolute path by modifying string and resolving path MACROs
            for line in config_lines:
                line = line.strip()
                is_config_file = False
                # ignore line which are comments
                if re.match(r'^\s*#',line):
                    continue

                # Take care of backslash (\) continuation
                while line.endswith('\\'):
                    try:
                        line = line[:-1] + next(config_lines).strip()
                    except StopIteration:
                        line = line[:-1]

                #parse lines for include or inherit-product
                if re.search("(-)?include ",line):
                    if "wildcard" not in line:
                        is_config_file = True
                        line = line.replace('-include','',1)
                        line = line.replace('include ','',1)
                elif re.search("inherit-product(-if-exists)?",line):
                    if "wildcard" not in line:
                        if re.match(r'.*.mk', line):
                            is_config_file = True
                            line = re.split("inherit-product(-if-exists)?",line)
                            line = line[-1]
                            line = re.sub(',','',line)

                #Replacing file path MACROs
                if is_config_file:
                    if re.search("QCPATH",line):
                        line = line.replace('$','',1)
                        line = re.sub("QCPATH",QCPATH,line)

                    if re.search("SRC_TARGET_DIR",line):
                        line = line.replace('$','',1)
                        line = re.sub("SRC_TARGET_DIR" ,SRC_TARGET_DIR,line)

                    if re.search("QC_PROP_ROOT",line):
                        line = line.replace('$','',1)
                        line = re.sub("QC_PROP_ROOT",QC_PROP_ROOT,line)

                    if re.search("TOPDIR",line):
                        line = line.replace('$','',1)
                        line = re.sub("TOPDIR",'',line)

                    if re.search("TARGET_BOARD_PLATFORM",line):
                        line = line.replace('$','',2) #Exception for this macro as some file use this twice in the same line
                        line = re.sub("TARGET_BOARD_PLATFORM",TARGET_BOARD_PLATFORM,line)

                    if re.search("ANDROID_PARTNER_GMS_HOME",line):
                        line = line.replace('$','',1)
                        line = re.sub("ANDROID_PARTNER_GMS_HOME",ANDROID_PARTNER_GMS_HOME,line)

                    if re.search("TARGET_VENDOR",line):
                        line = line.replace('$','',1)
                        line = re.sub("TARGET_VENDOR",TARGET_VENDOR,line)

                    if re.search("OVERLAY_PATH_VND",line):
                        line = line.replace('$','',1)
                        line = re.sub("OVERLAY_PATH_VND",OVERLAY_PATH_VND,line)

                    if re.search("target_base_product",line):
                        line = line.replace('$','',1)
                        line = re.sub("target_base_product",target_base_product,line)

                    if re.search("PREBUILT_BOARD_PLATFORM_DIR",line):
                        line = line.replace('$','',1)
                        line = re.sub("PREBUILT_BOARD_PLATFORM_DIR",PREBUILT_BOARD_PLATFORM_DIR,line)

                    # Processed further only if all macros resolved.
                    if '$' not in line:
                      # Remove all whitepaces and paranthesis
                      line = re.sub(r"[\s\(\)]+","",line)

                    # Maintain a local list of all config files to include/add
                    config_files_to_add=[]
                    # Add the line/file with "include"
                    config_files_to_add.append(line)

                    #Resolve for * in file path names
                    if re.search(r'\*',line):
                        # Remove the flie path with * as we are resolving * below
                        config_files_to_add.remove(line)
                        # Resolve * in file path
                        glob_list = glob.glob(line)
                        # Add all resolved files to list of files to be included
                        config_files_to_add = config_files_to_add + glob_list

                    #Append the expanded glob files to config list
                    for file_add in config_files_to_add:
                           if file_add not in config_files:
                              config_files.append(file_add) #Append file if unique
                              inherited_files.append(file_add)

    except IOError:
        #Check if the file currently parsed is not in the whitelist ( new failures )
        if file_path not in VENDOR_FAILED_FILEPATHS_WHITELIST and \
           file_path not in FAILED_FILEPATHS_WHITELIST:
            if file_path.startswith('vendor/qcom') or \
               file_path.startswith('device/qcom') or \
               file_path.startswith('hardware/qcom') or \
               file_path.startswith('$'):
                warning_message = "WARNING: Error opening file: " + file_path
                print(warning_message)
            failed_filepaths.add(file_path)


def extract_product_board_makefile():
    global board_config_files,product_config_files,inherited_files_product,inherited_files_board

    #Explicitly adding boardConfig.mk and TARGET files to be parsed
    board_config_path_str = "device/qcom/"+TARGET_PRODUCT+"/BoardConfig.mk"
    product_config_path_str = "device/qcom/"+TARGET_PRODUCT+"/"+TARGET_PRODUCT+".mk"

    #Append BoardConfig to the board config list
    board_config_files.append(board_config_path_str)

    #Append target product makefile to the product config list
    product_config_files.append(product_config_path_str)

    # In future, the script parse the target_product.mk to find all the defs folder without hard-coding the file paths
    product_path_str = ANDROID_BUILD_TOP + "vendor/qcom/defs/product-defs"
    board_path_str = ANDROID_BUILD_TOP + "vendor/qcom/defs/board-defs"

    #Extract the split product make files from the vendor/qcom/defs folder
    split_product_config_files = subprocess.check_output(
            """find %s -iname "*.mk" -exec readlink -f {} \\;""" % product_path_str, shell=True)
    split_product_config_files = split_product_config_files.decode().strip()
    split_product_config_files = split_product_config_files.replace(ANDROID_BUILD_TOP, '')
    split_product_config_files = split_product_config_files.split('\n')

    split_board_config_files = subprocess.check_output(
            """find %s -iname "*.mk" -exec readlink -f {} \\;""" % board_path_str, shell=True)
    split_board_config_files = split_board_config_files.decode().strip()
    split_board_config_files = split_board_config_files.replace(ANDROID_BUILD_TOP, '')
    split_board_config_files = split_board_config_files.split('\n')

    #Chaining the two lists together
    board_config_files = board_config_files+split_board_config_files
    product_config_files = product_config_files+split_product_config_files

    #Loop over and parse all files in the list to recursively include/inherit-product other config makefiles
    for file in board_config_files:
        parse_makefile_for_includes(file,board_config_files,inherited_files_board)

    for file in product_config_files:
        parse_makefile_for_includes(file,product_config_files,inherited_files_product)

    #Concatenating the known vendor whitelist path and the failed to open file paths
    config_files_to_remove = failed_filepaths.union(VENDOR_FAILED_FILEPATHS_WHITELIST)
    #Concatenating the qssi failed to open files with the overall list
    config_files_to_remove = config_files_to_remove.union(FAILED_FILEPATHS_WHITELIST)

    #Looping over the config_files_to_remove to remove the files from product and board config list
    #The files failed to open have already been printed as a warning.
    #Removing the files will prevent the warning message from appearing multiple times
    for var in config_files_to_remove:
        if var in board_config_files:
            board_config_files.remove(var)
            inherited_files_board.remove(var)
        if var in product_config_files:
            product_config_files.remove(var)
            inherited_files_product.remove(var)


def scan_files(file_list):
    global fnd_c_include
    global fnd_add_dep
    global cnt_module

    # Scan each file
    for f in file_list:
        if f == '%s/makefile-violation-scanner.py' % QTI_BUILDTOOLS_DIR.replace(ANDROID_BUILD_TOP, ''):
            continue

        # create flags to check if the file is a android make file or product/board config make file
        is_android_make = False
        is_config_make = False

        if re.match(r'.*/Android.mk', f):
            is_android_make = True

        elif f in board_config_files or f in product_config_files:
            is_config_make = True

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

                        # check for android make file
                        if is_android_make:
                            # Check all makefile only issues
                            if (f not in SHELL_WHITELIST) and \
                               (f not in VENDOR_SHELL_WHITELIST):
                                check_shell(line, f)
                            if (f not in RM_WHITELIST) and \
                               (f not in VENDOR_RM_WHITELIST):
                                check_rm(line, f)
                            if (f not in LOCAL_COPY_HEADERS_WHITELIST) and \
                               (f not in VENDOR_LOCAL_COPY_HEADERS_WHITELIST):
                                check_local_copy_headers(line, f)
                            if (f not in DATETIME_WHITELIST) and \
                               (f not in VENDOR_DATETIME_WHITELIST):
                                check_datetime(line, f)
                            # if (f not in RECURSIVE_WHITELIST) and \
                            #    (f not in VENDOR_RECURSIVE_WHITELIST):
                            #     check_recursive(line, f)
                            if (f not in KERNEL_WHITELIST) and \
                               (f not in VENDOR_KERNEL_WHITELIST):
                                check_kernel_deps(line, f)

                        # check for product/board make file and Scan each product/board config file
                        elif is_config_make:
                            #Check for make file violations
                            if (f not in SHELL_WHITELIST) and \
                               (f not in VENDOR_SHELL_WHITELIST):
                               check_shell(line, f)

                            if (f not in FOREACH_WHITELIST) and \
                               (f not in VENDOR_FOREACH_WHITELIST):
                               check_foreach(line,f)

                            if (f not in MACRO_WHITELIST) and \
                               (f not in VENDOR_MACRO_WHITELIST):
                               check_macro(line,f)

                            if (f not in OVERRIDE_WHITELIST) and \
                               (f not in VENDOR_OVERRIDE_WHITELIST):
                               check_override(line,f)

                            if (f not in SOONG_WHITELIST) and \
                               (f not in VENDOR_SOONG_WHITELIST):
                               check_soong_namespace(line,f)

                            if (f not in RECURSIVE_WHITELIST) and \
                               (f not in VENDOR_RECURSIVE_WHITELIST):
                               check_recursive(line, f)


                        # Check TARGET_PRODUCT in all files
                        if (f not in TARGET_PRODUCT_WHITELIST) and \
                           (f not in VENDOR_TARGET_PRODUCT_WHITELIST):
                            check_target_product_related(line, f)

                # Check kernel issue at end of file (in case no CLEAR_VARS)
                if fnd_c_include and not fnd_add_dep:
                    kernel_errors.add(str(cnt_module)+'::'+f)
                fnd_c_include = False
                fnd_add_dep = False
                cnt_module = 0

        except IOError:
            print("Error opening file %s" % f)


def print_messages():

    found_errors = False

    if BUILD_REQUIRES_KERNEL_DEPS and len(kernel_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_kernel_error : %s" % len(kernel_errors))
        print("Error: Missing LOCAL_ADDITIONAL_DEPENDENCIES in below modules.")
        print("please use LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr")
        for error in sorted(kernel_errors):
            module = error.split("::")[0]
            file_name = error.split("::")[1]
            print("    Module: %s in %s" % (module, file_name))
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_SHELL) and len(shell_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_shell_error : %s" % len(shell_errors))
        print("Error: Using $(shell) in below files. Please remove usage of $(shell)")
        for file_name in sorted(shell_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_RECURSIVE_VARS) and len(recursive_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_recursive_error : %s" % len(recursive_errors))
        print("Warning: Using recursive assignment (=) in below files.")
        print("please review use of recursive assignment and convert to simple assignment (:=) if necessary.")
        for file_name in sorted(recursive_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_RM_OUT) and len(rm_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_rm_error : %s" % len(rm_errors))
        print("Error: Using rm in below makefiles. Please remove use of rm to prevent recompilation.")
        for file_name in sorted(rm_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_DATETIME) and len(datetime_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_datetime_error : %s" % len(datetime_errors))
        print("Error: Using CFLAG -Wno-error=date-time or -Wno-date-time in below makefiles. This may lead to varying build output.")
        print("Please remove use of this CFLAG.")
        for file_name in sorted(datetime_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_LOCAL_COPY_HEADERS) and len(local_copy_headers_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_local_copy_headers_error : %s" %
              len(local_copy_headers_errors))
        print("Error: Using local_copy_headers in below makefiles. This will be deprecated soon, please remove.")
        for file_name in sorted(local_copy_headers_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_TARGET_PRODUCT) and len(target_product_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_target_product_error : %s" % len(target_product_errors))
        print("Error: Using TARGET_PRODUCT in below makefiles. Please replace them with TARGET_BOARD_PLATFORM")
        for file_name in sorted(target_product_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_TARGET_PRODUCT) and len(is_product_in_list_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_is_product_in_list_error : %s" %
              len(is_product_in_list_errors))
        print("Error: Using is-product-in-list in below makefiles. Please replace them with is-board-platform-in-list")
        for file_name in sorted(is_product_in_list_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True
    if (not BUILD_BROKEN_USES_TARGET_PRODUCT) and len(ro_build_product_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_ro_build_product_error : %s" % len(ro_build_product_errors))
        print("Error: Using ro.build.product in below makefiles. Please replace them with ro.board.platform")
        for file_name in sorted(ro_build_product_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True

    if (not BUILD_BROKEN_USES_FOREACH) and len(foreach_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_foreach_errors : %s" % len(foreach_errors))
        print("Error: Using foreach in below makefiles. Please replace them with find-copy-subdir-files OR copy-files OR product-copy-files-by-pattern")
        print("Please refer this wiki for reference solution: https://confluence.qualcomm.com/confluence/display/ACore/FR83654+-+Prepare+product+config+makefiles+for+Bazel+migration")
        for file_name in sorted(foreach_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True

    if (not BUILD_BROKEN_USES_DEPRICATED_MACROS) and len(is_macro_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_macro_errors : %s" % len(is_macro_errors))
        print("Error: Using macro in below makefiles.")
        print("Please refer this wiki for reference solution: https://confluence.qualcomm.com/confluence/display/ACore/FR83654+-+Prepare+product+config+makefiles+for+Bazel+migration")
        for file_name in sorted(is_macro_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True

    if (not BUILD_BROKEN_USES_SOONG_NAMESPACE) and len(soong_namespace_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_soong_namespace_errors : %s" % len(soong_namespace_errors))
        print("Error: Using soong_namespace in below makefiles.")
        print("Please refer this wiki for reference solution: https://confluence.qualcomm.com/confluence/display/ACore/FR83654+-+Prepare+product+config+makefiles+for+Bazel+migration")
        for file_name in sorted(soong_namespace_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True

    if (not BUILD_BROKEN_USES_OVERRIDE) and len(override_errors) > 0:
        print("-----------------------------------------------------")
        print("cnt_override_errors : %s" % len(override_errors))
        print("Error: Using override_errors in below makefiles.")
        for file_name in sorted(override_errors):
            print("    %s" % file_name)
        print("-----------------------------------------------------")
        found_errors = True


    return found_errors


def main():

    global QTI_BUILDTOOLS_DIR, ANDROID_BUILD_TOP, TARGET_PRODUCT
    QTI_BUILDTOOLS_DIR = "%s/build" % os.environ.get('QTI_BUILDTOOLS_DIR')
    subdirs = ["hardware/qcom", "vendor/qcom", "device/qcom"]
    subdir_abspath_str = " ".join([ANDROID_BUILD_TOP+i for i in subdirs])

    print("-----------------------------------------------------")
    print(" Checking makefile errors ")
    print("      Checking dependency on kernel headers ......")
    print("      Checking $(shell) usage ......")
    print("      Checking recursive usage ......")
    print("      Checking rm usage ......")
    print("      Checking -Wno-error=date-time usage ......")
    print("      Checking local_copy_headers usage ......")
    print("      Checking TARGET_PRODUCT usage ......")
    print("      Checking is-product-in-list usage ......")
    print("      Checking ro.build.product usage ......")
    print("      Checking foreach usage ......")
    print("      Checking is-xxx macro usage ......")
    print("      Checking override usage ......")
    print("      Checking soong_config usage ......")
    print("-----------------------------------------------------")
    sys.stdout.flush()

    # Read configuration
    try:
        with open("%s/makefile_violation_config.mk" % QTI_BUILDTOOLS_DIR) as config_file:
            for line in config_file.readlines():
                line = line.strip()
                if not line.startswith('#'):
                    statement = line.strip().replace(":=", "=").replace(
                        "true", "True").replace("false", "False")
                    exec(statement, globals())
                else:
                    continue
    except IOError:
        pass  # Uses default values

    # Read enforcement override flags
    try:
        with open("%sdevice/qcom/%s/enforcement_override.mk" % (ANDROID_BUILD_TOP, TARGET_PRODUCT)) as override_file:
            for line in override_file.readlines():
                statement = line.strip().replace(":=", "=").replace(
                    "true", "True").replace("false", "False")
                exec(statement, globals())
    except IOError:
        pass  # No overrides for the target

    #Extracting product and board config make files
    extract_product_board_makefile()


    # Find all files and convert to relative path
    with open(os.devnull, 'w') as dev_null:
        files = subprocess.check_output(
            """find %s -type f \( -iname '*.mk' -o -iname '*.sh' -o -iname '*.py' \); :;""" % subdir_abspath_str, shell=True, stderr=dev_null)
        files = files.decode().strip()
        files = files.replace(ANDROID_BUILD_TOP, '')
        files = files.split('\n')

    # This loop will iterate over all the inherited/included make files
    # Resolve any symbolic links for the files that are parsed and add them to the product/board config lists
    for file in inherited_files_board:
        file_path = resolve_symlink(file)
        # if the file was a symbolic link, remove it from the list since the resolved file is added instead
        if file_path != file:
            board_config_files.remove(file)
        if file_path not in board_config_files:
            board_config_files.append(file_path)

    for file in inherited_files_product:
        file_path = resolve_symlink(file)
        #if the file was a symbolic link, remove it from the list since the resolved file is added instead
        if file_path != file:
            product_config_files.remove(file)
        if file_path not in product_config_files:
            product_config_files.append(file_path)

    # Append any file that isnt found in files list
    for file in board_config_files:
        if file not in files:
            if file.startswith('vendor/qcom') or \
               file.startswith('device/qcom') or \
               file.startswith('hardware/qcom'):
               files.append(file)

    for file in product_config_files:
        if file not in files:
            if file.startswith('vendor/qcom') or \
               file.startswith('device/qcom') or \
               file.startswith('hardware/qcom'):
               files.append(file)


    scan_files(files)

    found_errors = print_messages()
    if found_errors:
        exit(1)
    else:
        exit(0)


if __name__ == '__main__':
    main()
