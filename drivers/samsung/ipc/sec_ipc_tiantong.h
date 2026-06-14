#include <linux/ioctl.h>

#define IOCTL_MAGIC 'G'
#define TT_BOOT_MODE		_IOW(IOCTL_MAGIC, 1, int)
#define TT_RESET_N		_IOW(IOCTL_MAGIC, 2, int)
#define TT_AP2CP_WAKE		_IOW(IOCTL_MAGIC, 3, int)
#define TT_POWER_32K		_IOW(IOCTL_MAGIC, 4, int)
#define TT_POWER_19P2M		_IOW(IOCTL_MAGIC, 5, int)
#define IOCTL_CHECK_REGION	_IOR(IOCTL_MAGIC, 7, int)

#define IOCTL_MAGIC_RIL	'o'
#define IOCTL_POWER_ON			_IO(IOCTL_MAGIC_RIL, 0x19)
#define IOCTL_POWER_OFF			_IO(IOCTL_MAGIC_RIL, 0x20)
#define IOCTL_POWER_RESET		_IO(IOCTL_MAGIC_RIL, 0x21)

#define DEVICE_NAME "tt_control"

/*This variable must explicitly be initialized to 0 */
static const unsigned int MINOR_BASE;
static const unsigned int MINOR_NUM = 1;

const char *tiantong_bootmode1_str = "tiantong_bootmode1";
const char *tiantong_ap2cp_wakeup_str = "tiantong_ap2cp_wakeup";
const char *tiantong_cp2ap_wakeup_str = "tiantong_cp2ap_wakeup";
const char *tiantong_reset_str = "tiantong_reset";
const char *tiantong_vrf_1p8_str = "tiantong_vrf_1p8";
const char *tiantong_vrf_1p8_2_str = "tiantong_vrf_1p8_2";
const char *tiantong_vrf_0p8_str = "tiantong_vrf_0p8";
const char *gpio_chn_ht_str = "gpio_chn_ht";

struct tiantong_gpio {
	int bootmode1;
	int reset;
	int ap2cp_wakeup;
	int cp2ap_wakeup;
	int vrf_1p8;
	int vrf_1p8_2;
	int vrf_0p8;
};

static int tiantong_open(struct inode *inode, struct file *file);
static int tiantong_close(struct inode *inode, struct file *file);
static long tiantong_ioctl(struct file *file, unsigned int cmd, unsigned long arg);