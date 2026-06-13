/*******************************************************************************
* Copyright (c) 2023, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L8 Kernel Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L8 Kernel Driver may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>

#include "vl53l8_k_module.h"
#include "vl53l8_k_driver_config.h"
#include "vl53l8_k_gpio_utils.h"
#include "vl53l8_k_logging.h"
#include "vl53l8_k_ioctl_controls.h"
#include "vl53l8_k_error_converter.h"
#include "vl53l8_k_error_codes.h"
#include "vl53l8_k_ioctl_codes.h"
#include "vl53l8_k_version.h"
#include "vl53l5_results_config.h"
#include "vl53l5_platform.h"
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
#include "vl53l5_platform_init.h"
#endif
#ifdef VL53L8_INTERRUPT
#include "vl53l8_k_interrupt.h"
#endif


#ifdef SS_SUPPORT_SEC_CODE
#include <linux/delay.h>
#include <linux/time.h>
#include "vl53l8_k_range_wait_handler.h"

#include <linux/spi/spidev.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>

#define NUM_OF_ZONE 	64
#define NUM_OF_TARGET	1

#ifdef VL53L8_INTERRUPT
#define MAX_READ_COUNT        45
#else
#define MAX_READ_COUNT        10
#endif

#define ENTER 1
#define END   2

#define NUM_OF_PRINT 8

#define FAIL_GET_RANGE	-1
#define EXCEED_AMBIENT	-2
#define EXCEED_XTALK	-3

#define MAX_FAILING_ZONES_LIMIT_FAIL -452581885
#define NO_VALID_ZONES               -452516349

#define FAC_CAL		1
#define USER_CAL	2

#define DEFAULT_SAMPING_RATE	15
#define MIN_SAMPLING_RATE	5
#define MAX_SAMPLING_RATE	30

#define DEFAULT_INTEGRATION_TIME	5000
#define MIN_INTEGRATION_TIME	2000
#define MAX_INTEGRATION_TIME	15000

#define MODULE_NAME "range_sensor"

static const struct of_device_id vl53l8_k_match_table[] = {
	{ .compatible = "vl53l8",},
	{ .compatible = VL53L8_K_DRIVER_NAME,},
	{},
};

#ifdef CONFIG_SENSORS_VL53L8_SLSI
extern int sensors_register(struct device **dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
extern int sensordump_notifier_register(struct notifier_block *nb);
#endif

#define TEST_300_MM_MAX_ZONE 64
#define TEST_500_MM_MAX_ZONE 16

#endif // SS_SUPPORT_SEC_CODE

static struct spi_device *vl53l8_k_spi_device;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
static struct vl53l8_k_module_t *module_table[VL53L8_CFG_MAX_DEV];
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE

static const struct spi_device_id vl53l8_k_spi_id[] = {
	{ VL53L8_K_DRIVER_NAME, 0 },
	{ },
};
#ifdef SS_SUPPORT_SEC_CODE
#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
void vl53l8_initialize_state_for_resume(struct vl53l8_k_module_t *p_module)
{
	int status = 0;

	usleep_range(1000, 1100);
	p_module->stdev.host_dev.p_fw_buff = NULL;
	status = vl53l5_init(&p_module->stdev);
	if (status < 0) {
		vl53l8_k_log_error("resume init err: %d\n", status);
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, status);
#endif
		p_module->stdev.last_dev_error = VL53L8_RESUME_INIT_ERROR;
		return;
	}

	p_module->stdev.last_dev_error = VL53L5_ERROR_NONE;
	p_module->stdev.status_probe = STATUS_OK;
	p_module->last_driver_error = STATUS_OK;

	p_module->state_preset = VL53L8_STATE_INITIALISED;
	p_module->power_state = VL53L5_POWER_STATE_HP_IDLE;
	vl53l8_k_log_info("VL53L8_STATE_INITIALISED\n");
}

static void resume_work_func(struct work_struct *work)
{
	struct vl53l8_k_module_t *p_module = container_of((struct work_struct *)work,
						struct vl53l8_k_module_t, resume_work);

	vl53l8_initialize_state_for_resume(p_module);
	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_ULP_IDLE);
}
#endif
static int vl53l8_suspend(struct device *dev)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	int status;
#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	cancel_work_sync(&p_module->resume_work);
#endif
	vl53l8_k_log_info("fac en %d, state %d\n",
		p_module->enabled,
		p_module->state_preset);

	p_module->suspend_state = true;

	if (p_module->state_preset >= VL53L8_STATE_INITIALISED) {
		vl53l8_k_log_info("force stop\n");
		status = vl53l8_ioctl_stop(p_module);
		if (status != STATUS_OK) {
			p_module->last_driver_error = VL53L8_SUSPEND_IOCTL_STOP_ERROR;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
			vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
			vl53l8_k_log_info("force stop fail\n");
		} else {
			vl53l8_k_log_info("stop success\n");
		}
		p_module->force_suspend_count++;
	}

	vl53l8_power_onoff(p_module, false);

	return 0;
}

static int vl53l8_resume(struct device *dev)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	p_module->suspend_state = false;
	vl53l8_k_log_info("err %d, force stop count %u\n", p_module->last_driver_error, p_module->force_suspend_count);

	vl53l8_power_onoff(p_module, true);

#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	schedule_work(&p_module->resume_work);
#else
	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_ULP_IDLE);
#endif

	return 0;
}

static const struct dev_pm_ops vl53l8_pm_ops = {
	.suspend = vl53l8_suspend,
	.resume = vl53l8_resume,
};

int set_sampling_rate(struct vl53l8_k_module_t *p_module, uint32_t sampling_rate)
{
	if ((sampling_rate < MIN_SAMPLING_RATE)
		|| (sampling_rate > MAX_SAMPLING_RATE)) {
		vl53l8_k_log_error("Out of Rate\n");
		return -EINVAL;
	}

	p_module->rate = sampling_rate;
	return STATUS_OK;
}

int set_integration_time(struct vl53l8_k_module_t *p_module, uint32_t integration_time)
{
	if ((integration_time < MIN_INTEGRATION_TIME)
		|| (integration_time > MAX_INTEGRATION_TIME)) {
		vl53l8_k_log_error("Out of Integration\n");
		return -EINVAL;
	}

	p_module->integration = integration_time;
	return STATUS_OK;
}
#endif // SS_SUPPORT_SEC_CODE

static struct spi_driver vl53l8_k_spi_driver = {
	.driver = {
		.name	= VL53L8_K_DRIVER_NAME,
		.owner	= THIS_MODULE,
#ifdef SS_SUPPORT_SEC_CODE
		.of_match_table = vl53l8_k_match_table,
		.pm = &vl53l8_pm_ops
#endif // SS_SUPPORT_SEC_CODE

	},
	.probe	= vl53l8_k_spi_probe,
#ifdef SS_SUPPORT_SEC_CODE
	.shutdown = vl53l8_k_spi_shutdown,
#endif // SS_SUPPORT_SEC_CODE
	.remove	= vl53l8_k_spi_remove,
	.id_table = vl53l8_k_spi_id,
};

static const struct file_operations ranging_fops = {
	.owner =		THIS_MODULE,
	.unlocked_ioctl =	vl53l8_k_ioctl,
	.open =			vl53l8_k_open,
	.release =		vl53l8_k_release
};

MODULE_DEVICE_TABLE(of, vl53l8_k_match_table);
MODULE_DEVICE_TABLE(spi, vl53l8_k_spi_id);

static DEFINE_MUTEX(dev_table_mutex);

struct vl53l8_k_module_t *global_p_module;

#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_KERNEL_INTERFACE
enum vl53l8_external_control {
	VL53L8_EXT_START,
	VL53L8_EXT_STOP,
	VL53L8_EXT_GET_RANGE,
	VL53L8_EXT_SET_SAMPLING_RATE,
	VL53L8_EXT_SET_INTEG_TIME,
};

int vl53l8_ext_control(enum vl53l8_external_control input, void *data, u32 *size)
{
	int status = STATUS_OK;
	uint32_t value = 0;

	if (global_p_module == NULL) {
		if ((input == VL53L8_EXT_START)
			|| (input == VL53L8_EXT_STOP))
			vl53l8_k_log_error("probe failed\n");
		if (size != NULL)
			*size = 0;
		return -EPERM;
	} else if ((global_p_module->last_driver_error == VL53L8_PROBE_FAILED)
			|| (global_p_module->last_driver_error == VL53L8_DELAYED_LOAD_FIRMWARE)
			|| (global_p_module->last_driver_error == VL53L8_SHUTDOWN)
			|| (global_p_module->suspend_state == true)) {
		if ((input == VL53L8_EXT_START)
			|| (input == VL53L8_EXT_STOP))
			vl53l8_k_log_error("failed %d\n", global_p_module->last_driver_error);
		else
			if (size)
				*size = 0;
		if (global_p_module->suspend_state == true)
			vl53l8_k_log_error("cmd %d is called in suspend\n", input);
		return -EPERM;
	}

	switch (input) {
	case VL53L8_EXT_START:
		vl53l8_k_log_debug("Lock");
		mutex_lock(&global_p_module->mutex);
		status = vl53l8_ioctl_start(global_p_module);
		if ((status != STATUS_OK) || (global_p_module->last_driver_error != STATUS_OK)) {
			status = vl53l8_ioctl_init(global_p_module);
			vl53l8_k_log_error("fail reset %d\n", status);
			status = vl53l8_ioctl_start(global_p_module);
		}

		if (status != STATUS_OK) {
			vl53l8_k_log_error("start err\n");
			mutex_unlock(&global_p_module->mutex);
			goto out_state;
		} else {
			global_p_module->enabled = 1;
			mutex_unlock(&global_p_module->mutex);
		}
		break;

	case VL53L8_EXT_STOP:
		vl53l8_k_log_debug("Lock");
		mutex_lock(&global_p_module->mutex);
		global_p_module->enabled = 0;
		status = vl53l8_ioctl_stop(global_p_module);
		if ((status != STATUS_OK) || (global_p_module->last_driver_error != STATUS_OK)) {
			status = vl53l8_ioctl_init(global_p_module);
			vl53l8_k_log_error("reset err %d\n", status);
			status = vl53l8_ioctl_stop(global_p_module);
		}

		mutex_unlock(&global_p_module->mutex);
		if (status != STATUS_OK) {
			vl53l8_k_log_error("stop err\n");
			goto out_state;
		} else
			vl53l8_k_log_info("stop\n");
		break;

	case VL53L8_EXT_GET_RANGE:
		if ((global_p_module->state_preset != VL53L8_STATE_RANGING)
			|| (data == NULL)) {
			goto out_state;
		}
		status = vl53l8_ioctl_get_range(global_p_module, NULL);
		if (status != STATUS_OK) {
			*size = 0;
			vl53l8_k_log_error("get_range err %d\n", *size);
			goto out_state;
		} else {
			memcpy(data, &global_p_module->af_range_data, sizeof(global_p_module->af_range_data));
			*size = sizeof(global_p_module->af_range_data);
		}
		break;

	case VL53L8_EXT_SET_SAMPLING_RATE:
		memcpy(&value, data, sizeof(uint32_t));
		status = set_sampling_rate(global_p_module, value);
		break;

	case VL53L8_EXT_SET_INTEG_TIME:
		memcpy(&value, data, sizeof(uint32_t));
		status = set_integration_time(global_p_module, value);
		if (status != VL53L5_ERROR_NONE)
			break;

		status = vl53l8_ioctl_set_integration_time_us(global_p_module, global_p_module->integration);
		if (status != VL53L5_ERROR_NONE)
			vl53l8_k_log_error("Fail %d\n", status);
		break;

	default:
		vl53l8_k_log_error("invalid %d\n", input);
		break;
	}

	return status;
out_state:

	vl53l8_k_log_error("Err %d\n", status);
	return status;
}
EXPORT_SYMBOL_GPL(vl53l8_ext_control);
#endif // CONFIG_SENSORS_VL53L8_SUPPORT_KERNEL_INTERFACE

static void memory_release(struct kref *kref)
{
	struct vl53l8_k_module_t *p_module =
		container_of(kref, struct vl53l8_k_module_t, spi_info.ref);

	LOG_FUNCTION_START("");

	kfree(p_module);

	LOG_FUNCTION_END(0);
}

static int vl53l8_parse_dt(struct device *dev,
			   struct vl53l8_k_module_t *p_module)
{
	struct device_node *np = dev->of_node;
	int status = 0;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("start");
#else // SS_SUPPORT_SEC_CODE
	vl53l8_k_log_info("start\n");
#endif // SS_SUPPORT_SEC_CODE

	if (of_find_property(np, "spi-cpha", NULL))
		p_module->spi_info.device->mode |= SPI_CPHA;
	if (of_find_property(np, "spi-cpol", NULL))
		p_module->spi_info.device->mode |= SPI_CPOL;

	p_module->comms_type = VL53L8_K_COMMS_TYPE_SPI;

	p_module->gpio.interrupt = -1;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	p_module->gpio.comms_select = -1;
	p_module->gpio.power_enable_owned = 0;
	p_module->gpio.power_enable =
			of_get_named_gpio(np, "stm,power_enable", 0);
	if (gpio_is_valid(p_module->gpio.power_enable)) {
		status = gpio_request(p_module->gpio.power_enable,
				      "vl53l8_k_pwren");
		if (!status) {
			vl53l8_k_log_debug(
				"Set gpio %d direction",
				p_module->gpio.power_enable);
			p_module->gpio.power_enable_owned = 1;
			status = gpio_direction_output(
					p_module->gpio.power_enable, 0);
			if (status) {
				vl53l8_k_log_error(
				"Set direction failed %d: %d",
				p_module->gpio.power_enable, status);
			}
		} else
			vl53l8_k_log_error("Request gpio failed %d: %d",
					p_module->gpio.power_enable, status);
	} else
		vl53l8_k_log_info(
			"Power enable is not configured.");

	p_module->gpio.low_power_owned = 0;
	p_module->gpio.low_power = of_get_named_gpio(np, "stm,low_power", 0);
	if (gpio_is_valid(p_module->gpio.low_power)) {
		status = gpio_request(
				p_module->gpio.low_power, "vl53l8_k_lp");
		if (!status) {
			vl53l8_k_log_debug(
					"Set gpio %d direction",
					p_module->gpio.low_power);
			p_module->gpio.low_power_owned = 1;
			status = gpio_direction_output(
						p_module->gpio.low_power, 0);
			if (status) {
				vl53l8_k_log_error(
				"Set direction failed %d: %d",
				p_module->gpio.low_power, status);
			}
		} else
			vl53l8_k_log_error("Request gpio failed %d: %d",
			p_module->gpio.low_power, status);
	} else
		vl53l8_k_log_info(
			"The GPIO setting of Low power is not configured.");

	p_module->gpio.comms_select_owned = 0;
	p_module->gpio.comms_select = of_get_named_gpio(
						np, "stm,comms_select", 0);
	if (gpio_is_valid(p_module->gpio.comms_select)) {
		status = gpio_request(
				p_module->gpio.comms_select, "vl53l8_k_cs");
		if (!status) {
			vl53l8_k_log_debug(
					"Set gpio %d direction",
					p_module->gpio.comms_select);
			p_module->gpio.comms_select_owned = 1;
			status = gpio_direction_output(
						p_module->gpio.comms_select, 0);
			if (status) {
				vl53l8_k_log_error(
				"Set direction failed gpio %d: %d",
				p_module->gpio.comms_select, status);
			}
		} else
			vl53l8_k_log_error("Request gpio failed %d: %d",
			p_module->gpio.comms_select, status);
	} else
		vl53l8_k_log_info(
			"The GPIO setting of comms select is not configured.");
#endif // STM_VL53L8_SUPPORT_LEGACY_CODE
#ifdef VL53L8_INTERRUPT
	p_module->gpio.interrupt_owned = 0;
	p_module->gpio.interrupt = of_get_named_gpio(
						np, "stm,interrupt", 0);
	if (gpio_is_valid(p_module->gpio.interrupt)) {
		status = gpio_request(
				p_module->gpio.interrupt, "vl53l8_k_interrupt");
		if (!status) {
			vl53l8_k_log_debug(
					"Set gpio %d direction",
					p_module->gpio.interrupt);
			p_module->gpio.interrupt_owned = 1;
			status = gpio_direction_input(
						p_module->gpio.interrupt);
			if (status) {
				p_module->gpio.interrupt_owned = 0;
				vl53l8_k_log_error(
				"Set input direction failed gpio %d: %d\n",
				p_module->gpio.interrupt, status);
			}
		} else
			vl53l8_k_log_error("Request gpio failed %d: %d\n",
			p_module->gpio.interrupt, status);
	} else
		vl53l8_k_log_info(
			"The GPIO setting of comms select is not configured.\n");
#endif // VL53L8_INTERRUPT
#ifdef SS_SUPPORT_SEC_CODE
#ifdef CONFIG_SEPARATE_IO_CORE_POWER
	if (of_property_read_string_index(np, "stm,core_vdd", 0,
			(const char **)&p_module->corevdd_vreg_name)) {
		p_module->corevdd_vreg_name = NULL;
	}
	vl53l8_k_log_info("%s core_vdd %s\n", __func__, p_module->corevdd_vreg_name);
#endif // CONFIG_SEPARATE_IO_CORE_POWER
	if (of_property_read_string_index(np, "stm,io_vdd", 0,
			(const char **)&p_module->iovdd_vreg_name)) {
		p_module->iovdd_vreg_name = NULL;
	}
	vl53l8_k_log_info("%s io_vdd %s\n", __func__, p_module->iovdd_vreg_name);

	if (of_property_read_string_index(np, "stm,a_vdd", 0,
			(const char **)&p_module->avdd_vreg_name)) {
		p_module->avdd_vreg_name = NULL;
	}
	vl53l8_k_log_info("%s: a_vdd %s\n", __func__, p_module->avdd_vreg_name);
#endif // SS_SUPPORT_SEC_CODE

	of_property_read_string(np, "stm,firmware_name",
				&p_module->firmware_name);
#ifdef SS_SUPPORT_SEC_CODE
	if (!p_module->firmware_name) {
		vl53l8_k_log_error(
			"%s firmware is not set\n", __func__);
		p_module->firmware_name = "range_sensor/vl53l8.bin";
	}

	of_property_read_string(np, "stm,genshape_name",
				&p_module->genshape_name);
	if (!p_module->genshape_name) {
		vl53l8_k_log_error(
			"%s generic is not set", __func__);
		p_module->genshape_name = "range_sensor/generic_xtalk_shape.bin";
	}

	if (of_property_read_u32(np, "stm,fac_rotation_mode", &p_module->fac_rotation_mode))
		p_module->fac_rotation_mode = 0;
	else
		vl53l8_k_log_info("fac_rotation %d", p_module->fac_rotation_mode);

	if (of_property_read_u32(np, "stm,rotation_mode", &p_module->rotation_mode))
		p_module->rotation_mode = 0;
	else
		vl53l8_k_log_info("rotation %d", p_module->rotation_mode);

	if (of_property_read_u32(np, "stm,integration_time", &p_module->integration_init))
		p_module->integration_init = DEFAULT_INTEGRATION_TIME;
	else
		vl53l8_k_log_info("integ_t %d", p_module->integration_init);

	if (of_property_read_u32(np, "stm,sampling_rate", &p_module->rate_init))
		p_module->rate_init = DEFAULT_SAMPING_RATE;
	else
		vl53l8_k_log_info("rate %d", p_module->rate);

	p_module->rate = p_module->rate_init;
	p_module->integration = p_module->integration_init;

	/* ASZ Setting*/
	if (of_property_read_u32(np, "stm,asz_0_zone_id", &p_module->asz_0_ll_zone_id))
		p_module->asz_0_ll_zone_id = 16;
	else
		vl53l8_k_log_info("asz_0 %d", p_module->asz_0_ll_zone_id);
	if (of_property_read_u32(np, "stm,asz_1_zone_id", &p_module->asz_1_ll_zone_id))
		p_module->asz_1_ll_zone_id = 18;
	else
		vl53l8_k_log_info("asz_1 %d", p_module->asz_1_ll_zone_id);
	if (of_property_read_u32(np, "stm,asz_2_zone_id", &p_module->asz_2_ll_zone_id))
		p_module->asz_2_ll_zone_id = 20;
	else
		vl53l8_k_log_info("asz_2 %d", p_module->asz_2_ll_zone_id);
	if (of_property_read_u32(np, "stm,asz_3_zone_id", &p_module->asz_3_ll_zone_id))
		p_module->asz_3_ll_zone_id = 22;
	else
		vl53l8_k_log_info("asz_3 %d", p_module->asz_3_ll_zone_id);
#endif // SS_SUPPORT_SEC_CODE

	return status;
}

static void vl53l8_k_clean_up_spi(void)
{
	LOG_FUNCTION_START("");
	if (vl53l8_k_spi_device != NULL) {
		vl53l8_k_log_debug("Unregistering spi device");
		spi_unregister_device(vl53l8_k_spi_device);
	}
	LOG_FUNCTION_END(0);
}

#ifdef SS_SUPPORT_SEC_CODE
void vl53l8_k_re_init(struct vl53l8_k_module_t *p_module)
{
	if (p_module->last_driver_error == VL53L8_PROBE_FAILED
		|| p_module->last_driver_error == VL53L8_DELAYED_LOAD_FIRMWARE) {
		int status;

		vl53l8_k_log_error("last err %d", p_module->last_driver_error);
		p_module->last_driver_error = STATUS_OK;
		status = vl53l8_ioctl_init(p_module);

		p_module->enabled = 0;
		if (status != STATUS_OK) {
			vl53l8_k_log_error("fail err %d", status);
			p_module->last_driver_error = VL53L8_PROBE_FAILED;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
			vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
			vl53l8_power_onoff(p_module, false);
		} else {
			status = vl53l8_ioctl_stop(p_module);
			if (status != STATUS_OK)
				vl53l8_k_log_error("stop err");
		}
	}
}

int vl53l8_check_calibration_condition(struct vl53l8_k_module_t *p_module, int seq)
{
	int status = STATUS_OK, prev_state = VL53L8_STATE_RANGING;
	int count = 0;
	int ret = 0;
	int max_ambient = 0, max_xtalk = 0, max_peaksignal = 0;
	int i = 0, j = 0, idx = 0;

	vl53l8_k_log_info("state_preset %d, power_state %d", p_module->state_preset, p_module->power_state);
	if (p_module->state_preset != VL53L8_STATE_RANGING) {
		prev_state = p_module->state_preset;

		status = vl53l8_ioctl_start(p_module);
		if (status != STATUS_OK) {
			status = vl53l8_ioctl_init(p_module);
			vl53l8_k_log_info("restart %d", status);
			status = vl53l8_ioctl_start(p_module);
		}

		if (status == STATUS_OK) {
			p_module->enabled = 1;
		} else {
			vl53l8_k_log_error("start err");
			vl53l8_k_store_error(p_module, status);
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
			vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
			ret = FAIL_GET_RANGE;
			goto out_result;
		}
	}
	msleep(180);

	memset(&p_module->range_data, 0, sizeof(p_module->range_data));

	while ((p_module->range.count == 0) && (count < MAX_READ_COUNT)) {
		usleep_range(10000, 10100);
		++count;
	}

	memcpy(&p_module->range_data.per_zone_results,
			&p_module->range.data.tcpm_0_patch.per_zone_results,
			sizeof(struct vl53l5_range_per_zone_results_t));
	memcpy(&p_module->range_data.per_tgt_results,
			&p_module->range.data.tcpm_0_patch.per_tgt_results,
			sizeof(struct vl53l5_range_per_tgt_results_t));
	memcpy(&p_module->range_data.d16_per_target_data,
			&p_module->range.data.tcpm_0_patch.d16_per_target_data,
			sizeof(struct vl53l5_d16_per_tgt_results_t));

	for (i = 0; i < NUM_OF_PRINT; i++) {
		for (j = 0; j < NUM_OF_PRINT; j++) {
			idx = (i * NUM_OF_PRINT + j);
			if (seq == END && max_xtalk < (p_module->calibration.cal_data.core.pxtalk_grid_rate.cal__grid_data__rate_kcps_per_spad[idx] >> 11))
				max_xtalk = p_module->calibration.cal_data.core.pxtalk_grid_rate.cal__grid_data__rate_kcps_per_spad[idx] >> 11;
			if (max_peaksignal < (p_module->range_data.per_tgt_results.peak_rate_kcps_per_spad[idx * NUM_OF_TARGET] >> 11))
				max_peaksignal = p_module->range_data.per_tgt_results.peak_rate_kcps_per_spad[idx * NUM_OF_TARGET] >> 11;
			if (max_ambient < (p_module->range_data.per_zone_results.amb_rate_kcps_per_spad[idx] >> 11))
				max_ambient = p_module->range_data.per_zone_results.amb_rate_kcps_per_spad[idx] >> 11;
		}
	}

	p_module->max_peak_signal = max_peaksignal;

	if (max_ambient > 35)
		ret = EXCEED_AMBIENT;
	else if (seq == END && max_xtalk > 1200)
		ret = EXCEED_XTALK;

	vl53l8_k_log_info("max: ambient %d xtalk %d peak %d ret %d\n", max_ambient, max_xtalk, max_peaksignal, ret);

out_result:
	if (prev_state > VL53L8_STATE_LOW_POWER) {
		p_module->enabled = 0;
		vl53l8_ioctl_stop(p_module);
	}

	return ret;
}


int vl53l8_perform_open_calibration(struct vl53l8_k_module_t *p_module)
{
	int ret;

	vl53l8_k_log_info("Do open CAL\n");
	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);
	ret = vl53l8_perform_calibration(p_module, 1);
	usleep_range(5000, 5100);
	vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_ULP_IDLE);

	return ret;
}
#if IS_ENABLED(CONFIG_SENSORS_VL53L8_QCOM) || IS_ENABLED(CONFIG_SENSORS_VL53L8_SLSI)
#define DISTANCE_300MM_MIN_SPEC 255
#define DISTANCE_300MM_MAX_SPEC 345

static ssize_t vl53l8_distance_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t i = 0, j = 0, idx = 0;
	int min = 0, max = 0,  avg = 0;
	int status = STATUS_OK;
#ifdef VL53L8_INTERRUPT
	int count = 0;
#endif

	p_module->read_data_valid = false;
#ifdef SS_SUPPORT_SEC_CODE
	if ((p_module->state_preset != VL53L8_STATE_RANGING)
		|| (p_module->last_driver_error == VL53L8_PROBE_FAILED)) {
		vl53l8_k_log_error("state %d err %d", p_module->state_preset, p_module->last_driver_error);
#ifdef SS_TEST_ENABLE
		msleep(50);
		panic("probe err %d", p_module->last_driver_error);
#endif
		goto out;
	}
#endif

#ifdef VL53L8_INTERRUPT
	while ((p_module->range.count == 0) && (count < MAX_READ_COUNT)) {
		usleep_range(10000, 10100);
		++count;
	}
#endif

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	vl53l8_k_log_info("get_range %d", p_module->fac_rotation_mode);
	status = vl53l8_ioctl_get_range(p_module, NULL);

	vl53l8_k_log_debug("unLock");
	mutex_unlock(&p_module->mutex);
#endif

	if (status != STATUS_OK) {
		vl53l8_k_log_info("get_range err %d", status);
#ifdef VL53L5_TCDM_ENABLE

			_tcdm_dump(p_module);

#endif
#ifdef SS_TEST_ENABLE
		msleep(50);
		panic("get_range err %d", p_module->last_driver_error);
#endif
	} else {
		vl53l8_k_log_info("get_range %d\n", p_module->print_counter);
		p_module->read_data_valid = true;
	}
#ifdef SS_SUPPORT_SEC_CODE
out:
#endif // SS_SUPPORT_SEC_CODE
	mutex_lock(&p_module->mutex);
	memcpy(&p_module->range_data.per_zone_results,
			&p_module->range.data.tcpm_0_patch.per_zone_results,
			sizeof(struct vl53l5_range_per_zone_results_t));
	memcpy(&p_module->range_data.per_tgt_results,
			&p_module->range.data.tcpm_0_patch.per_tgt_results,
			sizeof(struct vl53l5_range_per_tgt_results_t));
	memcpy(&p_module->range_data.d16_per_target_data,
			&p_module->range.data.tcpm_0_patch.d16_per_target_data,
			sizeof(struct vl53l5_d16_per_tgt_results_t));
	mutex_unlock(&p_module->mutex);

	if (p_module->read_data_valid) {
		for (i = 0; i < p_module->print_counter; i++) {
			for (j = 0; j < p_module->print_counter; j++) {
				uint16_t confidence;

				idx = (i * p_module->print_counter + j);
				confidence = (p_module->range_data.d16_per_target_data.depth16[idx] & 0xE000U) >> 13;

				if (0 == confidence || 5 <= confidence)
					p_module->data[idx] = p_module->range_data.d16_per_target_data.depth16[idx] & 0x1FFFU;
				else
					p_module->data[idx] = 0;
			}
		}
	} else {
		for (i = 0 ; i < NUM_OF_ZONE; i++)
			p_module->data[i] = -1;
	}

	vl53l8_rotate_report_data_int32(p_module->data, p_module->fac_rotation_mode);

	min = max = p_module->data[1];

	for (i = 1; i < NUM_OF_ZONE; i++) {
		if ((i == 7) || (i == 56) || (i == 63))
			continue;
		if (p_module->data[i] < min)
			min = p_module->data[i];
		if (max < p_module->data[i])
			max = p_module->data[i];
		avg += p_module->data[i];
	}

	for (i = 0; i < 64; i += 8) {
		vl53l8_k_log_info("%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}

	if (NUM_OF_ZONE - 4 != 0)
		avg = avg / (NUM_OF_ZONE - 4);
	else
		avg = 0;

	i = j = 0;

	j += snprintf(buf + j, PAGE_SIZE - j, "Distance,%d,%d,%d,%d,%d,",
		DISTANCE_300MM_MIN_SPEC, DISTANCE_300MM_MAX_SPEC, min, max, avg);
	for (; i < TEST_300_MM_MAX_ZONE; i++) {
		if ((i != 0) && (i != 7) && (i != 56) && (i != 63))
			j += snprintf(buf + j, PAGE_SIZE - j, "%d", p_module->data[i]);
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "N");

		if (i != p_module->number_of_zones - 1)
			j += snprintf(buf + j, PAGE_SIZE - j, ",");
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "\n");
	}

	return j;
}


static ssize_t vl53l8_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", p_module->enabled);
}

static ssize_t vl53l8_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	u8 val;
	int status;


	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	vl53l8_k_re_init(p_module);

	if (p_module->last_driver_error <= VL53L8_PROBE_FAILED) {
		vl53l8_k_log_error("enable err %d", p_module->last_driver_error);
#ifdef SS_TEST_ENABLE
		msleep(50);
		panic("enable err %d", p_module->last_driver_error);
#endif
		vl53l8_k_log_debug("unLock");
		mutex_unlock(&p_module->mutex);
		return count;
	}

	status = kstrtou8(buf, 10, &val);

	switch (val) {
	case 1:
		vl53l8_k_log_info("enable start\n");
		if (p_module->enabled < 0 || p_module->last_driver_error == VL53L8_PROBE_FAILED)
			vl53l8_k_log_info("probe fail\n");
		else {
			status = vl53l8_ioctl_start(p_module);

#ifdef SS_TEST_ENABLE
			if (status == STATUS_OK) {
				vl53l8_k_log_error("test for reset");
				status = -EPERM;
			}
#endif
			if (status != STATUS_OK) {
				status = vl53l8_ioctl_init(p_module);
				vl53l8_k_log_error("reset err %d", status);
				status = vl53l8_ioctl_start(p_module);
			}

			if (status == STATUS_OK)
				p_module->enabled = 1;
			else {
				vl53l8_k_log_error("start err");
				vl53l8_k_store_error(p_module, status);
#ifdef SS_TEST_ENABLE
				msleep(50);
				panic("enable err %d", p_module->last_driver_error);
#endif
			}
		}
		vl53l8_k_log_info("enable done\n");
		break;
	default:
		vl53l8_k_log_info("disable start\n");
		if (p_module->enabled < 0 || p_module->last_driver_error == VL53L8_PROBE_FAILED)
			vl53l8_k_log_info("probe err\n");
		else {
			p_module->enabled = 0;
			status = vl53l8_ioctl_stop(p_module);
#ifdef SS_TEST_ENABLE
			if (status == STATUS_OK) {
				vl53l8_k_log_error("test for reset");
				status = -EPERM;
			}
#endif
			if (status != STATUS_OK) {
				status = vl53l8_ioctl_init(p_module);
				vl53l8_k_log_error("reset err%d", status);
				status = vl53l8_ioctl_stop(p_module);
			}

			if (status != STATUS_OK) {
				vl53l8_k_store_error(p_module, status);
#ifdef SS_TEST_ENABLE
				msleep(50);
				panic("disable err %d", p_module->last_driver_error);
#endif
			}

		}
		vl53l8_k_log_info("disable done\n");
		break;
	}

	vl53l8_k_log_debug("unLock");
	mutex_unlock(&p_module->mutex);
	return count;
}

// JUSHIN : add for input sys for STM
static ssize_t vl53l8_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	if (p_module->read_p2p_cal_data == true)
		return snprintf(buf, PAGE_SIZE, "O\n");
	else
		return snprintf(buf, PAGE_SIZE, "X\n");
}

int vl53l8_reset(struct vl53l8_k_module_t *p_module)
{
	int status = STATUS_OK;

	status = vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_ULP_IDLE);
	if (status != STATUS_OK) {
		vl53l8_k_log_error("Set ulp err");
		goto out;
	}
	usleep_range(2000, 2100);

	status = vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);
	if (status != STATUS_OK) {
		vl53l8_k_log_error("Set hp err");
		goto out;
	}

	if (p_module->load_calibration) {
		vl53l8_ioctl_read_generic_shape(p_module);
		if (status != STATUS_OK)
			vl53l8_k_log_error("generic set err");
		usleep_range(1000, 1100);
	}

	usleep_range(1000, 1100);
	return status;
out:
	vl53l8_k_log_error("Reset err %d", status);
	vl53l8_k_store_error(p_module, status);
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
	return status;
}

static ssize_t vl53l8_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	u8 val = 0;
	int ret;

	ret = kstrtou8(buf, 10, &val);

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	switch (val) {
	case 0: /* Calibration Load */
		vl53l8_k_re_init(p_module);
		if (p_module->last_driver_error <= VL53L8_PROBE_FAILED) {
			vl53l8_k_log_error("err %d", p_module->last_driver_error);
#ifdef SS_TEST_ENABLE
			panic("failed %d", p_module->last_driver_error);
#endif
			mutex_unlock(&p_module->mutex);
			return count;
		}
		break;

	case 1:  /* Factory Calibration */
		vl53l8_k_log_info("Do CAL start");
		if (p_module->last_driver_error <= VL53L8_PROBE_FAILED) {
			vl53l8_k_log_error("err %d", p_module->last_driver_error);
			vl53l8_k_re_init(p_module);
		}

		vl53l8_reset(p_module);

		ret = vl53l8_perform_calibration(p_module, 0);
		vl53l8_k_log_info("Do CAL %d\n", p_module->load_calibration);
		if (ret != STATUS_OK) {
			vl53l8_k_log_info("Do CAL err %d\n", ret);
			p_module->read_p2p_cal_data = false;
		} else {
			p_module->load_calibration = true;
			ret = vl53l8_ioctl_read_p2p_calibration(p_module);
			vl53l8_k_log_info("Do CAL Success\n");
			if (ret == STATUS_OK)
				p_module->read_p2p_cal_data = true;
			else
				p_module->read_p2p_cal_data = false;
		}
		break;

	case 2:  /* Cal Erase */
		vl53l8_k_log_info("Erase CAL\n");
		val = 0;

		mutex_lock(&p_module->cal_mutex);

		memset(&p_module->cal_data, 0, sizeof(struct vl53l8_cal_data_t));
		memcpy(p_module->cal_data.pcal_data, &val, 1);
		p_module->cal_data.size = 1;
		p_module->cal_data.cmd = CMD_WRITE_CAL_FILE;

		mutex_unlock(&p_module->cal_mutex);

		vl53l8_input_report(p_module, 2, p_module->cal_data.cmd);

		p_module->read_p2p_cal_data = false;
		vl53l8_reset(p_module);
		break;

	case 3:
		ret = vl53l8_perform_open_calibration(p_module);
		if (ret < 0)
			vl53l8_k_log_error("Do open cal err\n");
		break;

	case 4: /* Calibration Load */
		ret = vl53l8_load_open_calibration(p_module);
		if (ret < 0)
			vl53l8_k_log_error("Load open cal err\n");
		break;

	default:
		vl53l8_k_log_info("Not support %d\n", val);
		break;
	}

	ret = vl53l8_ioctl_stop(p_module);
	if (ret != STATUS_OK)
		vl53l8_k_log_error("stop err");

	vl53l8_k_log_debug("unLock");
	mutex_unlock(&p_module->mutex);

	return count;
}

#define CAL_XTALK_MIN_SPEC 0
#define CAL_XTALK_MAX_SPEC 1200

static ssize_t vl53l8_cal_xtalk_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t idx = 0;
	int32_t min = 0;
	int32_t max = 0;
	int32_t avg = 0;

	for (i = 0; i < p_module->print_counter; i++) {
		for (j = 0; j < p_module->print_counter; j++) {
			idx = (i * p_module->print_counter + j);
			if (p_module->read_p2p_cal_data == true)
				p_module->data[idx] = p_module->calibration.cal_data.core.pxtalk_grid_rate.cal__grid_data__rate_kcps_per_spad[idx] >> 11;
			else
				p_module->data[idx] = -1;
		}
	}

	vl53l8_rotate_report_data_int32(p_module->data, p_module->fac_rotation_mode);

	min = max = avg = p_module->data[0];
	for (i = 1; i < NUM_OF_ZONE; i++) {
		if (p_module->data[i] < min)
			min = p_module->data[i];
		if (max < p_module->data[i])
			max = p_module->data[i];
		avg += p_module->data[i];
	}

	if (p_module->read_p2p_cal_data == true)
		avg = avg / p_module->number_of_zones;
	else
		avg = 0;

	i = j = 0;
	j += snprintf(buf + j, PAGE_SIZE - j, "crosstalk,%d,%d,%d,%d,%d,",
		CAL_XTALK_MIN_SPEC, CAL_XTALK_MAX_SPEC, min, max, avg);
	for (; i < NUM_OF_ZONE; i++) {
		j += snprintf(buf + j, PAGE_SIZE - j, "%d", p_module->data[i]);
		if (i != p_module->number_of_zones - 1)
			j += snprintf(buf + j, PAGE_SIZE - j, ",");
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "\n");
	}

	return j;
}

#define MIN_CAL_OFFSET_SPEC -500
#define MAX_CAL_OFFSET_SPEC 500

static ssize_t vl53l8_cal_offset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t i = 0, j = 0, idx = 0;
	int min = 0, max = 0,  avg = 0;

	for (i = 0; i < p_module->print_counter; i++) {
		for (j = 0; j < p_module->print_counter; j++) {
			idx = (i * p_module->print_counter + j);
			if (p_module->read_p2p_cal_data == true)
				p_module->data[idx] = p_module->calibration.cal_data.core.poffset_grid_offset.cal__grid_data__range_mm[idx] >> 2;
			else
				p_module->data[idx] = -999999;
		}
	}

	vl53l8_rotate_report_data_int32(p_module->data, p_module->fac_rotation_mode);

	min = max = avg = p_module->data[0];
	for (i = 1; i < NUM_OF_ZONE; i++) {
		if (p_module->data[i] < min)
			min = p_module->data[i];
		if (max < p_module->data[i])
			max = p_module->data[i];
		avg += p_module->data[i];
	}

	if (p_module->read_p2p_cal_data == true)
		avg = avg / p_module->number_of_zones;
	else
		avg = 0;

	i = j = 0;
	j += snprintf(buf + j, PAGE_SIZE - j, "offset,%d,%d,%d,%d,%d,",
		MIN_CAL_OFFSET_SPEC, MAX_CAL_OFFSET_SPEC, min, max, avg);
	for (; i < NUM_OF_ZONE; i++) {
		j += snprintf(buf + j, PAGE_SIZE - j, "%d", p_module->data[i]);
		if (i != p_module->number_of_zones - 1)
			j += snprintf(buf + j, PAGE_SIZE - j, ",");
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "\n");
	}

	return j;
}

static ssize_t vl53l8_open_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	int ret = 0;
	int status = 0;
	int cal_status;

	mutex_lock(&p_module->mutex);
	cal_status = vl53l8_check_calibration_condition(p_module, ENTER);
	if (cal_status < 0) {
		mutex_unlock(&p_module->mutex);
		vl53l8_k_log_info("Cal Condition err\n");
		p_module->last_driver_error = VL53L8_OPENCAL_ERROR;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
		if (cal_status == FAIL_GET_RANGE)
			return snprintf(buf, PAGE_SIZE, "FAIL_MODULE_CONNECTOR\n");
		else
			return snprintf(buf, PAGE_SIZE, "FAIL_AMBIENT\n");
	}
	ret = vl53l8_perform_open_calibration(p_module);
	if (ret == STATUS_OK)
		ret = vl53l8_load_open_calibration(p_module);

	cal_status = vl53l8_check_calibration_condition(p_module, END);
	if (cal_status < 0 || ret < 0) {
		vl53l8_k_log_info("cal err\n");
		status = vl53l8_input_report(p_module, 7, CMD_DELTE_OPENCAL_FILE);
		if (status < 0)
			vl53l8_k_log_error("delete cal err\n");

		mutex_unlock(&p_module->mutex);
		p_module->last_driver_error = VL53L8_OPENCAL_ERROR;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
		vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
		if (cal_status == EXCEED_AMBIENT)
			return snprintf(buf, PAGE_SIZE, "FAIL_AMBIENT\n");
		else if ((ret == MAX_FAILING_ZONES_LIMIT_FAIL || ret == NO_VALID_ZONES) && p_module->max_peak_signal > 200)
			return snprintf(buf, PAGE_SIZE, "FAIL_CLOSED\n");
		else if ((ret == MAX_FAILING_ZONES_LIMIT_FAIL || ret == NO_VALID_ZONES) && p_module->max_peak_signal <= 200)
			return snprintf(buf, PAGE_SIZE, "FAIL_BG\n");
		else if (cal_status == EXCEED_XTALK)
			return snprintf(buf, PAGE_SIZE, "FAIL_CLOSED_DUST\n");
		else
			return snprintf(buf, PAGE_SIZE, "FAIL_MODULE_CONNECTOR\n");
	}

	mutex_unlock(&p_module->mutex);
	vl53l8_k_log_info("cal done\n");
	return snprintf(buf, PAGE_SIZE, "PASS\n");

}


#ifdef STM_VL53L5_FORCE_ERROR_COMMAND
static ssize_t vl53l8_force_error(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int status = STATUS_OK;

	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	vl53l8_set_device_parameters(p_module, VL53L5_CFG__FORCE_ERROR);

	return status;
}
#endif
static ssize_t vl53l8_integration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%i\n", p_module->integration);

}

static ssize_t vl53l8_integration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t integration = 0;
	int ret;

	ret = kstrtou32(buf, 10, &integration);

	if (ret) {
		vl53l8_k_log_error("Invalid\n");
		return count;
	}

	ret = set_integration_time(p_module, integration);
	if (ret != STATUS_OK)
		return count;
	mutex_lock(&p_module->mutex);
	vl53l8_ioctl_set_integration_time_us(p_module, p_module->integration);
	mutex_unlock(&p_module->mutex);
	return count;
}

static ssize_t vl53l8_spi_clock_speed_hz(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	unsigned int freq = 0;
	int ret = 0;

	ret = kstrtou32(buf, 10, &freq);
	if (ret < 0) {
		vl53l8_k_log_error("Invalid\n");
		return count;
	}
	vl53l8_k_log_debug("spi clock %d", freq);

	mutex_lock(&p_module->mutex);
	vl53l8_set_transfer_speed_hz(p_module, freq);
	mutex_unlock(&p_module->mutex);

	return count;
}

static ssize_t vl53l8_ranging_rate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%i\n", p_module->rate);
}

static ssize_t vl53l8_ranging_rate_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t rate = 0;
	int ret;

	ret = kstrtou32(buf, 10, &rate);
	if (ret) {
		vl53l8_k_log_error("Invalid\n");
		return count;
	}

	ret = set_sampling_rate(p_module, rate);
	if (ret != STATUS_OK)
		return count;

	if (p_module->state_preset == VL53L8_STATE_RANGING) {
		vl53l8_k_log_info("Skip set rate");
		return count;
	}

	p_module->rate = rate;
	return count;
}

static ssize_t vl53l8_asz_tuning_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "asz_0:%i, asz_1:%i, asz_2:%i, asz_3:%i\n",
		p_module->asz_0_ll_zone_id, p_module->asz_1_ll_zone_id,
		p_module->asz_2_ll_zone_id, p_module->asz_3_ll_zone_id);

}

static ssize_t vl53l8_asz_tuning_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	int ret;
	uint32_t asz_0 = 0, asz_1 = 0, asz_2 = 0, asz_3 = 0;

	LOG_FUNCTION_START("");

	ret = sscanf(buf, "%d %d %d %d", &asz_0, &asz_1, &asz_2, &asz_3);
	if (ret < 0) {
		vl53l8_k_log_error("Invalid\n");
		return count;
	}

	vl53l8_k_log_info("ASZ %d %d %d %d", asz_0, asz_1, asz_2, asz_3);

	p_module->asz_0_ll_zone_id = asz_0;
	p_module->asz_1_ll_zone_id = asz_1;
	p_module->asz_2_ll_zone_id = asz_2;
	p_module->asz_3_ll_zone_id = asz_3;

	return count;
}

// Samsung specific code
#ifdef SS_SUPPORT_SEC_CODE
static ssize_t vl53l8_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "vl53l8\n");
}

static ssize_t vl53l8_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "STM\n");
}

static ssize_t vl53l8_firmware_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	struct vl53l8_k_version_t p_version;
	int status = STATUS_OK;
	enum vl53l8_k_state_preset prev_state;

	prev_state = p_module->state_preset;

	mutex_lock(&p_module->mutex);
	if (p_module->state_preset <= VL53L8_STATE_LOW_POWER)
		vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);

	status = vl53l8_get_firmware_version(p_module, &p_version);

	vl53l8_k_log_info("fw read %d", status);

	if (prev_state <= VL53L8_STATE_LOW_POWER)
		vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_ULP_IDLE);
	mutex_unlock(&p_module->mutex);

	return snprintf(buf, PAGE_SIZE, "FW:%d.%d.%d.%d, Patch:%d.%d, Config:%d.%d\n",
		p_version.driver.firmware.ver_major,
		p_version.driver.firmware.ver_minor,
		p_version.driver.firmware.ver_build,
		p_version.driver.firmware.ver_revision,
		p_version.patch.patch_version.ver_major,
		p_version.patch.patch_version.ver_minor,
		p_version.bin_version.config_ver_major,
		p_version.bin_version.config_ver_minor);
}

#define MIN_TEMP_SPEC 10
#define MAX_TEMP_SPEC 80

static ssize_t vl53l8_temp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	int prev_state = VL53L8_STATE_RANGING, count = 7;
	int8_t temp = -100;

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	vl53l8_k_log_info("state_preset %d, power_state %d", p_module->state_preset, p_module->power_state);
	if (p_module->state_preset != VL53L8_STATE_RANGING) {
		prev_state = p_module->state_preset;
		vl53l8_ioctl_start(p_module);
	}
	p_module->range.data.tcpm_0_patch.meta_data.silicon_temp_degc = temp;
	vl53l8_k_log_debug("unLock");
	mutex_unlock(&p_module->mutex);

	msleep(130);

	while (count > 0) {
		if (p_module->range.data.tcpm_0_patch.meta_data.silicon_temp_degc == temp)
			msleep(20);
		else
			break;
		count--;
	}

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);

	temp = p_module->range.data.tcpm_0_patch.meta_data.silicon_temp_degc;
	vl53l8_k_log_info("temp %d", temp);

	if (prev_state != VL53L8_STATE_RANGING)
		vl53l8_ioctl_stop(p_module);

	vl53l8_k_log_debug("unLock");
	mutex_unlock(&p_module->mutex);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", MIN_TEMP_SPEC, MAX_TEMP_SPEC, temp);
}

#define MIN_SPEC 0
#define MAX_SPEC 100000

static ssize_t vl53l8_ambient_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t i = 0, j = 0, idx = 0;
	int min = 0, max = 0,  avg = 0;

	for (i = 0; i < p_module->print_counter; i++) {
		for (j = 0; j < p_module->print_counter; j++) {
			idx = i * p_module->print_counter + j;
			p_module->data[idx] = p_module->range_data.per_zone_results.amb_rate_kcps_per_spad[idx] >> 11;
		}
	}

	min = max = avg = p_module->data[0];
	for (i = 1; i < NUM_OF_ZONE; i++) {
		if (p_module->data[i] < min)
			min = p_module->data[i];
		if (max < p_module->data[i])
			max = p_module->data[i];
		avg += p_module->data[i];
	}
	avg = avg / p_module->number_of_zones;

	for (i = 0; i < 64; i += 8) {
		vl53l8_k_log_info("%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}

	i = j = 0;
	j += snprintf(buf + j, PAGE_SIZE - j, "Ambient,%d,%d,%d,%d,%d,",
		MIN_SPEC, MAX_SPEC, min, max, avg);
	for (; i < NUM_OF_ZONE; i++) {
		j += snprintf(buf + j, PAGE_SIZE - j, "%d", p_module->data[i]);
		if (i != p_module->number_of_zones - 1)
			j += snprintf(buf + j, PAGE_SIZE - j, ",");
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "\n");
	}

	return j;
}

#define MIN_MAX_RANGE_SPEC 500
#define MAX_MAX_RANGE_SPEC 10500

static ssize_t vl53l8_max_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t i = 0, j = 0, idx = 0;
	int32_t min = 0, max = 0, avg = 0;

	if (p_module->read_data_valid) {
		for (i = 0; i < p_module->print_counter; i++) {
			for (j = 0; j < p_module->print_counter; j++) {
				idx = i * p_module->print_counter + j;
				p_module->data[idx] = p_module->range_data.per_zone_results.amb_dmax_mm[idx];
			}
		}
	} else {
		for (i = 0 ; i < NUM_OF_ZONE; i++)
			p_module->data[i] = -1;
	}

	vl53l8_rotate_report_data_int32(p_module->data, p_module->fac_rotation_mode);

	min = max = avg = p_module->data[0];

	for (i = 1; i < NUM_OF_ZONE; i++) {
		if (p_module->data[i] < min)
			min = p_module->data[i];
		if (max < p_module->data[i])
			max = p_module->data[i];
		avg += p_module->data[i];
	}

	avg = avg / p_module->number_of_zones;

	for (i = 0; i < 64; i += 8) {
		vl53l8_k_log_info("%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}

	i = j = 0;
	j += snprintf(buf + j, PAGE_SIZE - j, "Maxrange,%d,%d,%d,%d,%d,",
		MIN_MAX_RANGE_SPEC, MAX_MAX_RANGE_SPEC, min, max, avg);

	for (; i < TEST_300_MM_MAX_ZONE; i++) {
		j += snprintf(buf + j, PAGE_SIZE - j, "%d", p_module->data[i]);
		if (i != TEST_300_MM_MAX_ZONE - 1)
			j += snprintf(buf + j, PAGE_SIZE - j, ",");
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "\n");
	}

	return j;
}

#define MIN_PEAK_SIGNAL_SPEC 0
#define MAX_PEAK_SIGNAL_SPEC 3000

static ssize_t vl53l8_peak_signal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t i = 0, j = 0, idx = 0;
	int32_t min = 0, max = 0, avg = 0;

	if (p_module->read_data_valid) {
		for (i = 0; i < p_module->print_counter; i++) {
			for (j = 0; j < p_module->print_counter; j++) {
				idx = (i * p_module->print_counter + j);
			p_module->data[idx] = p_module->range_data.per_tgt_results.peak_rate_kcps_per_spad[idx] >> 11;
			}
		}
	} else {
		for (i = 0 ; i < NUM_OF_ZONE; i++)
			p_module->data[i] = -1;
	}

	min = max = avg = p_module->data[0];

	for (i = 1; i < NUM_OF_ZONE; i++) {
		if (p_module->data[i] < min)
			min = p_module->data[i];
		if (max < p_module->data[i])
			max = p_module->data[i];
		avg += p_module->data[i];
	}

	avg = avg / p_module->number_of_zones;

	for (i = 0; i < NUM_OF_ZONE; i += 8) {
		vl53l8_k_log_info("%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}

	vl53l8_rotate_report_data_int32(p_module->data, p_module->fac_rotation_mode);

	i = j = 0;
	j += snprintf(buf + j, PAGE_SIZE - j, "Peaksignal,%d,%d,%d,%d,%d,",
		MIN_PEAK_SIGNAL_SPEC, MAX_PEAK_SIGNAL_SPEC, min, max, avg);

	for (; i < TEST_300_MM_MAX_ZONE; i++) {
		j += snprintf(buf + j, PAGE_SIZE - j, "%d", p_module->data[i]);
		if (i != TEST_300_MM_MAX_ZONE - 1)
			j += snprintf(buf + j, PAGE_SIZE - j, ",");
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "\n");
	}

	return j;
}

static ssize_t vl53l8_target_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	uint32_t i = 0, j = 0;
	int min = 0, max = 0,  avg = 0;
	int idx;

	for (i = 0; i < p_module->print_counter; i++) {
		for (j = 0; j < p_module->print_counter; j++) {
			idx = (i * p_module->print_counter + j);
			p_module->data[idx] = (uint16_t)(p_module->range_data.d16_per_target_data.depth16[idx] & 0xE000U) >> 13;
		}
	}

	min = max = avg = p_module->data[0];
	for (i = 1; i < NUM_OF_ZONE; i++) {
		if (p_module->data[i] < min)
			min = p_module->data[i];
		if (max < p_module->data[i])
			max = p_module->data[i];
		avg += p_module->data[i];
	}


	vl53l8_rotate_report_data_int32(p_module->data, p_module->fac_rotation_mode);
	avg = avg / p_module->number_of_zones;

	for (i = 0; i < NUM_OF_ZONE; i += 8) {
		vl53l8_k_log_info("%d, %d, %d, %d, %d, %d, %d, %d\n",
		p_module->data[i], p_module->data[i+1], p_module->data[i+2],
		p_module->data[i+3], p_module->data[i+4], p_module->data[i+5],
		p_module->data[i+6], p_module->data[i+7]);
	}

	i = j = 0;
	j += snprintf(buf + j, PAGE_SIZE - j, "Targetstatus,%d,%d,%d,%d,%d,",
		MIN_SPEC, MAX_SPEC, min, max, avg);
	for (; i < NUM_OF_ZONE; i++) {
		j += snprintf(buf + j, PAGE_SIZE - j, "%d", p_module->data[i]);
		if (i != p_module->number_of_zones - 1)
			j += snprintf(buf + j, PAGE_SIZE - j, ",");
		else
			j += snprintf(buf + j, PAGE_SIZE - j, "\n");
	}

	return j;
}

static ssize_t vl53l8_asz_distance_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	u16 asz_distance[4], asz_status[4];
	u8 i = 0;

	for (; i < 4; i++) {
		asz_distance[i] =
			(uint16_t)(p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[64 + i] & 0x1FFFU);
		asz_status[i] =
			(uint16_t)(p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[64 + i] & 0xE000U) >> 13;
	}

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d, %d, %d, %d, %d\n",
		asz_distance[0], asz_status[0], asz_distance[1], asz_status[1],
		asz_distance[2], asz_status[2], asz_distance[3], asz_status[3]);
}

static ssize_t vl53l8_test_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d\n", p_module->test_mode, TEST_300_MM_MAX_ZONE);
}

static ssize_t vl53l8_test_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = kstrtou8(buf, 10, &val);
	if (ret) {
		vl53l8_k_log_error("Invalid\n");
		return count;
	}

	switch (val) {
	case 1:
		vl53l8_k_log_info("Set 500 mm\n");
		break;
	default:
		vl53l8_k_log_info("Set 300 mm\n");
	}
	p_module->test_mode = 0;

	return count;
}

static ssize_t vl53l8_uid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	vl53l8_k_log_info("%x,%x\n", p_module->m_info.module_id_hi, p_module->m_info.module_id_lo);
	return snprintf(buf, PAGE_SIZE, "%x%x\n", p_module->m_info.module_id_hi, p_module->m_info.module_id_lo);
}

static ssize_t vl53l8_cal_uid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	enum vl53l8_k_state_preset prev_state;

	if (p_module->read_p2p_cal_data == false) {
		prev_state = p_module->state_preset;

		mutex_lock(&p_module->mutex);
		if (p_module->state_preset <= VL53L8_STATE_LOW_POWER)
			vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_HP_IDLE);

		vl53l8_load_calibration(p_module);

		if (prev_state <= VL53L8_STATE_LOW_POWER)
			vl53l8_ioctl_set_power_mode(p_module, NULL, VL53L5_POWER_STATE_ULP_IDLE);
		mutex_unlock(&p_module->mutex);
	}

	if (p_module->read_p2p_cal_data == true)
		vl53l8_k_log_info("%x,%x\n",
				p_module->calibration.info.module_id_hi,
				p_module->calibration.info.module_id_lo);

	return snprintf(buf, PAGE_SIZE, "%x%x\n", p_module->calibration.info.module_id_hi, p_module->calibration.info.module_id_lo);
}

static ssize_t vl53l8_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "continuos\n");
}

static ssize_t vl53l8_frame_rate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "15\n");
}


static ssize_t vl53l8_zone_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "8,8\n");
}

static ssize_t vl53l8_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", p_module->last_driver_error);
}

#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
void sort_error_code(int *top5_error_code, u8 *top5_error_cnt)
{
	int i;
	int temp;

	for (i = 4; i > 0; i--) {
		if (top5_error_cnt[i] > top5_error_cnt[i-1]) {
			temp = top5_error_cnt[i];
			top5_error_cnt[i] = top5_error_cnt[i-1];
			top5_error_cnt[i-1] = temp;

			temp = top5_error_code[i];
			top5_error_code[i] = top5_error_code[i-1];
			top5_error_code[i-1] = temp;
		} else {
			break;
		}
	}
}

static ssize_t vl53l8_error_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);

	int i = 0;
	int j = 0;
	int top5_error_code[5] = {0};
	u8 top5_error_cnt[5] = {0};

	vl53l8_last_error_counter(p_module, p_module->last_driver_error);
	vl53l8_check_ldo_onoff(p_module);

	for (; i < MAX_TABLE; i++) {
		if (p_module->errdata[i].last_error_cnt == 0)
			continue;
		if (top5_error_cnt[4] < p_module->errdata[i].last_error_cnt) {
			top5_error_cnt[4] = p_module->errdata[i].last_error_cnt;
			top5_error_code[4] = p_module->errdata[i].last_error_code;
			sort_error_code(top5_error_code, top5_error_cnt);
		}
	}
	j += snprintf(buf + j, PAGE_SIZE, "\"RS_UID\":\"%x%x\"", p_module->m_info.module_id_hi, p_module->m_info.module_id_lo);
	for (i = 0; i < 5; i++) {
		j += snprintf(buf + j, PAGE_SIZE - j, ",");
		j += snprintf(buf + j, PAGE_SIZE - j, "\"errno%d\":\"%d\",\"cnt%d\":\"%d\"", i, top5_error_code[i], i, top5_error_cnt[i]);
	}
	j += snprintf(buf + j, PAGE_SIZE - j, "\n");

	memset(&p_module->errdata, 0, sizeof(p_module->errdata));

	return j;
}
#endif
static ssize_t vl53l8_interrupt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vl53l8_k_module_t *p_module = dev_get_drvdata(dev);
	int count = 0;

	while ((p_module->range.count == 0) && (count < MAX_READ_COUNT)) {
		usleep_range(10000, 10100);
		++count;
	}

	if (p_module->range.count > 0)
		return snprintf(buf, PAGE_SIZE, "1,%lu\n", p_module->range.count);
	else
		return snprintf(buf, PAGE_SIZE, "0,%lu\n", p_module->range.count);
}
#endif


static DEVICE_ATTR(name, 0440, vl53l8_name_show, NULL);
static DEVICE_ATTR(vendor, 0440, vl53l8_vendor_show, NULL);
static DEVICE_ATTR(fw_version, 0440, vl53l8_firmware_version_show, NULL);
static DEVICE_ATTR(enable, 0660, vl53l8_enable_show, vl53l8_enable_store);
static DEVICE_ATTR(asz_test, 0440, vl53l8_asz_distance_show, NULL);
static DEVICE_ATTR(test_mode, 0660, vl53l8_test_mode_show, vl53l8_test_mode_store);
static DEVICE_ATTR(temp, 0440, vl53l8_temp_show, NULL);
static DEVICE_ATTR(test01, 0440, vl53l8_distance_show, NULL);
static DEVICE_ATTR(test02, 0440, vl53l8_peak_signal_show, NULL);
static DEVICE_ATTR(test03, 0440, vl53l8_max_range_show, NULL);
static DEVICE_ATTR(ambient, 0440, vl53l8_ambient_show, NULL);
static DEVICE_ATTR(target_status, 0440, vl53l8_target_status_show, NULL);
static DEVICE_ATTR(uid, 0440, vl53l8_uid_show, NULL);
static DEVICE_ATTR(cal_uid, 0440, vl53l8_cal_uid_show, NULL);
static DEVICE_ATTR(mode, 0440, vl53l8_mode_show, NULL);
static DEVICE_ATTR(zone, 0440, vl53l8_zone_show, NULL);
static DEVICE_ATTR(frame_rate, 0440, vl53l8_frame_rate_show, NULL);
static DEVICE_ATTR(cal01, 0440, vl53l8_cal_offset_show, NULL);
static DEVICE_ATTR(cal02, 0440, vl53l8_cal_xtalk_show, NULL);
static DEVICE_ATTR(status, 0440, vl53l8_status_show, NULL);
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
static DEVICE_ATTR(error, 0440, vl53l8_error_show, NULL);
#endif
static DEVICE_ATTR(calibration, 0660, vl53l8_calibration_show, vl53l8_calibration_store);
static DEVICE_ATTR(open_calibration, 0440, vl53l8_open_calibration_show, NULL);
static DEVICE_ATTR(asz, 0660, vl53l8_asz_tuning_show, vl53l8_asz_tuning_store);
static DEVICE_ATTR(ranging_rate, 0660, vl53l8_ranging_rate_show, vl53l8_ranging_rate_store);
static DEVICE_ATTR(integration, 0660, vl53l8_integration_show, vl53l8_integration_store);
static DEVICE_ATTR(spi_clock_speed_hz, 0220, NULL, vl53l8_spi_clock_speed_hz);
static DEVICE_ATTR(interrupt, 0440, vl53l8_interrupt_show, NULL);


static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_fw_version,
	&dev_attr_enable,
	&dev_attr_asz_test,
	&dev_attr_temp,
	&dev_attr_test01,
	&dev_attr_test02,
	&dev_attr_test03,
	&dev_attr_ambient,
	&dev_attr_target_status,
	&dev_attr_cal_uid,
	&dev_attr_uid,
	&dev_attr_mode,
	&dev_attr_zone,
	&dev_attr_frame_rate,
	&dev_attr_cal01,
	&dev_attr_cal02,
	&dev_attr_calibration,
	&dev_attr_open_calibration,
	&dev_attr_status,
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	&dev_attr_error,
#endif
	&dev_attr_test_mode,
	&dev_attr_ranging_rate,
	&dev_attr_asz,
	&dev_attr_integration,
	&dev_attr_spi_clock_speed_hz,
	&dev_attr_interrupt,
	NULL,
};
//for sec dump  -----
int vl53l5_dump_data_notify(struct notifier_block *nb,
	unsigned long val, void *v)
{
	struct vl53l8_k_module_t *p_module = container_of(nb, struct vl53l8_k_module_t, dump_nb);

	if ((val == 1) && (p_module != NULL)) {
		vl53l8_k_log_info("probe status %d", p_module->stdev.status_probe);
		vl53l8_k_log_info("cal status %d", !(p_module->stdev.status_cal)?1:p_module->stdev.status_cal);
		vl53l8_k_log_info("cal load %d", p_module->load_calibration);
		vl53l8_k_log_info("cal p2p %d", p_module->read_p2p_cal_data);
		vl53l8_k_log_info("last_driver_err %d", p_module->last_driver_error);
		vl53l8_k_log_info("device id 0x%02X", p_module->stdev.host_dev.device_id);
		vl53l8_k_log_info("revision id 0x%02X", p_module->stdev.host_dev.revision_id);
		vl53l8_k_log_info("uid %x,%x\n", p_module->m_info.module_id_hi, p_module->m_info.module_id_lo);
		vl53l8_k_log_info("state %d force count %u", p_module->state_preset, p_module->force_suspend_count);
	}
	return 0;
}
//----- for sec dump

#endif
#endif

void vl53l8_k_prelaod(struct vl53l8_k_module_t *p_module)
{
	if (p_module != NULL) {
		int ret;

		if (p_module->input_dev == NULL) {
			p_module->input_dev = input_allocate_device();

			if (p_module->input_dev != NULL) {
				p_module->input_dev->name = MODULE_NAME;

				input_set_capability(p_module->input_dev, EV_REL, REL_MISC);
				input_set_drvdata(p_module->input_dev, p_module);

				ret = input_register_device(p_module->input_dev);
				vl53l8_k_log_info("input_register_device err %d\n", ret);
			} else {
				vl53l8_k_log_error("input alloc fail");
			}
		} else {
			vl53l8_k_log_info("already done input");
		}

		if (!p_module->sensors_register_done) {
			ret = sensors_register(&p_module->factory_device,
				p_module, sensor_attrs, MODULE_NAME);
			if (ret)
				vl53l8_k_log_error("could not register sensor-%d\n", ret);
			else {
				vl53l8_k_log_info("create success");
				p_module->sensors_register_done = true;
			}
		} else {
			vl53l8_k_log_info("already done register");
		}
	}
}

#ifdef CONFIG_SEPARATE_IO_CORE_POWER
static void power_delayed_work_func(struct work_struct *work)
{
	static int retry;
	int status = STATUS_OK;
	struct delayed_work *delayed_work = to_delayed_work(work);
	struct vl53l8_k_module_t *p_module = container_of(delayed_work,
						  struct vl53l8_k_module_t, power_delayed_work);

	status = vl53l8_regulator_init_state(p_module);
	if (status < 0 && retry++ < 10) {
		vl53l8_k_log_error("regulator not ready : %d retry : %d\n", status, retry);
		schedule_delayed_work(&p_module->power_delayed_work, msecs_to_jiffies(10000));
		return;
	}

	vl53l8_power_onoff(p_module, true);
}
#endif

int vl53l8_k_spi_probe(struct spi_device *spi)
{
	int status = 0;
	struct vl53l8_k_module_t *p_module = NULL;
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	unsigned char device_id = 0;
	unsigned char revision_id = 0;
	unsigned char page = 0;
#endif
	u8 ldo_num = 0;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Allocate module data");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	p_module = kzalloc(sizeof(struct vl53l8_k_module_t), GFP_KERNEL);
	if (!p_module) {
		vl53l8_k_log_error("Failed.");
		status = -ENOMEM;
		goto done;
	}
#else // SS_SUPPORT_SEC_CODE
	if (global_p_module == NULL)
		p_module = kzalloc(sizeof(struct vl53l8_k_module_t), GFP_KERNEL);
	else
		p_module = global_p_module;
#endif // SS_SUPPORT_SEC_CODE

	if (!p_module) {
		vl53l8_k_log_error("alloc fail");
		status = -ENOMEM;
		goto done;
	}

	vl53l8_k_log_debug("Assign data to spi handle");
	p_module->spi_info.device = spi;

	vl53l8_k_log_debug("Set client data");
	spi_set_drvdata(spi, p_module);

	vl53l8_k_log_debug("Init kref");
	kref_init(&p_module->spi_info.ref);

	if (spi->dev.of_node) {
		status = vl53l8_parse_dt(&spi->dev, p_module);
		if (status) {
			vl53l8_k_log_error("%s - Failed to parse DT", __func__);
			goto done_cleanup;
		}
	}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	status = vl53l5_platform_init(&p_module->stdev);
	if (status != VL53L5_ERROR_NONE)
		goto done;

	status = vl53l5_write_multi(&p_module->stdev, 0x7FFF, &page, 1);
	if (status < VL53L5_ERROR_NONE)
		goto done;

	status = vl53l5_read_multi(&p_module->stdev, 0x00, &device_id, 1);
	if (status < VL53L5_ERROR_NONE)
		goto done_change_page;

	status = vl53l5_read_multi(&p_module->stdev, 0x01, &revision_id, 1);
	if (status < VL53L5_ERROR_NONE)
		goto done_change_page;

	vl53l8_k_log_info("device_id (0x%02X), revision_id (0x%02X)",
			  device_id, revision_id);

	if (device_id != 0xF0 || revision_id != 0x0c)
		vl53l8_k_log_error("Unsupported device type");
#endif

	vl53l8_k_log_debug("Set driver");
	status = vl53l8_k_setup(p_module);
	if (status != 0)
		goto done_freemem;


#ifdef SS_SUPPORT_SEC_CODE
	for (ldo_num = 0; ldo_num < VDD_MAX_CNT; ldo_num++)
		p_module->ldo_prev_state[ldo_num] = 0xFF;
	p_module->number_of_zones = NUM_OF_ZONE;
	p_module->print_counter = 8;
	vl53l8_k_log_info("print_counter = %d",
			  p_module->print_counter);

	vl53l8_k_prelaod(p_module);
#if IS_ENABLED(CONFIG_SENSORS_VL53L8_QCOM) || IS_ENABLED(CONFIG_SENSORS_VL53L8_SLSI)
	//for sec dump  -----
	p_module->dump_nb.notifier_call = vl53l5_dump_data_notify;
	p_module->dump_nb.priority = 1;

	{
		int ret;

		ret = sensordump_notifier_register(&p_module->dump_nb);
		vl53l8_k_log_info("notifier %d", ret);
	}
	//----- for sec dump
#endif
#endif
#ifdef CONFIG_SEPARATE_IO_CORE_POWER
#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	INIT_WORK(&p_module->resume_work, resume_work_func);
#endif
	INIT_DELAYED_WORK(&p_module->power_delayed_work, power_delayed_work_func);
	schedule_delayed_work(&p_module->power_delayed_work, msecs_to_jiffies(5000));
#endif

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
done_change_page:
page = 2;
	if (status != VL53L5_ERROR_NONE)
		(void)vl53l5_write_multi(&p_module->stdev, 0x7FFF, &page, 1);
	else
		status = vl53l5_write_multi(&p_module->stdev, 0x7FFF, &page, 1);
#endif
done:
#ifdef SS_SUPPORT_SEC_CODE
	if (p_module != NULL)
		p_module->probe_done = true;
#endif // SS_SUPPORT_SEC_CODE
	LOG_FUNCTION_END(status);
	return status;

done_cleanup:
	vl53l8_k_cleanup(p_module);

done_freemem:
	kfree(p_module);

	LOG_FUNCTION_END(status);
	return status;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
void vl53l8_k_spi_remove(struct spi_device *device)
#else
int vl53l8_k_spi_remove(struct spi_device *device)
#endif
{
	int status = 0;
	struct vl53l8_k_module_t *p_module = spi_get_drvdata(device);

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Driver cleanup");
	vl53l8_k_cleanup(p_module);

	kref_put(&p_module->spi_info.ref, memory_release);

	LOG_FUNCTION_END(status);
	status = vl53l8_k_convert_error_to_linux_error(status);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	return;
#else
	return status;
#endif
}

#ifdef SS_SUPPORT_SEC_CODE
void vl53l8_k_spi_shutdown(struct spi_device *device)
{
	struct vl53l8_k_module_t *p_module = spi_get_drvdata(device);

	vl53l8_k_log_info("state %d err %d", p_module->state_preset, p_module->last_driver_error);
#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_RESUME_WORK
	cancel_work_sync(&p_module->resume_work);
#endif
	p_module->last_driver_error = VL53L8_SHUTDOWN;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
	if (p_module->state_preset == VL53L8_STATE_RANGING)
		vl53l8_ioctl_stop(p_module);

#ifdef CONFIG_SENSORS_VL53L8_SUPPORT_KERNEL_INTERFACE
	global_p_module = NULL;
#endif

	vl53l8_power_onoff(p_module, false);
}
#endif

static int setup_miscdev(struct vl53l8_k_module_t *p_module)
{
	int status = 0;

	vl53l8_k_log_debug("Setting up misc dev");
	p_module->miscdev.minor = MISC_DYNAMIC_MINOR;
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (p_module->id == 0)
		strcpy(p_module->name, VL53L8_K_DRIVER_NAME);
	else
		sprintf(p_module->name, "%s_%d", VL53L8_K_DRIVER_NAME,
			p_module->id);
#else // SS_SUPPORT_SEC_CODE
		strcpy(p_module->name, VL53L8_K_DRIVER_NAME);
#endif // SS_SUPPORT_SEC_CODE

	p_module->miscdev.name = p_module->name;
	p_module->miscdev.fops = &ranging_fops;
	vl53l8_k_log_debug("Misc device reg:%s", p_module->miscdev.name);
	vl53l8_k_log_debug("Reg misc device");
	status = misc_register(&p_module->miscdev);
	if (status != 0) {
		vl53l8_k_log_error("Failed to register misc dev: %d", status);
		goto done;
	}

#ifdef SS_SUPPORT_SEC_CODE
#ifdef CONFIG_SENSORS_VL53L5_SUPPORT_KERNEL_INTERFACE
	if (global_p_module == NULL)
		global_p_module = p_module;
#endif
	p_module->last_driver_error = VL53L8_DELAYED_LOAD_FIRMWARE;
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
#endif
done:
	return status;
}

static void cleanup_miscdev(struct vl53l8_k_module_t *p_module)
{
	if (!IS_ERR(p_module->miscdev.this_device) &&
			p_module->miscdev.this_device != NULL) {
		misc_deregister(&p_module->miscdev);
	}
}

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
static void deallocate_dev_id(int id)
{
	mutex_lock(&dev_table_mutex);

	module_table[id] = NULL;

	mutex_unlock(&dev_table_mutex);
}

static void allocate_dev_id(struct vl53l8_k_module_t *p_module)
{
	int i = 0;

	mutex_lock(&dev_table_mutex);

	while ((i < VL53L8_CFG_MAX_DEV) && (module_table[i]))
		i++;

	i = i < VL53L8_CFG_MAX_DEV ? i : -1;

	p_module->id = i;

	if (i != -1) {
		vl53l8_k_log_debug("Obtained device id %d", p_module->id);
		module_table[p_module->id] = p_module;
	}

	mutex_unlock(&dev_table_mutex);
}
#endif // SS_SUPPORT_SEC_CODE

static int vl53l8_k_ioctl_handler(struct vl53l8_k_module_t *p_module,
				  unsigned int cmd, unsigned long arg,
				  void __user *p)
{
	int status = 0;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	if (!p_module) {
		status = -EINVAL;
		goto exit;
	}
	vl53l8_k_log_debug("cmd%d", cmd);
#else // SS_SUPPORT_SEC_CODE
	int value = 0;

	if (p_module == NULL) {
		if ((cmd == VL53L8_IOCTL_START)
			|| (cmd == VL53L8_IOCTL_STOP))
			vl53l8_k_log_error("probe fail");

		status = -EINVAL;
		goto exit;
	} else if ((p_module->last_driver_error == VL53L8_PROBE_FAILED)
			|| (p_module->last_driver_error == VL53L8_DELAYED_LOAD_FIRMWARE)
			|| (p_module->last_driver_error == VL53L8_SHUTDOWN)
			|| (p_module->suspend_state == true)) {
		if ((cmd == VL53L8_IOCTL_START)
			|| (cmd == VL53L8_IOCTL_STOP))
			vl53l8_k_log_error("err %d", p_module->last_driver_error);
		status = p_module->last_driver_error;
		if (p_module->suspend_state == true)
			vl53l8_k_log_error("cmd %d is called in suspend", cmd);
		goto exit;
	}
#endif // SS_SUPPORT_SEC_CODE

	switch (cmd) {
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	case VL53L8_IOCTL_INIT:
		vl53l8_k_log_debug("VL53L8_IOCTL_INIT");
		status = vl53l8_ioctl_init(p_module);
		break;
	case VL53L8_IOCTL_TERM:
		vl53l8_k_log_debug("VL53L8_IOCTL_TERM");
		status = vl53l8_ioctl_term(p_module);
		break;
	case VL53L8_IOCTL_GET_VERSION:
		vl53l8_k_log_debug("VL53L8_IOCTL_GET_VERSION");
		status = vl53l8_ioctl_get_version(p_module, p);
		break;
#endif

	case VL53L8_IOCTL_START:
		vl53l8_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		vl53l8_k_log_info("VL53L8_IOCTL_START");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
		status = vl53l8_ioctl_start(p_module, p);
#else // SS_SUPPORT_SEC_CODE
		status = vl53l8_ioctl_start(p_module);
		if ((status != STATUS_OK) || (p_module->last_driver_error != STATUS_OK)) {
			status = vl53l8_ioctl_init(p_module);
			vl53l8_k_log_error("re init %d", status);
			status = vl53l8_ioctl_start(p_module);
		}
#endif
		if (status == STATUS_OK) {
			p_module->enabled = 1;
#ifdef SS_SUPPORT_SEC_CODE
			p_module->ioctl_enable_status = 1;
#endif // SS_SUPPORT_SEC_CODE
		}
		vl53l8_k_log_debug("unLock");
		mutex_unlock(&p_module->mutex);
		break;

	case VL53L8_IOCTL_STOP:
		if (p_module->state_preset < VL53L8_STATE_RANGING) {
			vl53l8_k_log_error("Can't stop. Not in ranging state: %d\n", p_module->state_preset);
			break;
		}
		vl53l8_k_log_debug("Lock");
		mutex_lock(&p_module->mutex);
		vl53l8_k_log_debug("VL53L8_IOCTL_STOP");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
		status = vl53l8_ioctl_stop(p_module, p);
#else // SS_SUPPORT_SEC_CODE
		p_module->enabled = 0;
		p_module->ioctl_enable_status = 0;
		status = vl53l8_ioctl_stop(p_module);
		vl53l8_k_log_info("STOP");
		if ((status != STATUS_OK) || (p_module->last_driver_error != STATUS_OK)) {
			status = vl53l8_ioctl_init(p_module);
			vl53l8_k_log_error("re init %d", status);
			status = vl53l8_ioctl_stop(p_module);
		}
#endif // SS_SUPPORT_SEC_CODE
		vl53l8_k_log_debug("unLock");
		mutex_unlock(&p_module->mutex);
		break;
	case VL53L8_IOCTL_GET_RANGE:
		vl53l8_k_log_debug("VL53L8_IOCTL_GET_RANGE");
		status = vl53l8_ioctl_get_range(p_module, p);
		break;

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	case VL53L8_IOCTL_SET_DEVICE_PARAMETERS:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_DEVICE_PARAMETERS");
		status = vl53l8_ioctl_set_device_parameters(p_module, p);
		break;
	case VL53L8_IOCTL_GET_DEVICE_PARAMETERS:
		vl53l8_k_log_debug("VL53L8_IOCTL_GET_DEVICE_PARAMETERS");
		status = vl53l8_ioctl_get_device_parameters(p_module, p);
		break;
	case VL53L8_IOCTL_GET_MODULE_INFO:
		vl53l8_k_log_debug("VL53L8_IOCTL_GET_MODULE_INFO");
		status = vl53l8_ioctl_get_module_info(p_module, p);
		break;
	case VL53L8_IOCTL_GET_ERROR_INFO:
		vl53l8_k_log_debug("VL53L8_IOCTL_GET_ERROR_INFO");
		status = vl53l8_ioctl_get_error_info(p_module, p);
		break;
	case VL53L8_IOCTL_SET_POWER_MODE:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_POWER_MODE");
		status = vl53l8_ioctl_set_power_mode(p_module, p);
		break;
	case VL53L8_IOCTL_POLL_DATA_READY:
		vl53l8_k_log_debug("VL53L8_IOCTL_POLL_DATA_READY");
		status = vl53l8_ioctl_poll_data_ready(p_module);
		break;
	case VL53L8_IOCTL_READ_P2P_FILE:
		vl53l8_k_log_debug("VL53L8_IOCTL_READ_P2P_FILE");
		status = vl53l8_ioctl_read_p2p_calibration(p_module);
		break;
	case VL53L8_IOCTL_PERFORM_CALIBRATION_300:
		vl53l8_k_log_debug("VL53L8_IOCTL_PERFORM_CALIBRATION_300");
		status = vl53l8_perform_calibration(p_module, 0);
		break;
	case VL53L8_IOCTL_READ_SHAPE_FILE:
		vl53l8_k_log_debug("VL53L8_IOCTL_READ_SHAPE_FILE");
		status = vl53l8_ioctl_read_shape_calibration(p_module);
		break;
	case VL53L8_IOCTL_READ_GENERIC_SHAPE:
		vl53l8_k_log_debug("VL53L8_IOCTL_READ_GENERIC_SHAPE");
		status = vl53l8_ioctl_read_generic_shape(p_module);
		break;
	case VL53L8_IOCTL_PERFORM_CHARACTERISATION_1000:
		vl53l8_k_log_debug(
			"VL53L8_IOCTL_PERFORM_CHARACTERISATION_1000");
		status = vl53l8_perform_calibration(p_module, 1);
		break;
#else //SS_SUPPORT_SEC_CODE
	case VL53L8_IOCTL_GET_STATUS:
		status = vl53l8_ioctl_get_status(p_module, p);
		break;
	case VL53L5_IOCTL_GET_CAL_DATA:
		vl53l8_k_log_info("GET_CAL_DATA");
		status = vl53l8_ioctl_get_cal_data(p_module, p);
		break;
	case VL53L5_IOCTL_SET_CAL_DATA:
		vl53l8_k_log_info("SET_CAL_DATA");
		status = vl53l8_ioctl_set_cal_data(p_module, p);
		break;
	case VL53L5_IOCTL_SET_PASS_FAIL:
		vl53l8_k_log_info("SET_PASS_FAIL");
		status = vl53l8_ioctl_set_pass_fail(p_module, p);
		break;
	case VL53L5_IOCTL_SET_FILE_LIST:
		vl53l8_k_log_info("SET_FILE_LIST");
		status = vl53l8_ioctl_set_file_list(p_module, p);
		break;
#endif
	case VL53L8_IOCTL_SET_RANGING_RATE:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_RANGING_RATE");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
		status = vl53l8_ioctl_set_ranging_rate(p_module, p);
#else // SS_SUPPORT_SEC_CODE
		status = copy_from_user(&value, p, sizeof(int));
		if (status != VL53L5_ERROR_NONE) {
			status = VL53L8_K_ERROR_FAILED_TO_COPY_RANGING_RATE;
			return status;
		}
		status = set_sampling_rate(p_module, (uint32_t)value);
		if (status != STATUS_OK) {
			vl53l8_k_log_error("RR Fail %d", status);
			return status;
		} else
			vl53l8_k_log_info("RR %d", value);
#endif // SS_SUPPORT_SEC_CODE
		break;
	case VL53L8_IOCTL_SET_INTEGRATION_TIME_US:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_INTEGRATION_TIME_US");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
		status = vl53l8_ioctl_set_integration_time_us(p_module, p);
#else // SS_SUPPORT_SEC_CODE
		status = copy_from_user(&value, p, sizeof(int));
		if (status != VL53L5_ERROR_NONE) {
			status = VL53L8_K_ERROR_FAILED_TO_COPY_MAX_INTEGRATION;
			return status;
		}
		status = set_integration_time(p_module, (uint32_t)value);
		if (status != STATUS_OK)
			return status;
		mutex_lock(&p_module->mutex);
		status = vl53l8_ioctl_set_integration_time_us(p_module, p_module->integration);
		mutex_unlock(&p_module->mutex);
		if (status != VL53L5_ERROR_NONE) {
			status = vl53l5_read_device_error(&p_module->stdev, status);
			vl53l8_k_log_error("IT Fail %d", status);
		} else
			vl53l8_k_log_info("IT %d", p_module->integration);
#endif // SS_SUPPORT_SEC_CODE
		break;
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	case VL53L8_IOCTL_SET_ASZ_TUNING:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_ASZ_TUNING");
		status = vl53l8_ioctl_set_asz_tuning(p_module, p);
		break;
	case VL53L8_IOCTL_SET_GLARE_FILTER_TUNING:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_GLARE_FILTER_TUNING");
		status = vl53l8_ioctl_set_glare_filter_tuning(p_module, p);
		break;
	case VL53L8_IOCTL_SET_TRANSFER_SPEED_HZ:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_TRANSFER_SPEED_HZ");
		status = vl53l8_ioctl_set_transfer_speed_hz(p_module, p);
		break;
#else // SS_SUPPORT_SEC_CODE
	case VL53L8_IOCTL_SET_ASZ_TUNING:
		vl53l8_k_log_debug("VL53L8_IOCTL_SET_ASZ_TUNING");
		status = vl53l8_ioctl_set_asz_tuning(p_module, p, NULL);
		break;
#endif // SS_SUPPORT_SEC_CODE

	default:
		status = -EINVAL;
	}

exit:
	return status;
}

int vl53l8_k_open(struct inode *inode, struct file *file)
{
	int status = VL53L5_ERROR_NONE;
	struct vl53l8_k_module_t *p_module = NULL;

	LOG_FUNCTION_START("");

	p_module = container_of(file->private_data, struct vl53l8_k_module_t,
				miscdev);

	vl53l8_k_log_debug("Get SPI bus");

	kref_get(&p_module->spi_info.ref);

	LOG_FUNCTION_END(status);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_store_error(p_module, status);
#endif
	status = vl53l8_k_convert_error_to_linux_error(status);
	return status;
}

int vl53l8_k_release(struct inode *inode, struct file *file)
{
	int status = VL53L5_ERROR_NONE;
	struct vl53l8_k_module_t *p_module = NULL;

	LOG_FUNCTION_START("");

	p_module = container_of(file->private_data, struct vl53l8_k_module_t,
			    miscdev);

	kref_put(&p_module->spi_info.ref, memory_release);

	LOG_FUNCTION_END(status);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_store_error(p_module, status);
#endif
	status = vl53l8_k_convert_error_to_linux_error(status);
	return status;
}

long vl53l8_k_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int status = VL53L5_ERROR_NONE;
	struct vl53l8_k_module_t *p_module = NULL;

	LOG_FUNCTION_START("");

	p_module = container_of(file->private_data, struct vl53l8_k_module_t,
			    miscdev);

	vl53l8_k_log_debug("Lock");

	status = vl53l8_k_ioctl_handler(p_module, cmd, arg, (void __user *)arg);

	vl53l8_k_log_debug("unLock");

	LOG_FUNCTION_END(status);
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_store_error(p_module, status);
#else // SS_SUPPORT_SEC_CODE
	vl53l8_k_store_error(p_module, status);
#ifdef CONFIG_SENSORS_LAF_FAILURE_DEBUG
	vl53l8_last_error_counter(p_module, p_module->last_driver_error);
#endif
#endif // SS_SUPPORT_SEC_CODE
	status = vl53l8_k_convert_error_to_linux_error(status);
	return status;
}

void vl53l8_k_cleanup(struct vl53l8_k_module_t *p_module)
{
	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("Unregistering misc device");
	cleanup_miscdev(p_module);

	if (p_module->state_preset == VL53L8_STATE_RANGING) {
		vl53l8_k_log_debug("Stop ranging");
		(void)vl53l5_stop(&p_module->stdev, &p_module->ranging_flags);
#ifdef VL53L8_INTERRUPT
			vl53l8_k_log_debug("Stop interrupt");
			vl53l8_k_stop_interrupt(p_module);
#endif

	}

	(void)vl53l8_ioctl_term(p_module);

	vl53l8_k_log_debug("Acquiring lock");
	mutex_lock(&p_module->mutex);
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Deallocating dev id");
	deallocate_dev_id(p_module->id);

	vl53l8_k_log_debug("Setting state flags");
	p_module->state_preset = VL53L8_STATE_NOT_PRESENT;

	vl53l8_k_log_debug("Platform terminate");
	vl53l5_platform_terminate(&p_module->stdev);
#endif

	vl53l8_k_log_debug("Releasing all gpios");
	vl53l8_k_release_gpios(p_module);

	vl53l8_k_log_debug("Releasing mutex");
	mutex_unlock(&p_module->mutex);

	LOG_FUNCTION_END(0);
}

int vl53l8_k_setup(struct vl53l8_k_module_t *p_module)
{
	int status = 0;

	LOG_FUNCTION_START("");
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Acquire device id");
	allocate_dev_id(p_module);
	if (p_module->id < 0) {
		vl53l8_k_log_error("Failed");
		status = -EPERM;
		goto done;
	}
#endif

	vl53l8_k_log_debug("Init device mutex");
	mutex_init(&p_module->mutex);
	mutex_init(&p_module->cal_mutex);

	vl53l8_k_log_debug("Acquired lock");
	mutex_lock(&p_module->mutex);

	vl53l8_k_log_debug("Set default sleep time");
	p_module->polling_sleep_time_ms = VL53L8_K_SLEEP_TIME_MS;
	p_module->range_mode = VL53L8_RANGE_SERVICE_DEFAULT;
	p_module->transfer_speed_hz = STM_VL53L8_SPI_DEFAULT_TRANSFER_SPEED_HZ;

#ifdef VL53L8_INTERRUPT
	p_module->range_mode = VL53L8_RANGE_SERVICE_INTERRUPT;
#endif
	vl53l8_k_log_debug("Range mode %d", p_module->range_mode);

	vl53l8_k_log_debug("INIT waitqueue");
	INIT_LIST_HEAD(&p_module->reader_list);
	init_waitqueue_head(&p_module->wait_queue);

	vl53l8_k_log_debug("Set state flags");
	p_module->state_preset = VL53L8_STATE_PRESENT;

	vl53l8_k_log_debug("Releasing mutex");
	mutex_unlock(&p_module->mutex);

	status = setup_miscdev(p_module);
	if (status != 0) {
		vl53l8_k_log_error("Misc dev setup failed: %d", status);
		goto done;
	}

done:
	LOG_FUNCTION_END(status);
	return status;
}

static int __init vl53l8_k_init(void)
{
	int status = 0;

	LOG_FUNCTION_START("");

	vl53l8_k_log_info("Init %s driver", VL53L8_K_DRIVER_NAME);

#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
	vl53l8_k_log_debug("Init spi bus");
	status = spi_register_driver(&vl53l8_k_spi_driver);
	if (status != 0) {
		vl53l8_k_log_error("Failed init bus: %d", status);
		goto done;
	}
#else // SS_SUPPORT_SEC_CODE
	global_p_module = NULL;
	global_p_module = kzalloc(sizeof(struct vl53l8_k_module_t), GFP_KERNEL);
	if (!global_p_module)
		vl53l8_k_log_error("Failed.");

	vl53l8_k_prelaod(global_p_module);
	vl53l8_k_log_debug("Init spi bus");
	status = spi_register_driver(&vl53l8_k_spi_driver);
	if (status != 0) {
		vl53l8_k_log_error("Failed init bus: %d", status);
		if (global_p_module != NULL)
			kfree(global_p_module);
		goto done;
	}
#endif // SS_SUPPORT_SEC_CODE

	vl53l8_k_log_info("Kernel driver version: %d.%d.%d.%d",
			  VL53L8_K_VER_MAJOR,
			  VL53L8_K_VER_MINOR,
			  VL53L8_K_VER_BUILD,
			  VL53L8_K_VER_REVISION);

done:
	LOG_FUNCTION_END(status);
	status = vl53l8_k_convert_error_to_linux_error(status);
	return status;
}

static void __exit vl53l8_k_exit(void)
{
	LOG_FUNCTION_START("");

	vl53l8_k_log_info("Exiting %s driver", VL53L8_K_DRIVER_NAME);

	vl53l8_k_log_debug("Exiting spi bus");
	spi_unregister_driver(&vl53l8_k_spi_driver);
	vl53l8_k_log_debug("Cleaning up");
	vl53l8_k_clean_up_spi();

	LOG_FUNCTION_END(0);
}

module_init(vl53l8_k_init);
module_exit(vl53l8_k_exit);

MODULE_DESCRIPTION("Sample VL53L8 ToF client driver");
MODULE_LICENSE("GPL v2");
