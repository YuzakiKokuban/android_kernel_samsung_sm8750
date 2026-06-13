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

#ifndef __LINUX_MUIC_PARAM_H__
#define __LINUX_MUIC_PARAM_H__

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
extern int get_switch_sel(void);
extern int get_uart_sel(void);
extern int get_afc_mode(void);
extern int get_pdic_info(void);
#else
static inline int get_switch_sel(void) {return 0; }
static inline int get_uart_sel(void) {return 0; }
static inline int get_afc_mode(void) {return 0; }
static inline int get_pdic_info(void) {return 1; }
#endif
#endif /* __LINUX_MUIC_PARAM_H__ */
