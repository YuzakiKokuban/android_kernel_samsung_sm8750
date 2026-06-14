/*
 * Copyright (C) 2020 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __ESE_LOG__H
#define __ESE_LOG__H

#include <linux/printk.h>

#define LOG_D(msg...) pr_debug("[star-protocol] : " msg);
#define LOG_I(msg...) pr_info("[star-protocol] : " msg);
#define LOG_W(msg...) pr_warn("[star-protocol] : " msg);
#define LOG_E(msg...) pr_err("[star-protocol] : " msg);

#endif
