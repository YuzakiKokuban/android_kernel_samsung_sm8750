/*
 *  common_i2c.c - Linux kernel modules for sensortek stk6d2x
 *  ambient light sensor (Common function)
 *
 *  Copyright (C) 2019 Bk, sensortek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/pm.h>
#include <common_define.h>

#define MAX_I2C_MANAGER_NUM     5

struct i2c_manager *pi2c_mgr[MAX_I2C_MANAGER_NUM] = {NULL};

int i2c_init(void* st)
{
	int i2c_idx = 0;

	if (!st)
	{
		return -1;
	}

	for (i2c_idx = 0; i2c_idx < MAX_I2C_MANAGER_NUM; i2c_idx ++)
	{
		if (pi2c_mgr[i2c_idx] == (struct i2c_manager*)st)
		{
			printk(KERN_INFO "%s: i2c is exist\n", __func__);
			break;
		}
		else if (pi2c_mgr[i2c_idx] == NULL)
		{
			pi2c_mgr[i2c_idx] = (struct i2c_manager*)st;
			break;
		}
	}

	return i2c_idx;
}

int i2c_reg_read(int i2c_idx, unsigned int reg, unsigned char *val)
{
	int error = 0;
	struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
	I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
	mutex_lock(&_pi2c->lock);

	if (addr_type == ADDR_8BIT)
	{
		unsigned char reg_ = (unsigned char)(reg & 0xFF);
		error = i2c_smbus_read_byte_data(_pi2c->client, reg_);

		if (error < 0)
		{
			dev_err(&_pi2c->client->dev,
					"%s: failed to read reg:0x%x error:%d\n",
					__func__, reg , error);
		}
		else
		{
			*(unsigned char *)val = error & 0xFF;
		}
	}
	else if (addr_type == ADDR_16BIT)
	{
	}

	mutex_unlock(&_pi2c->lock);
	return error;
}

int i2c_reg_write(int i2c_idx, unsigned int reg, unsigned char val)
{
	int error = 0;
	struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
	I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
	mutex_lock(&_pi2c->lock);

	if (addr_type == ADDR_8BIT)
	{
		unsigned char reg_ = (unsigned char)(reg & 0xFF);
		error = i2c_smbus_write_byte_data(_pi2c->client, reg_, val);
	}
	else if (addr_type == ADDR_16BIT)
	{
	}

	mutex_unlock(&_pi2c->lock);

	if (error < 0)
	{
		dev_err(&_pi2c->client->dev,
				"%s: failed to write reg:0x%x with val:0x%x error:%d\n",
				__func__, reg, val, error);
	}

	return error;
}

int i2c_reg_write_block(int i2c_idx, unsigned int reg, void *val, int length)
{
	int error = 0;
	struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
	I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
	mutex_lock(&_pi2c->lock);

	if (addr_type == ADDR_8BIT)
	{
		unsigned char reg_ = (unsigned char)(reg & 0xFF);
		error = i2c_smbus_write_i2c_block_data(_pi2c->client, reg_, length, val);
	}
	else if (addr_type == ADDR_16BIT)
	{
		int i = 0;
		unsigned char *buffer_inverse;
		struct i2c_msg msgs;
		buffer_inverse = kzalloc((sizeof(unsigned char) * (length + 2)), GFP_KERNEL);
		buffer_inverse[0] = reg >> 8;
		buffer_inverse[1] = reg & 0xff;

		for (i = 0; i < length; i ++)
		{
			buffer_inverse[2 + i] = *(u8*)((u8*)val + ((length - 1) - i));
		}

		msgs.addr = _pi2c->client->addr;
		msgs.flags = _pi2c->client->flags & I2C_M_TEN;
		msgs.len = length + 2;
		msgs.buf = buffer_inverse;
#ifdef STK_RETRY_I2C
		i = 0;

		do
		{
			error = i2c_transfer(_pi2c->client->adapter, &msgs, 1);
		}
		while (error != 1 && ++i < 3);

#else
		error = i2c_transfer(_pi2c->client->adapter, &msgs, 1);
#endif //  STK_RETRY_I2C
		kfree(buffer_inverse);
	}

	mutex_unlock(&_pi2c->lock);

	if (error < 0)
	{
		dev_err(&_pi2c->client->dev,
				"%s: failed to write reg:0x%x\n",
				__func__, reg);
	}

	return error;
}

int i2c_reg_read_modify_write(int i2c_idx, unsigned int reg, unsigned char val, unsigned char mask)
{
	uint8_t rw_buffer = 0;
	int error = 0;
	struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];

	if ((mask == 0xFF) || (mask == 0x0))
	{
		error = i2c_reg_write(i2c_idx, reg, val);

		if (error < 0)
		{
			dev_err(&_pi2c->client->dev,
					"%s: failed to write reg:0x%x with val:0x%x\n",
					__func__, reg, val);
		}
	}
	else
	{
		error = (uint8_t)i2c_reg_read(i2c_idx, reg, &rw_buffer);

		if (error < 0)
		{
			dev_err(&_pi2c->client->dev,
					"%s: failed to read reg:0x%x\n",
					__func__, reg);
			return error;
		}
		else
		{
			rw_buffer = (rw_buffer & (~mask)) | (val & mask);
			error = i2c_reg_write(i2c_idx, reg, rw_buffer);

			if (error < 0)
			{
				dev_err(&_pi2c->client->dev,
						"%s: failed to write reg(mask):0x%x with val:0x%x\n",
						__func__, reg, val);
			}
		}
	}

	return error;
}

int i2c_reg_read_block(int i2c_idx, unsigned int reg, int count, void *buf)
{
	int ret = 0;
	// int loop_cnt = 0;
	struct i2c_manager *_pi2c = pi2c_mgr[i2c_idx];
	I2C_REG_ADDR_TYPE addr_type = _pi2c->addr_type;
	mutex_lock(&_pi2c->lock);

	if (addr_type == ADDR_8BIT)
	{
		struct i2c_msg msgs[2] =
		{
			{
				.addr = _pi2c->client->addr,
				.flags = 0,
				.len = 1,
				.buf = (u8*)&reg
			},
			{
				.addr = _pi2c->client->addr,
				.flags = I2C_M_RD,
				.len = count,
				.buf = buf
			}
		};
		ret = i2c_transfer(_pi2c->client->adapter, msgs, 2);

		if (2 == ret)
		{
			ret = 0;
		}
		// unsigned char reg_ = (unsigned char)(reg & 0xFF);

		// while (count)
		// {
		//     ret = i2c_smbus_read_i2c_block_data(_pi2c->client, reg_,
		//                                         (count > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : count,
		//                                         (buf + (loop_cnt * I2C_SMBUS_BLOCK_MAX))
		//                                        );
		//     (count > I2C_SMBUS_BLOCK_MAX) ? (count -= I2C_SMBUS_BLOCK_MAX) : (count -= count);
		//     loop_cnt ++;
		// }
	}
	else if (addr_type == ADDR_16BIT)
	{
		int i = 0;
		u16 reg_inverse = (reg & 0x00FF) << 8 | (reg & 0xFF00) >> 8;
		int read_length = count;
		u8 buffer_inverse[99] = { 0 };
		struct i2c_msg msgs[2] =
		{
			{
				.addr = _pi2c->client->addr,
				.flags = 0,
				.len = 2,
				.buf = (u8*)&reg_inverse
			},
			{
				.addr = _pi2c->client->addr,
				.flags = I2C_M_RD,
				.len = read_length,
				.buf = buffer_inverse
			}
		};
#ifdef STK_RETRY_I2C
		i = 0;

		do
		{
			ret = i2c_transfer(_pi2c->client->adapter, msgs, 2);
		}
		while (ret != 2 && ++i < 3);

#else
		ret = i2c_transfer(_pi2c->client->adapter, msgs, 2);
#endif //  STK_RETRY_I2C

		if (2 == ret)
		{
			ret = 0;

			for (i = 0; i < read_length; i ++)
			{
				*(u8*)((u8*)buf + i) = ((buffer_inverse[read_length - 1 - i]));
			}
		}
	}

	mutex_unlock(&_pi2c->lock);
	return ret;
}

int i2c_remove(void* st)
{
	int i2c_idx = 0;

	if (!st)
	{
		return -1;
	}

	for (i2c_idx = 0; i2c_idx < MAX_I2C_MANAGER_NUM; i2c_idx ++)
	{
		printk(KERN_INFO "%s: i2c_idx = %d\n", __func__, i2c_idx);

		if (pi2c_mgr[i2c_idx] == (struct i2c_manager*)st)
		{
			printk(KERN_INFO "%s: release i2c_idx = %d\n", __func__, i2c_idx);
			pi2c_mgr[i2c_idx] = NULL;
			break;
		}
	}

	return 0;
}

const struct stk_bus_ops stk_i2c_bops =
{
	.bustype            = BUS_I2C,
	.init               = i2c_init,
	.write              = i2c_reg_write,
	.write_block        = i2c_reg_write_block,
	.read               = i2c_reg_read,
	.read_block         = i2c_reg_read_block,
	.read_modify_write  = i2c_reg_read_modify_write,
	.remove             = i2c_remove,
};
