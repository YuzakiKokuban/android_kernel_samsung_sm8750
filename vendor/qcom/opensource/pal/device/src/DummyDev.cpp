/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: DummyDev"
#include "DummyDev.h"
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"

#include "PayloadBuilder.h"
#include "Stream.h"
#include "Session.h"

std::shared_ptr<Device> DummyDev::objRx = nullptr;
std::shared_ptr<Device> DummyDev::objTx = nullptr;

std::shared_ptr<Device> DummyDev::getObject(pal_device_id_t id)
{
    if (id == PAL_DEVICE_OUT_DUMMY)
        return objRx;
    else
        return objTx;
}


std::shared_ptr<Device> DummyDev::getInstance(struct pal_device *device,
                                              std::shared_ptr<ResourceManager> Rm)
{
    if (device->id == PAL_DEVICE_OUT_DUMMY) {
        if (!objRx) {
            PAL_INFO(LOG_TAG, "creating instance for %d", device->id);
            std::shared_ptr<Device> sp(new DummyDev(device, Rm));
            objRx = sp;
        }
        return objRx;
    } else {
        if (!objTx) {
            PAL_INFO(LOG_TAG, "creating instance for %d", device->id);
            std::shared_ptr<Device> sp(new DummyDev(device, Rm));
            objTx = sp;
        }
        return objTx;
    }
}


DummyDev::DummyDev(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{

}

DummyDev::~DummyDev()
{

}

