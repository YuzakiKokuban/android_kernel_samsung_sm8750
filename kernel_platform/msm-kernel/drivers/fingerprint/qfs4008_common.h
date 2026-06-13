#ifndef _UAPI_QFS4008_H_
#define _UAPI_QFS4008_H_

#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/spi/spidev.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include "fingerprint_common.h"

#undef DISABLED_GPIO_PROTECTION

#define VENDOR "QCOM"
#define CHIP_ID "QFS4008"
#define SPI_CLOCK_MAX 35000000

#define QFS4008_DEV "qbt2000"
#define MAX_FW_EVENTS 128
#define IPC_MSG_ID_CBGE_REQUIRED 29
#define IPC_MSG_ID_FINGER_ON_SENSOR 55
#define IPC_MSG_ID_FINGER_OFF_SENSOR 56
#define QFS4008_WAKELOCK_HOLD_TIME 500

#define MINOR_NUM_FD 0
#define MINOR_NUM_IPC 1

#define FINGER_DOWN_GPIO_STATE 1
#define FINGER_LEAVE_GPIO_STATE 0

enum qfs4008_commands {
	QFS4008_POWER_CONTROL = 21,
	QFS4008_ENABLE_SPI_CLOCK = 22,
	QFS4008_DISABLE_SPI_CLOCK = 23,
	QFS4008_ENABLE_IPC = 24,
	QFS4008_DISABLE_IPC = 25,
	QFS4008_ENABLE_WUHB = 26,
	QFS4008_DISABLE_WUHB = 27,
	QFS4008_CPU_SPEEDUP = 28,
	QFS4008_SET_SENSOR_TYPE = 29,
	QFS4008_SET_LOCKSCREEN = 30,
	QFS4008_SENSOR_RESET = 31,
	QFS4008_SENSOR_TEST = 32,
	QFS4008_GET_MODELINFO = 38,
	QFS4008_SET_TOUCH_IGNORE = 39,
	QFS4008_IS_WHUB_CONNECTED = 105,
};

#define QFS4008_SENSORTEST_DONE		0x0000 // do nothing or test done
#define QFS4008_SENSORTEST_BGECAL	0x0001 // begin the BGECAL
#define QFS4008_SENSORTEST_NORMALSCAN	0x0002 // begin the normalscan
#define QFS4008_SENSORTEST_SNR		0x0004 // begin the snr
#define QFS4008_SENSORTEST_CAPTURE	0x0008 // begin the image capture. it also needs liveness capture.

/*
 * enum qfs4008_fw_event -
 *      enumeration of firmware events
 * @FW_EVENT_FINGER_DOWN - finger down detected
 * @FW_EVENT_FINGER_UP - finger up detected
 * @FW_EVENT_INDICATION - an indication IPC from the firmware is pending
 */
enum qfs4008_fw_event {
	FW_EVENT_FINGER_DOWN = 1,
	FW_EVENT_FINGER_UP = 2,
	FW_EVENT_CBGE_REQUIRED = 3,
};

struct finger_detect_gpio {
	int gpio;
	int active_low;
	int irq;
	struct work_struct work;
	int last_gpio_state;
};

struct fw_event_desc {
	enum qfs4008_fw_event ev;
};

struct fw_ipc_info {
	int gpio;
	int irq;
};

struct qfs4008_drvdata {
	struct class *qfs4008_class;
	struct cdev qfs4008_fd_cdev;
	struct cdev qfs4008_ipc_cdev;
	struct device *dev;
	struct device *fp_device;
	atomic_t fd_available;
	atomic_t ipc_available;
	struct mutex mutex;
	struct mutex ioctl_mutex;
	struct mutex fd_events_mutex;
	struct mutex ipc_events_mutex;
	struct fw_ipc_info fw_ipc;
	struct finger_detect_gpio fd_gpio;
	DECLARE_KFIFO(fd_events, struct fw_event_desc, MAX_FW_EVENTS);
	DECLARE_KFIFO(ipc_events, struct fw_event_desc, MAX_FW_EVENTS);
	wait_queue_head_t read_wait_queue_fd;
	wait_queue_head_t read_wait_queue_ipc;

	int sensortype;
	int cbge_count;
	int wuhb_count;
	int reset_count;
	bool enabled_ipc;
	bool enabled_wuhb;
	bool enabled_ldo;
	bool tz_mode;
	bool touch_ignore;
	const char *model_info;
	const char *chipid;
	struct pinctrl *p;
	struct pinctrl_state *pins_poweron;
	struct pinctrl_state *pins_poweroff;
	const char *sensor_position;
	const char *btp_vdd_1p8;
	const char *btp_vdd_3p0;
	struct regulator *regulator_1p8;
	struct regulator *regulator_3p0;
	struct wakeup_source *fp_signal_lock;
	struct spi_clk_setting *clk_setting;
	struct boosting_config *boosting;
	struct debug_logger *logger;
};

#ifdef CONFIG_BATTERY_SAMSUNG
extern unsigned int lpcharge;
#endif

struct debug_logger *g_logger;

#endif /* _UAPI_QFS4008_H_ */
