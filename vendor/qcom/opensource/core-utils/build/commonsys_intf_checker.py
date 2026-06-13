#!/usr/bin/python
# -*- coding: utf-8 -*-
#Copyright (c) 2021 The Linux Foundation. All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are
#met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
#    * Neither the name of The Linux Foundation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
#WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
#ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
#BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear

import os,json,sys,re
import subprocess
from xml.etree import ElementTree as et

module_info_dict = {}
git_project_dict = {}
out_path = os.getenv("OUT")
croot = os.getenv("ANDROID_BUILD_TOP")
violated_modules = []
git_repository_list = []
whitelist_projects_list = []
whitelist_groups_list = []
qssi_install_keywords = ["system","system_ext","product"]
vendor_install_keywords = ["vendor"]
violation_file_path = out_path
aidl_metadata_file = croot + "/out/soong/.intermediates/system/tools/aidl/build/aidl_metadata_json/aidl_metadata.json"
noship_project_marker = os.path.join(croot,os.getenv("QCPATH"),"interfaces-noship")
aidl_metadata_dict = {}
manifest_artifacts = {}

def parse_xml_file(path):
    xml_element = None
    if os.path.isfile(path):
        try:
            xml_element = et.parse(path).getroot()
        except Exception as e:
            print("Exiting!! Xml Parsing Failed : " + path)
            sys.exit(1)
    else:
        print("Exiting!! File not Present : " + path)
        sys.exit(1)
    return xml_element

def load_json_file(path):
    json_dict = {}
    if os.path.isfile(path):
        json_file_handler = open(path,'r')
        try:
            json_dict = json.load(json_file_handler)
        except Exception as e:
            print("Exiting!! Json Loading Failed : " + path)
            sys.exit(1)
    else:
        print("Exiting!! File not Present : " + path)
        sys.exit(1)
    return json_dict


def find_and_update_git_project_path(path):
    for git_repository in manifest_artifacts["git_project_path"]:
        git_repository_mark = git_repository + "/"
        if git_repository_mark in path:
            return git_repository
    return path

def filter_whitelist_projects(integrity_violation_list):
    filtered_project_violation_list = []
    for violator in integrity_violation_list:
        if not ignore_whitelist_projects(violator):
            filtered_project_violation_list.append(violator)
    return filtered_project_violation_list

def print_violations_to_file(project_list):
    violation_flag = False
    violation_file_handler = open(violation_file_path + "/commonsys-intf-violator.txt", "w")
    violation_file_handler.write("############ Violation List ###########\n\n")
    for violator_project in project_list:
        violation_flag = True
        violation_file_handler.writelines("Git Project : " + str(violator_project)+"\n")
        violation_file_handler.writelines("Modules in Project \n")
        for module_metadata in project_list[violator_project]["module_list"]:
            violation_file_handler.writelines("Module name : " + module_metadata["module_name"])
            violation_file_handler.writelines("\n")
            violation_file_handler.writelines("Module path : " + module_metadata["install_path"])
            violation_file_handler.writelines("\n")
        violation_file_handler.writelines("\n################################################# \n\n")
    violation_file_handler.close()
    if commonsys_intf_enforcement and violation_flag:
        print("Commonsys-Intf Violation found !! Exiting Compilation !!")
        print("For details execute : cat $OUT/configs/commonsys-intf-violator.txt")
        return True
    else:
        return False

def check_for_hidl_aidl_intermediate_libs(module_name,class_type):
    #"VtsCallInfoV1_0Test" is a temp exception to avoid putting inrerface project in whitelist.
    if "@" in module_name or "-ndk" in module_name or re.search("-V.*-java",module_name) or "VtsCallInfoV1_0Test" in module_name:
        if class_type == "EXECUTABLES" or "-impl" in module_name:
            return False
        else:
            return True
    else:
        ## Refer aidl_metadata database
        for aidl_info in aidl_metadata_dict:
            if not aidl_info is None:
                aidl_module_name = aidl_info["name"]
                if aidl_module_name in module_name :
                    if class_type == "JAVA_LIBRARIES" or class_type == "SHARED_LIBRARIES" or "cpp-analyzer" in module_name:
                        return True
        return False

def ignore_whitelist_projects(project_name):
    for project in whitelist_projects_list :
        if project == project_name:
            return True
    return False

def is_dylib_file(filepath):
    DYLIB_SO_EXTENSION = ".dylib.so"
    return filepath.endswith(DYLIB_SO_EXTENSION)

def get_commonsys_intf_project_from_manifest():
    ## Read project information from manifest and consider
    ## projects which start with "vendor"
    global manifest_artifacts
    project_list_paths = []
    commonsys_intf_group_list = []
    manifest_root = parse_xml_file(croot + "/.repo/manifest.xml")
    for project in manifest_root.findall("project"):
        git_project_path = str(project.attrib.get('path'))
        aosp_flag = project.attrib.get('x-prj-type')
        groups = project.attrib.get('groups')
        if not aosp_flag == "aosp":
            if not git_project_path is None and git_project_path.startswith("vendor"):
                project_list_paths.append(git_project_path)
                if "qc-common-sys-intf" in groups:
                    commonsys_intf_group_list.append(git_project_path)
                if git_project_path not in project_list_paths:
                    project_list_paths.append(git_project_path)
    manifest_artifacts["git_project_path"] = project_list_paths
    manifest_artifacts["commonsys_intf_group"] =  commonsys_intf_group_list

def enforce_commonsys_intf_groups_checker(prj_list_build):
    prj_list_manifest = manifest_artifacts["commonsys_intf_group"]
    violation_flag = False
    for prj in prj_list_manifest:
        if prj not in prj_list_build and "QIIFA" not in prj and prj not in whitelist_groups_list:
           violation_flag = True
           print("Project cannot be classified as commonsys-intf : " + str(prj))

    for prj in prj_list_build:
        if prj not in prj_list_manifest and prj not in whitelist_groups_list:
            violation_flag = True
            print("Project should be marked as commonsys-intf in manifest : " + str(prj))
            for module in prj_list_build[prj]["module_list"]:
                print(module["install_path"])
    if commonsys_intf_enforcement and violation_flag:
        return violation_flag
    else:
        return False

def filter_whitelist_projects(project_list):
    filtered_project_list = {}
    for project in project_list:
        if not project in whitelist_projects_list:
            filtered_project_list[project] = project_list[project]
    return filtered_project_list


def enforce_commonsys_intf_integrity_checker(project_list):
     return print_violations_to_file(filter_whitelist_projects(project_list))

def find_commonsys_intf_project_paths_from_build_system():
    project_modules_map = {}
    commonsys_intf_project_list = {}
    ## Only interested in projects which starts with vendor
    project_interest_prefix = "vendor"
    for module in module_info_dict:
        try:
            install_path = module_info_dict[module]['installed'][0]
            project_path = module_info_dict[module]['path'][0]
            class_type   = module_info_dict[module]['class'][0]
        except IndexError:
            continue
        except KeyError:
            continue

        if(is_dylib_file(install_path)):
            continue

        if project_path is None or install_path is None or class_type is None:
            continue
        relative_out_path = out_path.split(croot + "/")[1]
        ## Ignore host and other paths
        if not relative_out_path in install_path:
            continue
        if not project_path.startswith(project_interest_prefix) or project_path.startswith("vendor/widevine"):
            continue

        project_path = find_and_update_git_project_path(project_path)
        if project_path not in project_modules_map:
            project_metadata = {}
            module_list = []
            project_metadata["module_list"] = module_list
            project_modules_map[project_path]= project_metadata
        module_metadata = {}
        module_metadata["install_path"] = install_path
        module_metadata["class_type"] = class_type
        module_metadata["module_name"] = module
        project_modules_map[project_path]["module_list"].append(module_metadata)
    for project in project_modules_map:
        commonsys_intf = check_if_project_is_commonsys_intf(project_modules_map[project])
        if commonsys_intf:
            commonsys_intf_project_list[project] = project_modules_map[project]
    return commonsys_intf_project_list

def filter_interface_projects(commonsys_intf_project_list):
    commonsys_intf_filtered_project_list = {}
    for project in commonsys_intf_project_list:
        interface_project_flag = True
        for module_metadata in commonsys_intf_project_list[project]["module_list"]:
            if not check_for_hidl_aidl_intermediate_libs(module_metadata["module_name"],module_metadata["class_type"]):
                interface_project_flag = False
                break
        if not interface_project_flag:
            commonsys_intf_filtered_project_list[project] = commonsys_intf_project_list[project]
    return commonsys_intf_filtered_project_list

def check_if_project_is_commonsys_intf(project_metadata):
    qssi_path = False
    vendor_path = False
    for module_metadata in project_metadata["module_list"]:
        module_install_path = module_metadata["install_path"]
        installed_image = module_install_path.split(out_path.split(croot+"/")[1] + "/")[1]
        for qssi_keyword in qssi_install_keywords:
            if installed_image.startswith(qssi_keyword):
                qssi_path = True

        for vendor_keyword in vendor_install_keywords:
            if installed_image.startswith(vendor_keyword):
                vendor_path = True

        if qssi_path and vendor_path:
            return True
    return False

def start_commonsys_intf_checker():
    global module_info_dict
    global violation_file_path
    global whitelist_projects_list
    global whitelist_groups_list
    global aidl_metadata_dict
    violation_flag = False
    script_dir = os.path.dirname(os.path.realpath(__file__))
    if os.path.exists(violation_file_path + "/configs"):
        violation_file_path = violation_file_path + "/configs"
    module_info_dict = load_json_file(out_path + "/module-info.json")
    whitelist_projects_list = load_json_file(script_dir + "/whitelist_commonsys_intf_project.json")
    whitelist_groups_list = load_json_file(script_dir + "/whitelist_commonsys_intf_groups.json")
    aidl_metadata_dict = load_json_file(aidl_metadata_file)
    get_commonsys_intf_project_from_manifest()
    commonsys_intf_project_list = find_commonsys_intf_project_paths_from_build_system()
    filtered_project_list = filter_interface_projects(commonsys_intf_project_list)
    success_bit = True
    if enforce_commonsys_intf_groups_checker(commonsys_intf_project_list):
        success_bit = False

    if enforce_commonsys_intf_integrity_checker(filtered_project_list):
        success_bit = False
    if success_bit:
        print("Success!")
    else :
        print("Failed!")
        sys.exit(-1)

def read_enforcement_value_from_mkfile():
    global commonsys_intf_enforcement
    script_dir = os.path.dirname(os.path.realpath(__file__))
    configs_enforcement_mk_path = os.path.join(script_dir,"configs_enforcement.mk")
    if os.path.exists(configs_enforcement_mk_path):
        configs_enforcement = open(configs_enforcement_mk_path,"r")
        mkfile_lines = configs_enforcement.readlines()
        for line in mkfile_lines:
            enforcement_flag = (line.split(":=")[0]).strip()
            enforcement_value = (line.split(":=")[1]).strip()
            if enforcement_flag == "PRODUCT_ENFORCE_COMMONSYSINTF_CHECKER":
                if enforcement_value == "true":
                    commonsys_intf_enforcement = True
                elif enforcement_value == "false":
                    commonsys_intf_enforcement = False
                else:
                    print("Unrecongnized Enforcement flag option : " + str(enforcement_value))
                    sys.exit(-1)
                break
    else:
        print("configs_enforcement.mk fime missing. Exiting!!")
        sys.exit(-1)

def main():
    if os.path.exists(noship_project_marker):
        read_enforcement_value_from_mkfile()
        start_commonsys_intf_checker()
        print("Commonsys-Intf Script Executed Successfully!!")
    else:
        print("Skipping Commonsys-Intf Checker!!")

if __name__ == '__main__':
    main()
