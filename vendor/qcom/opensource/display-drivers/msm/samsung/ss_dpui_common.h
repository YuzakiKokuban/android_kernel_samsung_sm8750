/*
 * Header file for Samsung Common LCD Driver.
 *
 * Copyright (c) 2017 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DPUI_H__
#define __DPUI_H__

#define MAX_DPUI_KEY_LEN	(20)
/* Increase buf size for SS LOG.
 * TODO: Instead of fixed length, allocate buf dynamically using kmalloc.
 */
#define MAX_DPUI_VAL_LEN	(700) // (128)

enum {
	DPUI_AUTO_CLEAR_OFF,
	DPUI_AUTO_CLEAR_ON,
	MAX_DPUI_AUTO_CLEAR,
};

enum dpui_log_level {
	DPUI_LOG_LEVEL_DEBUG,
	DPUI_LOG_LEVEL_INFO,
	DPUI_LOG_LEVEL_ALL,
	MAX_DPUI_LOG_LEVEL,
};

enum dpui_type {
	DPUI_TYPE_NONE,
	DPUI_TYPE_PANEL,
	DPUI_TYPE_CTRL,
	MAX_DPUI_TYPE,
};

enum dpui_var_type {
	DPUI_VAR_BOOL,
	DPUI_VAR_S32,
	DPUI_VAR_S64,
	DPUI_VAR_U32,
	DPUI_VAR_U64,
	DPUI_VAR_STR,
	MAX_DPUI_VAR_TYPE,
};

#define DPUI_VALID_VAR_TYPE(_vtype_)	\
	(((_vtype_) >= DPUI_VAR_BOOL) && ((_vtype_) < MAX_DPUI_VAR_TYPE))

enum dpui_key {
	DPUI_KEY_NONE,
	DPUI_KEY_WCRD_X,	/* white color x-coordinate */
	DPUI_KEY_WCRD_Y,	/* white color y-coordinate */
	DPUI_KEY_WOFS_R,	/* mdnie whiteRGB red offset from user and factory */
	DPUI_KEY_WOFS_G,	/* mdnie whiteRGB green offset from user and factory */
	DPUI_KEY_WOFS_B,	/* mdnie whiteRGB blue offset from user and factory */
	DPUI_KEY_WOFS_R_ORG,	/* mdnie whiteRGB red offset from factory */
	DPUI_KEY_WOFS_G_ORG,	/* mdnie whiteRGB green offset from factory */
	DPUI_KEY_WOFS_B_ORG,	/* mdnie whiteRGB blue offset from factory */
	DPUI_KEY_LCDID1,	/* panel id 1 */
	DPUI_KEY_LCDID2,	/* panel id 2 */
	DPUI_KEY_LCDID3,	/* panel id 3 */
	DPUI_KEY_MAID_DATE,	/* panel manufacture date */
	DPUI_KEY_CELLID,	/* panel cell id */
	DPUI_KEY_OCTAID,	/* panel octa id */
	DPUI_KEY_SUB_CELLID,	/* panel cell id */
	DPUI_KEY_SUB_OCTAID,	/* panel octa id */
	DPUI_KEY_PNDSIE,	/* panel dsi error count */
	DPUI_KEY_PNELVDE,	/* panel ELVDD error count */
	DPUI_KEY_PNVLI1E,	/* panel VLIN1 error count */
	DPUI_KEY_PNVLO3E,	/* panel VLOUT3 error count */
	DPUI_KEY_PNSDRE,	/* panel OTP loading error count */
	/* POC */
	DPUI_KEY_PNPOCT,	/* panel POC try count */
	DPUI_KEY_PNPOCF,	/* panel POC fail count */
	DPUI_KEY_PNPOCI,	/* panel POC image index */
	DPUI_KEY_PNPOCI_ORG,	/* panel POC image index in factory */
	DPUI_KEY_PNPOC_ER_TRY,	/* panel POC erase try count */
	DPUI_KEY_PNPOC_ER_FAIL,	/* panel POC erase fail count */
	DPUI_KEY_PNPOC_WR_TRY,	/* panel POC write try count */
	DPUI_KEY_PNPOC_WR_FAIL,	/* panel POC write fail count */
	DPUI_KEY_PNPOC_RD_TRY,	/* panel POC read try count */
	DPUI_KEY_PNPOC_RD_FAIL,	/* panel POC read fail count */
	/* GAMMA FLASH */
	DPUI_KEY_PNGFLS,	/* panel gamma flash loading result */
	/* dependent on processor */
	DPUI_KEY_QCT_DSIE,	/* display controller dsi error count */
	DPUI_KEY_QCT_PPTO,	/* display controller pingpong timeout count */
	DPUI_KEY_QCT_NO_TE,	/* display controller no TE response  count */
	DPUI_KEY_QCT_RCV_CNT,	/* display controller ESD recovery count */
	DPUI_KEY_QCT_SSLOG,	/* display controller ss debugging log */
	DPUI_KEY_QCT_MAIN_RX_FAIL_CNT, /* main display mipi rx fail count */
	DPUI_KEY_QCT_SUB_RX_FAIL_CNT, /* sub display mipi rx fail count */

	/* GPU */
	DPUI_KEY_QCT_GPU_PF,	/* GPU Page Fault Count */

	DPUI_KEY_UB_CON,		/* UB con detect */
	DPUI_KEY_SUB_UB_CON,	/* SUB UB con detect */

	DPUI_KEY_ECC_ERR,	/* ECC Error */
	DPUI_KEY_FLASH_LOAD,	/* Flash loading failure count */

	MAX_DPUI_KEY,
};


#define DPUI_VALID_KEY(_key_)	\
	(((_key_) > DPUI_KEY_NONE) && ((_key_) < MAX_DPUI_KEY))

struct dpui_field {
	enum dpui_type type;
	enum dpui_var_type var_type;
	bool auto_clear;
	bool initialized;
	enum dpui_key key;
	enum dpui_log_level level;
	char *default_value;
	char buf[MAX_DPUI_VAL_LEN];
};

#define DPUI_FIELD_INIT(_level_, _type_, _var_type_, _auto_clr_, _default_val_, _key_)	\
	[(_key_)] = {								\
		.type = (_type_),						\
		.var_type = (_var_type_),				\
		.auto_clear = (_auto_clr_),				\
		.initialized = (false),					\
		.key = (_key_),							\
		.level = (_level_),						\
		.default_value = (_default_val_),		\
		.buf[0] = ('\0'),						\
	}

struct dpui_info {
	void *pdata;
	struct dpui_field field[MAX_DPUI_KEY];
};

static const char * const dpui_key_name[] = {
	[DPUI_KEY_NONE] = "NONE",
	/* common hw parameter */
	[DPUI_KEY_WCRD_X] = "WCRD_X",
	[DPUI_KEY_WCRD_Y] = "WCRD_Y",
	[DPUI_KEY_WOFS_R] = "WOFS_R",
	[DPUI_KEY_WOFS_G] = "WOFS_G",
	[DPUI_KEY_WOFS_B] = "WOFS_B",
	[DPUI_KEY_WOFS_R_ORG] = "WOFS_R_ORG",
	[DPUI_KEY_WOFS_G_ORG] = "WOFS_G_ORG",
	[DPUI_KEY_WOFS_B_ORG] = "WOFS_B_ORG",
	[DPUI_KEY_LCDID1] = "LCDM_ID1",
	[DPUI_KEY_LCDID2] = "LCDM_ID2",
	[DPUI_KEY_LCDID3] = "LCDM_ID3",
	[DPUI_KEY_MAID_DATE] = "MAID_DATE",
	[DPUI_KEY_CELLID] = "CELLID",
	[DPUI_KEY_OCTAID] = "OCTAID",
	[DPUI_KEY_SUB_CELLID] = "SUB_CELLID",
	[DPUI_KEY_SUB_OCTAID] = "SUB_OCTAID",
	[DPUI_KEY_PNDSIE] = "PNDSIE",
	[DPUI_KEY_PNELVDE] = "PNELVDE",
	[DPUI_KEY_PNVLI1E] = "PNVLI1E",
	[DPUI_KEY_PNVLO3E] = "PNVLO3E",
	[DPUI_KEY_PNSDRE] = "PNSDRE",
	[DPUI_KEY_PNPOCT] = "PNPOCT",
	[DPUI_KEY_PNPOCF] = "PNPOCF",
	[DPUI_KEY_PNPOCI] = "PNPOCI",
	[DPUI_KEY_PNPOCI_ORG] = "PNPOCI_ORG",
	[DPUI_KEY_PNPOC_ER_TRY] = "PNPOC_ER_T",
	[DPUI_KEY_PNPOC_ER_FAIL] = "PNPOC_ER_F",
	[DPUI_KEY_PNPOC_WR_TRY] = "PNPOC_WR_T",
	[DPUI_KEY_PNPOC_WR_FAIL] = "PNPOC_WR_F",
	[DPUI_KEY_PNPOC_RD_TRY] = "PNPOC_RD_T",
	[DPUI_KEY_PNPOC_RD_FAIL] = "PNPOC_RD_F",

	[DPUI_KEY_PNGFLS] = "PNGFLS",

	/* dependent on processor */
	[DPUI_KEY_QCT_DSIE] = "QCT_DSIE",
	[DPUI_KEY_QCT_PPTO] = "QCT_PPTO",
	[DPUI_KEY_QCT_NO_TE] = "QCT_NO_TE",
	[DPUI_KEY_QCT_RCV_CNT] = "QCT_RCV_CNT",
	[DPUI_KEY_QCT_SSLOG] = "QCT_SSLOG",
	[DPUI_KEY_QCT_MAIN_RX_FAIL_CNT] = "QCT_MAIN_RX_FAIL_CNT",
	[DPUI_KEY_QCT_SUB_RX_FAIL_CNT] = "QCT_SUB_RX_FAIL_CNT",

	/* GPU */
	[DPUI_KEY_QCT_GPU_PF] = "QCT_GPU_PF",

	/* ub con detect */
	[DPUI_KEY_UB_CON] = "UB_CON",
	[DPUI_KEY_SUB_UB_CON] = "SUB_UB_CON",

	[DPUI_KEY_ECC_ERR] = "ECC_ERR",

	/* Flash loading failure count */
	[DPUI_KEY_FLASH_LOAD] = "FLASH_LOAD",
};

static const char * const dpui_type_name[] = {
	[DPUI_TYPE_NONE] = "NONE",
	/* common hw parameter */
	[DPUI_TYPE_PANEL] = "PANEL",
	/* dependent on processor */
	[DPUI_TYPE_CTRL] = "CTRL",
};

static struct dpui_info dpui = {
	.pdata = NULL,
	.field = {
		/* common hw parameter */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WCRD_X),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WCRD_Y),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_R),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_G),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_B),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_R_ORG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_G_ORG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_B_ORG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_LCDID1),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_LCDID2),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_LCDID3),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "19000000 000000", DPUI_KEY_MAID_DATE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "0000000000000000000000", DPUI_KEY_CELLID),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "00000000000000000000000", DPUI_KEY_OCTAID),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "0000000000000000000000", DPUI_KEY_SUB_CELLID),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "00000000000000000000000", DPUI_KEY_SUB_OCTAID),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNDSIE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNELVDE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNVLI1E),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNVLO3E),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNSDRE),

		/* POC */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_PNPOCT),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_PNPOCF),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-100", DPUI_KEY_PNPOCI),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-100", DPUI_KEY_PNPOCI_ORG),

		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_ER_TRY),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_ER_FAIL),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_WR_TRY),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_WR_FAIL),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_RD_TRY),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_RD_FAIL),

		/* Gamma Flash */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_PNGFLS),

		/* dependent on processor */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_QCT_DSIE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_QCT_PPTO),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_QCT_NO_TE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_QCT_RCV_CNT),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_QCT_SSLOG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_QCT_MAIN_RX_FAIL_CNT),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_QCT_SUB_RX_FAIL_CNT),

		/* GPU */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_QCT_GPU_PF),

		/* UB CON */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_UB_CON),

		/* SUB UB CON */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_SUB_UB_CON),

		/* ECC Error */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_ECC_ERR),

		/* Flash loading failure count */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL,
			DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_FLASH_LOAD),
	},
};
int dpui_logging_register(struct notifier_block *n, enum dpui_type type);
int dpui_logging_unregister(struct notifier_block *n);
void update_dpui_log(enum dpui_log_level level, enum dpui_type type);
int clear_dpui_log(enum dpui_log_level level, enum dpui_type type);
int get_dpui_log(char *buf, enum dpui_log_level level, enum dpui_type type);
int set_dpui_field(enum dpui_key key, char *buf, int size);
int get_dpui_field(enum dpui_key key, char *buf);
int set_dpui_u32_field(enum dpui_key key, u32 value);
int get_dpui_u32_field(enum dpui_key key, u32 *value);
int inc_dpui_u32_field(enum dpui_key key, u32 value);
int inc_dpui_u32_field_nolock(enum dpui_key key, u32 value);
#endif /* __DPUI_H__ */
