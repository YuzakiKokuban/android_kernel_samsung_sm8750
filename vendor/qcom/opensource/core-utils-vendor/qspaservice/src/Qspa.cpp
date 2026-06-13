/*
  * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
  * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "Qspa.h"
#include <fstream>
#include <regex>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

using namespace std;

namespace aidl {
namespace vendor {
namespace qti {
namespace hardware {
namespace qspa {

ndk::ScopedAStatus Qspa::get_all_subparts(std::vector<PartInfo>* _aidl_return) {
    ALOGI("Request received for api: get_all_subparts");
    vector<PartInfo> partinfo_return;
    for (int i = 0; i < defective_parts.size(); i++) {
        string subpart_path(PART_INFO_PATH);
        subpart_path.append(defective_parts[i]);
        fstream fin;
        fin.open(subpart_path, ios::in);
        string line;
        if (!fin) {
            ALOGE("No device path found for Subpart- %s", defective_parts[i].c_str());
            continue;
        }
        if (!getline(fin, line)) {
            ALOGE("Unable to read file: %s", subpart_path.c_str());
            return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
        }
        ALOGI("part is: %s", defective_parts[i].c_str());
        stringstream data(line);
        string intermediate;
        std::vector<int32_t> tokens;
        PartInfo partinfo;
        partinfo.part = defective_parts[i];
        // Tokenizing w.r.t ','
        while (getline(data, intermediate, ',')) {
            ALOGI("data is %s", intermediate.c_str());
            tokens.push_back(stoi(intermediate,0,16));
        }
        partinfo.subpart_data = tokens;
        partinfo_return.push_back(partinfo);
    }
    *_aidl_return = partinfo_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Qspa::get_available_parts(std::vector<std::string>* _aidl_return) {
    ALOGI("Request received for api: get_available_parts");
    vector<std::string> parts;
    for (int i = 0; i < defective_parts.size(); i++) {
        string subpart_path(PART_INFO_PATH);
        subpart_path.append(defective_parts[i]);
        fstream fin;
        fin.open(subpart_path, ios::in);
        string line;
        if (!fin) {
            ALOGE("No device path found for Subpart- %s", defective_parts[i].c_str());
            continue;
        }
        if (!getline(fin, line)) {
            ALOGE("Unable to read file: %s", subpart_path.c_str());
            return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
        }
        ALOGI("part is: %s", defective_parts[i].c_str());
        parts.push_back(defective_parts[i].c_str());
    }
    *_aidl_return = parts;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Qspa::get_subpart_info(const std::string& in_part,
        std::vector<int32_t>* _aidl_return) {
    ALOGI("Request received for api: get_subpart_info(%s)", in_part.c_str());
    string subpart_path(PART_INFO_PATH);
    subpart_path.append(in_part);
    fstream fin;
    fin.open(subpart_path, ios::in);
    string line;
    if (!fin || !getline(fin, line)) {
        ALOGE("Unable to open or read file: %s", subpart_path.c_str());
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    stringstream data(line);
    string intermediate;
    std::vector<int32_t> tokens;
    // Tokenizing w.r.t ','
    while (getline(data, intermediate, ',')) {
        ALOGI("data is %s", intermediate.c_str());
        tokens.push_back(stoi(intermediate,0,16));
    }
    *_aidl_return = tokens;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Qspa::get_num_available_cpus(int32_t* _aidl_return) {
    ALOGI("Request received for api: get_num_available_cpus");
    fstream fin;
    fin.open(CPU_INFO_PATH, ios::in);
    string line;
    if (!fin || !getline(fin, line)) {
        ALOGE("Unable to open or read file: %s", CPU_INFO_PATH);
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    ALOGI("cpus Range: %s",line.c_str());
    int32_t cpu = 0;
    cpu = stoi(std::regex_replace(line, std::regex("0-"), ""));
    ALOGI("Number of cpus: %d",cpu+1);
    *_aidl_return = cpu+1;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Qspa::get_num_available_clusters(int32_t* _aidl_return) {
    ALOGI("Request received for api: get_num_available_clusters");
    DIR *clusters_dir = opendir(CLUSTER_INFO_PATH);
    struct dirent *clusters_entry;
    if (clusters_dir == NULL) {
        ALOGE("Could not open directory: %s", CLUSTER_INFO_PATH);
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    int32_t clusters = 0;
    while ((clusters_entry = readdir(clusters_dir))) {
        string policyname = clusters_entry -> d_name;
        regex str_expr ("policy[0-9]*");
        if (std::regex_match(policyname.begin(), policyname.end(), str_expr)) {
            ALOGI("policyname is %s",policyname.c_str());
            clusters++;
        }
    }
    ALOGI("Number of clusters: %d",clusters);
    *_aidl_return = clusters;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Qspa::get_num_physical_clusters(int32_t* _aidl_return) {
    ALOGI("Request received for api: get_num_physical_clusters");
    DIR *clusters_dir = opendir(PHYCLUSTER_INFO_PATH);
    struct dirent *clusters_entry;
    if (clusters_dir == NULL) {
        ALOGE("Could not open directory: %s", PHYCLUSTER_INFO_PATH);
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    int32_t clusters = 0;
    while ((clusters_entry = readdir(clusters_dir))) {
        string dir_name = clusters_entry -> d_name;
        if (dir_name.rfind("cluster") == 0) {
            ALOGI("%s", dir_name.c_str());;
            clusters++;
        }
        cout<<endl;
    }
    ALOGI("Number of clusters: %d",clusters);
    *_aidl_return = clusters;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Qspa::get_cpus_of_physical_clusters(int32_t in_physical_cluster_number,
        std::vector<int32_t>* _aidl_return) {
    ALOGI("Request received for api: get_cpus_of_physical_clusters(%d)", in_physical_cluster_number);
    fstream fin;
    fin.open(CPU_INFO_PATH, ios::in);
    string line;
    if (!fin || !getline(fin, line)) {
        ALOGE("Unable to open or read file: %s", CPU_INFO_PATH);
        return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
    }
    ALOGI("cpus Range: %s",line.c_str());
    int cpus = 0;
    cpus = stoi(std::regex_replace(line, std::regex("0-"), "")) + 1;
    ALOGI("Number of available cpus: %d", cpus);
    int first_cpu_in_cluster = -1;
    for (int i = 0; i < cpus; i++) {
        string cpu_cluster_map_path = "/sys/devices/system/cpu/cpu" + to_string(i) + "/topology/";
        struct stat sb;
        string path1 = cpu_cluster_map_path.append("/cluster_id");
        string path2 = cpu_cluster_map_path.append("/physical_package_id");
        if (stat(path1.c_str(), &sb) == 0 && !(sb.st_mode & S_IFDIR)) {
        ALOGI("cluster_id exists for cpu: %d", i);
            fstream fin;
            fin.open(path1, ios::in);
            string cid;
            if (!fin || !getline(fin, cid)) {
                ALOGE("Unable to open or read file: %s", path1.c_str());
                return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
            }
            if (stoi(cid) == in_physical_cluster_number) {
                first_cpu_in_cluster = i;
                break;
            }
        }
        else if (stat(path2.c_str(), &sb) == 0 && !(sb.st_mode & S_IFDIR)) {
            fstream fin;
            fin.open(path2, ios::in);
            string cid;
            if (!fin || !getline(fin, cid)) {
                ALOGE("Unable to open or read file: %s", path2.c_str());
                return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
            }
            if (stoi(cid) == in_physical_cluster_number) {
                first_cpu_in_cluster = i;
                break;
            }
        }
    }
    ALOGI("First cpu in cluster is: %d", first_cpu_in_cluster);
    std::vector<int32_t> tokens;
    if (first_cpu_in_cluster != -1) {
        string cpus_path(CLUSTER_INFO_PATH);
        cpus_path.append("/policy" + to_string(first_cpu_in_cluster) + "/related_cpus");
        fstream fi;
        fi.open(cpus_path, ios::in);
        string rel_cpus;
        if (!fi || !getline(fi, rel_cpus)) {
            ALOGE("Unable to open or read file: %s", cpus_path.c_str());
            return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
        }
        stringstream data(rel_cpus);
        string intermediate;
        // Tokenizing w.r.t ' '
        while (getline(data, intermediate, ' ')) {
            ALOGI("data is %s", intermediate.c_str());
            tokens.push_back(stoi(intermediate,0,16));
        }
    }
    else {
        tokens.push_back(-1);
    }
    *_aidl_return = tokens;
    return ndk::ScopedAStatus::ok();
}

Qspa::Qspa() {
    ALOGI("Within Qspa constructor");
}

}
}
}
}
}
