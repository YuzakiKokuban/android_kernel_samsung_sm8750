/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "PAL: UltrasoundDevice"
#include "UltrasoundDevice.h"
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"

std::shared_ptr<Device> UltrasoundDevice::objRx = nullptr;
std::shared_ptr<Device> UltrasoundDevice::objTx = nullptr;

std::shared_ptr<Device> UltrasoundDevice::getObject(pal_device_id_t id)
{
    if (id == PAL_DEVICE_OUT_ULTRASOUND ||
        id == PAL_DEVICE_OUT_ULTRASOUND_DEDICATED)
        return objRx;
    else
        return objTx;
}

std::shared_ptr<Device> UltrasoundDevice::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    if (device->id == PAL_DEVICE_OUT_ULTRASOUND ||
        device->id == PAL_DEVICE_OUT_ULTRASOUND_DEDICATED) {
        if (!objRx) {
            PAL_INFO(LOG_TAG, "%s creating instance for  %d\n", __func__, device->id);
            std::shared_ptr<Device> sp(new UltrasoundDevice(device, Rm));
            objRx = sp;
        }
        return objRx;
    } else {
        if (!objTx) {
            PAL_INFO(LOG_TAG, "%s creating instance for  %d\n", __func__, device->id);
            std::shared_ptr<Device> sp(new UltrasoundDevice(device, Rm));
            objTx = sp;
        }
        return objTx;
    }
}


UltrasoundDevice::UltrasoundDevice(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{

}

UltrasoundDevice::~UltrasoundDevice()
{

}

