/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __SMCI_QSEECOMCOMPAT_H
#define __SMCI_QSEECOMCOMPAT_H

#include "smci_object.h"
#include "IQSEEComCompat.h"

#define SMCI_QSEECOMCOMPAT_ERROR_APP_UNAVAILABLE INT32_C(10)
#define SMCI_QSEECOMCOMPAT_OP_SENDREQUEST 0
#define SMCI_QSEECOMCOMPAT_OP_DISCONNECT 1
#define SMCI_QSEECOMCOMPAT_OP_UNLOAD 2


static inline int32_t
smci_qseecomcompat_release(struct smci_object self)
{
	return IQSEEComCompat_release(self);
}

static inline int32_t
smci_qseecomcompat_retain(struct smci_object self)
{
	return IQSEEComCompat_retain(self);
}

static inline int32_t
smci_qseecomcompat_sendrequest(struct smci_object self,
		const void *req_in_ptr, size_t req_in_len,
		const void *rsp_in_ptr, size_t rsp_in_len,
		void *req_out_ptr, size_t req_out_len, size_t *req_out_lenout,
		void *rsp_out_ptr, size_t rsp_out_len, size_t *rsp_out_lenout,
		const uint32_t *embedded_buf_offsets_ptr,
		size_t embedded_buf_offsets_len, uint32_t is64_val,
		struct smci_object smo1_val, struct smci_object smo2_val,
		struct smci_object smo3_val, struct smci_object smo4_val)
{
	return IQSEEComCompat_sendRequest(self,
		req_in_ptr, req_in_len,
		rsp_in_ptr, rsp_in_len,
		req_out_ptr, req_out_len, req_out_lenout,
		rsp_out_ptr, rsp_out_len, rsp_out_lenout,
		embedded_buf_offsets_ptr,
		embedded_buf_offsets_len, is64_val,
		smo1_val, smo2_val,
		smo3_val, smo4_val);
}

static inline int32_t
smci_qseecomcompat_disconnect(struct smci_object self)
{
	return IQSEEComCompat_disconnect(self);
}

static inline int32_t
smci_qseecomcompat_unload(struct smci_object self)
{
	return IQSEEComCompat_unload(self);
}

#endif /* __SMCI_QSEECOMCOMPAT_H */
