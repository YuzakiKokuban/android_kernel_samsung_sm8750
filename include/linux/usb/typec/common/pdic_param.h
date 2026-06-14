/*
 *
 * Copyright (C) 2021 Samsung Electronics
 *
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
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LINUX_PDIC_PARAM_H__
#define __LINUX_PDIC_PARAM_H__

enum pdic_param_usbmode {
	PDIC_PARAM_MODE_NO,
	PDIC_PARAM_MODE_OB,
	PDIC_PARAM_MODE_IB,
	PDIC_PARAM_MODE_DL,
	PDIC_PARAM_MODE_LC,
};

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
extern int check_factory_mode_boot(void);
extern int get_usb_factory_mode(void);
extern int is_lpcharge_pdic_param(void);
extern int is_factory_mode_pdic_param(void);
extern int is_recovery_mode_pdic_param(void);
#else
static inline int check_factory_mode_boot(void)
	{return 0; }
static inline int get_usb_factory_mode(void)
	{return PDIC_PARAM_MODE_NO;}
static inline int is_lpcharge_pdic_param(void)
	{return 0; }
static inline int is_factory_mode_pdic_param(void)
	{return 0; }
static inline int is_recovery_mode_pdic_param(void)
	{return 0; }
#endif
#endif /* __LINUX_PDIC_PARAM_H__ */
