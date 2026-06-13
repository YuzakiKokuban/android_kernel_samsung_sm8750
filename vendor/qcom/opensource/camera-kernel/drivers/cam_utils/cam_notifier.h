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

#ifndef CAM_NOTIFIER_H
#define CAM_NOTIFIER_H
#include <linux/notifier.h>

int is_eeprom_info_update(uint32_t, char *);
int is_eeprom_wacom_update_notifier(void);
int is_register_notifier(struct raw_notifier_head*, struct notifier_block*);
int is_unregister_notifier(struct raw_notifier_head*, struct notifier_block*);

int is_register_eeprom_notifier(struct notifier_block *nb);
int is_unregister_eeprom_notifier(struct notifier_block *nb);

#if defined(CONFIG_SAMSUNG_WACOM_NOTIFIER)
extern struct notifier_block *g_nb_wac_camera;
#endif

#endif /* CAM_NOTIFIER_H */
