#ifndef __SEC_AP_PMIC_H__
#define __SEC_AP_PMIC_H__

#define SEC_PON_KEY_CNT	2

struct sec_ap_pmic_info {
	struct device *dev;

	struct notifier_block sec_pm_debug_nb;
	struct delayed_work ws_work;
	unsigned int ws_log_period;

	/* ocp warn */
	int ocp_warn_gpio;
	int ocp_warn_irq;
	int ocpw_cnt;
	int ocpw_cnt_reset_offset;
	ktime_t ocpw_start_time;
	int ocpw_time;	/* accumulated */
	int ocpw_time_reset_offset;
};

enum sec_pon_type {
	SEC_PON_KPDPWR = 0,
	SEC_PON_RESIN,
	SEC_PON_KPDPWR_RESIN,
};

/* for enable/disable manual reset, from retail group's request */
extern int sec_get_s2_reset(enum sec_pon_type type);
extern int sec_set_pm_key_wk_init(enum sec_pon_type type, int en);
extern int sec_get_pm_key_wk_init(enum sec_pon_type type);

extern void msm_gpio_print_enabled(void);
extern void pmic_gpio_sec_dbg_enabled(void);

#if IS_ENABLED(CONFIG_SEC_GPIO_DUMP)
extern void sec_ap_gpio_debug_print(void);
extern void sec_pmic_gpio_debug_print(void);
static bool gpio_dump_enabled;
#endif

#endif /* __SEC_AP_PMIC_H__ */
