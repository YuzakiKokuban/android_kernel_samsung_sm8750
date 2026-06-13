/*
 * secgpio_dvs.h -- Samsung GPIO debugging and verification system
 */

#ifndef __SECGPIO_DVS_H
#define __SECGPIO_DVS_H

#include <linux/types.h>

enum gdvs_phone_status {
	PHONE_INIT = 0,
	PHONE_SLEEP,
	GDVS_PHONE_STATUS_MAX
};

enum gdvs_io_value {
	GDVS_IO_FUNC = 0x00,
	GDVS_IO_IN,
	GDVS_IO_OUT,
};

enum gdvs_pupd_value {
	GDVS_PUPD_NP = 0x00,
	GDVS_PUPD_PD,
	GDVS_PUPD_PU,
	GDVS_PUPD_KEEPER,
	GDVS_PUPD_ERR = 0x3F
};

struct gpiomap_result {
	unsigned char *init;
	unsigned char *sleep;
};

struct gpio_dvs_t {
	struct gpiomap_result *result;
	unsigned int count;
	bool check_sleep;
	void (*check_gpio_status)(unsigned char phonestate);
};

struct secgpio_dvs_data {
	struct gpio_dvs_t *gpio_dvs;
};

void gpio_dvs_check_initgpio(void);
void gpio_dvs_check_sleepgpio(void);

extern const struct secgpio_dvs_data msm_gpio_dvs_data;
#endif /* __SECGPIO_DVS_H */
