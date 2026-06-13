#ifndef __INTERNAL__SEC_RELOC_GPIO_H__
#define __INTERNAL__SEC_RELOC_GPIO_H__

#include <linux/device.h>
#include <linux/samsung/builder_pattern.h>

struct reloc_gpio_chip {
	const char *label;
	size_t label_len;
	int base;
};

struct reloc_gpio_drvdata {
	struct builder bd;
	struct device *reloc_gpio_dev;
	struct reloc_gpio_chip *chip;
	size_t nr_chip;
	/* relocated gpio number which is request from sysfs interface */
	int gpio_num;
	/* found index after calling gpiochip_find */
	int chip_idx_found;
};

#endif /* */
