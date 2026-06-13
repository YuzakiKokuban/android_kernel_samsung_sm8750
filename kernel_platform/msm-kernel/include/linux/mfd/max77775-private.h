/*
 * max77775-private.h - Voltage regulator driver for the Maxim 77775
 *
 *  Copyright (C) 2016 Samsung Electrnoics
 *  Insun Choi <insun77.choi@samsung.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LINUX_MFD_MAX77775_PRIV_H
#define __LINUX_MFD_MAX77775_PRIV_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/usb/typec/maxim/max77775.h>
#include <linux/mfd/max77775_log.h>

/* To Do : Customer Definition for product id */
#define MAX77775_USBC_PRODUCT_ID	(0x75)

#define MAX77775_REG_INVALID		(0xff)

#define MAX77775_IRQSRC_CHG		(1 << 0)
#define MAX77775_IRQSRC_TOP		(1 << 1)
#define MAX77775_IRQSRC_FG              (1 << 2)
#define MAX77775_IRQSRC_USBC		(1 << 3)
#define MAX77775_ELRN			(1 << 0)
#define MAX77775_FILT_EMPTY		(1 << 2)

enum max77775_hw_rev {
	MAX77775_PASS1 = 0x1,
	MAX77775_PASS2 = 0x2,
	MAX77775_PASS3 = 0x3,
	MAX77775_PASS4 = 0x4,
	MAX77775_PASS5 = 0x5,
};

enum max77775_reg {
	/* Slave addr = 0xCC */
	/* PMIC Top-Level Registers */
	MAX77775_PMIC_REG_PMICID		= 0x00,
	MAX77775_PMIC_REG_PMICREV		= 0x01,
	MAX77775_PMIC_REG_MAINCTRL1		= 0x02,
	MAX77775_PMIC_REG_INTSRC		= 0x22,
	MAX77775_PMIC_REG_INTSRC_MASK		= 0x23,
	MAX77775_PMIC_REG_SYSTEM_INT		= 0x24,
	MAX77775_PMIC_REG_SYSTEM_INT_MASK	= 0x26,
	MAX77775_PMIC_REG_SW_RESET		= 0x50,

	MAX77775_CHG_REG_INT			= 0xB0,
	MAX77775_CHG_REG_INT_MASK		= 0xB1,
	MAX77775_CHG_REG_INT_OK			= 0xB2,
	MAX77775_CHG_REG_DETAILS_00		= 0xB3,
	MAX77775_CHG_REG_DETAILS_01		= 0xB4,
	MAX77775_CHG_REG_DETAILS_02		= 0xB5,
	MAX77775_CHG_REG_CNFG_00		= 0xB7,
	MAX77775_CHG_REG_CNFG_01		= 0xB8,
	MAX77775_CHG_REG_CNFG_02		= 0xB9,
	MAX77775_CHG_REG_CNFG_03		= 0xBA,
	MAX77775_CHG_REG_CNFG_04		= 0xBB,
	MAX77775_CHG_REG_CNFG_05		= 0xBC,
	MAX77775_CHG_REG_CNFG_06		= 0xBD,
	MAX77775_CHG_REG_CNFG_07		= 0xBE,
	MAX77775_CHG_REG_CNFG_08		= 0xBF,
	MAX77775_CHG_REG_CNFG_09		= 0xC0,
	MAX77775_CHG_REG_CNFG_10		= 0xC1,
	MAX77775_CHG_REG_CNFG_11		= 0xC2,
	MAX77775_CHG_REG_CNFG_12		= 0xC3,
	MAX77775_CHG_REG_CNFG_13		= 0xC4,
	MAX77775_CHG_REG_CNFG_14		= 0xC5,
	MAX77775_CHG_REG_CNFG_16		= 0xC7,
	MAX77775_CHG_REG_CNFG_17		= 0xC8,

	MAX77775_PMIC_REG_END,
};

/* Slave addr = 0x6C : Fuelgauge */
enum max77775_fuelgauge_reg {
	MAX77775_FG_REG_STATUS		= 0x00,
	MAX77775_FG_REG_VALRTTH		= 0x01,
	MAX77775_FG_REG_TALRTTH		= 0x02,
	MAX77775_FG_REG_SALRTTH		= 0x03,
	MAX77775_FG_REG_REPCAP		= 0x05,
	MAX77775_FG_REG_REPSOC		= 0x06,
	MAX77775_FG_REG_TEMP		= 0x08,
	MAX77775_FG_REG_VCELL		= 0x09,
	MAX77775_FG_REG_CURRENT		= 0x0A,
	MAX77775_FG_REG_AVGCURRENT	= 0x0B,
	MAX77775_FG_REG_AVSOC		= 0x0E,
	MAX77775_FG_REG_MIXCAP		= 0x0F,
	MAX77775_FG_REG_FULLCAP		= 0x10,
	MAX77775_FG_QRTABLE00		= 0x12,
	MAX77775_FG_REG_FULLSOCTHR	= 0x13,
	MAX77775_FG_REG_CYCLES		= 0x17,
	MAX77775_FG_REG_DESIGNCAP	= 0x18,
	MAX77775_FG_REG_AVGVCELL	= 0x19,
	MAX77775_FG_REG_CONFIG		= 0x1D,
	MAX77775_FG_REG_ICHGTERM	= 0x1E,
	MAX77775_FG_REG_REMCAPAV	= 0x1F,
	MAX77775_FG_QRTABLE10		= 0x22,
	MAX77775_FG_REG_FULLCAPNOM	= 0x23,
	MAX77775_FG_REG_LEARNCFG	= 0x28,
	MAX77775_FG_REG_FILTERCFG	= 0x29,
	MAX77775_FG_REG_MISCCFG		= 0x2B,
	MAX77775_FG_REG_CGAIN		= 0x2E,
	MAX77775_FG_REG_COFF		= 0x2F,
	MAX77775_FG_QRTABLE20		= 0x32,
	MAX77775_FG_REG_FULLCAPREP	= 0x35,
	MAX77775_FG_REG_RCOMP0		= 0x38,
	MAX77775_FG_REG_TEMPCO		= 0x39,
	MAX77775_FG_REG_VEMPTY		= 0x3A,
	MAX77775_FG_QRTABLE30		= 0x42,
	MAX77775_FG_REG_ISYS		= 0x43,
	MAX77775_FG_REG_DQACC		= 0x45,
	MAX77775_FG_REG_DPACC		= 0x46,
	MAX77775_FG_REG_AVGISYS		= 0x4B,
	MAX77775_FG_REG_QH			= 0x4D,
	MAX77775_FG_REG_VSYS		= 0xB1,
	MAX77775_FG_REG_TALRTTH2	= 0xB2,
	/* "not used REG(0xB2)" is for checking fuelgague init result. */
	MAX77775_FG_INIT_RESULT_REG	= MAX77775_FG_REG_TALRTTH2,
	MAX77775_FG_REG_VBYP		= 0xB3,
	MAX77775_FG_REG_CONFIG2		= 0xBB,
	MAX77775_FG_REG_IIN			= 0xD0,
	MAX77775_FG_REG_VFOCV		= 0xFB,
	MAX77775_FG_REG_VFSOC		= 0xFF,

	MAX77775_FG_REG_END
};

#define MAX77775_REG_MAINCTRL1_BIASEN		(1 << 7)

/* Slave addr = 0x4A: USBC */
enum max77775_usbc_reg {
	MAX77775_USBC_REG_PRODUCT_ID		= 0x10, /* replaced address */
	MAX77775_USBC_REG_UIC_FW_REV		= 0x01,
	MAX77775_USBC_REG_UIC_INT			= 0x02,
	MAX77775_USBC_REG_CC_INT			= 0x03,
	MAX77775_USBC_REG_PD_INT			= 0x04,
	MAX77775_USBC_REG_VDM_INT			= 0x05,
	MAX77775_USBC_REG_SPARE_INT			= 0x06,
	MAX77775_USBC_REG_USBC_STATUS1		= 0x08,
	MAX77775_USBC_REG_USBC_STATUS2		= 0x09,
	MAX77775_USBC_REG_BC_STATUS			= 0x0A,
	MAX77775_USBC_REG_UIC_FW_REV2		= 0x0B,
	MAX77775_USBC_REG_CC_STATUS1		= 0x0C,
	MAX77775_USBC_REG_CC_STATUS2		= 0x0D,
	MAX77775_USBC_REG_PD_STATUS1		= 0x0E,
	MAX77775_USBC_REG_PD_STATUS2		= 0x0F,
	MAX77775_USBC_REG_SPARE_STATUS1		= 0x11,
	MAX77775_USBC_REG_UIC_INT_M			= 0x12,
	MAX77775_USBC_REG_CC_INT_M			= 0x13,
	MAX77775_USBC_REG_PD_INT_M			= 0x14,
	MAX77775_USBC_REG_VDM_INT_M			= 0x15,
	MAX77775_USBC_REG_SPARE_INT_M		= 0x16,
	MAX77775_USBC_REG_AP_DATAOUT_M1		= 0x20,
	MAX77775_USBC_REG_AP_DATAOUT0		= 0x21,
	MAX77775_USBC_REG_AP_DATAOUT1		= 0x22,
	MAX77775_USBC_REG_AP_DATAOUT2		= 0x23,
	MAX77775_USBC_REG_AP_DATAOUT3		= 0x24,
	MAX77775_USBC_REG_AP_DATAOUT4		= 0x25,
	MAX77775_USBC_REG_AP_DATAOUT5		= 0x26,
	MAX77775_USBC_REG_AP_DATAOUT6		= 0x27,
	MAX77775_USBC_REG_AP_DATAOUT7		= 0x28,
	MAX77775_USBC_REG_AP_DATAOUT8		= 0x29,
	MAX77775_USBC_REG_AP_DATAOUT9		= 0x2a,
	MAX77775_USBC_REG_AP_DATAOUT10		= 0x2b,
	MAX77775_USBC_REG_AP_DATAOUT11		= 0x2c,
	MAX77775_USBC_REG_AP_DATAOUT12		= 0x2d,
	MAX77775_USBC_REG_AP_DATAOUT13		= 0x2e,
	MAX77775_USBC_REG_AP_DATAOUT14		= 0x2f,
	MAX77775_USBC_REG_AP_DATAOUT15		= 0x30,
	MAX77775_USBC_REG_AP_DATAOUT16		= 0x31,
	MAX77775_USBC_REG_AP_DATAOUT17		= 0x32,
	MAX77775_USBC_REG_AP_DATAOUT18		= 0x33,
	MAX77775_USBC_REG_AP_DATAOUT19		= 0x34,
	MAX77775_USBC_REG_AP_DATAOUT20		= 0x35,
	MAX77775_USBC_REG_AP_DATAOUT21		= 0x36,
	MAX77775_USBC_REG_AP_DATAOUT22		= 0x37,
	MAX77775_USBC_REG_AP_DATAOUT23		= 0x38,
	MAX77775_USBC_REG_AP_DATAOUT24		= 0x39,
	MAX77775_USBC_REG_AP_DATAOUT25		= 0x3a,
	MAX77775_USBC_REG_AP_DATAOUT26		= 0x3b,
	MAX77775_USBC_REG_AP_DATAOUT27		= 0x3c,
	MAX77775_USBC_REG_AP_DATAOUT28		= 0x3d,
	MAX77775_USBC_REG_AP_DATAOUT29		= 0x3e,
	MAX77775_USBC_REG_AP_DATAOUT30		= 0x3f,
	MAX77775_USBC_REG_AP_DATAOUT31		= 0x40,
	MAX77775_USBC_REG_AP_DATAOUT32		= 0x41,
	MAX77775_USBC_REG_AP_DATAIN_M1		= 0x50,
	MAX77775_USBC_REG_AP_DATAIN0		= 0x51,
	MAX77775_USBC_REG_AP_DATAIN1		= 0x52,
	MAX77775_USBC_REG_AP_DATAIN2		= 0x53,
	MAX77775_USBC_REG_AP_DATAIN3		= 0x54,
	MAX77775_USBC_REG_AP_DATAIN4		= 0x55,
	MAX77775_USBC_REG_AP_DATAIN5		= 0x56,
	MAX77775_USBC_REG_AP_DATAIN6		= 0x57,
	MAX77775_USBC_REG_AP_DATAIN7		= 0x58,
	MAX77775_USBC_REG_AP_DATAIN8		= 0x59,
	MAX77775_USBC_REG_AP_DATAIN9		= 0x5a,
	MAX77775_USBC_REG_AP_DATAIN10		= 0x5b,
	MAX77775_USBC_REG_AP_DATAIN11		= 0x5c,
	MAX77775_USBC_REG_AP_DATAIN12		= 0x5d,
	MAX77775_USBC_REG_AP_DATAIN13		= 0x5e,
	MAX77775_USBC_REG_AP_DATAIN14		= 0x5f,
	MAX77775_USBC_REG_AP_DATAIN15		= 0x60,
	MAX77775_USBC_REG_AP_DATAIN16		= 0x61,
	MAX77775_USBC_REG_AP_DATAIN17		= 0x62,
	MAX77775_USBC_REG_AP_DATAIN18		= 0x63,
	MAX77775_USBC_REG_AP_DATAIN19		= 0x64,
	MAX77775_USBC_REG_AP_DATAIN20		= 0x65,
	MAX77775_USBC_REG_AP_DATAIN21		= 0x66,
	MAX77775_USBC_REG_AP_DATAIN22		= 0x67,
	MAX77775_USBC_REG_AP_DATAIN23		= 0x68,
	MAX77775_USBC_REG_AP_DATAIN24		= 0x69,
	MAX77775_USBC_REG_AP_DATAIN25		= 0x6a,
	MAX77775_USBC_REG_AP_DATAIN26		= 0x6b,
	MAX77775_USBC_REG_AP_DATAIN27		= 0x6c,
	MAX77775_USBC_REG_AP_DATAIN28		= 0x6d,
	MAX77775_USBC_REG_AP_DATAIN29		= 0x6e,
	MAX77775_USBC_REG_AP_DATAIN30		= 0x6f,
	MAX77775_USBC_REG_AP_DATAIN31		= 0x70,
	MAX77775_USBC_REG_AP_DATAIN32		= 0x71,
	MAX77775_USBC_REG_UIC_SWRST			= 0x80,

	MAX77775_USBC_REG_END,
};

enum max77775_irq_source {
	SYS_INT = 0,
	CHG_INT,
	FUEL_INT,
	USBC_INT,
	CC_INT,
	PD_INT,
	VDM_INT,
	SPARE_INT,
	VIR_INT,
	MAX77775_IRQ_GROUP_NR,
};

enum max77775_irq {
	/* PMIC; TOPSYS */
	MAX77775_SYSTEM_IRQ_SYSUVLO_INT,
	MAX77775_SYSTEM_IRQ_SYSOVLO_INT,
	MAX77775_SYSTEM_IRQ_TSHDN_INT,
	MAX77775_SYSTEM_IRQ_SCP_INT,

	/* PMIC; Charger */
	MAX77775_CHG_IRQ_BYP_I,
	MAX77775_CHG_IRQ_BATP_I,
	MAX77775_CHG_IRQ_BAT_I,
	MAX77775_CHG_IRQ_CHG_I,
	MAX77775_CHG_IRQ_WCIN_I,
	MAX77775_CHG_IRQ_CHGIN_I,
	MAX77775_CHG_IRQ_AICL_I,

	/* Fuelgauge */
	MAX77775_FG_IRQ_ALERT,

	/* USBC */
	MAX77775_USBC_IRQ_APC_INT,

	/* CC */
	MAX77775_CC_IRQ_VCONNCOP_INT,
	MAX77775_CC_IRQ_VSAFE0V_INT,
	MAX77775_CC_IRQ_DETABRT_INT,
	MAX77775_CC_IRQ_VCONNSC_INT,
	MAX77775_CC_IRQ_CCPINSTAT_INT,
	MAX77775_CC_IRQ_CCISTAT_INT,
	MAX77775_CC_IRQ_CCVCNSTAT_INT,
	MAX77775_CC_IRQ_CCSTAT_INT,

	MAX77775_USBC_IRQ_VBUS_INT,
	MAX77775_USBC_IRQ_VBADC_INT,
	MAX77775_USBC_IRQ_DCD_INT,
	MAX77775_USBC_IRQ_STOPMODE_INT,
	MAX77775_USBC_IRQ_CHGT_INT,
	MAX77775_USBC_IRQ_UIDADC_INT,

	/*
	 * USBC: SYSMSG INT should be after CC INT
	 * because of 2 times of CC Sync INT at WDT reset
	 */
	MAX77775_USBC_IRQ_SYSM_INT,

	/* PD */
	MAX77775_PD_IRQ_PDMSG_INT,
	MAX77775_PD_IRQ_PS_RDY_INT,
	MAX77775_PD_IRQ_DATAROLE_INT,
	MAX77775_PD_IRQ_SSACCI_INT,
	MAX77775_PD_IRQ_FCTIDI_INT,


	/* VDM */
	MAX77775_IRQ_VDM_DISCOVER_ID_INT,
	MAX77775_IRQ_VDM_DISCOVER_SVIDS_INT,
	MAX77775_IRQ_VDM_DISCOVER_MODES_INT,
	MAX77775_IRQ_VDM_ENTER_MODE_INT,
	MAX77775_IRQ_VDM_DP_STATUS_UPDATE_INT,
	MAX77775_IRQ_VDM_DP_CONFIGURE_INT,
	MAX77775_IRQ_VDM_ATTENTION_INT,

	/* SPARE */
	MAX77775_IRQ_USBID_INT,
	MAX77775_IRQ_TACONN_INT,

	/* VIRTUAL */
	MAX77775_VIR_IRQ_ALTERROR_INT,

	MAX77775_IRQ_NR,
};

struct max77775_dev {
	struct device *dev;
	struct i2c_client *i2c; /* 0xCC; PMIC */
	struct i2c_client *charger; /* 0xD2; Charger */
	struct i2c_client *fuelgauge; /* 0x6C; Fuelgauge */
	struct i2c_client *muic; /* 0x4A; MUIC */
	struct i2c_client *testsid; /* 0xC4; TestSID */
	struct mutex i2c_lock;
	struct wakeup_source ws;

	int type;

	int irq;
	int irq_base;
	int irq_gpio;
	bool blocking_waterevent;
	u32 required_hw_rev;
	u32 required_fw_pid;
	struct mutex irqlock;
	int irq_masks_cur[MAX77775_IRQ_GROUP_NR];
	int irq_masks_cache[MAX77775_IRQ_GROUP_NR];

	u8 FW_Revision;
	u8 FW_Minor_Revision;
	u8 FW_Product_ID;
	u8 FW_Revision_bin;
	u8 FW_Minor_Revision_bin;
	u8 FW_Product_ID_bin;

	struct work_struct fw_work;
	struct workqueue_struct *fw_workqueue;
	struct completion fw_completion;
	int fw_update_state;
	int fw_size;

	/* For hibernation */
	u8 reg_pmic_dump[MAX77775_PMIC_REG_END];
	u8 reg_muic_dump[MAX77775_USBC_REG_END];

	u8 pmic_id;	/* pmic id. 0x75 */
	u8 pmic_rev;	/* pmic Rev */

	u8 cc_booting_complete;

	int set_altmode_en;
	int enable_nested_irq;
	u8 usbc_irq;

	bool shutdown;
	bool suspended;
	wait_queue_head_t suspend_wait;

	void (*check_pdmsg)(void *data, u8 pdmsg);
	void *usbc_data;

	struct max77775_platform_data *pdata;
};

enum max77775_types {
	TYPE_MAX77775,
};

extern int max77775_irq_init(struct max77775_dev *max77775);
extern void max77775_irq_exit(struct max77775_dev *max77775);

/* MAX77775 shared i2c API function */
extern int max77775_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int max77775_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int max77775_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int max77775_write_reg_nolock(struct i2c_client *i2c, u8 reg, u8 value);
extern int max77775_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int max77775_write_word(struct i2c_client *i2c, u8 reg, u16 value);
extern int max77775_read_word(struct i2c_client *i2c, u8 reg);

extern int max77775_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

/* MAX77775 check muic path function */
extern bool is_muic_usb_path_ap_usb(void);
extern bool is_muic_usb_path_cp_usb(void);

/* for charger api */
extern void max77775_hv_muic_charger_init(void);
extern int max77775_usbc_fw_update(struct max77775_dev *max77775, const u8 *fw_bin, int fw_bin_len, int enforce_do);
extern int max77775_usbc_fw_setting(struct max77775_dev *max77775, int enforce_do);
extern void max77775_register_pdmsg_func(struct max77775_dev *max77775,
	void (*check_pdmsg)(void *, u8), void *data);

#define MAX77775_DEBUG_ENABLED
#ifdef MAX77775_DEBUG_ENABLED
#define msg_maxim(format, args...) \
		md75_info_usb("max77775: %s: " format "\n", __func__, ## args)
#define err_maxim(format, args...) \
		md75_err_usb("max77775: %s: " format "\n", __func__, ## args)
#else
#define msg_maxim(format, args...)
#define err_maxim(format, args...)
#endif // MAX77775_DEBUG_ENABLED

#endif /* __LINUX_MFD_MAX77775_PRIV_H */

