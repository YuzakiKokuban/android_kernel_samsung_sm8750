
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <sstream>
#include <string>
#include <vector>

/** Default path of service interface **/
constexpr const char* DEFAULT_NAME = "vendor_audio_interfaces.xml";

struct Interface {
    std::string name;        // interface name
    std::string libraryName; // name of library
    std::string method;      // function pointer used to register to ServiceManager
    bool mandatory;          // Interface is mandatory or not

    std::string toString() const {
        std::ostringstream os;
        os << "Interface: [";
        os << "name: " << name << " ";
        os << "path: " << libraryName << " ";
        os << "mandatory: " << mandatory;
        os << "]";
        return os.str();
    }
};

using Interfaces = std::vector<Interface>;

Interfaces parseInterfaces();