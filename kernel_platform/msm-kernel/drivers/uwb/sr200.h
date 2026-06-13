/*====================================================================================*/
/*                                                                                    */
/*                        Copyright 2022 NXP                                     */
/*                                                                                    */
/* This program is free software; you can redistribute it and/or modify               */
/* it under the terms of the GNU General Public License as published by               */
/* the Free Software Foundation; either version 2 of the License, or                  */
/* (at your option) any later version.                                                */
/*                                                                                    */
/* This program is distributed in the hope that it will be useful,                    */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                     */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      */
/* GNU General Public License for more details.                                       */
/*                                                                                    */
/* You should have received a copy of the GNU General Public License                  */
/* along with this program; if not, write to the Free Software                        */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA          */
/*                                                                                    */
/*====================================================================================*/

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

#define SR200_MAGIC 0xEA
#define SR200_SET_PWR _IOW(SR200_MAGIC, 0x01, long)
#define SR200_SET_DBG _IOW(SR200_MAGIC, 0x02, long)
#define SR200_SET_POLL _IOW(SR200_MAGIC, 0x03, long)
#define SR200_SET_FWD _IOW(SR200_MAGIC, 0x04, long)
#define SR200_GET_THROUGHPUT _IOW(SR200_MAGIC, 0x05, long)
#define SR200_ESE_RESET _IOW(SR200_MAGIC, 0x06, long)
#define SR200_GET_ANT_CONNECTION_STATUS _IOW(SR200_MAGIC, 0x61, long)
#define UCI_HEADER_LEN 4
#define HDLL_MODE_HEADER_LEN 2
#define NORMAL_MODE_LEN_OFFSET 3
#define UCI_NORMAL_PKT_SIZE 0
#define DIRECTIONAL_BYTE_LEN 1
#define CRC_BYTES_LEN 2
#define UCI_MT_MASK 0xE0
#define UCI_EXTND_LEN_INDICATOR_OFFSET 1
#define UCI_EXTND_LEN_INDICATOR_OFFSET_MASK 0x80
#define UCI_EXTENDED_LENGTH_OFFSET 2

#define SR200_TXBUF_SIZE 4201 //4200 + one directional byte
#define SR200_RXBUF_SIZE 4202 //4200 + two directional bytes
#define SR200_MAX_TX_BUF_SIZE 4200
#define MAX_READ_RETRY_COUNT 10
/* Macro to define SPI clock frequency */
#define SR200_SPI_CLOCK 20000000L;
/* Maximum UCI packet size supported from the driver */
#define MAX_UCI_PKT_SIZE 4200
/* Different driver debug lever */
struct sr200_spi_platform_data {
  unsigned int irq_gpio;
//#ifdef CONFIG_UWB_USE_CLK_REQ
  unsigned int clk_req_gpio;
//#endif
  unsigned int reset_gpio;
  unsigned int ant_connection_status_gpio;
  const char *uwb_vdd_io;
  const char *uwb_vdd_sw;
};
enum {
  PWR_DISABLE = 0,
  PWR_ENABLE,
  ABORT_READ_PENDING
};
enum SR200_DEBUG_LEVEL {
  SR200_DEBUG_OFF,
  SR200_FULL_DEBUG
};
enum spi_status_codes {
  spi_transcive_success,
  spi_transcive_fail,
  spi_irq_wait_request,
  spi_irq_wait_timeout
};
enum spi_operation_modes {
  SR200_WRITE_MODE,
  SR200_READ_MODE
};
/* Device specific macro and structure */
struct sr200_dev {
  wait_queue_head_t read_wq;      /* wait queue for read interrupt */
  struct spi_device *spi;         /* spi device structure */
  struct miscdevice sr200_device; /* char device as misc driver */
  unsigned int reset_gpio;           /* SW Reset gpio */
  unsigned int irq_gpio;          /* sr200 will interrupt DH for any ntf */
//#ifdef CONFIG_UWB_USE_CLK_REQ
  unsigned int clk_req_gpio;      /* sr200 will interrupt DH for requesting clk */
  int clk_req_irq;
//#endif
  unsigned int ant_connection_status_gpio; /* antenna connection status */
  bool irq_enabled;  /* flag to indicate disable/enable irq sequence */
  bool irq_received; /* flag to indicate that irq is received */
  spinlock_t irq_enabled_lock; /* spin lock for read irq */
  unsigned char *tx_buffer;    /* transmit buffer */
  unsigned char *rx_buffer;    /* receive buffer buffer */
  unsigned char *rx_dir_byte_buff; /* rx directional byte buffer */
  unsigned int write_count;    /* Holds nubers of  byte writen*/
  unsigned int read_count;     /* Hold nubers of  byte read */
  struct mutex sr200_access_lock; /* Hold mutex lock to between read and write */
  size_t total_bytes_to_read;
  size_t is_ext_payload_len_bit_set;
  int mode;
  long timeout_in_ms;
  const char *uwb_vdd_io;
  const char *uwb_vdd_sw;
  struct mutex sr200_read_count_lock;
};
