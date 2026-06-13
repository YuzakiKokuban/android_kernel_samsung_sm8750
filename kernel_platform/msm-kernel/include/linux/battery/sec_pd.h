/*
 * include/linux/battery/sec_pd.h
 *
 * header file supporting samsung pd information
 *
 * Copyright (C) 2020 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __SEC_PD_H__
#define __SEC_PD_H__
#define MAX_PDO_NUM 8
#define AVAILABLE_VOLTAGE 9000
#define DEFAULT_VOLTAGE 5000
#define UNIT_FOR_VOLTAGE 50
#define UNIT_FOR_CURRENT 10
#define UNIT_FOR_APDO_VOLTAGE 100
#define UNIT_FOR_APDO_CURRENT 50

/* d2d 15w authentication information */
#define AUTH_VENDOR_ID		0x04e8
#define AUTH_PRODUCT_ID		0x6860

typedef enum {
	PDIC_NOTIFY_EVENT_DETACH = 0,
	PDIC_NOTIFY_EVENT_PDIC_ATTACH,
	PDIC_NOTIFY_EVENT_PD_SINK,
	PDIC_NOTIFY_EVENT_PD_SOURCE,
	PDIC_NOTIFY_EVENT_PD_SINK_CAP,
	PDIC_NOTIFY_EVENT_PD_PRSWAP_SNKTOSRC,
	PDIC_NOTIFY_EVENT_PD_PRSWAP_SRCTOSNK,
} pdic_notifier_event_t;

typedef enum
{
	RP_CURRENT_LEVEL_NONE = 0,
	RP_CURRENT_LEVEL_DEFAULT,
	RP_CURRENT_LEVEL2,
	RP_CURRENT_LEVEL3,
	RP_CURRENT_ABNORMAL,
} RP_CURRENT_LEVEL;

typedef enum
{
	FPDO_TYPE = 0,
	APDO_TYPE,
	VPDO_TYPE,
} PDO_TYPE_T;

typedef enum {
	AUTH_NONE = 0,
	AUTH_LOW_PWR,
	AUTH_HIGH_PWR,
} AUTH_TYPE_T;

typedef struct _power_list {
	int accept;
	int max_voltage;
	int min_voltage;
	int max_current;
	int pdo_type;
	int apdo;
	int comm_capable;
	int suspend;
 } POWER_LIST;

typedef struct sec_pd_sink_status
{
	POWER_LIST power_list[MAX_PDO_NUM+1];
	int has_apdo; // pd source has apdo or not
	int has_vpdo;
	int available_pdo_num; // the number of available PDO
	int selected_pdo_num; // selected number of PDO to change
	int current_pdo_num; // current number of PDO
	unsigned short vid;
	unsigned short pid;
	unsigned int xid;

	int pps_voltage;
	int pps_current;

	unsigned int rp_currentlvl; // rp current level by ccic

	void (*fp_sec_pd_select_pdo)(int num);
	int (*fp_sec_pd_select_pps)(int num, int ppsVol, int ppsCur);
	void (*fp_sec_pd_vpdo_auth)(int auth, int d2d_type);
	void (*fp_sec_pd_ext_cb)(unsigned short v_id, unsigned short p_id);
	void (*fp_sec_pd_manual_ccopen_req)(int is_on);
	void (*fp_sec_pd_manual_jig_ctrl)(bool mode);
	void (*fp_sec_pd_detach_with_cc)(int state);
	void (*fp_sec_pd_change_src)(int max_cur);
} SEC_PD_SINK_STATUS;

struct pdic_notifier_struct {
	pdic_notifier_event_t event;
	SEC_PD_SINK_STATUS sink_status;
	void *pusbpd;
};

#if IS_ENABLED(CONFIG_SEC_PD)
const char* sec_pd_pdo_type_str(int pdo_type);
int sec_pd_select_pdo(int num);
int sec_pd_select_pps(int num, int ppsVol, int ppsCur);
int sec_pd_vpdo_auth(int auth, int d2d_type);
int sec_pd_get_current_pdo(unsigned int *pdo);
int sec_pd_get_selected_pdo(unsigned int *pdo);
int sec_pd_is_apdo(unsigned int pdo);
int sec_pd_get_apdo_prog_volt(unsigned int pdo_type, unsigned int max_volt);
int sec_pd_get_max_power(unsigned int pdo_type, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr);
int sec_pd_get_pdo_power(unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *curr);
int sec_pd_get_apdo_max_power(unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr);
void sec_pd_init_data(SEC_PD_SINK_STATUS* psink_status);
int sec_pd_register_chg_info_cb(void *cb);
int sec_pd_get_chg_info(void);
void sec_pd_get_vid_pid(unsigned short *vid, unsigned short *pid, unsigned int *xid);
void sec_pd_get_vid(unsigned short *vid);
void sec_pd_manual_ccopen_req(int is_on);
void sec_pd_manual_jig_ctrl(bool mode);
int sec_pd_detach_with_cc(int state);
int sec_pd_change_src(int max_cur);
#else
static inline char* sec_pd_pdo_type_str(int pdo_type) { return "\0"; }
static inline int sec_pd_select_pdo(int num) { return -ENODEV; }
static inline int sec_pd_select_pps(int num, int ppsVol, int ppsCur) { return -ENODEV; }
static inline int sec_pd_vpdo_auth(int auth, int d2d_type) { return -ENODEV; }
static inline int sec_pd_get_current_pdo(unsigned int *pdo) { return -ENODEV; }
static inline int sec_pd_get_selected_pdo(unsigned int *pdo) { return -ENODEV; }
static inline int sec_pd_is_apdo(unsigned int pdo) { return -ENODEV; }
static inline int sec_pd_get_apdo_prog_volt(unsigned int pdo_type, unsigned int max_volt) { return -ENODEV; }
static inline int sec_pd_get_max_power(unsigned int pdo_type, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr) { return -ENODEV; }
static inline int sec_pd_get_pdo_power(unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *curr) { return -ENODEV; }
static inline int sec_pd_get_apdo_max_power(unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr) { return -ENODEV; }
static inline void sec_pd_init_data(SEC_PD_SINK_STATUS* psink_status) { }
static inline int sec_pd_register_chg_info_cb(void *cb) { return 0; }
static inline void sec_pd_get_vid_pid(unsigned short *vid, unsigned short *pid, unsigned int *xid) { }
static inline void sec_pd_get_vid(unsigned short *vid) { }
static inline void sec_pd_manual_ccopen_req(int is_on) { }
static inline void sec_pd_manual_jig_ctrl(bool mode) { }
static inline int sec_pd_detach_with_cc(int state) { return 0; }
static inline int sec_pd_change_src(int max_cur) { return 0; }
#endif
#endif /* __SEC_PD_H__ */
