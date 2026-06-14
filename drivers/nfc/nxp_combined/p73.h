/******************************************************************************
 *
 *  Copyright (C) 2012-2020 NXP Semiconductors
 *   *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************/

#ifndef _P73_H_
#define _P73_H_
#define P61_MAGIC 0xEB
#define P61_SET_PWR _IOW(P61_MAGIC, 0x01, long)
#define P61_SET_DBG _IOW(P61_MAGIC, 0x02, long)
#define P61_SET_POLL _IOW(P61_MAGIC, 0x03, long)
/*
 * SPI Request NFCC to enable p61 power, only in param
 * Only for SPI
 * level 1 = Enable power
 * level 0 = Disable power
 */
#define P61_SET_SPM_PWR    _IOW(P61_MAGIC, 0x04, long)

/* SPI or DWP can call this ioctl to get the current
 * power state of P61
 *
*/
#define P61_GET_SPM_STATUS    _IOR(P61_MAGIC, 0x05, long)

/* throughput measurement is deprecated */
/* #define P61_SET_THROUGHPUT    _IOW(P61_MAGIC, 0x06, long) */
#define P61_GET_ESE_ACCESS    _IOW(P61_MAGIC, 0x07, long)

#define P61_SET_POWER_SCHEME  _IOW(P61_MAGIC, 0x08, long)

#define P61_SET_DWNLD_STATUS    _IOW(P61_MAGIC, 0x09, long)

#define P61_INHIBIT_PWR_CNTRL  _IOW(P61_MAGIC, 0x0A, long)

/* SPI can call this IOCTL to perform the eSE COLD_RESET
 * via NFC driver.
 */
#define ESE_PERFORM_COLD_RESET  _IOW(P61_MAGIC, 0x0C, long)

#define PERFORM_RESET_PROTECTION  _IOW(P61_MAGIC, 0x0D, long)

#define ESE_SET_TRUSTED_ACCESS  _IOW(P61_MAGIC, 0x0B, long)

#define P61_RW_SPI_DATA	_IOWR(P61_MAGIC, 0x0F, unsigned long)

#ifdef CONFIG_COMPAT
#define P61_SET_PWR_COMPAT				_IOW(P61_MAGIC, 0x01, unsigned int)
#define P61_SET_DBG_COMPAT				_IOW(P61_MAGIC, 0x02, unsigned int)
#define P61_SET_POLL_COMPAT				_IOW(P61_MAGIC, 0x03, unsigned int)
#define P61_SET_SPM_PWR_COMPAT			_IOW(P61_MAGIC, 0x04, unsigned int)
#define P61_GET_SPM_STATUS_COMPAT		_IOR(P61_MAGIC, 0x05, unsigned int)
/* throughput measurement is deprecated */
/* #define P61_SET_THROUGHPUT_COMPAT		_IOW(P61_MAGIC, 0x06, unsigned int) */
#define P61_GET_ESE_ACCESS_COMPAT		_IOW(P61_MAGIC, 0x07, unsigned int)
#define P61_SET_POWER_SCHEME_COMPAT		_IOW(P61_MAGIC, 0x08, unsigned int)
#define P61_SET_DWNLD_STATUS_COMPAT		_IOW(P61_MAGIC, 0x09, unsigned int)
#define P61_INHIBIT_PWR_CNTRL_COMPAT	_IOW(P61_MAGIC, 0x0A, unsigned int)
#define P61_RW_SPI_DATA_COMPAT			_IOWR(P61_MAGIC, 0x0F, unsigned int)

#define ESE_PERFORM_COLD_RESET_COMPAT	_IOW(P61_MAGIC, 0x0C, unsigned int)
#define PERFORM_RESET_PROTECTION_COMPAT	_IOW(P61_MAGIC, 0x0D, unsigned int)
#endif

typedef enum ese_spi_transition_state {
	ESE_SPI_IDLE = 0x00,
	ESE_SPI_BUSY
} ese_spi_transition_state_t;

struct p61_spi_platform_data {
	int irq_gpio;
	int rst_gpio;
	int trusted_ese_gpio;
	bool gpio_coldreset;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	int ap_vendor;
#endif
};

struct p61_ioctl_transfer {
	unsigned char *rx_buffer;
	unsigned char *tx_buffer;
	unsigned int len;
};

#ifdef CONFIG_COMPAT
struct p61_ioctl_transfer32 {
	u32 rx_buffer;
	u32 tx_buffer;
	u32 len;
};
#endif

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
void store_nfc_i2c_device(struct device *nfc_i2c_dev);
void p61_print_status(const char *func_name);
#endif
#endif
