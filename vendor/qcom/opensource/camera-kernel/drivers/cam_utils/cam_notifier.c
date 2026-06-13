/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "cam_notifier.h"
#include <cam_sensor_cmn_header.h>

/*
 * Notifier list for kernel code which wants to know
 * camera module versions
 */
static RAW_NOTIFIER_HEAD(dev_cam_eeprom_noti_chain);

#if defined(CONFIG_SAMSUNG_WACOM_NOTIFIER)

/*
 * notifyCameraList   - store camera list
 * wacom_notify_value - store camera eeprom information
 *
 * each camera use 1 byte.
 *
 * store up to 8 cameras. (64bit)
 */
static char checkCamera[] = { 'S', 'C', 'P', 'M', 'V', 'N', 'A', 'Y', 'H' };

uint32_t notifyCameraList[] = {
	SEC_WIDE_SENSOR,
#if defined(CONFIG_SAMSUNG_REAR_DUAL)
	SEC_ULTRA_WIDE_SENSOR,
#endif
#if defined(CONFIG_SAMSUNG_REAR_TRIPLE)
	SEC_TELE_SENSOR,
#endif
#if defined(CONFIG_SAMSUNG_REAR_QUADRA)
	SEC_TELE2_SENSOR,
#endif
	SEC_FRONT_SENSOR,
#if defined(CONFIG_SAMSUNG_FRONT_TOP)
	SEC_FRONT_TOP_SENSOR,
#endif
};

static unsigned long wacom_notify_value = 0;

/**
 * is_eeprom_info_update - update camera eeprom information to wacom_notify_value
 *	@type:	        eeprom type
 *	@header_ver:	eeprom header string.
 */
int is_eeprom_info_update(uint32_t type, char *header_ver)
{
	int  result = 0;
	uint listIndex = 0, listSize = 0;
	uint checkIndex = 0, checkSize = 0;
	bool bCheck = false;

	if (!header_ver)
	{
		return -1;
	}

	listSize = sizeof(notifyCameraList) / sizeof(uint32_t);
	checkSize = sizeof(checkCamera) / sizeof(char);

	for (listIndex = 0; listIndex < listSize; listIndex++)
	{
		if (type == notifyCameraList[listIndex])
		{
			for (checkIndex = 0; checkIndex < checkSize; checkIndex++)
			{
				if (header_ver[9] == checkCamera[checkIndex])
				{
					wacom_notify_value |= (unsigned long)(checkIndex + 1) << (listIndex * 8);
					bCheck = true;
					break;
				}
			}
		}

		if (bCheck == true)
		{
			 break;
		}
	}

	//pr_info("[NOTI_DBG] [wacom] notify header info:0x%lx(%d, %c)", wacom_notify_value, type, header_ver[9]);

	return result;
}

/**
 * is_eeprom_wacom_update_notifier - Send wacom_notify_value to notifier block.
 *
 *	Define the mask if you need to send only information from certain cameras.
 */
int is_eeprom_wacom_update_notifier(void)
{
	static bool is_registered = false;

	if (is_registered == false && g_nb_wac_camera != NULL) {
		is_register_eeprom_notifier (g_nb_wac_camera);
		is_registered = true;
	}

	if (is_registered == true) {
		pr_info("[NOTI_DBG] send value 0x%lx to wacom", wacom_notify_value);
		return raw_notifier_call_chain(&dev_cam_eeprom_noti_chain,
				wacom_notify_value, NULL);
	}
	else
		return 0;
}
#endif

/* Common Notifier API */
int is_register_eeprom_notifier(struct notifier_block *nb)
{
	return is_register_notifier(&dev_cam_eeprom_noti_chain, nb);
}
EXPORT_SYMBOL(is_register_eeprom_notifier);

int is_unregister_eeprom_notifier(struct notifier_block *nb)
{
	return is_unregister_notifier(&dev_cam_eeprom_noti_chain, nb);
}
EXPORT_SYMBOL(is_unregister_eeprom_notifier);


int is_register_notifier(struct raw_notifier_head *head, struct notifier_block *nb)
{
	if (!nb)
		return -ENOENT;

	return raw_notifier_chain_register(head,nb);
}

int is_unregister_notifier(struct raw_notifier_head *head, struct notifier_block *nb)
{
	if (!nb)
		return -ENOENT;

	return raw_notifier_chain_unregister(head,nb);
}

