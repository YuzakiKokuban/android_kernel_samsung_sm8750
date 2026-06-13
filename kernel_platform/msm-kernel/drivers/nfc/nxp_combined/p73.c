/******************************************************************************
 *
 *  Copyright 2012-2023 NXP
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

/**
* \addtogroup spi_driver
*
* @{ */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/ktime.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/spi/spidev.h>
#include <linux/of_platform.h>
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
#include <linux/rcupdate.h>
#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
#include "nfc_wakelock.h"
#if IS_ENABLED(CONFIG_MSM_GENI_SE)
#include <linux/msm-geni-se.h>
#include <linux/msm_gpi.h>
#endif
#endif
#endif

#include "p73.h"
#include "common_ese.h"
#include "ese_reset.h"
#define DRAGON_P61 0

/* Device driver's configuration macro */
/* Macro to configure poll/interrupt based req*/
#undef P61_IRQ_ENABLE
//#define P61_IRQ_ENABLE

/* Macro to configure Hard/Soft reset to P61 */
//#define P61_HARD_RESET
#undef P61_HARD_RESET

#ifdef P61_HARD_RESET
static struct regulator *p61_regulator = NULL;
#else
#endif

#define P61_IRQ   33		/* this is the same used in omap3beagle.c */
#define P61_RST  138

/* Macro to define SPI clock frequency */

//#define P61_SPI_CLOCK_7Mzh
#undef P61_SPI_CLOCK_7Mzh
#undef P61_SPI_CLOCK_13_3_Mzh
#undef P61_SPI_CLOCK_8Mzh
#undef P61_SPI_CLOCK_20Mzh
#define P61_SPI_CLOCK_25Mzh

#ifdef P61_SPI_CLOCK_13_3_Mzh
//#define P61_SPI_CLOCK 13300000L;Further debug needed
#define P61_SPI_CLOCK     19000000L;
#else
#ifdef P61_SPI_CLOCK_7Mzh
#define P61_SPI_CLOCK     7000000L;
#else
#ifdef P61_SPI_CLOCK_8Mzh
#define P61_SPI_CLOCK     8000000L;
#else
#ifdef P61_SPI_CLOCK_20Mzh
#define P61_SPI_CLOCK     20000000L;
#else
#ifdef P61_SPI_CLOCK_25Mzh
#define P61_SPI_CLOCK     25000000L;
#else
#define P61_SPI_CLOCK     4000000L;
#endif
#endif
#endif
#endif
#endif

/* size of maximum read/write buffer supported by driver */
#ifdef MAX_BUFFER_SIZE
#undef MAX_BUFFER_SIZE
#endif //MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE   780U

/* Different driver debug lever */
enum P61_DEBUG_LEVEL {
	P61_DEBUG_OFF,
	P61_FULL_DEBUG
};

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
static char *p61_pinctrl_name[] = {"ese_off", "ese_on", "lpm", "default", "active", "suspend"};
enum p61_pin_ctrl {
	P61_PIN_CTRL_ESE_OFF,
	P61_PIN_CTRL_ESE_ON,
	P61_PIN_CTRL_LPM,
	P61_PIN_CTRL_DEFAULT,
	P61_PIN_CTRL_ACTIVE,
	P61_PIN_CTRL_SUSPEND,
	P61_PIN_CTRL_MAX
};
#endif

/* Variable to store current debug level request by ioctl */
static unsigned char debug_level;

static DEFINE_MUTEX(open_close_mutex);

#define P61_DBG_MSG(msg...)  \
        switch(debug_level)      \
        {                        \
        case P61_DEBUG_OFF:      \
        break;                 \
        case P61_FULL_DEBUG:     \
        printk(KERN_INFO "[NXP-P61] :  " msg); \
        break; \
        default:                 \
        printk(KERN_ERR "[NXP-P61] :  Wrong debug level %d", debug_level); \
        break; \
        } \

#define P61_ERR_MSG(msg...) printk(KERN_ERR "[NFC-P61] : " msg );

/* Device specific macro and structure */
struct p61_dev {
	wait_queue_head_t read_wq;	/* wait queue for read interrupt */
	struct mutex read_mutex;	/* read mutex */
	struct mutex write_mutex;	/* write mutex */
	struct spi_device *spi;	/* spi device structure */
	struct miscdevice p61_device;	/* char device as misc driver */
	int rst_gpio;	/* SW Reset gpio */
	int irq_gpio;	/* P61 will interrupt DH for any ntf */
	bool irq_enabled;	/* flag to indicate irq is used */
	unsigned char enable_poll_mode;	/* enable the poll mode */
	spinlock_t irq_enabled_lock;	/*spin lock for read irq */
	int trusted_ese_gpio; /* i/p to eSE to distunguish trusted Session */
	ese_spi_transition_state_t ese_spi_transition_state; /* State of the driver */
	struct device *nfcc_device;	/*nfcc driver handle for driver to driver comm */
	struct nfc_dev *nfcc_data;
	const char *nfcc_name;
	bool gpio_coldreset;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	int ap_vendor;
	struct device_node *nfc_node;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_state[P61_PIN_CTRL_MAX];
	struct platform_device *spi_pdev;
	struct nfc_wake_lock ese_lock;
	int open_pid;
	int release_pid;
	char open_task_name[TASK_COMM_LEN];
	char release_task_name[TASK_COMM_LEN];
#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
	struct delayed_work spi_release_work;
	struct nfc_wake_lock spi_release_wakelock;
#endif
#endif
	unsigned char *r_buf;
	unsigned char *w_buf;
};

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
struct device  *g_nfc_device;
struct p61_dev *g_p61_dev;
#endif

/* T==1 protocol specific global data */
const unsigned char SOF = 0xA5u;

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
static void p61_get_task_info(struct p61_dev *p61_dev, char *task_name, int *pid)
{
	struct task_struct *task;

	if (!p61_dev)
		return;

	rcu_read_lock();
	*pid = task_pid_nr(current);
	task = pid_task(find_vpid(*pid), PIDTYPE_PID);
	if (task) {
		memcpy(task_name, task->comm, TASK_COMM_LEN);
		task_name[TASK_COMM_LEN - 1] = '\0';
	}
	rcu_read_unlock();
}

static void p61_init_task_info(struct p61_dev *p61_dev)
{
	p61_dev->open_pid = 0;
	p61_dev->release_pid = 0;
	memset(p61_dev->open_task_name, 0, TASK_COMM_LEN);
	memset(p61_dev->release_task_name, 0, TASK_COMM_LEN);
}

void p61_print_status(const char *func_name)
{
	struct p61_dev *p61_dev = g_p61_dev;

	if (!p61_dev)
		return;

	NFC_LOG_INFO("%s: state=%d o_pid=%d rel_pid=%d o_task=%s rel_task=%s\n",
			func_name, p61_dev->ese_spi_transition_state,
			p61_dev->open_pid,
			p61_dev->release_pid,
			p61_dev->open_task_name,
			p61_dev->release_task_name);
}

static void p61_pinctrl_select(struct p61_dev *p61_dev, enum p61_pin_ctrl stat)
{
	int ret;

	NFC_LOG_INFO("pinctrl[%s]\n", p61_pinctrl_name[stat]);

	if (!p61_dev->pinctrl || !p61_dev->pinctrl_state[stat])
		return;

	ret = pinctrl_select_state(p61_dev->pinctrl, p61_dev->pinctrl_state[stat]);
	if (ret < 0)
		NFC_LOG_INFO("pinctrl[%d] failed\n", stat);
}

#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
static void p61_spi_release_work(struct work_struct *work)
{
	struct p61_dev *p61_dev = g_p61_dev;

	if (p61_dev == NULL) {
		NFC_LOG_ERR("%s: spi probe is not called\n", __func__);
		return;
	}

	NFC_LOG_INFO("release ese spi\n");
	p61_pinctrl_select(p61_dev, P61_PIN_CTRL_SUSPEND); /* for QC AP */
}
#endif
#endif

/**
 * \ingroup spi_driver
 * \brief Will be called on device close to release resources
 *
 * \param[in]       struct inode *
 * \param[in]       struct file *
 *
 * \retval 0 if ok.
 *
 */
static int p61_dev_release(struct inode *inode, struct file *filp)
{
	struct p61_dev *p61_dev = NULL;

	NFC_LOG_INFO("Enter %s: ESE driver release\n", __func__);

	mutex_lock(&open_close_mutex);
	p61_dev = filp->private_data;
	p61_dev->ese_spi_transition_state = ESE_SPI_IDLE;
#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
	gpio_set_value(p61_dev->trusted_ese_gpio, 0);
#endif
	nfc_ese_pwr(p61_dev->nfcc_data, ESE_RST_PROT_DIS);

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	p61_pinctrl_select(p61_dev, P61_PIN_CTRL_ESE_OFF); /* for LSI AP */
#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
	schedule_delayed_work(&p61_dev->spi_release_work,
				msecs_to_jiffies(2000));
	wake_lock_timeout(&p61_dev->spi_release_wakelock,
				msecs_to_jiffies(2100));
#endif
	if (wake_lock_active(&p61_dev->ese_lock))
		wake_unlock(&p61_dev->ese_lock);
#endif
	mutex_unlock(&open_close_mutex);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	p61_get_task_info(p61_dev, p61_dev->release_task_name, &p61_dev->release_pid);
	p61_print_status(__func__);
#endif
	NFC_LOG_INFO("Exit %s: ESE driver release\n", __func__);
	return 0;
}

static int p61_xfer(struct p61_dev *p61_dev,
		struct p61_ioctl_transfer *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;
	static u32 read_try_cnt;
	/*For SDM845 & linux4.9: need to change spi buffer
	 * from stack to dynamic memory
	 */

	if (p61_dev == NULL || tr == NULL)
		return -EFAULT;

	if (tr->len > MAX_BUFFER_SIZE || !tr->len)
		return -EMSGSIZE;

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	memset(p61_dev->w_buf, 0, tr->len); /*memset 0 for write */
	memset(p61_dev->r_buf, 0, tr->len); /*memset 0 for read */
	if (tr->tx_buffer != NULL) { /*write */
		if (copy_from_user(p61_dev->w_buf, tr->tx_buffer, tr->len) != 0) {
			NFC_LOG_ERR("p61_wr: copy_from_user fail %d\n", tr->len);
			status = -EFAULT;
			goto xfer_exit;
		}
	}

	t.rx_buf = p61_dev->r_buf;
	t.tx_buf = p61_dev->w_buf;
	t.len = tr->len;

	spi_message_add_tail(&t, &m);

	status = spi_sync(p61_dev->spi, &m);
	if (status == 0) {
		if (tr->rx_buffer != NULL) { /*read */
			unsigned long missing = 0;

			missing = copy_to_user(tr->rx_buffer, p61_dev->r_buf, tr->len);
			if (missing != 0)
				tr->len = tr->len - (unsigned int)missing;
		}
	}

	if (tr->tx_buffer != NULL) {
		if (read_try_cnt)
			NFC_LOG_REC("p61w%d try%u\n", tr->len, read_try_cnt);
		else
			NFC_LOG_REC("p61w%d\n", tr->len);
	}
	if (tr->rx_buffer != NULL) {
		if (tr->len == 2 && ((p61_dev->r_buf[0] == 0x0 && p61_dev->r_buf[2] == 0x0) ||
				(p61_dev->r_buf[0] == 0xff && p61_dev->r_buf[2] == 0xff))) {
			read_try_cnt++;
		} else {
			if (read_try_cnt)
				NFC_LOG_REC("p61r%d try%u\n", tr->len, read_try_cnt);
			else
				NFC_LOG_REC("p61r%d\n", tr->len);
			read_try_cnt = 0;
		}
	}

xfer_exit:
	return status;
} /* vfsspi_xfer */

static int p61_rw_spi_message(struct p61_dev *p61_dev,
		unsigned long arg)
{
	struct p61_ioctl_transfer	*dup = NULL;
	int err = 0;

	dup = kmalloc(sizeof(struct p61_ioctl_transfer), GFP_KERNEL);
	if (dup == NULL)
		return -ENOMEM;

	if (copy_from_user(dup, (void *)arg,
				sizeof(struct p61_ioctl_transfer)) != 0) {
		kfree(dup);
		return -EFAULT;
	}

	err = p61_xfer(p61_dev, dup);
	if (err != 0) {
		kfree(dup);
		NFC_LOG_ERR("%s: p61_xfer failed, %d\n", __func__, err);
		return err;
	}

	if (copy_to_user((void *)arg, dup,
				sizeof(struct p61_ioctl_transfer)) != 0) {
		kfree(dup);
		return -EFAULT;
	}
	kfree(dup);
	return 0;
}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
void store_nfc_i2c_device(struct device *nfc_i2c_dev)
{
	g_nfc_device = nfc_i2c_dev;
}
#endif

#ifdef CONFIG_COMPAT
static int p61_rw_spi_message_compat(struct p61_dev *p61_dev,
				 unsigned long arg)
{
	struct p61_ioctl_transfer32 __user *argp = compat_ptr(arg);
	struct p61_ioctl_transfer32 it32;
	struct p61_ioctl_transfer   *dup = NULL;
	int err = 0;

	dup = kmalloc(sizeof(struct p61_ioctl_transfer), GFP_KERNEL);
	if (dup == NULL)
		return -ENOMEM;

	if (copy_from_user(&it32, argp, sizeof(it32))) {
		kfree(dup);
		return -EFAULT;
	}

	dup->rx_buffer = (__u8 *)(uintptr_t)it32.rx_buffer;
	dup->tx_buffer = (__u8 *)(uintptr_t)it32.tx_buffer;
	dup->len = it32.len;

	err = p61_xfer(p61_dev, dup);
	if (err != 0) {
		kfree(dup);
		NFC_LOG_ERR("%s: p61_xfer failed, %d\n", __func__, err);
		return err;
	}

	if (it32.rx_buffer) {
		if (__put_user(dup->len, &argp->len)) {
			kfree(dup);
			return -EFAULT;
		}
	}
	kfree(dup);
	return 0;
}
#endif	/*CONFIG_COMPAT */

/**
 * \ingroup spi_driver
 * \brief Called from SPI LibEse to initilaize the P61 device
 *
 * \param[in]       struct inode *
 * \param[in]       struct file *
 *
 * \retval 0 if ok.
 *
 */

static int p61_dev_open(struct inode *inode, struct file *filp)
{

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	struct p61_dev *p61_dev = g_p61_dev;

	if (p61_dev == NULL) {
		NFC_LOG_ERR("%s: spi probe is not called\n", __func__);
		return -EAGAIN;
	}
#else
	struct p61_dev
	*p61_dev = container_of(filp->private_data,
				struct p61_dev,
				p61_device);
#endif

	/* Find the NFC parent device if it exists. */
	if (p61_dev != NULL && p61_dev->nfcc_data == NULL) {
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
		struct device *nfc_dev = g_nfc_device;
		if (!nfc_dev) {
			NFC_LOG_ERR("%s: cannot find NFC controller\n", __func__);
			return -ENODEV;
		}
#else
		struct device *nfc_dev = bus_find_device_by_name(&i2c_bus_type, NULL,
					 p61_dev->nfcc_name);
		if (!nfc_dev) {
			NFC_LOG_ERR("%s: cannot find NFC controller '%s'\n", __func__,
				    p61_dev->nfcc_name);
			return -ENODEV;
		}
#endif
		p61_dev->nfcc_data = dev_get_drvdata(nfc_dev);
		if (!p61_dev->nfcc_data) {
			NFC_LOG_ERR("%s: cannot find NFC controller device data\n", __func__);
			put_device(nfc_dev);
			return -ENODEV;
		}
		P61_DBG_MSG("%s: NFC controller found\n", __func__);
		p61_dev->nfcc_device = nfc_dev;
	}

	mutex_lock(&open_close_mutex);
	filp->private_data = p61_dev;
	if(p61_dev->ese_spi_transition_state == ESE_SPI_BUSY) {
		NFC_LOG_ERR("%s : ESE is busy\n", __func__);
		mutex_unlock(&open_close_mutex);
		return -EBUSY;
	}

	p61_dev->ese_spi_transition_state = ESE_SPI_BUSY;

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	/* for checking previous open/close tasks */
	p61_print_status("p61_dev_open pre");
#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
	cancel_delayed_work_sync(&p61_dev->spi_release_work);
#endif
	if (!wake_lock_active(&p61_dev->ese_lock))
		wake_lock(&p61_dev->ese_lock);

	p61_pinctrl_select(p61_dev, P61_PIN_CTRL_ESE_ON);

	if (p61_dev->ap_vendor != AP_VENDOR_QCT)
		msleep(60);

#endif
	mutex_unlock(&open_close_mutex);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	p61_init_task_info(p61_dev);
	p61_get_task_info(p61_dev, p61_dev->open_task_name, &p61_dev->open_pid);
	p61_print_status(__func__);
#endif
	return 0;
}



/**
 * \ingroup spi_driver
 * \brief To configure the P61_SET_PWR/P61_SET_DBG/P61_SET_POLL
 * \n         P61_SET_PWR - hard reset (arg=2), soft reset (arg=1)
 * \n         P61_SET_DBG - Enable/Disable (based on arg value) the driver logs
 * \n         P61_SET_POLL - Configure the driver in poll (arg = 1), interrupt (arg = 0) based read operation
 * \param[in]       struct file *
 * \param[in]       unsigned int
 * \param[in]       unsigned long
 *
 * \retval 0 if ok.
 *
*/

static long p61_dev_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;
	struct p61_dev *p61_dev = NULL;
#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
	unsigned char buf[100];
#endif

	P61_DBG_MSG(KERN_ALERT "p61_dev_ioctl-Enter %u arg = %ld\n", cmd, arg);
	p61_dev = filp->private_data;

	switch (cmd) {
	case P61_SET_PWR:
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
		if (arg == 2)
			NFC_LOG_INFO("[NXP-P61] - P61_SET_PWR. No Action.\n");
#else
		if (arg == 2) {
#ifdef P61_HARD_RESET
			P61_DBG_MSG(KERN_ALERT " Disabling p61_regulator");
			if (p61_regulator != NULL) {
				regulator_disable(p61_regulator);
				msleep(50);
				regulator_enable(p61_regulator);
				P61_DBG_MSG(KERN_ALERT " Enabling p61_regulator");
			} else {
				NFC_LOG_ERR(KERN_ALERT " ERROR : p61_regulator is not enabled\n");
			}
#endif

		} else if (arg == 1) {
			P61_DBG_MSG(KERN_ALERT " Soft Reset");
			//gpio_set_value(p61_dev->rst_gpio, 1);
			//msleep(20);
			gpio_set_value(p61_dev->rst_gpio, 0);
			msleep(50);
			ret = spi_read(p61_dev->spi, (void *)buf, sizeof(buf));
			msleep(50);
			gpio_set_value(p61_dev->rst_gpio, 1);
			msleep(20);

		}
#endif
		break;

	case P61_SET_DBG:
		debug_level = (unsigned char)arg;
		NFC_LOG_INFO("[NXP-P61] -  Debug level %d\n",
			    debug_level);
		break;

	case P61_SET_POLL:

		p61_dev->enable_poll_mode = (unsigned char)arg;
		if (p61_dev->enable_poll_mode == 0) {
			NFC_LOG_INFO("[NXP-P61] - IRQ Mode is set\n");
		} else {
			NFC_LOG_INFO("[NXP-P61] - Poll Mode is set\n");
			p61_dev->enable_poll_mode = 1;
		}
		break;
	case P61_RW_SPI_DATA:
		ret = p61_rw_spi_message(p61_dev, arg);
		break;
	case P61_SET_SPM_PWR:
		NFC_LOG_INFO("P61_SET_SPM_PWR: enter\n");
		ret = nfc_ese_pwr(p61_dev->nfcc_data, arg);
		NFC_LOG_INFO("P61_SET_SPM_PWR: exit\n");
		break;
	case P61_GET_SPM_STATUS:
		NFC_LOG_INFO("P61_GET_SPM_STATUS: enter\n");
		ret = nfc_ese_pwr(p61_dev->nfcc_data, ESE_POWER_STATE);
		NFC_LOG_INFO("P61_GET_SPM_STATUS: exiti\n");
		break;
	case P61_SET_DWNLD_STATUS:
		NFC_LOG_INFO("P61_SET_DWNLD_STATUS: enter\n");
		//ret = nfc_dev_ioctl(filp, PN544_SET_DWNLD_STATUS, arg);
		NFC_LOG_INFO("P61_SET_DWNLD_STATUS: =%lu exit\n", arg);
		break;
	case P61_GET_ESE_ACCESS:
		NFC_LOG_INFO("P61_GET_ESE_ACCESS: enter\n");
		//ret = nfc_dev_ioctl(filp, P544_GET_ESE_ACCESS, arg);
		NFC_LOG_INFO("P61_GET_ESE_ACCESS ret: %d exit\n", ret);
		break;
	case P61_SET_POWER_SCHEME:
		NFC_LOG_INFO("P61_SET_POWER_SCHEME: enter\n");
		//ret = nfc_dev_ioctl(filp, P544_SET_POWER_SCHEME, arg);
		NFC_LOG_INFO("P61_SET_POWER_SCHEME ret: %d exit\n",
			    ret);
		break;
	case P61_INHIBIT_PWR_CNTRL:
		NFC_LOG_INFO("P61_INHIBIT_PWR_CNTRL: enter\n");
		//ret = nfc_dev_ioctl(filp, P544_SECURE_TIMER_SESSION, arg);
		NFC_LOG_INFO("P61_INHIBIT_PWR_CNTRL ret: %d exit\n",
			    ret);
		break;
	case ESE_PERFORM_COLD_RESET:
		NFC_LOG_INFO("ESE_PERFORM_COLD_RESET: enter\n");
		if (p61_dev->gpio_coldreset)
			ret = perform_ese_gpio_reset(p61_dev->rst_gpio);
		else
			ret = nfc_ese_pwr(p61_dev->nfcc_data, ESE_CLD_RST);
		NFC_LOG_INFO("ESE_PERFORM_COLD_RESET ret: %d exit\n", ret);
		break;
	case PERFORM_RESET_PROTECTION:
		if (p61_dev->gpio_coldreset) {
			NFC_LOG_INFO("PERFORM_RESET_PROTECTION is not required and not supported\n");
		} else {
			NFC_LOG_INFO("PERFORM_RESET_PROTECTION: enter\n");
			ret = nfc_ese_pwr(p61_dev->nfcc_data,
				  (arg == 1 ? ESE_RST_PROT_EN : ESE_RST_PROT_DIS));
			NFC_LOG_INFO("PERFORM_RESET_PROTECTION ret: %d exit\n", ret);
		}
		break;
	case ESE_SET_TRUSTED_ACCESS:
		NFC_LOG_INFO("Enter %s: TRUSTED access enabled=%lu\n", __func__, arg);
#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
		if(arg == 1) {
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS: enter Enabling\n");
			gpio_set_value(p61_dev->trusted_ese_gpio, 1);
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS ret: exit\n");
		} else if (arg == 0) {
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS: enter Disabling\n");
			gpio_set_value(p61_dev->trusted_ese_gpio, 0);
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS ret: exit\n");
		}
#endif
		break;
	default:
		NFC_LOG_ERR("Error case\n");
		ret = -EINVAL;
	}

	P61_DBG_MSG(KERN_ALERT "p61_dev_ioctl-exit %u arg = %lu\n", cmd, arg);
	return ret;
}

#ifdef CONFIG_COMPAT
/**
 * \ingroup spi_driver
 * \brief To configure the P61_SET_PWR/P61_SET_DBG/P61_SET_POLL
 * \n         P61_SET_PWR - hard reset (arg=2), soft reset (arg=1)
 * \n         P61_SET_DBG - Enable/Disable (based on arg value) the driver logs
 * \n         P61_SET_POLL - Configure the driver in poll (arg = 1), interrupt (arg = 0) based read operation
 * \param[in]       struct file *
 * \param[in]       unsigned int
 * \param[in]       unsigned long
 *
 * \retval 0 if ok.
 *
 */
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
static long p61_dev_compat_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;
	struct p61_dev *p61_dev = NULL;

	if (_IOC_TYPE(cmd) != P61_MAGIC)
		return -ENOTTY;

	p61_dev = filp->private_data;

	switch (cmd) {
	case P61_SET_PWR_COMPAT:
		if (arg == 2)
			NFC_LOG_INFO("%s: P61_SET_PWR. No Action.\n", __func__);
		break;

	case P61_SET_DBG_COMPAT:
		debug_level = (unsigned char)arg;
		P61_DBG_MSG(KERN_INFO"[NXP-P61] -  Debug level %d",
			debug_level);
		break;
	case P61_SET_POLL_COMPAT:
		p61_dev->enable_poll_mode = (unsigned char)arg;
		if (p61_dev->enable_poll_mode == 0) {
			P61_DBG_MSG(KERN_INFO"[NXP-P61] - IRQ Mode is set\n");
		} else {
			P61_DBG_MSG(KERN_INFO"[NXP-P61] - Poll Mode is set\n");
			p61_dev->enable_poll_mode = 1;
		}
		break;

	case P61_RW_SPI_DATA_COMPAT:
		ret = p61_rw_spi_message_compat(p61_dev, arg);
		break;

	case P61_SET_SPM_PWR_COMPAT:
		NFC_LOG_INFO("%s: P61_SET_SPM_PWR: enter\n", __func__);
		ret = nfc_ese_pwr(p61_dev->nfcc_data, arg);
		NFC_LOG_INFO("%s: P61_SET_SPM_PWR: exit\n", __func__);
		break;

	case P61_GET_SPM_STATUS_COMPAT:
		NFC_LOG_INFO("%s: P61_GET_SPM_STATUS: enter\n", __func__);
		ret = nfc_ese_pwr(p61_dev->nfcc_data, ESE_POWER_STATE);
		NFC_LOG_INFO("%s: P61_GET_SPM_STATUS: exit\n", __func__);
		break;

	case P61_SET_DWNLD_STATUS_COMPAT:
		NFC_LOG_INFO("P61_SET_DWNLD_STATUS: enter\n");
		//ret = pn547_dev_ioctl(filp, PN547_SET_DWNLD_STATUS, arg);
		NFC_LOG_INFO("%s: P61_SET_DWNLD_STATUS: =%lu exit\n", __func__, arg);
		break;

	case P61_GET_ESE_ACCESS_COMPAT:
		NFC_LOG_INFO("P61_GET_ESE_ACCESS: enter\n");
		//ret = pn547_dev_ioctl(filp, P547_GET_ESE_ACCESS, arg);
		NFC_LOG_INFO("P61_GET_ESE_ACCESS ret: %d exit\n", ret);
		break;

	case P61_SET_POWER_SCHEME_COMPAT:
		NFC_LOG_INFO("P61_SET_POWER_SCHEME: enter\n");
		//ret = pn547_dev_ioctl(filp, P544_SET_POWER_SCHEME, arg);
		NFC_LOG_INFO("P61_SET_POWER_SCHEME ret: %d exit\n",
			    ret);
		break;

	case P61_INHIBIT_PWR_CNTRL_COMPAT:
		NFC_LOG_INFO("P61_INHIBIT_PWR_CNTRL: enter\n");
		//ret = pn547_dev_ioctl(filp, P544_SECURE_TIMER_SESSION, arg);
		NFC_LOG_INFO("P61_INHIBIT_PWR_CNTRL ret: %d exit\n",
			    ret);
		break;

	case ESE_PERFORM_COLD_RESET_COMPAT:
		NFC_LOG_INFO("ESE_PERFORM_COLD_RESET: enter\n");
		if (p61_dev->gpio_coldreset)
			ret = perform_ese_gpio_reset(p61_dev->rst_gpio);
		else
			ret = nfc_ese_pwr(p61_dev->nfcc_data, ESE_CLD_RST);
		NFC_LOG_INFO("ESE_PERFORM_COLD_RESET ret: %d exit\n", ret);
		break;

	case PERFORM_RESET_PROTECTION_COMPAT:
		if (p61_dev->gpio_coldreset) {
			NFC_LOG_INFO("PERFORM_RESET_PROTECTION is not required and not supported\n");
		} else {
			NFC_LOG_INFO("PERFORM_RESET_PROTECTION: enter\n");
			ret = nfc_ese_pwr(p61_dev->nfcc_data,
				  (arg == 1 ? ESE_RST_PROT_EN : ESE_RST_PROT_DIS));
			NFC_LOG_INFO("PERFORM_RESET_PROTECTION ret: %d exit\n", ret);
		}
		break;

	case ESE_SET_TRUSTED_ACCESS:
		NFC_LOG_INFO("Enter %s: TRUSTED access enabled=%lu\n", __func__, arg);
#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
		if (arg == 1) {
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS: enter Enabling\n");
			gpio_set_value(p61_dev->trusted_ese_gpio, 1);
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS ret: exit\n");
		} else if (arg == 0) {
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS: enter Disabling\n");
			gpio_set_value(p61_dev->trusted_ese_gpio, 0);
			NFC_LOG_INFO("ESE_SET_TRUSTED_ACCESS ret: exit\n");
		}
#endif
		break;

	default:
		NFC_LOG_INFO("%s: no matching ioctl!\n", __func__);
		ret = -EINVAL;
	}

	P61_DBG_MSG(KERN_ALERT "%s %u arg = %lu\n", __func__, cmd, arg);
	return ret;
}
#else
static long p61_dev_compat_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;

	arg = (compat_u64)arg;
	NFC_LOG_INFO(KERN_ALERT "%s-Enter %u arg = %ld\n", __func__, cmd, arg);
	NFC_LOG_DBG("%s: cmd = %x arg = %zx\n", __func__, cmd, arg);
	ret = p61_dev_ioctl(filp, cmd, arg);
	return ret;
}
#endif
#endif

/**
 * \ingroup spi_driver
 * \brief Write data to P61 on SPI
 *
 * \param[in]       struct file *
 * \param[in]       const char *
 * \param[in]       size_t
 * \param[in]       loff_t *
 *
 * \retval data size
 *
*/

static ssize_t p61_dev_write(struct file *filp, const char *buf, size_t count,
			     loff_t * offset)
{

	int ret = -1;
	struct p61_dev *p61_dev;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	unsigned char *tx_buffer;
#else
	unsigned char tx_buffer[MAX_BUFFER_SIZE];
#endif

	P61_DBG_MSG(KERN_ALERT "p61_dev_write -Enter count %zu\n", count);

	p61_dev = filp->private_data;

	mutex_lock(&p61_dev->write_mutex);
	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	tx_buffer = p61_dev->w_buf;
	memset(&tx_buffer[0], 0, MAX_BUFFER_SIZE);
#else
	memset(&tx_buffer[0], 0, sizeof(tx_buffer));
#endif
	if (copy_from_user(&tx_buffer[0], &buf[0], count)) {
		NFC_LOG_ERR("%s: failed to copy from user space\n", __func__);
		mutex_unlock(&p61_dev->write_mutex);
		return -EFAULT;
	}
	/* Write data */
	ret = spi_write(p61_dev->spi, &tx_buffer[0], count);
	if (ret < 0) {
		NFC_LOG_ERR("%s: spi_write fail %d\n", __func__, ret);
		ret = -EIO;
	} else {
		ret = count;
	}

	mutex_unlock(&p61_dev->write_mutex);
	NFC_LOG_REC("%s ret %d- Exit\n", __func__, ret);
	return ret;
}

#ifdef P61_IRQ_ENABLE

/**
 * \ingroup spi_driver
 * \brief To disable IRQ
 *
 * \param[in]       struct p61_dev *
 *
 * \retval void
 *
*/

static void p61_disable_irq(struct p61_dev *p61_dev)
{
	unsigned long flags;

	P61_DBG_MSG("Entry : %s\n", __func__);

	spin_lock_irqsave(&p61_dev->irq_enabled_lock, flags);
	if (p61_dev->irq_enabled) {
		disable_irq_nosync(p61_dev->spi->irq);
		p61_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&p61_dev->irq_enabled_lock, flags);

	P61_DBG_MSG("Exit : %s\n", __func__);
}

/**
 * \ingroup spi_driver
 * \brief Will get called when interrupt line asserted from P61
 *
 * \param[in]       int
 * \param[in]       void *
 *
 * \retval IRQ handle
 *
*/

static irqreturn_t p61_dev_irq_handler(int irq, void *dev_id)
{
	struct p61_dev *p61_dev = dev_id;

	P61_DBG_MSG("Entry : %s\n", __func__);
	p61_disable_irq(p61_dev);

	/* Wake up waiting readers */
	wake_up(&p61_dev->read_wq);

	P61_DBG_MSG("Exit : %s\n", __func__);
	return IRQ_HANDLED;
}
#endif

/**
 * \ingroup spi_driver
 * \brief Used to read data from P61 in Poll/interrupt mode configured using ioctl call
 *
 * \param[in]       struct file *
 * \param[in]       char *
 * \param[in]       size_t
 * \param[in]       loff_t *
 *
 * \retval read size
 *
*/

static ssize_t p61_dev_read(struct file *filp, char *buf, size_t count,
			    loff_t * offset)
{
	int ret = -EIO;
	struct p61_dev *p61_dev = filp->private_data;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	unsigned char *rx_buffer = p61_dev->r_buf;
#else
	unsigned char rx_buffer[MAX_BUFFER_SIZE];
#endif

	NFC_LOG_REC("%s count %zu - Enter\n", __func__, count);

	mutex_lock(&p61_dev->read_mutex);
	if (count > MAX_BUFFER_SIZE) {
		count = MAX_BUFFER_SIZE;
	}

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	memset(&rx_buffer[0], 0x00, MAX_BUFFER_SIZE);
#else
	memset(&rx_buffer[0], 0x00, sizeof(rx_buffer));
#endif

	if (p61_dev->enable_poll_mode) {
		NFC_LOG_REC("%s Poll Mode Enabled\n", __func__);

		NFC_LOG_REC("SPI_READ returned %zu\n", count);
		ret = spi_read(p61_dev->spi, (void *)&rx_buffer[0], count);
		if (0 > ret) {
			NFC_LOG_ERR("spi_read failed [SOF]\n");
			goto fail;
		}
	} else {
#ifdef P61_IRQ_ENABLE
		NFC_LOG_REC("%s Interrupt Mode Enabled\n", __func__);
		if (!gpio_get_value(p61_dev->irq_gpio)) {
			if (filp->f_flags & O_NONBLOCK) {
				ret = -EAGAIN;
				goto fail;
			}
			while (1) {
				NFC_LOG_REC("%s waiting for interrupt\n", __func__);
				p61_dev->irq_enabled = true;
				enable_irq(p61_dev->spi->irq);
				ret = wait_event_interruptible(p61_dev->read_wq, !p61_dev->irq_enabled);
				p61_disable_irq(p61_dev);
				if (ret) {
					NFC_LOG_ERR("wait_event_interruptible() : Failed\n");
					goto fail;
				}

				if (gpio_get_value(p61_dev->irq_gpio))
					break;

				NFC_LOG_ERR("%s: spurious interrupt detected\n", __func__);
			}
		}
#else
		NFC_LOG_REC(" %s P61_IRQ_ENABLE not Enabled\n", __func__);
#endif
		ret = spi_read(p61_dev->spi, (void *)&rx_buffer[0], count);
		if (0 > ret) {
			NFC_LOG_ERR("SPI_READ returned 0x%x\n", ret);
			ret = -EIO;
			goto fail;
		}
	}

	NFC_LOG_REC("total_count = %zu\n", count);

	if (copy_to_user(buf, &rx_buffer[0], count)) {
		NFC_LOG_ERR("%s : failed to copy to user space\n", __func__);
		ret = -EFAULT;
		goto fail;
	}
	NFC_LOG_REC("%s ret %d Exit\n", __func__, ret);
	NFC_LOG_REC("%s ret %d Exit\n", __func__, rx_buffer[0]);

	mutex_unlock(&p61_dev->read_mutex);

	return ret;

fail:
	NFC_LOG_ERR("Error %s ret %d Exit\n", __func__, ret);
	mutex_unlock(&p61_dev->read_mutex);
	return ret;
}

/**
 * \ingroup spi_driver
 * \brief It will configure the GPIOs required for soft reset, read interrupt & regulated power supply to P61.
 *
 * \param[in]       struct p61_spi_platform_data *
 * \param[in]       struct p61_dev *
 * \param[in]       struct spi_device *
 *
 * \retval 0 if ok.
 *
*/

#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
static int p61_hw_setup(struct p61_spi_platform_data *platform_data,
			struct p61_dev *p61_dev, struct spi_device *spi)
{
	int ret = -1;

	NFC_LOG_INFO("Entry : %s\n", __func__);
#ifdef P61_IRQ_ENABLE
	ret = gpio_request(platform_data->irq_gpio, "p61 irq");
	if (ret < 0) {
		NFC_LOG_ERR("gpio request failed gpio = 0x%x\n", platform_data->irq_gpio);
		goto fail;
	}

	ret = gpio_direction_input(platform_data->irq_gpio);
	if (ret < 0) {
		NFC_LOG_ERR("gpio request failed gpio = 0x%x\n", platform_data->irq_gpio);
		goto fail_irq;
	}
#endif

#ifdef P61_HARD_RESET
	/* RC : platform specific settings need to be declare */
#if !DRAGON_P61
	p61_regulator = regulator_get(&spi->dev, "vaux3");
#else
	p61_regulator = regulator_get(&spi->dev, "8941_l18");
#endif
	if (IS_ERR(p61_regulator)) {
		ret = PTR_ERR(p61_regulator);
#if !DRAGON_P61
		NFC_LOG_ERR(" Error to get vaux3 (error code) = %d\n", ret);
#else
		NFC_LOG_ERR(" Error to get 8941_l18 (error code) = %d\n", ret);
#endif
		return -ENODEV;
	} else {
		NFC_LOG_INFO("successfully got regulator\n");
	}

	ret = regulator_set_voltage(p61_regulator, 1800000, 1800000);
	if (ret != 0) {
		NFC_LOG_ERR("Error setting the regulator voltage %d\n", ret);
		regulator_put(p61_regulator);
		return ret;
	} else {
		regulator_enable(p61_regulator);
		NFC_LOG_INFO("successfully set regulator voltage\n");

	}
	ret = gpio_request(platform_data->rst_gpio, "p61 reset");
	if (ret < 0) {
		NFC_LOG_ERR("gpio reset request failed = 0x%x\n", platform_data->rst_gpio);
		goto fail_gpio;
	}

	/*soft reset gpio is set to default high */
	ret = gpio_direction_output(platform_data->rst_gpio, 1);
	if (ret < 0) {
		NFC_LOG_ERR("gpio rst request failed gpio = 0x%x\n", platform_data->rst_gpio);
		goto fail_gpio;
	}
#endif
	ret = gpio_request( platform_data->trusted_ese_gpio, "Trusted SE switch");
	if (ret < 0) {
		NFC_LOG_ERR("gpio reset request failed = 0x%x\n",
			    platform_data->trusted_ese_gpio);
		gpio_free(platform_data->trusted_ese_gpio);
		NFC_LOG_ERR("%s failed\n", __func__);
		return ret;
	}
	ret = gpio_direction_output(platform_data->trusted_ese_gpio,0);
	if (ret < 0) {
		NFC_LOG_ERR("gpio rst request failed gpio = 0x%x\n",
			    platform_data->trusted_ese_gpio);
		gpio_free(platform_data->trusted_ese_gpio);
		NFC_LOG_ERR("%s failed\n, __func__");
		return ret;
	}

	ret = ese_reset_gpio_setup(platform_data);
	if (ret < 0) {
		P61_ERR_MSG("Failed to setup ese reset gpio");
		goto fail;
	}

	ret = 0;
	NFC_LOG_INFO("Exit : %s\n", __func__);
	return ret;
#ifdef P61_IRQ_ENABLE
fail_irq:
	gpio_free(platform_data->irq_gpio);
#endif
fail:
	NFC_LOG_ERR("p61_hw_setup failed\n");
	return ret;
}
#endif

/**
 * \ingroup spi_driver
 * \brief Set the P61 device specific context for future use.
 *
 * \param[in]       struct spi_device *
 * \param[in]       void *
 *
 * \retval void
 *
*/

static inline void p61_set_data(struct spi_device *spi, void *data)
{
	dev_set_drvdata(&spi->dev, data);
}

/**
 * \ingroup spi_driver
 * \brief Get the P61 device specific context.
 *
 * \param[in]       const struct spi_device *
 *
 * \retval Device Parameters
 *
*/

static inline void *p61_get_data(const struct spi_device *spi)
{
	return dev_get_drvdata(&spi->dev);
}

/* possible fops on the p61 device */
static const struct file_operations p61_dev_fops = {
	.owner = THIS_MODULE,
	.read = p61_dev_read,
	.write = p61_dev_write,
	.open = p61_dev_open,
	.release = p61_dev_release,
	.unlocked_ioctl = p61_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = p61_dev_compat_ioctl,
#endif
};

#if DRAGON_P61 || IS_ENABLED(CONFIG_SAMSUNG_NFC)
static int p61_parse_dt(struct device *dev, struct p61_spi_platform_data *data)
{
	struct device_node *np = dev->of_node;
	int errorno = 0;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	int ese_det_gpio;
	const char *ap_str;
	int ret;

	ese_det_gpio = of_get_named_gpio(np, "ese-det-gpio", 0);
	if (!gpio_is_valid(ese_det_gpio)) {
		NFC_LOG_INFO("%s: ese-det-gpio is not set\n", __func__);
	} else {
		ret = gpio_request(ese_det_gpio, "ese_det_gpio");
		if (ret < 0)
			NFC_LOG_ERR("%s failed to get gpio ese_det_gpio\n", __func__);

		gpio_direction_input(ese_det_gpio);
		if (!gpio_get_value(ese_det_gpio)) {
			NFC_LOG_INFO("%s: ese is not supported\n", __func__);
			return -ENODEV;
		}
		NFC_LOG_INFO("%s: ese is supported\n", __func__);
	}

	if (!of_property_read_string(np, "p61,ap_vendor", &ap_str)) {
		if (!strcmp(ap_str, "slsi"))
			data->ap_vendor = AP_VENDOR_SLSI;
		else if (!strcmp(ap_str, "qct") || !strcmp(ap_str, "qualcomm"))
			data->ap_vendor = AP_VENDOR_QCT;
		else if (!strcmp(ap_str, "mtk"))
			data->ap_vendor = AP_VENDOR_MTK;
		NFC_LOG_INFO("AP vendor is %d\n", data->ap_vendor);
	} else {
		NFC_LOG_INFO("AP vendor is not set\n");
	}

	data->gpio_coldreset = of_property_read_bool(np, "p61,gpio_coldreset_support");
	if (data->gpio_coldreset) {
		NFC_LOG_INFO("gpio coldreset supports\n");
		data->rst_gpio = of_get_named_gpio(np, "p61,gpio-rst", 0);
		if ((!gpio_is_valid(data->rst_gpio)))
			return -EINVAL;
	}
#else
	data->irq_gpio = of_get_named_gpio(np, "nxp,p61-irq", 0);
	if ((!gpio_is_valid(data->irq_gpio)))
		return -EINVAL;

	data->rst_gpio = of_get_named_gpio(np, "nxp,p61-rst", 0);
	if ((!gpio_is_valid(data->rst_gpio)))
		return -EINVAL;

	data->trusted_ese_gpio = of_get_named_gpio(np, "nxp,trusted-se", 0);
	if ((!gpio_is_valid(data->trusted_ese_gpio)))
		return -EINVAL;

	NFC_LOG_INFO("%s: %d, %d, %d %d\n", __func__, data->irq_gpio, data->rst_gpio,
		data->trusted_ese_gpio, errorno);
#endif

	return errorno;
}
#endif
#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
#if IS_ENABLED(CONFIG_MSM_GENI_SE)
/*
 * eSE driver can't access spi_geni_master structure because it's defined in drivers/spi/spi-msm-geni.c file.
 * so, we need a logic to search se_geni_rsc in "void *spi_geni_master".
 */
struct se_geni_rsc *p61_find_spi_src(struct p61_dev *p61_dev, void *spi_geni_master)
{
	char *offset = spi_geni_master;
	struct se_geni_rsc *rsc;
	int i;
	int max_addr_cnt = 250;

	for (i = 0; i < max_addr_cnt; i++) {
		rsc = (struct se_geni_rsc *)offset;

		if (rsc->geni_gpio_active == p61_dev->pinctrl_state[P61_PIN_CTRL_DEFAULT]) {
			NFC_LOG_INFO("%s, found se_geni_rsc!\n", __func__);
			return rsc;
		}
		offset++;
	}

	/* Check if spi_geni_master structure member which defined in spi-msm-geni.c file is changed or not when failed to find se_geni_src. */
	NFC_LOG_ERR("%s, failed to find se_geni_rsc!\n", __func__);

	return 0;
}

#else
/* CONFIG_QCOM_GENI_SE */
struct qc_spi_pinctrl {
	struct pinctrl *geni_pinctrl;
	struct pinctrl_state *geni_gpio_active;
	struct pinctrl_state *geni_gpio_sleep;
};

struct qc_spi_pinctrl *p61_find_spi_src(struct p61_dev *p61_dev, void *spi_geni_master)
{
	char *offset = spi_geni_master;
	struct qc_spi_pinctrl *spi_pinctrl;
	int i;
	int max_addr_cnt = 250;

	for (i = 0; i < max_addr_cnt; i++) {
		spi_pinctrl = (struct qc_spi_pinctrl *)offset;

		if (spi_pinctrl->geni_pinctrl == p61_dev->pinctrl &&
			spi_pinctrl->geni_gpio_active == p61_dev->pinctrl_state[P61_PIN_CTRL_DEFAULT]) {
			NFC_LOG_INFO("%s, found pinctrl in spi master!\n", __func__);
			return spi_pinctrl;
		}

		offset++;
	}

	NFC_LOG_ERR("%s, failed to find spi pinctrl!\n", __func__);
	return 0;
}
#endif
static void p61_set_spi_bus_pincontrol(struct p61_dev *p61_dev)
{
	struct spi_master *master;
	void *geni_mas;
#if IS_ENABLED(CONFIG_MSM_GENI_SE)
	struct se_geni_rsc *spi_pinctrl;
#else
	struct qc_spi_pinctrl *spi_pinctrl;
#endif
	static bool called;

	if (!p61_dev || called)
		return;

	if (!p61_dev->pinctrl_state[P61_PIN_CTRL_ACTIVE] || !p61_dev->pinctrl_state[P61_PIN_CTRL_SUSPEND])
		return;
	NFC_LOG_INFO("%s\n", __func__);
	called = true;
	master = platform_get_drvdata(p61_dev->spi_pdev);
	geni_mas = spi_master_get_devdata(master);
	spi_pinctrl = p61_find_spi_src(p61_dev, geni_mas);
	if (spi_pinctrl) {
		spi_pinctrl->geni_gpio_sleep =
			pinctrl_lookup_state(spi_pinctrl->geni_pinctrl,
				p61_pinctrl_name[P61_PIN_CTRL_SUSPEND]);
		spi_pinctrl->geni_gpio_active =
			pinctrl_lookup_state(spi_pinctrl->geni_pinctrl,
				p61_pinctrl_name[P61_PIN_CTRL_ACTIVE]);
	}
}
#endif

#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
static void p61_parse_pinctrl_dt(struct device *dev, struct p61_dev *p61_dev)
{
	struct device_node *spi_dev_node;
	struct device_node *np = dev->of_node;
	struct platform_device *spi_pdev;
	struct pinctrl_state *tmp_pinctrl;
	int i;

	spi_dev_node = of_get_parent(np);
	if (IS_ERR_OR_NULL(spi_dev_node)) {
		NFC_LOG_INFO("no spi pinctrl\n");
		return;
	}

	spi_pdev = of_find_device_by_node(spi_dev_node);
	if (!spi_pdev) {
		NFC_LOG_ERR("finding spi_dev failed\n");
		return;
	}

	p61_dev->pinctrl = devm_pinctrl_get(&spi_pdev->dev);
	if (IS_ERR(p61_dev->pinctrl)) {
		NFC_LOG_ERR("pinctrl_get failed\n");
		return;
	}

	for (i = 0; i < P61_PIN_CTRL_MAX; i++) {
		tmp_pinctrl = pinctrl_lookup_state(p61_dev->pinctrl, p61_pinctrl_name[i]);
		if (!IS_ERR(tmp_pinctrl)) {
			NFC_LOG_INFO("pinctrl[%s] found\n", p61_pinctrl_name[i]);
			p61_dev->pinctrl_state[i] = tmp_pinctrl;
		}
	}

	p61_dev->spi_pdev = spi_pdev;
#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
	p61_set_spi_bus_pincontrol(p61_dev);
#endif
	if (nfc_check_pvdd_status())
		ese_set_spi_pinctrl_for_ese_off(p61_dev);
}

void ese_set_spi_pinctrl_for_ese_off(void *p61)
{
	struct p61_dev *p61_dev = (struct p61_dev *)p61;

	if (!p61_dev)
		p61_dev = g_p61_dev;

	if (!p61_dev) {
		pr_err("%s: spi probe is not called\n", __func__);
		return;
	}

	p61_pinctrl_select(p61_dev, P61_PIN_CTRL_ESE_OFF); /* for LSI AP */
	p61_pinctrl_select(p61_dev, P61_PIN_CTRL_SUSPEND); /* for QC AP */
}
#endif

/**
 * \ingroup spi_driver
 * \brief To probe for P61 SPI interface. If found initialize the SPI clock, bit rate & SPI mode.
 * It will create the dev entry (P61) for user space.
 *
 * \param[in]       struct spi_device *
 *
 * \retval 0 if ok.
 *
*/

static int p61_probe(struct spi_device *spi)
{
	int ret = -1;
	struct p61_spi_platform_data *platform_data = NULL;
	struct p61_spi_platform_data platform_data1;
	struct p61_dev *p61_dev = NULL;
	unsigned int max_speed_hz;
	struct device_node *np = spi->dev.of_node;
#ifdef P61_IRQ_ENABLE
	unsigned int irq_flags;
#endif

#ifdef CONFIG_SEC_NFC_LOGGER
	nfc_logger_init();
	nfc_logger_set_max_count(-1);
#endif

	NFC_LOG_INFO("%s chip select : %d , bus number = %d\n", __func__,
		spi->chip_select, spi->master->bus_num);

	memset(&platform_data1, 0x00, sizeof(struct p61_spi_platform_data));

#if !DRAGON_P61 && !IS_ENABLED(CONFIG_SAMSUNG_NFC)
	platform_data = spi->dev.platform_data;
	if (platform_data == NULL) {
		/* RC : rename the platformdata1 name */
		/* TBD: This is only for Panda as we are passing NULL for platform data */
		NFC_LOG_ERR("%s : p61 probe fail\n", __func__);
		platform_data1.irq_gpio = P61_IRQ;
		platform_data1.rst_gpio = P61_RST;
		platform_data = &platform_data1;
		NFC_LOG_ERR("%s : p61 probe fail1\n", __func__);
		//return  -ENODEV;
	}
#else
	ret = p61_parse_dt(&spi->dev, &platform_data1);
	if (ret) {
		NFC_LOG_ERR("%s - Failed to parse DT\n", __func__);
		goto err_exit;
	}
	platform_data = &platform_data1;
#endif
	p61_dev = kzalloc(sizeof(*p61_dev), GFP_KERNEL);
	if (p61_dev == NULL) {
		NFC_LOG_ERR("failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
	ret = p61_hw_setup(platform_data, p61_dev, spi);
	if (ret < 0) {
		NFC_LOG_ERR("Failed to p61_enable_P61_IRQ_ENABLE\n");
		goto err_exit0;
	}
#endif
	if (platform_data->gpio_coldreset) {
		ret = ese_reset_gpio_setup(platform_data);
		if (ret < 0) {
			P61_ERR_MSG("Failed to setup ese reset gpio");
			goto err_exit0;
		}
	}
	/* gpio cold reset doesn't work. so, set gpio coldreset to false to use i2c cold reset */
	platform_data->gpio_coldreset = false;

	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	ret = of_property_read_u32(np, "spi-max-frequency", &max_speed_hz);
	if (ret < 0) {
		NFC_LOG_ERR("%s: There's no spi-max-frequency property\n", __func__);
		goto err_exit0;
	}
	spi->max_speed_hz = max_speed_hz;
	NFC_LOG_INFO("%s : spi max_speed_hz = %d\n", __func__, spi->max_speed_hz);
	//spi->chip_select = SPI_NO_CS;
	ret = spi_setup(spi);
	if (ret < 0) {
		NFC_LOG_ERR("failed to do spi_setup()\n");
		goto err_exit0;
	}

	p61_dev->spi = spi;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	p61_parse_pinctrl_dt(&spi->dev, p61_dev);
#endif
#ifndef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	p61_dev->p61_device.minor = MISC_DYNAMIC_MINOR;
	p61_dev->p61_device.name = "p61";
	p61_dev->p61_device.fops = &p61_dev_fops;
	p61_dev->p61_device.parent = &spi->dev;
#endif
	p61_dev->irq_gpio = platform_data->irq_gpio;
	p61_dev->rst_gpio = platform_data->rst_gpio;
	p61_dev->trusted_ese_gpio = platform_data->trusted_ese_gpio;
	p61_dev->gpio_coldreset = platform_data->gpio_coldreset;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	p61_dev->ap_vendor = platform_data->ap_vendor;
#else
	p61_dev->trusted_ese_gpio = platform_data->trusted_ese_gpio;
#endif
	p61_dev->ese_spi_transition_state = ESE_SPI_IDLE;
	dev_set_drvdata(&spi->dev, p61_dev);

	/* init mutex and queues */
	init_waitqueue_head(&p61_dev->read_wq);
	mutex_init(&p61_dev->read_mutex);
	mutex_init(&p61_dev->write_mutex);
	if (p61_dev->gpio_coldreset)
		ese_reset_init();

#ifdef P61_IRQ_ENABLE
	spin_lock_init(&p61_dev->irq_enabled_lock);
#endif
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	p61_dev->nfc_node = of_parse_phandle(np, "nxp,nfcc", 0);
	if (!p61_dev->nfc_node) {
		NFC_LOG_ERR("%s: nxp,nfcc invalid or missing in device tree\n", __func__);
		goto err_exit0;
	}
#else
	ret = of_property_read_string(np, "nxp,nfcc", &p61_dev->nfcc_name);
	if (ret < 0) {
		NFC_LOG_ERR("%s: nxp,nfcc invalid or missing in device tree (%d)\n",
			__func__, ret);
		goto err_exit0;
	}
	NFC_LOG_INFO("%s: device tree set '%s' as eSE power controller\n",
			__func__, p61_dev->nfcc_name);
#endif

#ifndef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	ret = misc_register(&p61_dev->p61_device);
	if (ret < 0) {
		NFC_LOG_ERR("misc_register failed! %d\n", ret);
		goto err_exit0;
	}
#endif
#ifdef P61_IRQ_ENABLE
	p61_dev->spi->irq = gpio_to_irq(platform_data->irq_gpio);

	if (p61_dev->spi->irq < 0) {
		NFC_LOG_ERR("gpio_to_irq request failed gpio = 0x%x\n",
			platform_data->irq_gpio);
		goto err_exit1;
	}
	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	p61_dev->irq_enabled = true;
	irq_flags = IRQF_TRIGGER_RISING | IRQF_ONESHOT;

	ret = request_irq(p61_dev->spi->irq, p61_dev_irq_handler, irq_flags,
			p61_dev->p61_device.name, p61_dev);
	if (ret) {
		NFC_LOG_ERR("request_irq failed\n");
		goto err_exit1;
	}
	p61_disable_irq(p61_dev);

#endif
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	p61_dev->r_buf = kzalloc(sizeof(unsigned char) * MAX_BUFFER_SIZE, GFP_KERNEL);
	if (p61_dev->r_buf == NULL) {
		NFC_LOG_ERR("failed to allocate for spi read buffer\n");
		ret = -ENOMEM;
		goto err_exit2;
	}

	p61_dev->w_buf = kzalloc(sizeof(unsigned char) * MAX_BUFFER_SIZE, GFP_KERNEL);
	if (p61_dev->w_buf == NULL) {
		NFC_LOG_ERR("failed to allocate for spi write buffer\n");
		ret = -ENOMEM;
		goto err_exit3;
	}
	wake_lock_init(&p61_dev->ese_lock, WAKE_LOCK_SUSPEND, "ese_lock");
#if IS_ENABLED(CONFIG_SPI_MSM_GENI)
	INIT_DELAYED_WORK(&p61_dev->spi_release_work, p61_spi_release_work);
	wake_lock_init(&p61_dev->spi_release_wakelock, WAKE_LOCK_SUSPEND, "ese_spi_wake_lock");
#endif
#endif

	p61_dev->enable_poll_mode = 0;	/* Default IRQ read mode */
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	g_p61_dev = p61_dev;
#endif
	NFC_LOG_INFO("Exit : %s\n", __func__);
	return ret;
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
err_exit3:
	kfree(p61_dev->r_buf);
err_exit2:
#endif
#ifdef P61_IRQ_ENABLE
err_exit1:
#ifndef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	misc_deregister(&p61_dev->p61_device);
#endif
#endif
err_exit0:
	mutex_destroy(&p61_dev->read_mutex);
	mutex_destroy(&p61_dev->write_mutex);
	if (p61_dev->gpio_coldreset)
		ese_reset_deinit();
	if (p61_dev != NULL)
		kfree(p61_dev);
err_exit:
	NFC_LOG_ERR("ERROR: Exit : %s ret %d\n", __func__, ret);
	return ret;
}

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
static struct miscdevice p61_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "p61",
	.fops = &p61_dev_fops,
};

static int p61_platform_probe(struct platform_device *pdev)
{
	int ret = -1;

	ret = misc_register(&p61_misc_device);
	if (ret < 0)
		NFC_LOG_ERR("misc_register failed! %d\n", ret);

	NFC_LOG_INFO("%s: finished...\n", __func__);
	return 0;
}

static int p61_platform_remove(struct platform_device *pdev)
{
	NFC_LOG_INFO("Entry : %s\n", __func__);
	return 0;
}

static const struct of_device_id p61_secure_match_table[] = {
	{ .compatible = "p61_platform",},
	{},
};

static struct platform_driver p61_platform_driver = {
	.driver = {
		.name = "p61_platform",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = p61_secure_match_table,
#endif
	},
	.probe =  p61_platform_probe,
	.remove = p61_platform_remove,
};
#endif /* CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE */

/**
 * \ingroup spi_driver
 * \brief Will get called when the device is removed to release the resources.
 *
 * \param[in]       struct spi_device
 *
 * \retval 0 if ok.
 *
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int p61_remove(struct spi_device *spi)
#else
static void p61_remove(struct spi_device *spi)
#endif
{
	struct p61_dev *p61_dev = p61_get_data(spi);
	P61_DBG_MSG("Entry : %s\n", __func__);

#ifdef P61_HARD_RESET
	if (p61_regulator != NULL) {
		regulator_disable(p61_regulator);
		regulator_put(p61_regulator);
	} else {
		NFC_LOG_ERR("ERROR %s p61_regulator not enabled\n", __func__);
	}
#endif
	if (p61_dev != NULL) {
		gpio_free(p61_dev->rst_gpio);

#ifdef P61_IRQ_ENABLE
		free_irq(p61_dev->spi->irq, p61_dev);
		gpio_free(p61_dev->irq_gpio);
#endif
#if !IS_ENABLED(CONFIG_SAMSUNG_NFC)
		if (gpio_is_valid(p61_dev->trusted_ese_gpio)) {
			gpio_free(p61_dev->trusted_ese_gpio);
		}
#endif

		mutex_destroy(&p61_dev->read_mutex);
#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
		misc_deregister(&p61_misc_device);
#else
		misc_deregister(&p61_dev->p61_device);
#endif
		if (p61_dev->gpio_coldreset)
			ese_reset_deinit();
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
		wake_lock_destroy(&p61_dev->ese_lock);
#endif
		kfree(p61_dev);
	}
	P61_DBG_MSG("Exit : %s\n", __func__);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
        return 0;
#endif
}

#if DRAGON_P61
static struct of_device_id p61_dt_match[] = {
	{
		.compatible = "p61",
	},
	{}
};
#endif
static struct spi_driver p61_driver = {
	.driver = {
		.name = "p61",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
#if DRAGON_P61
		.of_match_table = p61_dt_match,
#endif
	},
	.probe = p61_probe,
	.remove = (p61_remove),
};

/**
 * \ingroup spi_driver
 * \brief Module init interface
 *
 * \param[in]       void
 *
 * \retval handle
 *
*/

#if IS_MODULE(CONFIG_SAMSUNG_NFC)
int p61_dev_init(void)
{
	int ret;
	debug_level = P61_DEBUG_OFF;

	P61_DBG_MSG("Entry : %s\n", __func__);

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	ret = platform_driver_register(&p61_platform_driver);
	NFC_LOG_INFO("%s: platform_driver_register, ret %d\n", __func__, ret);
#endif
	ret = spi_register_driver(&p61_driver);
	return ret;
}
EXPORT_SYMBOL(p61_dev_init);

void p61_dev_exit(void)
{
	P61_DBG_MSG("Entry : %s\n", __func__);

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	platform_driver_unregister(&p61_platform_driver);
#endif

	spi_unregister_driver(&p61_driver);
}
EXPORT_SYMBOL(p61_dev_exit);
#else
static int __init p61_dev_init(void)
{
	int ret;

	debug_level = P61_DEBUG_OFF;

	P61_DBG_MSG("Entry : %s\n", __func__);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_get_lpcharge() == LPM_TRUE)
		return 0;
#endif

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	ret = platform_driver_register(&p61_platform_driver);
	NFC_LOG_INFO("%s: platform_driver_register, ret %d\n", __func__, ret);
#endif
	ret = spi_register_driver(&p61_driver);

	return ret;
}

module_init(p61_dev_init);

/**
 * \ingroup spi_driver
 * \brief Module exit interface
 *
 * \param[in]       void
 *
 * \retval void
 *
*/

static void __exit p61_dev_exit(void)
{
	P61_DBG_MSG("Entry : %s\n", __func__);
#if IS_ENABLED(CONFIG_SAMSUNG_NFC)
	if (nfc_get_lpcharge() == LPM_TRUE)
		return;
#endif

#ifdef CONFIG_MAKE_NODE_USING_PLATFORM_DEVICE
	platform_driver_unregister(&p61_platform_driver);
#endif
	spi_unregister_driver(&p61_driver);
}

module_exit(p61_dev_exit);
MODULE_ALIAS("spi:p61");
MODULE_AUTHOR("BHUPENDRA PAWAR");
MODULE_DESCRIPTION("NXP P61 SPI driver");
MODULE_LICENSE("GPL");
#endif
/** @} */
