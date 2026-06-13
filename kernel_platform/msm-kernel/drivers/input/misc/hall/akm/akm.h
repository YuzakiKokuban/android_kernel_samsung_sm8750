
#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/version.h>

#include "../../../sec_input/sec_input.h"
//#include <linux/samsung/sec_class.h>
#include <linux/sec_class.h>
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
#include <linux/hall/hall_ic_notifier.h>
#endif

#define AKM_READ_DEVICE_ID	0x00 /* 4 byte */
#define AKM_READ_STATUS	0x01 /* 1 byte */
#define AKM_READ_STATUS_X_AXIS_Y_AXIS	0x13 /* 5 byte */
#define AKM_READ_STATUS_X_AXIS_Y_AXIS_Z_AXIS	0x17 /* 7 byte */

#define AKM_REG_WORD_ST             0x10
#define AKM_REG_WORD_ST_X           0x11
#define AKM_REG_WORD_ST_Y           0x12
#define AKM_REG_WORD_ST_Y_X         0x13
#define AKM_REG_WORD_ST_Z           0x14
#define AKM_REG_WORD_ST_Z_X         0x15
#define AKM_REG_WORD_ST_Z_Y         0x16
#define AKM_REG_WORD_ST_Z_Y_X       0x17
#define AKM_REG_BYTE_ST_V           0x18
#define AKM_REG_BYTE_ST_X           0x19
#define AKM_REG_BYTE_ST_Y           0x1A
#define AKM_REG_BYTE_ST_Y_X         0x1B
#define AKM_REG_BYTE_ST_Z           0x1C
#define AKM_REG_BYTE_ST_Z_X         0x1D
#define AKM_REG_BYTE_ST_Z_Y         0x1E
#define AKM_REG_BYTE_ST_Z_Y_X       0x1F
#define AKM_REG_CNTL1               0x20
#define AKM_REG_CNTL2               0x21
#define AKM_REG_THRESHOLD_X                 0x22
#define AKM_REG_THRESHOLD_Y                 0x23
#define AKM_REG_THRESHOLD_Z                 0x24
#define AKM_REG_THRESHOLD_V                 0x25
#define AKM_REG_SRST                0x30

#define AKM_AK09973D	0
#define AKM_AK09973DW	1

#define AKM_SPEC_NUMBER				6 /* minX, maxX, minY, maxY, minZ, maxZ */
int selftest_spec_table[2][AKM_SPEC_NUMBER] = {
	{ -120, 120, -120, 120, 60, 400 }, /* AK09973D */
	{ -45, 45, -45, 45, 20, 150 } /* AK09973DW */
};

u8 measurement_mode[9] = { 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10 };
/*                         DOWN  5Hz   10Hz  20Hz  50Hz  100Hz 500Hz 1kHz  2kHz */
struct akm_platform_data {
	int gpio_int;
	int gpio_scl;
	int gpio_sda;
	int gpio_rst;

	struct pinctrl *pinctrl;

	struct regulator *dvdd;
	struct regulator *avdd;
	struct regulator *pullup;

	u8 msrNo;

	/* settings */
	u8 measurement_number;
	/* DRDY event */
	u8 drdy_en;
	/* SW event */
	u8 sw_x_en;
	u8 sw_y_en;
	u8 sw_z_en;
	u8 sw_v_en;
	/* ERR event*/
	u8 err_en;
	/* POL setting*/
	u8 pol_x;
	u8 pol_y;
	u8 pol_z;
	u8 pol_v;
	/* SDR setting */
	u8 sdr;
	/* SMR setting */
	u8 smr;
	/* THRESHOLD setting */
	s16 bop_x;
	s16 brp_x;
	s16 bop_y;
	s16 brp_y;
	s16 bop_z;
	s16 brp_z;
	s16 bop_v;
	s16 brp_v;
	/* device id */
	u8 device_id;
	/* do not calibration from efs, factory or GD Tool */
	bool not_cal_device;
};

struct akm_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct akm_platform_data *pdata;
	struct mutex i2c_lock;
	struct delayed_work dwork;
	struct device *sec_dev;
	struct work_struct probe_work;
	struct workqueue_struct *probe_workqueue;
	bool probe_done;

	u8 mode;

	/* shell command */
	u8 dwork_state;

	u8 state;
	s16 x;
	s16 y;
	s16 z;
	s16 v;

	int cal_value[6];
	bool calibrated;
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
	struct notifier_block hall_ic_nb;
#endif
	struct completion resume_done;
};

static int akm_power_control(struct akm_info *info, bool on);
static int akm_setup_device_control(struct akm_info *info);

