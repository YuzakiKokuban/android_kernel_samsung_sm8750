/*
  * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
  * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <aidl/vendor/qti/hardware/qspa/BnQspa.h>
#include <vector>
#include <log/log.h>
#include <iostream>
#include <android-base/properties.h>
#include<map>

#define CPU_INFO_PATH "/sys/devices/system/cpu/possible"
#define CLUSTER_INFO_PATH "/sys/devices/system/cpu/cpufreq"
#define PHYCLUSTER_INFO_PATH "/proc/device-tree/cpus/cpu-map"
#define PART_INFO_PATH "/sys/devices/soc0/"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "qspa-service"

using namespace std;
using aidl::vendor::qti::hardware::qspa::PartInfo;
namespace aidl {
namespace vendor {
namespace qti {
namespace hardware {
namespace qspa {

class Qspa : public BnQspa {
    public:
        Qspa();
        ndk::ScopedAStatus get_all_subparts(std::vector<PartInfo>* _aidl_return) override;
        ndk::ScopedAStatus get_available_parts(std::vector<std::string>* _aidl_return) override;
        ndk::ScopedAStatus get_subpart_info(const std::string& in_part,
                std::vector<int32_t>* _aidl_return) override;
        ndk::ScopedAStatus get_num_available_cpus(int32_t* _aidl_return) override;
        ndk::ScopedAStatus get_num_available_clusters(int32_t* _aidl_return) override;
        ndk::ScopedAStatus get_num_physical_clusters(int32_t* _aidl_return) override;
        ndk::ScopedAStatus get_cpus_of_physical_clusters(int32_t in_physical_cluster_number,
                std::vector<int32_t>* _aidl_return) override;
    private:
        std::vector<std::string> defective_parts = {"gpu", "video", "camera", "display", "audio",
                "modem", "wlan", "comp", "sensors", "npu", "spss", "nav", "comp1", "display1",
                "nsp", "eva"};

};

}
}
}
}
}

