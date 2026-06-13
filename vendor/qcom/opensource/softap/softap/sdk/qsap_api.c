/*
 * Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation, Inc. nor the names of its
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/wireless.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include "nl80211_copy.h"

#include "qsap_api.h"
#include "qsap.h"

#define QCSAP_IOCTL_GETPARAM          (SIOCIWFIRSTPRIV+1)
#define WLAN_PRIV_SET_THREE_INT_GET_NONE  (SIOCIWFIRSTPRIV + 4)
#define QCSAP_IOCTL_GET_CHANNEL       (SIOCIWFIRSTPRIV+9)
#define QCSAP_IOCTL_ASSOC_STA_MACADDR (SIOCIWFIRSTPRIV+10)
#define QCSAP_IOCTL_DISASSOC_STA      (SIOCIWFIRSTPRIV+11)
#define QCSAP_IOCTL_AP_STATS          (SIOCIWFIRSTPRIV+12)
#define QCSAP_IOCTL_SET_CHANNEL_RANGE (SIOCIWFIRSTPRIV+17)
#define QCSAP_PARAM_GET_AUTO_CHANNEL 9
#define WE_SET_SAP_CHANNELS  3

#define LOG_TAG "QCSDK"

#include <cutils/properties.h>
#include <cutils/log.h>

#define SKIP_BLANK_SPACE(x) {while(*x != '\0') { if((*x == ' ') || (*x == '\t')) x++; else break; }}

/** If this variable is enabled, the soft AP is reloaded, after the commit
  * command is received */
static volatile int gIniUpdated = 0;

/** Supported command requests.
  * WANRING: The enum eCMD_REQ in the file qsap_api.h should be
  * updated if Cmd_req[], us updated
  */
s8 *Cmd_req[eCMD_REQ_LAST] = {
    "get",
    "set"
};

/** Supported config file requests.
  * WANRING: The enum eConf_req in the file qsap_api.h should be
  * updated if Conf_req[], us updated
  */
s8 *Conf_req[CONF_REQ_LAST] = {
    "dual2g",
    "dual5g",
    "owe",
    "60g",
};

/*
 * WARNING: On updating the cmd_list, the enum esap_cmd in file
 * qsap_api.h must be updates to reflect the changes
 */
static struct Command cmd_list[eCMD_LAST] = {
    { "ssid",                  "QualcommSoftAP" },
    { "ignore_broadcast_ssid", "0"              },
    { "channel",               "1"              },
    { "beacon_int",            "100"            },
    { "dtim_period",           "2"              },
    { "hw_mode",               "n"              },
    { "auth_algs",             "3"              },
    { "security_mode",         "0"              },
    { "wep_key0",              NULL             },
    { "wep_key1",              NULL             },
    { "wep_key2",              NULL             },
    { "wep_key3",              NULL             },
    { "wep_default_key",       NULL             },
    { "wpa_passphrase",        NULL             },
    { "wpa_pairwise",          NULL             },
    { "rsn_pairwise",          NULL             },
    { "mac_address",           "00deadbeef04"   },
    { "reset_ap",              NULL             },
    { "macaddr_acl",           "0"              },
    { "add_to_allow_list",     NULL             },
    { "add_to_deny_list",      NULL             },
    { "remove_from_allow_list", NULL            },
    { "remove_from_deny_list", NULL             },
    { "allow_list",            ""               },
    { "deny_list",             ""               },
    { "commit",                NULL             },
    { "enable_softap",         NULL             },
    { "disassoc_sta",          NULL             },
    { "reset_to_default",      NULL             },
    { "protection_flag",       "1"              },
    { "data_rate",             "0"              },
    { "sta_mac_list",          NULL             },
    { "tx_power",              "27"             },
    { "sdk_version",           SDK_VERSION      },
    { "wmm_enabled",           "0"              },

    /** Warning: Do not change the order of the WPS commands */
    { "wps_state",             "0"              },
    { "config_methods",        NULL             },
    { "uuid",                  NULL             },
    { "device_name",           NULL             },
    { "manufacturer",          NULL             },
    { "model_name",            NULL             },
    { "model_number",          NULL             },
    { "serial_number",         NULL             },
    { "device_type",           NULL             },
    { "os_version",            NULL             },
    { "friendly_name",         NULL             },
    { "manufacturer_url",      NULL             },
    { "model_description",     NULL             },
    { "model_url",             NULL             },
    { "upc",                   NULL             },
    /************ WPS commands end *********/

    { "fragm_threshold",       NULL             },
    { "rts_threshold",         NULL             },
    { "wpa_group_rekey",       NULL             },
    { "country_code",          NULL             },
    { "ap_isolate",            NULL             },
    { "ieee80211d",            NULL             },
    { "apstat",                NULL             },
    { "auto_shut_off_time",    NULL             },
    { "energy_detect_threshold", "128"          },
    { "basic_rates",            NULL            },
    { "require_ht",            NULL             },
    { "ieee80211n",            "1"              },
    { "setchannelrange",       NULL             },
    { "autochannel",           NULL             },
    { "ieee80211w",            NULL             },
    { "wpa_key_mgmt",          NULL             },
    { "max_num_sta",           "8"              },
    { "ieee80211ac",           NULL             },
    { "vht_oper_chwidth",      NULL             },
    { "chanlist",              NULL             },
    { "ht_capab",              NULL             },
    { "ieee80211h",            NULL             },
    { "enable_wigig_softap",   NULL             },
    { "interface",             NULL             },
    { "ssid2",                 NULL             },
    { "bridge",                NULL             },
    { "ctrl_interface",        NULL             },
    { "vendor_elements",       NULL             },
    { "assocresp_elements",    NULL             },
    { "acs_exclude_dfs",       NULL             },
    { "wowlan_triggers",       "any"            },
    { "accept_mac_file",       NULL             },
    { "deny_mac_file",         NULL             },
    { "owe_transition_ifname", NULL             },
    { "sae_require_mfp",       NULL             },
    { "ieee80211ax",           NULL             },
    { "enable_edmg",           NULL             },
    { "edmg_channel",          NULL             },

};

struct Command qsap_str[eSTR_LAST] = {
    { "wpa",                     NULL           },
    { "accept_mac_file",         NULL           },
    { "deny_mac_file",           NULL           },
    { "gAPMacAddr",              "00deadbeef04" },/** AP MAC address */
    { "gEnableApProt",           "1"            },/** protection flag in ini file */
    { "gFixedRate",              "0"            },/** Fixed rate in ini */
    { "gTxPowerCap",             "27"           },/** Tx power in ini */
    { "gFragmentationThreshold", "2346"         },/** Fragmentation threshold in ini */
    { "RTSThreshold",            "2347"         },/** RTS threshold in ini */
    { "gAPCntryCode",            "USI"          },/** Country code in ini */
    { "gDisableIntraBssFwd",     "0"            },/** Intra-bss forward in ini */
    { "WmmIsEnabled",            "0"            },/** WMM */
    { "g11dSupportEnabled",      "1"            },/** 802.11d support */
    { "ieee80211n",              NULL           },
    { "ctrl_interface",          NULL           },
    { "interface",               NULL           },
    { "eap_server",              NULL           },
    { "gAPAutoShutOff",          "0"            },
    { "gEnablePhyAgcListenMode",  "128"          },
};

/** Supported operating mode */
char *hw_mode[HW_MODE_UNKNOWN] = {
    "b", "g", "n", "g-only", "n-only", "a", "any", "ad"
};

/** configuration file path */
char *pconffile = CONFIG_FILE;
char *fIni =  WIFI_DRIVER_CONF_FILE;
s8 ini_file[PROPERTY_VALUE_MAX] = {0};

static int qsap_scnprintf(char *str, size_t size, const char *format, ...)
{
    va_list arg_ptr;
    int ret = 0;

    if (size < 1)
        return 0;
    va_start(arg_ptr, format);
    ret = vsnprintf(str, size, format, arg_ptr);
    va_end(arg_ptr);
    return (ret < (int)size) ? ret : (int)(size - 1);
}

/**
 * @brief
 *        For a give configuration parameter, read the configuration value from the file.
 * @param pfile [IN] configuration file path
 * @param pcmd [IN] pointer to the comand structure
 * @param presp [OUT] buffer to store the configuration value
 * @param plen [IN-OUT] The length of the buffer is provided as input.
 *                      The length of the configuration parameter value, stored
 *                      in the 'presp', is provided as the output
 * @param ignore_comment [IN] if set, read the commented value also
 * @return void
*/
static s32 qsap_read_cfg(s8 *pfile, struct Command * pcmd, s8 *presp, u32 *plen, s8 *var, s32 ignore_comment)
{
    FILE *fcfg;
    s8    buf[MAX_CONF_LINE_LEN];
    u16   len;
    s8   *val;

    /** Open the configuration file */
    fcfg = fopen(pfile, "r");

    if(NULL == fcfg) {
        ALOGE("%s : unable to open file \n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        return eERR_FILE_OPEN;
    }

    /** Read the line from the configuration file */
    len = strlen(pcmd->name);
    while(NULL != fgets(buf, MAX_CONF_LINE_LEN, fcfg)) {
        s8 *pline = buf;

        if (strlen(buf) == 0)
           continue;

        /** Skip the commented lines */
        if(buf[0] == '#') {
            if (ignore_comment) {
                pline++;
            }
            else continue;
        }

        /** Identify the configuration parameter in the configuration file */
        if(!strncmp(pline, pcmd->name, len) && (pline[len] == '=')) {
            int tmp_indx;

           /* Delate all \r \n combinations infront of the config string */
            tmp_indx = strlen(buf)-1;
            while( (buf[tmp_indx] == '\r') || (buf[tmp_indx] == '\n') ) tmp_indx--;

            buf[tmp_indx+1] = '\0';

            if ( NULL != var ) {
                val = strchr(pline, '=');
                if(NULL == val)
                    break;
                *plen = qsap_scnprintf(presp, *plen, "%s %s%s", SUCCESS, var, val);
            }
            else {
                *plen = qsap_scnprintf(presp, *plen, "%s %s", SUCCESS, pline);
            }
            fclose(fcfg);
            return eSUCCESS;
        }
    }

#if 0
    /** Configuration parameter is absent in the file */
    *plen = qsap_scnprintf(presp, *plen, "%s", ERR_FEATURE_NOT_ENABLED);
#else
    /** Value not found in the configuration file */
    /** Send the default value, if we are reading from ini file */
    if ( pcmd->default_value ) {
        *plen = qsap_scnprintf(presp, *plen, "%s %s=%s", SUCCESS, var?var:pcmd->name, pcmd->default_value);
        fclose(fcfg);
        return eSUCCESS;
    }
    else {
        /** Configuration parameter is absent in the file */
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_FEATURE_NOT_ENABLED);
    }
#endif

    fclose(fcfg);

    return eERR_CONFIG_PARAM_MISSING;
}

/**
 * @brief
 *        Write the configuration parameter value into the configuration file.
 * @param pfile [IN] configuration file path.
 * @param pcmd [IN] command name
 * @param pVal [IN] configuration parameter to be written to the file.
 * @param presp [OUT] buffer to store the configuration value.
 * @param plen [IN-OUT] The length of the buffer is provided as input.
 *                      The length of the configuration parameter value, stored
 *                      in the 'presp', is provided as the output
 * @return void
*/
static s32 qsap_write_cfg(s8 *pfile, struct Command * pcmd, s8 *pVal, s8 *presp, u32 *plen, s32 inifile)
{
    FILE *fcfg, *ftmp;
    s8 buf[MAX_CONF_LINE_LEN+1];
    s16 len, result = FALSE;

    ALOGD("cmd=%s, Val:%s, INI:%d \n", pcmd->name, pVal, (int)inifile);

    /** Open the configuration file */
    fcfg = fopen(pfile, "r");
    if(NULL == fcfg) {
        ALOGE("%s : unable to open file \n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        return eERR_FILE_OPEN;
    }

    qsap_scnprintf(buf, sizeof(buf), "%s~", pfile);

    /** Open a temporary file */
    ftmp = fopen(buf, "w+");
    if(NULL == ftmp) {
        ALOGE("%s : unable to open tmp file \n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        fclose(fcfg);
        return eERR_FILE_OPEN;
    }

    /** Read the values from the configuration file */
    len = strlen(pcmd->name);
    while(NULL != fgets(buf, MAX_CONF_LINE_LEN, fcfg)) {
        s8 *pline = buf;

        /** commented line */
        if(buf[0] == '#')
            pline++;

        /** Identify the configuration parameter to be updated */
        if((!strncmp(pline, pcmd->name, len)) && (result == FALSE)) {
            if(pline[len] == '=') {
                qsap_scnprintf(buf, sizeof(buf), "%s=%s\n", pcmd->name, pVal);
                result = TRUE;
                ALOGD("Updated:%s\n", buf);
            }
        }

        if(inifile && (!strncmp(pline, "END", 3)))
            break;

        fprintf(ftmp, "%s", buf);
    }

    if (result == FALSE) {
        /* Configuration line not found */
        /* Add the new line at the end of file */
        qsap_scnprintf(buf, sizeof(buf), "%s=%s\n", pcmd->name, pVal);
        fprintf(ftmp, "%s", buf);
        ALOGD("Adding a new line in %s file: [%s] \n", inifile ? "inifile" : "hostapd.conf", buf);
    }

    if(inifile) {
        gIniUpdated = 1;
        fprintf(ftmp, "END\n");
        while(NULL != fgets(buf, MAX_CONF_LINE_LEN, fcfg))
            fprintf(ftmp, "%s", buf);
    }

    fclose(fcfg);
    fclose(ftmp);

    qsap_scnprintf(buf, sizeof(buf), "%s~", pfile);

    /** Restore the updated configuration file */
    result = rename(buf, pfile);

    *plen = qsap_scnprintf(presp, *plen, "%s", (result == eERR_UNKNOWN) ? ERR_FEATURE_NOT_ENABLED : SUCCESS);

    /** Remove the temporary file. Dont care the return value */
    unlink(buf);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(pfile, 0660) < 0) {
        ALOGE("Error changing permissions of %s to 0660: %s",
                pfile, strerror(errno));
        unlink(pfile);
        return -1;
    }
    if(result == eERR_UNKNOWN)
        return eERR_FEATURE_NOT_ENABLED;

    return eSUCCESS;
}

/**
 * @brief Read the security mode set in the configuration
 * @param pfile [IN] configuration file path.
 * @param presp [OUT] buffer to store the security mode.
 * @param plen [IN-OUT] The length of the buffer is provided as input.
 *                      The length of the security mode value, stored
 *                      in the 'presp', is provided as the output
 * @return void
*/
static sec_mode_t qsap_read_security_mode(s8 *pfile, s8 *presp, u32 *plen)
{
    sec_mode_t mode;
    u32 temp = *plen;

    /** Read the WEP default key */
    qsap_read_cfg(pfile, &cmd_list[eCMD_DEFAULT_KEY], presp, plen, NULL, GET_ENABLED_ONLY);

    if ( !strcmp(presp, ERR_FEATURE_NOT_ENABLED) ) {
        *plen = temp;

        /* WEP, is not enabled */

        /** Read WPA security status */
        qsap_read_cfg(pfile, &qsap_str[STR_WPA], presp, plen, NULL, GET_ENABLED_ONLY);
        if ( !strcmp(presp, ERR_FEATURE_NOT_ENABLED) ) {
            /** WPA is disabled, No security */
            mode = SEC_MODE_NONE;
        }
        else {
            /** WPA, WPA2 or WPA-WPA2 mixed security */
            s8 * ptmp = presp;
            while((*plen)-- && (*ptmp++ != '=') );
            mode =     *plen ? (
                    *ptmp == '1' ? SEC_MODE_WPA_PSK :
                    *ptmp == '2' ? SEC_MODE_WPA2_PSK :
                    *ptmp == '3' ? SEC_MODE_WPA_WPA2_PSK : SEC_MODE_INVALID ): SEC_MODE_INVALID;
        }
    }
    else {
        /** Verify if, WPA is disabled */
        *plen = temp;
        qsap_read_cfg(pfile, &qsap_str[STR_WPA], presp, plen, NULL, GET_ENABLED_ONLY);
        if ( !strcmp(presp, ERR_FEATURE_NOT_ENABLED) ) {
            /** WPA is disabled, hence WEP is enabled */
            mode = SEC_MODE_WEP;
        }
        else {
            *plen = qsap_scnprintf(presp, temp, "%s", ERR_UNKNOWN);
            return SEC_MODE_INVALID;
        }
    }

    if(mode != SEC_MODE_INVALID) {
        *plen = qsap_scnprintf(presp, temp,"%s %s=%d", SUCCESS, cmd_list[eCMD_SEC_MODE].name, mode);
    }
    else {
        *plen = qsap_scnprintf(presp, temp,"%s", ERR_NOT_SUPPORTED);
    }

    return mode;
}

/**
 * @brief
 *         Enable or disable a configuration parameter in the configuration file.
 * @param pfile [IN] configuration file name
 * @param pcmd [IN] configuration command structure
 * @param status [IN] status to be set. The valid values are 'ENABLE' or 'DISABLE'
 * @return On success, return 0
 *         On failure, return -1
*/
static s32 qsap_change_cfg(s8 *pfile, struct Command * pcmd, u32 status)
{
    FILE *fcfg, *ftmp;
    s8 buf[MAX_CONF_LINE_LEN+1];
    u16 len;

    /** Open the configuartion file */
    fcfg = fopen(pfile, "r");
    if(NULL == fcfg) {
        ALOGE("%s : unable to open file \n", __func__);
        return eERR_UNKNOWN;
    }

    qsap_scnprintf(buf, sizeof(buf), "%s~", pfile);

    /** Open a temporary file */
    ftmp = fopen(buf, "w");
    if(NULL == ftmp) {
        ALOGE("%s : unable to open tmp file \n", __func__);
        fclose(fcfg);
        return eERR_UNKNOWN;
    }

    /** Read the configuration parameters from the configuration file */
    len = strlen(pcmd->name);
    while(NULL != fgets(buf+1, MAX_CONF_LINE_LEN, fcfg)) {
        s8 *p = buf+1;

        /** Commented line */
        if(p[0] == '#')
            p++;

        /** Identify the configuration parameter */
        if(!strncmp(p, pcmd->name, len)) {
            if(p[len] == '=') {
                if(status == DISABLE) {
                    fprintf(ftmp, "#%s", p);
                }
                else {
                    fprintf(ftmp, "%s", p);
                }
                continue;
            }
        }
        fprintf(ftmp, "%s", buf+1);
    }

    fclose(fcfg);
    fclose(ftmp);

    qsap_scnprintf(buf, sizeof(buf), "%s~", pfile);

    /** Restore the new configuration file */
    if(eERR_UNKNOWN == rename(buf, pfile)) {
        ALOGE("unable to rename the file \n");
        return eERR_UNKNOWN;
    }

    /** Delete the temporary file */
    unlink(buf);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(pfile, 0660) < 0) {
        ALOGE("Error changing permissions of %s to 0660: %s",
                pfile, strerror(errno));
        unlink(pfile);
        return -1;
    }
    return 0;
}

/**
 * @brief
 *         Set the security mode in the configuration. The security mode
 *         can be :
 *                   1. No security
 *                   2. WEP
 *                   3. WPA
 *                   4. WPA2
 *                   5. WPA and WPA2 mixed mode
 * @param pfile [IN] configuration file name
 * @param sec_mode [IN] security mode to be set
 * @param presp [OUTPUT] presp The command output format :
 *                    On success,
 *                            success <cmd>=<value>
 *                    On failure,
 *                            failure <error message>
 * @param plen [IN-OUT] plen
 *                      [IN] The length of the buffer, presp
 *                      [OUT] The length of the response in the buffer, presp
 * @return void
*/
static void qsap_set_security_mode(s8 *pfile, u32 sec_mode, s8 *presp, u32 *plen)
{
    s16 wep, wpa;
    s8 sec[MAX_INT_STR];
    s32 rsn_status = DISABLE;
    s32 ret = eERR_UNKNOWN;

    /** Is valid security mode ? */
    if(sec_mode >= SEC_MODE_INVALID) {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);
        return;
    }

    /** No security */
    if(SEC_MODE_NONE == sec_mode) {
        wep = DISABLE;
        wpa = DISABLE;
    }
    /** WEP security */
    else if(SEC_MODE_WEP == sec_mode) {
        wep = ENABLE;
        wpa = DISABLE;
    }
    else {
        /** WPA, WPA2 and mixed-mode security */
        u16 wpa_val;
        u32 tmp = *plen;

        wep = DISABLE;
        wpa = ENABLE;

        if(sec_mode == SEC_MODE_WPA_PSK)
            wpa_val = WPA_IN_CONF_FILE;

        else if(sec_mode == SEC_MODE_WPA2_PSK) {
            wpa_val = WPA2_IN_CONF_FILE;
            rsn_status = ENABLE;
        }

        else if(sec_mode == SEC_MODE_WPA_WPA2_PSK) {
            wpa_val = WPA_WPA2_IN_CONF_FILE;
            rsn_status = ENABLE;
        }

        else {
            goto end;
        }

        qsap_scnprintf(sec, sizeof(sec), "%u", wpa_val);
        qsap_write_cfg(pfile, &qsap_str[STR_WPA], sec, presp, plen, HOSTAPD_CONF_QCOM_FILE);
        *plen = tmp;
    }

    /** The configuration parameters for the security to be set are enabled
      * and the configuration parameters for the other security types are
      * disabled in the configuration file
      */
    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_DEFAULT_KEY], wep)) {
        ALOGE("%s: wep_default_key error\n", __func__);
        goto end;
    }

    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_WEP_KEY0], wep)) {
        ALOGE("%s: CMD_WEP_KEY0 \n", __func__);
        goto end;
    }

    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_WEP_KEY1], wep)) {
        ALOGE("%s: CMD_WEP_KEY1 \n", __func__);
        goto end;
    }

    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_WEP_KEY2], wep)) {
        ALOGE("%s: CMD_WEP_KEY2 \n", __func__);
        goto end;
    }

    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_WEP_KEY3], wep)) {
        ALOGE("%s: CMD_WEP_KEY3 \n", __func__);
        goto end;
    }

    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_PASSPHRASE], wpa)) {
        ALOGE("%s: Passphrase error\n", __func__);
        goto end;
    }

    if((sec_mode != SEC_MODE_NONE) && (sec_mode != SEC_MODE_WEP)) {
        u32 state = !rsn_status;

        if(sec_mode == SEC_MODE_WPA_WPA2_PSK) state = ENABLE;

        if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_WPA_PAIRWISE], state)) {
            ALOGE("%s: WPA Pairwise\n", __func__);
            goto end;
        }
    }

    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &cmd_list[eCMD_RSN_PAIRWISE], rsn_status)) {
        ALOGE("%s: WPA2 Pairwise\n", __func__);
        goto end;
    }

    if(eERR_UNKNOWN == qsap_change_cfg(pfile, &qsap_str[STR_WPA], wpa)) {
        ALOGE("%s: WPA\n", __func__);
        goto end;
    }

    ret = eSUCCESS;

end:
    *plen = qsap_scnprintf(presp, *plen, "%s", (ret == eSUCCESS) ? SUCCESS : ERR_UNKNOWN);

    return;
}

/**
 * @brief
 *         Get the file path having the allow or deny MAC address list
 * @param pcfgfile [IN] configuration file name
 * @param pcmd [IN] pcmd pointer to the command string
 * @param pfile [OUT] buffer to store the return value, containing the file name
 *                   or the error message.
 * @param plen [IN-OUT] size of the buffer 'pfile', is provided as input and
 *                      the length of the file name is returned as output
 * @return
 *           On success, a pointer to the file name in the buffer 'pfile'.
 *           On failure, NULL is returned
*/
static s8 *qsap_get_allow_deny_file_name(s8 *pcfgfile, struct Command * pcmd, s8 *pfile, u32 *plen)
{
    if(eSUCCESS == qsap_read_cfg(pcfgfile, pcmd, pfile, plen, NULL, GET_ENABLED_ONLY)) {
        return strchr(pfile, '=') + 1;
    }

    return NULL;
}

int qsap_hostd_exec(int argc, char ** argv)  {

#define MAX_CMD_SIZE 256
    char qcCmdBuf[MAX_CMD_SIZE], *pCmdBuf;
    u32 len = MAX_CMD_SIZE;
    int i = 2, ret;

    if ( argc < 4 ) {
        ALOGD("failure: invalid arguments");
        return -1;
    }

    argc -= 2;
    pCmdBuf = qcCmdBuf;

    while (argc--) {
        ret = snprintf(pCmdBuf, len, " %s", argv[i]);
        if ((ret < 0) || (ret >= (int)len)) {
            /* Error case */
            /* TODO: Command too long send the error message */
            *pCmdBuf = '\0';
             break;
        }
        ALOGD("argv[%d] (%s)",i, argv[i]);
        pCmdBuf += ret;
        len -= ret;
        i++;
    }

    ALOGD("QCCMD data (%s)", pCmdBuf);
    len = MAX_CMD_SIZE;
    qsap_hostd_exec_cmd(qcCmdBuf, qcCmdBuf, (u32*)&len);
    return 0;
}
/** Function to identify a valid MAC address */
static int isValid_MAC_address(char *pMac)
{
    int i, len;

    len = strlen(pMac);

    if(len < MAC_ADDR_LEN)
        return FALSE;

    for(i=0; i<MAC_ADDR_LEN; i++) {
        switch(i) {
            case 2: case 5: case 8: case 11: case 14:
                if(pMac[i] != ':')
                    return FALSE;
                break;
            default:
                if(isxdigit(pMac[i]) == 0)
                    return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief
 *        Add a given MAC address to the allow or deny MAC list file.
 *        A maximum of 15 MAC addresses are allowed in the list. If the input
 *        MAC addresses are more than the allowed number, then the allowed number
 *        of MAC addresses are updated to the MAC list file and the remaining
 *        MAC addresses are discarded
 *
 * @param pfile [IN] Path of the allow or deny MAC list file
 * @param pVal [IN] A string containing one or more MAC addresses. Multiple
 *                  MAC addresses are separated by a SPACE separator
 *                  Ex. "11:22:33:44:55:66 77:88:99:00:88:00"
 * @param presp [OUT] buffer to store the response
 * @param plen [IN-OUT] The length of the buffer 'presp' is provided as input.
 *                      The length of the response, stored in buffer 'presp' is
 *                      provided as output.
 * @return void
*/
static void qsap_add_mac_to_file(s8 *pfile, s8 *pVal, s8 *presp, u32 *plen)
{
    s32 len;
    s16 num_macs = 0;
    s8 buf[32];
    s8 macbuf[32];
    FILE *fp;

    /** Create the file if it does not exists and open it for reading */
    fp = fopen(pfile, "a");
    if(NULL != fp) {
        fclose(fp);
        fp = fopen(pfile, "r+");
    }

    if(NULL == fp) {
        ALOGE("%s : unable to open the file \n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        return;
    }

    /** count the MAC address in the MAC list file */
    while(NULL != (fgets(buf, 32, fp))) {
        num_macs++;
    }

    /** Evaluate the allowed limit */
    if(num_macs >= MAX_ALLOWED_MAC) {
        ALOGE("%s : File is full\n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);
        fclose(fp);
        return;
    }

    /** Update all the input MAC addresses into the MAC list file */
    len = strlen(pVal);
    while(len > 0) {
        int i = 0;

        /** Get a MAC address from the input string */
        while((*pVal != ' ' ) && (*pVal != '\0')) {
            macbuf[i] = *pVal;
            i++;
            pVal++;

            if(i == MAC_ADDR_LEN)
                break;
        }
        macbuf[i] = '\0';
        pVal++;

        /** Is valid MAC address input ? */
        if(TRUE == isValid_MAC_address(macbuf)) {

            /** Append the MAC address to the file */
            fprintf(fp, "%s\n", macbuf);
            num_macs++;

            /** Evaluate with the allowed limit */
            if(num_macs == MAX_ALLOWED_MAC) {
                ALOGE("MAC file is full now.... \n");
                break;
            }

        }
        len -= strlen(macbuf);
        if(*pVal != '\0')
            len--;
    }

    fclose(fp);

    *plen = qsap_scnprintf(presp, *plen, "%s", SUCCESS);

    return;
}

/**
 * @brief
 *         Remove one or more MAC addresses from the allow or deny MAC list file.
 * @param pfile [IN] path of the allow or deny list file.
 * @param pVal [IN] a list of MAC addresses to be removed from the MAC list file.
 * @param presp [OUT] the buffer to store the response
 * @param plen [IN-OUT] The length of the 'presp' buffer is provided as input.
 *                      The lenght of the response, stored in 'presp', is
 *                      provided as output
 * @return void
*/
static void qsap_remove_from_file(s8 *pfile, s8 *pVal, s8 *presp, u32 *plen)
{
    FILE *fp;
    FILE *ftmp;
    s8 buf[MAX_CONF_LINE_LEN];
    int status;

    /** Open the allow or deny MAC list file */
    fp = fopen(pfile, "r+");

    if(NULL == fp) {
        ALOGE("%s : unable to open the file \n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        return;
    }

    qsap_scnprintf(buf, sizeof(buf), "%s~", pfile);

    /** Open a temporary file */
    ftmp = fopen(buf, "w");

    if(ftmp == NULL) {
        ALOGE("%s : unable to open the file \n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        fclose(fp);
        return;
    }

    /** Read all the MAC addresses from the file */
    while(NULL != fgets(buf, MAX_CONF_LINE_LEN, fp)) {
        s8 *plist;
        s32 slen;
        int write_back = 1;

        plist = pVal;
        slen = strlen(pVal);

        /** Compare each MAC address in the file with all the
          * input MAC addresses */
        write_back = 1;
        while(slen > 0) {

            if(0 == strncmp(buf, plist, MAC_ADDR_LEN)) {
                write_back = 0;
                break;
            }

            while((*plist != ' ') && (*plist != '\0')) {
                plist++;
                slen--;
            }

            while(((*plist == ' ') || (*plist == '\t')) && (*plist != '\0')) {
                plist++; slen--;
            }
        }

        /** Update the file */
        if(write_back) {
            fprintf(ftmp, "%s", buf);
        }
    }

    fclose(fp);
    fclose(ftmp);

    qsap_scnprintf(buf, sizeof(buf), "%s~", pfile);

    /** Restore the configuration file */
    status = rename(buf, pfile);

    qsap_scnprintf(presp, *plen, "%s", (status == eERR_UNKNOWN) ? ERR_FEATURE_NOT_ENABLED : SUCCESS);

    unlink(buf);

    return;
}

/**
 * @brief
 *         Identify the MAC list file and the type of updation on the file.
 *         The MAC list file can be : Allow file or Deny file.
 *         The type of operation is : Add to file or Delete from file
 *
 * @param file [IN] path of the allow or deny MAC list file.
 * @param cNum [IN] command number to 'type of file' and the 'type of updation'
 *                  to be done.
 * @param pVal [IN] A list of one or more MAC addresses. Multiple MAC addresses
 *                  are separated by a SPACE character
 * @param presp [OUT] Buffer to store the command response
 * @param plen [IN-OUT] The length of the 'presp' buffer is provided as input
 *                      The length of the response, stored in the 'presp' is provided
 *                      as the output
 * @return void
*/
static void qsap_update_mac_list(s8 *pfile, esap_cmd_t cNum, s8 *pVal, s8 *presp, u32 *plen)
{
    ALOGD("%s : Updating file %s \n", __func__, pfile);

    switch(cNum) {
        case eCMD_ADD_TO_ALLOW:
        case eCMD_ADD_TO_DENY:
                qsap_add_mac_to_file(pfile, pVal, presp, plen);
                break;

        case eCMD_REMOVE_FROM_ALLOW:
        case eCMD_REMOVE_FROM_DENY:
                qsap_remove_from_file(pfile, pVal, presp, plen);
                break;

        default:
                *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);
                return;
    }

    return;
}

/**
 * @brief
 * @param fconfig [INPUT] configuration file name
 * @param cNum [INPUT] command number. The valid command numbers supported by
 *                     this function are :
 *                     eCMD_ALLOW_LIST - Get the MAC address list from the allow list
 *                     eCMD_DENY_LIST - Get the MAC address list from the deny list
 * @param presp [OUTPUT] presp The command output format :
 *                    On success,
 *                            success <cmd>=<value>
 *                    On failure,
 *                            failure <error message>
 * @param plen [IN-OUT] plen
 *                      [IN] The length of the buffer, presp
 *                      [OUT] The length of the response in the buffer, presp
 * @return void
**/
static void qsap_get_mac_list(s8 *fconfile, esap_cmd_t cNum, s8 *presp, u32 *plen)
{
    s8 buf[MAX_CONF_LINE_LEN];
    FILE *fp;
    u32 len_remain;
    s8 *pfile, *pOut;
    esap_str_t sNum;
    int cnt = 0;

    /** Identify the allow or deny file */
    if(eCMD_ALLOW_LIST == cNum) {
        sNum = STR_ACCEPT_MAC_FILE;
    }
    else if(eCMD_DENY_LIST == cNum) {
        sNum = STR_DENY_MAC_FILE;
    }
    else {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);
        return;
    }

    /** Get the MAC allow or MAC deny file path */
    len_remain = MAX_CONF_LINE_LEN;
    if(NULL == (pfile = qsap_get_allow_deny_file_name(fconfile, &qsap_str[sNum], buf, &len_remain))) {
        ALOGE("%s:Unknown error\n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        return;
    }

    /** Open allow / deny file, and read the MAC addresses */
    fp = fopen(pfile, "r");
    if(NULL == fp) {
        ALOGE("%s: file open error\n",__func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
        return;
    }

    *plen -= qsap_scnprintf(presp, *plen, "%s %s=", SUCCESS, cmd_list[cNum].name);

    pOut = presp + strlen(presp);

    /** Read the MAC address from the MAC allow or deny file */
    while(NULL != (fgets(buf, MAX_CONF_LINE_LEN, fp))) {
        u32 len;

        /** Avoid the commented lines */
        if(buf[0] == '#')
            continue;

        if(FALSE == isValid_MAC_address(buf))
            continue;

        buf[strlen(buf)-1] = '\0';

        if(*plen < strlen(buf)) {
            *pOut = '\0';
            break;
        }

        len = qsap_scnprintf(pOut, *plen, "%s ", buf);
        cnt++;

        if (cnt >= MAX_ALLOWED_MAC) {
            break;
        }

        pOut += len;
        *plen -= len;
    }

    *plen = strlen(presp);

    fclose(fp);

    return;
}

static int qsap_read_mac_address(s8 *presp, u32 *plen)
{
    char *ptr;
    char  mac[MAC_ADDR_LEN];
    u32   len, i;
    int   nRet = eERR_INVALID_MAC_ADDR;

    len = *plen;

    if(eSUCCESS != qsap_read_cfg(fIni, &qsap_str[STR_MAC_IN_INI], presp, plen, cmd_list[eCMD_MAC_ADDR].name, GET_ENABLED_ONLY)) {
        ALOGE("%s :MAC addr read failure \n",__func__);
        goto end;
    }

    ptr = strchr(presp, '=');
    if(NULL == ptr)
        goto end;

    strlcpy(mac, ptr+1, MAC_ADDR_LEN);
    *plen = qsap_scnprintf(presp, len, "%s %s=", SUCCESS, cmd_list[eCMD_MAC_ADDR].name);
    ptr = presp + strlen(presp);
    len -= strlen(presp); /* decrease the total available buf length */

    for(i=0; i<MAC_ADDR_LEN-5; i+=2) {
        u32 tlen;

        tlen = qsap_scnprintf(ptr, len, "%c%c:", mac[i], mac[i+1]);
        *plen += tlen;
        ptr += tlen;
        len -= tlen; /* decrease the available len of ptr */
    }
    presp[*plen-1] = '\0';
    (*plen)--;

    ptr = strchr(presp, '=');
    if(NULL == ptr)
        goto end;

    ptr++;

    ALOGD("MAC :%s \n", ptr);
    if(TRUE == isValid_MAC_address(ptr)) {
        nRet = eSUCCESS;
    }
    else {
        ALOGE("Invalid MAC in conf file \n");
    }
end:
    return nRet;
}

s8 *qsap_get_config_value(s8 *pfile, struct Command  *pcmd, s8 *pbuf, u32 *plen)
{
    s8 *ptr = NULL;

    if(eSUCCESS == qsap_read_cfg(pfile, pcmd, pbuf, (u32 *)plen, NULL, GET_ENABLED_ONLY)) {
        ptr = strchr(pbuf, '=');
        if(NULL != ptr){
            ptr++;
        }
        else {
            ALOGE("Invalid entry, %s\n", pcmd->name);
        }
    }

    return ptr;
}

static void qsap_read_wps_state(s8 *presp, u32 *plen)
{
    u32  tlen = *plen;
    s32  status;
    s8 *pstate;

    if(NULL == (pstate = qsap_get_config_value(pconffile, &cmd_list[eCMD_WPS_STATE], presp, &tlen))) {
        /** unable to read the wps configuration, WPS is disabled !*/
        ALOGD("%s :wps_state not in cfg file \n", __func__);
        status = DISABLE;
    }
    else {
        status = (atoi(pstate) == WPS_STATE_ENABLE) ? ENABLE : DISABLE;
    }

    *plen = qsap_scnprintf(presp, *plen, "success %s=%ld", cmd_list[eCMD_WPS_STATE].name, status);

    return;
}

/**
 *    Get the channel being used in the soft AP.
 */
int qsap_get_operating_channel(s32 *pchan)
{
    int sock;
    struct iwreq wrq;
    s8 interface[MAX_CONF_LINE_LEN];
    u32 len = MAX_CONF_LINE_LEN;
    s8 *pif;
    int ret;

    if(ENABLE != is_softap_enabled()) {
        goto error;
    }

    if(NULL == (pif = qsap_get_config_value(pconffile, &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGE("%s :interface error \n", __func__);
        goto error;
    }

    interface[len] = '\0';

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        ALOGE("%s :socket error \n", __func__);
        goto error;
    }

    *pchan = 0;

    strlcpy(wrq.ifr_name, pif, sizeof(wrq.ifr_name));
    wrq.u.data.length = sizeof(s32);
    wrq.u.data.pointer = pchan;
    wrq.u.data.flags = 0;

    ret = ioctl(sock, QCSAP_IOCTL_GET_CHANNEL, &wrq);
    if(ret < 0) {
        ALOGE("%s: ioctl failure \n",__func__);
        close(sock);
        goto error;
    }

    ALOGE("Recv len :%d \n", wrq.u.data.length);
    *pchan = *(int *)(&wrq.u.name[0]);
    ALOGE("Operating channel :%d \n", (int)*pchan);
    close(sock);
    return eSUCCESS;

error:
    *pchan = 0;
    ALOGE("%s: Failed to read channel \n", __func__);
    return eERR_CHAN_READ;
}

/**
 *    Get the sap auto channel selection for soft AP.
 */
int qsap_get_sap_auto_channel_selection(s32 *pautochan)
{
    int sock;
    struct iwreq wrq;
    s8 interface[MAX_CONF_LINE_LEN];
    u32 len = MAX_CONF_LINE_LEN;
    s8 *pif;
    int ret;
    sap_auto_channel_info sap_autochan_info;

    if(ENABLE != is_softap_enabled()) {
        ALOGE("%s :is_softap_enabled() goto error \n", __func__);
        goto error;
    }

    if(NULL == (pif = qsap_get_config_value(pconffile,
                                 &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGD("%s :interface error \n", __func__);
        goto error;
    }

    interface[len] = '\0';

     sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        ALOGD("%s :socket error \n", __func__);
        goto error;
    }

    *pautochan = 0;

    memset(&sap_autochan_info, 0, sizeof(sap_autochan_info));
    memset(&wrq, 0, sizeof(wrq));

    strlcpy(wrq.ifr_name, pif, sizeof(wrq.ifr_name));
    wrq.u.data.length = sizeof(s32);
    sap_autochan_info.subioctl = QCSAP_PARAM_GET_AUTO_CHANNEL;
    wrq.u.data.pointer = pautochan;
    memcpy(wrq.u.name, (char *)(&sap_autochan_info), sizeof(sap_autochan_info));
    wrq.u.data.flags = 0;

    ret = ioctl(sock, QCSAP_IOCTL_GETPARAM, &wrq);
    if(ret < 0) {
        ALOGE("%s: ioctl failure \n",__func__);
        close(sock);
        goto error;
    }

    ALOGD("Recv len :%d \n", wrq.u.data.length);
    *pautochan = *(int *)(&wrq.u.name[0]);
    ALOGD("Sap auto channel selection pautochan=%d \n", (int)*pautochan);
    close(sock);
    return eSUCCESS;

error:
    *pautochan = 0;
    ALOGE("%s: Failed to read sap auto channel selection\n", __func__);
    return eERR_GET_AUTO_CHAN;
}


static int iftypeCallback(struct nl_msg* msg, void* arg)
{
    struct nlmsghdr* ret_hdr = nlmsg_hdr(msg);
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    int* type = arg;

    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(ret_hdr);

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (tb_msg[NL80211_ATTR_IFTYPE]) {
        *type = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);

    }
    return NL_SKIP;
}

/**
 *    Get the mode of operation.
 */
int qsap_get_mode(s32 *pmode)
{
    int ret = eERR_UNKNOWN;
    struct nl_sock* sk = NULL;
    int nl80211_id = -1;
    int if_type = -1;
    s8 interface[MAX_CONF_LINE_LEN];
    u32 len = MAX_CONF_LINE_LEN;
    s8 *pif;
    struct nl_msg* msg = NULL;

    //get interface name
    if(NULL == (pif = qsap_get_config_value(pconffile,
                                 &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGD("%s :interface error \n", __func__);
        goto nla_put_failure;
    }

    interface[len] = '\0';

    //allocate socket
    sk = nl_socket_alloc();

    //return if socket allocation fails
    if(sk == NULL){
       ALOGE( "socket allocation failure");
       return ret;
    }

    //connect to generic netlink
    if (genl_connect(sk)) {
        ALOGE( "Netlink socket Connection failure");
        ret = eERR_UNKNOWN;
        goto nla_put_failure;
    }

    //find the nl80211 driver ID
    nl80211_id = genl_ctrl_resolve(sk, "nl80211");

    //attach a callback
    nl_socket_modify_cb(sk, NL_CB_VALID, NL_CB_CUSTOM,
            iftypeCallback, &if_type);

    //allocate a message
    msg = nlmsg_alloc();

    //return if message allocation fails
    if(msg == NULL){
       ALOGE( "message allocation failure");
       goto nla_put_failure;
    }

    // setup the message
    genlmsg_put(msg, 0, 0, nl80211_id, 0, 0, NL80211_CMD_GET_INTERFACE, 0);

    //add message attributes
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, if_nametoindex(pif));

    // Send the message
    ret = nl_send_auto_complete(sk, msg);
    if (ret < 0 ) {
        ALOGE( "nl_send_auto_complete failure");
        ret = eERR_UNKNOWN;
        goto nla_put_failure;
    }

    //block for message to return
    nl_recvmsgs_default(sk);
    *pmode = if_type;
    ALOGI("%s: (%s) NL80211 Get Mode = %d \n",__func__, pif, (int)*pmode);
    ret = eSUCCESS;

nla_put_failure:
    if (sk)
        nl_socket_free(sk);
    if (msg)
        nlmsg_free(msg);
    return ret;
}

/**
 *    Set the channel Range for soft AP.
 */
int qsap_set_channel_range(s8 *buf)
{
    int sock;
    struct iwreq wrq;
    s8 interface[MAX_CONF_LINE_LEN];
    u32 len = MAX_CONF_LINE_LEN;
    s8 *pif;
    s8 *temp;
    int ret;
    sap_channel_info sap_chan_range;
    sta_channel_info sta_chan_range;

    ALOGE("buf :%s\n", buf);

    temp = buf;
    temp = strchr(buf, '=');
    if (NULL == temp) {
        goto error;
    }
    temp++;

    if (NULL == (pif = qsap_get_config_value(pconffile,
                    &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGE("%s :interface error\n", __func__);
        goto error;
    }

    interface[len] = '\0';

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ALOGE("%s :socket error\n", __func__);
        goto error;
    }

    memset(&sap_chan_range, 0, sizeof(sap_chan_range));
    memset(&sta_chan_range, 0, sizeof(sta_chan_range));
    memset(&wrq, 0, sizeof(wrq));

    if (ENABLE != is_softap_enabled()) {
        strlcpy(wrq.ifr_name, "wlan0", sizeof(wrq.ifr_name));
        sta_chan_range.subioctl = WE_SET_SAP_CHANNELS;
        ret = sscanf(temp, "%d %d %d", &(sta_chan_range.stastartchan),
                &(sta_chan_range.staendchan), &(sta_chan_range.staband));
        if (3 != ret) {
            ALOGE("%s : sscanf is not successful\n", __func__);
            close(sock);
            goto error;
        }
        memcpy(wrq.u.name, (char *)(&sta_chan_range), sizeof(sta_chan_range));

        ALOGE("%s :Softap is off,Send SET_CHANNEL_RANGE over sta interface\n",
                __func__);
        ret = ioctl(sock, WLAN_PRIV_SET_THREE_INT_GET_NONE, &wrq);
    } else {
          strlcpy(wrq.ifr_name, pif, sizeof(wrq.ifr_name));
          ret = sscanf(temp, "%d %d %d", &(sap_chan_range.startchan),
                  &(sap_chan_range.endchan), &(sap_chan_range.band));
          if (3 != ret) {
              ALOGE("%s : sscanf is not successful\n", __func__);
              close(sock);
              goto error;
          }
          memcpy(wrq.u.name, (char *)(&sap_chan_range), sizeof(sap_chan_range));

          ALOGE("%s :SAP is on,Send SET_CHANNEL_RANGE over softap interface\n",
                    __func__);
          ret = ioctl(sock, QCSAP_IOCTL_SET_CHANNEL_RANGE, &wrq);
    }

    if (ret < 0) {
        ALOGE("%s: ioctl failure\n", __func__);
        close(sock);
        goto error;
    }

    ALOGE("Recv len :%d\n", wrq.u.data.length);

    close(sock);
    return eSUCCESS;

error:
    ALOGE("%s: Failed to set channel range\n", __func__);
    return eERR_SET_CHAN_RANGE;
}

int qsap_read_channel(s8 *pfile, struct Command *pcmd, s8 *presp, u32 *plen, s8 *pvar)
{
    s32  chan;
    u32  len = *plen;

   if(eSUCCESS == qsap_get_operating_channel(&chan)) {
            *plen = qsap_scnprintf(presp, len, "%s %s=%lu", SUCCESS, pcmd->name, chan);
             ALOGD("presp :%s\n", presp);
   } else {
          *plen = qsap_scnprintf(presp, len, "%s", ERR_UNKNOWN);
   }

    return eSUCCESS;
}

int qsap_read_auto_channel(struct Command *pcmd, s8 *presp, u32 *plen)
{
    s8   *pval, *pautoval;
    s32  pautochan;
    u32  len = *plen;

    ALOGE("%s :\n", __func__);

    if (eSUCCESS == qsap_get_sap_auto_channel_selection(&pautochan)) {
          *plen = qsap_scnprintf(presp, len, "%s autochannel=%lu", SUCCESS, pautochan);
          ALOGE("presp :%s\n", presp);
    } else {
          *plen = qsap_scnprintf(presp, len, "%s", ERR_UNKNOWN);
    }

    return eSUCCESS;
}

static int qsap_mac_to_macstr(s8 *pmac, u32 slen, s8 *pmstr, u32 *plen)
{
    int len;
    int totlen = 0;

    while((slen > 0) && (*plen > 0)) {
        len = qsap_scnprintf(pmstr, *plen, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X ", (int)pmac[0], (int)pmac[1], (int)pmac[2],
                            (int)pmac[3], (int)pmac[4], (int)pmac[5]);
        pmac += 6;
        slen -= 6;
        *plen -= len;
        pmstr += len;
        totlen += len;
    }

    if(totlen > 0) {
        *pmstr--;
        totlen--;
    }
    *pmstr = '\0';
    *plen = totlen;

    return 0;
}

#define MAX_STA_ALLOWED  8
void qsap_get_associated_sta_mac(s8 *presp, u32 *plen)
{
    int sock, ret;
    struct iwreq wrq;
    s8 interface[MAX_CONF_LINE_LEN];
    u32 len = MAX_CONF_LINE_LEN;
    s8 *pif;
    s8 *pbuf, *pout;
    u32 buflen;
    u32 recvLen;
    u32 tlen;

    if(ENABLE != is_softap_enabled()) {
        goto error;
    }

    if(NULL == (pif = qsap_get_config_value(pconffile, &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGE("%s :interface error \n", __func__);
        goto error;
    }
    interface[len] = '\0';

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        ALOGE("%s :socket failure \n", __func__);
        goto error;
    }

    /* response has length field + 6 bytes per STA */
    buflen = sizeof(u32) + (MAX_STA_ALLOWED * 6);
    pbuf = (s8 *)malloc(buflen);
    if(NULL == pbuf) {
        ALOGE("%s :No memory \n", __func__);
        close(sock);
        goto error;
    }


#define SIZE_OF_MAC_INT   (6)
    strlcpy(wrq.ifr_name, pif, sizeof(wrq.ifr_name));
    wrq.u.data.length = buflen;
    wrq.u.data.pointer = (void *)pbuf;
    wrq.u.data.flags = 0;

    ret = ioctl(sock, QCSAP_IOCTL_ASSOC_STA_MACADDR, &wrq);
    if(ret < 0) {
        ALOGE("%s :ioctl failure \n", __func__);
        free(pbuf);
        close(sock);
        goto error;
    }

    recvLen = *(u32 *)(wrq.u.data.pointer);
    recvLen -= sizeof(u32);

    len = qsap_scnprintf(presp, *plen, "%s %s=", SUCCESS, cmd_list[eCMD_ASSOC_STA_MACS].name);
    pout = presp + len;
    tlen = *plen - len;

    qsap_mac_to_macstr(pbuf+sizeof(u32), recvLen, pout, &tlen);

    *plen = len + tlen;

    free(pbuf);
    close(sock);

    return;
error:
    *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);

    return;
}

static void qsap_read_wep_key(s8 *pfile, struct Command *pcmd, s8 *presp, u32 *plen, s8 *var)
{
    s8 *pwep;
    s8 *pkey;

    if(eSUCCESS != qsap_read_cfg(pfile, pcmd, presp, plen, var, GET_COMMENTED_VALUE))
        return;

    pwep = strchr(presp, '=');
    if(NULL == pwep)
        return;
    pwep++;

    if(pwep[0] == '"') {
        pkey = pwep;
        pwep++;

        while(*pwep != '\0') {
            *pkey = *pwep;
             pkey++;
             pwep++;
        }
        *pkey--;
        *pkey = '\0';
        *plen -= 2;
    }

    return;
}

void qsap_read_ap_stats(s8 *presp, u32 *plen)
{
    int sock, ret;
    struct iwreq wrq;
    s8 interface[MAX_CONF_LINE_LEN];
    u32 len = MAX_CONF_LINE_LEN;
    s8 *pif;
    s8 *pbuf;

    if(ENABLE != is_softap_enabled()) {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_SOFTAP_NOT_STARTED);
        return;
    }

    if(NULL == (pif = qsap_get_config_value(pconffile, &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGE("%s :interface error \n", __func__);
        goto error;
    }
    interface[len] = '\0';

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        ALOGE("%s :socket failure \n", __func__);
        goto error;
    }

    pbuf = (s8 *)malloc(MAX_RESP_LEN);
    if(NULL == pbuf) {
        ALOGE("%s :No memory \n", __func__);
        close(sock);
        goto error;
    }

    strlcpy(wrq.ifr_name, pif, sizeof(wrq.ifr_name));
    wrq.u.data.length = MAX_RESP_LEN;
    wrq.u.data.pointer = (void *)pbuf;
    wrq.u.data.flags = 0;

    ret = ioctl(sock, QCSAP_IOCTL_AP_STATS, &wrq);
    if(ret < 0) {
        ALOGE("%s :ioctl failure \n", __func__);
        free(pbuf);
        close(sock);
        goto error;
    }

    *plen = qsap_scnprintf(presp, *plen, "%s %s=%s", SUCCESS, cmd_list[eCMD_AP_STATISTICS].name, pbuf);

    free(pbuf);
    close(sock);
    return;

error:
    *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);

    return;
}

void qsap_read_autoshutoff(s8 *presp, u32 *plen)
{
    u32 tlen, time = 0;
    s8 *ptime;

    tlen = *plen;

    if(NULL == (ptime = qsap_get_config_value(fIni, &qsap_str[STR_AP_AUTOSHUTOFF], presp, &tlen))) {
        /** unable to read the AP shutoff time */
        ALOGE("%s :Failed to read AP shutoff time\n", __func__);
    }
    else {
        time = atoi(ptime);
        time = time / 60; /** Convert seconds to minutes */
    }

    *plen = qsap_scnprintf(presp, *plen, "success %s=%ld", cmd_list[eCMD_AP_AUTOSHUTOFF].name, time);

    return;
}


/**
 * @brief
 *       Get the configuration information from the softAP configuration
 *       files
 * @param cNum [INPUT]
 * @param presp [OUTPUT] presp The command output format :
 *                    On success,
 *                            success <cmd>=<value>
 *                    On failure,
 *                            failure <error message>
 * @param plen [IN-OUT] plen
 *                      [IN] The length of the buffer, presp
 *                      [OUT] The length of the response in the buffer, presp
 * @return void
**/
static void qsap_get_from_config(esap_cmd_t cNum, s8 *presp, u32 *plen)
{
    u32 len;
    int status;

    switch(cNum) {
        case eCMD_ENABLE_SOFTAP:
            status = is_softap_enabled();
            *plen = qsap_scnprintf(presp, *plen, "%s %s=%d", SUCCESS, cmd_list[cNum].name, status);
            break;

        case eCMD_WPA_PAIRWISE:
        case eCMD_RSN_PAIRWISE:
        case eCMD_DEFAULT_KEY:
        case eCMD_PASSPHRASE:
        case eCMD_GTK_TIMEOUT:
                qsap_read_cfg(pconffile, &cmd_list[cNum], presp, plen, NULL, GET_COMMENTED_VALUE);
            break;

        case eCMD_SSID:
        case eCMD_BSSID:
        case eCMD_BCN_INTERVAL:
        case eCMD_DTIM_PERIOD:
        case eCMD_HW_MODE:
        case eCMD_AUTH_ALGS:
        case eCMD_MAC_ACL:
        case eCMD_WPS_CONFIG_METHOD:
        case eCMD_UUID:
        case eCMD_DEVICE_NAME:
        case eCMD_MANUFACTURER:
        case eCMD_MODEL_NAME:
        case eCMD_MODEL_NUMBER:
        case eCMD_SERIAL_NUMBER:
        case eCMD_DEVICE_TYPE:
        case eCMD_OS_VERSION:
        case eCMD_FRIENDLY_NAME:
        case eCMD_MANUFACTURER_URL:
        case eCMD_MODEL_DESC:
        case eCMD_MODEL_URL:
        case eCMD_UPC:
        case eCMD_SDK_VERSION:
        case eCMD_COUNTRY_CODE:
                qsap_read_cfg(pconffile, &cmd_list[cNum], presp, plen, NULL, GET_ENABLED_ONLY);
                break;

        case eCMD_WEP_KEY0:
        case eCMD_WEP_KEY1:
        case eCMD_WEP_KEY2:
        case eCMD_WEP_KEY3:
                qsap_read_wep_key(pconffile, &cmd_list[cNum], presp, plen, NULL);
                break;

        case eCMD_CHAN:
                qsap_read_channel(pconffile, &cmd_list[cNum], presp, plen, NULL);
                break;

        case eCMD_FRAG_THRESHOLD:
        case eCMD_REGULATORY_DOMAIN:
        case eCMD_RTS_THRESHOLD:
        case eCMD_IEEE80211H:
                qsap_read_cfg(pconffile, &cmd_list[cNum], presp, plen, NULL, GET_ENABLED_ONLY);
                break;

        case eCMD_ALLOW_LIST: /* fall through */
        case eCMD_DENY_LIST:
                qsap_get_mac_list(pconffile, cNum, presp, plen);
                break;

        case eCMD_SEC_MODE:
                qsap_read_security_mode(pconffile, presp, plen);
                break;

        case eCMD_MAC_ADDR:
                if(eSUCCESS != qsap_read_mac_address(presp, plen)) {
                    *plen = qsap_scnprintf(presp, *plen, "%s", ERR_NOT_SUPPORTED);
                }
                break;

        case eCMD_WMM_STATE:
                qsap_read_cfg(pconffile, &cmd_list[cNum], presp, plen, NULL, GET_ENABLED_ONLY);
                break;

        case eCMD_WPS_STATE:
                qsap_read_wps_state(presp, plen);
                break;

        case eCMD_PROTECTION_FLAG:
                qsap_read_cfg(fIni, &qsap_str[STR_PROT_FLAG_IN_INI], presp, plen, cmd_list[eCMD_PROTECTION_FLAG].name, GET_ENABLED_ONLY);
                break;

        case eCMD_DATA_RATES:
                qsap_read_cfg(fIni, &qsap_str[STR_DATA_RATE_IN_INI], presp, plen, cmd_list[eCMD_DATA_RATES].name, GET_ENABLED_ONLY);
                break;

        case eCMD_ASSOC_STA_MACS:
                qsap_get_associated_sta_mac(presp, plen);
                break;

        case eCMD_TX_POWER:
                qsap_read_cfg(fIni, &qsap_str[STR_TX_POWER_IN_INI], presp, plen, cmd_list[eCMD_TX_POWER].name, GET_ENABLED_ONLY);
                break;

        case eCMD_INTRA_BSS_FORWARD:
                qsap_read_cfg(pconffile, &cmd_list[eCMD_INTRA_BSS_FORWARD], presp, plen, NULL, GET_ENABLED_ONLY);
                break;

        case eCMD_AP_STATISTICS:
                qsap_read_ap_stats(presp, plen);
                break;

        case eCMD_AP_AUTOSHUTOFF:
            qsap_read_autoshutoff(presp, plen);
            break;

        case eCMD_AP_ENERGY_DETECT_TH:
                qsap_read_cfg(fIni, &qsap_str[STR_AP_ENERGY_DETECT_TH], presp, plen, cmd_list[eCMD_AP_ENERGY_DETECT_TH].name, GET_ENABLED_ONLY);
                break;
        case eCMD_GET_AUTO_CHANNEL:
                qsap_read_auto_channel(&cmd_list[cNum],presp, plen);
               break;
        default:
            /** Error case */
            *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_ARG);
    }

    len = *plen-1;

    /** Remove the space or tabs in the end of response */
    while(len) {
        if((presp[len] == ' ') || (presp[len] == '\t'))
            len--;
        else
            break;
    }
    presp[len+1] = '\0';
    *plen = len+1;

    return;
}

/**
 * @brief
 *        Identify the command number corresponding to the input user command.
 * @param cName [INPUT] command name
 * @return
 *             On success,
 *                     command number in the range 0 to (eCMD_INVALID-1)
 *          On failure,
 *                  eCMD_INVALID
**/
static esap_cmd_t qsap_get_cmd_num(s8 *cName)
{
    s16 i, len;

    for(i=0; i<eCMD_LAST; i++)     {
        len = strlen(cmd_list[i].name);

        if(!strncmp(cmd_list[i].name, cName, len)) {
            if((cName[len] == '=') || (cName[len] == '\0'))
                return i;
        }
    }
    return eCMD_INVALID;
}

/**
 * @brief
 *            Handle the user requests of the form,
 *                "get <cmd num> [<value1> ...]"
 *           These commands are used to retreive the soft AP
 *           configuration information
 *
 * @param pcmd [IN] pointer to the structure, storing the command.
 * @param presp [OUT] pointer to the buffer, to store the command response.
 *                         The command output format :
 *                    On success,
 *                            success <cmd>=<value>
 *                    On failure,
 *                            failure <error message>
 * @param plen [IN-OUT]
 *                 [IN] : Maximum length of the reponse buffer
 *                [OUT]: Reponse length
 * @return
 *         void
*/
static void qsap_handle_get_request(s8 *pcmd, s8 *presp, u32 *plen)
{
    esap_cmd_t cNum;

    pcmd += strlen("get");

    SKIP_BLANK_SPACE(pcmd);

    cNum = qsap_get_cmd_num(pcmd);

    if(cNum == eCMD_INVALID) {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_PARAM);
        return;
    }

    qsap_get_from_config(cNum, presp, plen);

    return;
}

static s16 is_valid_wep_key(s8 *pwep, s8 *pkey, s16 len)
{
    int weplen;
    s16 ret = TRUE;
    int ascii = FALSE;

    weplen = strlen(pwep);

    /** Remove the double quotes if any */
    if((pwep[0] == '"') && (pwep[weplen-1] == '"')) {
        pwep[weplen-1] = '\0';
        pwep++;
        weplen -= 2;
    }

    /** The WEP key should be of length 5, 13 or 16 characters
      * or 10, 26, or 32 digits */
    switch(weplen) {
        case WEP_64_KEY_ASCII:
        case WEP_128_KEY_ASCII:
        case WEP_152_KEY_ASCII:
                weplen--;
                while(weplen--) {
                    if(0 == isascii(pwep[weplen])) {
                        ALOGD("%c not ascii \n", pwep[weplen]);
                        return FALSE;
                    }
                }
                ascii = TRUE;
                break;

        case WEP_64_KEY_HEX:
        case WEP_128_KEY_HEX:
        case WEP_152_KEY_HEX:
                while(weplen--) {
                    if(0 == isxdigit(pwep[weplen]))
                        return FALSE;
                }
                break;

        default:
            ret = FALSE;
    }

    qsap_scnprintf(pkey, len, (ascii == TRUE) ? "\"%s\"" : "%s", pwep);

    return ret;
}

s16 wifi_qsap_reset_to_default(s8 *pcfgfile, s8 *pdefault)
{
    FILE *fcfg, *ftmp;
    char buf[MAX_CONF_LINE_LEN];
    int status = eSUCCESS;

    fcfg = fopen(pdefault, "r");

    if(NULL == fcfg) {
        ALOGE("%s : unable to open file \n", __func__);
        return eERR_FILE_OPEN;
    }

    qsap_scnprintf(buf, sizeof(buf), "%s~", pcfgfile);

    ftmp = fopen(buf, "w+");
    if(NULL == ftmp) {
        ALOGE("%s : unable to open file \n", __func__);
        fclose(fcfg);
        return eERR_FILE_OPEN;
    }

    while(NULL != fgets(buf, MAX_CONF_LINE_LEN, fcfg)) {
        fprintf(ftmp, "%s", buf);
    }

    fclose(fcfg);
    fclose(ftmp);

    qsap_scnprintf(buf, sizeof(buf), "%s~", pcfgfile);

    if(eERR_UNKNOWN == rename(buf, pcfgfile))
        status = eERR_CONF_FILE;

    /** Remove the temporary file. Dont care the return value */
    unlink(buf);

    return status;
}

#define CTRL_IFACE_PATH_LEN   (128)

void qsap_del_ctrl_iface(void)
{
    u32 len;
    s8 dst_path[CTRL_IFACE_PATH_LEN], *pcif, *pif;
    s8 interface[64];
    s8 path[CTRL_IFACE_PATH_LEN + 64];

    len = CTRL_IFACE_PATH_LEN;

    if(NULL == (pcif = qsap_get_config_value(pconffile, &qsap_str[STR_CTRL_INTERFACE], dst_path, &len))) {
        ALOGE("%s :ctrl_iface path error \n", __func__);
        goto error;
    }

    len = 64;

    if(NULL == (pif = qsap_get_config_value(pconffile, &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGE("%s :interface error \n", __func__);
        goto error;
    }

    if ((int)sizeof(path) <= snprintf(path, sizeof(path), "%s/%s", pcif, pif)) {
        ALOGE("Iface path : truncating error, %s \n", path);
        goto error;
    }

    unlink(path);

error:
    return;
}

static int qsap_send_cmd_to_hostapd(s8 *pcmd)
{
    int sock;
    struct sockaddr_un cli;
    struct sockaddr_un ser;
    struct timeval timeout;
    int ret = eERR_SEND_TO_HOSTAPD;
    u32 len;
    fd_set read;
    s8 dst_path[CTRL_IFACE_PATH_LEN], *pcif, *pif;
    s8 interface[64];
    s8 *ptr;
    u32 retry_cnt = 3;
    size_t ptr_size;

    len = CTRL_IFACE_PATH_LEN;
    ptr_size = (sizeof(ser.sun_path) < CTRL_IFACE_PATH_LEN)?
        CTRL_IFACE_PATH_LEN : sizeof(ser.sun_path);
    ptr = malloc(ptr_size);
    if(NULL == ptr) {
        ALOGE("%s :No memory \n", __func__);
        return ret;
    }

    if(NULL == (pcif = qsap_get_config_value(pconffile, &qsap_str[STR_CTRL_INTERFACE], dst_path, &len))) {
        ALOGE("%s :ctrl_iface path error \n", __func__);
        goto error;
    }

    len = 64;

    if(NULL == (pif = qsap_get_config_value(pconffile, &qsap_str[STR_INTERFACE], interface, &len))) {
        ALOGE("%s :interface error \n", __func__);
        goto error;
    }

    if ((int)sizeof(ser.sun_path) <= snprintf(ptr, sizeof(ser.sun_path), "%s/%s", pcif, pif)) {
        /* the sun_path is truncated. */
        ALOGE("Iface path : truncating error, %s \n", ptr);
        goto error;
    }

    ALOGD("Connect to :%s\n", ptr);

    sock = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(sock < 0) {
        ALOGE("%s :Socket error \n", __func__);
        goto error;
    }

    cli.sun_family = AF_UNIX;
    qsap_scnprintf(cli.sun_path, sizeof(cli.sun_path), SDK_CTRL_IF);

    ret = bind(sock, (struct sockaddr *)&cli, sizeof(cli));

    if(ret < 0) {
        ALOGE("Bind Failure\n");
        goto close_ret;
    }

    ser.sun_family = AF_UNIX;
    qsap_scnprintf(ser.sun_path, sizeof(ser.sun_path), "%s", ptr);
    ALOGD("Connect to: %s,(%d)\n", ser.sun_path, sock);

    ret = connect(sock, (struct sockaddr *)&ser, sizeof(ser));
    if(ret < 0) {
        ALOGE("Connect Failure...\n");
        goto close_ret;
    }

    ret = send(sock, pcmd, strlen(pcmd), 0);
    if(ret < 0) {
        ALOGE("Unable to send cmd to hostapd \n");
        goto close_ret;
    }
    /* reset the ptr buffer size for recv */
    len = ptr_size;

#define HOSTAPD_RECV_TIMEOUT    (2)
    while(1) {
        timeout.tv_sec = HOSTAPD_RECV_TIMEOUT;
        timeout.tv_usec = 0;

        FD_ZERO(&read);
        FD_SET(sock, &read);

        ret = select(sock+1, &read, NULL, NULL, &timeout);

        if(FD_ISSET(sock, &read)) {

            ret = recv(sock, ptr, len, 0);

            if(ret < 0) {
                ALOGE("%s: recv() failed \n", __func__);
                goto close_ret;
            }

            if ((u32)ret >= len)
                ptr[len-1] = 0;
            else
                ptr[ret] = 0;

            if((ret > 0) && (ptr[0] == '<')) {
                ALOGE("Not the expected response...\n: %s", ptr);
                retry_cnt--;
                if(retry_cnt)
                    continue;
                break;
            }

            if(!strncmp(ptr, "FAIL", 4)) {
                ALOGE("Command failed in hostapd \n");
                goto close_ret;
            }
            else {
                break;
            }
        }
        else {
            ALOGE("%s: Select failed \n", __func__);
            goto close_ret;
        }
    }

    ret = eSUCCESS;

close_ret:
    close(sock);

error:
    free(ptr);
    unlink(SDK_CTRL_IF);
    return ret;
}

static void qsap_update_wps_config(s8 *pVal, s8 *presp, u32 *plen)
{
    u32 tlen = *plen;
    s32 status;
    s8  pwps_state[MAX_INT_STR+1];
    s32 i;

    /* Enable/disable the following in hostapd.conf
     * 1. Update the wps_state
     * 2. Set eap_server=1
     * 3. Update UPnP related variables
     */
    status = atoi(pVal);

    qsap_scnprintf(pwps_state, sizeof(pwps_state), "%d", (status == ENABLE) ? WPS_STATE_ENABLE : WPS_STATE_DISABLE);

    qsap_write_cfg(pconffile, &cmd_list[eCMD_WPS_STATE], pwps_state, presp, &tlen, HOSTAPD_CONF_QCOM_FILE);

    if(eERR_UNKNOWN == qsap_change_cfg(pconffile, &cmd_list[eCMD_WPS_STATE], status)) {
        ALOGE("%s: Failed to enable %s\n", __func__, cmd_list[eCMD_WPS_STATE].name);
        goto error;
    }

    qsap_scnprintf(pwps_state, sizeof(pwps_state), "%d", ENABLE);

    /** update the eap_server=1 */
    qsap_write_cfg(pconffile, &qsap_str[STR_EAP_SERVER], pwps_state, presp, plen, HOSTAPD_CONF_QCOM_FILE);

    for(i=eCMD_UUID; i<=eCMD_UPC; i++) {
        if(eERR_UNKNOWN == qsap_change_cfg(pconffile, &cmd_list[i], status)) {
            ALOGE("%s: failed to set %s\n", __func__, cmd_list[i].name);
            goto error;
        }
    }

    return;
error:
    *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);

    return;
}

static void qsap_config_wps_method(s8 *pVal, s8 *presp, u32 *plen)
{
    s8 buf[64];
    s8 *ptr;
    int i;
    s32 value;

    /** INPUT : <0/1> <PIN> */
    /** PBC method : WPS_PBC */
    /** PIN method : WPS_PIN any <key> */
    ptr = pVal;
    i = 0;

    while((*ptr != '\0') && (*ptr != ' ')) {
        buf[i] = *ptr;
        ptr++;
        i++;
    }

    buf[i] = '\0';

    /** Identify the WPS method */
    value = atoi(buf);
    if(TRUE != IS_VALID_WPS_CONFIG(value)) {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_PARAM);
        return;
    }

    SKIP_BLANK_SPACE(ptr);

    if( (value == WPS_CONFIG_PIN) && (*ptr == '\0') ){
        ALOGE("%s :Invalid command \n", __func__);
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_PARAM);
        return;
    }

    if(value == WPS_CONFIG_PBC)
        qsap_scnprintf(buf, sizeof(buf), "WPS_PBC");
    else {
        if(strlen(ptr) < WPS_KEY_LEN) {
            ALOGD("%s :Invalid WPS key length\n", __func__);
            *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_PARAM);
            return;
        }
        qsap_scnprintf(buf, sizeof(buf), "WPS_PIN any %s", ptr);
    }

    value = qsap_send_cmd_to_hostapd(buf);

    *plen = qsap_scnprintf(presp, *plen, "%s", (value == eSUCCESS) ? SUCCESS: ERR_UNKNOWN);

    return;
}


s32 atoh(u8 *str)
{
    u32 val = 0;
    u32 pos = 0;
    s32 len = strlen((char *)str) - 1;

    while(len >= 0) {
        switch(str[len]) {

        case '0' ... '9':
                val += (str[len] - '0') << pos;
                break;

        case 'a' ... 'f':
                val += (str[len] - 'a' + 10) << pos;
                break;

        case 'A'... 'F':
                val += (str[len] - 'A' + 10) << pos;
                break;
        }
        len--;
        pos += 4;
    }

    return val;
}

int qsap_get_mac_in_bytes(char *psmac, char *pbmac)
{
    int val;
    u8 str[3];
    u32 i;

    str[2] = '\0';

    if(FALSE == isValid_MAC_address(psmac)) {
        return FALSE;
    }

    for(i=0; i<strlen(psmac); i++) {
        if(psmac[i] == ':')
            continue;

        str[0] = psmac[i];
        str[1] = psmac[i+1];
        val = atoh(str);
        *pbmac = val;
        pbmac++;
        i += 2;
    }
    *pbmac = 0;

    return TRUE;
}

void qsap_disassociate_sta(s8 *pVal, s8 *presp, u32 *plen)
{
    int sock, ret = eERR_UNKNOWN;
    struct iwreq wrq;
    s8 pbuf[MAX_CONF_LINE_LEN];
    u32 len = MAX_CONF_LINE_LEN;
    s8 *pif;

    if(ENABLE != is_softap_enabled()) {
        goto end;
    }

    if(NULL == (pif = qsap_get_config_value(pconffile, &qsap_str[STR_INTERFACE], pbuf, &len))) {
        ALOGE("%s :interface error \n", __func__);
        goto end;
    }

    pbuf[len] = '\0';

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        ALOGE("%s: socket failure \n", __func__);
        goto end;
    }

    strlcpy(wrq.ifr_name, pif, sizeof(wrq.ifr_name));

    if (TRUE != qsap_get_mac_in_bytes(pVal, (char *) &wrq.u)) {
        ALOGE("%s: Invalid input \n", __func__);
        close(sock);
        goto end;
    }

    ret = ioctl(sock, QCSAP_IOCTL_DISASSOC_STA, &wrq);
    if(ret < 0) {
        ALOGE("%s: ioctl failure \n", __func__);
    }
    close(sock);

end:
    *plen = qsap_scnprintf(presp, *plen, "%s", (ret == eSUCCESS) ? SUCCESS : ERR_UNKNOWN);

    return;
}

static int qsap_set_channel(s32 channel, s8 *tbuf, u32 *tlen)
{
    u32 ulen;
    s8 *pcfgval;
    s8 schan[MAX_INT_STR+1];
    s8 *pcfg = pconffile;

    ulen = *tlen;

    /* Do not worry about hw_mode if intention is to use ACS (channel=0) */
    if (channel == 0)
        goto end;

    /** Read the current operating mode */
    if(NULL == (pcfgval = qsap_get_config_value(pconffile, &cmd_list[eCMD_HW_MODE], tbuf, &ulen))) {
        return eERR_UNKNOWN;
    }

    /** If the operating mode is 'A' and the channel to be set is in between 1 and 14
      * then change the operating mode to 'G' mode */
    if((!strcmp(hw_mode[HW_MODE_A], pcfgval)) && (channel <=14)) {
        /** Change the operating mode to 'G' */
        ulen = *tlen;
        if(eSUCCESS != qsap_write_cfg(pcfg, &cmd_list[eCMD_HW_MODE], hw_mode[HW_MODE_G], tbuf, &ulen, HOSTAPD_CONF_QCOM_FILE)) {
            ALOGE("%s :Unable to update the operating mode \n", __func__);
            return eERR_UNKNOWN;
        }
    }

    /** If the operating mode is NOT 'B' and the channel to be set is  14
      * then change the operating mode to 'B' mode */
    if(strcmp(hw_mode[HW_MODE_B], pcfgval) && (channel == 14)) {
        /** Change the operating mode to 'B' */
        ulen = *tlen;
        if(eSUCCESS != qsap_write_cfg(pcfg, &cmd_list[eCMD_HW_MODE], hw_mode[HW_MODE_B], tbuf, &ulen, HOSTAPD_CONF_QCOM_FILE)) {
            ALOGE("%s :Unable to update the operating mode \n", __func__);
            return eERR_UNKNOWN;
        }
    }

    if(channel > 14) {
        /** Change the operating mode to 'A' */
        ulen = *tlen;
        if(eSUCCESS != qsap_write_cfg(pcfg, &cmd_list[eCMD_HW_MODE], hw_mode[HW_MODE_A], tbuf, &ulen, HOSTAPD_CONF_QCOM_FILE)) {
            ALOGE("%s :Unable to update the operating mode \n", __func__);
            return eERR_UNKNOWN;
        }
    }

end:
    qsap_scnprintf(schan, sizeof(schan), "%ld", channel);

    return qsap_write_cfg(pcfg, &cmd_list[eCMD_CHAN], schan, tbuf, tlen, HOSTAPD_CONF_QCOM_FILE);
}

static int qsap_set_operating_mode(s32 mode, s8 *pmode, int pmode_len, s8 *tbuf, u32 *tlen)
{
    u32 ulen;
    s8 *pcfg = pconffile;
    s8  ieee11n_enable[] = "1";
    s8  ieee11n_disable[] = "0";

    ulen = *tlen;

    /** Update the operating mode */
    qsap_change_cfg(pcfg, &cmd_list[eCMD_BASIC_RATES],DISABLE);
    qsap_change_cfg(pcfg, &cmd_list[eCMD_REQUIRE_HT],DISABLE);
    qsap_write_cfg(pcfg, &cmd_list[eCMD_IEEE80211N],ieee11n_disable, tbuf, &ulen, HOSTAPD_CONF_QCOM_FILE);
    switch(mode)
    {
        case HW_MODE_G_ONLY:
            qsap_change_cfg(pcfg, &cmd_list[eCMD_BASIC_RATES],ENABLE);
            break;
        case HW_MODE_N_ONLY:
            qsap_change_cfg(pcfg, &cmd_list[eCMD_REQUIRE_HT],ENABLE);
            /* fall through */
        case HW_MODE_N:
        case HW_MODE_G:
        case HW_MODE_A:
        case HW_MODE_ANY:
            ulen = *tlen;
            qsap_write_cfg(pcfg, &cmd_list[eCMD_IEEE80211N],ieee11n_enable, tbuf, &ulen, HOSTAPD_CONF_QCOM_FILE);
            break;
        case HW_MODE_B:
            ulen = *tlen;
            qsap_write_cfg(pcfg, &cmd_list[eCMD_IEEE80211N],ieee11n_disable, tbuf, &ulen, HOSTAPD_CONF_QCOM_FILE);
            break;
        case HW_MODE_AD:
            /** For 802.11ad, disable the 802.11 HT */
            qsap_change_cfg(pcfg, &cmd_list[eCMD_HT_CAPAB], DISABLE);
            break;
    }
    if(mode == HW_MODE_G_ONLY || mode == HW_MODE_N_ONLY || mode == HW_MODE_N ) {
        qsap_scnprintf(pmode, pmode_len, "%s",hw_mode[HW_MODE_G]);
    }
    return qsap_write_cfg(pcfg, &cmd_list[eCMD_HW_MODE], pmode, tbuf, tlen, HOSTAPD_CONF_QCOM_FILE);
}

static int qsap_set_data_rate(s32 drate_idx, s8 *presp, u32 *plen)
{
    u32 ulen;
    s8 *pmode;
    s8 sconf[MAX_INT_STR+1];
    int ret = eERR_UNKNOWN;

    if(TRUE != IS_VALID_DATA_RATE_IDX(drate_idx)) {
        ALOGE("%s :Invalid rate index \n", __func__);
        goto end;
    }

    ulen = *plen;
    /** Read the current operating mode */
    if(NULL == (pmode = qsap_get_config_value(pconffile, &cmd_list[eCMD_HW_MODE], presp, &ulen))) {
        ALOGE("%s :Unable to read mode \n", __func__);
        goto end;
    }

    /** Validate the rate index against the current operating mode */
    if(((!strcmp(pmode, hw_mode[HW_MODE_B])) && (drate_idx > B_MODE_MAX_DATA_RATE_IDX)) ||
        ((!strcmp(pmode, hw_mode[HW_MODE_G]) || (!strcmp(pmode, hw_mode[HW_MODE_G_ONLY]))) &&
        (drate_idx > G_ONLY_MODE_MAX_DATA_RATE_IDX))) {
        ALOGE("%s :Invalid rate index \n", __func__);
        goto end;
    }

    qsap_scnprintf(sconf, sizeof(sconf), "%ld", drate_idx);

    /** Update the rate index in the configuration */
    return qsap_write_cfg(fIni, &qsap_str[STR_DATA_RATE_IN_INI], sconf, presp, plen, INI_CONF_FILE);

end:
    *plen = qsap_scnprintf(presp, *plen, "%s", ERR_UNKNOWN);

    return ret;
}

/**
 * @brief
 *     Handle the user requests of the form,
 *     "set <cmd num> <value1> ..."
 *     These commands are used to update the soft AP
 *     configuration information
 *
 * @param pcmd [IN]   pointer to the string, storing the command.
 * @param presp [OUT] pointer to the buffer, to store the command response.
 *                    The command output format :
 *                    On success,
 *                            success
 *                    On failure,
 *                            failure <error message>
 * @param plen [IN-OUT]
 *                 [IN]: Maximum length of the reponse buffer
 *                [OUT]: Reponse length
 * @return
 *         void
*/
static void qsap_handle_set_request(s8 *pcmd, s8 *presp, u32 *plen)
{
    esap_cmd_t cNum;
    esap_str_t sNum = STR_DENY_MAC_FILE;
    s8 *pVal, *pfile;
    s8 filename[MAX_FILE_PATH_LEN];
    u32 ulen;
    s32 status;
    s32 value;
    s16 ini = HOSTAPD_CONF_QCOM_FILE;
    s8 *pcfg = pconffile;

    pcmd += strlen("set");

    SKIP_BLANK_SPACE(pcmd);

    if(!(strncmp(pcmd, Conf_req[CONF_2g], strlen(Conf_req[CONF_2g])))) {
           pcmd += strlen(Conf_req[CONF_2g]);
           SKIP_BLANK_SPACE(pcmd);
    } else if (!(strncmp(pcmd, Conf_req[CONF_5g], strlen(Conf_req[CONF_5g])))) {
           pcmd += strlen(Conf_req[CONF_5g]);
           SKIP_BLANK_SPACE(pcmd);
    } else if (!(strncmp(pcmd, Conf_req[CONF_owe], strlen(Conf_req[CONF_owe])))) {
           pcmd += strlen(Conf_req[CONF_owe]);
           SKIP_BLANK_SPACE(pcmd);
    } else if (!(strncmp(pcmd, Conf_req[CONF_60g], strlen(Conf_req[CONF_60g])))) {
           pcmd += strlen(Conf_req[CONF_60g]);
           SKIP_BLANK_SPACE(pcmd);
    } else {
	    // DO NOTHING
    }
    cNum = qsap_get_cmd_num(pcmd);
    if(cNum == eCMD_INVALID) {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_ARG);
        ALOGE("Invalid command number :%d\n", cNum);
        return;
    }
    pVal = pcmd + strlen(cmd_list[cNum].name);
    if( (cNum != eCMD_COMMIT) &&
        (cNum != eCMD_RESET_TO_DEFAULT) &&
        ((*pVal != '=') || (((eCMD_PASSPHRASE != cNum)) && (strlen(pVal) < 2)))) {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_ARG);
        return;
    }

    pVal++;

    if((cNum != eCMD_COMMIT) && (cNum != eCMD_RESET_TO_DEFAULT)) {
        ALOGE("Cmd: %s Argument :%s \n", cmd_list[cNum].name, pVal);
    }
    switch(cNum) {
        case eCMD_ADD_TO_ALLOW:
        case eCMD_REMOVE_FROM_ALLOW:
            sNum = STR_ACCEPT_MAC_FILE;
            /* fall through */

        case eCMD_ADD_TO_DENY:
        case eCMD_REMOVE_FROM_DENY:
            ulen = MAX_FILE_PATH_LEN;
            if(NULL != (pfile = qsap_get_allow_deny_file_name(pconffile, &qsap_str[sNum], filename, &ulen))) {
                qsap_update_mac_list(pfile, cNum, pVal, presp, plen);
            }
            else {
                *plen = qsap_scnprintf(presp, *plen, "%s", ERR_RES_UNAVAILABLE);
            }
            return;

        case eCMD_SEC_MODE:
            value = atoi(pVal);
            if(FALSE == IS_VALID_SEC_MODE(value))
                goto error;
            /** Write back the integer value. This is to avoid values like 01, 001, 0001
             * being written to the configuration.
             */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            qsap_set_security_mode(pconffile, value, presp, plen);
            return;

        case eCMD_MAC_ACL:
            value = atoi(pVal);
            if(FALSE == IS_VALID_MAC_ACL(value))
                goto error;

            /** Write back the integer value. This is to avoid values like 01, 001, 0001
              * being written to the configuration
              */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);

            if(ACL_ALLOW_LIST == value) {
                value = ENABLE;
                status = DISABLE;
            }
            else if(ACL_DENY_LIST == value){
                value = DISABLE;
                status = ENABLE;
            }
            else {
                // must be ACL_ALLOW_AND_DENY_LIST
                value = ENABLE;
                status = ENABLE;
            }

            if(eERR_UNKNOWN != qsap_change_cfg(pconffile, &qsap_str[STR_ACCEPT_MAC_FILE], value)) {
                if(eERR_UNKNOWN != qsap_change_cfg(pconffile, &qsap_str[STR_DENY_MAC_FILE], status))
                {
                    qsap_write_cfg(pconffile, &cmd_list[cNum], pVal, presp, plen, HOSTAPD_CONF_QCOM_FILE);
                }
                else {
                    goto error;
                }
            }
            else {
                goto error;
            }
            return;

        case eCMD_COMMIT:
#if 0 // COMMIT is not required currently for ICS framework
            if ( gIniUpdated ) {
                status = wifi_qsap_reload_softap();
                gIniUpdated = 0;
            }
            else {
                status = commit();
            }
            *plen = qsap_scnprintf(presp, *plen, "%s", (status ==  eSUCCESS)? SUCCESS : ERR_UNKNOWN);
#endif
            *plen = qsap_scnprintf(presp, *plen, "%s", SUCCESS);
            return;

        case eCMD_ENABLE_SOFTAP:
            value = atoi(pVal);

            if(TRUE != IS_VALID_SOFTAP_ENABLE(value))
                goto error;

            if ( *pVal == '0' ) {
                    status = wifi_qsap_unload_driver();
            }
            else {
                status = wifi_qsap_load_driver();
            }
            *plen = qsap_scnprintf(presp, *plen, "%s", (status==eSUCCESS) ? SUCCESS : "failure Could not enable softap");
            return;

        case eCMD_ENABLE_WIGIG_SOFTAP:
            value = atoi(pVal);

            if (TRUE != IS_VALID_SOFTAP_ENABLE(value))
                goto error;

            if ( *pVal == '0' ) {
                status = wifi_qsap_stop_wigig_softap();
            }
            else {
                status = wifi_qsap_start_wigig_softap();
            }
            *plen = qsap_scnprintf(presp, *plen, "%s", (status==eSUCCESS) ? SUCCESS : "failure Could not enable Wigig softap");
            return;
        case eCMD_SSID:
            value = strlen(pVal);
            if(SSD_MAX_LEN < value)
                goto error;
            /* Disable ssid2 while setting ssid */
            qsap_change_cfg(pcfg, &cmd_list[eCMD_SSID2], DISABLE);
            break;

        case eCMD_SET_MAX_CLIENTS:
            value = strlen(pVal);
            break;
        case eCMD_BSSID:
            value = atoi(pVal);
            if(FALSE == IS_VALID_BSSID(value))
                goto error;
            /** Write back the integer value. This is to avoid values like 01, 001, 0001
              * being written to the configuration
              */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;
        case eCMD_PASSPHRASE:
            value = strlen(pVal);
            if(FALSE == IS_VALID_PASSPHRASE_LEN(value))
                goto error;
            break;

        case eCMD_CHAN:
            value = atoi(pVal);

            ulen = MAX_FILE_PATH_LEN;
            value = qsap_set_channel(value, filename, &ulen);

            *plen = qsap_scnprintf(presp, *plen, "%s", (value == eSUCCESS) ? SUCCESS : ERR_UNKNOWN);
            return;

        case eCMD_BCN_INTERVAL:
            value = atoi(pVal);
            if(FALSE == IS_VALID_BEACON(value))
                goto error;
            /** Write back the integer value. This is to avoid values like 01, 001, 0001
              * being written to the configuration
              */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;

        case eCMD_DTIM_PERIOD:
            value = atoi(pVal);
            if(FALSE == IS_VALID_DTIM_PERIOD(value))
                goto error;
            /** Write back the integer value. This is to avoid values like 01, 001, 0001
              * being written to the configuration
              */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;

        case eCMD_HW_MODE:
            status = FALSE;
            for(value=HW_MODE_B; value<HW_MODE_UNKNOWN; value++) {
                if(!strcmp(pVal, hw_mode[value])) {
                    status = TRUE;
                    break;
                }
            }

            if(status == FALSE)
                goto error;

            ulen = MAX_FILE_PATH_LEN;
            /* pVal is anull terminated string */
            value = qsap_set_operating_mode(value, pVal, strlen(pVal)+1, filename, &ulen);
            *plen = qsap_scnprintf(presp, *plen, "%s", (value == eSUCCESS) ? SUCCESS : ERR_UNKNOWN);
            return;

        case eCMD_AUTH_ALGS:
            value = atoi(pVal);
            if((value != AHTH_ALG_OPEN) && (value != AUTH_ALG_SHARED) &&
                          (value != AUTH_ALG_OPEN_SHARED))
                goto error;
            /** Write back the integer value. This is to avoid values like 01, 001, 0001
              * being written to the configuration
              */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;

        case eCMD_DEFAULT_KEY:
            value = atoi(pVal);
            if(FALSE == IS_VALID_WEP_KEY_IDX(value))
                goto error;
             /** Write back the integer value. This is to avoid values like 01, 001, 0001
               * being written to the configuration
               */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);

            qsap_write_cfg(pcfg, &cmd_list[cNum], pVal, presp, plen, ini);

            ulen = MAX_FILE_PATH_LEN;
            if(SEC_MODE_WEP != qsap_read_security_mode(pcfg, filename, &ulen)) {
                if(eERR_UNKNOWN == qsap_change_cfg(pcfg, &cmd_list[cNum], 0)) {
                    ALOGE("%s: eCMD_DEFAULT_KEY \n", __func__);
                    goto error;
                }
            }

            return;

        case eCMD_WPA_PAIRWISE:
        case eCMD_RSN_PAIRWISE:
            if(FALSE == IS_VALID_PAIRWISE(pVal))
                goto error;

            /** If the encryption type is TKIP, disable the 802.11 HT */
            value = 1;
            if(!strcmp(pVal, "TKIP")) {
                value = 0;
            }

            if(eERR_UNKNOWN == qsap_change_cfg(pconffile, &qsap_str[STR_HT_80211N], value)) {
                ALOGE("%s: unable to update 802.11 HT\n", __func__);
                goto error;
            }

            break;

        case eCMD_WEP_KEY0:
        case eCMD_WEP_KEY1:
        case eCMD_WEP_KEY2:
        case eCMD_WEP_KEY3:
            if(FALSE == is_valid_wep_key(pVal, filename, MAX_FILE_PATH_LEN))
                goto error;

            qsap_write_cfg(pcfg, &cmd_list[cNum], filename, presp, plen, ini);

            /** if the security mode is not WEP, update the WEP features, and
                do NOT set the WEP security */
            ulen = MAX_FILE_PATH_LEN;
            if(SEC_MODE_WEP != qsap_read_security_mode(pcfg, filename, &ulen)) {
                if(eERR_UNKNOWN == qsap_change_cfg(pcfg, &cmd_list[cNum], 0)) {
                    ALOGE("%s: CMD_WEP_KEY0 \n", __func__);
                    goto error;
                }
            }

            return;

        case eCMD_RESET_AP:
            value = atoi(pVal);
            ALOGE("Reset :%d \n", (int)value);
            if(SAP_RESET_BSS == value) {
                status = wifi_qsap_stop_softap();
                if(status == eSUCCESS) {
                    status = wifi_qsap_start_softap();
                    if (eSUCCESS != status)
                        wifi_qsap_unload_driver();
                }
            }
            else if(SAP_RESET_DRIVER_BSS == value){
                status = wifi_qsap_reload_softap();
            }
            else if(SAP_STOP_BSS == value) {
                status = wifi_qsap_stop_bss();
            }
            else if(SAP_STOP_DRIVER_BSS == value) {
                status = wifi_qsap_stop_softap();
                if(status == eSUCCESS)
                    status = wifi_qsap_unload_driver();
            }
#ifdef QCOM_WLAN_CONCURRENCY
            else if(SAP_INITAP == value) {
                status = wifi_qsap_start_softap_in_concurrency();
            }
            else if(SAP_EXITAP == value) {
                status = wifi_qsap_stop_softap_in_concurrency();
            }
#endif
            else {
                status = !eSUCCESS;
            }
            *plen = qsap_scnprintf(presp, *plen, "%s", (status == eSUCCESS) ? SUCCESS : ERR_UNKNOWN);
            return;

        case eCMD_DISASSOC_STA:
            qsap_disassociate_sta(pVal, presp, plen);
            return;

        case eCMD_RESET_TO_DEFAULT:
            if(eSUCCESS == (status = wifi_qsap_reset_to_default(pconffile, DEFAULT_CONFIG_FILE_PATH))) {
                if(eSUCCESS == (status = wifi_qsap_reset_to_default(fIni, DEFAULT_INI_FILE))) {
                    status = wifi_qsap_reload_softap();
                }
            }
            *plen = qsap_scnprintf(presp, *plen, "%s", (status ==  eSUCCESS) ? SUCCESS : ERR_UNKNOWN);
            return;

        case eCMD_DATA_RATES:
            value = atoi(pVal);
            qsap_set_data_rate(value, presp, plen);
            return;
        case eCMD_UUID:
            value = strlen(pVal);
            if(TRUE != IS_VALID_UUID_LEN(value))
                goto error;
            break;
        case eCMD_DEVICE_NAME:
            value = strlen(pVal);
            if(TRUE != IS_VALID_DEVICENAME_LEN(value))
                goto error;
            break;
        case eCMD_MANUFACTURER:
            value = strlen(pVal);
            if(TRUE != IS_VALID_MANUFACTURER_LEN(value))
                goto error;
            break;

        case eCMD_MODEL_NAME:
            value = strlen(pVal);
            if(TRUE != IS_VALID_MODELNAME_LEN(value))
                goto error;
            break;

        case eCMD_MODEL_NUMBER:
            value = strlen(pVal);
            if(TRUE != IS_VALID_MODELNUM_LEN(value))
                goto error;
            break;

        case eCMD_SERIAL_NUMBER:
            value = strlen(pVal);
            if(TRUE != IS_VALID_SERIALNUM_LEN(value))
                goto error;
            break;

        case eCMD_DEVICE_TYPE:
            value = strlen(pVal);
            if(TRUE != IS_VALID_DEV_TYPE_LEN(value))
                goto error;
            break;

        case eCMD_OS_VERSION:
            value = strlen(pVal);
            if(TRUE != IS_VALID_OS_VERSION_LEN(value))
                goto error;
            break;

        case eCMD_FRIENDLY_NAME:
            value = strlen(pVal);
            if(TRUE != IS_VALID_FRIENDLY_NAME_LEN(value))
                goto error;
            break;

        case eCMD_MANUFACTURER_URL:
        case eCMD_MODEL_URL:
            value = strlen(pVal);
            if(TRUE != IS_VALID_URL_LEN(value))
                goto error;
            break;

        case eCMD_MODEL_DESC:
            value = strlen(pVal);
            if(TRUE != IS_VALID_MODEL_DESC_LEN(value))
                goto error;
            break;

        case eCMD_UPC:
            value = strlen(pVal);
            if(TRUE != IS_VALID_UPC_LEN(value))
                goto error;
            break;
        case eCMD_WMM_STATE:
            value = atoi(pVal);
            if(TRUE != IS_VALID_WMM_STATE(value))
                goto error;

            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;
        case eCMD_WPS_STATE:
            value = atoi(pVal);
            if(TRUE != IS_VALID_WPS_STATE(value))
                goto error;
            /** Write back the integer value. This is to avoid values like 01, 001, 0001
              * being written to the configuration
              */
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            qsap_update_wps_config(pVal, presp, plen);
            return;

        case eCMD_WPS_CONFIG_METHOD:
            qsap_config_wps_method(pVal, presp, plen);
            return;

        case eCMD_PROTECTION_FLAG:
            value = atoi(pVal);
            if(TRUE != IS_VALID_PROTECTION(value))
                goto error;
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            sNum = STR_PROT_FLAG_IN_INI;
            ini = INI_CONF_FILE;
            break;

        case eCMD_FRAG_THRESHOLD:
            value = atoi(pVal);
            if(TRUE != IS_VALID_FRAG_THRESHOLD(value))
                goto error;
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;

        case eCMD_REGULATORY_DOMAIN:
            value = atoi(pVal);

            if(TRUE != IS_VALID_802DOT11D_STATE(value))
                goto error;

            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;

        case eCMD_RTS_THRESHOLD:
            value = atoi(pVal);
            if(TRUE != IS_VALID_RTS_THRESHOLD(value))
                goto error;
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;

        case eCMD_GTK_TIMEOUT:
            value = atoi(pVal);
            if(TRUE != IS_VALID_GTK(value))
                goto error;

            break;

        case eCMD_TX_POWER:
            value = atoi(pVal);
            if(TRUE != IS_VALID_TX_POWER(value))
                goto error;
            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            sNum = STR_TX_POWER_IN_INI;
            ini = INI_CONF_FILE;
            break;

        case eCMD_INTRA_BSS_FORWARD:
            value = atoi(pVal);
            if(TRUE != IS_VALID_INTRA_BSS_STATUS(value))
                goto error;

            if(DISABLE == value) {
                status = qsap_change_cfg(pcfg,
                          &cmd_list[eCMD_INTRA_BSS_FORWARD],DISABLE);
            }
             else {
                status = qsap_change_cfg(pcfg,
                          &cmd_list[eCMD_INTRA_BSS_FORWARD],ENABLE);
            }
            *plen = qsap_scnprintf(presp, *plen, "%s", (status == eSUCCESS) ?
                     SUCCESS : ERR_UNKNOWN);
            return;

        case eCMD_COUNTRY_CODE:
            value = strlen(pVal);
            if(value > CTRY_MAX_LEN )
                goto error;
            break;

        case eCMD_AP_AUTOSHUTOFF:
            value = atoi(pVal);
            if(TRUE != IS_VALID_APSHUTOFFTIME(value))
                goto error;
            /* copy a larger value back to pVal. Please pay special care
             * in caller to make sure that the buffer has sufficient size. */
            qsap_scnprintf(pVal, MAX_INT_STR, "%ld", value*60);
            sNum = STR_AP_AUTOSHUTOFF;
            ini = INI_CONF_FILE;
            break;

        case eCMD_AP_ENERGY_DETECT_TH:
            value = atoi(pVal);
            if(TRUE != IS_VALID_ENERGY_DETECT_TH(value))
                goto error;

            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            sNum = STR_AP_ENERGY_DETECT_TH;
            ini = INI_CONF_FILE;
            break;

        case eCMD_IEEE80211H:
            value = atoi(pVal);
            if(TRUE != IS_VALID_DFS_STATE(value))
                goto error;

            qsap_scnprintf(pVal, strlen(pVal)+1, "%ld", value);
            break;

        case eCMD_SET_CHANNEL_RANGE:
            ALOGE("eCMD_SET_CHANNEL_RANGE pcmd :%s\n", pcmd);
            value = qsap_set_channel_range(pcmd);
           *plen = qsap_scnprintf(presp, *plen, "%s", (value == eSUCCESS) ? SUCCESS :
                             ERR_UNKNOWN);
            return;
        case eCMD_SSID2:
            /* Disable ssid while setting ssid2 */
            qsap_change_cfg(pcfg, &cmd_list[eCMD_SSID], DISABLE);
            break;

        default: ;
            /** Do not goto error, in default case */
    }

    if(ini == INI_CONF_FILE) {
        ALOGD("WRITE TO INI FILE :%s\n", qsap_str[sNum].name);
        qsap_write_cfg(fIni, &qsap_str[sNum], pVal, presp, plen, ini);
    }
    else {
        qsap_write_cfg(pcfg, &cmd_list[cNum], pVal, presp, plen, ini);
    }

    return;

error:
    *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALID_PARAM);
    return;
}

/**
 * @brief
 *      Initiate the command and return response
 * @param pcmd string containing the command request
 *     The format of the command is
 *         get param=value
 *             or
 *         set param=value
 * @param presp buffer to store the command response
 * @param plen length of the respone buffer
 * @return
 *         void
*/
void qsap_hostd_exec_cmd(s8 *pcmd, s8 *presp, u32 *plen)
{
    ALOGD("CMD INPUT  [%s][%u]\n", pcmd, *plen);
    /* Skip any blank spaces */
    SKIP_BLANK_SPACE(pcmd);

    if(!(strncmp(pcmd, Cmd_req[eCMD_SET], strlen(Cmd_req[eCMD_SET])))) {
       if(!(strncmp(pcmd+4, Conf_req[CONF_2g], strlen(Conf_req[CONF_2g])))) {
           pconffile = CONFIG_FILE_2G;
       } else if (!(strncmp(pcmd+4, Conf_req[CONF_5g], strlen(Conf_req[CONF_5g])))) {
           pconffile = CONFIG_FILE_5G;
       } else if (!(strncmp(pcmd+4, Conf_req[CONF_owe], strlen(Conf_req[CONF_owe])))) {
           pconffile = CONFIG_FILE_OWE;
       } else if (!(strncmp(pcmd+4, Conf_req[CONF_60g], strlen(Conf_req[CONF_60g])))) {
           pconffile = CONFIG_FILE_60G;
       } else {
           pconffile = CONFIG_FILE;
       }
    }

    check_for_configuration_files();

    if(!strncmp(pcmd, Cmd_req[eCMD_GET], strlen(Cmd_req[eCMD_GET])) && isblank(pcmd[strlen(Cmd_req[eCMD_GET])])) {
        qsap_handle_get_request(pcmd, presp, plen);
    }

    else if(!(strncmp(pcmd, Cmd_req[eCMD_SET], strlen(Cmd_req[eCMD_SET]))) && isblank(pcmd[strlen(Cmd_req[eCMD_SET])]) ) {
        qsap_handle_set_request(pcmd, presp, plen);
    }

    else {
        *plen = qsap_scnprintf(presp, *plen, "%s", ERR_INVALIDREQ);
    }

    ALOGD("CMD OUTPUT [%s]\nlen :%u\n\n", presp, *plen);

    return;
}

/* netd and Froyo Native UI specific API */
#define DEFAULT_INTFERACE    "wlan0"
#define DEFAULT_SSID         "SOFTAP_SSID"
#define DEFAULT_CHANNEL      4
#define DEFAULT_PASSPHRASE   "12345678"
#define DEFAULT_AUTH_ALG     1
#define RECV_BUF_LEN         255
#define CMD_BUF_LEN          255
#define SET_BUF_LEN          15

/** Command input
    argv[3] = SSID,
    argv[4] = BROADCAST/HIDDEN,
    argv[5] = CHANNEL
    argv[6] = SECURITY,
    argv[7] = KEY,
*/
int qsapsetSoftap(int argc, char *argv[])
{
    char cmdbuf[CMD_BUF_LEN];
    char respbuf[RECV_BUF_LEN];
    u32 rlen = RECV_BUF_LEN;
    int i;
    int hidden = 0;
    int sec = SEC_MODE_NONE;
    char setCmd[SET_BUF_LEN] = "set";
    int offset = 0;

    ALOGD("%s, %s, %s, %d\n", __FUNCTION__, argv[0], argv[1], argc);

    for ( i=0; i<argc;i++) {
        ALOGD("ARG: %d - %s\n", i+1, argv[i]);
    }

    // check if 2nd arg is dual2g/dual5g/owe/60g
    if (argc > 2
         && (strncmp(argv[2], Conf_req[CONF_2g], 4) == 0
             || strncmp(argv[2], Conf_req[CONF_owe], 3) == 0
             || strncmp(argv[2], Conf_req[CONF_60g], 3) == 0)) {
            snprintf(setCmd, SET_BUF_LEN, "set %s", argv[2]);
            offset = 1;
            argc--;
    }

    /* set interface */
    if (argc > 2) {
        snprintf(cmdbuf, CMD_BUF_LEN, "%s interface=%s", setCmd, argv[2 + offset]);
    }
    else {
        snprintf(cmdbuf, CMD_BUF_LEN, "%s interface=%s", setCmd, DEFAULT_INTFERACE);
    }
    (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);


    /** set SSID */
    if(argc > 3) {
        // In case of dual2g/5g, Set ssid2 with hex values to accomodate sapce and special characters.
        if (offset)
            qsap_scnprintf(cmdbuf, sizeof(cmdbuf), "%s ssid2=%s", setCmd, argv[3 + offset]);
        else
            qsap_scnprintf(cmdbuf, sizeof(cmdbuf), "%s ssid=%s",setCmd, argv[3]);
    }
    else {
        qsap_scnprintf(cmdbuf, sizeof(cmdbuf), "%s ssid=%s_%d", setCmd, DEFAULT_SSID, rand());
    }
    (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);

    if(strncmp("success", respbuf, rlen) != 0) {
        ALOGE("Failed to set ssid\n");
        return eERR_UNKNOWN;
    }

    rlen = RECV_BUF_LEN;
    if (argc > 4) {
        if (strcmp(argv[4 + offset], "hidden") == 0) {
             hidden = 1;
        }
        snprintf(cmdbuf, CMD_BUF_LEN, "%s ignore_broadcast_ssid=%d", setCmd, hidden);
        (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);
        if(strncmp("success", respbuf, rlen) != 0) {
            ALOGE("Failed to set ignore_broadcast_ssid \n");
            return -1;
        }
    }
    /** channel */
    rlen = RECV_BUF_LEN;
    if(argc > 5) {
        snprintf(cmdbuf, CMD_BUF_LEN, "%s channel=%d", setCmd, atoi(argv[5 + offset]));
        (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);

        if(strncmp("success", respbuf, rlen) != 0) {
            ALOGE("Failed to set channel \n");
            return -1;
        }
    }

    /** Security */
    rlen = RECV_BUF_LEN;
    if(argc > 6) {

        /**TODO : need to identify the SEC strings for "wep", "wpa", "wpa2" */
        if(!strcmp(argv[6 + offset], "open"))
            sec = SEC_MODE_NONE;

        else if(!strcmp(argv[6 + offset], "wep"))
            sec = SEC_MODE_WEP;

        else if(!strcmp(argv[6 + offset], "wpa-psk"))
            sec = SEC_MODE_WPA_PSK;

        else if(!strcmp(argv[6 + offset], "wpa2-psk"))
            sec = SEC_MODE_WPA2_PSK;

        qsap_scnprintf(cmdbuf, sizeof(cmdbuf), "%s security_mode=%d",setCmd, sec);
    }
    else {
        qsap_scnprintf(cmdbuf, sizeof(cmdbuf) , "%s security_mode=%d", setCmd, DEFAULT_AUTH_ALG);
    }

    (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);

    if(strncmp("success", respbuf, rlen) != 0) {
        ALOGE("Failed to set security mode\n");
        return -1;
    }

    /** Key -- passphrase */
    rlen = RECV_BUF_LEN;
    if ( (sec == SEC_MODE_WPA_PSK) || (sec == SEC_MODE_WPA2_PSK) ) {
        if(argc > 7) {
            /* If the input passphrase is more than 63 characters, consider first 63 characters only*/
            if ( strlen(argv[7 + offset]) > 63 ) argv[7 + offset][63] = '\0';
            qsap_scnprintf(cmdbuf, CMD_BUF_LEN, "%s wpa_passphrase=%s",setCmd, argv[7 + offset]);
        }
        else {
            qsap_scnprintf(cmdbuf, sizeof(cmdbuf), "%s wpa_passphrase=%s", setCmd, DEFAULT_PASSPHRASE);
        }
    }

    (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);
    if(strncmp("success", respbuf, rlen) != 0) {
        ALOGE("Failed to set passphrase \n");
        return -1;
    }

    rlen = RECV_BUF_LEN;
    if(argc > 8) {
        qsap_scnprintf(cmdbuf, sizeof(cmdbuf), "%s max_num_sta=%d",setCmd, atoi(argv[8 + offset]));
    }
    (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);

    if(strncmp("success", respbuf, rlen) != 0) {
        ALOGE("Failed to set maximun client connections number \n");
        return -1;
    }
    rlen = RECV_BUF_LEN;

    qsap_scnprintf(cmdbuf, sizeof(cmdbuf), "%s commit", setCmd);

    (void) qsap_hostd_exec_cmd(cmdbuf, respbuf, &rlen);

    if(strncmp("success", respbuf, rlen) != 0) {
        ALOGE("Failed to COMMIT \n");
        return -1;
    }

    return 0;
}


static int check_for_config_file_size(FILE *fp)
{
   int length = 0;

   if( NULL != fp )
   {
      fseek(fp, 0L, SEEK_END);
      length = ftell(fp);
   }

   return length;
}

void check_for_configuration_files(void)
{
    FILE * fp;
    char *pfile;

    /* Check if configuration files are present, if not create the default files */

    /* If configuration file does not exist copy the default file */
    if ( NULL == (fp = fopen(pconffile, "r")) ) {
        wifi_qsap_reset_to_default(pconffile, DEFAULT_CONFIG_FILE_PATH);
    }
    else {

        /* The configuration file could be of 0 byte size, replace with default */
        if (check_for_config_file_size(fp) <= 0)
            wifi_qsap_reset_to_default(pconffile, DEFAULT_CONFIG_FILE_PATH);

        fclose(fp);
    }

    /* If Accept MAC list file does not exist, copy the default file */
    if ( NULL == (fp = fopen(ACCEPT_LIST_FILE, "r")) ) {
        wifi_qsap_reset_to_default(ACCEPT_LIST_FILE, DEFAULT_ACCEPT_LIST_FILE_PATH);
    }
    else {

        /* The configuration file could be of 0 byte size, replace with default */
        if (check_for_config_file_size(fp) <= 0)
            wifi_qsap_reset_to_default(ACCEPT_LIST_FILE, DEFAULT_ACCEPT_LIST_FILE_PATH);

        fclose(fp);
    }

    /* Provide read and write permissions to the owner */
    pfile = ACCEPT_LIST_FILE;
    if (chmod(pfile, 0660) < 0) {
        ALOGE("Error changing permissions of %s to 0660: %s",
                pfile, strerror(errno));
    }
    /* If deny MAC list file does not exist, copy the default file */
    if ( NULL == (fp = fopen(DENY_LIST_FILE, "r")) ) {
        wifi_qsap_reset_to_default(DENY_LIST_FILE, DEFAULT_DENY_LIST_FILE_PATH);
    }
    else {

        /* The configuration file could be of 0 byte size, replace with default */
        if (check_for_config_file_size(fp) <= 0)
            wifi_qsap_reset_to_default(DENY_LIST_FILE, DEFAULT_DENY_LIST_FILE_PATH);

        fclose(fp);
    }

    /* Provide read and write permissions to the owner */
    pfile = DENY_LIST_FILE;
    if (chmod(pfile, 0660) < 0) {
        ALOGE("Error changing permissions of %s to 0660: %s",
                pfile, strerror(errno));
    }
    return;
}

void qsap_set_ini_filename(void)
{
    if (property_get("wlan.driver.config", ini_file, NULL)) {
        fIni = ini_file;
        ALOGE("INI FILE PROP PRESENT %s\n", fIni);
    } else
        ALOGE("INI FILE PROP NOT PRESENT: Use default path %s\n", fIni);
    return;
}

// IOCTL command to create and delete bridge interface //
#ifndef SIOCBRADDBR
#define SIOCBRADDBR 0x89a0
#endif
#ifndef SIOCBRDELBR
#define SIOCBRDELBR 0x89a1
#endif

static int linux_set_iface_flags(int sock, const char *ifname, int dev_up)
{
    struct ifreq ifr;
    int ret;

    if (sock < 0)
        return -1;

    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
        ret = errno ? -errno : -999;
        ALOGE("Could not read interface %s flags: %s",
               ifname, strerror(errno));
        return ret;
    }

    if (dev_up) {
        if (ifr.ifr_flags & IFF_UP)
            return 0;
        ifr.ifr_flags |= IFF_UP;
    } else {
        if (!(ifr.ifr_flags & IFF_UP))
            return 0;
        ifr.ifr_flags &= ~IFF_UP;
    }

    if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0) {
        ret = errno ? -errno : -999;
        ALOGE("Could not set interface %s flags (%s): %s",
               ifname, dev_up ? "UP" : "DOWN", strerror(errno));
        return ret;
    }
    return 0;
}

int qsap_control_bridge(int argc, char ** argv)
{
    int br_socket;

    if (argc < 4) {
        ALOGE("Command not supported");
        return -1;
    }

    br_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (br_socket < 0) {
        ALOGE("socket(PF_INET,SOCK_DGRAM): %s",strerror(errno));
        return -1;
    }
    if (!strncmp(argv[2],"create", 6)) {
        if (ioctl(br_socket, SIOCBRADDBR, argv[3]) < 0) {
            ALOGE("Could not add bridge %s: %s", argv[3], strerror(errno));
            return -1;
        }
    } else if (!strncmp(argv[2],"remove", 6)) {
        if (ioctl(br_socket, SIOCBRDELBR, argv[3]) < 0) {
            ALOGE("Could not add remove %s: %s", argv[3], strerror(errno));
            return -1;
        }
    } else if (!strncmp(argv[2],"up", 2)) {
        return linux_set_iface_flags(br_socket, argv[3], 1);
    } else if (!strncmp(argv[2],"down", 4)) {
        return linux_set_iface_flags(br_socket, argv[3], 0);
    } else {
        ALOGE("Command %s not handled.", argv[2]);
        return -1;
    }

    return 0;
}


int linux_get_ifhwaddr(const char *ifname, char *addr)
{
    struct ifreq ifr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif
    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr)) {
        ALOGE("Could not get interface %s hwaddr: %s", ifname, strerror(errno));
        return -1;
    }

    if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
        ALOGE("%s: Invalid HW-addr family 0x%04x", ifname, ifr.ifr_hwaddr.sa_family);
        return -1;
    }
    memcpy(addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    ALOGE("%s: " MACSTR, ifname, MAC2STR(addr));

    return 0;
}


int qsap_add_or_remove_interface(const char *newIface , int createIface)
{
    const char *wlanIface = "wlan0";
    int retVal = 0;
    struct nl_msg *msg = NULL;
    struct nl_cb *cb = NULL;
    struct nl_cb *s_cb = NULL;
    struct nl_sock *nl_sock = NULL;
    int nl80211_id;
    enum nl80211_iftype type = NL80211_IFTYPE_AP;

    /*  Allocate a netlink socket */
    s_cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!s_cb) {
        ALOGE( "Failed to allocate Netlink Socket");
        retVal = -ENOMEM;
        goto nla_put_failure;
    }

    nl_sock = nl_socket_alloc_cb(s_cb);
    if (!nl_sock) {
        ALOGE( "Netlink socket Allocation failure");
        retVal = -ENOMEM;
        goto nla_put_failure;
    }

    /* connect to generic netlink socket */
    if (genl_connect(nl_sock)) {
        ALOGE( "Netlink socket Connection failure");
        retVal = -ENOLINK;
        goto nla_put_failure;
    }

    nl80211_id = genl_ctrl_resolve(nl_sock, "nl80211");
    if (nl80211_id < 0) {
        ALOGE( "nl80211 generic netlink not found");
        retVal = -ENOENT;
        goto nla_put_failure;
    }

    msg = nlmsg_alloc();
    if(!msg) {
        ALOGE( "Failed to allocate netlink message");
        retVal = -ENOMEM;
        goto nla_put_failure;
    }

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        ALOGE( "Failed to allocate netlink callback");
        retVal = -ENOMEM;
        goto nla_put_failure;
    }

    if (createIface == 1) {
       /* Issue NL80211_CMD_NEW_INTERFACE */
       genlmsg_put( msg, 0, 0, nl80211_id, 0, 0, NL80211_CMD_NEW_INTERFACE, 0);
       nla_put_u32( msg, NL80211_ATTR_IFINDEX, if_nametoindex( wlanIface ));
       NLA_PUT_STRING( msg, NL80211_ATTR_IFNAME, newIface);
       nla_put_u32( msg, NL80211_ATTR_IFTYPE, type);
    } else {
       genlmsg_put( msg, 0, 0, nl80211_id, 0, 0, NL80211_CMD_DEL_INTERFACE, 0);
       nla_put_u32( msg, NL80211_ATTR_IFINDEX, if_nametoindex( newIface ));
    }

    retVal = nl_send_auto_complete(nl_sock, msg );
    if (retVal < 0 ) {
        goto nla_put_failure;
    }
    else {
        ALOGD("Interface %s is %s - Ok", (createIface == 1 ? "created":"removed") ,newIface);
    }
nla_put_failure:
    if (nl_sock)
        nl_socket_free(nl_sock);
    if (s_cb)
        nl_cb_put(s_cb);
    if (msg)
        nlmsg_free(msg);
    if (cb)
        nl_cb_put(cb);
    return retVal;
}
