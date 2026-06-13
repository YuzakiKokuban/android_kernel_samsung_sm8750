/*
 *
 * Copyright (C) 2021 Samsung Electronics
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#define pr_fmt(fmt) "pdic_policy: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/usb/typec.h>
#include <linux/usb/typec/common/pdic_core.h>
#include <linux/usb/typec/common/pdic_notifier.h>
#include <linux/usb/typec/common/pdic_policy.h>
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
#include <linux/usb/typec/manager/if_cb_manager.h>
#endif
#if IS_ENABLED(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#else
#include <linux/battery/sec_pd.h>
#endif
#include <linux/usb_notify.h>
#include <linux/power_supply.h>

#define ROLE_SWAP_TIME_MS 5000
#define HOST_ON_WAIT_TIME_SEC 3

enum pp_status {
	PP_GOOD,
	PP_ERROR,
};

enum cc_on {
	PP_CCOFF,
	PP_CCON,
};

enum cc_state {
	PP_TOGGLE,
	PP_CCRP,
	PP_CCRD,
	PP_CCAUDIO,
	PP_CCDEBUG,
};

enum cc_direction {
	PP_CC1,
	PP_CC2,
};

enum cc_rp_current {
	PP_56K	= 1,
	PP_22K,
	PP_10K,
};

enum pp_power_role {
	PP_NO_POW_ROLE,
	PP_SINK,
	PP_SOURCE,
};

enum pp_data_role {
	PP_NO_DATA_ROLE,
	PP_UFP,
	PP_DFP,
};

enum pp_vbus {
	PP_VBUSOFF,
	PP_VBUSON,
};

enum pp_rid {
	PP_NORID,
	PP_R301K,
	PP_R255K,
	PP_R523K,
	PP_R619K,
};

enum pp_uid {
	PP_NOUID,
	PP_U301K,
	PP_U255K,
	PP_U523K,
	PP_U619K,
};

enum pp_water {
	PP_DRY,
	PP_WATER,
};

enum pp_ccshort {
	PP_CC_NORMAL,
	PP_CC_SHORT,
};

enum pp_sbushort {
	PP_SBU_NORMAL,
	PP_SBU_SHORT,
};

enum explicit_contract {
	PP_NO_EXCNT,
	PP_EXCNT,
};

enum pp_killer {
	PP_NO_KILLER,
	PP_KILLER,
};

struct role_swap {
	struct completion swap_coml;
	int try_power_role;
	int try_data_role;
	int try_port_type;
	int status;
};

struct usb_host_val {
	wait_queue_head_t host_turn_on_wait_q;
	int host_turn_on_event;
	int host_turn_on_wait_time;
	int detach_done_wait;
	int device_add;
};

struct pdic_policy {
	struct pp_ic_data *ic_data;
	struct typec_port *port;
	struct typec_partner *partner;
	struct usb_pd_identity partner_identity;
	struct typec_capability typec_cap;
	struct pdic_notifier_struct pd_noti;
	struct mutex p_lock;
	struct workqueue_struct *pdic_wq;
	struct delayed_work dischar_work;
	struct role_swap pp_r_s;
	struct usb_host_val usb_host;
	struct pdic_alt_info alt_info;
#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	struct usbpd_dev usbpd_d;
	struct if_cb_manager *man;
#endif
	struct notifier_block usb_ext_noti_nb;
	int cc_on;
	int cc_state;
	int cc_direction;
	int cc_rp_current;
	int power_role;
	int data_role;
	int vbus;
	int rid;
	int uid;
	int water;
	int cc_short;
	int sbu_short;
	int usb_killer;
	int explicit_contract;
	int support_pd;
	int acc_type;
	int temp_alt_mode_stop;
};

struct pdic_policy_pd_noti {
	struct pdic_policy *pp_data;
	struct pdic_notifier_struct *pd_noti;
};

struct pdic_policy_pd_noti pp_pd_noti;

static const char * const pp_msg[] = {
	[MSG_INITIAL]	= "initail",
	[MSG_SRC]	= "SRC",
	[MSG_SNK]	= "SNK",
	[MSG_VBUSON]	= "VBUSON",
	[MSG_VBUSOFF]	= "VBUSOFF",
	[MSG_RP56K]	= "RP56K",
	[MSG_RP22K]	= "RP22K",
	[MSG_RP10K]	= "RP10K",
	[MSG_UFP]	= "UFP",
	[MSG_DFP]	= "DFP",
	[MSG_AUDIO]	= "AUDIO",
	[MSG_DEBUG]	= "DEBUG",
	[MSG_CC1]	= "CC1",
	[MSG_CC2]	= "CC2",
	[MSG_NOCC_WAKE] = "NOCC_WAKE",
	[MSG_CC_SHORT]	= "CC_SHORT",
	[MSG_SBU_SHORT]	= "SBU_SHORT",
	[MSG_WATER]	= "WATER",
	[MSG_DRY]	= "DRY",
	[MSG_ROPEN]	= "ROPEN",
	[MSG_R301K]	= "R301K",
	[MSG_R255K]	= "R255K",
	[MSG_R523K]	= "R523K",
	[MSG_R619K]	= "R619K",
	[MSG_UOPEN] = "UOPEN",
	[MSG_U301K] = "U301K",
	[MSG_U255K] = "U255K",
	[MSG_U523K] = "U523K",
	[MSG_U619K] = "U619K",
	[MSG_FAC_ERR] = "FAC ERROR",
	[MSG_EX_CNT]	= "EXPLICIT CONTRACT",
	[MSG_KILLER]	= "USB_KILLER",
	[MSG_DCOVER]	= "DISCOVER ALT DEVICE",
	[MSG_DP_CONN]	= "DP CONNECT",
	[MSG_DP_DISCONN]	= "DP DISCONNECT",
	[MSG_DP_LINK_CONF]	= "DP LINK CONFIGURATION",
	[MSG_DP_HPD]	= "DP HPD",
	[MSG_DEVICE_INFO] = "DEVICE INFO",
	[MSG_SVID_INFO] = "SVID_INFO",
	[MSG_SELECT_PDO]	= "SELECT_PDO",
	[MSG_CURRENT_PDO]	= "CURRENT_PDO",
	[MSG_PD_POWER_STATUS]	= "PD_POWER_STATUS",
	[MSG_FAC_MODE_NOTI_TO_MUIC] = "FAC_MODE_NOTI_TO_MUIC",
	[MSG_GET_ROLESWAP_CHECK] = "GET_ROLESWAP_CHECK",
	[MSG_GET_ACC] = "GET_ACC",
	[MSG_MUIC_SET_BC12] = "MUIC_SET_BC12",
	[MSG_SHUTDOWN] = "SHUTDOWN",
	[MSG_CCOFF]	= "CC OFF",
	[MSG_MAX]	= "MSG MAX",
};

static const char * const pd_p_cc_on[] = {
	[PP_CCOFF]	= "ccoff",
	[PP_CCON]	= "ccon",
};

static const char * const pd_p_cc_state[] = {
	[PP_TOGGLE]	= "cc_toggle",
	[PP_CCRP]	= "cc_rp",
	[PP_CCRD]	= "cc_rd",
	[PP_CCAUDIO]	= "cc_audio",
	[PP_CCDEBUG]	= "cc_debug",
};

static const char * const pd_p_cc_direction[] = {
	[PP_CC1] = "cc1",
	[PP_CC2] = "cc2",
};

static const char * const pd_p_cc_rp_current[] = {
	[PP_56K] = "rp56k",
	[PP_22K] = "rp22k",
	[PP_10K] = "rp10k",
};

static const char * const pd_p_power_role[] = {
	[PP_NO_POW_ROLE] = "detach",
	[PP_SINK] = "sink",
	[PP_SOURCE] = "source",
};

static const char * const pd_p_data_role[] = {
	[PP_NO_DATA_ROLE] = "detach",
	[PP_UFP] = "UFP",
	[PP_DFP] = "DFP",
};

static const char * const pd_p_vbus[] = {
	[PP_VBUSOFF] = "vbus_off",
	[PP_VBUSON] = "vbus_on",
};

static const char * const pd_p_rid[] = {
	[PP_NORID]	= "NO RID",
	[PP_R301K]	= "R301K",
	[PP_R255K]	= "R255K",
	[PP_R523K]	= "R523K",
	[PP_R619K]	= "R619K",
};

static const char * const pd_p_uid[] = {
	[PP_NOUID]	= "NO RID",
	[PP_U301K]	= "U301K",
	[PP_U255K]	= "U255K",
	[PP_U523K]	= "U523K",
	[PP_U619K]	= "U619K",
};

static const char * const pd_p_water[] = {
	[PP_DRY] = "dry",
	[PP_WATER] = "water",
};

static const char * const pd_p_ccshort[] = {
	[PP_CC_NORMAL] = "normal",
	[PP_CC_SHORT] = "cc_short",
};

static const char * const pd_p_sbushort[] = {
	[PP_SBU_NORMAL] = "normal",
	[PP_SBU_SHORT] = "sbu_short",
};

static const char * const pd_p_contract[] = {
	[PP_NO_EXCNT] = "no contract",
	[PP_EXCNT] = "explicit contract",
};

#ifdef CONFIG_TYPEC
static const char * const pd_p_typec_roles[] = {
	[TYPEC_SINK] = "sink",
	[TYPEC_SOURCE] = "source",
};

static const char * const pd_p_data_roles[] = {
	[TYPEC_DEVICE]	= "device",
	[TYPEC_HOST]	= "host",
};

static const char * const pd_p_port_types[] = {
	[TYPEC_PORT_SRC] = "source",
	[TYPEC_PORT_SNK] = "sink",
	[TYPEC_PORT_DRP] = "dual",
};
#endif

static const char * const pd_noti_dp_pin[] = {
	[PDIC_NOTIFY_DP_PIN_UNKNOWN] = "unknown",
	[PDIC_NOTIFY_DP_PIN_A] = "Pin A",
	[PDIC_NOTIFY_DP_PIN_B] = "Pin B",
	[PDIC_NOTIFY_DP_PIN_C] = "Pin C",
	[PDIC_NOTIFY_DP_PIN_D] = "Pin D",
	[PDIC_NOTIFY_DP_PIN_E] = "Pin E",
	[PDIC_NOTIFY_DP_PIN_F] = "Pin F",
};

static void pdic_policy_event_notifier
		(struct work_struct *data)
{
	struct pdic_state_work *event_work =
		container_of(data, struct pdic_state_work, pdic_work);
	PD_NOTI_TYPEDEF pdic_noti;

	switch(event_work->dest){
		case PDIC_NOTIFY_DEV_USB :
			pr_info("usb:%s, dest=%s, id=%s, attach=%s, drp=%s\n", __func__,
				pdic_event_dest_string(event_work->dest),
				pdic_event_id_string(event_work->id),
				event_work->attach? "Attached": "Detached",
				pdic_usbstatus_string(event_work->event));
			break;
		default :
			pr_info("usb:%s, dest=%s, id=%s, attach=%d, event=%d\n", __func__,
				pdic_event_dest_string(event_work->dest),
				pdic_event_id_string(event_work->id),
				event_work->attach,
				event_work->event);
			break;
	}

	pdic_noti.src = PDIC_NOTIFY_DEV_PDIC;
	pdic_noti.dest = event_work->dest;
	pdic_noti.id = event_work->id;
	pdic_noti.sub1 = event_work->attach;
	pdic_noti.sub2 = event_work->event;
	pdic_noti.sub3 = event_work->sub;
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	pdic_noti.pd = pp_pd_noti.pd_noti;
#endif
	pdic_notifier_notify((PD_NOTI_TYPEDEF*)&pdic_noti, NULL, 0);

	kfree(event_work);
}

void pdic_policy_event_work(void *data, int dest,
			int id, int attach, int event, int sub)
{
	struct pdic_policy *pp_data = data;
	struct pdic_state_work * event_work;

	pr_info("usb: %s\n", __func__);
	event_work = kmalloc(sizeof(struct pdic_state_work), GFP_ATOMIC);
	if (!event_work) {
		pr_err("%s: failed to alloc for event_work\n", __func__);
		return;
	}
	INIT_WORK(&event_work->pdic_work, pdic_policy_event_notifier);

	event_work->dest = dest;
	event_work->id = id;
	event_work->attach = attach;
	event_work->event = event;
	event_work->sub = sub;

	queue_work(pp_data->pdic_wq, &event_work->pdic_work);
}

#define PDIC_POLICY_SEND_NOTI(data, dest, id, attach, event, sub) \
	pdic_policy_event_work(data, dest, id, attach, event, sub)

static bool is_short(struct pdic_policy *pp_data)
{
	if (pp_data->cc_short == PP_CC_SHORT ||
			pp_data->sbu_short == PP_SBU_SHORT)
		return true;
	else
		return false;
}

static void vbus_discharging_work(struct work_struct *work)
{
	struct pdic_policy *pp_data = container_of(to_delayed_work(work),
				struct pdic_policy, dischar_work);
	int val1 = 0, val2 = 0;
	
	val1 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
	gpio_set_value(pp_data->ic_data->vbus_dischar_gpio, 0);
	val2 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
	pr_info("%s vbus_discharging %d->%d\n", __func__, val1, val2);
}

#ifdef CONFIG_TYPEC
static int get_typec_pwr_mode(struct pdic_policy *pp_data)
{
	enum typec_pwr_opmode mode = TYPEC_PWR_MODE_USB;

	if (pp_data->support_pd && pp_data->explicit_contract)
		mode = TYPEC_PWR_MODE_PD;
	else if (pp_data->cc_rp_current == PP_22K)
		mode = TYPEC_PWR_MODE_1_5A;
	else if (pp_data->cc_rp_current == PP_10K)
		mode = TYPEC_PWR_MODE_3_0A;
	else
		;

	return mode;
}

static void process_typec_data_role(struct pdic_policy *pp_data,
		int set_role)
{
	struct typec_partner_desc desc;
	int mode = TYPEC_PWR_MODE_USB;
	int power = 0, data = 0;

	if (pp_data->ic_data->typec_implemented)
		goto done;

	if (set_role == PP_DFP)
		data = TYPEC_HOST;
	else
		data = TYPEC_DEVICE;

	if (pp_data->pp_r_s.try_data_role != PP_NO_DATA_ROLE) {
		if (set_role == pp_data->pp_r_s.try_data_role)
			pp_data->pp_r_s.status = PP_GOOD;
		else
			pp_data->pp_r_s.status = PP_ERROR;
		
		complete(&pp_data->pp_r_s.swap_coml);
	} 
	
	if (set_role == PP_NO_DATA_ROLE) {
		if (!IS_ERR(pp_data->partner))
			typec_unregister_partner(pp_data->partner);
		pp_data->partner = NULL;
		typec_set_data_role(pp_data->port, TYPEC_DEVICE);
		typec_set_pwr_opmode(pp_data->port, TYPEC_PWR_MODE_USB);
		goto done;
	}
	
	typec_set_data_role(pp_data->port, data);
	if (!pp_data->partner) {
		if (pp_data->power_role == PP_SOURCE)
			power = TYPEC_SOURCE;
		else
			power = TYPEC_SINK;
		typec_set_pwr_role(pp_data->port, power);
		mode = get_typec_pwr_mode(pp_data);
		desc.usb_pd = (mode == TYPEC_PWR_MODE_PD) ? 1 : 0;
		desc.accessory = TYPEC_ACCESSORY_NONE;
		desc.identity = NULL;
		pp_data->partner = typec_register_partner(pp_data->port, &desc);
		typec_set_pwr_opmode(pp_data->port, mode);
		pr_info("%s registerd partner. usb_pd=%d\n",
				__func__, desc.usb_pd);
	}
done:
	return;
}

static void process_typec_power_role(struct pdic_policy *pp_data,
		int set_role)
{
	int power = 0, try_role = PP_NO_POW_ROLE;
	int compl_cond = 0;

	if (pp_data->ic_data->typec_implemented)
		goto done;

	if (set_role == PP_SOURCE)
		power = TYPEC_SOURCE;
	else
		power = TYPEC_SINK;

	if (pp_data->pp_r_s.try_power_role != PP_NO_POW_ROLE) {
		try_role = pp_data->pp_r_s.try_power_role;
		compl_cond = 1;
	} else if (pp_data->pp_r_s.try_port_type != PP_NO_POW_ROLE) {
		try_role = pp_data->pp_r_s.try_port_type;
		if (set_role != PP_NO_POW_ROLE)
			compl_cond = 1;
	} else
		;

	if (compl_cond) {
		if (set_role == try_role)
			pp_data->pp_r_s.status = PP_GOOD;
		else
			pp_data->pp_r_s.status = PP_ERROR;
		
		complete(&pp_data->pp_r_s.swap_coml);
	}
	
	typec_set_pwr_role(pp_data->port, power);
done:
	return;
}
#else
inline int get_typec_pwr_mode(struct pdic_policy *pp_data) {return 0; }
inline void process_typec_data_role(struct pdic_policy *pp_data, int msg) {}
inline void process_typec_power_role(struct pdic_policy *pp_data, int msg) {}
#endif

static void pdic_policy_alt_dev_detach(struct pdic_policy *pp_data)
{
	struct pp_ic_data *ic_data;

	pr_info("%s\n", __func__);

	ic_data = pp_data->ic_data;
	if (!ic_data) {
		pr_err("%s ic_data in null\n", __func__);
		goto err;
	}

	if (pp_data->acc_type != PDIC_DOCK_DETACHED) {
		if (pp_data->acc_type != PDIC_DOCK_NEW)
			pdic_send_dock_intent(PDIC_DOCK_DETACHED);
		pdic_send_dock_uevent(pp_data->alt_info.vendor_id,
				pp_data->alt_info.product_id,
				PDIC_DOCK_DETACHED);
		memset(&pp_data->alt_info, 0, sizeof(struct pdic_alt_info));
		pp_data->acc_type = PDIC_DOCK_DETACHED;
		if (ic_data->p_ops && ic_data->p_ops->alt_info_clear)
			ic_data->p_ops->alt_info_clear(ic_data->drv_data);
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_ALL,
			PDIC_NOTIFY_ID_CLEAR_INFO, PDIC_NOTIFY_ID_DEVICE_INFO, 0, 0);
	}
err:
	return;
}

static void pdic_policy_dp_detach(struct pdic_policy *pp_data)
{
	struct pp_ic_data *ic_data;

	pr_info("%s\n", __func__);
	
	ic_data = pp_data->ic_data;
	if (!ic_data) {
		pr_err("%s ic_data in null\n", __func__);
		goto err;
	}

	PDIC_POLICY_SEND_NOTI(pp_data,
		PDIC_NOTIFY_DEV_USB_DP, PDIC_NOTIFY_ID_USB_DP,
		0/*dp_is_connect*/, 0/*dp_hs_connect*/, 0);
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_DP,
		PDIC_NOTIFY_ID_DP_CONNECT, 0/*attach*/, 0/*drp*/, 0);

	if (ic_data->p_ops && ic_data->p_ops->dp_info_clear)
		ic_data->p_ops->dp_info_clear(ic_data->drv_data);
err:
	return;
}

static void pdic_policy_pd_initial(struct pdic_notifier_struct *pd_noti)
{
	if (pd_noti->sink_status.available_pdo_num)
		memset(pd_noti->sink_status.power_list, 0,
			(sizeof(POWER_LIST) * (MAX_PDO_NUM + 1)));
	pd_noti->sink_status.has_apdo = false;
	pd_noti->sink_status.available_pdo_num = 0;
	pd_noti->sink_status.selected_pdo_num = 0;
	pd_noti->sink_status.current_pdo_num = 0;
#if !IS_ENABLED(CONFIG_BATTERY_NOTIFIER)
	pd_noti->sink_status.vid = 0;
	pd_noti->sink_status.pid = 0;
	pd_noti->sink_status.xid = 0;
#endif
	pd_noti->sink_status.pps_voltage = 0;
	pd_noti->sink_status.pps_current = 0;
	pd_noti->sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
}

static void pdic_policy_pd_detach(struct pdic_policy *pp_data)
{
	struct pdic_notifier_struct *pd_noti = NULL;

	if (!pp_pd_noti.pd_noti) {
		pr_info("%s pd_noti is null\n", __func__);
		goto skip;
	}

	pd_noti = pp_pd_noti.pd_noti;

	pdic_policy_pd_initial(pd_noti);

	pd_noti->event = PDIC_NOTIFY_EVENT_DETACH;
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT,
		PDIC_NOTIFY_ID_POWER_STATUS,
		0, 0, 0);
skip:
	return;
}

static void process_policy_cc_attach(struct pdic_policy *pp_data, int msg)
{
	struct otg_notify *o_notify = get_otg_notify();
	struct pdic_notifier_struct *pd_noti = NULL;

	int val1 = 0, val2 = 0;
	int event = 0;
	
	pp_data->cc_on = PP_CCON;

	if (gpio_is_valid(pp_data->ic_data->vbus_dischar_gpio)) {
		if (delayed_work_pending(&pp_data->dischar_work)) {
			val1 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
			gpio_set_value(pp_data->ic_data->vbus_dischar_gpio, 0);
			val2 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
			cancel_delayed_work_sync(&pp_data->dischar_work);
			pr_info("%s vbus_discharging %d->%d\n", __func__, val1, val2);
		}
	}
	
	if (msg == MSG_SRC) {
		if (pp_data->power_role == PP_SINK &&
				pp_data->cc_state == PP_CCRP) {
			if (pp_pd_noti.pd_noti) {
				pd_noti = pp_pd_noti.pd_noti;

				pdic_policy_pd_initial(pd_noti);

				pd_noti->event = PDIC_NOTIFY_EVENT_PD_PRSWAP_SNKTOSRC;
				PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT,
					PDIC_NOTIFY_ID_POWER_STATUS,
					0, 0, 0);
			}
		} 
		pp_data->cc_state = PP_CCRD;
		pp_data->power_role = PP_SOURCE;
		send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 1);
#ifdef CONFIG_TYPEC
		process_typec_power_role(pp_data, PP_SOURCE);
#endif
	} else if (msg == MSG_SNK) {
		if (pp_data->power_role == PP_SOURCE &&
				pp_data->cc_state == PP_CCRD) {
			if (pp_pd_noti.pd_noti) {
				pd_noti = pp_pd_noti.pd_noti;
				pdic_policy_pd_initial(pd_noti);
				pd_noti->event = PDIC_NOTIFY_EVENT_PD_PRSWAP_SRCTOSNK;
				PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT,
					PDIC_NOTIFY_ID_POWER_STATUS,
					0, 0, 0);
			}
		}
		pp_data->cc_state = PP_CCRP;
		pp_data->power_role = PP_SINK;
		send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 0);
#ifdef CONFIG_TYPEC
		process_typec_power_role(pp_data, PP_SINK);
#endif
	} else if (msg == MSG_AUDIO) {
		pp_data->cc_state = PP_CCAUDIO;

		event = NOTIFY_EXTRA_USB_ANALOGAUDIO;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
	} else if (msg == MSG_DEBUG) {
		pp_data->cc_state = PP_CCDEBUG;
	} else
		;
}

static void vbus_turn_on_ctrl(struct pdic_policy *pp_data, bool enable)
{
	struct otg_notify *o_notify = get_otg_notify();
	struct power_supply *psy_otg;
	union power_supply_propval val;
	int on = !!enable;
	int ret = 0;
	bool must_block_host = 0;
	bool unsupport_host = 0;
	static int reserve_booster;
	int val1 = 0, val2 = 0;

	if (!o_notify) {
		pr_err("%s o_notify is null\n", __func__);
		goto skip_notify;
	}	

#ifdef CONFIG_DISABLE_LOCKSCREEN_USB_RESTRICTION
	must_block_host = is_blocked(o_notify, NOTIFY_BLOCK_TYPE_HOST);
	unsupport_host  = !is_usb_host(o_notify);
#endif

	pr_info("%s : enable=%d, must_block_host=%d unsupport_host=%d\n",
		__func__, enable, must_block_host, unsupport_host);

	if (enable) {
		if (must_block_host || unsupport_host) {
			on = false;
			pr_info("%s : turn off vbus because of blocked host\n",
				__func__);
		}
	}

	if (o_notify->booting_delay_sec && on) {
		pr_info("%s is booting_delay_sec. skip to control booster\n",
			__func__);
		reserve_booster = 1;
		send_otg_notify(o_notify, NOTIFY_EVENT_RESERVE_BOOSTER, 1);
		goto err;
	}

	if (reserve_booster && !on) {
		reserve_booster = 0;
		send_otg_notify(o_notify, NOTIFY_EVENT_RESERVE_BOOSTER, 0);
	}

skip_notify:

	pr_info("%s on=%d\n", __func__, on);

	psy_otg = power_supply_get_by_name("otg");
	if (psy_otg) {
		val.intval = on;
		ret = psy_otg->desc->set_property(psy_otg,
			POWER_SUPPLY_PROP_ONLINE, &val);
		if (ret) {
			pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
				__func__, ret);
			goto err;
		}
	} else {
		pr_err("%s: Fail to get psy battery\n", __func__);
		goto err;
	}

	pr_info("otg accessory power = %d\n", on);

	if (pp_data->power_role == PP_SOURCE && !on &&
			gpio_is_valid(pp_data->ic_data->vbus_dischar_gpio)) {	
		val1 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
		gpio_set_value(pp_data->ic_data->vbus_dischar_gpio, 1);
		val2 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
		schedule_delayed_work
			(&pp_data->dischar_work, msecs_to_jiffies(160));
		pr_info("%s vbus_discharging %d->%d\n", __func__, val1, val2);
	}
err:
	return;
}

static void process_policy_vbus(struct pdic_policy *pp_data, int msg)
{
	if (msg == MSG_VBUSON)
		pp_data->vbus = PP_VBUSON;		
	else
		pp_data->vbus = PP_VBUSOFF;

	vbus_turn_on_ctrl(pp_data, pp_data->vbus);
}

static void process_policy_rpcurrent(struct pdic_policy *pp_data, int msg)
{
	struct pdic_notifier_struct *pd_noti = NULL; 
#if IS_ENABLED(CONFIG_TYPEC)
		enum typec_pwr_opmode mode = TYPEC_PWR_MODE_USB;
#endif

	pr_info("%s %s (%s)\n", __func__, pp_msg[msg],
			pd_p_contract[pp_data->explicit_contract]);

	if (pp_pd_noti.pd_noti) {
		pd_noti = pp_pd_noti.pd_noti;
		if (msg == MSG_RP56K) {
			pd_noti->sink_status.rp_currentlvl = RP_CURRENT_LEVEL_DEFAULT;
			pp_data->cc_rp_current = PP_56K;
		} else if (msg == MSG_RP22K) {
			pd_noti->sink_status.rp_currentlvl = RP_CURRENT_LEVEL2;
			pp_data->cc_rp_current = PP_22K;
#if IS_ENABLED(CONFIG_TYPEC)
			mode = TYPEC_PWR_MODE_1_5A;
#endif
		} else if (msg == MSG_RP10K) {
			pd_noti->sink_status.rp_currentlvl = RP_CURRENT_LEVEL3;
			pp_data->cc_rp_current = PP_10K;
#if IS_ENABLED(CONFIG_TYPEC)
			mode = TYPEC_PWR_MODE_3_0A;
#endif
		} else
			;
		pd_noti->event = PDIC_NOTIFY_EVENT_PDIC_ATTACH;

		if (pp_data->explicit_contract == PP_NO_EXCNT) {
#if IS_ENABLED(CONFIG_TYPEC)
			typec_set_pwr_opmode(pp_data->port, mode);
#endif
			if (msg == MSG_RP56K && !is_short(pp_data)) {
				PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_MUIC,
					PDIC_NOTIFY_ID_TA,
					1/*attach*/, 0/*rprd*/, 0);
			}
			PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT,
				PDIC_NOTIFY_ID_POWER_STATUS,
				0/* no power nego*/, 0, 0);
			PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_MUIC,
				PDIC_NOTIFY_ID_RPLEVEL,
				PDIC_NOTIFY_ATTACH,
				USB_STATUS_NOTIFY_DETACH,
				pd_noti->sink_status.rp_currentlvl);
		}
	}
}

static void process_policy_data_role(struct pdic_policy *pp_data, int msg)
{
	pr_info("%s msg=%s p_data=%s p_power=%s +\n", __func__, pp_msg[msg],
		pd_p_data_role[pp_data->data_role], pd_p_power_role[pp_data->power_role]);

	if (msg == MSG_UFP) {
		if (pp_data->data_role == PP_UFP)
			goto skip;
		if (pp_data->data_role == PP_DFP) {
			pdic_policy_alt_dev_detach(pp_data);
			pdic_policy_dp_detach(pp_data);
			
			PDIC_POLICY_SEND_NOTI(pp_data,
				PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
				0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
		}
		
		pp_data->data_role = PP_UFP;

		process_typec_data_role(pp_data, PP_UFP);
		
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_ATTACH,
			1/*attach*/, 0/*rprd*/, 0);
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
			1/*attach*/, USB_STATUS_NOTIFY_ATTACH_UFP/*drp*/, 0);
	} else if (msg == MSG_DFP) {
		if (pp_data->data_role == PP_DFP)
			goto skip;
		if (pp_data->data_role == PP_UFP) {
			PDIC_POLICY_SEND_NOTI(pp_data,
				PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
				0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
		}
		
		pp_data->data_role = PP_DFP;

		process_typec_data_role(pp_data, PP_DFP);
		
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_MUIC,
			PDIC_NOTIFY_ID_ATTACH, 1/*attach*/, 1/*rprd*/, 0);
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
			1/*attach*/, USB_STATUS_NOTIFY_ATTACH_DFP/*drp*/, 0);
	} else {
		pp_data->data_role = PP_NO_DATA_ROLE;
		
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_ATTACH,
			0/*attach*/, 0/*rprd*/, 0);
		/* USB */
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
			0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
	}
skip:
	pr_info("%s msg=%s p_data=%s p_power=%s -\n", __func__, pp_msg[msg],
		pd_p_data_role[pp_data->data_role], pd_p_power_role[pp_data->power_role]);
	return;
}

static void process_policy_cc_active(struct pdic_policy *pp_data, int msg)
{
	if (msg == MSG_CC1) {
		pp_data->cc_direction = PP_CC1;
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_PDIC,
			PDIC_NOTIFY_ID_CC_PIN_STATUS, PDIC_NOTIFY_PIN_STATUS_CC1_ACTIVE,
			0, 0);
	} else if (msg == MSG_CC2) {
		pp_data->cc_direction = PP_CC2;
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_PDIC,
			PDIC_NOTIFY_ID_CC_PIN_STATUS, PDIC_NOTIFY_PIN_STATUS_CC2_ACTIVE,
			0, 0);
	} else if (msg == MSG_NOCC_WAKE) {
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_PDIC,
			PDIC_NOTIFY_ID_CC_PIN_STATUS, PDIC_NOTIFY_PIN_STATUS_NOCC_USB_ACTIVE,
			0, 0);
	} else
		;
}

static void process_policy_water(struct pdic_policy *pp_data, int msg)
{
	if (msg == MSG_WATER) {
		pp_data->water = PP_WATER;
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT, PDIC_NOTIFY_ID_WATER,
			1/*attach*/, 0, 0);
	} else if (msg == MSG_DRY) {
		pp_data->water = PP_DRY;
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT, PDIC_NOTIFY_ID_WATER,
			0/*attach*/, 0, 0);
	} else
		;
}

static void process_policy_rid(struct pdic_policy *pp_data, int msg)
{
	int rid = 0;
	int no_usb = 0;

	pr_info("%s msg=%s previous rid=%s\n", __func__,
		pp_msg[msg], pd_p_rid[pp_data->rid]);
		
	switch (msg) {
	case MSG_ROPEN:
		rid = RID_OPEN;
		pp_data->rid = PP_NORID;
		break;
	case MSG_R301K:
		rid = RID_301K;
		pp_data->rid = PP_R301K;
		break;
	case MSG_R255K:
		rid = RID_255K;
		pp_data->rid = PP_R255K;
		break;
	case MSG_R523K:
		rid = RID_523K;
		no_usb = 1;
		pp_data->rid = PP_R523K;
		break;
	case MSG_R619K:
		rid = RID_619K;
		no_usb = 1;
		pp_data->rid = PP_R619K;
		break;
	default:
		pp_data->rid = PP_NORID;
		break;
	} 
	
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_RID,
		rid/*rid*/, 0, 0);

	if (no_usb)
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
			0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
}

static void process_policy_explicit_contract
		(struct pdic_policy *pp_data, int msg, int skip_event)
{
	struct pdic_notifier_struct *pd_noti = NULL;
	int mode = TYPEC_PWR_MODE_USB;
	
	if (msg == MSG_EX_CNT) {
		if (pp_data->explicit_contract != PP_EXCNT) {
			pp_data->explicit_contract = PP_EXCNT;
#if IS_ENABLED(CONFIG_TYPEC)
			if (!pp_data->ic_data->typec_implemented) {
				mode = get_typec_pwr_mode(pp_data);
				typec_set_pwr_opmode(pp_data->port, mode);
			}
#endif
		}
		if (pp_data->power_role == PP_SINK) {
			if (pp_pd_noti.pd_noti) {
				if (!skip_event) {
					pd_noti = pp_pd_noti.pd_noti;
					pd_noti->event = PDIC_NOTIFY_EVENT_PD_SINK;
				}

				PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT,
					PDIC_NOTIFY_ID_POWER_STATUS,
					1, 0, 0);
			}
		}
	}
}

static void process_policy_usb_killer(struct pdic_policy *pp_data)
{
	int event = 0;
	struct otg_notify *o_notify = get_otg_notify();

	pp_data->usb_killer = PP_KILLER;

	event = NOTIFY_EXTRA_USBKILLER;
	store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#if defined(CONFIG_USB_HW_PARAM)
	if (o_notify)
		inc_hw_param(o_notify, USB_CCIC_USB_KILLER_COUNT);
#endif
}

static void process_policy_fac_err(struct pdic_policy *pp_data, int err)
{
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_PDIC,
			PDIC_NOTIFY_ID_FAC, err, 0, 0);
}

static void process_policy_fac_mode_noti_to_muic(struct pdic_policy *pp_data, int attach)
{
	PDIC_POLICY_SEND_NOTI(pp_data,
		PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_ATTACH,
		attach/*attach*/,
		USB_STATUS_NOTIFY_DETACH/*rprd*/, 0);
}

static void process_policy_sbu_short(struct pdic_policy *pp_data, int need_pd_detach)
{
	pp_data->sbu_short = PP_SBU_SHORT;

	if (need_pd_detach)
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_ATTACH,
			PDIC_NOTIFY_DETACH/*attach*/,
			USB_STATUS_NOTIFY_DETACH/*rprd*/, 0);
}

static int process_policy_get_ppdata(struct pdic_policy *pp_data, int msg)
{
	int ret = -EINVAL;

	switch (msg) {
	case MSG_GET_ROLESWAP_CHECK:
		if (pp_data->pp_r_s.try_power_role || pp_data->pp_r_s.try_data_role ||
			pp_data->pp_r_s.try_port_type)
			ret = 1;
		else
			ret = 0;
		break;
	case MSG_GET_ACC:
		ret = pp_data->acc_type;
		break;
	default:
		break;
	}

	return ret;
}

static void process_policy_cc_short(struct pdic_policy *pp_data, int need_pd_noti)
{
	struct pdic_notifier_struct *pd_noti = NULL;

	pp_data->cc_short = PP_CC_SHORT;

	if (need_pd_noti && pp_pd_noti.pd_noti) {
		pd_noti = pp_pd_noti.pd_noti;
		if (pd_noti->sink_status.rp_currentlvl != RP_CURRENT_ABNORMAL) {
			pd_noti->sink_status.rp_currentlvl = RP_CURRENT_ABNORMAL;
			pd_noti->event = PDIC_NOTIFY_EVENT_PDIC_ATTACH;
			pr_info("%s : rp_currentlvl(%d)\n", __func__,
					pd_noti->sink_status.rp_currentlvl);
			PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT,
				PDIC_NOTIFY_ID_POWER_STATUS,
				0/* no power nego*/, 0, 0);
			PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_MUIC,
				PDIC_NOTIFY_ID_RPLEVEL,
				PDIC_NOTIFY_ATTACH,
				USB_STATUS_NOTIFY_DETACH,
				pd_noti->sink_status.rp_currentlvl);
		}
	}
}

static void process_policy_shutdown(struct pdic_policy *pp_data)
{
	int val1 = 0, val2 = 0;

	if (gpio_is_valid(pp_data->ic_data->vbus_dischar_gpio)) {
		if (delayed_work_pending(&pp_data->dischar_work)) {
			val1 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
			gpio_set_value(pp_data->ic_data->vbus_dischar_gpio, 0);
			val2 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
			cancel_delayed_work_sync(&pp_data->dischar_work);
			pr_info("%s vbus_discharging %d->%d\n", __func__, val1, val2);
		}
	}
}

static int pdic_policy_get_alt_info(struct pdic_policy *pp_data)
{
	struct pp_ic_data *ic_data;
	int ret = 0, i = 0;

	ic_data = pp_data->ic_data;
	if (!ic_data) {
		pr_err("%s ic_data in null\n", __func__);
		goto err;
	}
	
	if (ic_data->p_ops && ic_data->p_ops->get_alt_info) {
		 ret = ic_data->p_ops->get_alt_info(ic_data->drv_data,
		 	&pp_data->alt_info);
		 if (ret < 0) {
		 	pr_err("%s get_alt_info error. ret %d\n", __func__, ret);
		 	goto err;
		 }
		 pr_info("%s :\n", __func__);
		 pr_info("vendor_id 0x%02x\n", pp_data->alt_info.vendor_id);
		 pr_info("product_id 0x%02x\n", pp_data->alt_info.product_id);
		 pr_info("bcd_device 0x%02x\n", pp_data->alt_info.bcd_device);
		 for (i = 0; i < 12; i++) {
			 if (pp_data->alt_info.svid[i] == 0)
				 break;
			 pr_info("svid 0x%02x\n", pp_data->alt_info.svid[i]);
		 }
		 pr_info("dp_device %d\n",	pp_data->alt_info.dp_device);
		 pr_info("dp_pin_assignment 0x%x\n", pp_data->alt_info.dp_pin_assignment);
		 pr_info("dp_selected_pin %s\n",
		 	pd_noti_dp_pin[pp_data->alt_info.dp_selected_pin]);
		 pr_info("hpd_state %d\n", pp_data->alt_info.hpd_state);
		 pr_info("hpd_irq %d\n", pp_data->alt_info.hpd_irq);
	} else
		pr_err("%s func is not defined\n", __func__);
	return 0;
err:
	return ret;
}

static void process_policy_dicover_alt_dev(struct pdic_policy *pp_data)
{
	struct pdic_alt_info *alt_info;
	struct otg_notify *o_notify = get_otg_notify();
	uint16_t vid = 0, pid = 0, acc_type = 0;
	int ret = 0;

	ret = pdic_policy_get_alt_info(pp_data);
	if (ret < 0)
		goto err;
	
	alt_info = &pp_data->alt_info;

	vid = alt_info->vendor_id;
	pid = alt_info->product_id;

	if (vid == SAMSUNG_VENDOR_ID) {
		switch (pid) {
		/* GearVR: Reserved GearVR PID+6 */
		case GEARVR_PRODUCT_ID:
		case GEARVR_PRODUCT_ID_1:
		case GEARVR_PRODUCT_ID_2:
		case GEARVR_PRODUCT_ID_3:
		case GEARVR_PRODUCT_ID_4:
		case GEARVR_PRODUCT_ID_5:
			acc_type = PDIC_DOCK_HMT;
			pr_info("Samsung Gear VR connected.\n");
#if defined(CONFIG_USB_HW_PARAM)
			if (o_notify)
				inc_hw_param(o_notify, USB_CCIC_VR_USE_COUNT);
#endif
			break;
		case DEXDOCK_PRODUCT_ID:
			acc_type = PDIC_DOCK_DEX;
			pr_info("Samsung DEX connected.\n");
#if defined(CONFIG_USB_HW_PARAM)
			if (o_notify)
				inc_hw_param(o_notify, USB_CCIC_DEX_USE_COUNT);
#endif
			break;
		case DEXPAD_PRODUCT_ID:
			acc_type = PDIC_DOCK_DEXPAD;
			pr_info("Samsung DEX PAD connected.\n");
#if defined(CONFIG_USB_HW_PARAM)
			if (o_notify)
				inc_hw_param(o_notify, USB_CCIC_DEX_USE_COUNT);
#endif
			break;
		case HDMI_PRODUCT_ID:
			acc_type = PDIC_DOCK_HDMI;
			pr_info("Samsung HDMI connected.\n");
			break;
		default:
			pr_info("default device connected.\n");
			acc_type = PDIC_DOCK_NEW;
			break;
		}
	} else if (vid == SAMSUNG_MPA_VENDOR_ID) {
		switch (pid) {
		case MPA_PRODUCT_ID:
			acc_type = PDIC_DOCK_MPA;
			pr_info("Samsung MPA connected.\n");
			break;
		default:
			pr_info("default device connected.\n");
			acc_type = PDIC_DOCK_NEW;
			break;
		}
	} else {
		pr_info("unknown device connected.\n");
		acc_type = PDIC_DOCK_NEW;
	}
	pp_data->acc_type = acc_type;

	if (acc_type != PDIC_DOCK_NEW)
		pdic_send_dock_intent(acc_type);

	pdic_send_dock_uevent(vid, pid, acc_type);
err:
	return;
}

static void process_policy_dp(struct pdic_policy *pp_data, int msg)
{
	struct pdic_alt_info *alt_info;
	struct otg_notify *o_notify = get_otg_notify();
	uint16_t vid = 0, pid = 0, timeleft = 0;
	int ret = 0;

	pr_info("%s msg=%s +\n", __func__, pp_msg[msg]);
	
	ret = pdic_policy_get_alt_info(pp_data);
	if (ret < 0)
		goto err;
	
	alt_info = &pp_data->alt_info;

	vid = alt_info->vendor_id;
	pid = alt_info->product_id;

	switch (msg) {
	case MSG_DP_CONN:
		PDIC_POLICY_SEND_NOTI(pp_data,
				PDIC_NOTIFY_DEV_DP, PDIC_NOTIFY_ID_DP_CONNECT,
				PDIC_NOTIFY_ATTACH, vid, pid);
#if defined(CONFIG_USB_HW_PARAM)
		inc_hw_param(o_notify, USB_CCIC_DP_USE_COUNT);
#endif
		pr_info("%s wait_event %ds\n", __func__,
					(pp_data->usb_host.host_turn_on_wait_time));

		timeleft = wait_event_interruptible_timeout(
					pp_data->usb_host.host_turn_on_wait_q,
					pp_data->usb_host.host_turn_on_event &&
					!pp_data->usb_host.detach_done_wait,
					(pp_data->usb_host.host_turn_on_wait_time)*HZ);
		pr_info("%s host turn on wait = %d\n", __func__, timeleft);

		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_USB_DP, PDIC_NOTIFY_ID_USB_DP,
			1/*dp_is_connect*/, 1/*dp_hs_connect*/, 0);
		break;
	case MSG_DP_DISCONN:
		pp_data->usb_host.detach_done_wait = 1;
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_USB_DP, PDIC_NOTIFY_ID_USB_DP,
			0/*dp_is_connect*/, 0/*dp_hs_connect*/, 0);
		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_DP,
			PDIC_NOTIFY_ID_DP_CONNECT, 0/*attach*/, 0/*drp*/, 0);
		break;
	case MSG_DP_LINK_CONF:
		if (alt_info->dp_selected_pin == PDIC_NOTIFY_DP_PIN_C ||
				alt_info->dp_selected_pin == PDIC_NOTIFY_DP_PIN_E ||
				alt_info->dp_selected_pin == PDIC_NOTIFY_DP_PIN_A)
			usb_restart_host_mode(pp_data->man, 4);
		else
			usb_restart_host_mode(pp_data->man, 2);

		PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_DP,
			PDIC_NOTIFY_ID_DP_LINK_CONF,
			alt_info->dp_selected_pin, 0, 0);
		break;
	case MSG_DP_HPD:
		PDIC_POLICY_SEND_NOTI(pp_data,
			PDIC_NOTIFY_DEV_DP, PDIC_NOTIFY_ID_DP_HPD,
			alt_info->hpd_state,
			alt_info->hpd_irq, 0);
		break;
	default:
		break;
	}
err:
	pr_info("%s msg=%s -\n", __func__, pp_msg[msg]);
	return;
}

static void process_policy_device_info(struct pdic_policy *pp_data)
{
	struct pdic_alt_info *alt_info;
	uint16_t vid = 0, pid = 0, bcd_device = 0;
	int ret = 0;

	ret = pdic_policy_get_alt_info(pp_data);
	if (ret < 0)
		goto err;

	alt_info = &pp_data->alt_info;

	vid = alt_info->vendor_id;
	pid = alt_info->product_id;
	bcd_device = alt_info->bcd_device;
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_ALL,
			PDIC_NOTIFY_ID_DEVICE_INFO, vid,
			pid, bcd_device);
err:
	return;
}

static void process_policy_svid_info(struct pdic_policy *pp_data, int svid)
{
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_ALL,
		PDIC_NOTIFY_ID_SVID_INFO, svid, 0, 0);
}

static void pdic_policy_update_pdo_num(void *data, int msg, int pdo_num)
{
	struct pdic_notifier_struct *pd_noti = NULL;

	pr_info("%s pdo_num=%d\n", __func__, pdo_num);

	if (!data) {
		pr_err("%s data is NULL\n", __func__);
		goto skip;
	}

	if (!pp_pd_noti.pd_noti) {
		pr_info("%s pd_noti is null\n", __func__);
		goto skip;
	}

	pd_noti = pp_pd_noti.pd_noti;

	if (msg == MSG_SELECT_PDO)
		pd_noti->sink_status.selected_pdo_num = pdo_num;
	else if (msg == MSG_CURRENT_PDO)
		pd_noti->sink_status.current_pdo_num = pdo_num;
	else
		;

skip:
	return;
}

static void pdic_policy_pd_power_status(struct pdic_policy *pp_data, int attach, int event)
{
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_BATT,
		PDIC_NOTIFY_ID_POWER_STATUS, attach, event, 0);
}

static void process_policy_cc_detach(struct pdic_policy *pp_data)
{
	struct otg_notify *o_notify = get_otg_notify();
	int val1 = 0, val2 = 0;

	if (pp_data->cc_on == PP_CCOFF) {
		pr_err("%s. prev cc_on=CCOFF. skip.\n", __func__);
		goto skip;
	}
	
	if (gpio_is_valid(pp_data->ic_data->vbus_dischar_gpio)) {
		val1 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
		gpio_set_value(pp_data->ic_data->vbus_dischar_gpio, 1);
		val2 = gpio_get_value(pp_data->ic_data->vbus_dischar_gpio);
		schedule_delayed_work
			(&pp_data->dischar_work, msecs_to_jiffies(120));
		pr_info("%s vbus_discharging %d->%d\n", __func__, val1, val2);
	}

	pp_data->usb_host.detach_done_wait = 1;
	pp_data->cc_on = PP_CCOFF;
	pp_data->cc_state = PP_TOGGLE;
	pp_data->cc_direction = PP_CC1;
	pp_data->cc_rp_current = PP_56K;
	pp_data->power_role = PP_NO_POW_ROLE;
	pp_data->data_role = PP_NO_DATA_ROLE;
	pp_data->rid = PP_NORID;
	pp_data->cc_short = PP_CC_NORMAL;
	pp_data->sbu_short = PP_SBU_NORMAL;
	pp_data->explicit_contract = PP_NO_EXCNT;
	pp_data->usb_killer = PP_NO_KILLER;
	send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 0);
#ifdef CONFIG_TYPEC
	process_typec_data_role(pp_data, PP_NO_DATA_ROLE);
	process_typec_power_role(pp_data, PP_NO_POW_ROLE);
#endif
	pdic_policy_pd_detach(pp_data);

	pdic_policy_alt_dev_detach(pp_data);

	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_ALL,
		PDIC_NOTIFY_ID_CLEAR_INFO, PDIC_NOTIFY_ID_SVID_INFO, 0, 0);
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_MUIC, PDIC_NOTIFY_ID_ATTACH,
		0/*attach*/, 0/*rprd*/, 0);
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_USB, PDIC_NOTIFY_ID_USB,
		0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
	PDIC_POLICY_SEND_NOTI(pp_data, PDIC_NOTIFY_DEV_PDIC,
		PDIC_NOTIFY_ID_CC_PIN_STATUS, PDIC_NOTIFY_PIN_STATUS_NO_DETERMINATION,
		0, 0);
	if (pp_data->temp_alt_mode_stop) {
		if (pp_data->ic_data->p_ops && pp_data->ic_data->p_ops->set_alt_mode)
			pp_data->ic_data->p_ops->set_alt_mode(ALTERNATE_MODE_START);
	}	
skip:
	return;
}

void pdic_policy_select_pdo(int num)
{
	struct pdic_notifier_struct *pd_noti = NULL;

	pr_info("%s pdo num(%d) +\n", __func__, num);

	if (!pp_pd_noti.pd_noti) {
		pr_info("%s pd_noti is null\n", __func__);
		goto skip;
	}

	pd_noti = pp_pd_noti.pd_noti;

	if (pd_noti->sink_status.selected_pdo_num == num) {
		if (pp_pd_noti.pp_data->explicit_contract == PP_EXCNT) {
			pd_noti->event = PDIC_NOTIFY_EVENT_PD_SINK;

			PDIC_POLICY_SEND_NOTI(pp_pd_noti.pp_data,
				PDIC_NOTIFY_DEV_BATT,
				PDIC_NOTIFY_ID_POWER_STATUS,
				1, 0, 0);
		}
	} else if (num > pd_noti->sink_status.available_pdo_num)
		pd_noti->sink_status.selected_pdo_num =
			pd_noti->sink_status.available_pdo_num;
	else if (num < 1)
		pd_noti->sink_status.selected_pdo_num = 1;
	else
		pd_noti->sink_status.selected_pdo_num = num;

skip:
	pr_info("%s pdo num(%d) -\n", __func__, num);
	return;
}

int pdic_policy_update_pdo_list(void *data, int max_v, int min_v,
		int max_icl, int cnt, int num)
{
	struct pdic_policy *pp_data;
	struct pdic_notifier_struct *pd_noti = NULL;
	int ret = 0;

	if (!data) {
		pr_err("%s data is NULL\n", __func__);
		ret = -ENOENT;
		goto err;
	}

	pr_info("%s +\n", __func__);

	pp_data = data;

	mutex_lock(&pp_data->p_lock);

	if (pp_pd_noti.pd_noti) {
		pd_noti = pp_pd_noti.pd_noti;

		pd_noti->sink_status.available_pdo_num = num;

		pd_noti->sink_status.power_list[cnt].max_voltage = max_v;
		pd_noti->sink_status.power_list[cnt].min_voltage = min_v;
		pd_noti->sink_status.power_list[cnt].max_current = max_icl;
		pd_noti->sink_status.power_list[cnt].accept =
			(max_v > AVAILABLE_VOLTAGE) ? false : true;
		pr_info("%s: receive[PDO %d] max_v:%d, min_v:%d, max_icl:%d\n",
			__func__, cnt, max_v, min_v, max_icl);
	}
	mutex_unlock(&pp_data->p_lock);
	pr_info("%s -\n", __func__);
err:
	return ret;
}
EXPORT_SYMBOL_GPL(pdic_policy_update_pdo_list);

int pdic_policy_send_msg(void *data, int msg, int param1, int param2)
{
	struct pdic_policy *pp_data;
	int ret = 0;
	
	if (!data) {
		pr_err("%s data is NULL\n", __func__);
		ret = -ENOENT;
		goto err;
	}

	pp_data = data;

	if (msg >= MSG_MAX) {
		pr_err("%s msg is invalid. (%d)\n", __func__, msg);
		ret = -E2BIG;
		goto err;
	}

	pr_info("%s msg=(%s) +\n", __func__, pp_msg[msg]);
		
	mutex_lock(&pp_data->p_lock);
	
	switch (msg) {
	case MSG_SRC:
	case MSG_SNK:
	case MSG_AUDIO:
	case MSG_DEBUG:
		process_policy_cc_attach(pp_data, msg);
		break;
	case MSG_VBUSON:
	case MSG_VBUSOFF:
		process_policy_vbus(pp_data, msg);
		break;
	case MSG_RP56K:
	case MSG_RP22K:
	case MSG_RP10K:
		process_policy_rpcurrent(pp_data, msg);
		break;
	case MSG_CC_SHORT:
		process_policy_cc_short(pp_data, param1);
		break;
	case MSG_SBU_SHORT:
		process_policy_sbu_short(pp_data, param1);
		break;
	case MSG_UFP:
	case MSG_DFP:
		process_policy_data_role(pp_data, msg);
		break;
	case MSG_CC1:
	case MSG_CC2:
	case MSG_NOCC_WAKE:
		process_policy_cc_active(pp_data, msg);
	case MSG_WATER:
	case MSG_DRY:
		process_policy_water(pp_data, msg);
		break;
	case MSG_ROPEN:
	case MSG_R301K:
	case MSG_R255K:
	case MSG_R523K:
	case MSG_R619K:
		process_policy_rid(pp_data, msg);
		break;
	case MSG_FAC_ERR:
		process_policy_fac_err(pp_data, param1);
		break;
	case MSG_EX_CNT:
		process_policy_explicit_contract(pp_data, msg, param1);
		break;
	case MSG_KILLER:
		process_policy_usb_killer(pp_data);
		break;
	case MSG_DCOVER:
		process_policy_dicover_alt_dev(pp_data);
		break;
	case MSG_DP_CONN:
	case MSG_DP_DISCONN:
	case MSG_DP_LINK_CONF:
	case MSG_DP_HPD:
		process_policy_dp(pp_data, msg);
		break;
	case MSG_DEVICE_INFO:
		process_policy_device_info(pp_data);
	case MSG_SVID_INFO:
		process_policy_svid_info(pp_data, param1);
	case MSG_SELECT_PDO:
	case MSG_CURRENT_PDO:
		pdic_policy_update_pdo_num(pp_data, msg, param1);
		break;
	case MSG_PD_POWER_STATUS:
		pdic_policy_pd_power_status(pp_data, param1, param2);
		break;
	case MSG_FAC_MODE_NOTI_TO_MUIC:
		process_policy_fac_mode_noti_to_muic(pp_data, param1);
		break;
	case MSG_GET_ROLESWAP_CHECK:
	case MSG_GET_ACC:
		ret = process_policy_get_ppdata(pp_data, msg);
		break;
	case MSG_MUIC_SET_BC12:
		muic_set_bc12(pp_data->man, param1);
		break;
	case MSG_SHUTDOWN:
		process_policy_shutdown(pp_data);
		break;
	case MSG_CCOFF:
		process_policy_cc_detach(pp_data);
		break;
	default:
		break;
	}

	mutex_unlock(&pp_data->p_lock);
	pr_info("%s msg=(%s) -\n", __func__, pp_msg[msg]);
err:
	return ret;
}
EXPORT_SYMBOL_GPL(pdic_policy_send_msg);

#ifdef CONFIG_TYPEC	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
int port_to_data(struct typec_port *port,
	struct pdic_policy **p_pp_data, struct pp_ic_data **p_ic_data)
{
	struct pdic_policy *pp_data = typec_get_drvdata(port);
	struct pp_ic_data *ic_data;
	if (!pp_data)
		goto err;
	
	*p_pp_data = pp_data;
	
	ic_data = pp_data->ic_data;
	if (!ic_data)
		goto err;

	*p_ic_data = ic_data;
	return 0;
err:
	return -EINVAL;
}
#endif

void pp_role_swap_init(struct role_swap *pp_r_s)
{
	pp_r_s->status = PP_GOOD;
	pp_r_s->try_data_role = PP_NO_DATA_ROLE;
	pp_r_s->try_power_role = PP_NO_POW_ROLE;
	pp_r_s->try_port_type = PP_NO_POW_ROLE;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
int pdic_policy_dr_set(struct typec_port *port, enum typec_data_role role)
#else
int pdic_policy_dr_set(const struct typec_capability *cap,
		enum typec_data_role role)
#endif
{
	struct pdic_policy *pp_data = NULL;
	struct pp_ic_data *ic_data = NULL;
	int ret = 0, wait = 0;
	unsigned long time = 0;

	if (role < TYPEC_DEVICE || role > TYPEC_HOST) {
		pr_err("%s role=%d +\n", __func__, role);
		ret = -EINVAL;
		goto err;
	}
	
	pr_info("%s role=%s +\n", __func__, pd_p_data_roles[role]);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	if (port_to_data(port, &pp_data, &ic_data)) {
		ret = -ENOENT;
		goto err;
	}
#else
	pp_data = container_of(cap, struct pdic_policy, typec_cap);
	ic_data = pp_data->ic_data;
	if (!ic_data) {
		ret = -ENOENT;
		goto err;
	}
#endif

	if (!ic_data->p_ops || !ic_data->p_ops->dr_set) {
		ret = -ECHILD;
		goto err;
	}

	if (role == TYPEC_HOST) {
		pp_data->pp_r_s.try_data_role = PP_DFP;
		wait = 1;
	} else if (role == TYPEC_DEVICE) {
		pp_data->pp_r_s.try_data_role = PP_UFP;
		wait = 1;
	} else {
		wait = 0;
		goto skip;
	}

	reinit_completion(&pp_data->pp_r_s.swap_coml);
	pp_data->pp_r_s.status = PP_GOOD;
	ret = ic_data->p_ops->dr_set(ic_data->drv_data, role);
	if (ret)
		goto err;

	if (wait) {
		time = wait_for_completion_timeout(&pp_data->pp_r_s.swap_coml,
			msecs_to_jiffies(ROLE_SWAP_TIME_MS));
		if (time == 0) {
			ret = -ETIMEDOUT;
			goto err;
		} else if (pp_data->pp_r_s.status == PP_ERROR) {
			ret = -EPROTO;
			goto err;
		}	
	}
skip:
err:
	pp_role_swap_init(&pp_data->pp_r_s);

	pr_info("%s ret=%d -\n", __func__, ret);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
int pdic_policy_pr_set(struct typec_port *port, enum typec_role role)
#else
int pdic_policy_pr_set(const struct typec_capability *cap, enum typec_role role)
#endif
{
	struct pdic_policy *pp_data = NULL;
	struct pp_ic_data *ic_data = NULL;
	int ret = 0, wait = 0;
	unsigned long time = 0;

	if (role < TYPEC_SINK || role > TYPEC_SOURCE) {
		pr_err("%s role=%d +\n", __func__, role);
		ret = -EINVAL;
		goto err;
	}

	pr_info("%s role=%s +\n", __func__, pd_p_typec_roles[role]);
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	if (port_to_data(port, &pp_data, &ic_data)) {
		ret = -ENOENT;
		goto err;
	}
#else
	pp_data = container_of(cap, struct pdic_policy, typec_cap);
	ic_data = pp_data->ic_data;
	if (!ic_data) {
		ret = -ENOENT;
		goto err;
	}
#endif

	if (!ic_data->p_ops || !ic_data->p_ops->pr_set) {
		ret = -ECHILD;
		goto err;
	}

	if (role == TYPEC_SOURCE) {
		pp_data->pp_r_s.try_power_role = PP_SOURCE;
		wait = 1;
	} else if (role == TYPEC_SINK) {
		pp_data->pp_r_s.try_power_role = PP_SINK;
		wait = 1;
	} else {
		wait = 0;
		goto skip;
	}

	reinit_completion(&pp_data->pp_r_s.swap_coml);
	pp_data->pp_r_s.status = PP_GOOD;
	ret = ic_data->p_ops->pr_set(ic_data->drv_data, role);
	if (ret)
		goto err;
	
	if (wait) {
		time = wait_for_completion_timeout(&pp_data->pp_r_s.swap_coml,
			msecs_to_jiffies(ROLE_SWAP_TIME_MS));
		if (time == 0) {
			ret = -ETIMEDOUT;
			goto err;
		} else if (pp_data->pp_r_s.status == PP_ERROR) {
			ret = -EPROTO;
			goto err;
		}	
	}
skip:
err:
	pp_role_swap_init(&pp_data->pp_r_s);
	
	pr_info("%s ret=%d -\n", __func__, ret);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
int pdic_policy_vconn_set(struct typec_port *port, enum typec_role role)
#else
int pdic_policy_vconn_set(const struct typec_capability *cap,
		enum typec_role role)
#endif
{
	struct pdic_policy *pp_data = NULL;
	struct pp_ic_data *ic_data = NULL;
	int ret = 0;

	if (role < TYPEC_SINK || role > TYPEC_SOURCE) {
		pr_err("%s role=%d +\n", __func__, role);
		ret = -EINVAL;
		goto err;
	}

	pr_info("%s role=%s +\n", __func__, pd_p_typec_roles[role]);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	if (port_to_data(port, &pp_data, &ic_data)) {
		ret = -ENOENT;
		goto err;
	}
#else
	pp_data = container_of(cap, struct pdic_policy, typec_cap);
	ic_data = pp_data->ic_data;
	if (!ic_data) {
		ret = -ENOENT;
		goto err;
	}
#endif

	if (!ic_data->p_ops || !ic_data->p_ops->vconn_set) {
		ret = -ECHILD;
		goto err;
	}

	ret = ic_data->p_ops->vconn_set(ic_data->drv_data, role);
	if (ret)
		goto err;

err:
	pr_info("%s ret=%d -\n", __func__, ret);
	return ret;

}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
int pdic_policy_port_type_set(struct typec_port *port,
	enum typec_port_type role)
#else
int pdic_policy_port_type_set(const struct typec_capability *cap,
	enum typec_port_type role)
#endif
{
	struct pdic_policy *pp_data = NULL;
	struct pp_ic_data *ic_data = NULL;
	int ret = 0, wait = 0;
	unsigned long time = 0;

	if (role < TYPEC_PORT_SRC || role > TYPEC_PORT_DRP) {
		pr_err("%s role=%d +\n", __func__, role);
		ret = -EINVAL;
		goto err;
	}

	pr_info("%s role=%s +\n", __func__, pd_p_port_types[role]);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	if (port_to_data(port, &pp_data, &ic_data)) {
		ret = -ENOENT;
		goto err;
	}
#else
	pp_data = container_of(cap, struct pdic_policy, typec_cap);
	ic_data = pp_data->ic_data;
	if (!ic_data) {
		ret = -ENOENT;
		goto err;
	}
#endif
	
	if (!ic_data->p_ops || !ic_data->p_ops->port_type_set) {
		ret = -ECHILD;
		goto err;
	}

	if (role == TYPEC_PORT_SRC) {
		pp_data->pp_r_s.try_port_type = PP_SOURCE;
		wait = 1;
	} else if (role == TYPEC_PORT_SNK) {
		pp_data->pp_r_s.try_port_type = PP_SINK;
		wait = 1;
	} else {
		wait = 0;
	}

	reinit_completion(&pp_data->pp_r_s.swap_coml);
	pp_data->pp_r_s.status = PP_GOOD;
	ret = ic_data->p_ops->port_type_set(ic_data->drv_data, role);
	if (ret)
		goto err;
	
	if (wait) {
		time = wait_for_completion_timeout(&pp_data->pp_r_s.swap_coml,
			msecs_to_jiffies(ROLE_SWAP_TIME_MS));
		if (time == 0) {
			ret = -ETIMEDOUT;
			goto err;
		} else if (pp_data->pp_r_s.status == PP_ERROR) {
			ret = -EPROTO;
			goto err;
		}	
	}
err:
	pp_role_swap_init(&pp_data->pp_r_s);
	
	pr_info("%s ret=%d -\n", __func__, ret);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
static const struct typec_operations pp_typec_ops = {
	.dr_set			= pdic_policy_dr_set,
	.pr_set			= pdic_policy_pr_set,
	.vconn_set		= pdic_policy_vconn_set,
	.port_type_set	= pdic_policy_port_type_set,
};
#endif
#endif

static void pdic_policy_usbpd_set_host_on(void *data, int mode)
{
	struct pdic_policy *pp_data = data;

	if (!pp_data) {
		pr_err("%s pp_data is null\n", __func__);
		goto err;
	}

	pr_info("%s : current_set is %d!\n", __func__, mode);
	if (mode) {
		pp_data->usb_host.device_add = 0;
		pp_data->usb_host.detach_done_wait = 0;
		pp_data->usb_host.host_turn_on_event = 1;
		wake_up_interruptible(&pp_data->usb_host.host_turn_on_wait_q);
	} else {
		pp_data->usb_host.device_add = 0;
		pp_data->usb_host.detach_done_wait = 0;
		pp_data->usb_host.host_turn_on_event = 0;
	}
err:
	return;
}

static int pdic_policy_usbpd_sbu_test_read(void *data)
{
	struct pdic_policy *pp_data = data;
	struct pp_ic_data *ic_data = pp_data->ic_data;
	int ret = 0;

	if (!ic_data) {
		ret = -ENOENT;
		goto err;
	}

	ret = ic_data->p_ops->usbpd_sbu_test_read(ic_data->drv_data);

err:
	return ret;
}

static void pdic_policy_cc_control_command(void *data, int is_off)
{
	struct pdic_policy *pp_data = data;
	struct pp_ic_data *ic_data = pp_data->ic_data;

	if (!ic_data)
		goto err;

	ic_data->p_ops->cc_control_command(ic_data->drv_data, is_off);
err:
	return;
}

struct usbpd_ops ops_usbpd = {
	.usbpd_set_host_on = pdic_policy_usbpd_set_host_on,
	.usbpd_sbu_test_read = pdic_policy_usbpd_sbu_test_read,
	.usbpd_cc_control_command = pdic_policy_cc_control_command,
};

static int pdic_policy_handle_usb_ext_noti(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct pdic_policy *pp_data = container_of(nb,
		struct pdic_policy, usb_ext_noti_nb);
	struct pp_ic_data *ic_data = pp_data->ic_data;
	struct pdic_alt_info *alt_info = &pp_data->alt_info;
	int ret = 0;
	int enable = *(int *)data;

	pr_info("%s action=%lu enable=%d +\n", __func__, action, enable);
	switch (action) {
	case EXTERNAL_NOTIFY_HOSTBLOCK_PRE:
		ret = pdic_policy_get_alt_info(pp_data);
		if (ret < 0)
			goto err;
		if (enable) {
			if (ic_data->p_ops && ic_data->p_ops->set_alt_mode)
				ic_data->p_ops->set_alt_mode(ALTERNATE_MODE_STOP);
			if (alt_info->dp_device) {
				mutex_lock(&pp_data->p_lock);
				pdic_policy_dp_detach(pp_data);
				mutex_unlock(&pp_data->p_lock);
			}
		} else {
			if (alt_info->dp_device) {
				mutex_lock(&pp_data->p_lock);
				pdic_policy_dp_detach(pp_data);
				mutex_unlock(&pp_data->p_lock);
			}
		}
		break;
	case EXTERNAL_NOTIFY_HOSTBLOCK_POST:
		if (enable) {
		} else {
			if (ic_data->p_ops && ic_data->p_ops->set_alt_mode)
				ic_data->p_ops->set_alt_mode(ALTERNATE_MODE_START);
		}
		break;
	case EXTERNAL_NOTIFY_DEVICEADD:
		if (enable)
			pp_data->usb_host.device_add = 1;
		break;
	case EXTERNAL_NOTIFY_MDMBLOCK_PRE:
		ret = pdic_policy_get_alt_info(pp_data);
		if (ret < 0)
			goto err;
		if (enable) {
			if (alt_info->dp_device) {
				pp_data->temp_alt_mode_stop = 1;
				if (ic_data->p_ops && ic_data->p_ops->set_alt_mode)
					ic_data->p_ops->set_alt_mode(ALTERNATE_MODE_STOP);
				mutex_lock(&pp_data->p_lock);
				pdic_policy_dp_detach(pp_data);
				mutex_unlock(&pp_data->p_lock);
			}
		} else 
			;
		break;
	case EXTERNAL_NOTIFY_MDMBLOCK_POST:
		if (enable)
			;
		else {
			if (pp_data->temp_alt_mode_stop) {
				pp_data->temp_alt_mode_stop = 0;
				if (ic_data->p_ops && ic_data->p_ops->set_alt_mode)
					ic_data->p_ops->set_alt_mode(ALTERNATE_MODE_START);
			}
		}
	default:
		break;
	}
	pr_info("%s action=%lu enable=%d -\n", __func__, action, enable);
	return 0;
err:
	pr_err("%s action=%lu,enable=%d error. ret=%d -\n", __func__,
		action, enable, ret);
	return ret;
}

void *pdic_policy_init(struct pp_ic_data *ic_data)
{
	struct pdic_policy *pp_data;
	struct otg_notify *o_notify = get_otg_notify();
	int ret = 0;
		
	pp_data = kzalloc(sizeof(struct pdic_policy), GFP_KERNEL);
	if (!pp_data || !ic_data) {
		pr_err("%s alloc fail\n", __func__);
		goto err;
	}

	mutex_init(&pp_data->p_lock);
	init_completion(&pp_data->pp_r_s.swap_coml);
	INIT_DELAYED_WORK(&pp_data->dischar_work, vbus_discharging_work);

	pp_data->pdic_wq
		= create_singlethread_workqueue("pdic_policy_wq");
	if (!pp_data->pdic_wq) {
		pr_err("%s failed to create work queue\n", __func__);
		goto err1;
	 }	

	pp_data->ic_data = ic_data;
	ic_data->pp_data = pp_data;
	pp_data->acc_type = PDIC_DOCK_DETACHED;
	if (ic_data->pd_noti) {
		pp_pd_noti.pd_noti = ic_data->pd_noti;
		pr_info("%s ic_data pd_noti registered\n", __func__);
	} else {
#if !IS_ENABLED(CONFIG_BATTERY_NOTIFIER)
		pp_data->pd_noti.sink_status.fp_sec_pd_select_pdo
			= pdic_policy_select_pdo;
		pp_pd_noti.pd_noti = &pp_data->pd_noti;
		pr_info("%s ic_data pd_noti is none.\n", __func__);
#endif
	}
	pp_pd_noti.pp_data = pp_data;

	pp_data->support_pd = ic_data->support_pd;

	pdic_register_switch_device(1);

#if IS_ENABLED(CONFIG_TYPEC)
	if (!pp_data->ic_data->typec_implemented) {
		pp_data->typec_cap.revision = USB_TYPEC_REV_1_2;
		pp_data->typec_cap.pd_revision = 0x300;
		pp_data->typec_cap.prefer_role = TYPEC_NO_PREFERRED_ROLE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
		pp_data->typec_cap.driver_data = pp_data;
		pp_data->typec_cap.ops = &pp_typec_ops;
#else
		pp_data->typec_cap.dr_set = pdic_policy_dr_set;
		pp_data->typec_cap.pr_set = pdic_policy_pr_set;
		pp_data->typec_cap.vconn_set = pdic_policy_vconn_set;
		pp_data->typec_cap.port_type_set = pdic_policy_port_type_set;
#endif

		pp_data->typec_cap.type = TYPEC_PORT_DRP;
		pp_data->typec_cap.data = TYPEC_PORT_DRD;

		pp_data->port = typec_register_port(ic_data->dev, 
				&pp_data->typec_cap);
		if (IS_ERR(pp_data->port)) {
			pr_err("unable to register typec_register_port\n");
			goto err2;
		}
	}
#endif

#if IS_ENABLED(CONFIG_IF_CB_MANAGER)
	init_waitqueue_head(&pp_data->usb_host.host_turn_on_wait_q);
	pdic_policy_usbpd_set_host_on(pp_data, 0);
	pp_data->usb_host.host_turn_on_wait_time = HOST_ON_WAIT_TIME_SEC;

	pp_data->usbpd_d.ops = &ops_usbpd;
	pp_data->usbpd_d.data = (void *)pp_data;
	pp_data->man = register_usbpd(&pp_data->usbpd_d);
#endif

	ret = usb_external_notify_register(&pp_data->usb_ext_noti_nb,
		pdic_policy_handle_usb_ext_noti, EXTERNAL_NOTIFY_DEV_PDIC);
	if (ret < 0) {
		pr_err("unable usb_external_notify_register\n");
		goto err2;
	}

	send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 0);

	return pp_data;
err2:
	flush_workqueue(pp_data->pdic_wq);
	destroy_workqueue(pp_data->pdic_wq);	
err1:
	mutex_destroy(&pp_data->p_lock);
	kfree(pp_data);	
err:
	return NULL;
}
EXPORT_SYMBOL_GPL(pdic_policy_init);

void pdic_policy_deinit(struct pp_ic_data *ic_data)
{
	struct pdic_policy *pp_data;
	if (ic_data && ic_data->pp_data) {
		pp_data = ic_data->pp_data;

		usb_external_notify_unregister(&pp_data->usb_ext_noti_nb);
#ifdef CONFIG_TYPEC
		if (!pp_data->ic_data->typec_implemented)
			typec_unregister_port(pp_data->port);
#endif	
		flush_workqueue(pp_data->pdic_wq);
		destroy_workqueue(pp_data->pdic_wq);

		mutex_destroy(&pp_data->p_lock);
		kfree(pp_data);
	}
}
EXPORT_SYMBOL_GPL(pdic_policy_deinit);

