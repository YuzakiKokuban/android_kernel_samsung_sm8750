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

#ifndef __ESE_ERROR__H
#define __ESE_ERROR__H

enum ESESTATUS {
	ESESTATUS_SUCCESS					= (0x0000),
	ESESTATUS_FAILED					= (0x0001),
	ESESTATUS_NOT_INITIALISED			= (0x0002),
	ESESTATUS_ALREADY_INITIALISED		= (0x0003),
	ESESTATUS_INVALID_PARAMETER			= (0x0004),
	ESESTATUS_INVALID_DEVICE			= (0x0005),
	ESESTATUS_INVALID_STATE				= (0x0006),
	ESESTATUS_FEATURE_NOT_SUPPORTED		= (0x0007),

	ESESTATUS_INVALID_BUFFER			= (0x0030),
	ESESTATUS_NOT_ENOUGH_MEMORY			= (0x0031),
	ESESTATUS_MEMORY_ALLOCATION_FAIL	= (0x0032),

	ESESTATUS_INVALID_CLA				= (0x0050),
	ESESTATUS_INVALID_CPDU_TYPE			= (0x0051),
	ESESTATUS_INVALID_LE_TYPE			= (0x0052),
	ESESTATUS_INVALID_FORMAT			= (0x0053),
	ESESTATUS_INVALID_FRAME				= (0x0054),

	ESESTATUS_SEND_FAILED				= (0x0060),
	ESESTATUS_RECEIVE_FAILED			= (0x0061),
	ESESTATUS_RESPONSE_TIMEOUT			= (0x0062),
	ESESTATUS_INVALID_SEND_LENGTH		= (0x0063),
	ESESTATUS_INVALID_RECEIVE_LENGTH	= (0x0064),
};

typedef uint32_t ESE_STATUS;
#endif
