// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "cvp_comm_def.h"

#ifdef USE_PRESIL42
#include "cam_presil_hw_access.h"
#include "cvp_presil.h"
#include "msm_cvp_debug.h"
#include "cvp_hfi_io.h"
#include "cvp_core_hfi.h"

void presil42_write_register(struct iris_hfi_device *device, u32 reg, u32 value)
{
	int rc = 0;
	u8 *base_addr;
	bool enable_fatal;
	u32 hwiosymaddr = reg;

	if (!device) {
		dprintk(CVP_ERR, "Invalid params: %pK\n", device);
		return;
	}

	enable_fatal = !mutex_is_locked(&device->lock);
	enable_fatal &= device->res->debug_timeout;
	MSM_CVP_ERROR(enable_fatal);

	if (!device->power_enabled) {
		dprintk(CVP_WARN,
			"HFI Write register failed : Power is OFF\n");
		enable_fatal = true;
		enable_fatal &= device->res->debug_timeout;
		MSM_CVP_ERROR(enable_fatal);
		return;
	}

	base_addr = (void *)CVP_REG_BASE_ADDR;
	dprintk(CVP_REG, "Presil Base addr: %#x, written to: %#x, Value: %#x...\n",
		base_addr, hwiosymaddr, value);

	base_addr += hwiosymaddr;

	dprintk(CVP_REG, "Calling cam_presil_register_write");
	rc = cam_presil_register_write((void *)base_addr, value, 0);
	if (rc != CAM_PRESIL_SUCCESS) {
		dprintk(CVP_ERR,
			"%s failed with Base addr: %pK, written to: %#x, Value: %#x...\n",
			__func__, base_addr, hwiosymaddr, value);
	} else {
		dprintk(CVP_REG,
			"%s is successful with Base addr: %pK, written to: %#x, Value: %#x...\n",
			__func__, base_addr, hwiosymaddr, value);
	}

}

int presil42_read_register(u32 reg)
{
	u32 data;
	int rc = 0;
	u8 *base_addr;

	//override with presil base addr - defined between eva kernel &
	//cam presil API implementation
	base_addr = (void *)CVP_REG_BASE_ADDR;

	rc = cam_presil_register_read((void *)(base_addr + reg), &data);
	if (rc != CAM_PRESIL_SUCCESS) {
		dprintk(CVP_ERR,
			"%s failed with Base addr: %pK, read from %#x",
			__func__, base_addr, reg);
	} else {
		dprintk(CVP_REG,
			"%s is successful with Base addr: %pK, read from: %#x, value: %#x\n",
			__func__, base_addr, reg, data);
	}
	return data;

}

int presil42_iface_cmdq_write_relaxed(struct cvp_hal_cmd_pkt_hdr *cmd_packet,
		void *pkt)
{
	int result;

	dprintk(CVP_REG, "pkt %x cmd_packet->size %d\n", pkt, cmd_packet->size);

	result = cam_presil_hfi_write_cmd(pkt, cmd_packet->size, 0x2);
	if (result != CAM_PRESIL_SUCCESS) {
		dprintk(CVP_ERR,
			"Failed to execute %s on presil PCHOST ", __func__);
	} else {
		dprintk(CVP_REG,
			"Successfully execute %s on presil PCHOST ", __func__);
	}
	return result;

}

int presil42_iface_msgq_read(void *pkt)
{
	int presil_rc = CAM_PRESIL_BLOCKED;
	static u32 words_read;
	static u32 pkt_size;  /*bytes*/
	void *pkt_cur;
	int rc = 0;

	/*when more than one packets are received in one presil_hfi_read_message call*/
	if (words_read > 0) {
		rc = 0;
		dprintk(CVP_DBG,
			"MSG Q: more than one packets received in hfi_read_message call");
		pkt_cur = (u32 *)pkt + pkt_size;
		memmove(pkt, pkt_cur, words_read << 2);
		pkt_size = (*(u32 *)pkt) >> 2;
		dprintk(CVP_DBG, "current words_read = %u, pkt_size = %u", words_read,
			pkt_size);
		words_read = words_read - pkt_size;
		return rc;
	}

	presil_rc = cam_presil_hfi_read_message((u32 *)pkt, Q_MSG, &words_read, 0x2);
	dprintk(CVP_DBG,
		"MSG Q: cam_presil_hfi_read_message return presil_rc = %d words_read = %u",
		presil_rc, words_read);

	if (words_read > 0) {
		rc = 0;
		dprintk(CVP_DBG,
			"MSG Q: Successfully execute hfi_read_message directly on PCHOST ");
		pkt_size = (*(u32 *)pkt) >> 2;
		dprintk(CVP_DBG, "raw words_read = %u, pkt_size = %u", words_read, pkt_size);
		words_read = words_read - pkt_size;
	} else {
		rc = -1;
		dprintk(CVP_ERR,
			"MSG Q: Failed to execute hfi_read_message directly on PCHOST ");
	}
	return rc;
}

int presil42_iface_dbgq_read(void *pkt)
{
	int presil_rc = CAM_PRESIL_BLOCKED;
	u32 words_read = 0;
	int rc = 0;

	presil_rc = cam_presil_hfi_read_message((u32 *)pkt, Q_DBG, &words_read, 0x2);
	dprintk(CVP_DBG,
		"DBG Q: %s return presil_rc = %d words_read = %u",
		__func__, presil_rc, words_read);

	if (words_read > 0) {
		rc = 0;
		dprintk(CVP_DBG,
			"DBG Q: Successfully execute %s directly on Presil PCHOST ", __func__);
	} else {
		rc = -1;
		dprintk(CVP_ERR,
			"DBG Q: Failed to execute %s directly on Presil PCHOST ", __func__);
	}
	return rc;
}

void presil42_setup_ucregion_memory_map(struct iris_hfi_device *device)
{
	presil42_write_register(device, CVP_UC_REGION_ADDR,
		(u32)device->iface_q_table.align_device_addr -
		MAP_ADDR_OFFSET);
	presil42_write_register(device, CVP_UC_REGION_SIZE, SHARED_QSIZE);
	presil42_write_register(device, CVP_QTBL_ADDR,
		(u32)device->iface_q_table.align_device_addr -
		MAP_ADDR_OFFSET);
	presil42_write_register(device, CVP_QTBL_INFO, 0x01);

	dprintk(CVP_WARN,
		"driver: CVP_QTBL_INFO [0x%08x] = 0x%08x\n", (CVP_QTBL_INFO), 0x01);
	dprintk(CVP_WARN,
		"driver: UC_REGION_ADDR [0x%08x] = 0x%08x\n",
		(CVP_UC_REGION_ADDR), ((u32)device->iface_q_table.align_device_addr -
		MAP_ADDR_OFFSET));
	dprintk(CVP_WARN,
		"driver: CVP_UC_REGION_SIZE [0x%08x] = 0x%08x\n",
		(CVP_UC_REGION_SIZE), SHARED_QSIZE);
	dprintk(CVP_WARN, "driver: CVP_QTBL_ADDR [0x%08x] = 0x%08x\n",
		(CVP_QTBL_ADDR), (u32)device->iface_q_table.align_device_addr -
		MAP_ADDR_OFFSET);

	if (device->sfr.align_device_addr) {

		presil42_write_register(device, CVP_SFR_ADDR,
			(u32)device->sfr.align_device_addr -
			MAP_ADDR_OFFSET);

		dprintk(CVP_REG,
			"driver: CVP_SFR_ADDR [0x%08x] = 0x%08x\n",
			(CVP_SFR_ADDR), (u32)device->sfr.align_device_addr -
			MAP_ADDR_OFFSET);
	}
	if (device->qdss.align_device_addr)

		presil42_write_register(device, CVP_MMAP_ADDR,
		(u32)device->qdss.align_device_addr -
			MAP_ADDR_OFFSET);

		dprintk(CVP_REG,
			"driver: CVP_MMAP_ADDR qdss [0x%08x] = 0x%08x\n",
			(CVP_MMAP_ADDR), (u32)device->qdss.align_device_addr -
			MAP_ADDR_OFFSET);
}

void presil42_core_clear_interrupt(struct iris_hfi_device *device)
{
	u32 intr_status = 0;
	//to do: remove this temporary walkaround after fixing
	//PC_HOST IRQ handler called issue by Rumi42
	device->intr_status |= intr_status;
	device->reg_count++;
	dprintk(CVP_REG,
		"PreSil INTERRUPT for device: %pK: times: %d status: %d\n",
		device, device->reg_count, intr_status);
}

void presil42_hfi_core_work_handler(struct work_struct *work)
{
	struct msm_cvp_core *core;
	struct iris_hfi_device *device;
	int num_responses = 0, i = 0;
	u32 intr_status;
	static bool warning_on = true;

	dprintk(CVP_REG, "CVP_INFO presil_hfi_core_work_handler: E");
	core = cvp_driver->cvp_core;
	if (core)
		device = core->dev_ops->hfi_device_data;
	else
		return;

	mutex_lock(&device->lock);
	if (!(device->state != IRIS_STATE_DEINIT)) {
		if (warning_on) {
			dprintk(CVP_WARN, "%s Core not in init state\n",
				__func__);
			warning_on = false;
		}
		goto err_no_work;
	}

	warning_on = true;

	if (!device->callback) {
		dprintk(CVP_ERR, "No interrupt callback function: %pK\n",
				device);
		goto err_no_work;
	}

	if (__resume(device)) {
		dprintk(CVP_ERR, "%s: Power enable failed\n", __func__);
		goto err_no_work;
	}

	if (!device) {
		dprintk(CVP_ERR, "%s: NULL device\n", __func__);
		return;
	}
	presil42_core_clear_interrupt(device);

	num_responses = __response_handler(device);
	dprintk(CVP_HFI, "%s:: cvp_driver_debug num_responses = %d ",
		__func__, num_responses);

err_no_work:


	intr_status = device->intr_status;
	mutex_unlock(&device->lock);


	for (i = 0; !IS_ERR_OR_NULL(device->response_pkt) &&
		i < num_responses; ++i) {
		struct msm_cvp_cb_info *r = &device->response_pkt[i];
		void *rsp = (void *)&r->response;

		if (!(device->state != IRIS_STATE_DEINIT)) {
			dprintk(CVP_ERR, "Invalid state\n");
			break;
		}
		dprintk(CVP_HFI, "Processing response %d of %d, type %d\n",
			(i + 1), num_responses, r->response_type);

		device->callback(r->response_type, rsp);
	}


	if (!(intr_status & CVP_WRAPPER_INTR_STATUS_A2HWD_BMSK))
		enable_irq(device->cvp_hal_data->irq);
}

void presil42_setup_dsp_uc_memmap_vpu5(struct iris_hfi_device *device)
{
	presil42_write_register(device, HFI_DSP_QTBL_ADDR,
		(u32)device->dsp_iface_q_table.align_device_addr - MAP_ADDR_OFFSET);
	presil42_write_register(device, HFI_DSP_UC_REGION_ADDR,
		(u32)device->dsp_iface_q_table.align_device_addr - MAP_ADDR_OFFSET);
	presil42_write_register(device, HFI_DSP_UC_REGION_SIZE,
		device->dsp_iface_q_table.mem_data.size);
}

void presil42_cvp_iris_hfi_delete_device(struct iris_hfi_device *dev)
{
	bool bRetVal = false;

	// CVP Presil device unhook up
	bRetVal = cam_presil_unsubscribe_device_irq(dev->cvp_hal_data->irq);
	dprintk(CVP_INFO,
		"PRESIL_UNSUBS_IRQ: UnSubscribe for CVP IRQ: Ret=%d IRQ NUM=%dIRQ Name=iris_cvp",
		bRetVal, dev->cvp_hal_data->irq);
	if (bRetVal)
		dprintk(CVP_DBG, "() :cam_presil_unsubscribe_device_irq succeeded\n");
	else
		dprintk(CVP_ERR, "() :cam_presil_unsubscribe_device_irq failed\n");

}

void presil42_send_wncc_buffer(struct msm_cvp_smem *smem, struct cvp_internal_buf *cbuf)
{
	dprintk(CVP_DBG, "%s: %x for cam_presil_send_buffer with MAP_ADDR_OFFSET %x",
		__func__, (u64)(smem->device_addr) - MAP_ADDR_OFFSET, MAP_ADDR_OFFSET);

	cam_presil_send_buffer((u64)smem->dma_buf, 0,
		(u32)cbuf->offset, (u32)cbuf->size,
		(u64)(smem->device_addr) - MAP_ADDR_OFFSET,
		(uintptr_t)NULL, false);
}

void presil42_send_map_user_persist_buffer(struct msm_cvp_smem *smem,  u32 *iova,
					struct cvp_internal_buf *pbuf)
{
	*iova = smem->device_addr;

	dprintk(CVP_DBG, "%s: %x : with MAP_ADDR_OFFSET %x, buf offset is %x\n",
		__func__, (u64)(*iova)-MAP_ADDR_OFFSET, MAP_ADDR_OFFSET, (u32)pbuf->offset);

	cam_presil_send_buffer((u64)smem->dma_buf, 0,
		0,
		(u32)smem->dma_buf->size,
		(u64)(*iova)-MAP_ADDR_OFFSET,
		(uintptr_t)NULL, false);
}

void presil42_send_map_frame_buffer(struct msm_cvp_smem *smem,  u32 iova, struct cvp_buf_type *buf)
{
	iova = smem->device_addr;

	dprintk(CVP_DBG,
		"%s:presil_send_buffer  %x : offset %d size %d iova %x MAP_ADDR_OFFSET %d",
		__func__, (u64)smem->dma_buf, (u32)buf->offset, (u32)buf->size,
		(u64)iova - MAP_ADDR_OFFSET, MAP_ADDR_OFFSET);

	cam_presil_send_buffer((u64)smem->dma_buf, 0, 0,
		(u32)smem->dma_buf->size,
		(u64)iova - MAP_ADDR_OFFSET,
		(uintptr_t)NULL, false);
}

void presil42_unmap_frame_buf(struct msm_cvp_smem *smem, struct cvp_internal_buf *buf)
{
	dprintk(CVP_DBG,
		"%s: cam_presil_retrieve_buffer %x : offset %d size %d iova %x MAP_ADDR_OFFSET %d",
		__func__, (u64)smem->dma_buf,
		(u32)buf->offset, (u32)buf->size,
		(u64)smem->device_addr + buf->offset - MAP_ADDR_OFFSET, MAP_ADDR_OFFSET);

	cam_presil_retrieve_buffer(
		(u64)smem->dma_buf, 0,
		0,
		(u32)smem->dma_buf->size,
		(u64)smem->device_addr - MAP_ADDR_OFFSET,
		(uintptr_t)NULL, false);
}

void presil42_set_buf_fd(struct cvp_buf_type *buf, u32 iova, char *name)
{
	buf->fd = iova - MAP_ADDR_OFFSET;
	dprintk(CVP_DBG,
		"%s iova = %x with MAP_ADDR_OFFSET %x\n",
		name, iova - MAP_ADDR_OFFSET, MAP_ADDR_OFFSET);
}

void presil42_set_buf_iova(struct cvp_hfi_cmd_session_set_buffers_packet *pkt, u32 iova)
{
	dprintk(CVP_DBG,
		"arp buffer is %x for cvp_create_pkt_cmd_session_set_buffers for Presil\n",
		iova - MAP_ADDR_OFFSET);
	pkt->buf_type.iova =
		iova - MAP_ADDR_OFFSET;
}
void presil42_set_smem_flags(u32 smem_flags)
{
	smem_flags |= SMEM_UNCACHED; /*SMEM_NON_PIXEL*/;
}

int presil42_set_irq_settings(struct cvp_hal_data *hal, struct iris_hfi_device *device, int rc)
{

	//CVP IRQ Presil Set Memory
	//set_presil_base_address(101, hal->register_base, hal->firmware_base,
	//"iris_cvp", hal->register_size, 0); PreSil API changes due to the
	//last Register base address. Set to 0 now.

	int bRetVal = false;

	bRetVal = cam_presil_subscribe_device_irq(hal->irq, cvp_hfi_isr, device,
		"iris_cvp");

	dprintk(CVP_DBG, "%s: Subscribe for CVP IRQ: Ret=%d IRQ NUM=%d irq_data=0x%x",
		__func__, bRetVal, hal->irq, device, cvp_hfi_isr);

	dprintk(CVP_DBG, "%s: IRQ Name=iris_cvp IRQ-handler=0x%x", cvp_hfi_isr);

	if (bRetVal) {
		dprintk(CVP_DBG, "() :cam_presil_subscribe_device_irq succeeded\n");
		rc = 0;
	} else {
		dprintk(CVP_ERR, "() :cam_presil_subscribe_device_irq failed\n");
		rc = -EEXIST;
		kfree(hal);
	}
	return rc;
}
#endif
