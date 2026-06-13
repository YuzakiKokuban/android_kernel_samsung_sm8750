/*
 * max77775-irq.c - Interrupt controller support for MAX77775
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Insun Choi <insun77.choi@samsung.com>
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
 *
 * This driver is based on max77775-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/max77775_log.h>
#include <linux/mfd/max77775.h>
#include <linux/mfd/max77775-private.h>

static const u8 max77775_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[SYS_INT] = MAX77775_PMIC_REG_SYSTEM_INT_MASK,
	[CHG_INT] = MAX77775_CHG_REG_INT_MASK,
	[FUEL_INT] = MAX77775_REG_INVALID,
	[USBC_INT] = MAX77775_USBC_REG_UIC_INT_M,
	[CC_INT] = MAX77775_USBC_REG_CC_INT_M,
	[PD_INT] = MAX77775_USBC_REG_PD_INT_M,
	[VDM_INT] = MAX77775_USBC_REG_VDM_INT_M,
	[SPARE_INT] = MAX77775_USBC_REG_SPARE_INT_M,
	[VIR_INT] = MAX77775_REG_INVALID,
};

static struct i2c_client *get_i2c(struct max77775_dev *max77775,
				enum max77775_irq_source src)
{
	switch (src) {
	case SYS_INT:
		return max77775->i2c;
	case FUEL_INT:
		return max77775->fuelgauge;
	case CHG_INT:
		return max77775->charger;
	case USBC_INT:
	case CC_INT:
	case PD_INT:
	case VDM_INT:
	case SPARE_INT:
	case VIR_INT:
		return max77775->muic;
	default:
		return ERR_PTR(-EINVAL);
	}
}

struct max77775_irq_data {
	int mask;
	enum max77775_irq_source group;
};

static const struct max77775_irq_data max77775_irqs[] = {
	[MAX77775_SYSTEM_IRQ_SYSUVLO_INT] = { .group = SYS_INT, .mask = 1 << 4 },
	[MAX77775_SYSTEM_IRQ_SYSOVLO_INT] = { .group = SYS_INT, .mask = 1 << 5 },
	[MAX77775_SYSTEM_IRQ_TSHDN_INT] = { .group = SYS_INT, .mask = 1 << 6 },
	[MAX77775_SYSTEM_IRQ_SCP_INT] = { .group = SYS_INT, .mask = 1 << 7 },

	[MAX77775_CHG_IRQ_BYP_I] = { .group = CHG_INT, .mask = 1 << 0 },
	[MAX77775_CHG_IRQ_BATP_I] = { .group = CHG_INT, .mask = 1 << 2 },
	[MAX77775_CHG_IRQ_BAT_I] = { .group = CHG_INT, .mask = 1 << 3 },
	[MAX77775_CHG_IRQ_CHG_I] = { .group = CHG_INT, .mask = 1 << 4 },
	[MAX77775_CHG_IRQ_WCIN_I] = { .group = CHG_INT, .mask = 1 << 5 },
	[MAX77775_CHG_IRQ_CHGIN_I] = { .group = CHG_INT, .mask = 1 << 6 },
	[MAX77775_CHG_IRQ_AICL_I] = { .group = CHG_INT, .mask = 1 << 7 },

	[MAX77775_FG_IRQ_ALERT] = { .group = FUEL_INT, .mask = 1 << 1 },

	[MAX77775_USBC_IRQ_APC_INT] = { .group = USBC_INT, .mask = 1 << 7 },
	[MAX77775_USBC_IRQ_SYSM_INT] = { .group = USBC_INT, .mask = 1 << 6 },
	[MAX77775_USBC_IRQ_VBUS_INT] = { .group = USBC_INT, .mask = 1 << 5 },
	[MAX77775_USBC_IRQ_VBADC_INT] = { .group = USBC_INT, .mask = 1 << 4 },
	[MAX77775_USBC_IRQ_DCD_INT] = { .group = USBC_INT, .mask = 1 << 3 },
	[MAX77775_USBC_IRQ_STOPMODE_INT] = { .group = USBC_INT, .mask = 1 << 2 },
	[MAX77775_USBC_IRQ_CHGT_INT] = { .group = USBC_INT, .mask = 1 << 1 },
	[MAX77775_USBC_IRQ_UIDADC_INT] = { .group = USBC_INT, .mask = 1 << 0 },

	[MAX77775_CC_IRQ_VCONNCOP_INT] = { .group = CC_INT, .mask = 1 << 7 },
	[MAX77775_CC_IRQ_VSAFE0V_INT] = { .group = CC_INT, .mask = 1 << 6 },
	[MAX77775_CC_IRQ_DETABRT_INT] = { .group = CC_INT, .mask = 1 << 5 },
	[MAX77775_CC_IRQ_VCONNSC_INT] = { .group = CC_INT, .mask = 1 << 4 },
	[MAX77775_CC_IRQ_CCPINSTAT_INT] = { .group = CC_INT, .mask = 1 << 3 },
	[MAX77775_CC_IRQ_CCISTAT_INT] = { .group = CC_INT, .mask = 1 << 2 },
	[MAX77775_CC_IRQ_CCVCNSTAT_INT] = { .group = CC_INT, .mask = 1 << 1 },
	[MAX77775_CC_IRQ_CCSTAT_INT] = { .group = CC_INT, .mask = 1 << 0 },

	[MAX77775_PD_IRQ_PDMSG_INT] = { .group = PD_INT, .mask = 1 << 7 },
	[MAX77775_PD_IRQ_PS_RDY_INT] = { .group = PD_INT, .mask = 1 << 6 },
	[MAX77775_PD_IRQ_DATAROLE_INT] = { .group = PD_INT, .mask = 1 << 5 },
	[MAX77775_IRQ_VDM_ATTENTION_INT] = { .group = PD_INT, .mask = 1 << 4 },
	[MAX77775_IRQ_VDM_DP_CONFIGURE_INT] = { .group = PD_INT, .mask = 1 << 3 },
	[MAX77775_IRQ_VDM_DP_STATUS_UPDATE_INT] = { .group = PD_INT, .mask = 1 << 2 },
	[MAX77775_PD_IRQ_SSACCI_INT] = { .group = PD_INT, .mask = 1 << 1 },
	[MAX77775_PD_IRQ_FCTIDI_INT] = { .group = PD_INT, .mask = 1 << 0 },

	[MAX77775_IRQ_VDM_DISCOVER_ID_INT] = { .group = VDM_INT, .mask = 1 << 0 },
	[MAX77775_IRQ_VDM_DISCOVER_SVIDS_INT] = { .group = VDM_INT, .mask = 1 << 1 },
	[MAX77775_IRQ_VDM_DISCOVER_MODES_INT] = { .group = VDM_INT, .mask = 1 << 2 },
	[MAX77775_IRQ_VDM_ENTER_MODE_INT] = { .group = VDM_INT, .mask = 1 << 3 },

	[MAX77775_IRQ_USBID_INT] = { .group = SPARE_INT, .mask = 1 << 7 },
	[MAX77775_IRQ_TACONN_INT] = { .group = SPARE_INT, .mask = 1 << 6 },

	[MAX77775_VIR_IRQ_ALTERROR_INT] = { .group = VIR_INT, .mask = 1 << 0 },
};

static void max77775_irq_lock(struct irq_data *data)
{
	struct max77775_dev *max77775 = irq_get_chip_data(data->irq);

	mutex_lock(&max77775->irqlock);
}

static void max77775_irq_sync_unlock(struct irq_data *data)
{
	struct max77775_dev *max77775 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < MAX77775_IRQ_GROUP_NR; i++) {
		u8 mask_reg = max77775_mask_reg[i];
		struct i2c_client *i2c = get_i2c(max77775, i);

		if (mask_reg == MAX77775_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		max77775->irq_masks_cache[i] = max77775->irq_masks_cur[i];

		max77775_write_reg(i2c, max77775_mask_reg[i],
				max77775->irq_masks_cur[i]);
	}

	mutex_unlock(&max77775->irqlock);
}

static const inline struct max77775_irq_data *
irq_to_max77775_irq(struct max77775_dev *max77775, int irq)
{
	return &max77775_irqs[irq - max77775->irq_base];
}

static void max77775_irq_mask(struct irq_data *data)
{
	struct max77775_dev *max77775 = irq_get_chip_data(data->irq);
	const struct max77775_irq_data *irq_data =
	    irq_to_max77775_irq(max77775, data->irq);

	if (irq_data->group >= MAX77775_IRQ_GROUP_NR)
		return;

	max77775->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static void max77775_irq_unmask(struct irq_data *data)
{
	struct max77775_dev *max77775 = irq_get_chip_data(data->irq);
	const struct max77775_irq_data *irq_data =
	    irq_to_max77775_irq(max77775, data->irq);

	if (irq_data->group >= MAX77775_IRQ_GROUP_NR)
		return;

	max77775->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static void max77775_irq_disable(struct irq_data *data)
{
	max77775_irq_mask(data);
}

static struct irq_chip max77775_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock		= max77775_irq_lock,
	.irq_bus_sync_unlock	= max77775_irq_sync_unlock,
	.irq_mask		= max77775_irq_mask,
	.irq_unmask		= max77775_irq_unmask,
	.irq_disable            = max77775_irq_disable,
};

#define VB_LOW 0

static irqreturn_t max77775_irq_thread(int irq, void *data)
{
	struct max77775_dev *max77775 = data;
	u8 irq_reg[MAX77775_IRQ_GROUP_NR] = {0};
	u8 irq_src;
	int i, ret;
	u8 irq_vdm_mask = 0x0;
	u8 dump_reg[10] = {0, };
	u8 reg_data;
	u8 cc_status1 = 0;
	u8 cc_status2 = 0;
	u8 bc_status = 0;
	u8 ccstat = 0;
	u8 vbvolt = 0;
	u8 pre_ccstati = 0;
	u8 ic_alt_mode = 0;

	md75_info_usb("%s:%s irq gpio pre-state(0x%02x)\n",
		MFD_DEV_NAME, __func__,
		gpio_get_value(max77775->irq_gpio));

	__pm_stay_awake(&max77775->ws);

	if (max77775->suspended) {
		md75_err_usb("%s:%s skip.max77775 suspended. irq gpio post-state(0x%02x)\n",
			MFD_DEV_NAME, __func__, gpio_get_value(max77775->irq_gpio));
		/* Irq will occur again because of IRQF_TRIGGER_LOW */
		wait_event_interruptible_timeout(max77775->suspend_wait,
						!max77775->suspended,
						msecs_to_jiffies(50));
		__pm_relax(&max77775->ws);
		return IRQ_NONE;
	}

	ret = max77775_read_reg(max77775->i2c,
					MAX77775_PMIC_REG_INTSRC, &irq_src);
	if (ret) {
		md75_err_usb("%s:%s Failed to read interrupt source: %d\n",
			MFD_DEV_NAME, __func__, ret);
		__pm_relax(&max77775->ws);
		return IRQ_NONE;
	}

	md75_info_usb("%s:%s: irq[%d] %d/%d/%d irq_src=0x%02x (FW%02x.%02x)\n",
			MFD_DEV_NAME, __func__, irq, max77775->irq,
			max77775->irq_base, max77775->irq_gpio, irq_src,
			max77775->FW_Revision, max77775->FW_Minor_Revision);

	if (irq_src & MAX77775_IRQSRC_CHG) {
		/* CHG_INT */
		ret = max77775_read_reg(max77775->charger, MAX77775_CHG_REG_INT,
				&irq_reg[CHG_INT]);

		if (max77775->enable_nested_irq) {
			irq_reg[USBC_INT] |= max77775->usbc_irq;
			max77775->enable_nested_irq = 0x0;
			max77775->usbc_irq = 0x0;
		}

		pr_debug("%s:%s charger interrupt(0x%02x)\n",
				MFD_DEV_NAME, __func__, irq_reg[CHG_INT]);
		/* mask chgin to prevent chgin infinite interrupt
		 * chgin is unmasked chgin isr
		 */
		if (irq_reg[CHG_INT] &
				max77775_irqs[MAX77775_CHG_IRQ_CHGIN_I].mask) {
			max77775_read_reg(max77775->charger,
				MAX77775_CHG_REG_INT_MASK, &reg_data);
			reg_data |= (1 << 6);
			/*
			max77775_write_reg(max77775->charger,
				MAX77775_CHG_REG_INT_MASK, reg_data);
			*/
		}
	}

	if (irq_src & MAX77775_IRQSRC_FG) {
		pr_debug("%s:[%s] fuelgauge interrupt\n", MFD_DEV_NAME, __func__);
		pr_debug("%s:[%s]IRQ_BASE(%d), NESTED_IRQ(%d)\n",
			MFD_DEV_NAME, __func__, max77775->irq_base,
			max77775->irq_base + MAX77775_FG_IRQ_ALERT);
		irq_reg[FUEL_INT] = 1 << 1;
	}

	if (irq_src & MAX77775_IRQSRC_TOP) {
		/* SYS_INT */
		ret = max77775_read_reg(max77775->i2c, MAX77775_PMIC_REG_SYSTEM_INT,
			&irq_reg[SYS_INT]);
		pr_debug("%s:%s topsys interrupt(0x%02x)\n",
			MFD_DEV_NAME, __func__, irq_reg[SYS_INT]);
	}

	if ((irq_src & MAX77775_IRQSRC_USBC) && max77775->cc_booting_complete) {
		/* USBC INT */
		ret = max77775_bulk_read(max77775->muic, MAX77775_USBC_REG_UIC_INT,
				5, &irq_reg[USBC_INT]);
		ret = max77775_read_reg(max77775->muic, MAX77775_USBC_REG_VDM_INT_M,
				&irq_vdm_mask);
		if (irq_reg[USBC_INT] & BIT_VBUSDetI) {
			ret = max77775_read_reg(max77775->muic, REG_BC_STATUS, &bc_status);
			ret = max77775_read_reg(max77775->muic, REG_CC_STATUS1, &cc_status1);
			vbvolt = (bc_status & BIT_VBUSDet) >> FFS(BIT_VBUSDet);
			ccstat = (cc_status1 & BIT_CCStat) >> FFS(BIT_CCStat);
			if (cc_No_Connection == ccstat && vbvolt == VB_LOW) {
				pre_ccstati = irq_reg[CC_INT];
				irq_reg[CC_INT] |= 0x1;
				md75_info_usb("%s:%s [max77775] set the cc_stat int [work-around] :%x, %x\n",
						MFD_DEV_NAME, __func__,
						pre_ccstati, irq_reg[CC_INT]);
			}
		}
		if (irq_reg[SPARE_INT] & BIT_USBID) {
			// To Do
		}

		ret = max77775_bulk_read(max77775->muic, MAX77775_USBC_REG_USBC_STATUS1,
				10, dump_reg);
		md75_info_usb("%s:%s irq_reg, complete [%x], uicI=%x, ccI=%x, pdI=%x, vdmI=%x, vdmM=%x, sprI=%x\n",
			MFD_DEV_NAME, __func__,
			max77775->cc_booting_complete,
			irq_reg[USBC_INT], irq_reg[CC_INT], irq_reg[PD_INT], irq_reg[VDM_INT], irq_vdm_mask, irq_reg[SPARE_INT]);
		md75_info_usb("%s:%s dump_reg, S1=%x, S2=%x, bcS=%x, fwrev2=%x, ccS1=%x, ccS2=%x, pdS1=%x, pdS2=%x, sp1S=%x, sp2S=%x\n",
				MFD_DEV_NAME, __func__,
				dump_reg[0], dump_reg[1], dump_reg[2], dump_reg[3], dump_reg[4],
				dump_reg[5], dump_reg[6], dump_reg[7], dump_reg[8], dump_reg[9]);
	}

	if (max77775->cc_booting_complete) {
		max77775_read_reg(max77775->muic, REG_CC_STATUS2, &cc_status2);
		ic_alt_mode = (cc_status2 & BIT_Altmode) >> FFS(BIT_Altmode);
		if (!ic_alt_mode && max77775->set_altmode_en)
			irq_reg[VIR_INT] |= (1 << 0);
		md75_info_usb("%s:%s ic_alt_mode=%d\n", MFD_DEV_NAME, __func__, ic_alt_mode);

		if (irq_reg[PD_INT] & BIT_PDMsg) {
			if (dump_reg[6] == Sink_PD_PSRdy_received
					|| dump_reg[6] == SRC_CAP_RECEIVED) {
				if (max77775->check_pdmsg)
					max77775->check_pdmsg(max77775->usbc_data, dump_reg[6]);
			}
		}
	}

	/* Apply masking */
	for (i = 0; i < MAX77775_IRQ_GROUP_NR; i++)
		irq_reg[i] &= ~max77775->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < MAX77775_IRQ_NR; i++) {
		if (irq_reg[max77775_irqs[i].group] & max77775_irqs[i].mask)
			handle_nested_irq(max77775->irq_base + i);
	}

	__pm_relax(&max77775->ws);

	md75_info_usb("%s:%s irq gpio post-state(0x%02x)\n",
		MFD_DEV_NAME, __func__,
		gpio_get_value(max77775->irq_gpio));

	return IRQ_HANDLED;
}

int max77775_irq_init(struct max77775_dev *max77775)
{
	int i;
	int ret = 0;
	u8 i2c_data;
	int cur_irq;

	if (!gpio_is_valid(max77775->irq_gpio)) {
		dev_warn(max77775->dev, "No interrupt specified.\n");
		max77775->irq_base = 0;
		goto err;
	}

	if (max77775->irq_base < 0) {
		dev_err(max77775->dev, "No interrupt base specified.\n");
		goto err;
	}

	mutex_init(&max77775->irqlock);

	max77775->irq = gpio_to_irq(max77775->irq_gpio);
	md75_info_usb("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			max77775->irq, max77775->irq_gpio);

	ret = gpio_request(max77775->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(max77775->dev, "%s: failed requesting gpio %d\n",
			__func__, max77775->irq_gpio);
		goto err_gpio_request;
	}
	gpio_direction_input(max77775->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < MAX77775_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;
		/* MUIC IRQ  0:MASK 1:NOT MASK => NOT USE */
		/* Other IRQ 1:MASK 0:NOT MASK */
		max77775->irq_masks_cur[i] = 0xff;
		max77775->irq_masks_cache[i] = 0xff;

		i2c = get_i2c(max77775, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (max77775_mask_reg[i] == MAX77775_REG_INVALID)
			continue;
		max77775_write_reg(i2c, max77775_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < MAX77775_IRQ_NR; i++) {
		cur_irq = i + max77775->irq_base;
		irq_set_chip_data(cur_irq, max77775);
		irq_set_chip_and_handler(cur_irq, &max77775_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	/* Unmask max77775 interrupt */
	ret = max77775_read_reg(max77775->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		md75_err_usb("%s:%s fail to read muic reg\n", MFD_DEV_NAME, __func__);
		goto err_read_intsrc_mask2;
	}
	i2c_data |= 0xF;	/* mask intsrc interrupt */

	max77775_write_reg(max77775->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			   i2c_data);


	ret = request_threaded_irq(max77775->irq, NULL, max77775_irq_thread,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "max77775-irq", max77775);
	if (ret) {
		dev_err(max77775->dev, "Failed to request IRQ %d: %d\n",
			max77775->irq, ret);
		goto err_read_intsrc_mask2;
	}

	enable_irq_wake(max77775->irq);

	/* Unmask max77775 interrupt */
	ret = max77775_read_reg(max77775->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		md75_err_usb("%s:%s fail to read muic reg\n", MFD_DEV_NAME, __func__);
		goto err_read_intsrc_mask1;
	}

	i2c_data &= ~(MAX77775_IRQSRC_CHG);	/* Unmask charger interrupt */

	max77775_write_reg(max77775->i2c, MAX77775_PMIC_REG_INTSRC_MASK,
			   i2c_data);

	md75_info_usb("%s:%s max77775_PMIC_REG_INTSRC_MASK=0x%02x\n",
			MFD_DEV_NAME, __func__, i2c_data);

	return 0;
err_read_intsrc_mask1:
	if (max77775->irq)
		free_irq(max77775->irq, max77775);
err_read_intsrc_mask2:
	if (gpio_is_valid(max77775->irq_gpio))
		gpio_free(max77775->irq_gpio);
err_gpio_request:
	mutex_destroy(&max77775->irqlock);
err:
	return ret;
}

void max77775_irq_exit(struct max77775_dev *max77775)
{
	if (max77775->irq)
		free_irq(max77775->irq, max77775);
	if (gpio_is_valid(max77775->irq_gpio))
		gpio_free(max77775->irq_gpio);
	mutex_destroy(&max77775->irqlock);
}
