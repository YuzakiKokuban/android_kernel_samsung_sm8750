/*
 * header file supporting USB Type-C Manager hwpram call chain information
 *
 * Copyright (C) 2020 Samsung Electronics
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __USB_TYPEC_MANAGER_HWPARAM_H__
#define __USB_TYPEC_MANAGER_HWPARAM_H__

void water_dry_time_update(int mode);
void wVbus_time_update(int status);
unsigned long manager_hw_param_update(int param);
void manager_hw_param_init(void);
void usb_enum_hw_param_data_update(int speed);

#endif /* __USB_TYPEC_MANAGER_HWPARAM_H__ */
