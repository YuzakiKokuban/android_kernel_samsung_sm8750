/*******************************************************************************
* Copyright (c) 2022, STMicroelectronics - All Rights Reserved
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

#include <linux/irq.h>
#include <linux/gpio.h>
#include "vl53l8_k_interrupt.h"
#include "vl53l8_k_driver_config.h"
#include "vl53l8_k_logging.h"
#include "vl53l8_k_error_converter.h"
#include "vl53l5_api_ranging.h"
#include "vl53l5_error_codes.h"
#include "vl53l8_k_range_wait_handler.h"
#include "vl53l8_k_glare_filter.h"

#define ACCUMULATE_SINGLE_ZONE 64
#define CENTER_ZONE 27
#define DISTANCE_100MM 100
#define CONVERT_TO_DISTANCE 0x1FFFU
#define STUCK_CNT_THRESHOLD 100

bool is_close_for_stuck_check(u16 depth16)
{
	return ((depth16 & CONVERT_TO_DISTANCE) < DISTANCE_100MM);
}

void rs_stuck_check(struct vl53l8_k_module_t *p_module)
{
	static u16 prev_depth16[2] = {0, 0};
	u16 asz_depth16 = p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[ACCUMULATE_SINGLE_ZONE];
	u16 center_zone_depth16 = p_module->range.data.tcpm_0_patch.d16_per_target_data.depth16[CENTER_ZONE];

	if (is_close_for_stuck_check(asz_depth16) && is_close_for_stuck_check(center_zone_depth16))
		return;

	if(asz_depth16 == prev_depth16[0] && center_zone_depth16 == prev_depth16[1])
		p_module->stuck.stuck_cnt++;
	else
		p_module->stuck.stuck_cnt = 0;	

	prev_depth16[0] = asz_depth16;
	prev_depth16[1] = center_zone_depth16;

	if (p_module->stuck.stuck_cnt > STUCK_CNT_THRESHOLD) {
		vl53l8_last_error_counter(p_module, VL53L8_DATA_STUCK_ERROR);
		p_module->stuck.stuck_check = false;
	}
}

static int _interrupt_get_range_data(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	vl53l8_k_log_debug("disable irq");

	disable_irq(p_module->irq_val);
	p_module->range.is_valid = 0;

	status = vl53l5_get_range_data(&p_module->stdev);
	if (status == VL53L5_NO_NEW_RANGE_DATA_ERROR ||
			status == VL53L5_TOO_HIGH_AMBIENT_WARNING) {
		vl53l8_k_log_error("skip %d\n", status);
		status = VL53L5_ERROR_NONE;
		goto out;
	} else if (status != VL53L5_ERROR_NONE)
		goto out;

	status = vl53l5_decode_range_data(&p_module->stdev,
					  &p_module->range.data);
	if (status != VL53L5_ERROR_NONE)
		goto out;

	if (p_module->patch_ver.patch_version.ver_major < 6 &&
	    p_module->patch_ver.patch_version.ver_minor >= 0) {
		status = vl53l8_k_glare_filter(&p_module->gf_tuning,
						&p_module->range.data);
		if (status != VL53L5_ERROR_NONE)
			goto out;
	}

	p_module->range.count++;
	p_module->range.is_valid = 1;
	p_module->polling_count = 0;

	if (p_module->stuck.stuck_check)
		rs_stuck_check(p_module);

out:
	if (status != VL53L5_ERROR_NONE) {
		status = vl53l5_read_device_error(&p_module->stdev, status);
#ifdef STM_VL53L8_SUPPORT_LEGACY_CODE
		vl53l8_k_log_error("Failed: %d", status);
#else // SS_SUPPORT_SEC_CODE
		if (p_module->irq_is_active)
			vl53l8_k_log_error("Fail %d\n", status);
		else
			vl53l8_k_log_info("%d\n", status);
#endif
	}

	vl53l8_k_store_error(p_module, status);

	enable_irq(p_module->irq_val);
	vl53l8_k_log_debug("enable irq");

	LOG_FUNCTION_END(status);
	return status;
}

static irqreturn_t vl53l8_interrupt_handler(int irq, void *dev_id)
{

	struct vl53l8_k_module_t *p_module = (struct vl53l8_k_module_t *)dev_id;

	if (!p_module->irq_wq_is_running && p_module->irq_is_active)
		queue_work(p_module->irq_wq, &p_module->data_work);

	return IRQ_HANDLED;
}

static void vl53l8_irq_workqueue(struct work_struct *work)
{
	struct vl53l8_k_module_t *p_module =
		container_of(work, struct vl53l8_k_module_t, data_work);
	int status = VL53L5_ERROR_NONE;

	vl53l8_k_log_debug("Lock");
	mutex_lock(&p_module->mutex);
	p_module->irq_wq_is_running = true;

	if (p_module->state_preset == VL53L8_STATE_RANGING) {
		status = _interrupt_get_range_data(p_module);
		if (status == VL53L5_ERROR_NONE) {
			vl53l8_k_log_debug("Interrupt handled");
		} else {
#ifdef SS_SUPPORT_SEC_CODE
			if (p_module->irq_is_active)
				vl53l8_k_log_error("irq not handled %d\n", status);
#endif
			vl53l8_k_log_info("err getdata");
		}

	}
	p_module->irq_wq_is_running = false;
	vl53l8_k_log_debug("Unlock");
	mutex_unlock(&p_module->mutex);
}

int vl53l8_k_start_interrupt(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;
	const char *p_dev_name = p_module->name;

	LOG_FUNCTION_START("");

	if (p_module->gpio.interrupt < 0) {
		status = VL53L8_K_ERROR_DEVICE_INTERRUPT_NOT_OWNED;
		goto out;
	}

	p_module->irq_val = gpio_to_irq(p_module->gpio.interrupt);

	if (p_module->irq_wq == NULL) {
		p_module->irq_wq = alloc_workqueue("vl53l8_irq_workqueue", WQ_MEM_RECLAIM, 1);
		if (!p_module->irq_wq) {
			status = -ENOMEM;
			vl53l8_k_log_error("could not create irq work\n");
			goto out;
		} else {
			INIT_WORK(&p_module->data_work, vl53l8_irq_workqueue);
		}
	}

	status = request_threaded_irq(p_module->irq_val,
				      NULL,
				      vl53l8_interrupt_handler,
				      IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				      p_dev_name,
				      p_module);
	if (status) {
		vl53l8_k_log_error("Unable to assign IRQ: %d\n",
				   p_module->irq_val);
		goto out;
	} else {
		vl53l8_k_log_debug("IRQ %d now assigned\n",
				  p_module->irq_val);
		p_module->gpio.interrupt_owned = 1;
		p_module->irq_is_running = true;
	}

out:
	LOG_FUNCTION_END(status);
	return status;
}

int vl53l8_k_stop_interrupt(struct vl53l8_k_module_t *p_module)
{
	int status = VL53L5_ERROR_NONE;

	LOG_FUNCTION_START("");

	if (p_module->irq_is_active)
		disable_irq(p_module->irq_val);

	vl53l8_k_log_debug("disable irq");

	free_irq(p_module->irq_val, p_module);
	vl53l8_k_log_debug("IRQ %d now free", p_module->irq_val);
	p_module->irq_val = 0;
	p_module->irq_is_running = false;

	LOG_FUNCTION_END(status);
	return status;
}
