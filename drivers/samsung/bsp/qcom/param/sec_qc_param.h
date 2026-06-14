#ifndef __INTERNAL__SEC_QC_PARAM_H__
#define __INTERNAL__SEC_QC_PARAM_H__

#include <linux/debugfs.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/bsp/sec_param.h>
#include <linux/samsung/bsp/sec_sysup.h>	/* deprecated */

struct qc_param_drvdata {
	struct builder bd;
	struct device *param_dev;
	struct sec_param_operations ops;
	const char *bdev_path;
	loff_t negative_offset;
	struct block_device *bdev;
	loff_t offset;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs;
#endif
};

#define QC_PARAM_OFFSET(__member) \
	(offsetof(struct sec_qc_param_data, __member))

#define QC_PARAM_SIZE(__member) \
	(sizeof(((struct sec_qc_param_data *)NULL)->__member))

#define __QC_PARAM_INFO(__member, __verify_input) { \
	.name = #__member, \
	.offset = QC_PARAM_OFFSET(__member), \
	.size = QC_PARAM_SIZE(__member), \
	.verify_input = __verify_input, \
}

#define QC_PARAM_INFO(__index, __member, __verify_input) \
	[__index] = __QC_PARAM_INFO(__member, __verify_input)

struct qc_param_info;

typedef bool (*qc_param_verify_input_t)(const struct qc_param_info *info,
		const void *value);

struct qc_param_info {
	const char *name;
	loff_t offset;
	size_t size;
	qc_param_verify_input_t verify_input;
};

struct sec_qc_param_data {
	unsigned int debuglevel;
	unsigned int uartsel;
	unsigned int rory_control;
	unsigned int product_device;   /* product/dev device */
	unsigned int reserved1;
	unsigned int cp_debuglevel;
	unsigned int reserved2;
	unsigned int sapa[3];
	unsigned int normal_poweroff;
	unsigned int wireless_ic;
	char used0[80];
	char used1[80];
	char used2[80];
	char used3[80];
	char used4[80];
	unsigned int wireless_charging_mode;
	unsigned int afc_disable;
	unsigned int cp_reserved_mem;
	char used5[4];
	char used6[4];
	char reserved8[8];
	char used7[16];
	unsigned int api_gpio_test;
	char api_gpio_test_result[256];
	char reboot_recovery_cause[256];
	unsigned int user_partition_flashed;
	unsigned int force_upload_flag;
	unsigned int cp_reserved_mem_backup;
	unsigned int FMM_lock;
	unsigned int dump_sink;
	unsigned int fiemap_update;
	struct fiemap_p fiemap_result;
	char used8[80];
	char window_color[2];
	char VrrStatus[16];
	unsigned int pd_disable;
	unsigned int vib_le_est;
};

#endif /* __INTERNAL__SEC_QC_PARAM_H__ */
