/*
 * =================================================================
 *
 *	Description:  samsung display debug common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 *
 *
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015, Samsung Electronics. All rights reserved.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _SAMSUNG_DSI_PANEL_DEBUG_H_
#define _SAMSUNG_DSI_PANEL_DEBUG_H_

#if IS_ENABLED(CONFIG_SEC_DEBUG)
#include <linux/sec_debug.h>
#endif

struct samsung_display_driver_data;

#define SS_XLOG_ENTRY 256
#define SS_XLOG_BUF_MAX 128
#define SS_XLOG_MAX_DATA 7
#define SS_XLOG_BUF_ALIGN_TIME 14
#define SS_XLOG_BUF_ALIGN_NAME 32
#define SS_XLOG_START 0x1111
#define SS_XLOG_FINISH 0xFFFF
#define SS_XLOG_PANIC_DBG_LENGTH 256
#define SS_XLOG_DPCI_LENGTH (700 - 1)
#define DATA_LIMITER (-1)

#define DEBUG_DISPLAY_CMD_SIZE 2

enum mdss_samsung_xlog_flag {
	SS_XLOG_DEFAULT,
	SS_XLOG_BIGDATA,
	SS_XLOG_MAX
};

struct ss_tlog {
	int pid;
	s64 time;
	u32 data[SS_XLOG_MAX_DATA];
	u32 data_cnt;
	const char *name;
};

#define SS_RLOG_ENTRY 30

struct ss_rlog_data {
	s64 time;
	u32 data[SS_XLOG_MAX_DATA];
	int type;
	int ndx;
	int retry_cnt;
	int recovery_done;
};

/* PANEL DEBUG FUNCTION */
void ss_xlog(const char *name, int flag, ...);
void ss_dump_xlog(void);
void ss_store_xlog_panic_dbg(void);
int ss_panel_debug_init(struct samsung_display_driver_data *vdd);

#define SS_XLOG(...) ss_xlog(__func__, SS_XLOG_DEFAULT, \
		##__VA_ARGS__, DATA_LIMITER)
#define SS_XLOG_BG(...) ss_xlog(__func__, SS_XLOG_BIGDATA, \
		##__VA_ARGS__, DATA_LIMITER)

void ss_rlog(int ndx, int type, int retry_cnt, int recovery_done);

enum ss_smmu_type {
	SMMU_RT_DISPLAY_DEBUG,
	SMMU_NRT_ROTATOR_DEBUG,
	SMMU_MAX_DEBUG,
};

struct ss_smmu_logging {
	ktime_t time;

	struct sg_table *table; /*To compare with whole ion buffer(page_link) */

	struct list_head list;
};

struct ss_smmu_debug {
	int init_done;

	struct list_head list;
	spinlock_t lock;
};

struct ss_image_logging {
	uint32_t	dma_address;
	int src_width, src_height;
	int src_format;
};

void ss_display_panic(struct samsung_display_driver_data *vdd, const char *panic_msg);

int ss_check_rddpm(struct samsung_display_driver_data *vdd, u8 *rddpm);
int ss_check_rddsm(struct samsung_display_driver_data *vdd, u8 *rddsm);
int ss_check_esderr(struct samsung_display_driver_data *vdd, u16 *esderr);
int ss_check_dsierr(struct samsung_display_driver_data *vdd, u8 *dsierr_cnt);
int ss_check_mipi_protocol_err(struct samsung_display_driver_data *vdd, u16 *protocol_err);
int ss_read_self_diag(struct samsung_display_driver_data *vdd);
int ss_check_ecc(struct samsung_display_driver_data *vdd,
		u8 *enable, u8 *cnt_restore, u8 *cnt_fail);
int ss_read_ddi_debug_reg(struct samsung_display_driver_data *vdd);

int ss_read_ddi_cmd_log(struct samsung_display_driver_data *vdd, char *read_buf);
int ss_read_pps_data(struct samsung_display_driver_data *vdd);

int ss_smmu_debug_init(struct samsung_display_driver_data *vdd);
void ss_smmu_debug_map(enum ss_smmu_type type, struct sg_table *table);
void ss_smmu_debug_unmap(enum ss_smmu_type type, struct sg_table *table);
void ss_smmu_debug_log(void);
void ss_image_logging_update(uint32_t plane_addr, int width, int height, int src_format);

void ss_inc_ftout_debug(const char *name);
void ss_check_te(struct samsung_display_driver_data *vdd);

bool ss_is_panel_dead(int ndx);

int ss_dct_update_ref(u32 ndx, u32 tag, u32 enable);
void ss_dct_update_ref_for_dss(struct clk *clk, u32 enable);
bool ss_dct_is_clk_on(struct samsung_display_driver_data *vdd);
int ss_dct_dump_all_info(struct samsung_display_driver_data *vdd);
int ss_dct_update_clk(u32 ndx, u32 tag, struct clk *clk);
void ss_dct_update_clk_dss(struct clk *clk);

#endif
