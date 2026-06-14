#ifndef __SEC_QC_PARAM_H__
#define __SEC_QC_PARAM_H__

#define SAPA_KPARAM_MAGIC		0x41504153
#define FMMLOCK_MAGIC_NUM		0x464D4F4E

#define CP_MEM_RESERVE_OFF		0x00000000
#define CP_MEM_RESERVE_ON_1		0x00004350
#define CP_MEM_RESERVE_ON_2		0x00004D42

enum sec_qc_param_index {
	param_index_debuglevel,
	param_index_uartsel,
	param_rory_control,
	param_index_product_device,
	param_cp_debuglevel,
	param_index_sapa,
	param_index_normal_poweroff,
	param_index_wireless_ic,
	param_index_wireless_charging_mode,
	param_index_afc_disable,
	param_index_cp_reserved_mem,
	param_index_api_gpio_test,
	param_index_api_gpio_test_result,
	param_index_reboot_recovery_cause,
	param_index_user_partition_flashed,
	param_index_force_upload_flag,
	param_index_cp_reserved_mem_backup,
	param_index_FMM_lock,
	param_index_dump_sink,
	param_index_fiemap_update,
	param_index_fiemap_result,
	param_index_window_color,
	param_index_VrrStatus,
	param_index_pd_hv_disable,
	param_vib_le_est,
	/* */
	param_num_of_param_index,
};

#if IS_ENABLED(CONFIG_SEC_QC_PARAM)
extern ssize_t sec_qc_param_read_raw(void *buf, size_t len, loff_t pos);
extern ssize_t sec_qc_param_write_raw(const void *buf, size_t len, loff_t pos);
#else
static inline ssize_t sec_qc_param_read_raw(void *buf, size_t len, loff_t pos) { return -ENODEV; }
static inline ssize_t sec_qc_param_write_raw(const void *buf, size_t len, loff_t pos) { return -ENODEV; }
#endif

#endif /* __SEC_QC_PARAM_H__ */
