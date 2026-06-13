/*
 * Copyright (C) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc.nor the
 *      names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT
 * RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
 * PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "vendor.qti.lights"

#include "Lights.h"
#include <log/log.h>
#include <android-base/logging.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace {

enum rgb_led {
    LED_RED,
    LED_GREEN,
    LED_BLUE,
};

static const char *rgb_led_name[] = {
    [LED_RED] = "red",
    [LED_GREEN] = "green",
    [LED_BLUE] = "blue",
};

static int write_str_to_file(const char *path, char const *str) {
    int fd;

    fd = open(path, O_WRONLY);
    if (fd >= 0) {
        ssize_t amt = write(fd, str, strlen(str));
        close(fd);

        if (amt == -1) {
            ALOGE("Write to %s failed: %d", path, -errno);
            return -errno;
        }

        return 0;
    }

    ALOGE("Could not open %s: %d", path, -errno);
    return -errno;
}

static int write_int_to_file(const char *path, int value) {
    int fd;

    fd = open(path, O_WRONLY);
    if (fd >= 0) {
        char buf[16];
        size_t bytes = snprintf(buf, sizeof(buf), "%d\n", value);
        ssize_t amt = write(fd, buf, bytes);
        close(fd);

        if (amt == -1) {
            ALOGE("Write to %s failed: %d", path, -errno);
            return -errno;
        }

        return 0;
    }

    ALOGE("Could not open %s: %d", path, -errno);
    return -errno;
}

static int setLedBreathParam(enum rgb_led led, int breath) {
    char file[48];
    int rc;

    snprintf(file, sizeof(file),"/sys/class/leds/%s/breath", rgb_led_name[led]);
    rc = write_int_to_file(file, breath);
    if (rc < 0)
        ALOGE("%s does not support breath", rgb_led_name[led]);

    return rc;
}

static int setLedPattern(enum rgb_led led) {
    char file[48];
    char const pattern[264] = "0 100 0 0 26 100 26 0 51 100 51 0 77 100 77 0 102 100 102 0 128 100 128 0 153 100 153 0 179 100 179 0 204 100 204 0 230 100 230 0 255 100 255 0 230 100 230 0 204 100 204 0 179 100 179 0 153 100 153 0 128 100 128 0 102 100 102 0 77 100 77 0 51 100 51 0 26 100 26 0";
    int rc, retries = 20;

    snprintf(file, sizeof(file), "/sys/class/leds/%s/trigger", rgb_led_name[led]);
    rc = write_str_to_file(file, "pattern");
    if (rc < 0) {
        ALOGD("%s LED does not support pattern trigger\n", rgb_led_name[led]);
        return rc;
    }

    while(retries--) {
        ALOGD("retry %d set repeat and hw_pattern\n", retries);
        usleep(2000);

        snprintf(file, sizeof(file), "/sys/class/leds/%s/repeat", rgb_led_name[led]);
        rc = write_int_to_file(file, -1);
        if (rc < 0)
            continue;

        snprintf(file, sizeof(file), "/sys/class/leds/%s/hw_pattern", rgb_led_name[led]);
        rc = write_str_to_file(file, pattern);
        if (!rc)
            break;
    }

    if (rc < 0) {
        ALOGE("Error writing to repeat/hw_pattern for %s LED\n", rgb_led_name[led]);
        return rc;
    }

    return 0;
}

static int setLedDelayParams(enum rgb_led led, int flashOnMs, int flashOffMs) {
    char file_on[48];
    char file_off[48];
    int rc;
    int retries = 20;

    snprintf(file_on, sizeof(file_on), "/sys/class/leds/%s/trigger", rgb_led_name[led]);
    rc = write_str_to_file(file_on, "timer");
    if (rc < 0) {
        ALOGD("%s doesn't support timer trigger\n", rgb_led_name[led]);
        return rc;
    }

    snprintf(file_off, sizeof(file_off), "/sys/class/leds/%s/delay_off", rgb_led_name[led]);
    snprintf(file_on, sizeof(file_on), "/sys/class/leds/%s/delay_on", rgb_led_name[led]);

    while(retries--) {
        ALOGD("retry %d set delay_off and delay_on\n", retries);
        usleep(2000);

        rc = write_int_to_file(file_off, flashOffMs);
        if (rc < 0)
            continue;

        rc = write_int_to_file(file_on, flashOnMs);
        if (!rc)
            break;
    }

    if (rc < 0) {
        ALOGE("Error in writing to delay_on/off for %s\n", rgb_led_name[led]);
        return rc;
    }

    return 0;
}

static int setLedBrightness(enum rgb_led led, int brightness) {
    int rc;
    char file[48];

    snprintf(file, sizeof(file), "/sys/class/leds/%s/trigger", rgb_led_name[led]);
    rc = write_str_to_file(file, "none");
    if (rc < 0) {
        ALOGD("%s failed to set trigger to none\n", rgb_led_name[led]);
        return rc;
    }

    snprintf(file, sizeof(file), "/sys/class/leds/%s/brightness", rgb_led_name[led]);
    rc = write_int_to_file(file, brightness);
    if (rc < 0)
        return rc;

    return rc;
}

static bool isPatternTrigger(const char *path) {
    char buf[300] = {};
    int fd, ret;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ALOGE("Couldn't open %s", path);
        return false;
    }

    ret = TEMP_FAILURE_RETRY(read(fd, buf, sizeof(buf)));
    close(fd);

    if (ret < 0) {
        ALOGE("Couldn't read %s errno=%d", path, errno);
        return false;
    }

    return strstr(buf, "pattern") ? true : false;
}
} // namespace anonymous

namespace aidl {
namespace android {
namespace hardware {
namespace light {

const static std::vector<LightType> kLogicalLights = {
    LightType::BACKLIGHT,
    LightType::KEYBOARD,
    LightType::BUTTONS,
    LightType::BATTERY,
    LightType::NOTIFICATIONS,
    LightType::ATTENTION,
    LightType::BLUETOOTH,
    LightType::WIFI,
};

Lights::Lights() {
    for (auto i = kLogicalLights.begin(); i != kLogicalLights.end(); i++) {
        HwLight hwLight{};
        hwLight.id = (int)(*i);
        hwLight.type = *i;
        hwLight.ordinal = 0;
        mAvailableLights.emplace_back(hwLight);
    }
}

int Lights::setRgbLedsParams(const HwLightState& state) {
    int rc = 0;
    int const colorRGB = state.color;
    int const red = (colorRGB >> 16) & 0xFF;
    int const green = (colorRGB >> 8) & 0xFF;
    int const blue = colorRGB & 0xFF;
    int const breath = (state.flashOnMs != 0 && state.flashOffMs != 0);

    switch (state.flashMode) {
    case FlashMode::HARDWARE:
        if (mPpgDetected == PPG_BREATH) {
            if (!!red)
                rc = setLedBreathParam(LED_RED, breath);
            if (!!green)
                rc |= setLedBreathParam(LED_GREEN, breath);
            if (!!blue)
                rc |= setLedBreathParam(LED_BLUE, breath);
            /* Fallback to blinking if breath is not supported */
            if (rc == 0)
                break;
        } else if (mPpgDetected == PPG_PATTERN) {
            if (!!red)
                rc = setLedPattern(LED_RED);
            if (!!green)
                rc |= setLedPattern(LED_GREEN);
            if (!!blue)
                rc |= setLedPattern(LED_BLUE);
            /* Fallback to blinking if pattern trigger is not supported */
            if (rc == 0)
                break;
        }
        FALLTHROUGH_INTENDED;
    case FlashMode::TIMED:
        if (!!red)
            rc = setLedDelayParams(LED_RED, state.flashOnMs, state.flashOffMs);
        if (!!green)
            rc |= setLedDelayParams(LED_GREEN, state.flashOnMs, state.flashOffMs);
        if (!!blue)
            rc |= setLedDelayParams(LED_BLUE, state.flashOnMs, state.flashOffMs);
        /* Fallback to constant on if blinking is not supported */
        if (rc == 0)
            break;
    case FlashMode::NONE:
        FALLTHROUGH_INTENDED;
    default:
        rc = setLedBrightness(LED_RED, red);
        rc |= setLedBrightness(LED_GREEN, green);
        rc |= setLedBrightness(LED_BLUE, blue);
        break;
    }

    ALOGD("colorRGB = %08X, mode = %d, onMs = %d, offMs = %d, rc: %d", colorRGB, state.flashMode, state.flashOnMs, state.flashOffMs, rc);

    return rc;
}

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState& state) {
    /* For QMAA compliance, return OK even if leds device doesn't exist */
    if (!mLedDetected) {
        ALOGE("Tri Leds device doesn't exist");
        return ndk::ScopedAStatus::ok();
    }

    if (id < 0 || id >= mAvailableLights.size()) {
        ALOGE("Invalid Light id : %d", id);
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    HwLight const& light = mAvailableLights[id];

    // Don't set LEDs for other notifications if battery notification is set
    if (light.type != LightType::BATTERY && mBatteryNotification)
        return ndk::ScopedAStatus::ok();

    int ret = setRgbLedsParams(state);
    if (ret != 0)
            return ndk::ScopedAStatus::fromServiceSpecificError(ret);

    if (light.type == LightType::BATTERY)
        mBatteryNotification = (state.color & 0x00FFFFFF);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    char file[48];
    int fd;

    for (auto i = mAvailableLights.begin(); i != mAvailableLights.end(); i++) {
        lights->push_back(*i);
    }

    mLedDetected = false;
    mPpgDetected = PPG_NONE;
    snprintf(file, sizeof(file), "/sys/class/leds/%s/brightness", rgb_led_name[LED_RED]);
    fd = open(file, O_RDONLY);
    if (fd >= 0) {
       mLedDetected = true;
       close(fd);
    } else {
       ALOGE("Couldn't open %s", file);
    }

    /* Check which PPG mode is available */
    snprintf(file, sizeof(file), "/sys/class/leds/%s/breath", rgb_led_name[LED_RED]);
    fd = open(file, O_RDONLY);
    if (fd >= 0) {
       mPpgDetected = PPG_BREATH;
       close(fd);
    } else {
        snprintf(file, sizeof(file), "/sys/class/leds/%s/trigger", rgb_led_name[LED_RED]);
        if (isPatternTrigger(file))
            mPpgDetected = PPG_PATTERN;
    }

    return ndk::ScopedAStatus::ok();
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
