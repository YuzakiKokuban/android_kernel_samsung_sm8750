/*
 * Copyright (c) 2010, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.

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


#ifndef _QSAP_API_H_
#define _QSAP_API_H_

#if __cplusplus
extern "C" {
#endif
#include <android/log.h>
typedef unsigned char      u8;
typedef char               s8;
typedef unsigned short int u16;
typedef signed short int   s16;
typedef unsigned int       u32;
typedef signed int         s32;

/** Success and error messages */
#define SUCCESS                     "success"
#define ERR_INVALID_ARG             "failure invalid arguments"
#define ERR_INVALID_PARAM           "failure invalid parameter"
#define ERR_UNKNOWN                 "failure unknown error"
#define ERR_INVALIDCMD              "failure invalid command"
#define ERR_INVALIDREQ              "failure invalid request"
#define ERR_FEATURE_NOT_ENABLED     "failure feature not enabled"
#define ERR_NOT_SUPPORTED           "failure not supported"
#define ERR_NOT_READY               "failure not ready"
#define ERR_RES_UNAVAILABLE         "failure resource unavailable"
#define ERR_SOFTAP_NOT_STARTED      "failure softap not started"

/** Error numbers used with the SDK */
enum error_val {
    eERR_UNKNOWN = -1,
    eSUCCESS = 0,
    eERR_STOP_BSS,
    eERR_BSS_NOT_STARTED,
    eERR_COMMIT,
    eERR_START_SAP,
    eERR_STOP_SAP,
    eERR_RELOAD_SAP,
    eERR_FILE_OPEN,
    eERR_CONF_FILE,
    eERR_INVALID_MAC_ADDR,
    eERR_SEND_TO_HOSTAPD,
    eERR_CONFIG_PARAM_MISSING,
    eERR_CHAN_READ,
    eERR_FEATURE_NOT_ENABLED,
    eERR_UNLOAD_FAILED_SDIO,
    eERR_UNLOAD_FAILED_SOFTAP,
    eERR_LOAD_FAILED_SDIOIF,
    eERR_LOAD_FAILED_SOFTAP,
    eERR_SET_CHAN_RANGE,
    eERR_GET_AUTO_CHAN
};

#ifndef WIFI_DRIVER_CONF_FILE
#define WIFI_DRIVER_CONF_FILE            NULL
#endif

#ifndef WIFI_DRIVER_DEF_CONF_FILE
#define WIFI_DRIVER_DEF_CONF_FILE        NULL
#endif

/** Configuration file name for SAP+SAP*/
#define CONFIG_FILE_2G "/data/vendor/wifi/hostapd/hostapd_dual2g.conf"
#define CONFIG_FILE_5G "/data/vendor/wifi/hostapd/hostapd_dual5g.conf"
#define CONFIG_FILE_60G "/data/vendor/wifi/hostapd/hostapd_60g.conf"

/** Configuration file name for OWE-transition */
#define CONFIG_FILE_OWE "/data/vendor/wifi/hostapd/hostapd_owe.conf"

/** Configuration file name */
#define CONFIG_FILE "/data/vendor/wifi/hostapd/hostapd.conf"

/** Default configuration file path */
#define DEFAULT_CONFIG_FILE_PATH "/vendor/etc/hostapd/hostapd_default.conf"

/** Default Accept list file name */
#define DEFAULT_ACCEPT_LIST_FILE_PATH "/vendor/etc/hostapd/hostapd.accept"

/** Accept list file name */
#define ACCEPT_LIST_FILE "/data/vendor/wifi/hostapd/hostapd.accept"

/** Default Deny list file name */
#define DEFAULT_DENY_LIST_FILE_PATH "/vendor/etc/hostapd/hostapd.deny"

/** Deny list file name */
#define DENY_LIST_FILE "/data/vendor/wifi/hostapd/hostapd.deny"

/** Default Ini file */
#define DEFAULT_INI_FILE "/persist/qcom/softap/qcom_cfg_default.ini"

/** SDK control interface path */
#define SDK_CTRL_IF "/data/vendor/wifi/hostapd/ctrl/softap_sdk_ctrl"

/** Maximum length of the line in the configuration file */
#define MAX_CONF_LINE_LEN  (156)

/** MAC address length in acsii string format*/
#define MAC_ADDR_LEN       (17)

/** MAC address length, as integer */
#define MAC_ADDR_LEN_INT   (6)
/** Maximum number of MAC address in the allow / deny MAC list */
#define MAX_ALLOWED_MAC    (15)

/** Maximum length of the file path */
#define MAX_FILE_PATH_LEN  (128)

/** WPS key length - 8 digit key, usually*/
#define WPS_KEY_LEN   (8)

/** Maximum length of the SSID */
#define SSD_MAX_LEN (32)
#define CTRY_MAX_LEN (3)

/** Beacon interval 50 to 65535 */
#define BCN_INTERVAL_MIN  (1)
#define BCN_INTERVAL_MAX  (65535)

/** Passphrase max length 63 bytes, Minumum lenght is 8.
  * NOTE: If Passphrase length is 64, then phassphrase is treated as PSK.
  */
#define PASSPHRASE_MIN    (8)
#define PASSPHRASE_MAX    (63)

/** DTIM period 1 to 255 -- Qualcomm 10 */
#define DTIM_PERIOD_MIN    (1)
#define DTIM_PERIOD_MAX    (255)

/** WEP key lengths in ASCII and hex */
#define WEP_64_KEY_ASCII  (5)
#define WEP_64_KEY_HEX    (10)

#define WEP_128_KEY_ASCII (13)
#define WEP_128_KEY_HEX   (26)

#define WEP_152_KEY_ASCII (16)
#define WEP_152_KEY_HEX   (32)

#define WPS_PIN_LEN (8)

#define CHANNEL_MIN    (0)
#define CHANNEL_MAX    (14)
#define AUTO_CHANNEL    (0)
#define BG_MAX_CHANNEL  (11)

/** Fragmentation threshold 256 to 2346 */
#define FRAG_THRESHOLD_MIN  (256)
#define FRAG_THRESHOLD_MAX  (2346)

/** RTS threshold 1 to 2347 */
#define RTS_THRESHOLD_MIN   (1)
#define RTS_THRESHOLD_MAX   (2347)

#define MIN_UUID_LEN   (1)
#define MAX_UUID_LEN   (36)

#define MIN_DEVICENAME_LEN   (1)
#define MAX_DEVICENAME_LEN   (32)

#define MIN_MANUFACTURER_LEN   (1)
#define MAX_MANUFACTURER_LEN   (64)

#define MIN_MODELNAME_LEN   (1)
#define MAX_MODELNAME_LEN   (32)

#define MIN_MODELNUM_LEN   (1)
#define MAX_MODELNUM_LEN   (32)

#define MIN_SERIALNUM_LEN  (1)
#define MAX_SERIALNUM_LEN  (32)

#define MIN_DEV_TYPE_LEN  (1)
#define MAX_DEV_TYPE_LEN  (20)

#define MIN_OS_VERSION_LEN  (1)
#define MAX_OS_VERSION_LEN  (12)

#define MIN_FRIENDLY_NAME_LEN  (1)
#define MAX_FRIENDLY_NAME_LEN  (64)

#define MAX_URL_LEN  (128)

#define MIN_MODEL_DESC_LEN  (1)
#define MAX_MODEL_DESC_LEN  (128)

#define MIN_UPC_LEN  (1)
#define MAX_UPC_LEN  (128)

#define GTK_MIN   (30)

#define MAX_INT_STR (8)

/** Tx Power range 2dBm to 18 dBm */
#define MIN_TX_POWER (2)
#define MAX_TX_POWER (30)

/** Data rate index */
#define MIN_DATA_RATE_IDX (0)
#define MAX_DATA_RATE_IDX (28)
#define AUTO_DATA_RATE                   (0)
#define B_MODE_MAX_DATA_RATE_IDX         (4)
#define G_ONLY_MODE_MAX_DATA_RATE_IDX    (12)

/** parameters for read config */
#define GET_COMMENTED_VALUE 1
#define GET_ENABLED_ONLY    0

#define MAX_RESP_LEN 255

/** AP shutoff time */
#define AP_SHUTOFF_MIN    (0)
#define AP_SHUTOFF_MAX    (120)

/** AP shutoff time */
#define AP_ENERGY_DETECT_TH_MIN    (0)
#define AP_ENERGY_DETECT_TH_MAX    (9)

/** command request index - in the array Cmd_req[] */
enum eCmd_req {
    eCMD_GET = 0,
    eCMD_SET = 1,

    eCMD_REQ_LAST
};

/** config request index - in the array Conf_req[] */
enum eConf_req {
    CONF_2g = 0,
    CONF_5g = 1,
    CONF_owe = 2,
    CONF_60g = 3,

    CONF_REQ_LAST
};

/**
  * Command numbers, these numbers form the index into the array of
  * command names stored in the 'cmd_list'.
  *
  * Warning: An addtion of an entry in 'esap_cmd', should be followed
  * by an addition of a command name string in the 'cmd_list' array
  */
typedef enum esap_cmd {
    eCMD_INVALID             = -1,
    eCMD_SSID                = 0,
    eCMD_BSSID               = 1,
    eCMD_CHAN                = 2,
    eCMD_BCN_INTERVAL        = 3,
    eCMD_DTIM_PERIOD         = 4,
    eCMD_HW_MODE             = 5,
    eCMD_AUTH_ALGS           = 6,
    eCMD_SEC_MODE            = 7,
    eCMD_WEP_KEY0            = 8,
    eCMD_WEP_KEY1            = 9,
    eCMD_WEP_KEY2            = 10,
    eCMD_WEP_KEY3            = 11,
    eCMD_DEFAULT_KEY         = 12,
    eCMD_PASSPHRASE          = 13,
    eCMD_WPA_PAIRWISE        = 14,
    eCMD_RSN_PAIRWISE        = 15,
    eCMD_MAC_ADDR            = 16,
    eCMD_RESET_AP            = 17,
    eCMD_MAC_ACL             = 18,
    eCMD_ADD_TO_ALLOW        = 19,
    eCMD_ADD_TO_DENY         = 20,
    eCMD_REMOVE_FROM_ALLOW   = 21,
    eCMD_REMOVE_FROM_DENY    = 22,
    eCMD_ALLOW_LIST          = 23,
    eCMD_DENY_LIST           = 24,
    eCMD_COMMIT              = 25,
    eCMD_ENABLE_SOFTAP       = 26,
    eCMD_DISASSOC_STA        = 27,
    eCMD_RESET_TO_DEFAULT    = 28,
    eCMD_PROTECTION_FLAG     = 29,
    eCMD_DATA_RATES          = 30,
    eCMD_ASSOC_STA_MACS      = 31,
    eCMD_TX_POWER            = 32,
    eCMD_SDK_VERSION         = 33,
    eCMD_WMM_STATE           = 34,

    /** WARNING: The order of WPS commands should not be altered.
        New commands SHOULD be added above or below this            */
    eCMD_WPS_STATE           = 35,
    eCMD_WPS_CONFIG_METHOD     = 36,
    eCMD_UUID                = 37,
    eCMD_DEVICE_NAME         = 38,
    eCMD_MANUFACTURER        = 39,
    eCMD_MODEL_NAME          = 40,
    eCMD_MODEL_NUMBER        = 41,
    eCMD_SERIAL_NUMBER       = 42,
    eCMD_DEVICE_TYPE         = 43,
    eCMD_OS_VERSION          = 44,
    eCMD_FRIENDLY_NAME       = 45,
    eCMD_MANUFACTURER_URL    = 46,
    eCMD_MODEL_DESC          = 47,
    eCMD_MODEL_URL           = 48,
    eCMD_UPC                 = 49,
    /******************************************************/

    eCMD_FRAG_THRESHOLD      = 50,
    eCMD_RTS_THRESHOLD       = 51,
    eCMD_GTK_TIMEOUT         = 52,
    eCMD_COUNTRY_CODE        = 53,
    eCMD_INTRA_BSS_FORWARD   = 54,
    eCMD_REGULATORY_DOMAIN   = 55,
    eCMD_AP_STATISTICS       = 56,
    eCMD_AP_AUTOSHUTOFF      = 57,
    eCMD_AP_ENERGY_DETECT_TH = 58,
    eCMD_BASIC_RATES         = 59,
    eCMD_REQUIRE_HT          = 60,
    eCMD_IEEE80211N          = 61,
    eCMD_SET_CHANNEL_RANGE   = 62,
    eCMD_GET_AUTO_CHANNEL    = 63,
    eCMD_IEEE80211W          = 64,
    eCMD_WPA_KEY_MGMT        = 65,
    eCMD_SET_MAX_CLIENTS     = 66,
    eCMD_IEEE80211AC         = 67,
    eCMD_VHT_OPER_CH_WIDTH   = 68,
    eCMD_ACS_CHAN_LIST       = 69,
    eCMD_HT_CAPAB            = 70,
    eCMD_IEEE80211H          = 71,

    eCMD_ENABLE_WIGIG_SOFTAP = 72,
    eCMD_INTERFACE           = 73,
    eCMD_SSID2               = 74,
    eCMD_BRIDGE              = 75,
    eCMD_CTRL_INTERFACE      = 76,
    eCMD_VENDOR_ELEMENT      = 77,
    eCMD_ASSOCRESP_ELEMENT   = 78,
    eCMD_ACS_EXCLUDE_DFS     = 79,
    eCMD_WOWLAN_TRIGGERS     = 80,
    eCMD_ACCEPT_MAC_FILE     = 81,
    eCMD_DENY_MAC_FILE       = 82,
    eCMD_OWE_TRANS_IFNAME    = 83,
    eCMD_SAE_REQUIRE_MPF     = 84,

    eCMD_IEEE80211AX         = 85,

    eCMD_ENABLE_EDMG         = 86,
    eCMD_EDMG_CHANNEL        = 87,

    eCMD_LAST     /** New command numbers should be added above this */
} esap_cmd_t;

/** non-commands */
typedef enum esap_str {
    STR_WPA                      = 0,
    STR_ACCEPT_MAC_FILE          = 1,
    STR_DENY_MAC_FILE            = 2,
    STR_MAC_IN_INI               = 3,
    STR_PROT_FLAG_IN_INI         = 4,
    STR_DATA_RATE_IN_INI         = 5,
    STR_TX_POWER_IN_INI          = 6,
    STR_FRAG_THRESHOLD_IN_INI    = 7,
    STR_RTS_THRESHOLD_IN_INI     = 8,
    STR_COUNTRY_CODE_IN_INI      = 9,
    STR_INTRA_BSS_FORWARD_IN_INI = 10,
    STR_WMM_IN_INI               = 11,
    STR_802DOT11D_IN_INI         = 12,
    STR_HT_80211N                = 13,
    STR_CTRL_INTERFACE           = 14,
    STR_INTERFACE                = 15,
    STR_EAP_SERVER               = 16,
    STR_AP_AUTOSHUTOFF           = 17,
    STR_AP_ENERGY_DETECT_TH      = 18,
    eSTR_LAST
} esap_str_t;

/** Supported security mode */
typedef enum sec_mode {
    SEC_MODE_NONE            = 0,
    SEC_MODE_WEP             = 1,
    SEC_MODE_WPA_PSK         = 2,
    SEC_MODE_WPA2_PSK        = 3,
    SEC_MODE_WPA_WPA2_PSK    = 4,

    SEC_MODE_INVALID
} sec_mode_t;

/** security mode in the configuration file */
enum wpa_in_conf_file {
    WPA_IN_CONF_FILE = 1,
    WPA2_IN_CONF_FILE = 2,
    WPA_WPA2_IN_CONF_FILE = 3
};

enum {
    DISABLE   = 0,
    ENABLE    = 1
};

enum {
    FALSE = 0,
    TRUE = 1
};

/** IEEE 802.11 operating mode */
enum oper_mode {
    HW_MODE_B = 0,
    HW_MODE_G = 1,
    HW_MODE_N = 2,
    HW_MODE_G_ONLY = 3,
    HW_MODE_N_ONLY = 4,
    HW_MODE_A = 5,
    HW_MODE_ANY = 6,
    HW_MODE_AD = 7,

    HW_MODE_UNKNOWN
};

/** Authentication algorithm */
enum auth_alg {
    AHTH_ALG_OPEN = 1,
    AUTH_ALG_SHARED = 2,
    AUTH_ALG_OPEN_SHARED = 3,

    AUTH_ALG_INVALID
};

/** Allow or Deny MAC address list selection */
enum macaddr_acl {
    ACL_DENY_LIST = 0,
    ACL_ALLOW_LIST = 1,
    ACL_ALLOW_AND_DENY_LIST = 2
};

enum ap_reset {
    SAP_RESET_BSS = 0,
    SAP_RESET_DRIVER_BSS = 1,
    SAP_STOP_BSS = 2,
    SAP_STOP_DRIVER_BSS = 3,
#ifdef QCOM_WLAN_CONCURRENCY
    SAP_INITAP = 4,
    SAP_EXITAP = 5,
#endif
    SAP_RESET_INVALID
};

enum wmm_state {
    WMM_AUTO_IN_INI = 0,
    WMM_ENABLED_IN_INI = 1,
    WMM_DISABLED_IN_INI = 2
};

enum wps_state {
    WPS_STATE_DISABLE = 0,
    WPS_STATE_ENABLE  = 2
};


enum wps_config {
    WPS_CONFIG_PBC = 0,
    WPS_CONFIG_PIN = 1,
};

/** Choose the configuration file */
enum eChoose_conf_file {
    HOSTAPD_CONF_QCOM_FILE = 0,
    INI_CONF_FILE = 1
};

struct Command
{
    s8  * name;
    s8  * default_value;
};

/** STA Channel information*/
typedef struct sta_channel_info {
        int subioctl;
        int stastartchan;
        int staendchan;
        int staband;
} sta_channel_info;

/**SAP Channel information*/
typedef struct sap_channel_info {
        int startchan;
        int endchan;
        int band;
} sap_channel_info;

/**SAP  auto Channel information*/
typedef struct sap_auto_channel_info {
        int subioctl;
} sap_auto_channel_info;

/** Validate enable / disable softap */
#define IS_VALID_SOFTAP_ENABLE(x) (((value == ENABLE) || (value == DISABLE)) ? TRUE: FALSE)

/** Validate the channel */
#define IS_VALID_CHANNEL(x) ((value >= CHANNEL_MIN) && (value <= CHANNEL_MAX) ? TRUE : FALSE)

/** Validate the security mode */
#define IS_VALID_SEC_MODE(x) (((x >= SEC_MODE_NONE) && (x < SEC_MODE_INVALID)) ? TRUE : FALSE)

/** Validate the selection of access or deny MAC address list */
#define IS_VALID_MAC_ACL(x) (((x==ACL_DENY_LIST) || (x==ACL_ALLOW_LIST) || (x==ACL_ALLOW_AND_DENY_LIST)) ? TRUE : FALSE)

/** Validate the broadcast SSID status */
#define IS_VALID_BSSID(x) (((value == ENABLE) || (value == DISABLE)) ? TRUE: FALSE)

/** Validate the length of the passphrase */
#define IS_VALID_PASSPHRASE_LEN(x) ((((x >= PASSPHRASE_MIN) && (x <= PASSPHRASE_MAX)) || (x == 0)) ? TRUE: FALSE)

/** Validate the beacon interval */
#define IS_VALID_BEACON(x) (((x >= BCN_INTERVAL_MIN) && (x <= BCN_INTERVAL_MAX)) ? TRUE: FALSE)

/** Validate the DTIM period */
#define IS_VALID_DTIM_PERIOD(x) (((x >= DTIM_PERIOD_MIN) && (x <= DTIM_PERIOD_MAX)) ? TRUE: FALSE)

/** Validate the WEP index */
#define IS_VALID_WEP_KEY_IDX(x) ((x >= 0) && (x < 4) ? TRUE : FALSE)

/** Validate the pairwise encryption */
#define IS_VALID_PAIRWISE(x) (((!strcmp(x, "TKIP")) || (!strcmp(x, "CCMP")) || \
                    (!strcmp(x, "TKIP CCMP")) || (!strcmp(x, "CCMP TKIP")) || (!strcmp(x, "GCMP"))) ? TRUE : FALSE)

/** Validate the WMM status */
#define IS_VALID_WMM_STATE(x) (((x >= WMM_AUTO_IN_INI) && (x <= WMM_DISABLED_IN_INI)) ? TRUE: FALSE)

/** Validate the WPS status */
#define IS_VALID_WPS_STATE(x) (((x == ENABLE) || (x == DISABLE)) ? TRUE: FALSE)

/** Validate the fragmentation threshold */
#define IS_VALID_FRAG_THRESHOLD(x) (((x >= FRAG_THRESHOLD_MIN) && (x <= FRAG_THRESHOLD_MAX)) ? TRUE: FALSE)

/** Validate the RTS threshold value */
#define IS_VALID_RTS_THRESHOLD(x) (((x >= RTS_THRESHOLD_MIN) && (x <= RTS_THRESHOLD_MAX)) ? TRUE: FALSE)

/** Validate the GTK */
#define IS_VALID_GTK(x) ((x >= GTK_MIN) ? TRUE: FALSE)

/** Validate the intra-bss forwarding status */
#define IS_VALID_INTRA_BSS_STATUS(x) (((x == ENABLE) || (x == DISABLE)) ? TRUE: FALSE)

/** Validate the protection flag */
#define IS_VALID_PROTECTION(x) (((x == ENABLE) || (x == DISABLE)) ? TRUE: FALSE)

/** Validate the UUID length */
#define IS_VALID_UUID_LEN(x) (((x >= MIN_UUID_LEN) && (x <= MAX_UUID_LEN)) ? TRUE : FALSE)

/** Validate the device name length */
#define IS_VALID_DEVICENAME_LEN(x) (((x >= MIN_DEVICENAME_LEN) && (x <= MAX_DEVICENAME_LEN)) ? TRUE : FALSE)

/** Validate the Manufacturer length */
#define IS_VALID_MANUFACTURER_LEN(x) (((x >= MIN_MANUFACTURER_LEN) && (x <= MAX_MANUFACTURER_LEN)) ? TRUE : FALSE)

/** Validate the Model name length */
#define IS_VALID_MODELNAME_LEN(x) (((x >= MIN_MODELNAME_LEN) && (x <= MAX_MODELNAME_LEN)) ? TRUE : FALSE)

/** Validate the Model number length */
#define IS_VALID_MODELNUM_LEN(x) (((x >= MIN_MODELNUM_LEN) && (x <= MAX_MODELNUM_LEN)) ? TRUE : FALSE)

/** Validate the Model serial number length */
#define IS_VALID_SERIALNUM_LEN(x) (((x >= MIN_SERIALNUM_LEN) && (x <= MAX_SERIALNUM_LEN)) ? TRUE : FALSE)

/** Validate the Primary device type length */
#define IS_VALID_DEV_TYPE_LEN(x) (((x >= MIN_DEV_TYPE_LEN) && (x <= MAX_DEV_TYPE_LEN)) ? TRUE : FALSE)

/** Validate the OS version length */
#define IS_VALID_OS_VERSION_LEN(x) (((x >= MIN_OS_VERSION_LEN) && (x <= MAX_OS_VERSION_LEN)) ? TRUE : FALSE)

/** Validate the friendly name length */
#define IS_VALID_FRIENDLY_NAME_LEN(x) (((x >= MIN_FRIENDLY_NAME_LEN) && (x <= MAX_FRIENDLY_NAME_LEN)) ? TRUE : FALSE)

/** Validate the URL length */
#define IS_VALID_URL_LEN(x) (((x > 0) && (x <= MAX_URL_LEN)) ? TRUE : FALSE)

/** Validate the model description length */
#define IS_VALID_MODEL_DESC_LEN(x) (((x > MIN_MODEL_DESC_LEN) && (x <= MAX_MODEL_DESC_LEN)) ? TRUE : FALSE)

/** Validate the Universal Product Code (UPC) length */
#define IS_VALID_UPC_LEN(x) (((x > MIN_UPC_LEN) && (x <= MAX_UPC_LEN)) ? TRUE : FALSE)

/** Validate the Tx power index */
#define IS_VALID_TX_POWER(x) (((x >= MIN_TX_POWER ) && (x <= MAX_TX_POWER)) ? TRUE : FALSE)

/** Validate the Data rate */
#define IS_VALID_DATA_RATE_IDX(x) (((x >= MIN_DATA_RATE_IDX) && (x <= MAX_DATA_RATE_IDX)) ? TRUE : FALSE )

/** Validate WPS config */
#define IS_VALID_WPS_CONFIG(x) (((x == WPS_CONFIG_PBC) || (x == WPS_CONFIG_PIN)) ? TRUE : FALSE)

/** Validate the 802dot11d state */
#define IS_VALID_802DOT11D_STATE(x) (((x == ENABLE) || (x == DISABLE)) ? TRUE: FALSE)

/** Validate the AP shutoff time */
#define IS_VALID_APSHUTOFFTIME(x) (((x >= AP_SHUTOFF_MIN) && (x <= AP_SHUTOFF_MAX)) ? TRUE : FALSE)

/** Validate the AP shutoff time */
#define IS_VALID_ENERGY_DETECT_TH(x) ((((x >= AP_ENERGY_DETECT_TH_MIN) && (x <= AP_ENERGY_DETECT_TH_MAX)) ||( x == 128)) ? TRUE : FALSE)

/** Validate the 802dot11h state */
#define IS_VALID_DFS_STATE(x) (((x == ENABLE) || (x == DISABLE)) ? TRUE: FALSE)

/** Function declartion */
int qsap_hostd_exec(int argc, char ** argv);
void qsap_hostd_exec_cmd(s8 *pcmd, s8 *presp, u32 *plen);
s8 *qsap_get_config_value(s8 *pfile, struct Command *pcmd, s8 *pbuf, u32 *plen);
int qsapsetSoftap(int argc, char *argv[]);
int qsap_add_or_remove_interface(const char *iface_name, int create_iface);
void qsap_del_ctrl_iface(void);
s16 wifi_qsap_reset_to_default(s8 *pcfgfile, s8 *pdefault);
void check_for_configuration_files(void);
void qsap_set_ini_filename(void);
int qsap_set_channel_range(s8 * cmd);
int qsap_get_sap_auto_channel_slection(s32 *pautochan);
int qsap_get_mode(s32 *pmode);
int qsap_prepare_softap(void);
int qsap_unprepare_softap(void);
int qsap_is_fst_enabled(void);
int qsap_control_bridge(int argc, char ** argv);
int linux_get_ifhwaddr(const char *ifname, char *addr);

#if __cplusplus
};  // extern "C"
#endif

#endif

