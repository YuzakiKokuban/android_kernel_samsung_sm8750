/*
 *Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted provided that the following conditions are
 *met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/klog.h>
#include <batteryservice/BatteryService.h>
#include <cutils/android_reboot.h>
#include <cutils/properties.h>
#include <healthd/healthd.h>
#include "minui/minui.h"
#include "healthd_msm.h"
#define ARRAY_SIZE(x)           (sizeof(x)/sizeof(x[0]))

#define HVDCP_CHARGER           "USB_HVDCP"
#define HVDCP_BLINK_TYPE        2

#define RED_LED_PATH            "/sys/class/leds/red/brightness"
#define GREEN_LED_PATH          "/sys/class/leds/green/brightness"
#define BLUE_LED_PATH           "/sys/class/leds/blue/brightness"
#define RED_LED_BLINK_PATH      "/sys/class/leds/red/blink"
#define GREEN_LED_BLINK_PATH    "/sys/class/leds/green/blink"
#define BACKLIGHT_PATH          "/sys/class/backlight/panel0-backlight/brightness"

#define CHARGING_ENABLED_PATH   "/sys/class/power_supply/battery/charging_enabled"
#define CHARGER_TYPE_PATH       "/sys/class/power_supply/usb/type"
#define BMS_READY_PATH          "/sys/class/power_supply/bms/soc_reporting_ready"
#define BMS_BATT_INFO_PATH      "/sys/class/power_supply/bms/battery_info"
#define BMS_BATT_INFO_ID_PATH   "/sys/class/power_supply/bms/battery_info_id"
#define BMS_BATT_RES_ID_PATH    "/sys/class/power_supply/bms/resistance_id"
#define PERSIST_BATT_INFO_PATH  "/persist/bms/batt_info.txt"

#define CHGR_TAG                "charger"
#define HEALTHD_TAG             "healthd_msm"
#define LOGE(tag, x...) do { KLOG_ERROR(tag, x); } while (0)
#define LOGW(tag, x...) do { KLOG_WARNING(tag, x); } while (0)
#define LOGV(tag, x...) do { KLOG_DEBUG(tag, x); } while (0)

enum {
    RED_LED = 0x01 << 0,
    GREEN_LED = 0x01 << 1,
    BLUE_LED = 0x01 << 2,
};

enum batt_info_params {
    BATT_INFO_NOTIFY = 0,
    BATT_INFO_SOC,
    BATT_INFO_RES_ID,
    BATT_INFO_VOLTAGE,
    BATT_INFO_TEMP,
    BATT_INFO_FCC,
    BATT_INFO_MAX,
};

struct led_ctl {
    int color;
    const char *path;
};

struct led_ctl leds[3] =
    {{RED_LED, RED_LED_PATH},
    {GREEN_LED, GREEN_LED_PATH},
    {BLUE_LED, BLUE_LED_PATH}};

#define HVDCP_COLOR_MAP         (RED_LED | GREEN_LED)

struct soc_led_color_mapping {
    int soc;
    int color;
};

struct soc_led_color_mapping soc_leds[3] = {
    {15, RED_LED},
    {90, RED_LED | GREEN_LED},
    {100, GREEN_LED},
};

static int batt_info_cached[BATT_INFO_MAX];
static bool healthd_msm_err_log_once;
static int8_t healthd_msm_log_en;
static int8_t healthd_msm_store_params;

static int write_file_int(char const* path, int value)
{
    int fd;
    char buffer[20];
    int rc = -1, bytes;

    fd = open(path, O_WRONLY);
    if (fd >= 0) {
        bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
        rc = write(fd, buffer, bytes);
        close(fd);
    }

    return rc > 0 ? 0 : -1;
}

static int set_tricolor_led(int on, int color)
{
    int rc, i;
    char buffer[10];

    for (i = 0; i < (int)ARRAY_SIZE(leds); i++) {
        if ((color & leds[i].color) && (access(leds[i].path, R_OK | W_OK) == 0)) {
            rc = write_file_int(leds[i].path, on ? 255 : 0);
            if (rc < 0)
                return rc;
        }
    }

    return 0;
}

static bool is_hvdcp_inserted()
{
    bool hvdcp = false;
    char buff[12] = "\0";
    int fd, cnt;

    fd = open(CHARGER_TYPE_PATH, O_RDONLY);
    if (fd >= 0) {
        cnt = read(fd, buff, (sizeof(buff) - 1));
        if (cnt > 0 && !strncmp(buff, HVDCP_CHARGER, 9))
            hvdcp = true;
        close(fd);
    }

    return hvdcp;
}

static int get_blink_led_for_hvdcp(void)
{
    int ret, rc = 0, bytes;
    int red_blink_fd = -1, green_blink_fd = -1, type_fd = -1;
    char buf[20];

    type_fd = open(CHARGER_TYPE_PATH, O_RDONLY);

    if (type_fd < 0) {
        LOGE(CHGR_TAG, "Could not open USB type node\n");
        return rc;
    } else {
        close(type_fd);
        ret = write_file_int(GREEN_LED_BLINK_PATH, 0);
        if (ret < 0) {
            LOGE(CHGR_TAG, "Fail to write: %s\n", GREEN_LED_BLINK_PATH);
        } else {
            rc |= GREEN_LED;
	}

        ret = write_file_int(RED_LED_BLINK_PATH, 0);
        if (ret < 0) {
            LOGE(CHGR_TAG, "Fail to write: %s\n", RED_LED_BLINK_PATH);
        } else {
            rc |= RED_LED;
	}
    }

    return rc;
}

#if QTI_BSP
#define STR_LEN 8
void healthd_board_mode_charger_draw_battery(
                struct android::BatteryProperties *batt_prop)
{
    char cap_str[STR_LEN];
    int x, y;
    int str_len_px;
    static int char_height = -1, char_width = -1;

    if (char_height == -1 && char_width == -1)
        gr_font_size(&char_width, &char_height);
    snprintf(cap_str, (STR_LEN - 1), "%d%%", batt_prop->batteryLevel);
    str_len_px = gr_measure(cap_str);
    x = (gr_fb_width() - str_len_px) / 2;
    y = (gr_fb_height() + char_height) / 2;
    gr_color(0xa4, 0xc6, 0x39, 255);
    gr_text(x, y, cap_str, 0);
}
#endif

void healthd_board_mode_charger_battery_update(
                struct android::BatteryProperties *batt_prop)
{
    static int blink_for_hvdcp = -1;
    static int old_color = 0;
    int i, color, soc, rc;
    bool blink = false;

    if (blink_for_hvdcp == -1)
        blink_for_hvdcp = get_blink_led_for_hvdcp();

    if ((blink_for_hvdcp > 0) && is_hvdcp_inserted())
        blink = true;

    soc = batt_prop->batteryLevel;

    for (i = 0; i < ((int)ARRAY_SIZE(soc_leds) - 1); i++) {
        if (soc < soc_leds[i].soc)
            break;
    }
    color = soc_leds[i].color;

    if (old_color != color) {
        if ((color == HVDCP_COLOR_MAP) && blink) {
            if (blink_for_hvdcp & RED_LED) {
                rc = write_file_int(RED_LED_BLINK_PATH, HVDCP_BLINK_TYPE);
                if (rc < 0) {
                    LOGE(CHGR_TAG, "Fail to write: %s\n", RED_LED_BLINK_PATH);
                    return;
                }
            }
            if (blink_for_hvdcp & GREEN_LED) {
                rc = write_file_int(GREEN_LED_BLINK_PATH, HVDCP_BLINK_TYPE);
                if (rc < 0) {
                    LOGE(CHGR_TAG, "Fail to write: %s\n", GREEN_LED_BLINK_PATH);
                    return;
                }
            }
        } else {
                rc = set_tricolor_led(0, old_color);
                if (rc < 0)
                    LOGE(CHGR_TAG, "Error in setting old_color on tricolor_led\n");

                rc = set_tricolor_led(1, color);
                if (rc < 0)
                    LOGE(CHGR_TAG, "Error in setting color on tricolor_led\n");

                if (!rc) {
                    old_color = color;
                    LOGV(CHGR_TAG, "soc = %d, set led color 0x%x\n", soc, soc_leds[i].color);
                }
        }
    }
}

#define BACKLIGHT_ON_LEVEL    100
#define BACKLIGHT_OFF_LEVEL    0
void healthd_board_mode_charger_set_backlight(bool en)
{
    int rc;

    if (access(BACKLIGHT_PATH, R_OK | W_OK) != 0)
    {
        LOGW(CHGR_TAG, "Backlight control not support\n");
        return;
    }

    rc = write_file_int(BACKLIGHT_PATH, en ? BACKLIGHT_ON_LEVEL :
                BACKLIGHT_OFF_LEVEL);
    if (rc < 0) {
        LOGE(CHGR_TAG, "Could not write to backlight node : %s\n", strerror(errno));
        return;
    }

    LOGV(CHGR_TAG, "set backlight status to %d\n", en);
}

static inline void get_healthd_props()
{
    healthd_msm_log_en = property_get_bool("persist.healthd_msm.log_en", 1);
    healthd_msm_store_params =
                property_get_bool("persist.healthd_msm.store_params", 0);
}

#define WAIT_BMS_READY_TIMES_MAX	200
#define WAIT_BMS_READY_INTERVAL_USEC	200000
void healthd_board_mode_charger_init()
{
    int ret;
    char buff[8] = "\0";
    int charging_enabled = 0;
    int bms_ready = 0;
    int wait_count = 0;
    int fd;

    /* check the charging is enabled or not */
    fd = open(CHARGING_ENABLED_PATH, O_RDONLY);
    if (fd < 0)
        return;
    ret = read(fd, buff, (sizeof(buff) - 1));
    close(fd);
    if (ret > 0) {
        buff[ret] = '\0';
        sscanf(buff, "%d\n", &charging_enabled);
        LOGW(CHGR_TAG, "android charging is %s\n",
                !!charging_enabled ? "enabled" : "disabled");
        /* if charging is disabled, reboot and exit power off charging */
        if (!charging_enabled)
            android_reboot(ANDROID_RB_RESTART, 0, 0);
    }
    fd = open(BMS_READY_PATH, O_RDONLY);
    if (fd < 0)
            return;
    while (1) {
        ret = read(fd, buff, (sizeof(buff) - 1));
        if (ret > 0) {
            buff[ret] = '\0';
            sscanf(buff, "%d\n", &bms_ready);
        } else {
            LOGE(CHGR_TAG, "read soc-ready failed, ret=%d\n", ret);
            break;
        }

        if ((bms_ready > 0) || (wait_count++ > WAIT_BMS_READY_TIMES_MAX))
            break;
        usleep(WAIT_BMS_READY_INTERVAL_USEC);
        lseek(fd, 0, SEEK_SET);
    }
    close(fd);
    LOGV(CHGR_TAG, "Checking BMS SoC ready done %d!\n", bms_ready);
}

static void healthd_batt_info_notify()
{
    int rc, fd, id = 0;
    int bms_ready = 0;
    int wait_count = 0;
    char buff[100] = "";
    int batt_info[BATT_INFO_MAX];
    char *ptr, *tmp, *temp_str;
    char path_str[50] = "";
    bool notify_bms = false;

    if (!healthd_msm_store_params) {
        return;
    }

    fd = open(PERSIST_BATT_INFO_PATH, O_RDONLY);
    if (fd < 0) {
        LOGW(HEALTHD_TAG, "Error in opening batt_info.txt, fd=%d\n", fd);
        fd = creat(PERSIST_BATT_INFO_PATH, S_IRWXU);
        if (fd < 0) {
            LOGE(HEALTHD_TAG, "Couldn't create file, fd=%d errno=%s\n", fd,
                 strerror(errno));
            goto out;
        }
        LOGV(HEALTHD_TAG, "Created file %s\n", PERSIST_BATT_INFO_PATH);
        close(fd);
        goto out;
    } else {
        LOGV(HEALTHD_TAG, "opened %s\n", PERSIST_BATT_INFO_PATH);
    }

    rc = read(fd, buff, (sizeof(buff) - 1));
    if (rc < 0) {
        LOGE(HEALTHD_TAG, "Error in reading fd %d, rc=%d\n", fd, rc);
        close(fd);
        goto out;
    }
    close(fd);
    buff[rc] = '\0';
    temp_str = strtok_r(buff, ":", &ptr);
    id = 1;
    while (temp_str != NULL && id < BATT_INFO_MAX) {
        batt_info[id++] = (int)strtol(temp_str, &tmp, 10);
        temp_str = strtok_r(NULL, ":", &ptr);
    }

    if (id < BATT_INFO_MAX) {
        LOGE(HEALTHD_TAG, "Read %d batt_info parameters\n", id);
        goto out;
    }

    /* Send batt_info parameters to FG driver */
    for (id = 1; id < BATT_INFO_MAX; id++) {
        snprintf(path_str, sizeof(path_str), "%s", BMS_BATT_INFO_ID_PATH);
        rc = write_file_int(path_str, id);
        if (rc < 0) {
            LOGE(HEALTHD_TAG, "Error in writing batt_info_id %d, rc=%d\n", id,
                rc);
            goto out;
        }

        snprintf(path_str, sizeof(path_str), "%s", BMS_BATT_INFO_PATH);
        rc = write_file_int(path_str, batt_info[id]);
        if (rc < 0) {
            LOGE(HEALTHD_TAG, "Error in writing batt_info %d, rc=%d\n",
                batt_info[id], rc);
            goto out;
        }
    }

    notify_bms = true;

out:
    fd = open(BMS_READY_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE(HEALTHD_TAG, "Couldn't open %s\n", BMS_READY_PATH);
        return;
    }

    /* Wait for soc_reporting_ready */
    wait_count = 0;
    memset(buff, 0, sizeof(buff));
    while (1) {
        rc = read(fd, buff, 1);
        if (rc > 0) {
            sscanf(buff, "%d\n", &bms_ready);
        } else {
            LOGE(HEALTHD_TAG, "read soc-ready failed, rc=%d\n", rc);
            break;
        }

        if ((bms_ready > 0) || (wait_count++ > WAIT_BMS_READY_TIMES_MAX))
            break;

        usleep(WAIT_BMS_READY_INTERVAL_USEC);
        lseek(fd, 0, SEEK_SET);
    }
    close(fd);

    if (!bms_ready)
        notify_bms = false;

    if (!notify_bms) {
        LOGE(HEALTHD_TAG, "Not notifying BMS\n");
        return;
    }

    /* Notify FG driver */
    snprintf(path_str, sizeof(path_str), "%s", BMS_BATT_INFO_ID_PATH);
    rc = write_file_int(path_str, BATT_INFO_NOTIFY);
    if (rc < 0) {
        LOGE(HEALTHD_TAG, "Error in writing batt_info_id, rc=%d\n", rc);
        return;
    }

    snprintf(path_str, sizeof(path_str), "%s", BMS_BATT_INFO_PATH);
    rc = write_file_int(path_str, INT_MAX - 1);
    if (rc < 0)
        LOGE(HEALTHD_TAG, "Error in writing batt_info, rc=%d\n", rc);
}

void healthd_board_init(struct healthd_config*)
{
    // use defaults
    get_healthd_props();
    power_off_alarm_init();
    healthd_batt_info_notify();
}

static void healthd_store_batt_props(const struct android::BatteryProperties* props)
{
    char buff[100];
    int fd, rc, len, batteryId = 0;

    if (!healthd_msm_store_params) {
        return;
    }

    if (!props->batteryPresent) {
        return;
    }

    if (props->batteryLevel == 0 || props->batteryVoltage == 0) {
        return;
    }

    memset(buff, 0, sizeof(buff));
    fd = open(BMS_BATT_RES_ID_PATH, O_RDONLY);
    if (fd < 0) {
        if (!healthd_msm_err_log_once) {
            LOGE(HEALTHD_TAG, "Couldn't open %s\n", BMS_BATT_RES_ID_PATH);
            healthd_msm_err_log_once = true;
        }
    } else {
        rc = read(fd, buff, 6);
        if (rc > 0) {
            sscanf(buff, "%d\n", &batteryId);
            batteryId /= 1000;
        } else if (!healthd_msm_err_log_once) {
            LOGE(HEALTHD_TAG, "reading batt_res_id failed, rc=%d\n", rc);
            healthd_msm_err_log_once = true;
        }
    }

    if (fd >= 0)
        close(fd);

    if (props->batteryLevel == batt_info_cached[BATT_INFO_SOC] &&
        props->batteryVoltage == batt_info_cached[BATT_INFO_VOLTAGE] &&
        props->batteryTemperature == batt_info_cached[BATT_INFO_TEMP] &&
        props->batteryFullCharge == batt_info_cached[BATT_INFO_FCC] &&
        batteryId == batt_info_cached[BATT_INFO_RES_ID])
        return;

    fd = open(PERSIST_BATT_INFO_PATH, O_RDWR | O_TRUNC);
    if (fd < 0) {
        /*
         * Print the error just only once as this function can be called as
         * long as the system is running and logs should not flood the console.
         */
        if (!healthd_msm_err_log_once) {
            LOGE(HEALTHD_TAG, "Error in opening batt_info.txt, fd=%d\n", fd);
            healthd_msm_err_log_once = true;
        }
        return;
    }

    len = snprintf(buff, sizeof(buff), "%d:%d:%d:%d:%d", props->batteryLevel,
                   batteryId, props->batteryVoltage,
                   props->batteryTemperature, props->batteryFullCharge);
    if (len < 0) {
        if (!healthd_msm_err_log_once) {
            LOGE(HEALTHD_TAG, "Error in printing to buff, len=%d\n", len);
            healthd_msm_err_log_once = true;
        }
        goto out;
    }

    buff[len] = '\0';
    rc = write(fd, buff, sizeof(buff));
    if (rc < 0) {
        if (!healthd_msm_err_log_once) {
            LOGE(HEALTHD_TAG, "Error in writing to batt_info.txt, rc=%d\n", rc);
            healthd_msm_err_log_once = true;
        }
        goto out;
    }

    batt_info_cached[BATT_INFO_SOC] = props->batteryLevel;
    batt_info_cached[BATT_INFO_RES_ID] = batteryId;
    batt_info_cached[BATT_INFO_VOLTAGE] = props->batteryVoltage;
    batt_info_cached[BATT_INFO_TEMP] = props->batteryTemperature;
    batt_info_cached[BATT_INFO_FCC] = props->batteryFullCharge;

out:
    if (fd >= 0)
        close(fd);
}

int healthd_board_battery_update(struct android::BatteryProperties* props)
{
    // return 0 to log periodic polled battery status to kernel log
    healthd_store_batt_props(props);
    if (healthd_msm_log_en)
        return 0;
    return 1;
}
