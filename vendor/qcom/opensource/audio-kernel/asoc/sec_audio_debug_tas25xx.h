/* SPDX-License-Identifier: GPL-2.0-only */
/*
* Copyright (c) 2024, The Linux Foundation. All rights reserved.
*/
/*
*  sec_audio_debug.c
*
*  Copyright (c) 2024 Samsung Electronics
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
*/

// amp status register check list
#if defined(CONFIG_SND_SOC_TAS25XX)
#define TAS25XX_REG_POWER 0x2
#define TAS25XX_REG_MODE 0x3
#define TAS25XX_REG_INIT 0x4
#define TAS25XX_REG_TDM 0x14
#define TAS25XX_REG_ADC 0x26
#define TAS25XX_REG_NG_LP 0x65
#define TAS25XX_REG_ADC_INDEX 0x5
#endif