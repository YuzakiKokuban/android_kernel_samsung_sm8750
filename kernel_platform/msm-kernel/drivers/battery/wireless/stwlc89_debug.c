#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/crc32.h>
#include "stwlc89_charger.h"
#include "stwlc89_debug.h"

#define CMD_WRITE					0x01
#define CMD_WRITEREAD				0x02
#define CMD_UPDATE_FIRMWARE         0x09

#define STWLC_MAX_FW_SIZE           0x4020
#define STWLC_MAX_PACKET_SIZE       0x100

/* FIRST LEVEL ERROR CODE */
/** @defgroup first_level	First Level Error Code
  * @ingroup error_codes
  * Errors related to low level operation which are not under control of driver,
  * such as: communication protocol (I2C/SPI), timeout, file operations ...
  * @{
  */
#define OKOK			    ((u8)0x00)	/* /< No ERROR */
#define ERROR_ALLOC		    ((u8)0x01)	/* /< allocation of memory failed */
#define ERROR_BUS_R		    ((u8)0x02)	/* /< i2c/spi read failed */
#define ERROR_BUS_W		    ((u8)0x03)	/* /< i2c/spi write failed */
#define ERROR_BUS_WR	    ((u8)0x04)	/* /< i2c/spi write/read failed */
#define ERROR_BUS_O		    ((u8)0x05)	/* /< error during opening an i2c device */
#define ERROR_OP_NOT_ALLOW	((u8)0x06)	/* /< operation not allowed */
#define ERROR_TIMEOUT		((u8)0x07)	/* /< timeout expired! exceed the max number of retries or the max waiting time */
#define ERROR_WRONG_IDX		((u8)0x08)
#define ERROR_UPADTE_FW     ((u8)0x09)
#define ERROR_CRC32_FW      ((u8)0x0A)
#define ERROR_WRONG_FW      ((u8)0x0B)
#define ERROR_OVERFLOW      ((u8)0x0C)

#define CHUNK_PROC				1024

static int limit;	/* /< store the amount of data to print into the shell*/
static int chunk;	/* /< store the chuk of data that should be printed in this iteration */
static int printed;	/* /< store the amount of data already printed in the shell */
static u8 *output_buff;	/* /< pointer to an array of bytes used to store the result of the function executed */
static struct mfc_charger_data *Charger;
static struct proc_dir_entry *stwlc_dir;	/* /< reference to the directory stwlc under /proc */
char buf_chunk[CHUNK_PROC] = { 0 };	/* /< buffer used to store the message info received */

static int stwlc_i2c_read(struct i2c_client *client, u8 *cmd, int cmd_length,
			u8 *read_data, int read_count)
{
	struct i2c_msg msg[2];
	int err;

	msg[0].addr = client->addr;
	msg[0].buf = cmd;
	msg[0].len = cmd_length;
	msg[0].flags = 0;

	msg[1].addr = client->addr;
	msg[1].buf = read_data;
	msg[1].len = read_count;
	msg[1].flags = I2C_M_RD;

	err = i2c_transfer(client->adapter, msg, 2);
	if (err < OK) {
		pr_err("%s mfc, i2c transfer failed! err: %d\n", __func__, err);
		return err;
	}

	return OK;
}

static int stwlc_i2c_write(struct i2c_client *client, u8 *cmd, int cmd_length)
{
	struct i2c_msg msg[1];
	int err;

	msg[0].addr = client->addr;
	msg[0].buf = cmd;
	msg[0].len = cmd_length;
	msg[0].flags = 0;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err < OK) {
		pr_err("%s mfc, i2c transfer failed! err: %d\n", __func__, err);
		return err;
	}

	return OK;
}

/************************ SEQUENTIAL FILE UTILITIES **************************/
/**
  * This function is called at the beginning of the stream to a sequential file
  * or every time into the sequential were already written PAGE_SIZE bytes and
  * the stream need to restart
  * @param s pointer to the sequential file on which print the data
  * @param pos pointer to the offset where write the data
  * @return NULL if there is no data to print or the pointer to the beginning of
  * the data that need to be printed
  */
static void *stwlc89_debug_seq_start(struct seq_file *s, loff_t *pos)
{
	if (output_buff == NULL && *pos == 0) {
		pr_info("%s: No data to print!\n", __func__);

		output_buff = (u8 *)kzalloc(5 * sizeof(u8), GFP_KERNEL);
		snprintf(output_buff, 6, "{%.2X}\n", ERROR_OP_NOT_ALLOW);

		limit = strlen(output_buff);
	} else {
		if (*pos != 0)
			*pos += chunk - 1;

		if (*pos >= limit)
			return NULL;
	}

	chunk = CHUNK_PROC;
	if (limit - *pos < CHUNK_PROC)
		chunk = limit - *pos;

	memset(buf_chunk, 0, CHUNK_PROC);
	memcpy(buf_chunk, &output_buff[(int)*pos], chunk);

	return buf_chunk;
}

/**
  * This function actually print a chunk amount of data in the sequential file
  * @param s pointer to the sequential file where to print the data
  * @param v pointer to the data to print
  * @return 0
  */
static int stwlc89_debug_seq_show(struct seq_file *s, void *v)
{
	seq_write(s, (u8 *)v, chunk);
	printed += chunk;
	return 0;
}

/**
  * This function update the pointer and the counters to the next data to be
  * printed
  * @param s pointer to the sequential file where to print the data
  * @param v pointer to the data to print
  * @param pos pointer to the offset where write the next data
  * @return NULL if there is no data to print or the pointer to the beginning of
  * the next data that need to be printed
  */
static void *stwlc89_debug_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos) += chunk;	/* increase my position counter */
	chunk = CHUNK_PROC;

	if (*pos >= limit)	/* are we done? */
		return NULL;
	else if (limit - *pos < CHUNK_PROC)
		chunk = limit - *pos;

	memset(buf_chunk, 0, CHUNK_PROC);
	memcpy(buf_chunk, &output_buff[(int)*pos], chunk);
	return buf_chunk;
}

/**
  * This function is called when there are no more data to print  the stream
  *need to be terminated or when PAGE_SIZE data were already written into the
  *sequential file
  * @param s pointer to the sequential file where to print the data
  * @param v pointer returned by fts_seq_next
  */
static void stwlc89_debug_seq_stop(struct seq_file *s, void *v)
{
	if (v) {
		/* pr_info("%s %s: v is %X.\n", tag, __func__, v); */
	} else {
		/* pr_info("%s %s: v is null.\n", tag, __func__); */
		limit = 0;
		chunk = 0;
		printed = 0;
		if (output_buff != NULL) {
			kfree(output_buff);
			output_buff = NULL;
		}
	}
}

/**
  * Struct where define and specify the functions which implements the flow for
  *writing on a sequential file
  */
static const struct seq_operations debug_seq_ops = {
	.start	= stwlc89_debug_seq_start,
	.next	= stwlc89_debug_seq_next,
	.stop	= stwlc89_debug_seq_stop,
	.show	= stwlc89_debug_seq_show
};

/**
  * This function open a sequential file
  * @param inode Inode in the file system that was called and triggered this
  * function
  * @param file file associated to the file node
  * @return error code, 0 if success
  */
static int stwlc89_debug_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &debug_seq_ops);
};

static int fw_size;
static u8 *fw_pos;
static u8 fw_idx;
static u8 *firm_data_bin;
static u8 flag_start_fw_update;

static void mfc_uno_on(struct mfc_charger_data *charger, bool on)
{
	union power_supply_propval value = {0, };

	if (charger->wc_tx_enable && on) { /* UNO ON */
		value.intval = SEC_BAT_CHG_MODE_UNO_ONLY;
		psy_do_property("otg", set,
			POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
		pr_info("%s: UNO ON\n", __func__);
	} else if (on) { /* UNO ON */
		value.intval = 1;
		psy_do_property("otg", set,
			POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
		pr_info("%s: UNO ON\n", __func__);
	} else { /* UNO OFF */
		value.intval = 0;
		psy_do_property("otg", set,
			POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
		pr_info("%s: UNO OFF\n", __func__);
	}
}

extern int PgmOTPwRAM_STM(struct mfc_charger_data *charger, unsigned short OtpAddr,
					  const u8 *srcData, int srcOffs, int size);

static u8 update_stwlc_firmware(void) {
	u8 res = OKOK;
	u8 fver[4];
	u8 cmd[2] = {0x00, MFC_FW_MAJOR_REV_L_REG};

	mfc_uno_on(Charger, true);
	msleep(200);

	disable_irq(Charger->pdata->irq_wpc_int);
	disable_irq(Charger->pdata->irq_wpc_det);

	if (Charger->pdata->irq_wpc_pdrc)
		disable_irq(Charger->pdata->irq_wpc_pdrc);
	if (Charger->pdata->irq_wpc_pdet_b)
		disable_irq(Charger->pdata->irq_wpc_pdet_b);
	__pm_stay_awake(Charger->wpc_update_ws);
	pr_info("%s data size = %d\n", __func__, fw_size);

	if (PgmOTPwRAM_STM(Charger, 0, firm_data_bin, 0, fw_size) != MFC_FWUP_ERR_SUCCEEDED) {
		res = ERROR_UPADTE_FW;
	} else {
		if (stwlc_i2c_read(Charger->client, cmd, 2, fver, 4) == OK) {
			Charger->otp_firmware_ver = fver[0] | (fver[1] << 8);
			Charger->pdata->wc_ic_rev = fver[2] | (fver[3] << 8);
		}
	}

	mfc_uno_on(Charger, false);

	enable_irq(Charger->pdata->irq_wpc_int);
	enable_irq(Charger->pdata->irq_wpc_det);

	if (Charger->pdata->irq_wpc_pdrc)
		enable_irq(Charger->pdata->irq_wpc_pdrc);
	if (Charger->pdata->irq_wpc_pdet_b)
		enable_irq(Charger->pdata->irq_wpc_pdet_b);
	__pm_relax(Charger->wpc_update_ws);

	return res;
}
/**
  * Receive the OP code and the inputs from shell when the file node is called,
  * parse it and then execute the corresponding function
  * echo cmd+parameters > /proc/stwlc/debug to execute the select command
  * cat /proc/stwlc/debug			to obtain the result into the shell
  * the string returned in the shell is made up as follow:
  * { = start byte
  * the answer content and format strictly depend on the cmd executed. In
  * general can be: an HEX string or a byte array (e.g in case of 0xF- commands)
  * } = end byte
  */
static ssize_t stwlc89_debug_write(struct file *file, const char __user *buf,
									 size_t count, loff_t *pos)
{
	u8 res = OKOK;
	char *pbuf = NULL;
	char *p = NULL;
	u8 *cmd = NULL;
	u32 temp;
	int number_param = 0;
	int cmd_length;
	int read_count = 0;
	u8 *read_buf = NULL;
	int size = 0;
	int index = 0;
	int i;
	int packet_size;
	u32 checksum;

	if (count == 0) {
		pr_info("%s Error! The count is 0\n", __func__);
		return 0;
	}

	pbuf = kzalloc(count * sizeof(u8), GFP_KERNEL);
	if (pbuf == NULL) {
		pr_info("%s Error allocating memory for pbuf\n", __func__);
		res = ERROR_ALLOC;
		goto goto_end;
	}

	cmd = kmalloc(count * sizeof(u8), GFP_KERNEL);
	if (cmd == NULL) {
		pr_info("%s Error allocating memory for cmd\n", __func__);
		res = ERROR_ALLOC;
		goto goto_end;
	}

	if (access_ok(buf, count) < OK ||
		copy_from_user(pbuf, buf, count) != 0) {
		res = ERROR_ALLOC;
		goto goto_end;
	}

	p = pbuf;
	if ((count / 2) >= 1) {
		if (sscanf(p, "%02X", &temp) == 1) {
			p += 2;
			cmd[0] = (u8)temp;
			number_param = 1;
		}
	} else {
		res = ERROR_OP_NOT_ALLOW;
		goto goto_end;
	}

	for (; number_param < (count / 2); number_param++) {
		if (sscanf(p, "%02X", &temp) == 1) {
			p += 2;
			cmd[number_param] =	(u8)temp;
		}
	}

	if (number_param >= 1) {
		switch (cmd[0]) {
		case CMD_UPDATE_FIRMWARE:
			if (cmd[1] == 0) {
				if (number_param <= 12) {
					pr_info("%s Error data size[%d] is wrong!!\n", __func__, number_param);
					res = ERROR_WRONG_FW;
					break;
				}

				fw_size = *(int *)&cmd[2];
				packet_size = *(u16 *)&cmd[6];

				if (fw_size > STWLC_MAX_FW_SIZE) {
					pr_info("%s Error FW size has been exceeded!!\n", __func__);
					res = ERROR_WRONG_FW;
					break;
				}

				if (packet_size > STWLC_MAX_PACKET_SIZE) {
					pr_info("%s Error PACKET size has been exceeded!!\n", __func__);
					res = ERROR_WRONG_FW;
					break;
				}

				if (packet_size > fw_size) {
					pr_info("%s Error packet size exceeds fw size\n", __func__);
					res = ERROR_OVERFLOW;
					break;
				}

				if ((number_param-8) != packet_size) {
					pr_info("%s Error packet size[%d] is wrong!!\n", __func__, (number_param-8));
					res = ERROR_WRONG_FW;
					break;
				}

				if (*(u32 *)&cmd[8] != 0x34890556) {
					pr_info("%s Error file signature is NOT correct [0x%.8X]!!\n",
						__func__, *(u32 *)&cmd[8]);
					res = ERROR_WRONG_FW;
					break;
				}

				if (firm_data_bin == NULL) {
					firm_data_bin = kmalloc(STWLC_MAX_FW_SIZE, GFP_KERNEL);
					if (firm_data_bin == NULL) {
						pr_info("%s Error allocating memory for firm_data_bin\n", __func__);
						res = ERROR_ALLOC;
						break;
					}
				}

				fw_pos = firm_data_bin;
				memcpy(fw_pos, &cmd[8], packet_size);
				fw_pos += packet_size;
				fw_idx = 1;
				flag_start_fw_update = 1;
			} else if (cmd[1] == fw_idx) {
				size_t remaining_size;

				if (number_param <= 4) {
					pr_info("%s Error data size[%d] is wrong!!\n", __func__, number_param);
					res = ERROR_WRONG_FW;
					break;
				}

				if (firm_data_bin == NULL) {
					pr_info("%s Error allocating memory for firm_data_bin\n", __func__);
					res = ERROR_ALLOC;
					break;
				}
				packet_size = *(u16 *)&cmd[2];

				if (packet_size > STWLC_MAX_PACKET_SIZE) {
					pr_info("%s Error PACKET size has been exceeded!!\n", __func__);
					res = ERROR_WRONG_FW;
					break;
				}

				if ((number_param-4) != packet_size) {
					pr_info("%s Error packet size[%d] is wrong!!\n", __func__, (number_param-4));
					res = ERROR_WRONG_FW;
					break;
				}

				remaining_size = STWLC_MAX_FW_SIZE - (fw_pos - firm_data_bin);
				if (packet_size > remaining_size) {
					pr_info("%s Not enough space in firm_data_bin for packet_size\n", __func__);
					res = ERROR_OVERFLOW;
					kfree(firm_data_bin); // Clean up allocated memory if necessary
					break;
				}
				memcpy(fw_pos, &cmd[4], packet_size);
				fw_pos += packet_size;
				fw_idx++;
			} else if (cmd[1] == 0xFF) {
				if (firm_data_bin == NULL) {
					pr_info("%s Error allocating memory for firm_data_bin\n", __func__);
					res = ERROR_ALLOC;
					break;
				}

				if (number_param != 6) {
					pr_info("%s Error data size[%d] is wrong!!\n", __func__, number_param);
					res = ERROR_WRONG_FW;
					break;
				}

				checksum = crc32(0x80000000, firm_data_bin, fw_size);
				if (checksum != *(u32 *)&cmd[2]) {
					pr_info("%s Error CRC is NOT same 0x%.8X<>0x%.8X\n",
						__func__, checksum, *(u32 *)&cmd[2]);
					res = ERROR_CRC32_FW;
					break;
				}

				res = update_stwlc_firmware();

				if ((firm_data_bin != NULL) && (flag_start_fw_update == 1)) {
					kfree(firm_data_bin);
					firm_data_bin = NULL;
				}
			} else {
				if ((firm_data_bin != NULL) && (flag_start_fw_update == 1)) {
					kfree(firm_data_bin);
					firm_data_bin = NULL;
				}
				pr_info("%s Error wrong index for firm_data_bin\n", __func__);
				res = ERROR_WRONG_IDX;

				flag_start_fw_update = 0;
				break;
			}
			break;
		case CMD_WRITE:
			cmd_length = number_param - 1;
			if (stwlc_i2c_write(Charger->client, &cmd[1], cmd_length) < OK) {
				pr_err("%s, Error in writing Hardware I2c!\n", __func__);
				res = ERROR_BUS_WR;
				break;
			}
			break;

		case CMD_WRITEREAD:
			if (number_param >= 4) {
				read_count = cmd[1] + (cmd[2]<<8);
				cmd_length = number_param - 3;

				read_buf = kmalloc((read_count + 1) *
										sizeof(u8), GFP_KERNEL);
				if (read_buf == NULL) {
					pr_info("%s Error allocating memory for read_buf\n", __func__);
					res = ERROR_ALLOC;
					break;
				}

				if ((stwlc_i2c_read(Charger->client, &cmd[3], cmd_length, read_buf, read_count)) < OK) {
					pr_err("%s, Error in writing Hardware I2c!\n", __func__);
					res = ERROR_BUS_WR;
					break;
				} else {
					size += (read_count * sizeof(u8));
				}
			} else {
				pr_info("%s wrong number of parameters for CMD_WRITEREAD\n", __func__);
				res = ERROR_OP_NOT_ALLOW;
			}
			break;

		default:
			pr_info("%s COMMAND ID NOT VALID!!!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
	}

goto_end:
	size += 2;
	size *= 2;
	size += 1;

	output_buff = kmalloc(size * sizeof(u8), GFP_KERNEL);
	if (output_buff == NULL) {
		pr_info("%s Error allocating memory for output_buff\n", __func__);
		res = ERROR_ALLOC;
	}

	snprintf(&output_buff[index], 2, "{");
	index += 1;
	snprintf(&output_buff[index], 3, "%.2X", res);
	index += 2;
	if (res == OKOK) {
		switch (cmd[0]) {
		case CMD_WRITEREAD:
			for (i = 0; i < read_count; i++) {
				snprintf(&output_buff[index], 3, "%.2X", read_buf[i]);
				index += 2;
			}
			break;
		}
	}

	snprintf(&output_buff[index], 3, "}\n");
	limit = size;
	printed = 0;

	if (read_buf != NULL) {
		kfree(read_buf);
		read_buf = NULL;
	}

	if (cmd != NULL) {
		kfree(cmd);
		cmd = NULL;
	}

	if (pbuf != NULL) {
		kfree(pbuf);
		pbuf = NULL;
	}

	return count;
}

/**
  * file_operations struct which define the functions for the canonical
  *operation on a device file node (open. read, write etc.)
  */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
static const struct proc_ops stwlc89_debug_ops = {
	.proc_open	= stwlc89_debug_open,
	.proc_read	= seq_read,
	.proc_write	= stwlc89_debug_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= seq_release
};
#else
static const struct file_operations stwlc89_debug_ops = {
	.open		= stwlc89_debug_open,
	.read		= seq_read,
	.write		= stwlc89_debug_write,
	.llseek		= seq_lseek,
	.release	= seq_release
};
#endif

int stwlc89_debug_proc_init(struct mfc_charger_data *charger) {
	int retval = 0;
	struct proc_dir_entry *entry;

	Charger = charger;

	stwlc_dir = proc_mkdir_data("stwlc", 0777, NULL, NULL);
	if (stwlc_dir == NULL) {	/* directory creation failed */
		retval = -ENOMEM;
		goto out;
	}

	entry = proc_create("debug", 0777, stwlc_dir, &stwlc89_debug_ops);
	if (entry)
		pr_info("%s: proc entry CREATED!\n", __func__);
	else {
		pr_info("%s : error creating proc entry!\n", __func__);
		retval = -ENOMEM;
		goto badfile;
	}
	return OK;

badfile:
	remove_proc_entry("stwlc", NULL);
out:
	return retval;
}

/**
  * Delete and Clean from the file system, all the references to the driver test
  * file node
  * @return OK
  */
int stwlc89_debug_proc_remove(void)
{
	remove_proc_entry("debug", stwlc_dir);
	remove_proc_entry("stwlc", NULL);
	return OK;
}