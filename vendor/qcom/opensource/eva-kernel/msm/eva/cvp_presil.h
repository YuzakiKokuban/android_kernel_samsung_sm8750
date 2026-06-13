/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __H_CVP_PRESIL_H__
#define __H_CVP_PRESIL_H__

#include "cvp_comm_def.h"

#ifdef USE_PRESIL42
#include "cam_presil_hw_access.h"
#include "cvp_core_hfi.h"
#include "msm_cvp_internal.h"
#include "msm_cvp_buf.h"
#include "eva_shared_def.h"

#define MAP_ADDR_OFFSET 0x0 /* 0xD0000000 */

/* This base just for between EVA ko and Presil impl in camera ko to extract reg offset value */
#define CVP_REG_BASE_ADDR (0x06C00000 +  0x1100000) /* for Rumi with both EVA and Camera */

#define CVP_PRESIL_HFI_REG_CMD_Q_IOVA 0x0000901
#define CVP_PRESIL_HFI_REG_MSG_Q_IOVA 0x0000902
#define CVP_PRESIL_HFI_REG_DBG_Q_IOVA 0x0000903

enum { Q_CMD, Q_MSG, Q_DBG };

void presil42_write_register(struct iris_hfi_device *device, u32 reg, u32 value);
int  presil42_read_register(u32 reg);
int  presil42_iface_cmdq_write_relaxed(struct cvp_hal_cmd_pkt_hdr *cmd_packet, void *pkt);
int  presil42_iface_msgq_read(void *pkt);
int  presil42_iface_dbgq_read(void *pkt);
void presil42_setup_ucregion_memory_map(struct iris_hfi_device *device);
void presil42_core_clear_interrupt(struct iris_hfi_device *device);
void presil42_hfi_core_work_handler(struct work_struct *work);
void presil42_setup_dsp_uc_memmap_vpu5(struct iris_hfi_device *device);
void presil42_cvp_iris_hfi_delete_device(struct iris_hfi_device *dev);
void presil42_send_wncc_buffer(struct msm_cvp_smem *smem, struct cvp_internal_buf *cbuf);
void presil42_send_map_user_persist_buffer(struct msm_cvp_smem *smem,  u32 *iova,
					struct cvp_internal_buf *pbuf);
void presil42_send_map_frame_buffer(struct msm_cvp_smem *smem,  u32 iova, struct cvp_buf_type *buf);
void presil42_unmap_frame_buf(struct msm_cvp_smem *smem, struct cvp_internal_buf *buf);
void presil42_set_buf_fd(struct cvp_buf_type *buf, u32 iova, char *name);
void presil42_set_buf_iova(struct cvp_hfi_cmd_session_set_buffers_packet *pkt, u32 iova);
int  presil42_set_irq_settings(struct cvp_hal_data *hal, struct iris_hfi_device *device, int rc);
void presil42_set_smem_flags(u32 smem_flags);
#endif
#endif
