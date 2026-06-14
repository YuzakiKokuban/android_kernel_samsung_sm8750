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

/**
 * \addtogroup spi_driver
 *
 * @{ */
#include "sr200.h"
#include "../uwb/uwb_logger/uwb_logger.h"

#define DEBUG_LOG
/* Cold reset Feature in case of Secure Element tx timeout */
#define ESE_COLD_RESET 1
#if ESE_COLD_RESET
#ifdef CONFIG_NFC_NXP_COMBINED
#include "../nfc/nxp_combined/common_ese.h"
#else
#include "../nfc/common_ese.h"
#endif
/*Invoke cold reset if no response from eSE*/
extern int perform_ese_cold_reset(unsigned long source);
#endif
static bool read_abort_requested = false;
static bool is_fw_dwnld_enabled = false;
/* Variable to store current debug level request by ioctl */
static unsigned char debug_level;
#define SR200_DBG_MSG(msg...)                                                  \
  switch (debug_level) {                                                       \
  case SR200_DEBUG_OFF:                                                        \
    break;                                                                     \
  case SR200_FULL_DEBUG:                                                       \
    UWB_LOG_INFO(msg);                                                         \
    break;                                                                     \
  default:                                                                     \
    printk(KERN_ERR "[NXP-sr200] :  wrong debug level %d", debug_level);       \
    break;                                                                     \
  }
#define SR200_ERR_MSG(msg...) UWB_LOG_ERR(msg);

#if 1//MW WA
#define WRITE_GUARD_TIME 500
static bool isNtfReceived = false;
#endif

/******************************************************************************
 * Function    : sr200_dev_open
 *
 * Description : Open sr200 device node and returns instance to the user space
 *
 * Parameters  : inode  :  sr200 device node path
 *               filep  :  File pointer to structure of sr200 device
 *
 * Returns     : Returns file descriptor for sr200 device
 *               otherwise indicate each error code
 ****************************************************************************/
static int sr200_dev_open(struct inode *inode, struct file *filp) {
  struct sr200_dev *sr200_dev =
      container_of(filp->private_data, struct sr200_dev, sr200_device);
  filp->private_data = sr200_dev;
  SR200_DBG_MSG("%s : major No: %d, minor No: %d\n", __func__, imajor(inode),
                iminor(inode));
  if (device_can_wakeup(&sr200_dev->spi->dev)) {
    device_wakeup_enable(&sr200_dev->spi->dev);
  }
  return 0;
}
/******************************************************************************
 * Function    : sr200_disable_irq
 *
 * Description : To disable IR
 *
 * Parameters  : sr200_dev  :  sr200 device structure pointer
 *
 * Returns     : Returns void
 ****************************************************************************/
static void sr200_disable_irq(struct sr200_dev *sr200_dev) {
  unsigned long flags;
#ifdef DEBUG_LOG
  UWB_LOG_REC("d i\n");
#endif
  spin_lock_irqsave(&sr200_dev->irq_enabled_lock, flags);
  if ((sr200_dev->irq_enabled)) {
    disable_irq_nosync(sr200_dev->spi->irq);
    sr200_dev->irq_received = true;
    sr200_dev->irq_enabled = false;
  }
  spin_unlock_irqrestore(&sr200_dev->irq_enabled_lock, flags);
}
/******************************************************************************
 * Function    : sr200_enable_irq
 *
 * Description : Set the irq flag status
 *
 * Parameters  : sr200_dev  :  sr200 device structure pointer
 *
 * Returns     : Returns void
 ****************************************************************************/
static void sr200_enable_irq(struct sr200_dev *sr200_dev) {
  unsigned long flags;
#ifdef DEBUG_LOG
  UWB_LOG_REC("e i\n");
#endif
  spin_lock_irqsave(&sr200_dev->irq_enabled_lock, flags);
  if (!sr200_dev->irq_enabled) {
    enable_irq(sr200_dev->spi->irq);
    sr200_dev->irq_enabled = true;
    sr200_dev->irq_received = false;
  }
  spin_unlock_irqrestore(&sr200_dev->irq_enabled_lock, flags);
}
/******************************************************************************
 * Function    : sr200_dev_irq_handler
 *
 * Description : Will get called when interrupt line asserted from sr200
 *
 * Parameters  : irq    :  IRQ Number
 *               dev_id :  sr200 device Id
 *
 * Returns     : Returns IRQ Handler
 ****************************************************************************/
static irqreturn_t sr200_dev_irq_handler(int irq, void *dev_id) {
  struct sr200_dev *sr200_dev = dev_id;
#ifdef DEBUG_LOG
  UWB_LOG_REC("h\n");
#endif
  sr200_disable_irq(sr200_dev);
  /* Wake up waiting readers */
  wake_up(&sr200_dev->read_wq);
  return IRQ_HANDLED;
}

/******************************************************************************
 * Function    : sr200_dev_iotcl
 *
 * Description : Input/OutPut control from user space to perform required
 *               operation on sr200 device.
 *
 * Parameters  : cmd    :  Indicates what operation needs to be done sr200
 *               arg    :  Value to be passed to sr200 to do the required
 *                         opeation
 *
 * Returns     : 0 on success and (-1) on error
 ****************************************************************************/
static long sr200_dev_ioctl(struct file *filp, unsigned int cmd,
                            unsigned long arg) {
  int ret = 0;
  struct sr200_dev *sr200_dev = NULL;
  SR200_DBG_MSG("sr200 - %s\n", __FUNCTION__);
  sr200_dev = filp->private_data;
  if(sr200_dev == NULL) {
    ret = -EINVAL;
    SR200_ERR_MSG("sr200_dev is NULL");
    return ret;
  }
  switch (cmd) {
  case SR200_SET_PWR:
    if (arg == PWR_ENABLE) {
      SR200_DBG_MSG(" enable power request...\n");
      gpio_set_value(sr200_dev->reset_gpio, 1);
      msleep(10);
    } else if (arg == PWR_DISABLE) {
      SR200_DBG_MSG("disable power request...\n");
      gpio_set_value(sr200_dev->reset_gpio, 0);
      sr200_disable_irq(sr200_dev);
      msleep(10);
    } else if (arg == ABORT_READ_PENDING) {
      SR200_ERR_MSG("%s abort read pending\n", __func__);
      read_abort_requested = true;
      sr200_disable_irq(sr200_dev);
      /* Wake up waiting readers */
      wake_up(&sr200_dev->read_wq);
    }
    break;
  case SR200_SET_FWD:
    if (arg == 1) {
      is_fw_dwnld_enabled = true;
      read_abort_requested = false;
      SR200_DBG_MSG("%s fw download enabled.\n", __func__);
    } else if (arg == 0) {
      is_fw_dwnld_enabled = false;
      SR200_DBG_MSG("%s fw download disabled.\n", __func__);
    }
    break;
  case SR200_GET_THROUGHPUT:
    if (arg == 0) {
      SR200_DBG_MSG("%s measure throughput disabled.\n", __func__);
    }
    break;
#if ESE_COLD_RESET
  case SR200_ESE_RESET:
    SR200_DBG_MSG("%s SR200_ESE_RESET enter\n", __func__);
    ret = perform_ese_cold_reset(ESE_CLD_RST_OTHER);
    break;
#endif

  case SR200_GET_ANT_CONNECTION_STATUS:
    SR200_DBG_MSG("SR200_GET_ANT_CONNECTION_STATUS Enter\n");
    if((int)sr200_dev->ant_connection_status_gpio > 0) {
      ret = !gpio_get_value(sr200_dev->ant_connection_status_gpio);
    } else {
      ret = -1;
    }
    SR200_DBG_MSG("SR200_GET_ANT_CONNECTION_STATUS ret =%d \n", ret);
    break;
  default:
    SR200_ERR_MSG(" error case\n");
    ret = -EINVAL; // ToDo: After adding proper switch cases we have to
                   // return with error statusi here
  }
  return ret;
}
/******************************************************************************
 * Function    : sr200_dev_transceive
 *
 * Description : Used to Write/read data from sr200
 *
 * Parameters  : sr200_dev :sr200  device structure pointer
 *               op_mode   :Indicates write/read mode
 *               count  :  Number of bytes to be write/read
 * Returns     : return status code as per spi_status_codes enum,
 *               need added each status code and indicate
 *               when each status code returns
 ****************************************************************************/
static int sr200_dev_transceive(struct sr200_dev *sr200_dev, int op_mode,
                                int count) {
  int ret;
  mutex_lock(&sr200_dev->sr200_access_lock);
  sr200_dev->mode = op_mode;
  sr200_dev->total_bytes_to_read = 0;
  sr200_dev->is_ext_payload_len_bit_set = 0;
  ret = -1;
  switch (sr200_dev->mode) {
  case SR200_WRITE_MODE: {
#if 1 //MW WA
    if (!is_fw_dwnld_enabled && isNtfReceived) {
      isNtfReceived = false;
      usleep_range(WRITE_GUARD_TIME, WRITE_GUARD_TIME + 10);
    }
#endif
    sr200_dev->write_count = 0;
    ret = spi_write(sr200_dev->spi, sr200_dev->tx_buffer, count);
    if (ret < 0) {
      ret = -EIO;
      SR200_ERR_MSG("spi_write: failed.\n");
      goto transcive_end;
    }
    sr200_dev->write_count = count;
    ret = spi_transcive_success;
  } break;
  case SR200_READ_MODE: {
    int hdr_length = UCI_HEADER_LEN;
    int payloadLen = 0;
    struct spi_transfer hdr_transfer;
    if (!gpio_get_value(sr200_dev->irq_gpio)) {
      SR200_ERR_MSG("IRQ might have gone low due to write\n");
      ret = spi_irq_wait_request;
      goto transcive_end;
    }
    sr200_dev->read_count = 0;
    // set directional byte to 0XFF for read operation.
    sr200_dev->rx_dir_byte_buff[0] = 0xFF;
    if (is_fw_dwnld_enabled) {
      hdr_length = HDLL_MODE_HEADER_LEN;
    }
    hdr_transfer.len = (hdr_length + DIRECTIONAL_BYTE_LEN); // Total header bytes to read
                                                            // including directional byte.
    hdr_transfer.rx_buf = sr200_dev->rx_buffer;
    hdr_transfer.tx_buf = sr200_dev->rx_dir_byte_buff;
    ret = spi_sync_transfer(sr200_dev->spi, &hdr_transfer, 1);
    if (ret < 0) {
      SR200_ERR_MSG("spi_sync_transfer: spi transfer error.. %d\n ", ret);
      goto transcive_end;
    }
    if (is_fw_dwnld_enabled) {
      sr200_dev->total_bytes_to_read =
          (((sr200_dev->rx_buffer[1] & 0x1F) << 8) |
           sr200_dev->rx_buffer[2]); // reading 13 bits length from HDLL header
      sr200_dev->total_bytes_to_read += CRC_BYTES_LEN;
      payloadLen =
          sr200_dev->total_bytes_to_read + CRC_BYTES_LEN + DIRECTIONAL_BYTE_LEN;
    } else {
      if ((sr200_dev->rx_buffer[1] & UCI_MT_MASK) == 0) {
        sr200_dev->total_bytes_to_read =
            sr200_dev->rx_buffer[NORMAL_MODE_LEN_OFFSET + DIRECTIONAL_BYTE_LEN];
        sr200_dev->total_bytes_to_read =
            ((sr200_dev->total_bytes_to_read << 8) |
             sr200_dev->rx_buffer[UCI_EXTENDED_LENGTH_OFFSET + DIRECTIONAL_BYTE_LEN]);
      }else {
#if 1 //MW WA
        if ((sr200_dev->rx_buffer[1] & UCI_MT_MASK) == 0x60) {
          isNtfReceived = true;
        }
#endif
        sr200_dev->is_ext_payload_len_bit_set =
            (sr200_dev->rx_buffer[UCI_EXTND_LEN_INDICATOR_OFFSET +
                                  DIRECTIONAL_BYTE_LEN] &
             UCI_EXTND_LEN_INDICATOR_OFFSET_MASK);
        sr200_dev->total_bytes_to_read =
            sr200_dev->rx_buffer[NORMAL_MODE_LEN_OFFSET + DIRECTIONAL_BYTE_LEN];
        if (sr200_dev->is_ext_payload_len_bit_set) {
          sr200_dev->total_bytes_to_read =
              ((sr200_dev->total_bytes_to_read << 8) |
               sr200_dev->rx_buffer[UCI_EXTENDED_LENGTH_OFFSET +
                                    DIRECTIONAL_BYTE_LEN]);
        }
      }
      payloadLen = sr200_dev->total_bytes_to_read + DIRECTIONAL_BYTE_LEN;
    }
    if (sr200_dev->total_bytes_to_read > (MAX_UCI_PKT_SIZE - UCI_HEADER_LEN)) {
      SR200_ERR_MSG("length %d  exceeds the max limit %d....",
             (int)sr200_dev->total_bytes_to_read, (int)MAX_UCI_PKT_SIZE);
      ret = -1;
      goto transcive_end;
    }
    if (sr200_dev->total_bytes_to_read > 0) {
      struct spi_transfer payload_transfer = {
          .len = payloadLen, // Total bytes to read including directional byte
          .rx_buf = &sr200_dev->rx_buffer[hdr_length + DIRECTIONAL_BYTE_LEN],
          .tx_buf = sr200_dev->rx_dir_byte_buff,
      };
      ret = spi_sync_transfer(sr200_dev->spi, &payload_transfer, 1);
      if (ret < 0) {
        SR200_ERR_MSG("spi_sync_transfer: spi transfer error.. %d\n ", ret);
        goto transcive_end;
      }
      sr200_dev->total_bytes_to_read += (2 * DIRECTIONAL_BYTE_LEN);
    }
    sr200_dev->read_count =
        (unsigned int)(sr200_dev->total_bytes_to_read + hdr_length);
  } break;
  default:
    SR200_ERR_MSG("invalid operation .....\n");
    break;
  }
transcive_end:
  mutex_unlock(&sr200_dev->sr200_access_lock);
  return ret;
}
/******************************************************************************
 * Function    : sr200_dev_write
 *
 * Description : Write Data to sr200 on SPI line
 *
 * Parameters  : filp   :  Device Node  File Pointer
 *               buf    :  Buffer which contains data to be sent to sr200
 *               count  :  Number of bytes to be send
 *               offset :  Pointer to a object that indicates file position
 *                         user is accessing.
 * Returns     : Number of bytes writen if write is success else (-1)
 *               otherwise indicate each error code
 ****************************************************************************/
static ssize_t sr200_dev_write(struct file *filp, const char *buf, size_t count,
                               loff_t *offset) {
  int ret = -1;
  struct sr200_dev *sr200_dev;
  sr200_dev = filp->private_data;
#ifdef DEBUG_LOG
  UWB_LOG_REC("w\n");
#endif
  if (sr200_dev == NULL) {
    SR200_ERR_MSG("%s : sr200_dev is null \n", __func__);
    return ret;
  }
  if (sr200_dev->tx_buffer == NULL ) {
    SR200_ERR_MSG("sr200_dev->tx_buffer is null %s \n", __func__);
    return ret;
  }
  if (count >= SR200_TXBUF_SIZE) {
    SR200_ERR_MSG("%s : write size exceeds\n", __func__);
    ret = -ENOBUFS;
    goto write_end;
  }
  memset(sr200_dev->tx_buffer, 0x00, count + DIRECTIONAL_BYTE_LEN);
  if (copy_from_user(sr200_dev->tx_buffer + DIRECTIONAL_BYTE_LEN, buf, count)) {
    SR200_ERR_MSG("%s : failed to copy from user space \n", __func__);
    return -EFAULT;
  }
  count += DIRECTIONAL_BYTE_LEN; // Including Direction byte
  ret = sr200_dev_transceive(sr200_dev, SR200_WRITE_MODE, count);
  if (ret == spi_transcive_success) {
    ret = sr200_dev->write_count;
  } else {
    SR200_ERR_MSG("write failed......\n");
  }
#ifdef DEBUG_LOG
  UWB_LOG_REC("w %d\n", ret);
#endif
write_end:
  return ret;
}
/******************************************************************************
 * Function    : sr200_dev_read
 *
 * Description : Used to read data from sr200
 *
 * Parameters  : filp   :  Device Node  File Pointer
 *               buf    :  Buffer which contains data to be read from sr200
 *               count  :  Number of bytes to be read
 *               offset :  Pointer to a object that indicates file position
 *                         user is accessing.
 * Returns     : Number of bytes read if read is success else (-1)
 *               otherwise indicate each error code
 ****************************************************************************/
static ssize_t sr200_dev_read(struct file *filp, char *buf, size_t count,
                              loff_t *offset) {
  struct sr200_dev *sr200_dev = filp->private_data;
  int ret = -EIO;
  int retry_count = 0;
  int hdr_length = UCI_HEADER_LEN;
#ifdef DEBUG_LOG
  UWB_LOG_REC("r\n");
#endif
  if(sr200_dev == NULL) {
    SR200_ERR_MSG("sr200_dev is null %s \n", __func__);
    return ret;
  }
  if (sr200_dev->rx_buffer == NULL || sr200_dev->rx_dir_byte_buff == NULL) {
    SR200_ERR_MSG("sr200_dev->rx_buffer is null %s \n", __func__);
    return ret;
  }

  /*500ms timeout in jiffies*/
  sr200_dev->timeout_in_ms = ((500 * HZ) / 2000);
  memset(sr200_dev->rx_buffer, 0x00, SR200_RXBUF_SIZE);
  memset(sr200_dev->rx_dir_byte_buff, 0x00, SR200_TXBUF_SIZE);
  if (!gpio_get_value(sr200_dev->irq_gpio)) {
    if (filp->f_flags & O_NONBLOCK) {
      ret = -EAGAIN;
      return ret;
    }
  }
  /*FW download packet read*/
  if (is_fw_dwnld_enabled) {
    hdr_length = HDLL_MODE_HEADER_LEN;
  }
first_irq_wait:
  sr200_enable_irq(sr200_dev);
  if (!read_abort_requested) {
    ret = wait_event_interruptible(sr200_dev->read_wq, sr200_dev->irq_received);
    if (ret) {
      if (ret != -ERESTARTSYS) {
        SR200_ERR_MSG("read_wq wait_event_interruptible() :%d Failed.\n", ret);
      }
      return ret;
    }
  }
  if (read_abort_requested) {
    read_abort_requested = false;
    SR200_ERR_MSG("abort Read pending......\n");
    return ret;
  }
  mutex_lock(&sr200_dev->sr200_read_count_lock);
  ret = sr200_dev_transceive(sr200_dev, SR200_READ_MODE, count);
  if (ret == spi_transcive_success) {
    sr200_dev->read_count -= DIRECTIONAL_BYTE_LEN;
    if (copy_to_user(buf, &sr200_dev->rx_buffer[1], hdr_length)) {
      SR200_ERR_MSG("sr200_dev_read: copy to user failed for hdr_length\n");
      ret = -EFAULT;
      goto read_end;
    }

    if (sr200_dev->read_count > hdr_length) {
      sr200_dev->read_count -= DIRECTIONAL_BYTE_LEN;
      if (copy_to_user(&buf[hdr_length], &sr200_dev->rx_buffer[hdr_length+2*DIRECTIONAL_BYTE_LEN], (sr200_dev->read_count-hdr_length))) {
        SR200_ERR_MSG("sr200_dev_read: copy to user failed for payload\n");
        ret = -EFAULT;
        goto read_end;
      }
    }
    ret = sr200_dev->read_count;
  } else if (ret == spi_irq_wait_request) {
    SR200_DBG_MSG(" irg is low due to write hence irq is requested again...\n");
    mutex_unlock(&sr200_dev->sr200_read_count_lock);
    goto first_irq_wait;
  } else if (ret == spi_irq_wait_timeout) {
    SR200_DBG_MSG("second irq is not received..time out...\n");
    ret = -1;
  } else {
    SR200_ERR_MSG("spi read failed...%d", ret);
    ret = -1;
  }
#ifdef DEBUG_LOG
  UWB_LOG_REC("r %d\n", ret);
#endif
read_end:
  mutex_unlock(&sr200_dev->sr200_read_count_lock);
  retry_count = 0;
  return ret;
}
/******************************************************************************
 * Function    : sr200_hw_setup
 *
 * Description : Used to read data from sr200
 *
 * Parameters  : platform_data :  struct sr200_spi_platform_data *
 *
 * Returns     : retval 0 if ok else -1 on error
 ****************************************************************************/
static int sr200_hw_setup(struct sr200_spi_platform_data *platform_data) {
  int ret;
  SR200_DBG_MSG("entry : %s\n", __FUNCTION__);

  ret = gpio_request(platform_data->irq_gpio, "sr200 irq");
  if (ret < 0) {
    SR200_ERR_MSG("gpio request failed gpio = 0x%x\n", platform_data->irq_gpio);
    goto fail;
  }
  ret = gpio_direction_input(platform_data->irq_gpio);
  if (ret < 0) {
    SR200_ERR_MSG("gpio request failed gpio = 0x%x\n", platform_data->irq_gpio);
    goto fail_irq;
  }

  ret = gpio_request(platform_data->reset_gpio, "sr200 reset");
  if (ret < 0) {
    SR200_ERR_MSG("gpio request failed gpio = 0x%x\n", platform_data->reset_gpio);
    goto fail_irq;
  }
  ret = gpio_direction_output(platform_data->reset_gpio, 0);
  if (ret < 0) {
    SR200_ERR_MSG("sr200 - failed setting ce gpio - %d\n", platform_data->reset_gpio);
    goto fail_reset;
  }

  if((int)platform_data->ant_connection_status_gpio > 0) {
    ret = gpio_request(platform_data->ant_connection_status_gpio, "sr200 ant connection status");
    if (ret < 0) {
      SR200_ERR_MSG("sr200 - Failed requesting ant connection status gpio - %d\n", platform_data->ant_connection_status_gpio);
      goto fail_reset;
    }
    ret = gpio_direction_input(platform_data->ant_connection_status_gpio);
    if (ret < 0) {
      SR200_ERR_MSG("sr200 - Failed setting ant connection status gpio - %d\n", platform_data->ant_connection_status_gpio);
      goto fail_ant_conn;
    }
  }

//#ifdef CONFIG_UWB_USE_CLK_REQ
  if((int)platform_data->clk_req_gpio > 0) {
    ret = gpio_request(platform_data->clk_req_gpio, "sr200 clk_req");
    if (ret < 0) {
      SR200_ERR_MSG("gpio request failed gpio = 0x%x\n", platform_data->clk_req_gpio);
      goto fail_ant_conn;
    }
    ret = gpio_direction_input(platform_data->clk_req_gpio);
    if (ret < 0) {
      SR200_ERR_MSG("gpio request failed gpio = 0x%x\n", platform_data->clk_req_gpio);
      goto fail_clk_req;
    }
  }
//#endif

  ret = 0;
  SR200_DBG_MSG("Exit : %s\n", __FUNCTION__);
  return ret;

//#ifdef CONFIG_UWB_USE_CLK_REQ
fail_clk_req:
  if((int)platform_data->clk_req_gpio > 0) {
    gpio_free(platform_data->clk_req_gpio);
  }
//#endif
fail_ant_conn:
  if((int)platform_data->ant_connection_status_gpio > 0) {
    gpio_free(platform_data->ant_connection_status_gpio);
  }
fail_reset:
  gpio_free(platform_data->reset_gpio);
fail_irq:
  gpio_free(platform_data->irq_gpio);
fail:
  SR200_ERR_MSG("sr200_hw_setup failed\n");
  return ret;
}
/******************************************************************************
 * Function    : sr200_set_data
 *
 * Description : Set the sr200 device specific context for future use
 *
 * Parameters  : spi :  struct spi_device *
 *               data:  void*
 *
 * Returns     : retval 0 if ok else -1 on error
 ****************************************************************************/
static inline void sr200_set_data(struct spi_device *spi, void *data) {
  dev_set_drvdata(&spi->dev, data);
}
/******************************************************************************
 * Function    : sr200_get_data
 *
 * Description : Get the sr200 device specific context
 *
 * Parameters  : spi :  struct spi_device *
 *
 * Returns     : retval 0 if ok else -1 on error
 ****************************************************************************/
static inline void *sr200_get_data(const struct spi_device *spi) {
  return dev_get_drvdata(&spi->dev);
}
/* possible fops on the sr200 device */
static const struct file_operations sr200_dev_fops = {
    .owner = THIS_MODULE,
    .read = sr200_dev_read,
    .write = sr200_dev_write,
    .open = sr200_dev_open,
    .unlocked_ioctl = sr200_dev_ioctl,
};
/******************************************************************************
 * Function    : sr200_parse_dt
 *
 * Description : Parse the dtsi configartion
 *
 * Parameters  : dev :  struct spi_device *
 *               pdata: Ponter to platform data
 *
 * Returns     : retval 0 if ok else -1 on error
 ****************************************************************************/
static int sr200_parse_dt(struct device *dev,
                          struct sr200_spi_platform_data *pdata) {
  struct device_node *np = dev->of_node;
  SR200_DBG_MSG("sr200 - %s\n", __FUNCTION__);

  pdata->irq_gpio = of_get_named_gpio(np, "nxp,sr200-irq", 0);
  if (!gpio_is_valid(pdata->irq_gpio)) {
    return -EINVAL;
  }

  pdata->reset_gpio = of_get_named_gpio(np, "nxp,sr200-reset", 0);
  if (!gpio_is_valid(pdata->reset_gpio)) {
    return -EINVAL;
  }

  pdata->ant_connection_status_gpio = of_get_named_gpio(np, "nxp,sr200-ant-connection-status", 0);
  if (!gpio_is_valid(pdata->ant_connection_status_gpio)) {
    SR200_ERR_MSG("nxp,sr100-ant-connection-status not found\n");
    // return -EINVAL;
  }

//#ifdef CONFIG_UWB_USE_CLK_REQ
  pdata->clk_req_gpio = of_get_named_gpio(np, "nxp,sr200-clk_req", 0);
  if (!gpio_is_valid(pdata->clk_req_gpio)) {
    SR200_ERR_MSG("nxp,sr200-clk_req not found\n");
    // return -EINVAL;
  }
//#endif

  if (of_property_read_string(np, "nxp,vdd-io", &pdata->uwb_vdd_io) < 0) {
    SR200_ERR_MSG("uwb_vdd_io not found\n");
    pdata->uwb_vdd_io = NULL;
  } else {
    SR200_ERR_MSG("uwb_vdd_io :%s\n", pdata->uwb_vdd_io);
  }

  if (of_property_read_string(np, "nxp,vdd-sw", &pdata->uwb_vdd_sw) < 0) {
    SR200_ERR_MSG("uwb_vdd_sw not found\n");
    pdata->uwb_vdd_sw = NULL;
  } else {
    SR200_ERR_MSG("uwb_vdd_sw :%s\n", pdata->uwb_vdd_sw);
  }

  SR200_DBG_MSG("sr200 : irq_gpio = %d, reset_gpio = %d \n", pdata->irq_gpio,
          pdata->reset_gpio);
  SR200_DBG_MSG("sr200 : ant_connection_status_gpio = %d \n", pdata->ant_connection_status_gpio);
//#ifdef CONFIG_UWB_USE_CLK_REQ
  SR200_DBG_MSG("sr200 : clk_req_gpio = %d\n", pdata->clk_req_gpio);
//#endif
  return 0;
}

static int sr200_regulator_onoff(struct device *dev, struct sr200_dev* pdev, bool onoff)
{
  int rc = 0;
  struct regulator *regulator_uwb_vdd_io = NULL, *regulator_uwb_vdd_sw = NULL;

  if(pdev->uwb_vdd_io != NULL) {
    regulator_uwb_vdd_io = regulator_get(dev, "nxp,vdd-io");
    if (IS_ERR(regulator_uwb_vdd_io) || regulator_uwb_vdd_io == NULL) {
      SR200_ERR_MSG("regulator_uwb_vdd_io regulator_get fail\n");
      return -ENODEV;
    }
  } else {
    regulator_uwb_vdd_io = NULL;
    SR200_ERR_MSG("regulator_uwb_vdd_io not support\n");
  }

  if(pdev->uwb_vdd_sw != NULL) {
    regulator_uwb_vdd_sw = regulator_get(dev, "nxp,vdd-sw");
    if (IS_ERR(regulator_uwb_vdd_sw) || regulator_uwb_vdd_sw == NULL) {
      SR200_ERR_MSG("regulator_uwb_vdd_sw regulator_get fail\n");
      return -ENODEV;
    }
  } else {
    regulator_uwb_vdd_sw = NULL;
    SR200_ERR_MSG("regulator_uwb_vdd_sw not support\n");
  }

  SR200_DBG_MSG("sr200_regulator_onoff  = %d\n", onoff);
  if (onoff == true) {
    if(regulator_uwb_vdd_io != NULL) {
      rc = regulator_set_load(regulator_uwb_vdd_io, 400000);
      if (rc) {
        SR200_ERR_MSG("regulator_uwb_vdd_io set_load failed, rc=%d\n", rc);
        goto regulator_err;
      }
      regulator_set_voltage(regulator_uwb_vdd_io, 1800000, 1800000);
      rc = regulator_enable(regulator_uwb_vdd_io);
      if (rc) {
        SR200_ERR_MSG("regulator_uwb_vdd_io enable failed, rc=%d\n", rc);
        goto regulator_err;
      }
    }
    if(regulator_uwb_vdd_sw != NULL) {
      rc = regulator_enable(regulator_uwb_vdd_sw);
      if (rc) {
        SR200_ERR_MSG("regulator_uwb_vdd_sw enable failed, rc=%d\n", rc);
        goto regulator_err;
      }
    }
  } else {
    if(regulator_uwb_vdd_io != NULL) {
      rc = regulator_disable(regulator_uwb_vdd_io);
      if (rc) {
        SR200_ERR_MSG("regulator_uwb_vdd_io disable failed, rc=%d\n", rc);
        goto regulator_err;
      }
    }
    if(regulator_uwb_vdd_sw != NULL) {
      rc = regulator_disable(regulator_uwb_vdd_sw);
      if (rc) {
        SR200_ERR_MSG("regulator_uwb_vdd_sw disable failed, rc=%d\n", rc);
        goto regulator_err;
      }
    }
regulator_err:
    if(regulator_uwb_vdd_io != NULL) regulator_put(regulator_uwb_vdd_io);
    if(regulator_uwb_vdd_sw != NULL) regulator_put(regulator_uwb_vdd_sw);
  }

  return rc;
}

/******************************************************************************
 * Function    : sr200_probe
 *
 * Description : To probe for sr200 SPI interface. If found initialize the SPI
 *               clock,bit rate & SPI mode. It will create the dev entry
 *               (sr200) for user space.
 * Parameters  : spi :  struct spi_device *
 *
 * Returns     : retval 0 if ok else -1 on error
 ****************************************************************************/
static int sr200_probe(struct spi_device *spi) {
  int ret;
  struct sr200_spi_platform_data *platform_data = NULL;
  struct sr200_spi_platform_data platform_data1;
  struct sr200_dev *sr200_dev = NULL;
  unsigned int irq_flags;

  SR200_DBG_MSG("%s chip select : %d , bus number = %d \n", __FUNCTION__,
                spi->chip_select, spi->master->bus_num);
  ret = sr200_parse_dt(&spi->dev, &platform_data1);
  if (ret) {
    SR200_ERR_MSG("%s - failed to parse DT\n", __func__);
    goto err_exit;
  }
  platform_data = &platform_data1;
  sr200_dev = kzalloc(sizeof(*sr200_dev), GFP_KERNEL);
  if (sr200_dev == NULL) {
    SR200_ERR_MSG("failed to allocate memory for module data\n");
    ret = -ENOMEM;
    goto err_exit;
  }
  ret = sr200_hw_setup(platform_data);
  if (ret < 0) {
    SR200_ERR_MSG("failed to sr200_enable_SR200_IRQ_ENABLE\n");
    goto err_exit0;
  }
  spi->bits_per_word = 8;
  spi->mode = SPI_MODE_0;
  spi->max_speed_hz = SR200_SPI_CLOCK;
  ret = spi_setup(spi);
  if (ret < 0) {
    SR200_ERR_MSG("failed to do spi_setup()\n");
    goto err_exit0;
  }
  sr200_dev->spi = spi;
  sr200_dev->sr200_device.minor = MISC_DYNAMIC_MINOR;
  sr200_dev->sr200_device.name = "sr200";
  sr200_dev->sr200_device.fops = &sr200_dev_fops;
  sr200_dev->sr200_device.parent = &spi->dev;
  sr200_dev->irq_gpio = platform_data->irq_gpio;
//#ifdef CONFIG_UWB_USE_CLK_REQ
  sr200_dev->clk_req_gpio = platform_data->clk_req_gpio;
//#endif
  sr200_dev->reset_gpio = platform_data->reset_gpio;
  sr200_dev->ant_connection_status_gpio = platform_data->ant_connection_status_gpio;
  sr200_dev->uwb_vdd_io = platform_data->uwb_vdd_io;
  sr200_dev->uwb_vdd_sw = platform_data->uwb_vdd_sw;
  sr200_dev->tx_buffer = kzalloc(SR200_TXBUF_SIZE, GFP_KERNEL);
  sr200_dev->rx_buffer = kzalloc(SR200_RXBUF_SIZE, GFP_KERNEL);
  sr200_dev->rx_dir_byte_buff = kzalloc(SR200_RXBUF_SIZE, GFP_KERNEL);
  if (sr200_dev->tx_buffer == NULL) {
    ret = -ENOMEM;
    goto exit_free_dev;
  }
  if (sr200_dev->rx_buffer == NULL) {
    ret = -ENOMEM;
    goto exit_free_dev;
  }
  if (sr200_dev->rx_dir_byte_buff == NULL) {
    ret = -ENOMEM;
    goto exit_free_dev;
  }
  dev_set_drvdata(&spi->dev, sr200_dev);
  /* init mutex and queues */
  init_waitqueue_head(&sr200_dev->read_wq);
  mutex_init(&sr200_dev->sr200_access_lock);
  mutex_init(&sr200_dev->sr200_read_count_lock);

  spin_lock_init(&sr200_dev->irq_enabled_lock);
  ret = misc_register(&sr200_dev->sr200_device);
  if (ret < 0) {
    SR200_ERR_MSG("misc_register failed! %d\n", ret);
    goto err_exit0;
  }

  ret = sr200_regulator_onoff(&spi->dev, sr200_dev, true);
  if (ret < 0) {
    SR200_ERR_MSG("regulator_on fail err:%d\n", ret);
  }
  usleep_range(1000, 1100);

  sr200_dev->spi->irq = gpio_to_irq(platform_data->irq_gpio);
  if (sr200_dev->spi->irq < 0) {
    SR200_ERR_MSG("gpio_to_irq request failed gpio = 0x%x\n",
                  platform_data->irq_gpio);
    goto err_exit1;
  }
  SR200_DBG_MSG("%s : spi->irq = %d\n", __FUNCTION__, sr200_dev->spi->irq);

//#ifdef CONFIG_UWB_USE_CLK_REQ
  if((int)platform_data->clk_req_gpio > 0) {
    sr200_dev->clk_req_irq = gpio_to_irq(platform_data->clk_req_gpio);
    if (sr200_dev->clk_req_irq < 0) {
      SR200_ERR_MSG("clk_req_gpio request failed gpio = 0x%x\n",
                    platform_data->clk_req_gpio);
      goto err_exit1;
    }
    SR200_DBG_MSG("%s : clk_req_irq = %d\n", __FUNCTION__, sr200_dev->clk_req_irq);
  }
//#endif

  /* request irq.  the irq is set whenever the chip has data available
   * for reading.  it is cleared when all data has been read.
   */
  // irq_flags = IRQF_TRIGGER_RISING;
  irq_flags = IRQ_TYPE_LEVEL_HIGH;
  sr200_dev->irq_enabled = true;
  sr200_dev->irq_received = false;
  ret = request_irq(sr200_dev->spi->irq, sr200_dev_irq_handler, irq_flags,
                    sr200_dev->sr200_device.name, sr200_dev);
  if (ret) {
    SR200_ERR_MSG("request_irq for spi irq failed\n");
    goto err_exit1;
  } else {
    sr200_disable_irq(sr200_dev);
    device_set_wakeup_capable(&sr200_dev->spi->dev, true);
    device_wakeup_disable(&sr200_dev->spi->dev);
  }

  SR200_DBG_MSG("exit : %s\n", __FUNCTION__);
  return ret;

err_exit1:
exit_free_dev:
  if (sr200_dev != NULL) {
    if (sr200_dev->tx_buffer) {
      kfree(sr200_dev->tx_buffer);
    }
    if (sr200_dev->rx_buffer) {
      kfree(sr200_dev->rx_buffer);
    }
    if (sr200_dev->rx_dir_byte_buff) {
      kfree(sr200_dev->rx_dir_byte_buff);
    }
    misc_deregister(&sr200_dev->sr200_device);
  }
err_exit0:
  if (sr200_dev != NULL) {
    mutex_destroy(&sr200_dev->sr200_access_lock);
    mutex_destroy(&sr200_dev->sr200_read_count_lock);
  }
err_exit:
  if (sr200_dev != NULL)
    kfree(sr200_dev);
  SR200_DBG_MSG("ERROR: exit : %s ret %d\n", __FUNCTION__, ret);
  return ret;
}

/******************************************************************************
 * Function    : sr200_remove
 *
 * Description : Will get called when the device is removed to release the
 *                 resources.
 *
 * Parameters  : spi :  struct spi_device *
 *
 * Returns     : retval 0 if ok else -1 on error
 ****************************************************************************/
#if KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE
static void sr200_remove(struct spi_device *spi) {
#else
static int sr200_remove(struct spi_device *spi) {
#endif
  struct sr200_dev *sr200_dev = sr200_get_data(spi);
  SR200_DBG_MSG("entry : %s\n", __FUNCTION__);
  if (sr200_dev != NULL) {
    device_wakeup_disable(&sr200_dev->spi->dev);
    sr200_regulator_onoff(&spi->dev, sr200_dev, false);
    gpio_free(sr200_dev->reset_gpio);
    mutex_destroy(&sr200_dev->sr200_access_lock);
    mutex_destroy(&sr200_dev->sr200_read_count_lock);
//#ifdef CONFIG_UWB_USE_CLK_REQ
    if((int)sr200_dev->clk_req_gpio > 0) {
      gpio_free(sr200_dev->clk_req_gpio);
    }
//#endif
    free_irq(sr200_dev->spi->irq, sr200_dev);
    gpio_free(sr200_dev->irq_gpio);
    if((int)sr200_dev->ant_connection_status_gpio > 0) {
      gpio_free(sr200_dev->ant_connection_status_gpio);
    }
    misc_deregister(&sr200_dev->sr200_device);

    if (sr200_dev->tx_buffer != NULL)
      kfree(sr200_dev->tx_buffer);
    if (sr200_dev->rx_buffer != NULL)
      kfree(sr200_dev->rx_buffer);
    if(sr200_dev->rx_dir_byte_buff != NULL)
      kfree(sr200_dev->rx_dir_byte_buff);
    kfree(sr200_dev);
  }
  SR200_DBG_MSG("exit : %s\n", __FUNCTION__);
#if KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE
  return;
#else
  return 0;
#endif
}

/**
 * sr200_dev_resume
 *
 * Executed after waking the system up from a sleep state
 *
 */
int sr200_dev_resume(struct device *dev)
{
    struct sr200_dev *sr200_dev = dev_get_drvdata(dev);

    if (device_may_wakeup(dev)) {
        disable_irq_wake(sr200_dev->spi->irq);
    }

    return 0;
}

/**
 * sr200_dev_suspend
 *
 * Executed before putting the system into a sleep state
 *
 */
int sr200_dev_suspend(struct device *dev)
{
    struct sr200_dev *sr200_dev = dev_get_drvdata(dev);

    if (device_may_wakeup(dev)) {
        enable_irq_wake(sr200_dev->spi->irq);
    }

    return 0;
}

static struct of_device_id sr200_dt_match[] = {{
                                                   .compatible = "nxp,sr200",
                                               },
                                               {}};

static const struct dev_pm_ops sr200_dev_pm_ops = { SET_SYSTEM_SLEEP_PM_OPS(
        sr200_dev_suspend, sr200_dev_resume) };

static struct spi_driver sr200_driver = {
    .driver =
        {
            .name = "sr200",
            .pm = &sr200_dev_pm_ops,
            .bus = &spi_bus_type,
            .owner = THIS_MODULE,
            .of_match_table = sr200_dt_match,
        },
    .probe = sr200_probe,
    .remove = (sr200_remove),
};
/******************************************************************************
 * Function    : sr200_dev_init
 *
 * Description : Module init interface
 *
 * Parameters  :void
 *
 * Returns     : returns handle
 ****************************************************************************/
static int __init sr200_dev_init(void) {
  debug_level = SR200_FULL_DEBUG;
  uwb_logger_init();
  SR200_DBG_MSG("entry : %s\n", __FUNCTION__);
  return spi_register_driver(&sr200_driver);
}
module_init(sr200_dev_init);
/******************************************************************************
 * Function    : sr200_dev_exit
 *
 * Description : Module Exit interface
 *
 * Parameters  :void
 *
 * Returns     : returns void
 ****************************************************************************/
static void __exit sr200_dev_exit(void) {
  SR200_DBG_MSG("entry : %s\n", __FUNCTION__);
  spi_unregister_driver(&sr200_driver);
  SR200_DBG_MSG("exit : %s\n", __FUNCTION__);
}
module_exit(sr200_dev_exit);
MODULE_AUTHOR("Manjunatha Venkatesh");
MODULE_DESCRIPTION("NXP SR200 SPI driver");
MODULE_LICENSE("GPL");
/** @} */
