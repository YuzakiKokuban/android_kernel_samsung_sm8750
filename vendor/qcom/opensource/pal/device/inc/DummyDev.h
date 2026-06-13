/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DUMMY_DEV_H
#define DUMMY_DEV_H

#include "Device.h"

class DummyDev : public Device
{
protected:
    static std::shared_ptr<Device> objRx;
    static std::shared_ptr<Device> objTx;
    DummyDev(struct pal_device *device, std::shared_ptr<ResourceManager> Rm);
public:
    static std::shared_ptr<Device> getInstance(struct pal_device *device,
                                               std::shared_ptr<ResourceManager> Rm);
    static std::shared_ptr<Device> getObject(pal_device_id_t id);
    virtual ~DummyDev();
};
#endif //DUMMY_DEV_H
