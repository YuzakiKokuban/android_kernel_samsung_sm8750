// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <clocksource/arm_arch_timer.h>
#include "cam_sensor_util.h"
#include "cam_mem_mgr.h"
#include "cam_res_mgr_api.h"
#include "cam_mem_mgr_api.h"
#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
#include "cam_sensor_dev.h"
#endif
#include "cam_sensor_debug.h"
#include "cam_sensor_debug_gn3.h"
#include "cam_sensor_debug_hp2.h"
#include "cam_sensor_debug_imx564_754.h"
#include "cam_sensor_debug_jn3.h"


#define CAM_SENSOR_PINCTRL_STATE_SLEEP "cam_suspend"
#define CAM_SENSOR_PINCTRL_STATE_DEFAULT "cam_default"

#define VALIDATE_VOLTAGE(min, max, config_val) ((config_val) && \
	(config_val >= min) && (config_val <= max))

#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
int to_do_print_vc__sen_id = 0;
int to_dump_when_sof_freeze__sen_id = 0;
#endif

int cam_sensor_count_elems_i3c_device_id(struct device_node *dev,
	int *num_entries, char *sensor_id_table_str)
{
	if (!num_entries) {
		CAM_ERR(CAM_SENSOR_UTIL, "Num_entries ptr is null");
		return -EINVAL;
	}

	if (!dev) {
		CAM_ERR(CAM_SENSOR_UTIL, "dev ptr is null");
		return -EINVAL;
	}

	if (!sensor_id_table_str) {
		CAM_ERR(CAM_SENSOR_UTIL, "sensor_id_table_str ptr is null");
		return -EINVAL;
	}

	*num_entries = of_property_count_u32_elems(dev, sensor_id_table_str);
	if (*num_entries <= 0) {
		CAM_DBG(CAM_SENSOR_UTIL,
			"Failed while reading the property for dev: %s num_entries:%d",
			dev->full_name, *num_entries);
		return -ENOENT;
	}

	if (*num_entries >= MAX_I3C_DEVICE_ID_ENTRIES) {
		CAM_ERR(CAM_SENSOR_UTIL,
			"Num_entries are more than MAX_I3C_DEVICE_ID_ENTRIES for dev: %s",
			dev->full_name);
		return -ENOMEM;
	}

	return 0;
}

int cam_sensor_fill_i3c_device_id(struct device_node *dev, int num_entries,
	char *sensor_id_table_str, struct i3c_device_id *sensor_i3c_device_id)
{
	int                                      i = 0;
	uint8_t                                  ent_num = 0;
	uint32_t                                 mid;
	uint32_t                                 pid;
	int                                      rc;

	if (!dev) {
		CAM_ERR(CAM_SENSOR_UTIL, "dev ptr is null");
		return -EINVAL;
	}

	if (!sensor_id_table_str) {
		CAM_ERR(CAM_SENSOR_UTIL, "sensor_id_table_str ptr is null");
		return -EINVAL;
	}

	if (!sensor_i3c_device_id) {
		CAM_ERR(CAM_SENSOR_UTIL, "sensor_i3c_device_id ptr is null");
		return -EINVAL;
	}

	while (i < num_entries) {
		if (ent_num >= MAX_I3C_DEVICE_ID_ENTRIES) {
			CAM_WARN(CAM_SENSOR_UTIL,
				"Num_entries are more than MAX_I3C_DEVICE_ID_ENTRIES for dev: %s",
				dev->full_name);
			return -ENOMEM;
		}

		rc = of_property_read_u32_index(dev, sensor_id_table_str, i, &mid);
		if (rc) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed in reading the MID. dev: %s rc: %d",
				dev->full_name, rc);
			return rc;
		}
		i++;

		rc = of_property_read_u32_index(dev, sensor_id_table_str, i, &pid);
		if (rc) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed in reading the PID. dev: %s rc: %d",
				dev->full_name, rc);
			return rc;
		}
		i++;

		CAM_DBG(CAM_SENSOR_UTIL, "dev: %s PID: 0x%x, MID: 0x%x", dev->full_name, pid, mid);

		sensor_i3c_device_id[ent_num].manuf_id = mid;
		sensor_i3c_device_id[ent_num].match_flags = I3C_MATCH_MANUF_AND_PART;
		sensor_i3c_device_id[ent_num].part_id  = pid;
		sensor_i3c_device_id[ent_num].data     = 0;

		ent_num++;
	}

	return 0;
}

static struct i2c_settings_list*
	cam_sensor_get_i2c_ptr(struct i2c_settings_array *i2c_reg_settings,
		uint32_t size)
{
	struct i2c_settings_list *tmp;

	tmp = CAM_MEM_ZALLOC(sizeof(struct i2c_settings_list), GFP_KERNEL);

	if (tmp != NULL)
		list_add_tail(&(tmp->list),
			&(i2c_reg_settings->list_head));
	else
		return NULL;

	tmp->i2c_settings.reg_setting = (struct cam_sensor_i2c_reg_array *)
		vzalloc(size * sizeof(struct cam_sensor_i2c_reg_array));
	if (tmp->i2c_settings.reg_setting == NULL) {
		list_del(&(tmp->list));
		CAM_MEM_FREE(tmp);
		return NULL;
	}
	tmp->i2c_settings.size = size;

	return tmp;
}

int32_t cam_sensor_util_get_current_qtimer_ns(uint64_t *qtime_ns)
{
	uint64_t ticks = 0;
	int32_t rc = 0;

	ticks = arch_timer_read_counter();
	if (ticks == 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "qtimer returned 0, rc:%d", rc);
		return -EINVAL;
	}

	if (qtime_ns != NULL) {
		*qtime_ns = mul_u64_u32_div(ticks,
			QTIMER_MUL_FACTOR, QTIMER_DIV_FACTOR);
		CAM_DBG(CAM_SENSOR_UTIL, "Qtimer time: 0x%lx", *qtime_ns);
	} else {
		CAM_ERR(CAM_SENSOR_UTIL, "NULL pointer passed");
		return -EINVAL;
	}

	return rc;
}

int32_t delete_request(struct i2c_settings_array *i2c_array)
{
	struct i2c_settings_list *i2c_list = NULL, *i2c_next = NULL;
	int32_t rc = 0;

	if (i2c_array == NULL) {
		CAM_ERR(CAM_SENSOR_UTIL, "FATAL:: Invalid argument");
		return -EINVAL;
	}

	list_for_each_entry_safe(i2c_list, i2c_next,
		&(i2c_array->list_head), list) {
		vfree(i2c_list->i2c_settings.reg_setting);
		list_del(&(i2c_list->list));
		CAM_MEM_FREE(i2c_list);
	}
	INIT_LIST_HEAD(&(i2c_array->list_head));
	i2c_array->is_settings_valid = 0;

	return rc;
}

int32_t cam_sensor_handle_delay(
	uint32_t **cmd_buf,
	uint16_t generic_op_code,
	struct i2c_settings_array *i2c_reg_settings,
	uint32_t offset, uint32_t *byte_cnt,
	struct list_head *list_ptr)
{
	int32_t rc = 0;
	struct cam_cmd_unconditional_wait *cmd_uncond_wait =
		(struct cam_cmd_unconditional_wait *) *cmd_buf;
	struct i2c_settings_list *i2c_list = NULL;

	if (list_ptr == NULL) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid list ptr");
		return -EINVAL;
	}

	if (offset > 0) {
		i2c_list =
			list_entry(list_ptr, struct i2c_settings_list, list);
		if (generic_op_code ==
			CAMERA_SENSOR_WAIT_OP_HW_UCND)
			i2c_list->i2c_settings.reg_setting[offset - 1].delay =
				cmd_uncond_wait->delay;
		else
			i2c_list->i2c_settings.delay = cmd_uncond_wait->delay;
		(*cmd_buf) +=
			sizeof(
			struct cam_cmd_unconditional_wait) / sizeof(uint32_t);
		(*byte_cnt) +=
			sizeof(
			struct cam_cmd_unconditional_wait);
	} else {
		CAM_ERR(CAM_SENSOR_UTIL, "Delay Rxed Before any buffer: %d", offset);
		return -EINVAL;
	}

	return rc;
}

int32_t cam_sensor_handle_poll(
	uint32_t **cmd_buf,
	struct i2c_settings_array *i2c_reg_settings,
	uint32_t *byte_cnt, int32_t *offset,
	struct list_head **list_ptr)
{
	struct i2c_settings_list  *i2c_list;
	int32_t rc = 0;
	struct cam_cmd_conditional_wait *cond_wait
		= (struct cam_cmd_conditional_wait *) *cmd_buf;

	i2c_list =
		cam_sensor_get_i2c_ptr(i2c_reg_settings, 1);
	if (!i2c_list || !i2c_list->i2c_settings.reg_setting) {
		CAM_ERR(CAM_SENSOR_UTIL, "Failed in allocating mem for list");
		return -ENOMEM;
	}

	i2c_list->op_code = CAM_SENSOR_I2C_POLL;
	i2c_list->i2c_settings.data_type =
		cond_wait->data_type;
	i2c_list->i2c_settings.addr_type =
		cond_wait->addr_type;
	i2c_list->i2c_settings.reg_setting->reg_addr =
		cond_wait->reg_addr;
	i2c_list->i2c_settings.reg_setting->reg_data =
		cond_wait->reg_data;
	i2c_list->i2c_settings.reg_setting->delay =
		cond_wait->timeout;

	(*cmd_buf) += sizeof(struct cam_cmd_conditional_wait) /
		sizeof(uint32_t);
	(*byte_cnt) += sizeof(struct cam_cmd_conditional_wait);

	*offset = 1;
	*list_ptr = &(i2c_list->list);

	return rc;
}

int32_t cam_sensor_handle_random_write(
	struct cam_cmd_i2c_random_wr *cam_cmd_i2c_random_wr,
	struct i2c_settings_array *i2c_reg_settings,
	uint32_t *cmd_length_in_bytes, int32_t *offset,
	struct list_head **list)
{
	struct i2c_settings_list  *i2c_list;
	int32_t rc = 0, cnt, payload_count;

	payload_count = cam_cmd_i2c_random_wr->header.count;
	i2c_list = cam_sensor_get_i2c_ptr(i2c_reg_settings,
						payload_count);
	if (i2c_list == NULL ||
		i2c_list->i2c_settings.reg_setting == NULL) {
		CAM_ERR(CAM_SENSOR_UTIL, "Failed in allocating i2c_list");
		return -ENOMEM;
	}

	*cmd_length_in_bytes = (sizeof(struct i2c_rdwr_header) +
		sizeof(struct i2c_random_wr_payload) *
		payload_count);
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
	i2c_list->i2c_settings.addr_type =
		cam_cmd_i2c_random_wr->header.addr_type;
	i2c_list->i2c_settings.data_type =
		cam_cmd_i2c_random_wr->header.data_type;

	for (cnt = 0; cnt < payload_count; cnt++) {
		i2c_list->i2c_settings.reg_setting[cnt].reg_addr =
			cam_cmd_i2c_random_wr->random_wr_payload_flex[cnt].reg_addr;
		i2c_list->i2c_settings.reg_setting[cnt].reg_data =
			cam_cmd_i2c_random_wr->random_wr_payload_flex[cnt].reg_data;
		i2c_list->i2c_settings.reg_setting[cnt].data_mask = 0;
	}
	*offset = cnt;
	*list = &(i2c_list->list);

	return rc;
}

int32_t cam_sensor_handle_continuous_write(
	struct cam_cmd_i2c_continuous_wr *cam_cmd_i2c_continuous_wr,
	struct i2c_settings_array *i2c_reg_settings,
	uint32_t *cmd_length_in_bytes, int32_t *offset,
	struct list_head **list)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0, cnt, payload_count;

	payload_count = cam_cmd_i2c_continuous_wr->header.count;
	i2c_list = cam_sensor_get_i2c_ptr(i2c_reg_settings,
						payload_count);
	if (i2c_list == NULL ||
		i2c_list->i2c_settings.reg_setting == NULL) {
		CAM_ERR(CAM_SENSOR_UTIL, "Failed in allocating i2c_list");
		return -ENOMEM;
	}

	*cmd_length_in_bytes = (sizeof(struct i2c_rdwr_header) +
		sizeof(cam_cmd_i2c_continuous_wr->reg_addr) +
		sizeof(struct cam_cmd_read) *
		(payload_count));
	if (cam_cmd_i2c_continuous_wr->header.op_code ==
		CAMERA_SENSOR_I2C_OP_CONT_WR_BRST)
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
	else if (cam_cmd_i2c_continuous_wr->header.op_code ==
		CAMERA_SENSOR_I2C_OP_CONT_WR_SEQN)
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_SEQ;
	else
		return -EINVAL;

	i2c_list->i2c_settings.addr_type =
		cam_cmd_i2c_continuous_wr->header.addr_type;
	i2c_list->i2c_settings.data_type =
		cam_cmd_i2c_continuous_wr->header.data_type;
	i2c_list->i2c_settings.size =
		cam_cmd_i2c_continuous_wr->header.count;

	for (cnt = 0; cnt < payload_count; cnt++) {
		i2c_list->i2c_settings.reg_setting[cnt].reg_addr =
			cam_cmd_i2c_continuous_wr->reg_addr;
		i2c_list->i2c_settings.reg_setting[cnt].reg_data =
			cam_cmd_i2c_continuous_wr->data_read_flex[cnt].reg_data;
		i2c_list->i2c_settings.reg_setting[cnt].data_mask = 0;
	}
	*offset = cnt;
	*list = &(i2c_list->list);

	return rc;
}

static int32_t cam_sensor_get_io_buffer(
	struct cam_buf_io_cfg *io_cfg,
	struct cam_sensor_i2c_reg_setting *i2c_settings)
{
	uintptr_t buf_addr = 0x0;
	size_t buf_size = 0;
	int32_t rc = 0;

	if (io_cfg == NULL || i2c_settings == NULL) {
		CAM_ERR(CAM_SENSOR_UTIL,
			"Invalid args, io buf or i2c settings is NULL");
		return -EINVAL;
	}

	if (io_cfg->direction == CAM_BUF_OUTPUT) {
		rc = cam_mem_get_cpu_buf(io_cfg->mem_handle[0],
			&buf_addr, &buf_size);
		if ((rc < 0) || (!buf_addr)) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"invalid buffer, rc: %d, buf_addr: %pK",
				rc, buf_addr);
			return -EINVAL;
		}
		CAM_DBG(CAM_SENSOR_UTIL,
			"buf_addr: %pK, buf_size: %zu, offsetsize: %d",
			(void *)buf_addr, buf_size, io_cfg->offsets[0]);
		if (io_cfg->offsets[0] >= buf_size) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"invalid size:io_cfg->offsets[0]: %d, buf_size: %d",
				io_cfg->offsets[0], buf_size);
			cam_mem_put_cpu_buf(io_cfg->mem_handle[0]);
			return -EINVAL;
		}
		i2c_settings->read_buff =
			 (uint8_t *)buf_addr + io_cfg->offsets[0];
		i2c_settings->read_buff_len =
			buf_size - io_cfg->offsets[0];
	} else {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid direction: %d",
			io_cfg->direction);
		rc = -EINVAL;
	}
	cam_mem_put_cpu_buf(io_cfg->mem_handle[0]);
	return rc;
}

int32_t cam_sensor_util_write_qtimer_to_io_buffer(
	uint64_t qtime_ns, struct cam_buf_io_cfg *io_cfg)
{
	uintptr_t buf_addr = 0x0, target_buf = 0x0;
	size_t buf_size = 0, target_size = 0;
	int32_t rc = 0;

	if (io_cfg == NULL) {
		CAM_ERR(CAM_SENSOR_UTIL,
			"Invalid args, io buf is NULL");
		return -EINVAL;
	}

	if (io_cfg->direction == CAM_BUF_OUTPUT) {
		rc = cam_mem_get_cpu_buf(io_cfg->mem_handle[0],
			&buf_addr, &buf_size);
		if ((rc < 0) || (!buf_addr)) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"invalid buffer, rc: %d, buf_addr: %pK",
				rc, buf_addr);
			return -EINVAL;
		}
		CAM_DBG(CAM_SENSOR_UTIL,
			"buf_addr: %pK, buf_size: %zu, offsetsize: %d",
			(void *)buf_addr, buf_size, io_cfg->offsets[0]);
		if (io_cfg->offsets[0] >= buf_size) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"invalid size:io_cfg->offsets[0]: %d, buf_size: %d",
				io_cfg->offsets[0], buf_size);
			cam_mem_put_cpu_buf(io_cfg->mem_handle[0]);
			return -EINVAL;
		}

		target_buf  = buf_addr + io_cfg->offsets[0];
		target_size = buf_size - io_cfg->offsets[0];

		if (target_size < sizeof(uint64_t)) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"not enough size for qtimer, target_size:%d",
				target_size);
			cam_mem_put_cpu_buf(io_cfg->mem_handle[0]);
			return -EINVAL;
		}

		memcpy((void *)target_buf, &qtime_ns, sizeof(uint64_t));
		cam_mem_put_cpu_buf(io_cfg->mem_handle[0]);
	} else {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid direction: %d",
			io_cfg->direction);
		rc = -EINVAL;
	}

	return rc;
}

int32_t cam_sensor_handle_random_read(
	struct cam_cmd_i2c_random_rd *cmd_i2c_random_rd,
	struct i2c_settings_array *i2c_reg_settings,
	uint16_t *cmd_length_in_bytes,
	int32_t *offset,
	struct list_head **list,
	struct cam_buf_io_cfg *io_cfg)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0, cnt = 0, payload_count = 0;

	payload_count = cmd_i2c_random_rd->header.count;
	i2c_list = cam_sensor_get_i2c_ptr(i2c_reg_settings,
		payload_count);
	if ((i2c_list == NULL) ||
		(i2c_list->i2c_settings.reg_setting == NULL)) {
		CAM_ERR(CAM_SENSOR_UTIL,
			"Failed in allocating i2c_list: %pK",
			i2c_list);
		return -ENOMEM;
	}

	rc = cam_sensor_get_io_buffer(io_cfg, &(i2c_list->i2c_settings));
	if (rc) {
		CAM_ERR(CAM_SENSOR_UTIL, "Failed to get read buffer: %d", rc);
	} else {
		*cmd_length_in_bytes = sizeof(struct i2c_rdwr_header) +
			(sizeof(struct cam_cmd_read) *
			payload_count);
		i2c_list->op_code = CAM_SENSOR_I2C_READ_RANDOM;
		i2c_list->i2c_settings.addr_type =
			cmd_i2c_random_rd->header.addr_type;
		i2c_list->i2c_settings.data_type =
			cmd_i2c_random_rd->header.data_type;
		i2c_list->i2c_settings.size =
			cmd_i2c_random_rd->header.count;

		for (cnt = 0; cnt < payload_count; cnt++) {
			i2c_list->i2c_settings.reg_setting[cnt].reg_addr =
				cmd_i2c_random_rd->data_read_flex[cnt].reg_data;
		}
		*offset = cnt;
		*list = &(i2c_list->list);
	}

	return rc;
}

int32_t cam_sensor_handle_continuous_read(
	struct cam_cmd_i2c_continuous_rd *cmd_i2c_continuous_rd,
	struct i2c_settings_array *i2c_reg_settings,
	uint16_t *cmd_length_in_bytes, int32_t *offset,
	struct list_head **list,
	struct cam_buf_io_cfg *io_cfg)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0, cnt = 0;

	i2c_list = cam_sensor_get_i2c_ptr(i2c_reg_settings, 1);
	if ((i2c_list == NULL) ||
		(i2c_list->i2c_settings.reg_setting == NULL)) {
		CAM_ERR(CAM_SENSOR_UTIL,
			"Failed in allocating i2c_list: %pK",
			i2c_list);
		return -ENOMEM;
	}

	rc = cam_sensor_get_io_buffer(io_cfg, &(i2c_list->i2c_settings));
	if (rc) {
		CAM_ERR(CAM_SENSOR_UTIL, "Failed to get read buffer: %d", rc);
	} else {
		*cmd_length_in_bytes = sizeof(struct cam_cmd_i2c_continuous_rd);
		i2c_list->op_code = CAM_SENSOR_I2C_READ_SEQ;

		i2c_list->i2c_settings.addr_type =
			cmd_i2c_continuous_rd->header.addr_type;
		i2c_list->i2c_settings.data_type =
			cmd_i2c_continuous_rd->header.data_type;
		i2c_list->i2c_settings.size =
			cmd_i2c_continuous_rd->header.count;
		i2c_list->i2c_settings.reg_setting[0].reg_addr =
			cmd_i2c_continuous_rd->reg_addr;

		*offset = cnt;
		*list = &(i2c_list->list);
	}

	return rc;
}

static int cam_sensor_handle_slave_info(
	struct camera_io_master *io_master,
	uint32_t *cmd_buf)
{
	int rc = 0;
	struct cam_cmd_i2c_info *i2c_info = (struct cam_cmd_i2c_info *)cmd_buf;

	if (io_master == NULL || cmd_buf == NULL) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid args");
		return -EINVAL;
	}

	switch (io_master->master_type) {
	case CCI_MASTER:
		io_master->cci_client->sid = (i2c_info->slave_addr >> 1);
		io_master->cci_client->i2c_freq_mode = i2c_info->i2c_freq_mode;
		break;

	case I2C_MASTER:
		io_master->qup_client->i2c_client->addr = i2c_info->slave_addr;
		break;

	case SPI_MASTER:
		break;

	default:
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid master type: %d",
			io_master->master_type);
		rc = -EINVAL;
		break;
	}

	return rc;
}

/**
 * Name : cam_sensor_i2c_command_parser
 * Description : Parse CSL CCI packet and apply register settings
 * Parameters :  io_master        input  master information
 *               i2c_reg_settings output register settings to fill
 *               cmd_desc         input  command description
 *               num_cmd_buffers  input  number of command buffers to process
 *               io_cfg           input  buffer details for read operation only
 * Description :
 * Handle multiple I2C RD/WR and WAIT cmd formats in one command
 * buffer, for example, a command buffer of m x RND_WR + 1 x HW_
 * WAIT + n x RND_WR with num_cmd_buf = 1. Do not exepect RD/WR
 * with different cmd_type and op_code in one command buffer.
 */
int cam_sensor_i2c_command_parser(
	struct camera_io_master *io_master,
	struct i2c_settings_array *i2c_reg_settings,
	struct cam_cmd_buf_desc   *cmd_desc,
	int32_t num_cmd_buffers,
	struct cam_buf_io_cfg *io_cfg)
{
	int16_t                   rc = 0, i = 0;
	size_t                    len_of_buff = 0;
	uintptr_t                 generic_ptr;
	uint16_t                  cmd_length_in_bytes = 0;
	size_t                    remain_len = 0;
	size_t                    tot_size = 0;

	for (i = 0; i < num_cmd_buffers; i++) {
		uint32_t                  *cmd_buf = NULL;
		struct common_header      *cmm_hdr;
		uint16_t                  generic_op_code;
		uint32_t                  byte_cnt = 0;
		uint32_t                  j = 0;
		struct list_head          *list = NULL;

		/*
		 * It is not expected the same settings to
		 * be spread across multiple cmd buffers
		 */
		CAM_DBG(CAM_SENSOR_UTIL, "Total cmd Buf in Bytes: %d",
			cmd_desc[i].length);

		if (!cmd_desc[i].length)
			continue;

		rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
			&generic_ptr, &len_of_buff);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"cmd hdl failed:%d, Err: %d, Buffer_len: %zd",
				cmd_desc[i].mem_handle, rc, len_of_buff);
			return rc;
		}

		remain_len = len_of_buff;
		if ((len_of_buff < sizeof(struct common_header)) ||
			(cmd_desc[i].offset >
			(len_of_buff - sizeof(struct common_header)))) {
			CAM_ERR(CAM_SENSOR_UTIL, "buffer provided too small");
			cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
			return -EINVAL;
		}
		cmd_buf = (uint32_t *)generic_ptr;
		cmd_buf += cmd_desc[i].offset / sizeof(uint32_t);

		remain_len -= cmd_desc[i].offset;
		if (remain_len < cmd_desc[i].length) {
			CAM_ERR(CAM_SENSOR_UTIL, "buffer provided too small");
			cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
			return -EINVAL;
		}

		while (byte_cnt < cmd_desc[i].length) {
			if ((remain_len - byte_cnt) <
				sizeof(struct common_header)) {
				CAM_ERR(CAM_SENSOR_UTIL, "Not enough buffer");
				rc = -EINVAL;
				goto end;
			}
			cmm_hdr = (struct common_header *)cmd_buf;
			generic_op_code = cmm_hdr->fifth_byte;
			switch (cmm_hdr->cmd_type) {
			case CAMERA_SENSOR_CMD_TYPE_I2C_RNDM_WR: {
				uint32_t cmd_length_in_bytes   = 0;
				struct cam_cmd_i2c_random_wr
					*cam_cmd_i2c_random_wr =
					(struct cam_cmd_i2c_random_wr *)cmd_buf;

				if ((remain_len - byte_cnt) <
					sizeof(struct cam_cmd_i2c_random_wr)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer provided");
					rc = -EINVAL;
					goto end;
				}
				tot_size = sizeof(struct i2c_rdwr_header) +
					(sizeof(struct i2c_random_wr_payload) *
					cam_cmd_i2c_random_wr->header.count);

				if (tot_size > (remain_len - byte_cnt)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer provided");
					rc = -EINVAL;
					goto end;
				}

				rc = cam_sensor_handle_random_write(
					cam_cmd_i2c_random_wr,
					i2c_reg_settings,
					&cmd_length_in_bytes, &j, &list);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Failed in random write %d", rc);
					rc = -EINVAL;
					goto end;
				}

				cmd_buf += cmd_length_in_bytes /
					sizeof(uint32_t);
				byte_cnt += cmd_length_in_bytes;
				break;
			}
			case CAMERA_SENSOR_CMD_TYPE_I2C_CONT_WR: {
				uint32_t cmd_length_in_bytes   = 0;
				struct cam_cmd_i2c_continuous_wr
				*cam_cmd_i2c_continuous_wr =
				(struct cam_cmd_i2c_continuous_wr *)
				cmd_buf;

				if ((remain_len - byte_cnt) <
				sizeof(struct cam_cmd_i2c_continuous_wr)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer provided");
					rc = -EINVAL;
					goto end;
				}

				tot_size = sizeof(struct i2c_rdwr_header) +
				sizeof(cam_cmd_i2c_continuous_wr->reg_addr) +
				(sizeof(struct cam_cmd_read) *
				cam_cmd_i2c_continuous_wr->header.count);

				if (tot_size > (remain_len - byte_cnt)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer provided");
					rc = -EINVAL;
					goto end;
				}

				rc = cam_sensor_handle_continuous_write(
					cam_cmd_i2c_continuous_wr,
					i2c_reg_settings,
					&cmd_length_in_bytes, &j, &list);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Failed in continuous write %d", rc);
					goto end;
				}

				cmd_buf += cmd_length_in_bytes /
					sizeof(uint32_t);
				byte_cnt += cmd_length_in_bytes;
				break;
			}
			case CAMERA_SENSOR_CMD_TYPE_WAIT: {
				if ((((generic_op_code == CAMERA_SENSOR_WAIT_OP_HW_UCND) ||
					(generic_op_code == CAMERA_SENSOR_WAIT_OP_SW_UCND)) &&
					((remain_len - byte_cnt) <
					sizeof(struct cam_cmd_unconditional_wait))) ||
					((generic_op_code == CAMERA_SENSOR_WAIT_OP_COND) &&
					((remain_len - byte_cnt) <
					sizeof(struct cam_cmd_conditional_wait)))) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer space");
					rc = -EINVAL;
					goto end;
				}

				if (generic_op_code ==
					CAMERA_SENSOR_WAIT_OP_HW_UCND ||
					generic_op_code ==
						CAMERA_SENSOR_WAIT_OP_SW_UCND) {
					rc = cam_sensor_handle_delay(
						&cmd_buf, generic_op_code,
						i2c_reg_settings, j, &byte_cnt,
						list);
					if (rc < 0) {
						CAM_ERR(CAM_SENSOR_UTIL,
							"delay hdl failed: %d",
							rc);
						goto end;
					}

				} else if (generic_op_code ==
					CAMERA_SENSOR_WAIT_OP_COND) {
					rc = cam_sensor_handle_poll(
						&cmd_buf, i2c_reg_settings,
						&byte_cnt, &j, &list);
					if (rc < 0) {
						CAM_ERR(CAM_SENSOR_UTIL,
							"Random read fail: %d",
							rc);
						goto end;
					}
				} else {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Wrong Wait Command: %d",
						generic_op_code);
					rc = -EINVAL;
					goto end;
				}
				break;
			}
			case CAMERA_SENSOR_CMD_TYPE_I2C_INFO: {
				if (remain_len - byte_cnt <
					sizeof(struct cam_cmd_i2c_info)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer space");
					rc = -EINVAL;
					goto end;
				}
				rc = cam_sensor_handle_slave_info(
					io_master, cmd_buf);
				if (rc) {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Handle slave info failed with rc: %d",
					rc);
					goto end;
				}
				cmd_length_in_bytes =
					sizeof(struct cam_cmd_i2c_info);
				cmd_buf +=
					cmd_length_in_bytes / sizeof(uint32_t);
				byte_cnt += cmd_length_in_bytes;
				break;
			}
			case CAMERA_SENSOR_CMD_TYPE_I2C_RNDM_RD: {
				uint16_t cmd_length_in_bytes   = 0;
				struct cam_cmd_i2c_random_rd *i2c_random_rd =
				(struct cam_cmd_i2c_random_rd *)cmd_buf;

				if (remain_len - byte_cnt <
					sizeof(struct cam_cmd_i2c_random_rd)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer space");
					rc = -EINVAL;
					goto end;
				}

				tot_size = sizeof(struct i2c_rdwr_header) +
					(sizeof(struct cam_cmd_read) *
					i2c_random_rd->header.count);

				if (tot_size > (remain_len - byte_cnt)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer provided %d, %d, %d",
						tot_size, remain_len, byte_cnt);
					rc = -EINVAL;
					goto end;
				}

				rc = cam_sensor_handle_random_read(
					i2c_random_rd,
					i2c_reg_settings,
					&cmd_length_in_bytes, &j, &list,
					io_cfg);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Failed in random read %d", rc);
					goto end;
				}

				cmd_buf += cmd_length_in_bytes /
					sizeof(uint32_t);
				byte_cnt += cmd_length_in_bytes;
				break;
			}
			case CAMERA_SENSOR_CMD_TYPE_I2C_CONT_RD: {
				uint16_t cmd_length_in_bytes   = 0;
				struct cam_cmd_i2c_continuous_rd
				*i2c_continuous_rd =
				(struct cam_cmd_i2c_continuous_rd *)cmd_buf;

				if (remain_len - byte_cnt <
					sizeof(struct cam_cmd_i2c_continuous_rd)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer space");
					rc = -EINVAL;
					goto end;
				}

				tot_size =
				sizeof(struct cam_cmd_i2c_continuous_rd);

				if (tot_size > (remain_len - byte_cnt)) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Not enough buffer provided %d, %d, %d",
						tot_size, remain_len, byte_cnt);
					rc = -EINVAL;
					goto end;
				}

				rc = cam_sensor_handle_continuous_read(
					i2c_continuous_rd,
					i2c_reg_settings,
					&cmd_length_in_bytes, &j, &list,
					io_cfg);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Failed in continuous read %d", rc);
					goto end;
				}

				cmd_buf += cmd_length_in_bytes /
					sizeof(uint32_t);
				byte_cnt += cmd_length_in_bytes;
				break;
			}
			default:
				CAM_ERR(CAM_SENSOR_UTIL, "Invalid Command Type:%d",
					 cmm_hdr->cmd_type);
				rc = -EINVAL;
				goto end;
			}
		}
		i2c_reg_settings->is_settings_valid = 1;
		cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
	}

	return rc;

end:
	cam_mem_put_cpu_buf(cmd_desc[i].mem_handle);
	return rc;
}

int cam_sensor_util_i2c_apply_setting(
	struct camera_io_master *io_master_info,
	struct i2c_settings_list *i2c_list)
{
	int32_t rc = 0;
	uint32_t i, size;

	switch (i2c_list->op_code) {
	case CAM_SENSOR_I2C_WRITE_RANDOM: {
		rc = camera_io_dev_write(io_master_info,
			&(i2c_list->i2c_settings));
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Failed to random write I2C settings: %d",
				rc);
			return rc;
		}
	break;
	}
	case CAM_SENSOR_I2C_WRITE_SEQ: {
		rc = camera_io_dev_write_continuous(
			io_master_info, &(i2c_list->i2c_settings), CAM_SENSOR_I2C_WRITE_SEQ);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Failed to seq write I2C settings: %d",
				rc);
			return rc;
		}
	break;
	}
	case CAM_SENSOR_I2C_WRITE_BURST: {
		rc = camera_io_dev_write_continuous(
			io_master_info, &(i2c_list->i2c_settings), CAM_SENSOR_I2C_WRITE_BURST);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Failed to burst write I2C settings: %d",
				rc);
			return rc;
		}
	break;
	}
	case CAM_SENSOR_I2C_POLL: {
		size = i2c_list->i2c_settings.size;
		for (i = 0; i < size; i++) {
			rc = camera_io_dev_poll(
			io_master_info,
			i2c_list->i2c_settings.reg_setting[i].reg_addr,
			i2c_list->i2c_settings.reg_setting[i].reg_data,
			i2c_list->i2c_settings.reg_setting[i].data_mask,
			i2c_list->i2c_settings.addr_type,
			i2c_list->i2c_settings.data_type,
			i2c_list->i2c_settings.reg_setting[i].delay);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL,
					"i2c poll apply setting Fail: %d", rc);
				return rc;
			}
		}
	break;
	}
	default:
		CAM_ERR(CAM_SENSOR_UTIL, "Wrong Opcode: %d", i2c_list->op_code);
		rc = -EINVAL;
	break;
	}

	return rc;
}

int32_t cam_sensor_i2c_read_data(
	struct i2c_settings_array *i2c_settings,
	struct camera_io_master *io_master_info)
{
	int32_t                   rc = 0;
	struct i2c_settings_list  *i2c_list;
	uint32_t                  cnt = 0;
	uint8_t                   *read_buff = NULL;
	uint32_t                  buff_length = 0;
	uint32_t                  read_length = 0;

	list_for_each_entry(i2c_list,
		&(i2c_settings->list_head), list) {
		read_buff = i2c_list->i2c_settings.read_buff;
		buff_length = i2c_list->i2c_settings.read_buff_len;
		if ((read_buff == NULL) || (buff_length == 0)) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Invalid input buffer, buffer: %pK, length: %d",
				read_buff, buff_length);
			return -EINVAL;
		}

		if (i2c_list->op_code == CAM_SENSOR_I2C_READ_RANDOM) {
			read_length = i2c_list->i2c_settings.data_type *
				i2c_list->i2c_settings.size;
			if ((read_length > buff_length) ||
				(read_length < i2c_list->i2c_settings.size)) {
				CAM_ERR(CAM_SENSOR_UTIL,
				"Invalid size, readLen:%d, bufLen:%d, size: %d",
				read_length, buff_length,
				i2c_list->i2c_settings.size);
				return -EINVAL;
			}
			for (cnt = 0; cnt < (i2c_list->i2c_settings.size);
				cnt++) {
				struct cam_sensor_i2c_reg_array *reg_setting =
				&(i2c_list->i2c_settings.reg_setting[cnt]);
				rc = camera_io_dev_read(io_master_info,
					reg_setting->reg_addr,
					&reg_setting->reg_data,
					i2c_list->i2c_settings.addr_type,
					i2c_list->i2c_settings.data_type,
					false);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Failed: random read I2C settings: %d",
					rc);
					return rc;
				}
				if (i2c_list->i2c_settings.data_type <
					CAMERA_SENSOR_I2C_TYPE_MAX) {
					memcpy(read_buff,
					&reg_setting->reg_data,
					i2c_list->i2c_settings.data_type);
					read_buff +=
					i2c_list->i2c_settings.data_type;
				}
			}
		} else if (i2c_list->op_code == CAM_SENSOR_I2C_READ_SEQ) {
			read_length = i2c_list->i2c_settings.size;
			if (read_length > buff_length) {
				CAM_ERR(CAM_SENSOR_UTIL,
				"Invalid buffer size, readLen: %d, bufLen: %d",
				read_length, buff_length);
				return -EINVAL;
			}
			rc = camera_io_dev_read_seq(
				io_master_info,
				i2c_list->i2c_settings.reg_setting[0].reg_addr,
				read_buff,
				i2c_list->i2c_settings.addr_type,
				i2c_list->i2c_settings.data_type,
				i2c_list->i2c_settings.size);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL,
					"failed: seq read I2C settings: %d",
					rc);
				return rc;
			}
		}
	}

	return rc;
}

int32_t msm_camera_fill_vreg_params(
	struct cam_hw_soc_info *soc_info,
	struct cam_sensor_power_setting *power_setting,
	uint16_t power_setting_size)
{
	int32_t rc = 0, j = 0, i = 0;
	int num_vreg;

	/* Validate input parameters */
	if (!soc_info || !power_setting) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed: soc_info %pK power_setting %pK",
			soc_info, power_setting);
		return -EINVAL;
	}

	num_vreg = soc_info->num_rgltr;

	if ((num_vreg <= 0) || (num_vreg > CAM_SOC_MAX_REGULATOR)) {
		if (debug_bypass_drivers & CAM_BYPASS_RGLTR)
			CAM_DBG(CAM_SENSOR_UTIL, "Bypass parameter check num_vreg %d",
				num_vreg);
		else {
			CAM_ERR(CAM_SENSOR_UTIL, "failed: num_vreg %d", num_vreg);
			return -EINVAL;
		}
	}

	for (i = 0; i < power_setting_size; i++) {

		if (power_setting[i].seq_type < SENSOR_MCLK ||
			power_setting[i].seq_type >= SENSOR_SEQ_TYPE_MAX) {
			CAM_ERR(CAM_SENSOR_UTIL, "failed: Invalid Seq type: %d",
				power_setting[i].seq_type);
			return -EINVAL;
		}

		power_setting[i].valid_config = false;

		switch (power_setting[i].seq_type) {
		case SENSOR_VDIG:
			for (j = 0; j < num_vreg; j++) {
				if (!strcmp(soc_info->rgltr_name[j],
					"cam_vdig")) {

					CAM_DBG(CAM_SENSOR_UTIL,
						"i: %d j: %d cam_vdig", i, j);
					power_setting[i].seq_val = j;

					if (VALIDATE_VOLTAGE(
						soc_info->rgltr_min_volt[j],
						soc_info->rgltr_max_volt[j],
						power_setting[i].config_val))
						power_setting[i].valid_config = true;

					break;
				}
			}
			if (j == num_vreg)
				power_setting[i].seq_val = INVALID_VREG;
			break;

		case SENSOR_VIO:
			for (j = 0; j < num_vreg; j++) {

				if (!strcmp(soc_info->rgltr_name[j],
					"cam_vio")) {
					CAM_DBG(CAM_SENSOR_UTIL,
						"i: %d j: %d cam_vio", i, j);
					power_setting[i].seq_val = j;

					if (VALIDATE_VOLTAGE(
						soc_info->rgltr_min_volt[j],
						soc_info->rgltr_max_volt[j],
						power_setting[i].config_val))
						power_setting[i].valid_config = true;

					break;
				}

			}
			if (j == num_vreg)
				power_setting[i].seq_val = INVALID_VREG;
			break;

		case SENSOR_VANA:
			for (j = 0; j < num_vreg; j++) {

				if (!strcmp(soc_info->rgltr_name[j],
					"cam_vana")) {
					CAM_DBG(CAM_SENSOR_UTIL,
						"i: %d j: %d cam_vana", i, j);
					power_setting[i].seq_val = j;

					if (VALIDATE_VOLTAGE(
						soc_info->rgltr_min_volt[j],
						soc_info->rgltr_max_volt[j],
						power_setting[i].config_val))
						power_setting[i].valid_config = true;

					break;
				}

			}
			if (j == num_vreg)
				power_setting[i].seq_val = INVALID_VREG;
			break;

		case SENSOR_VANA1:
			for (j = 0; j < num_vreg; j++) {
				if (!strcmp(soc_info->rgltr_name[j],
					"cam_vana1")) {
					CAM_DBG(CAM_SENSOR_UTIL,
						"i: %d j: %d cam_vana1", i, j);
					power_setting[i].seq_val = j;

					if (VALIDATE_VOLTAGE(
						soc_info->rgltr_min_volt[j],
						soc_info->rgltr_max_volt[j],
						power_setting[i].config_val))
						power_setting[i].valid_config = true;

					break;
				}
			}
			if (j == num_vreg)
				power_setting[i].seq_val = INVALID_VREG;
			break;

		case SENSOR_VAF:
			for (j = 0; j < num_vreg; j++) {

				if (!strcmp(soc_info->rgltr_name[j],
					"cam_vaf")) {
					CAM_DBG(CAM_SENSOR_UTIL,
						"i: %d j: %d cam_vaf", i, j);
					power_setting[i].seq_val = j;

					if (VALIDATE_VOLTAGE(
						soc_info->rgltr_min_volt[j],
						soc_info->rgltr_max_volt[j],
						power_setting[i].config_val))
						power_setting[i].valid_config = true;

					break;
				}

			}
			if (j == num_vreg)
				power_setting[i].seq_val = INVALID_VREG;
			break;

		case SENSOR_CUSTOM_REG1:
			for (j = 0; j < num_vreg; j++) {

				if (!strcmp(soc_info->rgltr_name[j],
					"cam_v_custom1")) {
					CAM_DBG(CAM_SENSOR_UTIL,
						"i:%d j:%d cam_vcustom1", i, j);
					power_setting[i].seq_val = j;

					if (VALIDATE_VOLTAGE(
						soc_info->rgltr_min_volt[j],
						soc_info->rgltr_max_volt[j],
						power_setting[i].config_val))
						power_setting[i].valid_config = true;

					break;
				}

			}
			if (j == num_vreg)
				power_setting[i].seq_val = INVALID_VREG;
			break;
		case SENSOR_CUSTOM_REG2:
			for (j = 0; j < num_vreg; j++) {

				if (!strcmp(soc_info->rgltr_name[j],
					"cam_v_custom2")) {
					CAM_DBG(CAM_SENSOR_UTIL,
						"i:%d j:%d cam_vcustom2", i, j);
					power_setting[i].seq_val = j;

					if (VALIDATE_VOLTAGE(
						soc_info->rgltr_min_volt[j],
						soc_info->rgltr_max_volt[j],
						power_setting[i].config_val))
						power_setting[i].valid_config = true;

					break;
				}
			}
			if (j == num_vreg)
				power_setting[i].seq_val = INVALID_VREG;
			break;
		default:
			break;
		}
	}

	return rc;
}

int cam_sensor_util_request_gpio_table(
		struct cam_hw_soc_info *soc_info, int gpio_en)
{
	int rc = 0, i = 0;
	uint8_t size = 0;
	struct cam_soc_gpio_data *gpio_conf =
			soc_info->gpio_data;
	struct gpio *gpio_tbl = NULL;

	if (!gpio_conf) {
		CAM_DBG(CAM_SENSOR_UTIL, "No GPIO data");
		return 0;
	}

	if (gpio_conf->cam_gpio_common_tbl_size <= 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "No GPIO entry");
		return -EINVAL;
	}

	gpio_tbl = gpio_conf->cam_gpio_req_tbl;
	size = gpio_conf->cam_gpio_req_tbl_size;

	if (!gpio_tbl || !size) {
		CAM_ERR(CAM_SENSOR_UTIL, "invalid gpio_tbl %pK / size %d",
			gpio_tbl, size);
		return -EINVAL;
	}

	for (i = 0; i < size; i++) {
		CAM_DBG(CAM_SENSOR_UTIL, "%s%d, i: %d, gpio %d dir %lld",
			soc_info->dev_name, soc_info->index, i,
			gpio_tbl[i].gpio, gpio_tbl[i].flags);
	}

	if (gpio_en) {
		for (i = 0; i < size; i++) {
			rc = cam_res_mgr_gpio_request(soc_info->dev,
					gpio_tbl[i].gpio,
					gpio_tbl[i].flags, gpio_tbl[i].label);
			if (rc) {
				/*
				 * After GPIO request fails, contine to
				 * apply new gpios, outout a error message
				 * for driver bringup debug
				 */
				CAM_ERR(CAM_SENSOR_UTIL, "gpio %d:%s request fails",
					gpio_tbl[i].gpio, gpio_tbl[i].label);
				goto request_fail;
			}
		}
	} else {
		cam_res_mgr_gpio_free_arry(soc_info->dev, gpio_tbl, size);
	}

	return rc;

request_fail:
	if (gpio_en && (i > 0))
		cam_res_mgr_gpio_free_arry(soc_info->dev, gpio_tbl, i);
	return rc;
}

bool cam_sensor_util_check_gpio_is_shared(
	struct cam_hw_soc_info *soc_info)
{
	int rc = 0;
	uint8_t size = 0;
	struct cam_soc_gpio_data *gpio_conf =
			soc_info->gpio_data;
	struct gpio *gpio_tbl = NULL;

	if (!gpio_conf) {
		CAM_DBG(CAM_SENSOR_UTIL, "No GPIO data");
		return false;
	}

	if (gpio_conf->cam_gpio_common_tbl_size <= 0) {
		CAM_DBG(CAM_SENSOR_UTIL, "No GPIO entry");
		return false;
	}

	gpio_tbl = gpio_conf->cam_gpio_req_tbl;
	size = gpio_conf->cam_gpio_req_tbl_size;

	if (!gpio_tbl || !size) {
		CAM_ERR(CAM_SENSOR_UTIL, "invalid gpio_tbl %pK / size %d",
			gpio_tbl, size);
		return false;
	}

	rc = cam_res_mgr_util_check_if_gpio_is_shared(
		gpio_tbl, size);
	if (!rc) {
		CAM_DBG(CAM_SENSOR_UTIL,
			"dev: %s don't have shared gpio resources",
			soc_info->dev_name);
		return false;
	}

	return true;
}

static int32_t cam_sensor_validate(void *ptr, size_t remain_buf)
{
	struct common_header *cmm_hdr = (struct common_header *)ptr;
	size_t validate_size = 0;

	if (remain_buf < sizeof(struct common_header))
		return -EINVAL;

	if (cmm_hdr->cmd_type == CAMERA_SENSOR_CMD_TYPE_PWR_UP ||
		cmm_hdr->cmd_type == CAMERA_SENSOR_CMD_TYPE_PWR_DOWN)
		validate_size = sizeof(struct cam_cmd_power);
	else if (cmm_hdr->cmd_type == CAMERA_SENSOR_CMD_TYPE_WAIT)
		validate_size = sizeof(struct cam_cmd_unconditional_wait);

	if (remain_buf < validate_size) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid cmd_buf len %zu min %zu",
			remain_buf, validate_size);
		return -EINVAL;
	}
	return 0;
}

int32_t cam_sensor_update_power_settings(void *cmd_buf,
	uint32_t cmd_length, struct cam_sensor_power_ctrl_t *power_info,
	size_t cmd_buf_len)
{
	int32_t rc = 0, tot_size = 0, last_cmd_type = 0;
	int32_t i = 0, pwr_up = 0, pwr_down = 0;
	struct cam_sensor_power_setting *pwr_settings;
	void *ptr = cmd_buf, *scr;
	struct common_header *cmm_hdr = (struct common_header *)cmd_buf;
	struct cam_cmd_power *pwr_cmd =
		CAM_MEM_ZALLOC(sizeof(struct cam_cmd_power), GFP_KERNEL);
	if (!pwr_cmd)
		return -ENOMEM;
	memcpy(pwr_cmd, cmd_buf, sizeof(struct cam_cmd_power));

	if (!pwr_cmd || !cmd_length || cmd_buf_len < (size_t)cmd_length ||
		cam_sensor_validate(cmd_buf, cmd_buf_len)) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid Args: pwr_cmd %pK, cmd_length: %d",
			pwr_cmd, cmd_length);
		rc = -EINVAL;
		goto free_power_command;
	}

	power_info->power_setting_size = 0;
	power_info->power_setting =
		CAM_MEM_ZALLOC(sizeof(struct cam_sensor_power_setting) *
			MAX_POWER_CONFIG, GFP_KERNEL);
	if (!power_info->power_setting) {
		rc = -ENOMEM;
		goto free_power_command;
	}

	power_info->power_down_setting_size = 0;
	power_info->power_down_setting =
		CAM_MEM_ZALLOC(sizeof(struct cam_sensor_power_setting) *
			MAX_POWER_CONFIG, GFP_KERNEL);
	if (!power_info->power_down_setting) {
		CAM_MEM_FREE(power_info->power_setting);
		power_info->power_setting = NULL;
		power_info->power_setting_size = 0;
		rc = -ENOMEM;
		goto free_power_command;
	}

	while (tot_size < cmd_length) {
		if (cam_sensor_validate(ptr, (cmd_length - tot_size))) {
			rc = -EINVAL;
			goto free_power_settings;
		}
		if (cmm_hdr->cmd_type ==
			CAMERA_SENSOR_CMD_TYPE_PWR_UP) {
			struct cam_cmd_power *pwr_cmd =
				(struct cam_cmd_power *)ptr;

			if ((U16_MAX - power_info->power_setting_size) <
				pwr_cmd->count) {
				CAM_ERR(CAM_SENSOR_UTIL, "ERR: Overflow occurs");
				rc = -EINVAL;
				goto free_power_settings;
			}

			power_info->power_setting_size += pwr_cmd->count;
			if ((power_info->power_setting_size > MAX_POWER_CONFIG)
				|| (pwr_cmd->count >= SENSOR_SEQ_TYPE_MAX)) {
				CAM_ERR(CAM_SENSOR_UTIL,
				"pwr_up setting size %d, pwr_cmd->count: %d",
					power_info->power_setting_size,
					pwr_cmd->count);
				rc = -EINVAL;
				goto free_power_settings;
			}
			scr = ptr + sizeof(struct cam_cmd_power);
			tot_size = tot_size + sizeof(struct cam_cmd_power);

			if (pwr_cmd->count == 0)
				CAM_WARN(CAM_SENSOR_UTIL, "pwr_up_size is zero");

			for (i = 0; i < pwr_cmd->count; i++, pwr_up++) {
				power_info->power_setting[pwr_up].seq_type =
				pwr_cmd->power_settings_flex[i].power_seq_type;
				power_info->power_setting[pwr_up].config_val =
				pwr_cmd->power_settings_flex[i].config_val_low;
				power_info->power_setting[pwr_up].delay = 0;
				if (i) {
					scr = scr +
						sizeof(
						struct cam_power_settings);
					tot_size = tot_size +
						sizeof(
						struct cam_power_settings);
				}
				if (tot_size > cmd_length) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Error: Cmd Buffer is wrong");
					rc = -EINVAL;
					goto free_power_settings;
				}
				CAM_DBG(CAM_SENSOR_UTIL,
				"Seq Type[%d]: %d Config_val: %ld", pwr_up,
				power_info->power_setting[pwr_up].seq_type,
				power_info->power_setting[pwr_up].config_val);
			}
			last_cmd_type = CAMERA_SENSOR_CMD_TYPE_PWR_UP;
			ptr = (void *) scr;
			cmm_hdr = (struct common_header *)ptr;
		} else if (cmm_hdr->cmd_type == CAMERA_SENSOR_CMD_TYPE_WAIT) {
			struct cam_cmd_unconditional_wait *wait_cmd =
				(struct cam_cmd_unconditional_wait *)ptr;
			if ((wait_cmd->op_code ==
				CAMERA_SENSOR_WAIT_OP_SW_UCND) &&
				(last_cmd_type ==
				CAMERA_SENSOR_CMD_TYPE_PWR_UP)) {
				if (pwr_up > 0) {
					pwr_settings =
					&power_info->power_setting[pwr_up - 1];
					pwr_settings->delay +=
						wait_cmd->delay;
				} else {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Delay is expected only after valid power up setting");
				}
			} else if ((wait_cmd->op_code ==
				CAMERA_SENSOR_WAIT_OP_SW_UCND) &&
				(last_cmd_type ==
				CAMERA_SENSOR_CMD_TYPE_PWR_DOWN)) {
				if (pwr_down > 0) {
					pwr_settings =
					&power_info->power_down_setting[
						pwr_down - 1];
					pwr_settings->delay +=
						wait_cmd->delay;
				} else {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Delay is expected only after valid power up setting");
				}
			} else {
				CAM_DBG(CAM_SENSOR_UTIL, "Invalid op code: %d",
					wait_cmd->op_code);
			}

			tot_size = tot_size +
				sizeof(struct cam_cmd_unconditional_wait);
			if (tot_size > cmd_length) {
				CAM_ERR(CAM_SENSOR_UTIL, "Command Buffer is wrong");
				return -EINVAL;
			}
			scr = (void *) (wait_cmd);
			ptr = (void *)
				(scr +
				sizeof(struct cam_cmd_unconditional_wait));
			CAM_DBG(CAM_SENSOR_UTIL, "ptr: %pK sizeof: %d Next: %pK",
				scr, (int32_t)sizeof(
				struct cam_cmd_unconditional_wait), ptr);

			cmm_hdr = (struct common_header *)ptr;
		} else if (cmm_hdr->cmd_type ==
			CAMERA_SENSOR_CMD_TYPE_PWR_DOWN) {
			struct cam_cmd_power *pwr_cmd =
				(struct cam_cmd_power *)ptr;

			scr = ptr + sizeof(struct cam_cmd_power);
			tot_size = tot_size + sizeof(struct cam_cmd_power);
			if ((U16_MAX - power_info->power_down_setting_size) <
				pwr_cmd->count) {
				CAM_ERR(CAM_SENSOR_UTIL, "ERR: Overflow");
				rc = -EINVAL;
				goto free_power_settings;
			}

			power_info->power_down_setting_size += pwr_cmd->count;
			if ((power_info->power_down_setting_size >
				MAX_POWER_CONFIG) || (pwr_cmd->count >=
				SENSOR_SEQ_TYPE_MAX)) {
				CAM_ERR(CAM_SENSOR_UTIL,
				"pwr_down_setting_size %d, pwr_cmd->count: %d",
					power_info->power_down_setting_size,
					pwr_cmd->count);
				rc = -EINVAL;
				goto free_power_settings;
			}

			if (pwr_cmd->count == 0)
				CAM_ERR(CAM_SENSOR_UTIL, "pwr_down size is zero");

			for (i = 0; i < pwr_cmd->count; i++, pwr_down++) {
				pwr_settings =
				&power_info->power_down_setting[pwr_down];
				pwr_settings->seq_type =
				pwr_cmd->power_settings_flex[i].power_seq_type;
				pwr_settings->config_val =
				pwr_cmd->power_settings_flex[i].config_val_low;
				power_info->power_down_setting[pwr_down].delay
					= 0;
				if (i) {
					scr = scr +
						sizeof(
						struct cam_power_settings);
					tot_size =
						tot_size +
						sizeof(
						struct cam_power_settings);
				}
				if (tot_size > cmd_length) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Command Buffer is wrong");
					rc = -EINVAL;
					goto free_power_settings;
				}
				CAM_DBG(CAM_SENSOR_UTIL,
					"Seq Type[%d]: %d Config_val: %ld",
					pwr_down, pwr_settings->seq_type,
					pwr_settings->config_val);
			}
			last_cmd_type = CAMERA_SENSOR_CMD_TYPE_PWR_DOWN;
			ptr = (void *) scr;
			cmm_hdr = (struct common_header *)ptr;
		} else {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Error: Un expected Header Type: %d",
				cmm_hdr->cmd_type);
			rc = -EINVAL;
			goto free_power_settings;
		}
	}

	goto free_power_command;
free_power_settings:
	CAM_MEM_FREE(power_info->power_down_setting);
	CAM_MEM_FREE(power_info->power_setting);
	power_info->power_down_setting = NULL;
	power_info->power_setting = NULL;
	power_info->power_down_setting_size = 0;
	power_info->power_setting_size = 0;
free_power_command:
	CAM_MEM_FREE(pwr_cmd);
	pwr_cmd = NULL;
	return rc;
}

int cam_get_dt_power_setting_data(struct device_node *of_node,
	struct cam_hw_soc_info *soc_info,
	struct cam_sensor_power_ctrl_t *power_info)
{
	int rc = 0, i;
	int count = 0;
	const char *seq_name = NULL;
	uint32_t *array = NULL;
	struct cam_sensor_power_setting *ps;
	int c, end;

	if (!power_info)
		return -EINVAL;

	count = of_property_count_strings(of_node, "qcom,cam-power-seq-type");
	power_info->power_setting_size = count;

	CAM_DBG(CAM_SENSOR_UTIL, "qcom,cam-power-seq-type count %d", count);

	if (count <= 0)
		return 0;

	ps = CAM_MEM_ZALLOC_ARRAY(count, sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return -ENOMEM;
	power_info->power_setting = ps;

	for (i = 0; i < count; i++) {
		rc = of_property_read_string_index(of_node,
			"qcom,cam-power-seq-type", i, &seq_name);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "failed");
			goto ERROR1;
		}
		CAM_DBG(CAM_SENSOR_UTIL, "seq_name[%d] = %s", i, seq_name);
		if (!strcmp(seq_name, "cam_vio")) {
			ps[i].seq_type = SENSOR_VIO;
		} else if (!strcmp(seq_name, "cam_vana")) {
			ps[i].seq_type = SENSOR_VANA;
		} else if (!strcmp(seq_name, "cam_vana1")) {
			ps[i].seq_type = SENSOR_VANA1;
		} else if (!strcmp(seq_name, "cam_clk")) {
			ps[i].seq_type = SENSOR_MCLK;
#if defined(CONFIG_SAMSUNG_CAMERA)
		} else if (!strcmp(seq_name, "cam_vaf")) {
			ps[i].seq_type = SENSOR_VAF;
		} else if (!strcmp(seq_name, "cam_vdig")) {
			ps[i].seq_type = SENSOR_VDIG;
		} else if (!strcmp(seq_name, "cam_v_custom1")) {
			ps[i].seq_type = SENSOR_CUSTOM_REG1;
		} else if (!strcmp(seq_name, "cam_v_custom2")) {
			ps[i].seq_type = SENSOR_CUSTOM_REG2;
		} else if (!strcmp(seq_name, "cam_gpio_custom1")) {
			ps[i].seq_type = SENSOR_CUSTOM_GPIO1;
		} else if (!strcmp(seq_name, "cam_gpio_custom2")) {
			ps[i].seq_type = SENSOR_CUSTOM_GPIO2;
		} else if (!strcmp(seq_name, "cam_reset")) {
			ps[i].seq_type = SENSOR_RESET;
#endif
		} else {
			CAM_ERR(CAM_SENSOR_UTIL, "unrecognized seq-type %s",
				seq_name);
			rc = -EILSEQ;
			goto ERROR1;
		}
		CAM_DBG(CAM_SENSOR_UTIL, "seq_type[%d] %d", i, ps[i].seq_type);
	}

	array = CAM_MEM_ZALLOC_ARRAY(count, sizeof(uint32_t), GFP_KERNEL);
	if (!array) {
		rc = -ENOMEM;
		goto ERROR1;
	}

	rc = of_property_read_u32_array(of_node, "qcom,cam-power-seq-cfg-val",
		array, count);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed ");
		goto ERROR2;
	}

	for (i = 0; i < count; i++) {
		ps[i].config_val = array[i];
		CAM_DBG(CAM_SENSOR_UTIL, "power_setting[%d].config_val = %ld", i,
			ps[i].config_val);
	}

	rc = of_property_read_u32_array(of_node, "qcom,cam-power-seq-delay",
		array, count);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed");
		goto ERROR2;
	}
	for (i = 0; i < count; i++) {
		ps[i].delay = array[i];
		CAM_DBG(CAM_SENSOR_UTIL, "power_setting[%d].delay = %d", i,
			ps[i].delay);
	}
	CAM_MEM_FREE(array);

	power_info->power_down_setting =
		CAM_MEM_ZALLOC_ARRAY(count, sizeof(*ps), GFP_KERNEL);

	if (!power_info->power_down_setting) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed");
		rc = -ENOMEM;
		goto ERROR1;
	}

	power_info->power_down_setting_size = count;

	end = count - 1;

	for (c = 0; c < count; c++) {
		power_info->power_down_setting[c] = ps[end];
#if defined(CONFIG_SAMSUNG_LPAI_OIS)
		power_info->power_down_setting[c].config_val = 0;
#endif
		end--;
	}
	return rc;
ERROR2:
	CAM_MEM_FREE(array);
ERROR1:
	CAM_MEM_FREE(ps);
	return rc;
}

int cam_sensor_util_init_gpio_pin_tbl(
	struct cam_hw_soc_info *soc_info,
	struct msm_camera_gpio_num_info **pgpio_num_info)
{
	int rc = 0, val = 0;
	uint32_t gpio_array_size;
	struct device_node *of_node = NULL;
	struct cam_soc_gpio_data *gconf = NULL;
	struct msm_camera_gpio_num_info *gpio_num_info = NULL;

	if (!soc_info->dev) {
		CAM_ERR(CAM_SENSOR_UTIL, "device node NULL");
		return -EINVAL;
	}

	of_node = soc_info->dev->of_node;

	gconf = soc_info->gpio_data;
	if (!gconf) {
		CAM_ERR(CAM_SENSOR_UTIL, "No gpio_common_table is found");
		return -EINVAL;
	}

	if (!gconf->cam_gpio_common_tbl) {
		CAM_ERR(CAM_SENSOR_UTIL, "gpio_common_table is not initialized");
		return -EINVAL;
	}

	gpio_array_size = gconf->cam_gpio_common_tbl_size;

	if (!gpio_array_size) {
		CAM_ERR(CAM_SENSOR_UTIL, "invalid size of gpio table");
		return -EINVAL;
	}

	*pgpio_num_info = CAM_MEM_ZALLOC(sizeof(struct msm_camera_gpio_num_info),
		GFP_KERNEL);
	if (!*pgpio_num_info)
		return -ENOMEM;
	gpio_num_info = *pgpio_num_info;

	rc = of_property_read_u32(of_node, "gpio-vana", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "read gpio-vana failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-vana invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_VANA] =
				gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_VANA] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-vana %d",
			gpio_num_info->gpio_num[SENSOR_VANA]);
	}

	rc = of_property_read_u32(of_node, "gpio-vana1", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "read gpio-vana1 failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-vana1 invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_VANA1] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_VANA1] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-vana1 %d",
			gpio_num_info->gpio_num[SENSOR_VANA1]);
	}

	rc = of_property_read_u32(of_node, "gpio-vio", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "read gpio-vio failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-vio invalid %d", val);
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_VIO] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_VIO] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-vio %d",
			gpio_num_info->gpio_num[SENSOR_VIO]);
	}

	rc = of_property_read_u32(of_node, "gpio-vaf", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "read gpio-vaf failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-vaf invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_VAF] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_VAF] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-vaf %d",
			gpio_num_info->gpio_num[SENSOR_VAF]);
	}

	rc = of_property_read_u32(of_node, "gpio-vdig", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "read gpio-vdig failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-vdig invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_VDIG] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_VDIG] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-vdig %d",
				gpio_num_info->gpio_num[SENSOR_VDIG]);
	}

	rc = of_property_read_u32(of_node, "gpio-reset", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "read gpio-reset failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-reset invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_RESET] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_RESET] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-reset %d",
			gpio_num_info->gpio_num[SENSOR_RESET]);
	}

	rc = of_property_read_u32(of_node, "gpio-standby", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"read gpio-standby failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-standby invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_STANDBY] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_STANDBY] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-standby %d",
			gpio_num_info->gpio_num[SENSOR_STANDBY]);
	}

	rc = of_property_read_u32(of_node, "gpio-af-pwdm", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"read gpio-af-pwdm failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-af-pwdm invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_VAF_PWDM] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_VAF_PWDM] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-af-pwdm %d",
			gpio_num_info->gpio_num[SENSOR_VAF_PWDM]);
	}

	rc = of_property_read_u32(of_node, "gpio-custom1", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"read gpio-custom1 failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-custom1 invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_CUSTOM_GPIO1] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_CUSTOM_GPIO1] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-custom1 %d",
			gpio_num_info->gpio_num[SENSOR_CUSTOM_GPIO1]);
	}

	rc = of_property_read_u32(of_node, "gpio-custom2", &val);
	if (rc != -EINVAL) {
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"read gpio-custom2 failed rc %d", rc);
			goto free_gpio_info;
		} else if (val >= gpio_array_size) {
			CAM_ERR(CAM_SENSOR_UTIL, "gpio-custom2 invalid %d", val);
			rc = -EINVAL;
			goto free_gpio_info;
		}
		gpio_num_info->gpio_num[SENSOR_CUSTOM_GPIO2] =
			gconf->cam_gpio_common_tbl[val].gpio;
		gpio_num_info->valid[SENSOR_CUSTOM_GPIO2] = 1;

		CAM_DBG(CAM_SENSOR_UTIL, "gpio-custom2 %d",
			gpio_num_info->gpio_num[SENSOR_CUSTOM_GPIO2]);
	} else {
		rc = 0;
	}

	return rc;

free_gpio_info:
	CAM_MEM_FREE(gpio_num_info);
	gpio_num_info = NULL;
	return rc;
}

int msm_camera_pinctrl_init(
	struct msm_pinctrl_info *sensor_pctrl, struct device *dev)
{

	sensor_pctrl->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(sensor_pctrl->pinctrl)) {
		CAM_DBG(CAM_SENSOR_UTIL, "Getting pinctrl handle failed");
		return -EINVAL;
	}
	sensor_pctrl->gpio_state_active =
		pinctrl_lookup_state(sensor_pctrl->pinctrl,
				CAM_SENSOR_PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(sensor_pctrl->gpio_state_active)) {
		CAM_ERR(CAM_SENSOR_UTIL,
			"Failed to get the active state pinctrl handle");
		return -EINVAL;
	}
	sensor_pctrl->gpio_state_suspend
		= pinctrl_lookup_state(sensor_pctrl->pinctrl,
				CAM_SENSOR_PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(sensor_pctrl->gpio_state_suspend)) {
		CAM_ERR(CAM_SENSOR_UTIL,
			"Failed to get the suspend state pinctrl handle");
		return -EINVAL;
	}

	return 0;
}

int cam_sensor_bob_pwm_mode_switch(struct cam_hw_soc_info *soc_info,
	int bob_reg_idx, bool flag)
{
	int rc = 0;
	uint32_t op_current =
		(flag == true) ? soc_info->rgltr_op_mode[bob_reg_idx] : 0;

	if (soc_info->rgltr[bob_reg_idx] != NULL) {
		rc = cam_wrapper_regulator_set_load(soc_info->rgltr[bob_reg_idx],
			op_current, soc_info->rgltr_name[bob_reg_idx]);
		if (rc)
			CAM_WARN(CAM_SENSOR_UTIL,
				"BoB PWM SetLoad failed rc: %d", rc);
	}

	return rc;
}

int msm_cam_sensor_handle_reg_gpio(int seq_type,
	struct msm_camera_gpio_num_info *gpio_num_info, int val)
{
	int gpio_offset = -1;

	if (!gpio_num_info) {
		CAM_INFO(CAM_SENSOR_UTIL, "Input Parameters are not proper");
		return 0;
	}

	CAM_DBG(CAM_SENSOR_UTIL, "Seq type: %d, config: %d", seq_type, val);

	gpio_offset = seq_type;

	if (gpio_num_info->valid[gpio_offset] == 1) {
		CAM_DBG(CAM_SENSOR_UTIL, "VALID GPIO offset: %d, seqtype: %d",
			 gpio_offset, seq_type);
		cam_res_mgr_gpio_set_value(
			gpio_num_info->gpio_num
			[gpio_offset], val);
	}

	return 0;
}

static int cam_config_mclk_reg(struct cam_sensor_power_ctrl_t *ctrl,
	struct cam_hw_soc_info *soc_info, int32_t index)
{
	int32_t num_vreg = 0, j = 0, rc = 0, idx = 0;
	struct cam_sensor_power_setting *ps = NULL;
	struct cam_sensor_power_setting *pd = NULL;

	num_vreg = soc_info->num_rgltr;

	pd = &ctrl->power_down_setting[index];

	for (j = 0; j < num_vreg; j++) {
		if (!strcmp(soc_info->rgltr_name[j], "cam_clk")) {
			ps = NULL;
			for (idx = 0; idx < ctrl->power_setting_size; idx++) {
				if (ctrl->power_setting[idx].seq_type ==
					pd->seq_type) {
					ps = &ctrl->power_setting[idx];
					break;
				}
			}

			if (ps != NULL) {
				CAM_DBG(CAM_SENSOR_UTIL, "Disable MCLK Regulator");
				rc = cam_soc_util_regulator_disable(
					soc_info->rgltr[j],
					soc_info->rgltr_name[j],
					soc_info->rgltr_min_volt[j],
					soc_info->rgltr_max_volt[j],
					soc_info->rgltr_op_mode[j],
					soc_info->rgltr_delay[j]);

				if (rc) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"MCLK REG DISALBE FAILED: %d",
						rc);
					return rc;
				}

				ps->data[0] =
					soc_info->rgltr[j];
			}
		}
	}

	return rc;
}

int cam_sensor_core_power_up(struct cam_sensor_power_ctrl_t *ctrl,
		struct cam_hw_soc_info *soc_info, struct completion *i3c_probe_status)
{
	int rc = 0, index = 0, ret = 0, num_vreg, j = 0, i = 0;
	int32_t vreg_idx = -1;
	struct cam_sensor_power_setting *power_setting = NULL;
	struct msm_camera_gpio_num_info *gpio_num_info = NULL;
	long                             time_left;
	uint32_t                         seq_min_volt = 0;
	uint32_t                         seq_max_volt = 0;

	CAM_DBG(CAM_SENSOR_UTIL, "Enter");
	if (!ctrl) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid ctrl handle");
		return -EINVAL;
	}

	gpio_num_info = ctrl->gpio_num_info;
	num_vreg = soc_info->num_rgltr;

	if ((num_vreg <= 0) || (num_vreg > CAM_SOC_MAX_REGULATOR)) {
		if (debug_bypass_drivers & CAM_BYPASS_RGLTR)
			CAM_DBG(CAM_SENSOR_UTIL, "Bypass parameter check num_vreg %d",
				num_vreg);
		else {
			CAM_ERR(CAM_SENSOR_UTIL, "failed: num_vreg %d", num_vreg);
			return -EINVAL;
		}
	}

	ret = msm_camera_pinctrl_init(&(ctrl->pinctrl_info), ctrl->dev);
	if (ret < 0) {
		/* Some sensor subdev no pinctrl. */
		CAM_DBG(CAM_SENSOR_UTIL, "Initialization of pinctrl failed");
		ctrl->cam_pinctrl_status = 0;
	} else {
		ctrl->cam_pinctrl_status = 1;
	}

	rc = cam_sensor_util_request_gpio_table(soc_info, 1);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "request gpio table failed");
		if (ctrl->cam_pinctrl_status) {
			devm_pinctrl_put(ctrl->pinctrl_info.pinctrl);
			ctrl->cam_pinctrl_status = 0;
		}
		return -EINVAL;
	}

	if (ctrl->cam_pinctrl_status) {
		ret = pinctrl_select_state(
			ctrl->pinctrl_info.pinctrl,
			ctrl->pinctrl_info.gpio_state_active);
		if (ret)
			CAM_ERR(CAM_SENSOR_UTIL, "cannot set pin to active state");
	}

	CAM_DBG(CAM_SENSOR_UTIL, "power setting size: %d", ctrl->power_setting_size);

	for (index = 0; index < ctrl->power_setting_size; index++) {
		CAM_DBG(CAM_SENSOR_UTIL, "index: %d", index);
		power_setting = &ctrl->power_setting[index];
		if (!power_setting) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Invalid power up settings for index %d",
				index);
			return -EINVAL;
		}

		CAM_DBG(CAM_SENSOR_UTIL, "seq_type %d", power_setting->seq_type);

		switch (power_setting->seq_type) {
		case SENSOR_MCLK:
			if (debug_bypass_drivers & CAM_BYPASS_CLKS) {
				CAM_DBG(CAM_SENSOR_UTIL, "Bypass mclk enable");
				continue;
			}

			if (power_setting->seq_val >= soc_info->num_clk) {
				CAM_ERR(CAM_SENSOR_UTIL, "clk index %d >= max %u",
					power_setting->seq_val,
					soc_info->num_clk);
				goto power_up_failed;
			}
			for (j = 0; j < num_vreg; j++) {
				if (!strcmp(soc_info->rgltr_name[j],
					"cam_clk")) {
					CAM_DBG(CAM_SENSOR_UTIL,
						"Enable cam_clk: %d", j);

					if (IS_ERR_OR_NULL(
						soc_info->rgltr[j])) {
						rc = PTR_ERR(
							soc_info->rgltr[j]);
						rc = rc ? rc : -EINVAL;
						CAM_ERR(CAM_SENSOR_UTIL,
							"vreg %s %d",
							soc_info->rgltr_name[j],
							rc);
						goto power_up_failed;
					}

					rc =  cam_soc_util_regulator_enable(
					soc_info->rgltr[j],
					soc_info->rgltr_name[j],
					soc_info->rgltr_min_volt[j],
					soc_info->rgltr_max_volt[j],
					soc_info->rgltr_op_mode[j],
					soc_info->rgltr_delay[j]);
					if (rc) {
						CAM_ERR(CAM_SENSOR_UTIL,
							"Reg enable failed");
						goto power_up_failed;
					}
					power_setting->data[0] =
						soc_info->rgltr[j];
				}
			}
			if (power_setting->config_val)
				soc_info->clk_rate[0][power_setting->seq_val] =
					power_setting->config_val;

			for (j = 0; j < soc_info->num_clk; j++) {
				rc = cam_soc_util_clk_enable(soc_info, CAM_CLK_SW_CLIENT_IDX,
					false, j, 0);
				if (rc) {
					CAM_ERR(CAM_UTIL,
						"Failed in clk enable %d", i);
					break;
				}
			}

			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL, "clk enable failed");
				goto power_up_failed;
			}
			break;
		case SENSOR_RESET:
		case SENSOR_STANDBY:
		case SENSOR_CUSTOM_GPIO1:
		case SENSOR_CUSTOM_GPIO2:
			if (!gpio_num_info) {
				CAM_ERR(CAM_SENSOR_UTIL, "Invalid gpio_num_info");
				goto power_up_failed;
			}
			CAM_DBG(CAM_SENSOR_UTIL, "gpio set val %d",
				gpio_num_info->gpio_num
				[power_setting->seq_type]);

			rc = msm_cam_sensor_handle_reg_gpio(
				power_setting->seq_type,
				gpio_num_info,
				(int) power_setting->config_val);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL,
					"Error in handling VREG GPIO");
				goto power_up_failed;
			}
			break;
		case SENSOR_VANA:
		case SENSOR_VANA1:
		case SENSOR_VDIG:
		case SENSOR_VIO:
		case SENSOR_VAF:
		case SENSOR_VAF_PWDM:
		case SENSOR_CUSTOM_REG1:
		case SENSOR_CUSTOM_REG2:
			if (debug_bypass_drivers & CAM_BYPASS_RGLTR) {
				CAM_DBG(CAM_SENSOR_UTIL, "Bypass regulator enable seq_type %d",
					power_setting->seq_type);
				continue;
			}

			if (power_setting->seq_val == INVALID_VREG)
				break;

			if (power_setting->seq_val >= CAM_VREG_MAX) {
				CAM_ERR(CAM_SENSOR_UTIL, "vreg index %d >= max %d",
					power_setting->seq_val,
					CAM_VREG_MAX);
				goto power_up_failed;
			}
			if (power_setting->seq_val < num_vreg) {
				CAM_DBG(CAM_SENSOR_UTIL, "Enable Regulator");
				vreg_idx = power_setting->seq_val;

				if (IS_ERR_OR_NULL(
					soc_info->rgltr[vreg_idx])) {
					rc = PTR_ERR(soc_info->rgltr[vreg_idx]);
					rc = rc ? rc : -EINVAL;

					CAM_ERR(CAM_SENSOR_UTIL, "%s get failed %d",
						soc_info->rgltr_name[vreg_idx],
						rc);

					goto power_up_failed;
				}

				if (power_setting->valid_config) {
					seq_min_volt = power_setting->config_val;
					seq_max_volt = power_setting->config_val;
				} else {
					seq_min_volt = soc_info->rgltr_min_volt[vreg_idx];
					seq_max_volt = soc_info->rgltr_max_volt[vreg_idx];
				}

#if defined(CONFIG_SAMSUNG_CAMERA)
				if (power_setting->seq_type == SENSOR_VANA ||
					power_setting->seq_type == SENSOR_VDIG)
				{
					cam_soc_util_regulator_set_mode(
						soc_info->rgltr[vreg_idx],
						soc_info->rgltr_name[vreg_idx],
						REGULATOR_MODE_FAST);
				}
#endif

				rc =  cam_soc_util_regulator_enable(
					soc_info->rgltr[vreg_idx],
					soc_info->rgltr_name[vreg_idx],
					seq_min_volt,
					seq_max_volt,
					soc_info->rgltr_op_mode[vreg_idx],
					soc_info->rgltr_delay[vreg_idx]);
				if (rc) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Reg Enable failed for %s",
						soc_info->rgltr_name[vreg_idx]);
					goto power_up_failed;
				}
				power_setting->data[0] =
						soc_info->rgltr[vreg_idx];
			} else {
				CAM_ERR(CAM_SENSOR_UTIL, "usr_idx:%d dts_idx:%d",
					power_setting->seq_val, num_vreg);
			}

			rc = msm_cam_sensor_handle_reg_gpio(
				power_setting->seq_type,
				gpio_num_info, 1);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL,
					"Error in handling VREG GPIO");
				goto power_up_failed;
			}
			break;
		default:
			CAM_ERR(CAM_SENSOR_UTIL, "error power seq type %d",
				power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20)
			msleep(power_setting->delay);
		else if (power_setting->delay)
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 5);
	}

	if (i3c_probe_status) {
		time_left = cam_common_wait_for_completion_timeout(
			i3c_probe_status,
			msecs_to_jiffies(CAM_I3C_DEV_PROBE_TIMEOUT_MS));
		if (!time_left) {
			CAM_ERR(CAM_SENSOR_UTIL, "Wait for I3C probe timedout");
			rc = -ETIMEDOUT;
			goto power_up_failed;
		}
	}

	return 0;
power_up_failed:
	CAM_ERR(CAM_SENSOR_UTIL, "failed. rc:%d", rc);
	for (index--; index >= 0; index--) {
		CAM_DBG(CAM_SENSOR_UTIL, "index %d",  index);
		power_setting = &ctrl->power_setting[index];
		CAM_DBG(CAM_SENSOR_UTIL, "type %d",
			power_setting->seq_type);
		switch (power_setting->seq_type) {
		case SENSOR_MCLK:
			if (debug_bypass_drivers & CAM_BYPASS_CLKS) {
				CAM_DBG(CAM_SENSOR_UTIL, "Bypass mclk disable");
				continue;
			}

			for (i = soc_info->num_clk - 1; i >= 0; i--) {
				cam_soc_util_clk_disable(soc_info, CAM_CLK_SW_CLIENT_IDX, false, i);
			}
			ret = cam_config_mclk_reg(ctrl, soc_info, index);
			if (ret < 0) {
				CAM_ERR(CAM_SENSOR_UTIL,
					"config clk reg failed rc: %d", ret);
				continue;
			}
			break;
		case SENSOR_RESET:
		case SENSOR_STANDBY:
		case SENSOR_CUSTOM_GPIO1:
		case SENSOR_CUSTOM_GPIO2:
			if (!gpio_num_info)
				continue;
			if (!gpio_num_info->valid
				[power_setting->seq_type])
				continue;
			cam_res_mgr_gpio_set_value(
				gpio_num_info->gpio_num
				[power_setting->seq_type], GPIOF_OUT_INIT_LOW);
			break;
		case SENSOR_VANA:
		case SENSOR_VANA1:
		case SENSOR_VDIG:
		case SENSOR_VIO:
		case SENSOR_VAF:
		case SENSOR_VAF_PWDM:
		case SENSOR_CUSTOM_REG1:
		case SENSOR_CUSTOM_REG2:
			if (debug_bypass_drivers & CAM_BYPASS_RGLTR) {
				CAM_DBG(CAM_SENSOR_UTIL, "Bypass regulator disable seq_type %d",
					power_setting->seq_type);
				continue;
			}

			if (power_setting->seq_val < num_vreg) {
				CAM_DBG(CAM_SENSOR_UTIL, "Disable Regulator");
				vreg_idx = power_setting->seq_val;

				rc =  cam_soc_util_regulator_disable(
					soc_info->rgltr[vreg_idx],
					soc_info->rgltr_name[vreg_idx],
					soc_info->rgltr_min_volt[vreg_idx],
					soc_info->rgltr_max_volt[vreg_idx],
					soc_info->rgltr_op_mode[vreg_idx],
					soc_info->rgltr_delay[vreg_idx]);

				if (rc) {
					CAM_ERR(CAM_SENSOR_UTIL,
					"Fail to disalbe reg: %s",
					soc_info->rgltr_name[vreg_idx]);
					soc_info->rgltr[vreg_idx] = NULL;
					msm_cam_sensor_handle_reg_gpio(
						power_setting->seq_type,
						gpio_num_info,
						GPIOF_OUT_INIT_LOW);
					continue;
				}
				power_setting->data[0] =
						soc_info->rgltr[vreg_idx];

#if defined(CONFIG_SAMSUNG_CAMERA)
				if (power_setting->seq_type == SENSOR_VANA ||
					power_setting->seq_type == SENSOR_VDIG)
				{
					cam_soc_util_regulator_set_mode(
						soc_info->rgltr[vreg_idx],
						soc_info->rgltr_name[vreg_idx],
						REGULATOR_MODE_NORMAL);
				}
#endif

			} else {
				CAM_ERR(CAM_SENSOR_UTIL, "seq_val:%d > num_vreg: %d",
					power_setting->seq_val, num_vreg);
			}

			msm_cam_sensor_handle_reg_gpio(power_setting->seq_type,
				gpio_num_info, GPIOF_OUT_INIT_LOW);

			break;
		default:
			CAM_ERR(CAM_SENSOR_UTIL, "error power seq type %d",
				power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20) {
			msleep(power_setting->delay);
		} else if (power_setting->delay) {
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 5);
		}
	}

	if (ctrl->cam_pinctrl_status) {
		ret = pinctrl_select_state(
			ctrl->pinctrl_info.pinctrl,
			ctrl->pinctrl_info.gpio_state_suspend);
		if (ret)
			CAM_ERR(CAM_SENSOR_UTIL, "cannot set pin to suspend state");
		devm_pinctrl_put(ctrl->pinctrl_info.pinctrl);
	}

	ctrl->cam_pinctrl_status = 0;
	cam_sensor_util_request_gpio_table(soc_info, 0);

	return -EINVAL;
}

static struct cam_sensor_power_setting*
msm_camera_get_power_settings(struct cam_sensor_power_ctrl_t *ctrl,
				enum msm_camera_power_seq_type seq_type,
				uint16_t seq_val)
{
	struct cam_sensor_power_setting *power_setting, *ps = NULL;
	int idx;

	for (idx = 0; idx < ctrl->power_setting_size; idx++) {
		power_setting = &ctrl->power_setting[idx];
		if (power_setting->seq_type == seq_type &&
			power_setting->seq_val ==  seq_val) {
			ps = power_setting;
			return ps;
		}

	}

	return ps;
}

int cam_sensor_util_power_down(struct cam_sensor_power_ctrl_t *ctrl,
		struct cam_hw_soc_info *soc_info)
{
	int index = 0, ret = 0, num_vreg = 0, i;
	struct cam_sensor_power_setting *pd = NULL;
	struct cam_sensor_power_setting *ps = NULL;
	struct msm_camera_gpio_num_info *gpio_num_info = NULL;

	CAM_DBG(CAM_SENSOR_UTIL, "Enter");
	if (!ctrl || !soc_info) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed ctrl %pK",  ctrl);
		return -EINVAL;
	}

	gpio_num_info = ctrl->gpio_num_info;
	num_vreg = soc_info->num_rgltr;

	if ((num_vreg <= 0) || (num_vreg > CAM_SOC_MAX_REGULATOR)) {
		if (debug_bypass_drivers & CAM_BYPASS_RGLTR)
			CAM_DBG(CAM_SENSOR_UTIL, "Bypass parameter check num_vreg %d",
				num_vreg);
		else {
			CAM_ERR(CAM_SENSOR_UTIL, "failed: num_vreg %d", num_vreg);
			return -EINVAL;
		}
	}

	if (ctrl->power_down_setting_size > MAX_POWER_CONFIG) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid: power setting size %d",
			ctrl->power_setting_size);
		return -EINVAL;
	}

	for (index = 0; index < ctrl->power_down_setting_size; index++) {
		CAM_DBG(CAM_SENSOR_UTIL, "power_down_index %d",  index);
		pd = &ctrl->power_down_setting[index];
		if (!pd) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Invalid power down settings for index %d",
				index);
			return -EINVAL;
		}

		CAM_DBG(CAM_SENSOR_UTIL, "seq_type %d",  pd->seq_type);

#if defined(CONFIG_SEC_PA1Q_PROJECT) || defined(CONFIG_SEC_PA2Q_PROJECT)
		if (pd->seq_type == SENSOR_RESET) {
			usleep_range(300,400);
		}
#endif
		switch (pd->seq_type) {
		case SENSOR_MCLK:
			if (debug_bypass_drivers & CAM_BYPASS_CLKS) {
				CAM_DBG(CAM_SENSOR_UTIL, "Bypass mclk disable");
				continue;
			}

			usleep_range(1800,1900);

			for (i = soc_info->num_clk - 1; i >= 0; i--) {
				cam_soc_util_clk_disable(soc_info, CAM_CLK_SW_CLIENT_IDX, false, i);
			}

			ret = cam_config_mclk_reg(ctrl, soc_info, index);
			if (ret < 0) {
				CAM_ERR(CAM_SENSOR_UTIL,
					"config clk reg failed rc: %d", ret);
				continue;
			}
			break;
		case SENSOR_RESET:
		case SENSOR_STANDBY:
		case SENSOR_CUSTOM_GPIO1:
		case SENSOR_CUSTOM_GPIO2:

			if (!gpio_num_info->valid[pd->seq_type])
				continue;

			cam_res_mgr_gpio_set_value(
				gpio_num_info->gpio_num
				[pd->seq_type],
				(int) pd->config_val);

			break;
		case SENSOR_VANA:
		case SENSOR_VANA1:
		case SENSOR_VDIG:
		case SENSOR_VIO:
		case SENSOR_VAF:
		case SENSOR_VAF_PWDM:
		case SENSOR_CUSTOM_REG1:
		case SENSOR_CUSTOM_REG2:
			if (debug_bypass_drivers & CAM_BYPASS_RGLTR) {
				CAM_DBG(CAM_SENSOR_UTIL, "Bypass regulator disable seq_type %d",
					pd->seq_type);
				continue;
			}

			if (pd->seq_val == INVALID_VREG)
				break;

			ps = msm_camera_get_power_settings(
				ctrl, pd->seq_type,
				pd->seq_val);
			if (ps) {
				if (pd->seq_val < num_vreg) {
#if defined(CONFIG_SENSOR_RETENTION)
					if (pd->config_val > 0) {
						if (soc_info->rgltr[ps->seq_val] != NULL)
						{
							ret = regulator_set_voltage(
								soc_info->rgltr[ps->seq_val], pd->config_val, soc_info->rgltr_max_volt[ps->seq_val]);
							if (ret) {
								CAM_ERR(CAM_UTIL, "%s set voltage failed",
									soc_info->rgltr_name[ps->seq_val]);
							}
						}
					}
#endif
					CAM_DBG(CAM_SENSOR_UTIL,
						"Disable Regulator");
					ret =  cam_soc_util_regulator_disable(
					soc_info->rgltr[ps->seq_val],
					soc_info->rgltr_name[ps->seq_val],
					soc_info->rgltr_min_volt[ps->seq_val],
					soc_info->rgltr_max_volt[ps->seq_val],
					soc_info->rgltr_op_mode[ps->seq_val],
					soc_info->rgltr_delay[ps->seq_val]);
					if (ret) {
						CAM_ERR(CAM_SENSOR_UTIL,
						"Reg: %s disable failed",
						soc_info->rgltr_name[
							ps->seq_val]);
						msm_cam_sensor_handle_reg_gpio(
							pd->seq_type,
							gpio_num_info,
							GPIOF_OUT_INIT_LOW);
						continue;
					}
					ps->data[0] =
						soc_info->rgltr[ps->seq_val];

#if defined(CONFIG_SAMSUNG_CAMERA)
					if (pd->seq_type == SENSOR_VANA ||
						pd->seq_type == SENSOR_VDIG)
					{
						cam_soc_util_regulator_set_mode(
							soc_info->rgltr[ps->seq_val],
							soc_info->rgltr_name[ps->seq_val],
							REGULATOR_MODE_NORMAL);
					}
#endif
				} else {
					CAM_ERR(CAM_SENSOR_UTIL,
						"seq_val:%d > num_vreg: %d",
						 pd->seq_val,
						num_vreg);
				}
			} else
				CAM_ERR(CAM_SENSOR_UTIL,
					"error in power up/down seq");

			ret = msm_cam_sensor_handle_reg_gpio(pd->seq_type,
				gpio_num_info, GPIOF_OUT_INIT_LOW);

			if (ret < 0)
				CAM_ERR(CAM_SENSOR_UTIL,
					"Error disabling VREG GPIO");
			break;
		default:
			CAM_ERR(CAM_SENSOR_UTIL, "error power seq type %d",
				pd->seq_type);
			break;
		}
		if (pd->delay > 20)
			msleep(pd->delay);
		else if (pd->delay)
			usleep_range(pd->delay * 1000,
				(pd->delay * 1000) + 5);
	}

	if (ctrl->cam_pinctrl_status) {
		ret = pinctrl_select_state(
				ctrl->pinctrl_info.pinctrl,
				ctrl->pinctrl_info.gpio_state_suspend);
		if (ret)
			CAM_ERR(CAM_SENSOR_UTIL, "cannot set pin to suspend state");

		devm_pinctrl_put(ctrl->pinctrl_info.pinctrl);
	}

	cam_sensor_util_request_gpio_table(soc_info, 0);
	ctrl->cam_pinctrl_status = 0;

	return 0;
}

void cam_sensor_utils_parse_pm_ctrl_flag(struct device_node *of_node,
	struct camera_io_master *io_master_info)
{
	struct device_node *of_parent = of_get_next_parent(of_node);

	if ((of_parent != NULL) &&
		(io_master_info != NULL) &&
		(io_master_info->qup_client != NULL)) {
		io_master_info->qup_client->pm_ctrl_client_enable =
			of_property_read_bool(of_parent, "qcom,pm-ctrl-client");
	}
}

#if defined(CONFIG_SENSOR_RETENTION)
int cam_sensor_util_retention_power_up(struct cam_sensor_power_ctrl_t *ctrl,
		struct cam_hw_soc_info *soc_info)
{
	int rc = 0, index = 0, num_vreg;
	int32_t vreg_idx = -1;
	struct cam_sensor_power_setting *power_setting = NULL;
	struct msm_camera_gpio_num_info *gpio_num_info = NULL;
	uint32_t						 seq_min_volt = 0;
	uint32_t						 seq_max_volt = 0;
	uint8_t is_retention_power[SENSOR_SEQ_TYPE_MAX] = { 0, };

	CAM_DBG(CAM_SENSOR_UTIL, "Enter");
	if (!ctrl) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid ctrl handle");
		return -EINVAL;
	}

	// Find retention powers
	for (index = 0; index < ctrl->power_down_setting_size; index++) {
		power_setting = &ctrl->power_down_setting[index];
		if ((power_setting->seq_type < SENSOR_SEQ_TYPE_MAX) &&
			(power_setting->config_val > 0))
			is_retention_power[power_setting->seq_type] = 1;
	}

	gpio_num_info = ctrl->gpio_num_info;
	num_vreg = soc_info->num_rgltr;

	if ((num_vreg <= 0) || (num_vreg > CAM_SOC_MAX_REGULATOR)) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed: num_vreg %d", num_vreg);
		return -EINVAL;
	}

	CAM_DBG(CAM_SENSOR_UTIL, "power setting size: %d", ctrl->power_setting_size);

	for (index = 0; index < ctrl->power_setting_size; index++) {
		CAM_DBG(CAM_SENSOR_UTIL, "index: %d", index);
		power_setting = &ctrl->power_setting[index];
		if (!power_setting) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Invalid power up settings for index %d",
				index);
			return -EINVAL;
		}

		if (!is_retention_power[power_setting->seq_type])
			continue;

		CAM_DBG(CAM_SENSOR_UTIL, "seq_type %d", power_setting->seq_type);

		switch (power_setting->seq_type) {
		case SENSOR_MCLK:
			CAM_WARN(CAM_SENSOR_UTIL, "retention power up doesn't support MCLK");
			break;
		case SENSOR_RESET:
		case SENSOR_STANDBY:
		case SENSOR_CUSTOM_GPIO1:
		case SENSOR_CUSTOM_GPIO2:
			if (!gpio_num_info) {
				CAM_ERR(CAM_SENSOR_UTIL, "Invalid gpio_num_info");
				break;
			}

			if (!ctrl->is_retention_power_up[power_setting->seq_type]) {
				CAM_DBG(CAM_SENSOR_UTIL, "gpio set val %d",
					gpio_num_info->gpio_num
					[power_setting->seq_type]);

				rc = msm_cam_sensor_handle_reg_gpio(
					power_setting->seq_type,
					gpio_num_info,
					(int) power_setting->config_val);
				if (rc < 0) {
					CAM_ERR(CAM_SENSOR_UTIL,
						"Error in handling VREG GPIO");
					break;
				}
				ctrl->is_retention_power_up[power_setting->seq_type] = 1;
				CAM_INFO(CAM_SENSOR_UTIL,
					"[RET_DBG] GPIO %d for retention power up",
					gpio_num_info->gpio_num[power_setting->seq_type]);
			}
			break;
		case SENSOR_VANA:
		case SENSOR_VANA1:
		case SENSOR_VDIG:
		case SENSOR_VIO:
		case SENSOR_VAF:
		case SENSOR_VAF_PWDM:
		case SENSOR_CUSTOM_REG1:
		case SENSOR_CUSTOM_REG2:
			if (power_setting->seq_val == INVALID_VREG)
				break;

			if (power_setting->seq_val >= CAM_VREG_MAX) {
				CAM_ERR(CAM_SENSOR_UTIL, "vreg index %d >= max %d",
					power_setting->seq_val,
					CAM_VREG_MAX);
				break;
			}
			if (!ctrl->is_retention_power_up[power_setting->seq_type]) {
				if (power_setting->seq_val < num_vreg) {
					CAM_DBG(CAM_SENSOR_UTIL, "Enable Regulator");
					vreg_idx = power_setting->seq_val;

					if (IS_ERR_OR_NULL(
						soc_info->rgltr[vreg_idx])) {
						rc = PTR_ERR(soc_info->rgltr[vreg_idx]);
						rc = rc ? rc : -EINVAL;

						CAM_ERR(CAM_SENSOR_UTIL, "%s get failed %d",
							soc_info->rgltr_name[vreg_idx],
							rc);

						soc_info->rgltr[vreg_idx] = NULL;
						break;
					}

					if (power_setting->valid_config) {
						seq_min_volt = power_setting->config_val;
						seq_max_volt = power_setting->config_val;
					} else {
						seq_min_volt = soc_info->rgltr_min_volt[vreg_idx];
						seq_max_volt = soc_info->rgltr_max_volt[vreg_idx];
					}

					rc =  cam_soc_util_regulator_enable(
						soc_info->rgltr[vreg_idx],
						soc_info->rgltr_name[vreg_idx],
						seq_min_volt,
						seq_max_volt,
						soc_info->rgltr_op_mode[vreg_idx],
						soc_info->rgltr_delay[vreg_idx]);
					if (rc) {
						CAM_ERR(CAM_SENSOR_UTIL,
							"Reg Enable failed for %s",
							soc_info->rgltr_name[vreg_idx]);
						break;
					}
					power_setting->data[0] =
							soc_info->rgltr[vreg_idx];
					ctrl->is_retention_power_up[power_setting->seq_type] = 1;
					CAM_INFO(CAM_SENSOR_UTIL,
						"[RET_DBG] %s for retention power up",
						soc_info->rgltr_name[vreg_idx]);
				} else {
					CAM_ERR(CAM_SENSOR_UTIL, "usr_idx:%d dts_idx:%d",
						power_setting->seq_val, num_vreg);
				}
			}
			break;
		default:
			CAM_ERR(CAM_SENSOR_UTIL, "error power seq type %d",
				power_setting->seq_type);
			break;
		}
	}

	return 0;
}

int cam_sensor_util_retention_power_down(struct cam_sensor_power_ctrl_t *ctrl,
		struct cam_hw_soc_info *soc_info)
{
	int index = 0, ret = 0, num_vreg = 0;
	struct cam_sensor_power_setting *pd = NULL;
	struct cam_sensor_power_setting *ps = NULL;
	struct msm_camera_gpio_num_info *gpio_num_info = NULL;
	uint8_t is_retention_power[SENSOR_SEQ_TYPE_MAX] = { 0, };

	CAM_DBG(CAM_SENSOR_UTIL, "Enter");
	if (!ctrl || !soc_info) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed ctrl %pK",  ctrl);
		return -EINVAL;
	}

	// Find retention power
	for (index = 0; index < ctrl->power_down_setting_size; index++) {
		pd = &ctrl->power_down_setting[index];
		if ((pd->seq_type < SENSOR_SEQ_TYPE_MAX) &&
			(pd->config_val > 0))
			is_retention_power[pd->seq_type] = 1;
	}

	gpio_num_info = ctrl->gpio_num_info;
	num_vreg = soc_info->num_rgltr;

	if ((num_vreg <= 0) || (num_vreg > CAM_SOC_MAX_REGULATOR)) {
		CAM_ERR(CAM_SENSOR_UTIL, "failed: num_vreg %d", num_vreg);
		return -EINVAL;
	}

	if (ctrl->power_down_setting_size > MAX_POWER_CONFIG) {
		CAM_ERR(CAM_SENSOR_UTIL, "Invalid: power setting size %d",
			ctrl->power_setting_size);
		return -EINVAL;
	}

	for (index = 0; index < ctrl->power_down_setting_size; index++) {
		CAM_DBG(CAM_SENSOR_UTIL, "power_down_index %d",  index);
		pd = &ctrl->power_down_setting[index];
		if (!pd) {
			CAM_ERR(CAM_SENSOR_UTIL,
				"Invalid power down settings for index %d",
				index);
			return -EINVAL;
		}

		if (!is_retention_power[pd->seq_type])
			continue;

		CAM_DBG(CAM_SENSOR_UTIL, "seq_type %d",  pd->seq_type);
		switch (pd->seq_type) {
		case SENSOR_MCLK:
			CAM_WARN(CAM_SENSOR_UTIL, "retention power up doesn't support MCLK");
			break;
		case SENSOR_RESET:
		case SENSOR_STANDBY:
		case SENSOR_CUSTOM_GPIO1:
		case SENSOR_CUSTOM_GPIO2:

			if (!gpio_num_info->valid[pd->seq_type])
				continue;

			if (pd->seq_type == SENSOR_RESET) {
				usleep_range(1000, 1010);
			}

			if (ctrl->is_retention_power_up[pd->seq_type]) {
				cam_res_mgr_gpio_set_value(
					gpio_num_info->gpio_num
					[pd->seq_type],
					0);
				ctrl->is_retention_power_up[pd->seq_type] = 0;
				CAM_INFO(CAM_SENSOR_UTIL,
					"[RET_DBG] GPIO %d for retention power down",
					gpio_num_info->gpio_num[pd->seq_type]);
			}

			break;
		case SENSOR_VANA:
		case SENSOR_VANA1:
		case SENSOR_VDIG:
		case SENSOR_VIO:
		case SENSOR_VAF:
		case SENSOR_VAF_PWDM:
		case SENSOR_CUSTOM_REG1:
		case SENSOR_CUSTOM_REG2:
			if (pd->seq_val == INVALID_VREG)
				break;

			if(ctrl->is_retention_power_up[pd->seq_type]) {
				ps = msm_camera_get_power_settings(
					ctrl, pd->seq_type,
					pd->seq_val);

				if (ps) {
					if (pd->seq_val < num_vreg) {
						CAM_DBG(CAM_SENSOR_UTIL,
							"Disable Regulator");
						ret =  cam_soc_util_regulator_disable(
						soc_info->rgltr[ps->seq_val],
						soc_info->rgltr_name[ps->seq_val],
						soc_info->rgltr_min_volt[ps->seq_val],
						soc_info->rgltr_max_volt[ps->seq_val],
						soc_info->rgltr_op_mode[ps->seq_val],
						soc_info->rgltr_delay[ps->seq_val]);
						if (ret) {
							CAM_ERR(CAM_SENSOR_UTIL,
							"Reg: %s disable failed",
							soc_info->rgltr_name[
								ps->seq_val]);
							msm_cam_sensor_handle_reg_gpio(
								pd->seq_type,
								gpio_num_info,
								GPIOF_OUT_INIT_LOW);
							continue;
						}
						ps->data[0] =
							soc_info->rgltr[ps->seq_val];
						ctrl->is_retention_power_up[pd->seq_type] = 0;
						CAM_INFO(CAM_SENSOR_UTIL,
							"[RET_DBG] %s for retention power down",
							soc_info->rgltr_name[ps->seq_val]);
					} else {
						CAM_ERR(CAM_SENSOR_UTIL,
							"seq_val:%d > num_vreg: %d",
							 pd->seq_val,
							num_vreg);
					}
				} else
					CAM_ERR(CAM_SENSOR_UTIL,
						"error in power up/down seq");
			}
			break;
		default:
			CAM_ERR(CAM_SENSOR_UTIL, "error power seq type %d",
				pd->seq_type);
			break;
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_SAMSUNG_DEBUG_SENSOR_I2C)
struct st_vc_addr {
	uint32_t img_addr;
	uint32_t pd_addr;
	enum camera_sensor_i2c_type	data_sz;
} vc_addr_info[e_vc_addr_max] = {
	[e_vc_addr_260_264_1b_read] = { 0x0260, 0x0264, CAMERA_SENSOR_I2C_TYPE_BYTE },
	[e_vc_addr_260_264_2b_read] = { 0x0260, 0x0264, CAMERA_SENSOR_I2C_TYPE_WORD },
	[e_vc_addr_110_264] = { 0x0110, 0x0264, CAMERA_SENSOR_I2C_TYPE_WORD },
	[e_vc_addr_110_30b0] = { 0x0110, 0x30B0, CAMERA_SENSOR_I2C_TYPE_BYTE },
	[e_vc_addr_110_30a4] = { 0x0110, 0x30A4, CAMERA_SENSOR_I2C_TYPE_BYTE }
};

struct st_sensordbg_addr {
	uint32_t cit;
	uint32_t aeb_cit_l;
	uint32_t aeb_cit_s;
	uint32_t fll;
	uint32_t aeb_fll_l;
	uint32_t aeb_fll_s;
	enum camera_sensor_i2c_type	data_sz; // other than aeb_vc
	uint32_t aeb_vc_l;
	uint32_t aeb_vc_s;
	enum camera_sensor_i2c_type	aeb_vc_sz; // only about aeb_vc
} sensordbg_addr_info[e_seq_sensor_idn_max_invalid] = {
	[e_seq_sensor_idn_s5khp2] = {
		.cit = 0x0202,
		.aeb_cit_l = 0x0e10,
		.aeb_cit_s = 0x0e2a,
		.fll = 0x0340,
		.aeb_fll_l = 0x0e16,
		.aeb_fll_s = 0x0e30,
		.data_sz = CAMERA_SENSOR_I2C_TYPE_WORD,
		.aeb_vc_l = 0x0e18,
		.aeb_vc_s = 0x0e32,
		.aeb_vc_sz = CAMERA_SENSOR_I2C_TYPE_WORD
	},
	[e_seq_sensor_idn_s5kgn3] = {
		.cit = 0x0202,
		.aeb_cit_l = 0x0e10,
		.aeb_cit_s = 0x0e1e,
		.fll = 0x0340,
		.aeb_fll_l = 0x0e16,
		.aeb_fll_s = 0x0e24,
		.data_sz = CAMERA_SENSOR_I2C_TYPE_WORD,
		.aeb_vc_l = 0x0e18,
		.aeb_vc_s = 0x0e26,
		.aeb_vc_sz = CAMERA_SENSOR_I2C_TYPE_BYTE
	},
	[e_seq_sensor_idn_s5k3lu] = {
		.cit = 0x0202,
		.aeb_cit_l = 0x0e10,
		.aeb_cit_s = 0x0e20,
		.fll = 0x0340,
		.aeb_fll_l = 0x0e16,
		.aeb_fll_s = 0x0e26,
		.data_sz = CAMERA_SENSOR_I2C_TYPE_WORD,
		.aeb_vc_l = 0x0e18,
		.aeb_vc_s = 0x0e28,
		.aeb_vc_sz = CAMERA_SENSOR_I2C_TYPE_BYTE
	},
	[e_seq_sensor_idn_imx564] = {0,},
	[e_seq_sensor_idn_imx754] = {0,},
	[e_seq_sensor_idn_s5k2ld] = {0,},
	[e_seq_sensor_idn_imx854] = {
		.cit = 0x0202,
		.aeb_cit_l = 0x0e10,
		.aeb_cit_s = 0x0e2a,
		.fll = 0x0340,
		.aeb_fll_l = 0x0e1e,
		.aeb_fll_s = 0x0e4e,
		.data_sz = CAMERA_SENSOR_I2C_TYPE_WORD,
		.aeb_vc_l = 0x0e20,
		.aeb_vc_s = 0x0e50,
		.aeb_vc_sz = CAMERA_SENSOR_I2C_TYPE_BYTE
	},
	[e_seq_sensor_idn_s5k3k1] = {0,},
	[e_seq_sensor_idn_s5kjn3] = {
		.cit = 0x0202,
		.aeb_cit_l = 0x0e10,
		.aeb_cit_s = 0x0e1e,
		.fll = 0x0340,
		.aeb_fll_l = 0x0e16,
		.aeb_fll_s = 0x0e24,
		.data_sz = CAMERA_SENSOR_I2C_TYPE_WORD,
		.aeb_vc_l = 0x0e18,
		.aeb_vc_s = 0x0e26,
		.aeb_vc_sz = CAMERA_SENSOR_I2C_TYPE_BYTE
	},
	[e_seq_sensor_idn_imx874] = {
		.cit = 0x0202,
		.aeb_cit_l = 0x0e10,
		.aeb_cit_s = 0x0e50,
		.fll = 0x0340,
		.aeb_fll_l = 0x0e1e,
		.aeb_fll_s = 0x0e5e,
		.data_sz = CAMERA_SENSOR_I2C_TYPE_WORD,
		.aeb_vc_l = 0x0e20,
		.aeb_vc_s = 0x0e60,
		.aeb_vc_sz = CAMERA_SENSOR_I2C_TYPE_BYTE
	}
};

const uint32_t reg_addr_aeb_on = 0xe00;
const uint32_t fcm_idx_addr = 0x0B30;

inline uint32_t get_seq_sensor_id(uint32_t sensor_id)
{
	e_seq_sensor_idnum ret;
	switch (sensor_id)
	{
	case SENSOR_ID_S5KHP2:
		ret = e_seq_sensor_idn_s5khp2;
		break;
	case SENSOR_ID_S5KGN3:
		ret = e_seq_sensor_idn_s5kgn3;
		break;
	case SENSOR_ID_S5K3LU:
		ret = e_seq_sensor_idn_s5k3lu;
		break;
	case SENSOR_ID_IMX564:
		ret = e_seq_sensor_idn_imx564;
		break;
	case SENSOR_ID_IMX754:
		ret = e_seq_sensor_idn_imx754;
		break;
	case SENSOR_ID_S5K2LD:
		ret = e_seq_sensor_idn_s5k2ld;
		break;
	case SENSOR_ID_S5K3K1:
		ret = e_seq_sensor_idn_s5k3k1;
		break;
	case SENSOR_ID_IMX854:
		ret = e_seq_sensor_idn_imx854;
		break;
	case SENSOR_ID_S5KJN3:
		ret = e_seq_sensor_idn_s5kjn3;
		break;
	case SENSOR_ID_IMX874:
		ret = e_seq_sensor_idn_imx874;
		break;
	default:
		ret = e_seq_sensor_idn_max_invalid;
		break;
	}
	return ret;
}


inline int32_t get_vc_pick_idx(uint32_t sensor_id)
{
	switch (sensor_id)
	{
	case SENSOR_ID_S5KGN3:
	case SENSOR_ID_S5K3LU:
		return e_vc_addr_260_264_1b_read;
	case SENSOR_ID_S5KJN3:
		return e_vc_addr_260_264_2b_read;
	case SENSOR_ID_S5KHP2:
		return e_vc_addr_110_264;
	case SENSOR_ID_IMX564:
	case SENSOR_ID_IMX754:
		return e_vc_addr_110_30b0;
	case SENSOR_ID_S5K2LD:
		return e_vc_addr_602a_6f12;
	case SENSOR_ID_IMX854:
		return e_vc_addr_110_30a4;
	}
	return e_vc_addr_invalid;
}


void cam_sensor_parse_reg(
	struct cam_sensor_ctrl_t* s_ctrl,
	struct i2c_settings_list* i2c_list,
	int32_t *dbg_sen_id,
	int32_t *update_type
	)
{
	uint32_t i;
	int32_t vc_pick_idx = e_vc_addr_invalid;
	bool need_to_check_exp_ctl_violation = false;
	uint32_t seq_sen_id = e_seq_sensor_idn_max_invalid;

	if (s_ctrl == NULL || i2c_list == NULL) {
		return;
	}

	vc_pick_idx = get_vc_pick_idx(s_ctrl->sensordata->slave_info.sensor_id);
	if (vc_pick_idx == e_vc_addr_invalid) {
		CAM_DBG(CAM_SENSOR_UTIL, "invalid vc_pick_idx");
		return;
	}
	seq_sen_id = get_seq_sensor_id(s_ctrl->sensordata->slave_info.sensor_id);

	if ((seq_sen_id == e_seq_sensor_idn_s5khp2) ||
		(seq_sen_id == e_seq_sensor_idn_s5kgn3)) {
		need_to_check_exp_ctl_violation = true;
	}

	if (i2c_list->op_code == CAM_SENSOR_I2C_WRITE_RANDOM) {
		for (i = 0; i < i2c_list->i2c_settings.size; i++) {
			if (i2c_list->i2c_settings.reg_setting[i].reg_addr == reg_addr_aeb_on) {
				*dbg_sen_id = s_ctrl->sensordata->slave_info.sensor_id;// AEB ctrl change results in vc change
				*update_type |= e_sensor_upd_event_vc;
			}
			else if ((i2c_list->i2c_settings.reg_setting[i].reg_addr == vc_addr_info[vc_pick_idx].img_addr) ||
				(i2c_list->i2c_settings.reg_setting[i].reg_addr == vc_addr_info[vc_pick_idx].pd_addr)) {
				*dbg_sen_id = s_ctrl->sensordata->slave_info.sensor_id;
				*update_type |= e_sensor_upd_event_vc;
			}
			if (need_to_check_exp_ctl_violation) {
				if ((i2c_list->i2c_settings.reg_setting[i].reg_addr == sensordbg_addr_info[seq_sen_id].cit) ||
					(i2c_list->i2c_settings.reg_setting[i].reg_addr == sensordbg_addr_info[seq_sen_id].aeb_cit_l) ||
					(i2c_list->i2c_settings.reg_setting[i].reg_addr == sensordbg_addr_info[seq_sen_id].aeb_cit_s)) {
					*update_type |= e_sensor_upd_event_exposure;
				}
			}
		}
	}
}


int cam_sensor_check_aeb_on(struct cam_sensor_ctrl_t* s_ctrl)
{
	int	rc = 0;
	uint32_t reg_addr_aeb_ctrl = 0xe00;
	int is_aeb_activated = false;
	uint32_t val_aeb_ctrl = 0;
	struct cam_camera_slave_info* slave_info;

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR_UTIL,	" failed: %pK",
			slave_info);
		return SENSOR_API_EINVALID;
	}
	if(SENSOR_ID_S5K2LD == s_ctrl->sensordata->slave_info.sensor_id) {
		reg_addr_aeb_ctrl = 0xe0a;
	}

	rc = camera_io_dev_read(
		&(s_ctrl->io_master_info),
		reg_addr_aeb_ctrl,
		&val_aeb_ctrl, CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD, false);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", reg_addr_aeb_ctrl);
		return SENSOR_API_EINVALID;
	}

	switch (s_ctrl->sensordata->slave_info.sensor_id)
	{
	case SENSOR_ID_S5KHP2:
		is_aeb_activated = ((val_aeb_ctrl & 0xf) == 0x3) ? SENSOR_API_RESULT_AEB_ON : SENSOR_API_RESULT_AEB_OFF;
		break;
	case SENSOR_ID_S5KGN3:
	case SENSOR_ID_S5K3LU:
	case SENSOR_ID_IMX564:
	case SENSOR_ID_IMX754:
	case SENSOR_ID_S5K3K1:
	case SENSOR_ID_IMX854:
	case SENSOR_ID_S5KJN3:
		is_aeb_activated = ((val_aeb_ctrl & 0x0f00) == 0x200) ? SENSOR_API_RESULT_AEB_ON : SENSOR_API_RESULT_AEB_OFF;
		break;
	case SENSOR_ID_S5K2LD:
		is_aeb_activated = ((val_aeb_ctrl & 0xf) == 0x2) ? SENSOR_API_RESULT_AEB_ON : SENSOR_API_RESULT_AEB_OFF;
		break;
	default:
		break;
	}
	CAM_DBG(CAM_SENSOR, "[AEB_DBG][%s] read addr 0x%x data 0x%x",
			s_ctrl->sensor_name,
			reg_addr_aeb_ctrl,
			val_aeb_ctrl
			);
	return is_aeb_activated;
}


void cam_sensor_dbg_print_vc(struct cam_sensor_ctrl_t* s_ctrl)
{
	int	rc = 0;
	uint32_t vc_img	= 0, vc_pd_h = 0, vc_pd_v = 0;
	uint32_t vc_pick_idx = e_vc_addr_invalid;
	int is_aeb_activated = -1;
	uint32_t seq_sen_id = get_seq_sensor_id(s_ctrl->sensordata->slave_info.sensor_id);

	struct cam_camera_slave_info* slave_info;

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR_UTIL,	" failed: %pK",
			slave_info);
		return;
	}

	if (seq_sen_id == e_seq_sensor_idn_max_invalid) {
		return;
	}

	switch (s_ctrl->sensordata->slave_info.sensor_id)
	{
	case SENSOR_ID_S5KHP2:
		vc_pick_idx = e_vc_addr_110_264;
		break;
	case SENSOR_ID_S5KGN3:
	case SENSOR_ID_S5K3LU:
		vc_pick_idx = e_vc_addr_260_264_1b_read;
		break;
	case SENSOR_ID_S5KJN3:
		vc_pick_idx = e_vc_addr_260_264_2b_read;
		break;
	case SENSOR_ID_IMX564:
	case SENSOR_ID_IMX754:
		vc_pick_idx = e_vc_addr_110_30b0;
		break;
	case SENSOR_ID_IMX854:
	case SENSOR_ID_IMX874:
		vc_pick_idx = e_vc_addr_110_30a4;
		break;
	case SENSOR_ID_S5K3K1:
		vc_pick_idx = e_vc_addr_invalid;
		break;
	case SENSOR_ID_S5K2LD:
		vc_pick_idx = e_vc_addr_602a_6f12;
		break;
	default:
		return;
	}

	is_aeb_activated = cam_sensor_check_aeb_on(s_ctrl);
	if (is_aeb_activated < 0)
		return;

	if (SENSOR_API_RESULT_AEB_ON == is_aeb_activated)
	{
		uint32_t vc_l = 0, vc_s = 0;

		if (sensordbg_addr_info[seq_sen_id].aeb_vc_sz != CAMERA_SENSOR_I2C_TYPE_INVALID) {
			rc = camera_io_dev_read(
				&(s_ctrl->io_master_info),
				sensordbg_addr_info[seq_sen_id].aeb_vc_l,
				&vc_l, CAMERA_SENSOR_I2C_TYPE_WORD,
				sensordbg_addr_info[seq_sen_id].aeb_vc_sz, false);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].aeb_vc_l);
			}
			rc = camera_io_dev_read(
				&(s_ctrl->io_master_info),
				sensordbg_addr_info[seq_sen_id].aeb_vc_s,
				&vc_s, CAMERA_SENSOR_I2C_TYPE_WORD,
				sensordbg_addr_info[seq_sen_id].aeb_vc_sz, false);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].aeb_vc_s);
			}

			CAM_INFO(CAM_SENSOR_UTIL, "[AEB_DBG]%s[%s] AEB switched on : VC img_l(0x%x) 0x%x img_s(0x%x) 0x%x, reqId %lld",
				s_ctrl->is_bubble_packet == true ? "[BUBBLE]" : "",
				s_ctrl->sensor_name,
				sensordbg_addr_info[seq_sen_id].aeb_vc_l, vc_l,
				sensordbg_addr_info[seq_sen_id].aeb_vc_s, vc_s, s_ctrl->last_applied_req);
		} else {
			CAM_INFO(CAM_SENSOR_UTIL, "[AEB_DBG]%s[%s] AEB switched on : odd vc, reqId %lld",
				s_ctrl->is_bubble_packet == true ? "[BUBBLE]" : "",
				s_ctrl->sensor_name, s_ctrl->last_applied_req);
		}
	} else {
		if (vc_pick_idx == e_vc_addr_invalid) {
			CAM_INFO(CAM_SENSOR_UTIL, "[AEB_DBG]%s[%s] AEB off, reqId %lld",
				s_ctrl->is_bubble_packet == true ? "[BUBBLE]" : "",
				s_ctrl->sensor_name, s_ctrl->last_applied_req);
		} else {
			rc = camera_io_dev_read(
				&(s_ctrl->io_master_info),
				vc_addr_info[vc_pick_idx].img_addr,
				&vc_img, CAMERA_SENSOR_I2C_TYPE_WORD,
				vc_addr_info[vc_pick_idx].data_sz, false);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", vc_addr_info[vc_pick_idx].img_addr);
			}

			rc = camera_io_dev_read(
				&(s_ctrl->io_master_info),
				vc_addr_info[vc_pick_idx].pd_addr,
				&vc_pd_h, CAMERA_SENSOR_I2C_TYPE_WORD,
				vc_addr_info[vc_pick_idx].data_sz, false);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", vc_addr_info[vc_pick_idx].pd_addr);
			}

			if ((s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5KHP2) ||
				(s_ctrl->sensordata->slave_info.sensor_id == SENSOR_ID_S5KJN3)) {
				vc_img = (vc_img & 0xf00) >> 8;
				vc_pd_v = (vc_pd_h & 0xf);
				vc_pd_h = (vc_pd_h & 0xf00) >> 8;
			}

			CAM_INFO(CAM_SENSOR_UTIL, "[AEB_DBG]%s[%s] AEB off, VC img 0x%x, hpd 0x%x, vpd 0x%x (%dByte), reqId %lld",
				s_ctrl->is_bubble_packet == true ? "[BUBBLE]" : "",
				s_ctrl->sensor_name,
				vc_img, vc_pd_h, vc_pd_v, vc_addr_info[vc_pick_idx].data_sz, s_ctrl->last_applied_req);
		}
	}
}


void cam_sensor_dbg_exp_violation(struct cam_sensor_ctrl_t* s_ctrl)
{
	 int rc = 0;
	 bool is_aeb_activated = false;
	uint32_t seq_sen_id = get_seq_sensor_id(s_ctrl->sensordata->slave_info.sensor_id);

	struct cam_camera_slave_info* slave_info;

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR_UTIL, " failed: %pK",
			slave_info);
		return;
	}

	// 1. if sony sesor, return
	if ((seq_sen_id != e_seq_sensor_idn_s5khp2) &&
		(seq_sen_id != e_seq_sensor_idn_s5kgn3)) {
		return;
	}

	is_aeb_activated = cam_sensor_check_aeb_on(s_ctrl);

	// 2. if AEB on
	if (is_aeb_activated) {
		// compare AEB L/S cit and AEB FLL
		uint32_t val_aeb_cit_l = 0, val_aeb_cit_s = 0;
		uint32_t val_aeb_fll_l = 0, val_aeb_fll_s = 0;
		bool long_cit_violation = false, short_cit_violation = false;

		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensordbg_addr_info[seq_sen_id].aeb_cit_l,
			&val_aeb_cit_l, CAMERA_SENSOR_I2C_TYPE_WORD,
			sensordbg_addr_info[seq_sen_id].data_sz, false);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].aeb_cit_l);
		}
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensordbg_addr_info[seq_sen_id].aeb_fll_l,
			&val_aeb_fll_l, CAMERA_SENSOR_I2C_TYPE_WORD,
			sensordbg_addr_info[seq_sen_id].data_sz, false);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].aeb_fll_l);
		}
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensordbg_addr_info[seq_sen_id].aeb_cit_s,
			&val_aeb_cit_s, CAMERA_SENSOR_I2C_TYPE_WORD,
			sensordbg_addr_info[seq_sen_id].data_sz, false);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].aeb_cit_s);
		}
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensordbg_addr_info[seq_sen_id].aeb_fll_s,
			&val_aeb_fll_s, CAMERA_SENSOR_I2C_TYPE_WORD,
			sensordbg_addr_info[seq_sen_id].data_sz, false);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].aeb_fll_s);
		}
		long_cit_violation = (val_aeb_cit_l > val_aeb_fll_l) ? true : false;
		short_cit_violation = (val_aeb_cit_s > val_aeb_fll_s) ? true : false;

		if (long_cit_violation || short_cit_violation)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] %s %s (l: 0x%x 0x%x, s: 0x%x 0x%x)",
				long_cit_violation ? "LONG_CIT_VIOLATION" : "",
				short_cit_violation ? "SHORT_CIT_VIOLATION" : "",
				val_aeb_cit_l, val_aeb_fll_l,
				val_aeb_cit_s, val_aeb_fll_s);
	}
	// 3. if AEB off
	else {
		uint32_t val_cit = 0;
		uint32_t val_fll = 0;
		bool cit_violation = false;

		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensordbg_addr_info[seq_sen_id].cit,
			&val_cit, CAMERA_SENSOR_I2C_TYPE_WORD,
			sensordbg_addr_info[seq_sen_id].data_sz, false);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].cit);
		}
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensordbg_addr_info[seq_sen_id].fll,
			&val_fll, CAMERA_SENSOR_I2C_TYPE_WORD,
			sensordbg_addr_info[seq_sen_id].data_sz, false);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR_UTIL, "Failed to read 0x%x", sensordbg_addr_info[seq_sen_id].fll);
		}
		// compare cit and FLL
		cit_violation = (val_cit > val_fll) ? true : false;
		if (cit_violation)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] CIT_VIOLATION (0x%x 0x%x)",
				val_cit, val_fll);
	}
}


void cam_sensor_dbg_print_by_upd_type(struct cam_sensor_ctrl_t* s_ctrl, int32_t sen_update_type)
{
	if ((sen_update_type & e_sensor_upd_event_vc) == e_sensor_upd_event_vc)
		cam_sensor_dbg_print_vc(s_ctrl);

	if ((sen_update_type & e_sensor_upd_event_exposure) == e_sensor_upd_event_exposure)
		cam_sensor_dbg_exp_violation(s_ctrl);
}

void cam_sensor_i2c_dump_util(
	struct cam_sensor_ctrl_t *s_ctrl,
	struct i2c_settings_list *i2c_list,
	int i2c_debug_cnt)
{
	uint32_t size=0, i;

	if (s_ctrl == NULL || i2c_list == NULL) {
		return;
	}

	size = i2c_list->i2c_settings.size;
	if (strstr(debug_sensor_name, s_ctrl->sensor_name) != NULL) {
		CAM_INFO(CAM_SENSOR_UTIL, "sensor_name:%s", s_ctrl->sensor_name);
		for (i = 0; i < size; i++) {
			if (i2c_debug_cnt == 0) {
				const char* str_i2c_cmd[CAM_SENSOR_I2C_POLL + 1] = {
					"[W]", "[WBur]", "[WSeq]", "[R]", "[RSeq]", "[P]" };

				CAM_INFO(CAM_SENSOR_UTIL, "%s%s addr:0x%4x data:0x%4x",
					str_i2c_cmd[i2c_list->op_code],
					s_ctrl->is_bubble_packet ? "[BUBBLE]" : "",
					i2c_list->i2c_settings.reg_setting[i].reg_addr,
					i2c_list->i2c_settings.reg_setting[i].reg_data);
			} else if (i2c_debug_cnt > 0) {
				if (i == 0) {
					CAM_INFO(CAM_SENSOR_UTIL,
						"[I2C_DBG] ====== op_code : %d, size : %d ======",
						i2c_list->op_code, i2c_list->i2c_settings.size);
				}
				CAM_INFO(CAM_SENSOR_UTIL,
					"[I2C_DBG] [%d] addr : 0x%04X, data : 0x%04X", i,
					i2c_list->i2c_settings.reg_setting[i].reg_addr,
					i2c_list->i2c_settings.reg_setting[i].reg_data);
				if (i >= i2c_debug_cnt)
					break;
			}
		}
	}
}


void cam_sensor_dbg_regdump_common(struct cam_sensor_ctrl_t* s_ctrl)
{
	uint32_t i;

	int rc = 0;
	int val = 0;

	int tbl_size = 0;
	const struct st_exposure_reg_dump_addr* p_arr = NULL;

	switch (s_ctrl->sensordata->slave_info.sensor_id)
	{
		case SENSOR_ID_S5KGN3:
			p_arr = gn3_dump_addr_arr;
			tbl_size = sizeof(gn3_dump_addr_arr) / sizeof(struct st_exposure_reg_dump_addr);
			break;
		case SENSOR_ID_S5KJN3:
			p_arr = jn3_dump_addr_arr;
			tbl_size = sizeof(jn3_dump_addr_arr) / sizeof(struct st_exposure_reg_dump_addr);
			break;
		case SENSOR_ID_IMX564:
		case SENSOR_ID_IMX754:
			p_arr = imx564_754_exp_addr_info;
			tbl_size = sizeof(imx564_754_exp_addr_info) / sizeof(struct st_exposure_reg_dump_addr);
			break;
		default:
			break;
	}

	if (p_arr == NULL) return;

	for (i = 0; i < tbl_size; i++)
	{
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			p_arr->addr,
			&val, CAMERA_SENSOR_I2C_TYPE_WORD,
			p_arr->data_sz, false);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR, "[SEN_DBG] read fail");
		else {
			CAM_INFO(CAM_SENSOR, "[SEN_DBG] addr:0x%4x  val:0x%4x //%s(%dB)",
				p_arr->addr, val,
				p_arr->addr_name,
				p_arr->data_sz);
		}
		p_arr++;
	}
}

void cam_sensor_dbg_regdump_hp2(struct cam_sensor_ctrl_t* s_ctrl)
{
	struct cam_sensor_i2c_reg_setting dbg_reg_setting_p4000;
	struct cam_sensor_i2c_reg_setting dbg_reg_setting_p1001;
	struct cam_sensor_i2c_reg_setting dbg_reg_setting_p1000;
	struct cam_sensor_i2c_reg_setting dbg_reg_setting_p1003;

	uint32_t i;

	int rc = 0;
	int val = 0;
	int size = 0;

	size = ARRAY_SIZE(hp2_page_4000_reg_array);
	dbg_reg_setting_p4000.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * size, GFP_KERNEL);
	if (dbg_reg_setting_p4000.reg_setting != NULL) {
		dbg_reg_setting_p4000.size = size;
		dbg_reg_setting_p4000.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p4000.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p4000.delay = 0;
		memcpy(dbg_reg_setting_p4000.reg_setting, &hp2_page_4000_reg_array, sizeof(struct cam_sensor_i2c_reg_array) * size);
	}

	size = ARRAY_SIZE(hp2_page_1001_reg_array);
	dbg_reg_setting_p1001.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * size, GFP_KERNEL);
	if (dbg_reg_setting_p1001.reg_setting != NULL) {
		dbg_reg_setting_p1001.size = size;
		dbg_reg_setting_p1001.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p1001.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p1001.delay = 0;
		memcpy(dbg_reg_setting_p1001.reg_setting, &hp2_page_1001_reg_array, sizeof(struct cam_sensor_i2c_reg_array) * size);
	}

	size = ARRAY_SIZE(hp2_page_1000_reg_array);
	dbg_reg_setting_p1000.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * size, GFP_KERNEL);
	if (dbg_reg_setting_p1000.reg_setting != NULL) {
		dbg_reg_setting_p1000.size = size;
		dbg_reg_setting_p1000.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p1000.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p1000.delay = 0;
		memcpy(dbg_reg_setting_p1000.reg_setting, &hp2_page_1000_reg_array, sizeof(struct cam_sensor_i2c_reg_array) * size);
	}

	size = ARRAY_SIZE(hp2_page_1003_reg_array);
	dbg_reg_setting_p1003.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * size, GFP_KERNEL);
	if (dbg_reg_setting_p1003.reg_setting != NULL) {
		dbg_reg_setting_p1003.size = size;
		dbg_reg_setting_p1003.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p1003.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_setting_p1003.delay = 0;
		memcpy(dbg_reg_setting_p1003.reg_setting, &hp2_page_1003_reg_array, sizeof(struct cam_sensor_i2c_reg_array) * size);
	}

	/*
	 *  page 4000
	 */
	rc = camera_io_dev_write(&s_ctrl->io_master_info, &dbg_reg_setting_p4000);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] Failed to write dbg_write_setting_4000 %d", rc);

	for (i = 0; i < sizeof(hp2_exp_addr_info) / sizeof(struct st_exposure_reg_dump_addr); i++)
	{
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			hp2_exp_addr_info[i].addr,
			&val, CAMERA_SENSOR_I2C_TYPE_WORD,
			hp2_exp_addr_info[i].data_sz, false);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] read fail");
		else {
			CAM_INFO(CAM_SENSOR_UTIL, "[SEN_DBG] addr: 0x%4x  val: 0x%4x //%s",
				hp2_exp_addr_info[i].addr, val, hp2_exp_addr_info[i].addr_name);
		}
	}


	for (i = 0; i < sizeof(hp2_dump_addr_page4000) / sizeof(uint32_t); i++)
	{
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			hp2_dump_addr_page4000[i],
			&val, CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD, false);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] read fail");
		else {
			CAM_INFO(CAM_SENSOR_UTIL, "[SEN_DBG] PAGE_4000 addr:0x%4x  val:0x%4x",
				hp2_dump_addr_page4000[i], val);
		}
	}


	/*
	 *  page 1001
	 */
	rc = camera_io_dev_write(&s_ctrl->io_master_info, &dbg_reg_setting_p1001);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] Failed to write dbg_write_setting_1001 %d", rc);

	for (i = 0; i < sizeof(hp2_dump_addr_page1001) / sizeof(uint32_t); i++)
	{
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			hp2_dump_addr_page1001[i],
			&val, CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD, false);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] read fail");
		else {
			CAM_INFO(CAM_SENSOR_UTIL, "[SEN_DBG] PAGE_1001 addr:0x%4x  val:0x%4x",
				hp2_dump_addr_page1001[i], val);
		}
	}

	/*
	 *  page 1000
	 */
	rc = camera_io_dev_write(&s_ctrl->io_master_info, &dbg_reg_setting_p1000);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] Failed to write dbg_write_setting_1000 %d", rc);

	for (i = 0; i < sizeof(hp2_dump_addr_page1000) / sizeof(uint32_t); i++)
	{
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			hp2_dump_addr_page1000[i],
			&val, CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD, false);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] read fail");
		else {
			CAM_INFO(CAM_SENSOR_UTIL, "[SEN_DBG] PAGE_1000 addr:0x%4x  val:0x%4x",
				hp2_dump_addr_page1000[i], val);
		}
	}

	/*
	 *  page 1003
	 */
	rc = camera_io_dev_write(&s_ctrl->io_master_info, &dbg_reg_setting_p1003);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] Failed to write dbg_write_setting_1003 %d", rc);

	for (i = 0; i < sizeof(dump_addr_page1003) / sizeof(uint32_t); i++)
	{
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			dump_addr_page1003[i],
			&val, CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD, false);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] read fail");
		else {
			CAM_INFO(CAM_SENSOR_UTIL, "[SEN_DBG] PAGE_1003 addr:0x%4x  val:0x%4x",
				dump_addr_page1003[i], val);
		}
	}

	if (dbg_reg_setting_p4000.reg_setting != NULL) {
		kfree(dbg_reg_setting_p4000.reg_setting);
		dbg_reg_setting_p4000.reg_setting = NULL;
	}
	if (dbg_reg_setting_p1001.reg_setting != NULL) {
		kfree(dbg_reg_setting_p1001.reg_setting);
		dbg_reg_setting_p1001.reg_setting = NULL;
	}
	if (dbg_reg_setting_p1000.reg_setting != NULL) {
		kfree(dbg_reg_setting_p1000.reg_setting);
		dbg_reg_setting_p1000.reg_setting = NULL;
	}
	if (dbg_reg_setting_p1003.reg_setting != NULL) {
		kfree(dbg_reg_setting_p1003.reg_setting);
		dbg_reg_setting_p1003.reg_setting = NULL;
	}
}


void cam_sensor_dbg_regdump(struct cam_sensor_ctrl_t* s_ctrl)
{
	cam_sensor_dbg_print_vc(s_ctrl);

	switch (s_ctrl->sensordata->slave_info.sensor_id)
	{
	case SENSOR_ID_S5KHP2:
		cam_sensor_dbg_regdump_hp2(s_ctrl);
		break;
	case SENSOR_ID_S5KGN3:
	case SENSOR_ID_S5KJN3:
	case SENSOR_ID_IMX564:
	case SENSOR_ID_IMX754:
		cam_sensor_dbg_regdump_common(s_ctrl);
		break;
	default:
		CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] not supported 0x%x", s_ctrl->sensordata->slave_info.sensor_id);
		break;
	}
}


const uint32_t dump_addr_when_stream_on_fail_hp2[] = {
	0xA200, 0xA202, 0xA204, 0xA206, 0xA20A,
};

void cam_sensor_dbg_regdump_stream_on_fail_hp2(struct cam_sensor_ctrl_t* s_ctrl)
{
	struct cam_sensor_i2c_reg_setting dbg_reg_page4000;
	struct cam_sensor_i2c_reg_setting dbg_reg_page1001;
	uint32_t i;

	int32_t rc = 0;
	uint32_t val = 0;
	uint32_t size = 0;

	size = ARRAY_SIZE(hp2_page_4000_reg_array);
	dbg_reg_page4000.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * size, GFP_KERNEL);
	if (dbg_reg_page4000.reg_setting != NULL) {
		dbg_reg_page4000.size = size;
		dbg_reg_page4000.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_page4000.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_page4000.delay = 0;
		memcpy(dbg_reg_page4000.reg_setting, &hp2_page_4000_reg_array, sizeof(struct cam_sensor_i2c_reg_array) * size);
	}

	size = ARRAY_SIZE(hp2_page_1001_reg_array);
	dbg_reg_page1001.reg_setting = kmalloc(sizeof(struct cam_sensor_i2c_reg_array) * size, GFP_KERNEL);
	if (dbg_reg_page1001.reg_setting != NULL) {
		dbg_reg_page1001.size = size;
		dbg_reg_page1001.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_page1001.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		dbg_reg_page1001.delay = 0;
		memcpy(dbg_reg_page1001.reg_setting, &hp2_page_1001_reg_array, sizeof(struct cam_sensor_i2c_reg_array) * size);
	}

	// page 1001
	rc = camera_io_dev_write(&s_ctrl->io_master_info, &dbg_reg_page1001);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] Failed to write dbg_write_setting_1001 %d", rc);

	for (i = 0; i < sizeof(dump_addr_when_stream_on_fail_hp2) / sizeof(uint32_t); i++)
	{
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			dump_addr_when_stream_on_fail_hp2[i],
			&val, CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD, false);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] read fail");
		else {
			CAM_INFO(CAM_SENSOR_UTIL, "[SEN_DBG] addr:0x%4x val:0x%4x",
				dump_addr_when_stream_on_fail_hp2[i], val);
		}
	}

	// page 4000
	rc = camera_io_dev_write(&s_ctrl->io_master_info, &dbg_reg_page4000);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR_UTIL, "[SEN_DBG] Failed to write dbg_write_setting_4000 %d", rc);

	if (dbg_reg_page4000.reg_setting != NULL) {
		kfree(dbg_reg_page4000.reg_setting);
		dbg_reg_page4000.reg_setting = NULL;
	}
	if (dbg_reg_page1001.reg_setting != NULL) {
		kfree(dbg_reg_page1001.reg_setting);
		dbg_reg_page1001.reg_setting = NULL;
	}
}


void cam_sensor_dbg_regdump_stream_on_fail(struct cam_sensor_ctrl_t* s_ctrl)
{
	switch (s_ctrl->sensordata->slave_info.sensor_id)
	{
	case SENSOR_ID_S5KHP2:
		cam_sensor_dbg_regdump_stream_on_fail_hp2(s_ctrl);
		break;
	default:
		CAM_DBG(CAM_SENSOR_UTIL, "[SEN_DBG] not supported 0x%x", s_ctrl->sensordata->slave_info.sensor_id);
		return;
	}
}



#endif
