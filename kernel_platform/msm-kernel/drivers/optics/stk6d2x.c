/*
 *  stk6d2x.c - Linux kernel modules for sensortek stk6d2x
 *  ambient light sensor (driver)
 *
 *  Copyright (C) 2012~2018 Bk, sensortek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stk6d2x.h>
#include <linux/delay.h>

/****************************************************************************************************
* Declaration function
****************************************************************************************************/
static int32_t stk6d2x_set_als_thd(struct stk6d2x_data *alps_data, uint16_t thd_h, uint16_t thd_l);
void stk6d2x_get_data_polling(stk_timer_info *t_info);

#ifdef STK_FIFO_ENABLE
	void stk6d2x_fifo_switch_it(struct stk6d2x_data *alps_data, bool is_long_it);
#endif

/****************************************************************************************************
* Declaration Initiation Setting
****************************************************************************************************/
stk6d2x_register_table stk6d2x_default_register_table[] =
{
	{STK6D2X_REG_ALS01_DGAIN,       ((STK6D2X_ALS_DGAIN128 << STK6D2X_ALS0_DGAIN_SHIFT)  | STK6D2X_ALS_DGAIN128),   0xFF},
	{STK6D2X_REG_ALS2_DGAIN,        (STK6D2X_ALS_DGAIN128 << STK6D2X_ALS2_DGAIN_SHIFT),         0xFF},
#ifndef STK_FIFO_ENABLE
	{STK6D2X_REG_IT1,               ((STK6D2X_ALS_IT50 >> 8) & STK6D2X_ALS_IT1_MASK),           0xFF},
	{STK6D2X_REG_IT2,               (STK6D2X_ALS_IT50 & 0xFF),                                  0xFF},
#endif
	{STK6D2X_REG_WAIT1,             0x00,                                                       0xFF},
	{STK6D2X_REG_WAIT2,             0x00,                                                       0xFF},
#ifdef STK_DATA_SUMMATION
	{STK6D2X_REG_ALS_SUM_GAIN1,     (STK6D2X_ALS_SUM_GAIN_DIV256 << 4) | STK6D2X_ALS_SUM_GAIN_DIV256,   0xFF},
	{STK6D2X_REG_ALS_SUM_GAIN2,     (STK6D2X_ALS_SUM_GAIN_DIV256 << 4),                                 0xFF},
	{STK6D2X_REG_ALS_SUM,           0x32,                                                       0xFF},
#endif
#ifndef STK_FIFO_ENABLE
	{STK6D2X_REG_ALS_IT_EXT,        0x00,                                                       0xFF},
#endif
	{STK6D2X_REG_ALS_PRST,          0x00,                                                       0xFF},
#ifdef STK_ALS_AGC
	{STK6D2X_REG_AGC1,              0x0F,                                                       0xFF},
	{STK6D2X_REG_AGC2,              0x70,                                                       0xFF},
#endif
#ifdef STK_FIFO_ENABLE
	{STK6D2X_REG_FIFO1,             STK6D2X_FIFO_SEL_STA01_STA2_ALS0_ALS1_ALS2,                 0xFF},
	{STK6D2X_REG_IT1,               ((STK_FLICKER_IT >> 8) & STK6D2X_ALS_IT1_MASK),                       0xFF},
	{STK6D2X_REG_IT2,               (STK_FLICKER_IT & 0xFF),                                              0xFF},
	{STK6D2X_REG_ALS_IT_EXT,        STK_FLICKER_EXT_IT,                                                       0xFF},
#endif
	{STK6D2X_REG_ALS_AGAIN,         0x00,                                                       0xFF},
	{0xF3,                          0x03,                                                       0xFF},
	{0x4F,                          0x0F,                                                       0xFF},
	{0x50,                          0x04,                                                       0xFF},
	{0xD1,                          0x16,                                                       0x1F}, //GC Setting
	{0xE3,                          0x80,                                                       0xFF}, //WDT 50ms
	{0xE5,                          0x80,                                                       0xFF}, //WDT 50ms
	{0xEA,                          0x80,                                                       0xFF}, //WDT 50ms
	{0xEC,                          0x80,                                                       0xFF}, //WDT 50ms
};

stk6d2x_register_table stk6d2x_default_otp_table[] =
{
	{0x93,  0x01,   0x01},
	{0x90,  0x11,   0x3f},
	{0x92,  0x01,   0x02},
	{0x93,  0x00,   0x01},
};

uint8_t stk6d2x_pid_list[] = {0x30};

uint32_t stk6d2x_power(uint32_t base, uint32_t exp)
{
	uint32_t result = 1;

	while (exp)
	{
		if (exp & 1)
			result *= base;

		exp >>= 1;
		base *= base;
	}

	return result;
}

/****************************************************************************************************
* stk6d2x.c API
****************************************************************************************************/
int32_t stk6d2x_request_registry(struct stk6d2x_data *alps_data)
{
	uint32_t FILE_SIZE = sizeof(stk6d2x_cali_table);
	uint8_t *file_out_buf;
	int32_t err = 0;
	ALS_info("start\n");
	file_out_buf = kzalloc(FILE_SIZE, GFP_KERNEL);
	memset(file_out_buf, 0, FILE_SIZE);
	return 0;
}

int32_t stk6d2x_update_registry(struct stk6d2x_data *alps_data)
{
	uint32_t FILE_SIZE = sizeof(stk6d2x_cali_table);
	uint8_t *file_buf;
	int32_t err = 0;
	ALS_info("start\n");
	file_buf = kzalloc(FILE_SIZE, GFP_KERNEL);
	memset(file_buf, 0, FILE_SIZE);
	memcpy(file_buf, &alps_data->cali_info.cali_para.als_version, FILE_SIZE);
	//err = STK6D2X_W_TO_STORAGE(alps_data, STK6D2X_CALI_FILE, file_buf, FILE_SIZE);

	if (err < 0)
	{
		kfree(file_buf);
		return -1;
	}

	ALS_info("Done\n");
	kfree(file_buf);
	return 0;
}

void stk6d2x_get_reg_default_setting(uint8_t reg, uint16_t* value)
{
	uint16_t reg_count = 0, reg_num = sizeof(stk6d2x_default_register_table) / sizeof(stk6d2x_register_table);

	for (reg_count = 0; reg_count < reg_num; reg_count++)
	{
		if (stk6d2x_default_register_table[reg_count].address == reg)
		{
			*value = (uint16_t)stk6d2x_default_register_table[reg_count].value;
			return;
		}
	}

	*value = 0x7FFF;
}

static int32_t stk6d2x_register_timer(struct stk6d2x_data *alps_data, stk6d2x_timer_type timer_type)
{
	ALS_info("regist 0x%X timer\n", timer_type);

	switch (timer_type)
	{
		case STK6D2X_DATA_TIMER_ALPS:
			strcpy(alps_data->alps_timer_info.wq_name, "stk_alps_wq");
			alps_data->alps_timer_info.timer_unit = M_SECOND;
			alps_data->alps_timer_info.interval_time = alps_data->als_info.als_it;
			alps_data->alps_timer_info.timer_cb = stk6d2x_get_data_polling;
			alps_data->alps_timer_info.is_active = false;
			alps_data->alps_timer_info.is_exist = false;
			alps_data->alps_timer_info.any = alps_data;
			STK6D2X_TIMER_REGISTER(alps_data, &alps_data->alps_timer_info);
			break;
	}

	return 0;
}

void stk6d2x_als_set_new_thd(struct stk6d2x_data *alps_data, uint16_t alscode)
{
	uint16_t high_thd, low_thd;
	high_thd = alscode + STK6D2X_ALS_THRESHOLD;
	low_thd  = (alscode > STK6D2X_ALS_THRESHOLD) ? (alscode - STK6D2X_ALS_THRESHOLD) : 0;
	stk6d2x_set_als_thd(alps_data, (uint16_t)high_thd, (uint16_t)low_thd);
}

void stk6d2x_dump_reg(struct stk6d2x_data *alps_data)
{
	uint8_t stk6d2x_reg_map[] =
	{
		STK6D2X_REG_STATE,
		STK6D2X_REG_ALS01_DGAIN,
		STK6D2X_REG_ALS2_DGAIN,
		STK6D2X_REG_IT1,
		STK6D2X_REG_IT2,
		STK6D2X_REG_WAIT1,
		STK6D2X_REG_WAIT2,
		STK6D2X_REG_ALS_SUM_GAIN1,
		STK6D2X_REG_ALS_SUM_GAIN2,
		STK6D2X_REG_THDH1_ALS,
		STK6D2X_REG_THDH2_ALS,
		STK6D2X_REG_THDL1_ALS,
		STK6D2X_REG_THDL2_ALS,
		STK6D2X_REG_ALS_IT_EXT,
		STK6D2X_REG_FLAG,
		STK6D2X_REG_DATA1_ALS0,
		STK6D2X_REG_DATA2_ALS0,
		STK6D2X_REG_DATA1_ALS1,
		STK6D2X_REG_DATA2_ALS1,
		STK6D2X_REG_DATA1_ALS2,
		STK6D2X_REG_DATA2_ALS2,
		STK6D2X_REG_AGC1_DG,
		STK6D2X_REG_AGC2_DG,
		STK6D2X_REG_AGC_CROS_THD_FLAG,
		STK6D2X_REG_AGC_AG,
		STK6D2X_REG_AGC_PD,
		STK6D2X_REG_DATA1_ALS0_SUM,
		STK6D2X_REG_DATA2_ALS0_SUM,
		STK6D2X_REG_DATA1_ALS1_SUM,
		STK6D2X_REG_DATA2_ALS1_SUM,
		STK6D2X_REG_DATA1_ALS2_SUM,
		STK6D2X_REG_DATA2_ALS2_SUM,
		STK6D2X_REG_DATA_AGC_SUM,
		STK6D2X_REG_ALS_PRST,
		STK6D2X_REG_FIFO1,
		STK6D2X_REG_FIFO1_WM_LV,
		STK6D2X_REG_FIFO2_WM_LV,
		STK6D2X_REG_FIFO_FCNT1,
		STK6D2X_REG_FIFO_FCNT2,
		STK6D2X_REG_FIFO_OUT,
		STK6D2X_REG_AGC1,
		STK6D2X_REG_AGC2,
		STK6D2X_REG_ALS_SUM,
		STK6D2X_REG_SW_RESET,
		STK6D2X_REG_PDT_ID,
		STK6D2X_REG_INT2,
		STK6D2X_REG_ALS_AGAIN,
		STK6D2X_REG_ALS_PD_REDUCE,
	};
	uint8_t i = 0;
	uint16_t n = sizeof(stk6d2x_reg_map) / sizeof(stk6d2x_reg_map[0]);
	int8_t reg_data;
	int ret = 0;

	for (i = 0; i < n; i++)
	{
		ret = STK6D2X_REG_READ(alps_data, stk6d2x_reg_map[i], &reg_data);

		if (ret < 0)
		{
			ALS_err("fail, ret=%d\n", ret);
			return;
		}
		else
		{
			ALS_info("Reg[0x%2X] = 0x%2X\n", stk6d2x_reg_map[i], (uint8_t)reg_data);
		}
	}
}

static int32_t stk6d2x_check_pid(struct stk6d2x_data *alps_data)
{
	uint8_t value;
	uint16_t i = 0, pid_num = (sizeof(stk6d2x_pid_list) / sizeof(stk6d2x_pid_list[0]));
	int err;
	err = STK6D2X_REG_READ(alps_data, STK6D2X_REG_PDT_ID, &value);

	if (err < 0)
	{
		ALS_err("fail, ret=%d\n", err);
		return err;
	}

	ALS_err("PID = 0x%x\n", value);

	if ((value & 0xF0) == 0x30)
		alps_data->isTrimmed = 1;
	else
		alps_data->isTrimmed = 0;

	for (i = 0; i < pid_num; i++)
	{
		if (value == stk6d2x_pid_list[i])
		{
			return 0;
		}
	}

	return -1;
}

static int32_t stk6d2x_check_rid(struct stk6d2x_data *alps_data)
{
	uint8_t value;
	int err;
	err = STK6D2X_REG_READ(alps_data, STK6D2X_REG_RID, &value);

	if (err < 0)
	{
		ALS_err("fail, ret=%d\n", err);
		return err;
	}

	ALS_err("RID = 0x%x\n", value);
	alps_data->rid = value;
	return 0;
}

static int32_t stk6d2x_software_reset(struct stk6d2x_data *alps_data)
{
	int32_t r;
	r = STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_SW_RESET, 0x0);

	if (r < 0)
	{
		ALS_err("software reset: read error after reset\n");
		return r;
	}

	STK6D2X_TIMER_BUSY_WAIT(alps_data, 13000, 15000, US_RANGE_DELAY);
	return 0;
}

int32_t stk6d2x_fsm_restart(struct stk6d2x_data *alps_data)
{
	int32_t r;
	r = STK6D2X_REG_READ_MODIFY_WRITE(alps_data,
									  STK6D2X_REG_STATE,
									  STK6D2X_STATE_EN_FSM_RESTART_MASK,
									  STK6D2X_STATE_EN_FSM_RESTART_MASK);
	if (r < 0)
	{
		ALS_err("fail\n");
		return r;
	}

	return 0;
}

/****************************************************************************************************
* ALS control API
****************************************************************************************************/
#ifdef STK_ALS_AGC
int32_t stk6d2x_cal_curDGain(uint8_t gain_val)
{
	uint32_t ret_gain = 0;
	if (gain_val > STK6D2X_ALS_DGAIN64)
	{
		ret_gain = stk6d2x_power(2, gain_val + 3);
	}
	else if (gain_val <= STK6D2X_ALS_DGAIN64)
	{
		ret_gain = stk6d2x_power(4, gain_val);
	}
	return ret_gain;
}

uint8_t stk6d2x_als_get_again_multiple(uint8_t gain)
{
	uint8_t value = 0;

	switch (gain)
	{
		case STK6D2X_ALS_CI_2_0:
			value = STK6D2X_ALS_AGAIN_MULTI4;
			break;

		case STK6D2X_ALS_CI_1_0:
			value = STK6D2X_ALS_AGAIN_MULTI2;
			break;

		case STK6D2X_ALS_CI_0_5:
			value = STK6D2X_ALS_AGAIN_MULTI1;
			break;

		default:
			break;
	}

	return value;
}

uint8_t stk6d2x_als_get_pd_multiple(uint8_t gain, uint8_t pd_mode, uint8_t data_type)
{
	uint8_t value = 0;
	if (data_type == STK6D2X_TYPE_ALS0 && (pd_mode == STK6D2X_ALS0_AGC_PDMODE0 || pd_mode == STK6D2X_ALS0_AGC_PDMODE1))
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = STK6D2X_ALS_PD_REDUCE_MULTI4;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = STK6D2X_ALS_PD_REDUCE_MULTI2;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV2:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;

			default:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS1 && pd_mode == STK6D2X_ALS0_AGC_PDMODE0)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = STK6D2X_ALS_PD_REDUCE_MULTI2;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;

			default:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS1 && pd_mode == STK6D2X_ALS0_AGC_PDMODE1)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = STK6D2X_ALS_PD_REDUCE_MULTI3;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = STK6D2X_ALS_PD_REDUCE_MULTI2;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV2:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;

			default:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS2 && pd_mode == STK6D2X_ALS0_AGC_PDMODE0)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = STK6D2X_ALS_PD_REDUCE_MULTI2;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;

			default:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS2 && pd_mode == STK6D2X_ALS0_AGC_PDMODE1)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;

			default:
				value = STK6D2X_ALS_PD_REDUCE_MULTI1;
				break;
		}
	}

	return value;
}

int32_t stk6d2x_get_curGain(struct stk6d2x_data *alps_data)
{
	int32_t ret = 0;
	uint8_t flag_value = 0;
	uint8_t reg_value = 0;
	uint8_t pd_mode = 0;

	ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_ALS_PD_REDUCE, &pd_mode);

	if (ret < 0)
	{
		return ret;
	}
	alps_data->als_info.als_cur_pd_mode = (pd_mode & STK6D2X_ALS0_AGC_PDMODE_MASK) >> STK6D2X_ALS0_AGC_PDMODE_SHIFT;

	// dgain
	ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_AGC1_DG, &reg_value);

	if (ret < 0)
	{
		return ret;
	}

	flag_value = reg_value & (STK6D2X_FLG_ALS2_DG_MASK | STK6D2X_FLG_ALS1_DG_MASK | STK6D2X_FLG_ALS0_DG_MASK);

	alps_data->als_info.als_cur_dgain[2] = stk6d2x_cal_curDGain(reg_value & STK6D2X_ALS2_AGC_DG_MASK);

	ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_AGC2_DG, &reg_value);

	if (ret < 0)
	{
		return ret;
	}

	alps_data->als_info.als_cur_dgain[0] = stk6d2x_cal_curDGain(reg_value & STK6D2X_ALS0_AGC_DG_MASK);
	alps_data->als_info.als_cur_dgain[1] = stk6d2x_cal_curDGain((reg_value & STK6D2X_ALS1_AGC_DG_MASK) >> 4);

	// again
	ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_AGC_AG, &reg_value);

	if (ret < 0)
	{
		return ret;
	}

	alps_data->als_info.als_cur_again[0] = stk6d2x_als_get_again_multiple(reg_value & STK6D2X_ALS0_AGC_AG_MASK);
	alps_data->als_info.als_cur_again[1] = stk6d2x_als_get_again_multiple((reg_value & STK6D2X_ALS1_AGC_AG_MASK) >> 2);
	alps_data->als_info.als_cur_again[2] = stk6d2x_als_get_again_multiple((reg_value & STK6D2X_ALS2_AGC_AG_MASK) >> 4);

	// PD
	ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_AGC_PD, &reg_value);

	if (ret < 0)
	{
		return ret;
	}
	alps_data->als_info.als_cur_pd_reduce[0] = stk6d2x_als_get_pd_multiple(reg_value & STK6D2X_ALS0_AGC_PD_MASK, \
																		   alps_data->als_info.als_cur_pd_mode, \
																		   STK6D2X_TYPE_ALS0);
	alps_data->als_info.als_cur_pd_reduce[1] = stk6d2x_als_get_pd_multiple((reg_value & STK6D2X_ALS1_AGC_PD_MASK) >> 2, \
																		   alps_data->als_info.als_cur_pd_mode, \
																		   STK6D2X_TYPE_ALS1);
	alps_data->als_info.als_cur_pd_reduce[2] = stk6d2x_als_get_pd_multiple((reg_value & STK6D2X_ALS2_AGC_PD_MASK) >> 4, \
																		   alps_data->als_info.als_cur_pd_mode, \
																		   STK6D2X_TYPE_ALS2);

	ALS_info("Current DG Gain = ALS0:%d, ALS1:%d, ALS2:%d, AG Gain multiple = ALS0:%d, ALS1:%d, ALS2:%d\n",
			alps_data->als_info.als_cur_dgain[0],
			alps_data->als_info.als_cur_dgain[1],
			alps_data->als_info.als_cur_dgain[2],
			alps_data->als_info.als_cur_again[0],
			alps_data->als_info.als_cur_again[1],
			alps_data->als_info.als_cur_again[2]);
	ALS_info("Current PD Reduce multiple = ALS0:%d, ALS1:%d, ALS2:%d\n",
			alps_data->als_info.als_cur_pd_reduce[0],
			alps_data->als_info.als_cur_pd_reduce[1],
			alps_data->als_info.als_cur_pd_reduce[2]);
	return 0;
}

void stk6d2x_get_als_ratio(struct stk6d2x_data *alps_data)
{
		alps_data->als_info.als_cur_ratio[0] = ((STK6D2X_ALS_DGAIN_MULTI1024 * STK6D2X_ALS_AGAIN_MULTI4 * STK6D2X_ALS_PD_REDUCE_MULTI4) / \
												((alps_data->als_info.als_cur_dgain[0])  * \
												(alps_data->als_info.als_cur_again[0]) * \
												(alps_data->als_info.als_cur_pd_reduce[0])));


		//ALS1
		if (alps_data->als_info.als_cur_pd_mode == STK6D2X_ALS0_AGC_PDMODE0)
		{
			alps_data->als_info.als_cur_ratio[1] = ((STK6D2X_ALS_DGAIN_MULTI1024 * STK6D2X_ALS_AGAIN_MULTI4 * STK6D2X_ALS_PD_REDUCE_MULTI2) / \
													((alps_data->als_info.als_cur_dgain[1])  * \
													(alps_data->als_info.als_cur_again[1]) * \
													(alps_data->als_info.als_cur_pd_reduce[1])));
		}
		else if (alps_data->als_info.als_cur_pd_mode == STK6D2X_ALS0_AGC_PDMODE1)
		{
			alps_data->als_info.als_cur_ratio[1] = ((STK6D2X_ALS_DGAIN_MULTI1024 * STK6D2X_ALS_AGAIN_MULTI4 * STK6D2X_ALS_PD_REDUCE_MULTI3) / \
													((alps_data->als_info.als_cur_dgain[1])  * \
													(alps_data->als_info.als_cur_again[1]) * \
													(alps_data->als_info.als_cur_pd_reduce[1])));
		}


		//ALS2
		if (alps_data->als_info.als_cur_pd_mode == STK6D2X_ALS0_AGC_PDMODE0)
		{
			alps_data->als_info.als_cur_ratio[2] = ((STK6D2X_ALS_DGAIN_MULTI1024 * STK6D2X_ALS_AGAIN_MULTI4 * STK6D2X_ALS_PD_REDUCE_MULTI2) / \
													((alps_data->als_info.als_cur_dgain[2])  * \
													(alps_data->als_info.als_cur_again[2]) * \
													(alps_data->als_info.als_cur_pd_reduce[2])));
		}
		else if (alps_data->als_info.als_cur_pd_mode == STK6D2X_ALS0_AGC_PDMODE1)
		{
			alps_data->als_info.als_cur_ratio[2] = ((STK6D2X_ALS_DGAIN_MULTI1024 * STK6D2X_ALS_AGAIN_MULTI4 * STK6D2X_ALS_PD_REDUCE_MULTI1) / \
													((alps_data->als_info.als_cur_dgain[2])  * \
													(alps_data->als_info.als_cur_again[2]) * \
													(alps_data->als_info.als_cur_pd_reduce[2])));
		}
		/*ALS_info("Current AGC ratio = ALS0:%d, ALS1:%d, ALS2:%d\n",
				alps_data->als_info.als_cur_ratio[0],
				alps_data->als_info.als_cur_ratio[1],
				alps_data->als_info.als_cur_ratio[2]);*/
}

#ifdef SEC_FFT_FLICKER_1024
uint8_t stk6d2x_sec_dgain(uint8_t gain_val)
{
	uint8_t ret_gain = 0;
	if (gain_val > STK6D2X_ALS_DGAIN64)
	{
		ret_gain = gain_val + 3;
	}
	else if (gain_val <= STK6D2X_ALS_DGAIN64)
	{
		ret_gain = gain_val * 2;
	}
	return ret_gain;
}

uint8_t stk6d2x_sec_again(uint8_t gain)
{
	uint8_t value = 0;

	switch (gain)
	{
		case STK6D2X_ALS_CI_2_0:
			value = 2;
			break;

		case STK6D2X_ALS_CI_1_0:
			value = 1;
			break;

		case STK6D2X_ALS_CI_0_5:
			value = 0;
			break;

		default:
			break;
	}

	return value;
}

uint8_t stk6d2x_sec_pd_multiple(uint8_t gain, uint8_t pd_mode, uint8_t data_type)
{
	uint8_t value = 0;
	if (data_type == STK6D2X_TYPE_ALS0 && (pd_mode == STK6D2X_ALS0_AGC_PDMODE0 || pd_mode == STK6D2X_ALS0_AGC_PDMODE1))
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = 2;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = 1;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV2:
				value = 0;
				break;

			default:
				value = 0;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS1 && pd_mode == STK6D2X_ALS0_AGC_PDMODE0)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = 1;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = 0;
				break;

			default:
				value = 0;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS1 && pd_mode == STK6D2X_ALS0_AGC_PDMODE1)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = 2;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = 1;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV2:
				value = 0;
				break;

			default:
				value = 0;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS2 && pd_mode == STK6D2X_ALS0_AGC_PDMODE0)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = 1;
				break;

			case STK6D2X_ALS_PD_REDUCE_LV1:
				value = 0;
				break;

			default:
				value = 0;
				break;
		}
	}

	if (data_type == STK6D2X_TYPE_ALS2 && pd_mode == STK6D2X_ALS0_AGC_PDMODE1)
	{
		switch (gain)
		{
			case STK6D2X_ALS_PD_REDUCE_DIS:
				value = 0;
				break;

			default:
				value = 0;
				break;
		}
	}

	return value;
}
#endif
#endif

static int32_t stk6d2x_als_latency(stk6d2x_data *alps_data)
{
#ifdef STK_FIFO_ENABLE
	int32_t ret;
	uint8_t reg_value[2] = {0};
	uint8_t ext_it_reg = 0;
	uint32_t  als_it_time = 0;
	if (!alps_data->is_long_it)
	{

		ret = STK6D2X_REG_BLOCK_READ(alps_data, STK6D2X_REG_IT1, sizeof(reg_value), reg_value);

		if (ret < 0)
		{
			ALS_err("fail, ret=%d\n", ret);
		}

		ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_ALS_IT_EXT, &ext_it_reg);

		if (ret < 0)
		{
			return ret;
		}

		als_it_time = ((((reg_value[0] & 0x3f) | reg_value[1]) + 1) * 26660 + ext_it_reg * 833 + 130828); //convert time 130.8281 us
		als_it_time *= STK_FIFO_I2C_READ_FRAME;
		als_it_time /= 1000000;
		alps_data->als_info.als_it = als_it_time * 12 / 10;
	}
	else
#endif
	{
		alps_data->als_info.als_it = 50 * 12 / 10;
	}

	ALS_err("ALS IT = %d\n", alps_data->als_info.als_it);
	return 0;
}

static int32_t stk6d2x_set_als_thd(stk6d2x_data *alps_data, uint16_t thd_h, uint16_t thd_l)
{
	unsigned char val[4];
	int ret = 0;
	val[0] = (thd_h & 0xFF00) >> 8;
	val[1] = thd_h & 0x00FF;
	val[2] = (thd_l & 0xFF00) >> 8;
	val[3] = thd_l & 0x00FF;
	ret = STK6D2X_REG_WRITE_BLOCK(alps_data, STK6D2X_REG_THDH1_ALS, val, sizeof(val));

	if (ret < 0)
	{
		ALS_err("fail, ret=%d\n", ret);
	}

	return ret;
}

int32_t stk6d2x_enable_als(stk6d2x_data *alps_data, bool en)
{
	int32_t ret = 0;
	uint8_t reg_value = 0;

	if (alps_data->als_info.enable == en)
	{
		ALS_err("ALS already set\n");
		return ret;
	}

	ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_STATE, &reg_value);

	if (ret < 0)
	{
		return ret;
	}

	reg_value &= (~(STK6D2X_STATE_EN_WAIT_MASK | STK6D2X_STATE_EN_ALS_MASK));

	if (en)
	{
		reg_value |= STK6D2X_STATE_EN_ALS_MASK;
#ifdef STK_DATA_SUMMATION
		reg_value |= STK6D2X_STATE_EN_SUMMATION_MASK;
#endif
	}
	else
	{
#ifdef STK_ALS_CALI

		if (alps_data->als_info.cali_enable)
		{
			reg_value |= STK6D2X_STATE_EN_ALS_MASK;
		}

#endif
	}

	ret = STK6D2X_REG_READ_MODIFY_WRITE(alps_data,
										STK6D2X_REG_STATE,
										reg_value,
										0xFF);

	if (ret < 0)
	{
		return ret;
	}

	alps_data->als_info.enable = en;
	return ret;
}

#ifdef STK_DATA_SUMMATION
static int32_t stk6d2x_als_get_data_summation(stk6d2x_data *alps_data, uint8_t agc_flag)
{
	uint8_t  raw_data[6];
	int      err = 0;

	err = STK6D2X_REG_BLOCK_READ(alps_data, STK6D2X_REG_DATA1_ALS0_SUM, 6, &raw_data[0]);

	if (err < 0)
	{
		ALS_err("return err\n");
		return err;
	}

	if (!(agc_flag & 0x1))
	{
		alps_data->als_info.last_raw_data[0] = ((*(raw_data) << 8) | *(raw_data + 1));
	}
	if (!(agc_flag & 0x2))
	{
		alps_data->als_info.last_raw_data[1] = ((*(raw_data + 2) << 8) | *(raw_data + 3));
	}
	if (!(agc_flag & 0x4))
	{
		alps_data->als_info.last_raw_data[2] = ((*(raw_data + 4) << 8) | *(raw_data + 5));
	}
	return err;
}

static int32_t stk6d2x_als_get_summation_gain(stk6d2x_data *alps_data)
{
	uint8_t  raw_data[2];
	int err = 0, gain_div_value = 0;
	uint8_t sum_num = 0;

	err = STK6D2X_REG_READ(alps_data, STK6D2X_REG_ALS_SUM, &sum_num);

	if (err < 0)
	{
		return err;
	}

	while (gain_div_value < STK6D2X_ALS_SUM_GAIN_DIV256) {
		if ((1 << gain_div_value) >= (sum_num + 1))
		{
			ALS_err("set summation gain div = %d\n", (1 << gain_div_value));
			break;
		}
		gain_div_value ++;
	}

	err = STK6D2X_REG_READ_MODIFY_WRITE(alps_data,
										STK6D2X_REG_ALS_SUM_GAIN1,
										(gain_div_value << 4) | gain_div_value,
										0xFF);

	if (err < 0)
	{
		return err;
	}

	err = STK6D2X_REG_READ_MODIFY_WRITE(alps_data,
										STK6D2X_REG_ALS_SUM_GAIN2,
										(gain_div_value << 4),
										0xFF);

	if (err < 0)
	{
		return err;
	}

	err = STK6D2X_REG_BLOCK_READ(alps_data, STK6D2X_REG_ALS_SUM_GAIN1, 2, &raw_data[0]);
	if (err < 0)
	{
		ALS_err("return err\n");
		return err;
	}
	alps_data->als_info.als_sum_gain_div[0] = (raw_data[0] & 0xF0) >> 4;
	alps_data->als_info.als_sum_gain_div[1] = (raw_data[0] & 0x0F);
	alps_data->als_info.als_sum_gain_div[2] = (raw_data[0] & 0xF0) >> 4;

	return err;
}
#endif

int32_t stk6d2x_als_get_data(stk6d2x_data *alps_data, bool is_skip)
{
	uint8_t  raw_data[6];
	int      loop_count;
	int      err = 0;
	err = STK6D2X_REG_BLOCK_READ(alps_data, STK6D2X_REG_DATA1_ALS0, 6, &raw_data[0]);

	if (err < 0)
	{
		ALS_err("return err\n");
		return err;
	}

	if (is_skip)
		return err;

	for (loop_count = 0; loop_count < 3; loop_count++)
	{
		*(alps_data->als_info.last_raw_data + loop_count ) = (*(raw_data + (2 * loop_count)) << 8 | *(raw_data + (2 * loop_count + 1) ));
	}
	return err;
}

void stk6d2x_get_data_polling(stk_timer_info *t_info)
{
	stk6d2x_data *alps_data = (stk6d2x_data*)t_info->any;
	uint32_t als_data[3];
	int32_t ret = 0;
	uint8_t flag_value;
#ifdef STK_FIFO_ENABLE
	uint8_t clk_status;
#endif

	if (!alps_data->als_info.enable)
	{
		return;
	}

	if (!alps_data->is_long_it)
	{
#ifdef STK_FIFO_ENABLE
		stk6d2x_get_fifo_data_polling(alps_data);
		// Check using external clk or not
		if (alps_data->is_long_it) {
			stk6d2x_fifo_switch_it(alps_data, 1);
			return;
		}

		if (alps_data->is_local_avg_update && alps_data->fifo_info.fft_buf_idx != 0)
			stk_sec_report(alps_data);	//Stop reporting if it uses internal clk

#ifdef STK_FIFO_DATA_SUMMATION
		alps_data->als_info.last_raw_data[0] = alps_data->fifo_info.fifo_sum_als0;
		alps_data->als_info.last_raw_data[1] = alps_data->fifo_info.fifo_sum_als1;
		alps_data->als_info.last_raw_data[2] = alps_data->fifo_info.fifo_sum_als2;
		alps_data->als_info.is_data_ready = true;
#endif
#ifdef STK_DATA_SUMMATION
		ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_DATA_AGC_SUM, &alps_data->als_info.als_agc_sum_flag);

		if (ret < 0)
		{
			return;
		}
		stk6d2x_als_get_data_summation(alps_data, alps_data->als_info.als_agc_sum_flag);
		stk6d2x_get_curGain(alps_data);
		stk6d2x_get_als_ratio(alps_data);
		if (!(alps_data->als_info.als_agc_sum_flag & 0x1))
		{
			alps_data->als_info.last_raw_data[0] = alps_data->als_info.last_raw_data[0] * alps_data->als_info.als_cur_ratio[0] * (1 << alps_data->als_info.als_sum_gain_div[0]);
		}

		if (!(alps_data->als_info.als_agc_sum_flag & 0x2))
		{
			//ALS1
			alps_data->als_info.last_raw_data[1] = alps_data->als_info.last_raw_data[1] * alps_data->als_info.als_cur_ratio[1] * (1 << alps_data->als_info.als_sum_gain_div[1]);
		}

		if (!(alps_data->als_info.als_agc_sum_flag & 0x4))
		{
			//ALS2
			alps_data->als_info.last_raw_data[2] = alps_data->als_info.last_raw_data[2] * alps_data->als_info.als_cur_ratio[2] * (1 << alps_data->als_info.als_sum_gain_div[2]);
		}
#endif
#endif
	}
	else
	{
#ifdef STK_FIFO_ENABLE
		ret = STK6D2X_REG_READ(alps_data, 0x50, &clk_status);

		if (ret < 0)
		{
			ALS_err("i2c failed\n");
			return;
		}

		if ((clk_status >> 4) & 0x1)
		{
			stk6d2x_fifo_switch_it(alps_data, 0);
			return;
		}
#endif
		// ALPS timer without FIFO control
		if (alps_data->als_info.enable)
		{
			ret = STK6D2X_REG_READ(alps_data, STK6D2X_REG_FLAG, &flag_value);

			if (ret < 0)
			{
				ALS_err("i2c failed\n");
				return;
			}

			if (flag_value & STK6D2X_FLG_ALSDR_MASK)
			{
				stk6d2x_als_get_data(alps_data, 0);
				stk6d2x_get_curGain(alps_data);
				stk6d2x_get_als_ratio(alps_data);
				//ALS0
				alps_data->als_info.last_raw_data[0] = alps_data->als_info.last_raw_data[0] * alps_data->als_info.als_cur_ratio[0];
				//ALS1
				alps_data->als_info.last_raw_data[1] = alps_data->als_info.last_raw_data[1] * alps_data->als_info.als_cur_ratio[1];
				//ALS2
				alps_data->als_info.last_raw_data[2] = alps_data->als_info.last_raw_data[2] * alps_data->als_info.als_cur_ratio[2];
				alps_data->als_info.is_data_ready = true;
			}
			else
			{
				ALS_err("ALS data was not ready\n");
			}
		}
	}

	if (alps_data->als_info.is_data_ready)
	{
		alps_data->als_info.is_data_ready = false;
		// Last information
		als_data[0] = (alps_data->als_info.last_raw_data[0]) * alps_data->als_info.scale / 1000;
		als_data[1] = (alps_data->als_info.last_raw_data[1]) * alps_data->als_info.scale / 1000;
		als_data[2] = (alps_data->als_info.last_raw_data[2]) * alps_data->als_info.scale / 1000;
		ALS_info("multiple ratio ALS0 Data = %llu, ALS1 data = %llu, ALS2 data = %llu\n",
			alps_data->als_info.last_raw_data[0],
			alps_data->als_info.last_raw_data[1],
			alps_data->als_info.last_raw_data[2]);
		STK6D2X_ALS_REPORT(alps_data, als_data, 3);
	}
}

#ifdef STK_FIFO_ENABLE
void stk6d2x_fifo_switch_it(struct stk6d2x_data *alps_data, bool is_long_it)
{
	if (!alps_data->als_info.enable) {
		ALS_err("Sensor is not enable return\n");
		return;
	}

	if (is_long_it)
	{
		alps_data->is_long_it = true;
		STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_IT1, (STK6D2X_ALS_IT50 >> 8) & STK6D2X_ALS_IT1_MASK);
		STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_IT2, STK6D2X_ALS_IT50 & 0xFF);
		STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_ALS_IT_EXT, 0x0);
		stk6d2x_enable_fifo(alps_data, false);
		//stk6d2x_fifo_stop_retry(alps_data);
		ALS_err("Switch to long IT\n");
	}
	else
	{
		alps_data->is_long_it = false;
		STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_IT1, (STK_FLICKER_IT >> 8) & STK6D2X_ALS_IT1_MASK);
		STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_IT2, STK_FLICKER_IT & 0xFF);
		STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_ALS_IT_EXT, STK_FLICKER_EXT_IT);
		stk6d2x_enable_fifo(alps_data, true);
		//stk6d2x_alloc_fifo_data(alps_data, STK_FIFO_I2C_READ_FRAME_TARGET);
		ALS_err("Switch to short IT\n");
	}

	stk6d2x_fsm_restart(alps_data);
	stk6d2x_als_get_data(alps_data, 1);
	STK6D2X_TIMER_STOP(alps_data, &alps_data->alps_timer_info);
	stk6d2x_als_latency(alps_data);
	alps_data->alps_timer_info.interval_time = alps_data->als_info.als_it;
	alps_data->alps_timer_info.change_interval_time = true;
	STK6D2X_TIMER_REGISTER(alps_data, &alps_data->alps_timer_info);
	STK6D2X_TIMER_START(alps_data, &alps_data->alps_timer_info);
}
#endif

/****************************************************************************************************
* System Init. API
****************************************************************************************************/
static void stk6d2x_init_para(struct stk6d2x_data *alps_data,
							  struct stk6d2x_platform_data *plat_data)
{
	memset(&alps_data->als_info, 0, sizeof(alps_data->als_info));
	alps_data->als_info.first_init  = true;
#ifdef STK_ALS_CALI

	if (alps_data->cali_info.cali_para.als_version != 0)
	{
		alps_data->als_info.scale = alps_data->cali_info.cali_para.als_scale;
	}
	else
#endif
	{
		alps_data->als_info.scale = plat_data->als_scale;
	}

	alps_data->pdata = plat_data;
	alps_data->als_info.is_data_ready = false;
#ifdef STK_FIFO_ENABLE
	alps_data->is_long_it = false;

	alps_data->index_last = 0;
	alps_data->is_first = true;
	alps_data->is_local_avg_update = false;
	alps_data->clear_local_average = 0;
	alps_data->uv_local_average    = 0;
	alps_data->ir_local_average    = 0;
	alps_data->is_clear_local_sat = false;
	alps_data->is_uv_local_sat    = false;
	alps_data->is_ir_local_sat    = false;
#else
	alps_data->is_long_it = true;
#endif
#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
	alps_data->eol_enabled = false;
#endif
}

static int32_t stk6d2x_init_all_reg(struct stk6d2x_data *alps_data)
{
	int32_t ret = 0;
	uint16_t reg_count = 0, reg_num = sizeof(stk6d2x_default_register_table) / sizeof(stk6d2x_register_table);

	for (reg_count = 0; reg_count < reg_num; reg_count++)
	{
		ret = STK6D2X_REG_READ_MODIFY_WRITE(alps_data,
											stk6d2x_default_register_table[reg_count].address,
											stk6d2x_default_register_table[reg_count].value,
											stk6d2x_default_register_table[reg_count].mask_bit);

		if (ret < 0)
		{
			ALS_err("write i2c error\n");
			return ret;
		}
	}

	return ret;
}

int32_t stk6d2x_init_all_setting(stk6d2x_data *alps_data)
{
	int32_t ret;
	ret = stk6d2x_software_reset(alps_data);

	if (ret < 0)
		return ret;

	ret = stk6d2x_check_pid(alps_data);

	if (ret < 0)
		return ret;

	ret = stk6d2x_check_rid(alps_data);

	if (ret < 0)
		return ret;

	ret = stk6d2x_init_all_reg(alps_data);

	if (ret < 0)
		return ret;

#ifdef STK_FIFO_ENABLE
	stk6d2x_fifo_init(alps_data);
#endif
	alps_data->first_init = true;
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	alps_data->als_flag = false;
	alps_data->flicker_flag = false;
#endif
	stk6d2x_dump_reg(alps_data);
	return 0;
}

void stk6d2x_force_stop(stk6d2x_data *alps_data)
{
	ALS_err("Stop ALPS timer\n");
	STK6D2X_TIMER_STOP(alps_data, &alps_data->alps_timer_info);

	ALS_err("FIFO disable\n");
	stk6d2x_enable_fifo(alps_data, false);
	stk_power_ctrl(alps_data, false);
	if (alps_data->als_info.enable == true)
		alps_data->als_info.enable = false;
}

int32_t stk6d2x_alps_set_config(stk6d2x_data *alps_data, bool en)
{
	int32_t ret = 0;

	mutex_lock(&alps_data->config_lock);
	if (alps_data->first_init)
	{
		ret = stk6d2x_request_registry(alps_data);

		if (ret < 0)
			goto err;

		stk6d2x_init_para(alps_data, alps_data->pdata);
		alps_data->first_init = false;
	}

	if (alps_data->als_info.enable == en)
	{
		ALS_err("ALS status is same (%d)\n", en);
		goto err;
	}

	if (en == 1)
		stk6d2x_init_all_reg(alps_data);

	stk6d2x_pin_control(alps_data, en);

	if (en)
	{
		ALS_err("ALPS Timer SETTING\n");

		if (!alps_data->alps_timer_info.is_exist)
		{
			stk6d2x_als_latency(alps_data);
			ret = stk6d2x_register_timer(alps_data, STK6D2X_DATA_TIMER_ALPS);
			if (ret < 0)
			{
				ALS_err("register timer fail\n");
				mutex_unlock(&alps_data->config_lock);
				return ret;
			}
		}
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
		if (alps_data->flicker_flag)
#endif
		{
			if (!alps_data->alps_timer_info.is_active)
			{
				ret = STK6D2X_TIMER_START(alps_data, &alps_data->alps_timer_info);
				if (ret < 0)
				{
					ALS_err(" start timer fail\n");
					mutex_unlock(&alps_data->config_lock);
					return ret;
				}
			}
		}
	}

#ifdef STK_ALS_ENABLE
	ret = stk6d2x_enable_als(alps_data, en);
	if (ret < 0) {
		stk6d2x_force_stop(alps_data);
		goto err;
	}
#endif

	if (!en &&  !alps_data->als_info.enable)
	{
		ALS_dbg("Stop ALPS timer\n");
        STK6D2X_TIMER_STOP(alps_data, &alps_data->alps_timer_info);
	}

#ifdef STK_FIFO_ENABLE

	if (alps_data->als_info.enable == true)
	{
		ALS_dbg("FIFO enable\n");
#ifdef STK_DATA_SUMMATION
		stk6d2x_als_get_summation_gain(alps_data);
#endif
		stk6d2x_enable_fifo(alps_data, true);
	}
	else
	{
		ALS_dbg("FIFO disable\n");
		stk6d2x_enable_fifo(alps_data, false);
	}
#endif
	// stk6d2x_dump_reg(alps_data);

err:
	mutex_unlock(&alps_data->config_lock);
	return 0;
}

#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
void stk_als_init(struct stk6d2x_data *alps_data)
{
	STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_IT1,
		(STK6D2X_ALS_IT40 >> 8) & STK6D2X_ALS_IT1_MASK);
	STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_IT2, STK6D2X_ALS_IT40 & 0xFF);
	STK6D2X_REG_WRITE(alps_data, STK6D2X_REG_ALS_IT_EXT, 0x0);
	stk6d2x_enable_fifo(alps_data, false);
	stk6d2x_fsm_restart(alps_data);
}

void stk_als_start(struct stk6d2x_data *alps_data)
{
	ALS_dbg("stk_als_start\n");

	if (!alps_data->flicker_flag) {
		stk6d2x_alps_set_config(alps_data, true);
		stk_als_init(alps_data);
	}
}

void stk_als_stop(struct stk6d2x_data *alps_data)
{
	ALS_dbg("%s\n",__func__);

	if (!alps_data->flicker_flag)
		stk6d2x_alps_set_config(alps_data, false);
}
#endif /* CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS */
