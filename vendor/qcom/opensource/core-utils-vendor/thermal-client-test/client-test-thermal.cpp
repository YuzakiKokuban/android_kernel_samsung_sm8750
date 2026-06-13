/*==============================================================================
*  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*  SPDX-License-Identifier: BSD-3-Clause-Clear
*===============================================================================
*/
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <aidl/android/hardware/thermal/BnThermal.h>
#include <cstring>

using namespace std;
using aidl::android::hardware::thermal::Temperature;
using aidl::android::hardware::thermal::IThermal;
using aidl::android::hardware::thermal::TemperatureType;

int main() {
    const string instance = string() + IThermal::descriptor + "/default";
    ndk::SpAIBinder binder(AServiceManager_waitForService(instance.c_str()));
    auto thermalBinder = IThermal::fromBinder(binder);
    if (thermalBinder == nullptr) {
        cout << "Cannot acccess thermal-hal!" << endl;
        return 0;
    }
    vector<Temperature>temperature;
    thermalBinder->getTemperatures(&temperature);
    for(int i=0;i<temperature.size();i++){
	if(temperature[i].type==(TemperatureType::SKIN)){
	    cout<<"Temperature Value: "<<temperature[i].value<<endl;
	}
    }
    return 0;
}
