/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SEC_DISPLAYPORT_H
#define __SEC_DISPLAYPORT_H

/*
 * this function waits for completion of dp disconnection.
 * return : zero if dp resource is released completely.
 *          non-zero if waiting timeout happens.
 */
int secdp_wait_for_disconnect_complete(void);

/**
 * if SSUSB gets reset, it need to call this callback to let DP know, since
 * DP shares PHY with SSUSB (combo PHY)
 * return  0    if success
 * return -1    otherwise
 */
int secdp_pdic_reset_cb(bool reset);

#endif/*__SEC_DISPLAYPORT_H*/
