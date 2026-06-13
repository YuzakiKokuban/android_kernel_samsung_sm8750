// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/uaccess.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#include "hw_fence_drv_priv.h"
#include "hw_fence_drv_utils.h"
#include "hw_fence_drv_ipc.h"
#include "hw_fence_drv_debug.h"
#include "hw_fence_drv_fence.h"
#if IS_ENABLED(CONFIG_QTI_HW_FENCE_USE_SYNX)
#include <synx_interop.h>
#else
#define SYNX_HW_FENCE_HANDLE_FLAG 0
#define SYNX_STATE_SIGNALED_CANCEL 4
#endif /* CONFIG_QTI_HW_FENCE_USE_SYNX */

/* Global atomic lock */
#define GLOBAL_ATOMIC_STORE(drv_data, lock, val) global_atomic_store(drv_data, lock, val)

#define IS_HW_FENCE_TX_QUEUE(queue_type) ((queue_type) == HW_FENCE_TX_QUEUE - 1)

#define REQUIRES_IDX_TRANSLATION(queue) \
	((queue)->rd_wr_idx_factor && ((queue)->rd_wr_idx_start || (queue)->rd_wr_idx_factor > 1))

#define IDX_TRANSLATE_CUSTOM_TO_DEFAULT(queue, idx) \
	(((idx) - (queue)->rd_wr_idx_start) * (queue)->rd_wr_idx_factor)

#define IDX_TRANSLATE_DEFAULT_TO_CUSTOM(queue, idx) \
	(((idx) / (queue)->rd_wr_idx_factor) + (queue)->rd_wr_idx_start)

/* number of fences searched for HW Fence import */
#define HW_FENCE_FIND_THRESHOLD 10

inline u64 hw_fence_get_qtime(struct hw_fence_driver_data *drv_data)
{
#ifdef HWFENCE_USE_SLEEP_TIMER
	return readl_relaxed(drv_data->qtime_io_mem);
#else /* USE QTIMER */
	return arch_timer_read_counter();
#endif /* HWFENCE_USE_SLEEP_TIMER */
}

/* on targets with soccp, read_index and write_index etc. fields are in different locations */
void hw_fence_get_queue_idx_ptrs(struct hw_fence_driver_data *drv_data, void *va_header,
	u32 **rd_idx_ptr, u32 **wr_idx_ptr, u32 **tx_wm_ptr)
{
	struct msm_hw_fence_hfi_queue_header *hfi_header;
	struct msm_hw_fence_hfi_queue_header_v2 *hfi_header_v2;

	/* if soccp is present, use v2 header data structures */
	if (drv_data->has_soccp) {
		hfi_header_v2 = va_header;
		*rd_idx_ptr = &hfi_header_v2->read_index;
		*wr_idx_ptr = &hfi_header_v2->write_index;
		if (tx_wm_ptr)
			*tx_wm_ptr = &hfi_header_v2->tx_wm;
	} else {
		hfi_header = va_header;
		*rd_idx_ptr = &hfi_header->read_index;
		*wr_idx_ptr = &hfi_header->write_index;
		if (tx_wm_ptr)
			*tx_wm_ptr = &hfi_header->tx_wm;
	}
}

static int init_hw_fences_queues(struct hw_fence_driver_data *drv_data,
	enum hw_fence_mem_reserve mem_reserve_id,
	struct msm_hw_fence_mem_addr *mem_descriptor,
	struct msm_hw_fence_queue *queues, int queues_num,
	int client_id)
{
	struct msm_hw_fence_hfi_queue_table_header *hfi_table_header;
	struct msm_hw_fence_hfi_queue_header *hfi_queue_header;
	struct hw_fence_client_type_desc *desc;
	void *ptr, *qptr;
	phys_addr_t phys, qphys;
	u32 size, start_queue_offset, txq_idx_start = 0, txq_idx_factor = 1;
	u32 *wr_idx_ptr, *rd_idx_ptr, *tx_wm_ptr;
	int headers_size, queue_size, payload_size;
	int start_padding = 0, end_padding = 0;
	int i, ret = 0;
	bool skip_txq_wr_idx = false;

	HWFNC_DBG_INIT("mem_reserve_id:%d client_id:%d\n", mem_reserve_id, client_id);
	switch (mem_reserve_id) {
	case HW_FENCE_MEM_RESERVE_CTRL_QUEUE:
		headers_size = HW_FENCE_HFI_CTRL_HEADERS_SIZE(drv_data->has_soccp);
		queue_size = drv_data->hw_fence_ctrl_queue_size;
		payload_size = HW_FENCE_CTRL_QUEUE_PAYLOAD;
		break;
	case HW_FENCE_MEM_RESERVE_CLIENT_QUEUE:
		if (client_id >= drv_data->clients_num ||
				!drv_data->hw_fence_client_queue_size[client_id].type) {
			HWFNC_ERR("Invalid client_id:%d for clients_num:%u\n", client_id,
				drv_data->clients_num);
			return -EINVAL;
		}

		desc = drv_data->hw_fence_client_queue_size[client_id].type;
		start_padding = desc->start_padding;
		end_padding = desc->end_padding;
		headers_size = HW_FENCE_HFI_CLIENT_HEADERS_SIZE(queues_num, drv_data->has_soccp) +
			start_padding + end_padding;
		queue_size = HW_FENCE_CLIENT_QUEUE_PAYLOAD * desc->queue_entries;
		payload_size = HW_FENCE_CLIENT_QUEUE_PAYLOAD;
		txq_idx_start = desc->txq_idx_start;
		txq_idx_factor = desc->txq_idx_factor ? desc->txq_idx_factor : 1;
		skip_txq_wr_idx = desc->skip_txq_wr_idx;
		break;
	default:
		HWFNC_ERR("Unexpected mem reserve id: %d\n", mem_reserve_id);
		return -EINVAL;
	}

	/* Reserve Virtual and Physical memory for HFI headers */
	ret = hw_fence_utils_reserve_mem(drv_data, mem_reserve_id, &phys, &ptr, &size, client_id);
	if (ret) {
		HWFNC_ERR("Failed to reserve id:%d client %d\n", mem_reserve_id, client_id);
		return -ENOMEM;
	}
	HWFNC_DBG_INIT("phys:0x%llx ptr:0x%pK size:%d\n", phys, ptr, size);

	/* Populate Memory descriptor with address */
	mem_descriptor->virtual_addr = ptr;
	mem_descriptor->device_addr = phys;
	mem_descriptor->size = size; /* bytes */
	mem_descriptor->mem_data = NULL; /* Currently we don't need any special info */

	HWFNC_DBG_INIT("Initialize headers: headers_size:%d start_padding:%d end_padding:%d\n",
		headers_size, start_padding, end_padding);
	/* Initialize headers info within hfi memory */
	hfi_table_header = (struct msm_hw_fence_hfi_queue_table_header *)ptr;
	hfi_table_header->version = 0;
	hfi_table_header->size = size; /* bytes */
	/* Offset, from the Base Address, where the first queue header starts */
	hfi_table_header->qhdr0_offset = HW_FENCE_HFI_TABLE_HEADER_SIZE(drv_data->has_soccp) +
		start_padding;
	hfi_table_header->qhdr_size = HW_FENCE_HFI_QUEUE_HEADER_SIZE(drv_data->has_soccp);
	hfi_table_header->num_q = queues_num; /* number of queues */
	hfi_table_header->num_active_q = queues_num;

	/* Initialize Queues Info within HFI memory */

	/*
	 * Calculate offset where hfi queue header starts, which it is at the
	 * end of the hfi table header
	 */
	HWFNC_DBG_INIT("Initialize queues\n");
	hfi_queue_header = (struct msm_hw_fence_hfi_queue_header *)
					   ((char *)ptr + hfi_table_header->qhdr0_offset);
	for (i = 0; i < queues_num; i++) {
		HWFNC_DBG_INIT("init queue[%d]\n", i);

		/* Calculate the offset where the Queue starts */
		start_queue_offset = headers_size + (i * queue_size); /* Bytes */
		qphys = phys + start_queue_offset; /* start of the PA for the queue elems */
		qptr = (char *)ptr + start_queue_offset; /* start of the va for queue elems */

		/* Set the physical start address in the HFI queue header */
		hfi_queue_header->start_addr = qphys;

		/* Set the queue type (i.e. RX or TX queue) */
		hfi_queue_header->type = IS_HW_FENCE_TX_QUEUE(i) ? HW_FENCE_TX_QUEUE :
			HW_FENCE_RX_QUEUE;

		/* Set the size of this header */
		hfi_queue_header->queue_size = queue_size;

		/* Set the payload size */
		hfi_queue_header->pkt_size = payload_size;

		hw_fence_get_queue_idx_ptrs(drv_data, hfi_queue_header, &rd_idx_ptr, &wr_idx_ptr,
			&tx_wm_ptr);

		/* Set write index for clients' tx queues that index from nonzero value */
		if (txq_idx_start && IS_HW_FENCE_TX_QUEUE(i) && !*wr_idx_ptr) {
			if (skip_txq_wr_idx)
				*tx_wm_ptr = txq_idx_start;
			*rd_idx_ptr = txq_idx_start;
			*wr_idx_ptr = txq_idx_start;
			HWFNC_DBG_INIT("init:TX_QUEUE client:%d rd_idx=%s=%u\n", client_id,
				skip_txq_wr_idx ? "wr_idx=tx_wm" : "wr_idx",
				txq_idx_start);
		}

		/* Update memory for hfi_queue_header */
		wmb();

		/* Store Memory info in the Client data */
		queues[i].va_queue = qptr;
		queues[i].pa_queue = qphys;
		queues[i].va_header = hfi_queue_header;
		queues[i].q_size_bytes = queue_size;
		HWFNC_DBG_INIT("init:%s client:%d q[%d] va=0x%pK pa=0x%llx hd:0x%pK sz:%u pkt:%d\n",
			hfi_queue_header->type == HW_FENCE_TX_QUEUE ? "TX_QUEUE" : "RX_QUEUE",
			client_id, i, queues[i].va_queue, queues[i].pa_queue, queues[i].va_header,
			queues[i].q_size_bytes, payload_size);

		/* Store additional tx queue rd_wr_idx properties */
		if (IS_HW_FENCE_TX_QUEUE(i)) {
			queues[i].rd_wr_idx_start = txq_idx_start;
			queues[i].rd_wr_idx_factor = txq_idx_factor;
			queues[i].skip_wr_idx = skip_txq_wr_idx;
		} else {
			queues[i].rd_wr_idx_factor = 1;
		}
		HWFNC_DBG_INIT("rd_wr_idx_start:%u rd_wr_idx_factor:%u skip_wr_idx:%s\n",
			queues[i].rd_wr_idx_start, queues[i].rd_wr_idx_factor,
			queues[i].skip_wr_idx ? "true" : "false");

		/* Next header */
		hfi_queue_header = (struct msm_hw_fence_hfi_queue_header *)
			((char *)hfi_queue_header + hfi_table_header->qhdr_size);
	}

	return ret;
}

static inline bool _lock_client_queue(int queue_type)
{
	/* Only lock Rx Queue */
	return (queue_type == (HW_FENCE_RX_QUEUE - 1)) ? true : false;
}

char *_get_queue_type(int queue_type)
{
	return (queue_type == (HW_FENCE_RX_QUEUE - 1)) ? "RXQ" : "TXQ";
}

static void _translate_queue_indexes_custom_to_default(struct msm_hw_fence_queue *queue,
	u32 *read_idx, u32 *write_idx)
{
	if (REQUIRES_IDX_TRANSLATION(queue)) {
		*read_idx = IDX_TRANSLATE_CUSTOM_TO_DEFAULT(queue, *read_idx);
		*write_idx = IDX_TRANSLATE_CUSTOM_TO_DEFAULT(queue, *write_idx);
		HWFNC_DBG_Q("rd_idx_u32:%u wr_idx_u32:%u rd_wr_idx start:%u factor:%u\n",
			*read_idx, *write_idx, queue->rd_wr_idx_start, queue->rd_wr_idx_factor);
	}
}

int hw_fence_read_queue(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct msm_hw_fence_queue_payload *payload,
	int queue_type)
{
	struct msm_hw_fence_queue *queue;

	if (queue_type >= HW_FENCE_CLIENT_QUEUES || !hw_fence_client || !payload) {
		HWFNC_ERR("Invalid queue type:%d hw_fence_client:0x%pK payload:0x%pK\n", queue_type,
			hw_fence_client, payload);
		return -EINVAL;
	}

	queue = &hw_fence_client->queues[queue_type];
	HWFNC_DBG_Q("read client:%d queue:0x%pK\n", hw_fence_client->client_id, queue);

	return hw_fence_read_queue_helper(drv_data, queue, payload);
}

int hw_fence_read_queue_helper(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_queue *queue, struct msm_hw_fence_queue_payload *payload)
{
	u32 read_idx, write_idx, to_read_idx;
	u32 *read_ptr, *rd_idx_ptr, *wr_idx_ptr;
	u32 payload_size_u32, q_size_u32;
	struct msm_hw_fence_queue_payload *read_ptr_payload;

	q_size_u32 = (queue->q_size_bytes / sizeof(u32));
	payload_size_u32 = (sizeof(struct msm_hw_fence_queue_payload) / sizeof(u32));
	HWFNC_DBG_Q("sizeof payload:%lu\n", sizeof(struct msm_hw_fence_queue_payload));

	if (!queue->va_header || !payload) {
		HWFNC_ERR("Invalid queue\n");
		return -EINVAL;
	}

	hw_fence_get_queue_idx_ptrs(drv_data, queue->va_header, &rd_idx_ptr, &wr_idx_ptr, NULL);

	/* Make sure data is ready before read */
	mb();

	/* Get read and write index */
	read_idx = readl_relaxed(rd_idx_ptr);
	write_idx = readl_relaxed(wr_idx_ptr);

	/* translate read and write indexes from custom indexing to dwords with no offset */
	_translate_queue_indexes_custom_to_default(queue, &read_idx, &write_idx);

	HWFNC_DBG_Q("read rd_ptr:0x%pK wr_ptr:0x%pK rd_idx:%d wr_idx:%d queue:0x%pK\n",
		rd_idx_ptr, wr_idx_ptr, read_idx, write_idx, queue);

	if (read_idx == write_idx) {
		HWFNC_DBG_Q("Nothing to read!\n");
		return -EINVAL;
	}

	/* Move the pointer where we need to read and cast it */
	read_ptr = ((u32 *)queue->va_queue + read_idx);
	read_ptr_payload = (struct msm_hw_fence_queue_payload *)read_ptr;
	HWFNC_DBG_Q("read_ptr:0x%pK queue: va=0x%pK pa=0x%llx read_ptr_payload:0x%pK\n", read_ptr,
		queue->va_queue, queue->pa_queue, read_ptr_payload);

	/* Calculate the index after the read */
	to_read_idx = read_idx + payload_size_u32;

	/*
	 * wrap-around case, here we are reading the last element of the queue, therefore set
	 * to_read_idx, which is the index after the read, to the beginning of the
	 * queue
	 */
	if (to_read_idx >= q_size_u32)
		to_read_idx = 0;

	/* translate to_read_idx to custom indexing with offset */
	if (REQUIRES_IDX_TRANSLATION(queue)) {
		to_read_idx = IDX_TRANSLATE_DEFAULT_TO_CUSTOM(queue, to_read_idx);
		HWFNC_DBG_Q("translated to_read_idx:%u rd_wr_idx start:%u factor:%u\n",
			to_read_idx, queue->rd_wr_idx_start, queue->rd_wr_idx_factor);
	}

	/* Read the Client Queue */
	*payload = *read_ptr_payload;

	/* update the read index */
	writel_relaxed(to_read_idx, rd_idx_ptr);

	/* update memory for the index */
	wmb();

	/* Return one if queue still has contents after read */
	return to_read_idx == write_idx ? 0 : 1;
}

static int _get_update_queue_params(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_queue *queue, u32 *q_size_u32, u32 *payload_size,
	u32 *payload_size_u32, u32 **rd_idx_ptr, u32 **wr_ptr)
{
	u32 *tx_wm_ptr;

	if (!queue || !queue->va_header) {
		HWFNC_ERR("invalid queue\n");
		return -EINVAL;
	}

	*q_size_u32 = (queue->q_size_bytes / sizeof(u32));
	*payload_size = sizeof(struct msm_hw_fence_queue_payload);
	*payload_size_u32 = (*payload_size / sizeof(u32));

	hw_fence_get_queue_idx_ptrs(drv_data, queue->va_header, rd_idx_ptr, wr_ptr, &tx_wm_ptr);

	/* if skipping update wr_index, then use hfi_header->tx_wm instead */
	if (queue->skip_wr_idx)
		*wr_ptr = tx_wm_ptr;

	return 0;
}

int hw_fence_update_queue_helper(struct hw_fence_driver_data *drv_data, u32 client_id,
	struct msm_hw_fence_queue *queue, u16 type, u64 ctxt_id, u64 seqno, u64 hash, u64 flags,
	u64 client_data, u32 error, int queue_type)
{
	u32 read_idx;
	u32 write_idx;
	u32 to_write_idx;
	u32 q_size_u32;
	u32 q_free_u32;
	u32 *q_payload_write_ptr;
	u32 payload_size, payload_size_u32;
	struct msm_hw_fence_queue_payload *write_ptr_payload;
	bool lock_client = false;
	u32 lock_idx;
	u64 timestamp;
	u32 *rd_idx_ptr, *wr_ptr;
	int ret = 0;

	if (_get_update_queue_params(drv_data, queue, &q_size_u32, &payload_size,
			&payload_size_u32, &rd_idx_ptr, &wr_ptr)) {
		HWFNC_ERR("Invalid client:%d q_type:%d queue\n", client_id, queue_type);
		return -EINVAL;
	}

	/*
	 * We need to lock the client if there is an Rx Queue update, since that
	 * is the only time when HW Fence driver can have a race condition updating
	 * the Rx Queue, which also could be getting updated by the Fence CTL
	 */
	lock_client = _lock_client_queue(queue_type);
	if (lock_client) {
		lock_idx = (client_id - 1) * HW_FENCE_LOCK_IDX_OFFSET;

		if (lock_idx >= drv_data->client_lock_tbl_cnt) {
			HWFNC_ERR("can't reset rxq, lock for client:%d lock_idx:%d exceed max:%d\n",
				client_id, lock_idx, drv_data->client_lock_tbl_cnt);
			return -EINVAL;
		}
		HWFNC_DBG_Q("Locking client id:%d: idx:%d\n", client_id, lock_idx);

		/* lock the client rx queue to update */
		GLOBAL_ATOMIC_STORE(drv_data, &drv_data->client_lock_tbl[lock_idx], 1); /* lock */
	}

	/* Make sure data is ready before read */
	mb();

	/* Get read and write index */
	read_idx = readl_relaxed(rd_idx_ptr);
	write_idx = readl_relaxed(wr_ptr);

	HWFNC_DBG_Q("wr client:%d r_ptr:0x%pK w_ptr:0x%pK r_idx:%d w_idx:%d q:0x%pK type:%d s:%s\n",
		client_id, rd_idx_ptr, wr_ptr, read_idx, write_idx, queue, queue_type,
		queue->skip_wr_idx ? "true" : "false");

	/* translate read and write indexes from custom indexing to dwords with no offset */
	_translate_queue_indexes_custom_to_default(queue, &read_idx, &write_idx);

	/* Check queue to make sure message will fit */
	q_free_u32 = read_idx <= write_idx ? (q_size_u32 - (write_idx - read_idx)) :
		(read_idx - write_idx);
	if (q_free_u32 <= payload_size_u32) {
		HWFNC_ERR("cannot fit the message size:%d\n", payload_size_u32);
		ret = -EINVAL;
		goto exit;
	}
	HWFNC_DBG_Q("q_free_u32:%d payload_size_u32:%d\n", q_free_u32, payload_size_u32);

	/* Move the pointer where we need to write and cast it */
	q_payload_write_ptr = ((u32 *)queue->va_queue + write_idx);
	write_ptr_payload = (struct msm_hw_fence_queue_payload *)q_payload_write_ptr;
	HWFNC_DBG_Q("q_payload_write_ptr:0x%pK queue: va=0x%pK pa=0x%llx write_ptr_payload:0x%pK\n",
		q_payload_write_ptr, queue->va_queue, queue->pa_queue, write_ptr_payload);

	/* calculate the index after the write */
	to_write_idx = write_idx + payload_size_u32;

	HWFNC_DBG_Q("to_write_idx:%u write_idx:%u payload_size:%u\n", to_write_idx, write_idx,
		payload_size_u32);
	HWFNC_DBG_L("client_id:%d update %s type:%u hash:%llu ctx:%llu seq:%llu flags:%llu e:%u\n",
		client_id, _get_queue_type(queue_type), type, hash, ctxt_id, seqno, flags, error);

	/*
	 * wrap-around case, here we are writing to the last element of the queue, therefore
	 * set to_write_idx, which is the index after the write, to the beginning of the
	 * queue
	 */
	if (to_write_idx >= q_size_u32)
		to_write_idx = 0;

	/* translate to_write_idx to custom indexing with offset */
	if (REQUIRES_IDX_TRANSLATION(queue)) {
		to_write_idx = IDX_TRANSLATE_DEFAULT_TO_CUSTOM(queue, to_write_idx);
		HWFNC_DBG_Q("translated to_write_idx:%d rd_wr_idx start:%d factor:%d\n",
			to_write_idx, queue->rd_wr_idx_start, queue->rd_wr_idx_factor);
	}

	/* Update Client Queue */
	writeq_relaxed(payload_size, &write_ptr_payload->size);
	writew_relaxed(type, &write_ptr_payload->type);
	writew_relaxed(HW_FENCE_PAYLOAD_REV(1, 0), &write_ptr_payload->version);
	writeq_relaxed(ctxt_id, &write_ptr_payload->ctxt_id);
	writeq_relaxed(seqno, &write_ptr_payload->seqno);
	writeq_relaxed(hash | SYNX_HW_FENCE_HANDLE_FLAG, &write_ptr_payload->hash);
	writeq_relaxed(flags, &write_ptr_payload->flags);
	writeq_relaxed(client_data, &write_ptr_payload->client_data);
	writel_relaxed(error, &write_ptr_payload->error);
	timestamp = hw_fence_get_qtime(drv_data);
	writel_relaxed(timestamp, &write_ptr_payload->timestamp_lo);
	writel_relaxed(timestamp >> 32, &write_ptr_payload->timestamp_hi);

	/* update memory for the message */
	wmb();

	/* update the write index */
	writel_relaxed(to_write_idx, wr_ptr);

	/* update memory for the index */
	wmb();

exit:
	if (lock_client)
		GLOBAL_ATOMIC_STORE(drv_data, &drv_data->client_lock_tbl[lock_idx], 0); /* unlock */

	return ret;
}

/*
 * This function writes to the queue of the client. The 'queue_type' determines
 * if this function is writing to the rx or tx queue
 */
int hw_fence_update_queue(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, u64 ctxt_id, u64 seqno, u64 hash,
	u64 flags, u64 client_data, u32 error, int queue_type)
{
	struct msm_hw_fence_queue *queue;

	if (queue_type >= hw_fence_client->queues_num) {
		HWFNC_ERR("Invalid queue type:%d client_id:%d q_num:%d\n", queue_type,
			hw_fence_client->client_id, hw_fence_client->queues_num);
		return -EINVAL;
	}
	queue = &hw_fence_client->queues[queue_type];

	return hw_fence_update_queue_helper(drv_data, hw_fence_client->client_id, queue,
		HW_FENCE_PAYLOAD_TYPE_1, ctxt_id, seqno, hash, flags, client_data, error,
		queue_type);
}

int hw_fence_update_existing_txq_payload(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, u64 hash, u32 error)
{
	u32 q_size_u32, payload_size, payload_size_u32, read_idx, write_idx, second_idx;
	struct msm_hw_fence_queue_payload tmp, *first_payload, *second_payload;
	struct msm_hw_fence_queue *queue;
	u32 *rd_idx_ptr, *wr_ptr;
	int ret = 0;

	queue = &hw_fence_client->queues[HW_FENCE_TX_QUEUE - 1];
	if (_get_update_queue_params(drv_data, queue, &q_size_u32, &payload_size,
			&payload_size_u32, &rd_idx_ptr, &wr_ptr)) {
		HWFNC_ERR("Invalid client:%d tx queue\n", hw_fence_client->client_id);
		return -EINVAL;
	}

	/* Make sure data is ready before read */
	mb();

	/* Get read and write index */
	read_idx = *rd_idx_ptr;
	write_idx = *wr_ptr;

	/* translate read and write indexes from custom indexing to dwords with no offset */
	_translate_queue_indexes_custom_to_default(queue, &read_idx, &write_idx);

	if (read_idx == write_idx) {
		HWFNC_DBG_Q("Empty queue, no entry matches with hash:%llu\n", hash);
		return -EINVAL;
	}

	first_payload = (struct msm_hw_fence_queue_payload *)((u32 *)queue->va_queue + read_idx);
	HWFNC_DBG_Q("client:%d txq: va=0x%pK pa=0x%llx idx:%d ptr_payload:0x%pK\n",
		hw_fence_client->client_id, queue->va_queue, queue->pa_queue, read_idx,
		first_payload);

	if (first_payload->hash == hash) {
		/* Swap not needed, update first payload in client queue with fence error */
		first_payload->error = error;
	} else {
		/* Check whether second entry matches hash */
		second_idx = read_idx + payload_size_u32;

		/* wrap-around case */
		if (second_idx >= q_size_u32)
			second_idx = 0;

		if (second_idx == write_idx) {
			HWFNC_ERR("Failed to find matching entry with hash:%llu\n", hash);
			return -EINVAL;
		}

		second_payload = (struct msm_hw_fence_queue_payload *)
			((u32 *)queue->va_queue + second_idx);
		HWFNC_DBG_Q("client:%d txq: va=0x%pK pa=0x%llx idx:%d ptr_payload:0x%pK\n",
			hw_fence_client->client_id, queue->va_queue, queue->pa_queue, second_idx,
			second_payload);

		if (second_payload->hash != hash) {
			HWFNC_ERR("hash:%llu not found in first two queue payloads:%u, %u\n", hash,
				read_idx, second_idx);
			return -EINVAL;
		}

		/* swap first and second payload, updating error field in new first payload */
		tmp = *first_payload;
		*first_payload = *second_payload;
		first_payload->error = error;
		*second_payload = tmp;

		HWFNC_DBG_L("client_id:%d txq move from idx:%u to idx:%u hash:%llu c:%llu s:%llu\n",
			hw_fence_client->client_id, read_idx, second_idx, hash, tmp.ctxt_id,
			tmp.seqno);
	}

	/* update memory for the messages */
	wmb();

	HWFNC_DBG_L("client_id:%d update tx queue index:%u hash:%llu error:%u\n",
		hw_fence_client->client_id, read_idx, hash, error);

	return ret;
}

static int init_global_locks(struct hw_fence_driver_data *drv_data)
{
	struct msm_hw_fence_mem_addr *mem_descriptor;
	phys_addr_t phys;
	void *ptr;
	u32 size;
	int ret;

	ret = hw_fence_utils_reserve_mem(drv_data, HW_FENCE_MEM_RESERVE_LOCKS_REGION, &phys, &ptr,
		&size, 0);
	if (ret) {
		HWFNC_ERR("Failed to reserve clients locks mem %d\n", ret);
		return -ENOMEM;
	}
	HWFNC_DBG_INIT("phys:0x%llx ptr:0x%pK size:%d\n", phys, ptr, size);

	/* Populate Memory descriptor with address */
	mem_descriptor = &drv_data->clients_locks_mem_desc;
	mem_descriptor->virtual_addr = ptr;
	mem_descriptor->device_addr = phys;
	mem_descriptor->size = size;
	mem_descriptor->mem_data = NULL; /* not storing special info for now */

	/* Initialize internal pointers for managing the tables */
	drv_data->client_lock_tbl = (u64 *)drv_data->clients_locks_mem_desc.virtual_addr;
	drv_data->client_lock_tbl_cnt = drv_data->clients_locks_mem_desc.size / sizeof(u64);

	return 0;
}

static int init_hw_fences_table(struct hw_fence_driver_data *drv_data)
{
	struct msm_hw_fence_mem_addr *mem_descriptor;
	phys_addr_t phys;
	void *ptr;
	u32 size;
	int ret;

	ret = hw_fence_utils_reserve_mem(drv_data, HW_FENCE_MEM_RESERVE_TABLE, &phys, &ptr,
		&size, 0);
	if (ret) {
		HWFNC_ERR("Failed to reserve table mem %d\n", ret);
		return -ENOMEM;
	}
	HWFNC_DBG_INIT("phys:0x%llx ptr:0x%pK size:%d\n", phys, ptr, size);

	/* Populate Memory descriptor with address */
	mem_descriptor = &drv_data->hw_fences_mem_desc;
	mem_descriptor->virtual_addr = ptr;
	mem_descriptor->device_addr = phys;
	mem_descriptor->size = size;
	mem_descriptor->mem_data = NULL; /* not storing special info for now */

	/* Initialize internal pointers for managing the tables */
	drv_data->hw_fences_tbl = (struct msm_hw_fence *)drv_data->hw_fences_mem_desc.virtual_addr;
	drv_data->hw_fences_tbl_cnt = drv_data->hw_fences_mem_desc.size /
		sizeof(struct msm_hw_fence);

	HWFNC_DBG_INIT("hw_fences_table:0x%pK cnt:%u\n", drv_data->hw_fences_tbl,
		drv_data->hw_fences_tbl_cnt);

	return 0;
}

static int init_hw_fences_events(struct hw_fence_driver_data *drv_data)
{
	phys_addr_t phys;
	void *ptr;
	u32 size;
	int ret;

	ret = hw_fence_utils_reserve_mem(drv_data, HW_FENCE_MEM_RESERVE_EVENTS_BUFF, &phys, &ptr,
		&size, 0);
	if (ret) {
		HWFNC_DBG_INFO("Failed to reserve events buffer %d\n", ret);
		return -ENOMEM;
	}
	drv_data->events = (struct msm_hw_fence_event *)ptr;
	drv_data->total_events = size / sizeof(struct msm_hw_fence_event);
	HWFNC_DBG_INIT("events:0x%pK total_events:%u event_sz:%lu total_size:%u\n",
		drv_data->events, drv_data->total_events, sizeof(struct msm_hw_fence_event), size);

	return 0;
}

static int init_ctrl_queue(struct hw_fence_driver_data *drv_data)
{
	struct msm_hw_fence_mem_addr *mem_descriptor;
	int ret;

	mem_descriptor = &drv_data->ctrl_queue_mem_desc;

	/* Init ctrl queue */
	ret = init_hw_fences_queues(drv_data, HW_FENCE_MEM_RESERVE_CTRL_QUEUE,
		mem_descriptor, drv_data->ctrl_queues,
		HW_FENCE_CTRL_QUEUES, 0);
	if (ret)
		HWFNC_ERR("Failure to init ctrl queue\n");

	return ret;
}

static void hw_fence_dma_fence_init_hash_table(struct hw_fence_driver_data *drv_data)
{
	hash_init(drv_data->dma_fence_table);
	spin_lock_init(&drv_data->dma_fence_table_lock);
}

int hw_fence_init(struct hw_fence_driver_data *drv_data)
{
	int ret;
	__le32 *mem;

	ret = hw_fence_utils_parse_dt_props(drv_data);
	if (ret) {
		HWFNC_ERR("failed to set dt properties\n");
		goto exit;
	}

	/* Allocate hw fence driver mem pool and share it with HYP */
	ret = hw_fence_utils_alloc_mem(drv_data);
	if (ret) {
		HWFNC_ERR("failed to alloc base memory\n");
		goto exit;
	}

	/* Initialize ctrl queue */
	ret = init_ctrl_queue(drv_data);
	if (ret)
		goto exit;

	ret = init_global_locks(drv_data);
	if (ret)
		goto exit;
	HWFNC_DBG_INIT("Locks allocated at 0x%pK total locks:%d\n", drv_data->client_lock_tbl,
		drv_data->client_lock_tbl_cnt);

	/* Initialize hw fences table */
	ret = init_hw_fences_table(drv_data);
	if (ret)
		goto exit;

	/* Initialize event log */
	ret = init_hw_fences_events(drv_data);
	if (ret)
		HWFNC_DBG_INFO("Unable to init events\n");

	/* Map ipcc registers */
	ret = hw_fence_utils_map_ipcc(drv_data);
	if (ret) {
		HWFNC_ERR("ipcc regs mapping failed\n");
		goto exit;
	}

	/* Map time register */
	ret = hw_fence_utils_map_qtime(drv_data);
	if (ret) {
		HWFNC_ERR("qtime reg mapping failed\n");
		goto exit;
	}

	/* Init debugfs */
	ret = hw_fence_debug_debugfs_register(drv_data);
	if (ret) {
		HWFNC_ERR("debugfs init failed\n");
		goto exit;
	}

	/* Init irq from fctl */
	if (drv_data->has_soccp)
		ret = hw_fence_utils_init_soccp_irq(drv_data);
	else
		ret = hw_fence_utils_init_virq(drv_data);
	if (ret) {
		HWFNC_ERR("failed to init irq has_soccp:%s\n", drv_data->has_soccp ? "true" :
			"false");
		goto exit;
	}

	if (drv_data->has_soccp) {
		ret = hw_fence_utils_register_soccp_ssr_notifier(drv_data);
		if (ret) {
			HWFNC_ERR("failed to register for soccp ssr notification\n");
			goto exit;
		}
	}

	hw_fence_dma_fence_init_hash_table(drv_data);

	mem = drv_data->io_mem_base;
	HWFNC_DBG_H("memory ptr:0x%pK val:0x%x\n", mem, *mem);

	HWFNC_DBG_INIT("HW Fences Table Initialized: 0x%pK cnt:%d\n",
		drv_data->hw_fences_tbl, drv_data->hw_fences_tbl_cnt);

exit:
	return ret;
}

int hw_fence_alloc_client_resources(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client,
	struct msm_hw_fence_mem_addr *mem_descriptor)
{
	int ret;

	if (!drv_data->hw_fence_client_queue_size[hw_fence_client->client_id].type) {
		HWFNC_ERR("invalid client_id:%d not reserved client queue; check dt props\n",
			hw_fence_client->client_id);
		return -EINVAL;
	}

	/* Init client queues */
	ret = init_hw_fences_queues(drv_data, HW_FENCE_MEM_RESERVE_CLIENT_QUEUE,
		&hw_fence_client->mem_descriptor, hw_fence_client->queues,
		drv_data->hw_fence_client_queue_size[hw_fence_client->client_id].type->queues_num,
		hw_fence_client->client_id);
	if (ret) {
		HWFNC_ERR("Failure to init the queue for client:%d\n",
			hw_fence_client->client_id);
		goto exit;
	}

	/* Init client memory descriptor */
	if (!IS_ERR_OR_NULL(mem_descriptor))
		memcpy(mem_descriptor, &hw_fence_client->mem_descriptor,
			sizeof(struct msm_hw_fence_mem_addr));
	else
		HWFNC_DBG_L("null mem descriptor, skipping copy\n");

exit:
	return ret;
}

int hw_fence_init_controller_signal(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client)
{
	int client_id, ret = 0;

	/*
	 * Initialize IPCC Signals for this client
	 *
	 * NOTE: For each Client HW-Core, the client drivers might be the ones making
	 * it's own initialization (in case that any hw-sequence must be enforced),
	 * however, if that is not the case, any per-client ipcc init to enable the
	 * signaling, can go here.
	 */
	switch ((int)hw_fence_client->client_id_ext) {
	case HW_FENCE_CLIENT_ID_CTX0:
		/* nothing to initialize for gpu client */
		break;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	case HW_FENCE_CLIENT_ID_VAL0:
	case HW_FENCE_CLIENT_ID_VAL1:
	case HW_FENCE_CLIENT_ID_VAL2:
	case HW_FENCE_CLIENT_ID_VAL3:
	case HW_FENCE_CLIENT_ID_VAL4:
	case HW_FENCE_CLIENT_ID_VAL5:
	case HW_FENCE_CLIENT_ID_VAL6:
		/* initialize ipcc signals for val clients */
		HWFNC_DBG_H("init_controller_signal: val client_id_ext:%d init:%d\n",
			hw_fence_client->client_id_ext, drv_data->ipcc_val_initialized);

		if (!drv_data->ipcc_val_initialized) {
			drv_data->ipcc_val_initialized = true;
			client_id = hw_fence_utils_get_client_id_priv(drv_data,
				HW_FENCE_CLIENT_ID_VAL0);

			if (drv_data->has_soccp) {
				/* init input-soccp signals for val clients */
				hw_fence_ipcc_enable_client_signal_pairs(drv_data, client_id);
			}
		}
		break;
#endif /* CONFIG_DEBUG_FS */
	case HW_FENCE_CLIENT_ID_CTL0:
	case HW_FENCE_CLIENT_ID_CTL1:
	case HW_FENCE_CLIENT_ID_CTL2:
	case HW_FENCE_CLIENT_ID_CTL3:
	case HW_FENCE_CLIENT_ID_CTL4:
	case HW_FENCE_CLIENT_ID_CTL5:
		/* initialize ipcc signals for dpu clients */
		HWFNC_DBG_H("init_controller_signal: DPU client_id_ext:%d initialized:%d\n",
			hw_fence_client->client_id_ext, drv_data->ipcc_dpu_initialized);
		if (!drv_data->ipcc_dpu_initialized) {
			drv_data->ipcc_dpu_initialized = true;
			client_id = hw_fence_utils_get_client_id_priv(drv_data,
				HW_FENCE_CLIENT_ID_CTL0);

			/* Init dpu client ipcc signal */
			hw_fence_ipcc_enable_protocol(drv_data, client_id);
			hw_fence_ipcc_enable_client_signal_pairs(drv_data, client_id);
		}
		break;
	case HW_FENCE_CLIENT_ID_IPE ... HW_FENCE_CLIENT_ID_IPE +
			MSM_HW_FENCE_MAX_SIGNAL_PER_CLIENT - 1:
		/* nothing to initialize for IPE client */
		break;
	case HW_FENCE_CLIENT_ID_VPU ... HW_FENCE_CLIENT_ID_VPU +
			MSM_HW_FENCE_MAX_SIGNAL_PER_CLIENT - 1:
		/* nothing to initialize for VPU client */
		break;
	case HW_FENCE_CLIENT_ID_IPA ... HW_FENCE_CLIENT_ID_IPA +
			MSM_HW_FENCE_MAX_SIGNAL_PER_CLIENT - 1:
		/* nothing to initialize for IPA clients */
		break;
	case HW_FENCE_CLIENT_ID_IFE0 ... HW_FENCE_CLIENT_ID_IFE11 +
			MSM_HW_FENCE_MAX_SIGNAL_PER_CLIENT - 1:
		/* nothing to initialize for IFE clients */
		break;
	default:
		HWFNC_ERR("Unexpected client_id_ext:%d\n", hw_fence_client->client_id_ext);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int hw_fence_init_controller_resources(struct msm_hw_fence_client *hw_fence_client)
{

	/*
	 * Initialize Fence Controller resources for this Client,
	 *  here we need to use the CTRL queue to communicate to the Fence
	 *  Controller the shared memory for the Rx/Tx queue for this client
	 *  as well as any information that Fence Controller might need to
	 *  know for this client.
	 *
	 * NOTE: For now, we are doing a static allocation of the
	 *  client's queues, so currently we don't need any notification
	 *  to the Fence CTL here through the CTRL queue.
	 *  Later-on we might need it, once the PVM to SVM (and vice versa)
	 *  communication for initialization is supported.
	 */

	return 0;
}

void hw_fence_cleanup_client(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client)
{
	/*
	 * Deallocate any resource allocated for this client.
	 *  If fence controller was notified about existence of this client,
	 *  we will need to notify fence controller that this client is gone
	 *
	 * NOTE: Since currently we are doing a 'fixed' memory for the clients queues,
	 *  we don't need any notification to the Fence Controller, yet..
	 *  however, if the memory allocation is removed from 'fixed' to a dynamic
	 *  allocation, then we will need to notify FenceCTL about the client that is
	 *  going-away here.
	 */
	mutex_lock(&drv_data->clients_register_lock);
	drv_data->clients[hw_fence_client->client_id] = NULL;
	mutex_unlock(&drv_data->clients_register_lock);

	/* Deallocate client's object */
	HWFNC_DBG_LUT("freeing client_id:%d\n", hw_fence_client->client_id);
	kfree(hw_fence_client);
}

static inline int _calculate_hash(u32 table_total_entries, u64 context, u64 seqno,
	u64 step, u64 *hash)
{
	u64 m_size = table_total_entries;
	int val = 0;

	if (step == 0) {
		u64 a_multiplier = HW_FENCE_HASH_A_MULT;
		u64 c_multiplier = HW_FENCE_HASH_C_MULT;
		u64 b_multiplier = context + (context - 1); /* odd multiplier */

		/*
		 * if m, is power of 2, we can optimize with right shift,
		 * for now we don't do it, to avoid assuming a power of two
		 */
		*hash = (a_multiplier * seqno * b_multiplier + (c_multiplier * context)) % m_size;
	} else {
		if (step >= m_size) {
			/*
			 * If we already traversed the whole table, return failure since this means
			 * there are not available spots, table is either full or full-enough
			 * that we couldn't find an available spot after traverse the whole table.
			 * Ideally table shouldn't be so full that we cannot find a value after some
			 * iterations, so this maximum step size could be optimized to fail earlier.
			 */
			HWFNC_ERR("Fence Table tranversed and no available space!\n");
			val = -EINVAL;
		} else {
			/*
			 * Linearly increment the hash value to find next element in the table
			 * note that this relies in the 'scrambled' data from the original hash
			 * Also, add a mod division to wrap-around in case that we reached the
			 * end of the table
			 */
			*hash = (*hash + 1) % m_size;
		}
	}

	return val;
}

static inline struct msm_hw_fence *_get_hw_fence(u32 table_total_entries,
	struct msm_hw_fence *hw_fences_tbl,
	u64 hash)
{
	if (hash >= table_total_entries) {
		HWFNC_ERR("hash:%llu out of max range:%u\n",
			hash, table_total_entries);
		return NULL;
	}

	return &hw_fences_tbl[hash];
}

static bool _is_hw_fence_free(struct msm_hw_fence *hw_fence, u64 context, u64 seqno)
{
	/* If valid is set, the hw fence is not free */
	return hw_fence->valid ? false : true;
}

static bool _hw_fence_match(struct msm_hw_fence *hw_fence, u64 context, u64 seqno)
{
	return ((hw_fence->ctx_id == context && hw_fence->seq_id == seqno) ? true : false);
}

/* clears everything but the 'valid' field */
static void _cleanup_hw_fence(struct msm_hw_fence *hw_fence)
{
	int i;

	hw_fence->error = 0;
	wmb(); /* update memory to avoid mem-abort */
	hw_fence->ctx_id = 0;
	hw_fence->seq_id = 0;
	hw_fence->wait_client_mask = 0;
	hw_fence->fence_allocator = 0;
	hw_fence->fence_signal_client = 0;

	hw_fence->flags = 0;

	hw_fence->fence_create_time = 0;
	hw_fence->fence_trigger_time = 0;
	hw_fence->fence_wait_time = 0;
	hw_fence->refcount = 0;
	hw_fence->parents_cnt = 0;
	hw_fence->pending_child_cnt = 0;
	hw_fence->h_synx = 0;

	for (i = 0; i < MSM_HW_FENCE_MAX_JOIN_PARENTS; i++)
		hw_fence->parent_list[i] = HW_FENCE_INVALID_PARENT_FENCE;

	memset(hw_fence->client_data, 0, sizeof(hw_fence->client_data));
}

/* This function must be called with the hw fence lock */
static int  _reserve_hw_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fence, u32 client_id,
	u64 context, u64 seqno, u32 hash, u32 pending_child_cnt)
{
	_cleanup_hw_fence(hw_fence);

	/* reserve this HW fence */
	hw_fence->valid = 1;

	hw_fence->ctx_id = context;
	hw_fence->seq_id = seqno;
	hw_fence->flags = 0; /* fence just reserved, there shouldn't be any flags set */
	hw_fence->fence_allocator = client_id;
	hw_fence->fence_create_time = hw_fence_get_qtime(drv_data);
	/* one released by creating client; one released by FCTL */
	hw_fence->refcount = HW_FENCE_FCTL_REFCOUNT + 1;

	HWFNC_DBG_LUT("Reserved fence client:%d ctx:%llu seq:%llu hash:%u\n",
		client_id, context, seqno, hash);

	return 0;
}

/* This function must be called with the hw fence lock */
static int  _unreserve_hw_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fence, u32 client_id,
	u64 context, u64 seqno, u32 hash, u32 pending_child_cnt)
{
	if (hw_fence->refcount & HW_FENCE_HLOS_REFCOUNT_MASK)
		hw_fence->refcount--;
	else
		return -EINVAL; /* keep hw-fence in table for debugging purposes */

	/* if both hlos and fctl refcounts are cleared, then delete the fence */
	if (!hw_fence->refcount) {
		_cleanup_hw_fence(hw_fence);

		/* unreserve this HW fence */
		hw_fence->valid = 0;
	}

	HWFNC_DBG_LUT("Unreserved fence client:%d ctx:%llu seq:%llu hash:%u refcount:%x\n",
		client_id, context, seqno, hash, hw_fence->refcount);

	return 0;
}

int hw_fence_destroy_refcount(struct hw_fence_driver_data *drv_data, u64 hash, u32 ref)
{
	struct msm_hw_fence *hw_fences_tbl = drv_data->hw_fences_tbl;
	struct msm_hw_fence *hw_fence = NULL;
	int ret = 0;

	hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, hw_fences_tbl, hash);
	if (!hw_fence) {
		HWFNC_ERR("bad hw fence hash:%llu\n", hash);
		return -EINVAL;
	}

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1); /* lock */
	if (hw_fence->refcount & ref) {
		hw_fence->refcount &= ~ref;
	} else {
		GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */
		HWFNC_ERR("fence ctx:%llu seq:%llu hash:%llu ref:0x%x before destroy ref:0x%x\n",
			hw_fence->ctx_id, hw_fence->seq_id, hash, hw_fence->refcount, ref);
		/* keep hw-fence in table for debugging purposes */
		return -EINVAL;
	}
	if (!hw_fence->refcount) {
		_cleanup_hw_fence(hw_fence);

		/* unreserve this HW fence */
		hw_fence->valid = 0;
	}

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */
	HWFNC_DBG_H("Removed 0x%x refcount on fence hash:%llu ref:0x%x\n", ref, hash,
		hw_fence->refcount);

	return ret;
}

/* This function must be called with the hw fence lock */
static int  _reserve_join_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fence, u32 client_id, u64 context,
	u64 seqno, u32 hash, u32 pending_child_cnt)
{
	_cleanup_hw_fence(hw_fence);

	/* reserve this HW fence */
	hw_fence->valid = true;

	hw_fence->ctx_id = context;
	hw_fence->seq_id = seqno;
	hw_fence->fence_allocator = client_id;
	hw_fence->fence_create_time = hw_fence_get_qtime(drv_data);
	/* one released by importing client; one released by FCTL */
	hw_fence->refcount = HW_FENCE_FCTL_REFCOUNT + 1;

	hw_fence->pending_child_cnt = pending_child_cnt;

	HWFNC_DBG_LUT("Reserved join fence client:%d ctx:%llu seq:%llu hash:%u\n",
		client_id, context, seqno, hash);

	return 0;
}

/* This function must be called with the hw fence lock */
static int  _fence_found(struct hw_fence_driver_data *drv_data,
	 struct msm_hw_fence *hw_fence, u32 client_id,
	u64 context, u64 seqno, u32 hash, u32 pending_child_cnt)
{
	if ((hw_fence->refcount & HW_FENCE_HLOS_REFCOUNT_MASK) == HW_FENCE_HLOS_REFCOUNT_MASK)
		return -EINVAL;

	/*
	 * Increment the hw-fence refcount. All other processing is done outside. After processing
	 * is done, the refcount needs to be decremented either explicitly by the client or as part
	 * of processing in HW Fence Driver.
	 */

	hw_fence->refcount++;
	HWFNC_DBG_LUT("Found fence client:%d ctx:%llu seq:%llu hash:%u ref:0x%x\n",
		client_id, context, seqno, hash, hw_fence->refcount);

	return 0;
}

char *_get_op_mode(enum hw_fence_lookup_ops op_code)
{
	switch (op_code) {
	case HW_FENCE_LOOKUP_OP_CREATE:
		return "CREATE";
	case HW_FENCE_LOOKUP_OP_DESTROY:
		return "DESTROY";
	case HW_FENCE_LOOKUP_OP_CREATE_JOIN:
		return "CREATE_JOIN";
	case HW_FENCE_LOOKUP_OP_FIND_FENCE:
		return "FIND_FENCE";
	default:
		return "UNKNOWN";
	}

	return "UNKNOWN";
}

struct msm_hw_fence *_hw_fence_lookup_and_process_range(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fences_tbl, u64 context, u64 seqno, u32 client_id,
	u32 pending_child_cnt, enum hw_fence_lookup_ops op_code, u64 *hash, u64 flags,
	u64 start_step, u64 end_step)
{
	bool (*compare_fnc)(struct msm_hw_fence *hfence, u64 context, u64 seqno);
	int (*process_fnc)(struct hw_fence_driver_data *drv_data, struct msm_hw_fence *hfence,
			u32 client_id, u64 context, u64 seqno, u32 hash, u32 pending);
	struct msm_hw_fence *hw_fence = NULL;
	u64 step = start_step;
	int ret = 0;
	bool hw_fence_found = false;

	if (!hash | !drv_data | !hw_fences_tbl) {
		HWFNC_ERR("Invalid input for hw_fence_lookup\n");
		return NULL;
	}

	/*
	 * When start_step != 0, the hash is already initialized at the correct value and should
	 * not be reset.
	 */
	if (!step)
		*hash = ~0;

	HWFNC_DBG_LUT("hw_fence_lookup: %d\n", op_code);

	switch (op_code) {
	case HW_FENCE_LOOKUP_OP_CREATE:
		compare_fnc = &_is_hw_fence_free;
		process_fnc = &_reserve_hw_fence;
		break;
	case HW_FENCE_LOOKUP_OP_DESTROY:
		compare_fnc = &_hw_fence_match;
		process_fnc = &_unreserve_hw_fence;
		break;
	case HW_FENCE_LOOKUP_OP_CREATE_JOIN:
		compare_fnc = &_is_hw_fence_free;
		process_fnc = &_reserve_join_fence;
		break;
	case HW_FENCE_LOOKUP_OP_FIND_FENCE:
		compare_fnc = &_hw_fence_match;
		process_fnc = &_fence_found;
		break;
	default:
		HWFNC_ERR("Unknown op code:%d\n", op_code);
		return NULL;
	}

	while (!hw_fence_found && (step < end_step)) {

		/* Calculate the Hash for the Fence */
		ret = _calculate_hash(drv_data->hw_fence_table_entries, context, seqno, step, hash);
		if (ret) {
			HWFNC_ERR("error calculating hash ctx:%llu seqno:%llu hash:%llu\n",
				context, seqno, *hash);
			break;
		}
		HWFNC_DBG_LUT("calculated hash:%llu [ctx:%llu seqno:%llu]\n", *hash, context,
			seqno);

		/* Get element from the table using the hash */
		hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, hw_fences_tbl, *hash);
		HWFNC_DBG_LUT("hw_fence_tbl:0x%pK hw_fence:0x%pK, hash:%llu valid:0x%x\n",
			hw_fences_tbl, hw_fence, *hash, hw_fence ? hw_fence->valid : 0xbad);
		if (!hw_fence) {
			HWFNC_ERR("bad hw fence ctx:%llu seqno:%llu hash:%llu\n",
				context, seqno, *hash);
			break;
		}

		GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1);

		/* compare to either find a free fence or find an allocated fence */
		if (compare_fnc(hw_fence, context, seqno)) {

			/* Process the hw fence found by the algorithm */
			if (process_fnc) {
				ret = process_fnc(drv_data, hw_fence, client_id, context, seqno,
					*hash, pending_child_cnt);

				/* update memory table with processing */
				wmb();
			}

			HWFNC_DBG_L("client_id:%u op:%s ctx:%llu seqno:%llu hash:%llu step:%llu\n",
				client_id, _get_op_mode(op_code), context, seqno, *hash, step);

			hw_fence_found = true;
		} else {
			if ((op_code == HW_FENCE_LOOKUP_OP_CREATE ||
				op_code == HW_FENCE_LOOKUP_OP_CREATE_JOIN) &&
				seqno == hw_fence->seq_id && context == hw_fence->ctx_id) {
				if (flags & MSM_HW_FENCE_FLAG_CREATE_SIGNALED) {
					/* hw-fence created for importing client */
					ret = _fence_found(drv_data, hw_fence, client_id, context,
						seqno, *hash, pending_child_cnt);
					hw_fence_found = true;
				} else {
					ret = -EALREADY;
				}
				GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0);
				if (ret == -EALREADY)
					HWFNC_ERR("can't create hfence w/ same ctx:%llu seq:%llu\n",
						context, seqno);

				break;
			}
			/* compare can fail if we have a collision, we will linearly resolve it */
			HWFNC_DBG_H("compare failed for hash:%llu [ctx:%llu seqno:%llu]\n", *hash,
				context, seqno);
		}

		GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0);

		if (hw_fence_found && ret)
			HWFNC_ERR("failed process_func client:%u op:%s ctx:%llu seq:%llu h:%llu\n",
				client_id, _get_op_mode(op_code), context, seqno, *hash);

		/* Increment step for the next loop */
		step++;
	}

	/* If we iterated through the whole list and didn't find the fence, return null */
	if (!hw_fence_found) {
		HWFNC_DBG_LUT("fail to process hw-fence op_code:%d step:%llu\n", op_code, step);
		hw_fence = NULL;
	}

	HWFNC_DBG_LUT("lookup:%d hw_fence:%pK ctx:%llu seqno:%llu hash:%llu flags:0x%llx\n",
		op_code, hw_fence, context, seqno, *hash, hw_fence ? hw_fence->flags : -1);

	return hw_fence;
}

struct msm_hw_fence *_hw_fence_lookup_and_process(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fences_tbl, u64 context, u64 seqno, u32 client_id,
	u32 pending_child_cnt, enum hw_fence_lookup_ops op_code, u64 *hash, u64 flags)
{
	return _hw_fence_lookup_and_process_range(drv_data, hw_fences_tbl, context, seqno,
		client_id, pending_child_cnt, op_code, hash, flags, 0,
		drv_data->hw_fence_table_entries);
}

struct dma_fence *hw_dma_fence_init(struct msm_hw_fence_client *hw_fence_client, u64 context,
	u64 seqno)
{
	struct hw_dma_fence *fence;
	spinlock_t *fence_lock;

	/* create dma fence */
	fence_lock = kzalloc(sizeof(*fence_lock), GFP_ATOMIC);
	if (!fence_lock)
		return ERR_PTR(-ENOMEM);

	fence = kzalloc(sizeof(*fence), GFP_ATOMIC);
	if (!fence) {
		kfree(fence_lock);
		return ERR_PTR(-ENOMEM);
	}

	snprintf(fence->name, HW_FENCE_NAME_SIZE, "hwfence:id:%d:ctx=%llu:seqno:%llu",
		hw_fence_client->client_id, context, seqno);
	spin_lock_init(fence_lock);

	HWFNC_DBG_L("creating dma_fence for client:%d ctx:%llu seqno:%llu\n",
		hw_fence_client->client_id, context, seqno);

	dma_fence_init(&fence->base, &hw_fence_dbg_ops, fence_lock, context, seqno);
	fence->client_handle = hw_fence_client;

	return (struct dma_fence *)fence;
}

int hw_fence_dma_fence_table_add(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct dma_fence *fence, u64 hw_fence_hash)
{
	struct hw_dma_fence *hw_dma_fence;
	u32 dma_fence_key = hw_fence_hash % DMA_FENCE_HASH_TABLE_SIZE;
	unsigned long flags;

	if (!fence || !drv_data || !hw_fence_client) {
		HWFNC_ERR("invalid params fence:0x%pK drv_data:0x%pK hw_fence_client:0x%pK\n",
			fence, drv_data, hw_fence_client);
		return -EINVAL;
	}

	hw_dma_fence = to_hw_dma_fence(fence);
	HWFNC_DBG_L("add hw_dma_fence:%pK client:%d ctx:%llu seqno:%llu key:%u hash:%llu\n",
		hw_dma_fence, hw_fence_client->client_id, fence->context, fence->seqno,
		dma_fence_key, hw_fence_hash);

	hw_dma_fence->dma_fence_key = dma_fence_key;
	hw_dma_fence->is_internal = true;
	hw_dma_fence->signal_cb.hash = hw_fence_hash;
	hw_dma_fence->signal_cb.drv_data = drv_data;

	spin_lock_irqsave(&drv_data->dma_fence_table_lock, flags);
	hash_add(drv_data->dma_fence_table, &hw_dma_fence->node, dma_fence_key);
	spin_unlock_irqrestore(&drv_data->dma_fence_table_lock, flags);

	return 0;
}

static void msm_hw_fence_internal_signal_callback(struct dma_fence *fence, struct dma_fence_cb *cb)
{
	struct hw_fence_signal_cb *signal_cb;

	if (!fence || !cb) {
		HWFNC_ERR("Invalid params fence:0x%pK cb:0x%pK\n", fence, cb);
		return;
	}

	HWFNC_DBG_IRQ("dma-fence signal callback ctx:%llu seqno:%llu flags:%lx err:%d\n",
		fence->context, fence->seqno, fence->flags, fence->error);

	signal_cb = (struct hw_fence_signal_cb *)cb;

	if (hw_fence_signal_fence(signal_cb->drv_data, fence, signal_cb->hash, fence->error, false))
		HWFNC_ERR("failed to signal fence ctx:%llu seq:%llu hash:%llu err:%u\n",
			fence->context, fence->seqno, signal_cb->hash, fence->error);
}


struct dma_fence *hw_fence_internal_dma_fence_create(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, u64 *hash)
{
	struct hw_dma_fence *hw_dma_fence;
	struct msm_hw_fence *hw_fence;
	struct dma_fence *fence;
	u64 context, seqno;
	int ret = 0;

	if (!drv_data || !hw_fence_client || !hash)
		return ERR_PTR(-EINVAL);

	context = hw_fence_client->context_id;
	seqno = atomic_add_return(1, &hw_fence_client->seqno);
	fence = hw_dma_fence_init(hw_fence_client, context, seqno);
	if (IS_ERR_OR_NULL(fence)) {
		HWFNC_ERR("failed to create internal dma-fence client:%d ctx:%llu seq:%llu\n",
			hw_fence_client->client_id, context, seqno);
		return ERR_PTR(-EINVAL);
	}

	ret = hw_fence_create(drv_data, hw_fence_client, context, seqno, hash);
	if (ret) {
		HWFNC_ERR("failed to back internal dma-fence client:%d ctx:%llu seq:%llu\n",
			hw_fence_client->client_id, context, seqno);
		ret = -EINVAL;
		goto error;
	}

	hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, drv_data->hw_fences_tbl, *hash);
	if (!hw_fence) {
		HWFNC_ERR("bad hw fence hash:%llu client:%u\n", *hash, hw_fence_client->client_id);
		ret = -EINVAL;
		goto error;
	}

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1); /* lock */
	hw_fence->flags |= MSM_HW_FENCE_FLAG_INTERNAL_OWNED;
	hw_fence->refcount |= HW_FENCE_DMA_FENCE_REFCOUNT;
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */

	/* If no error, set the HW Fence Flag in the dma-fence */
	set_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags);

	ret = hw_fence_dma_fence_table_add(drv_data, hw_fence_client, fence, *hash);
	if (ret) {
		HWFNC_ERR("failed to add hw-fence ctx:%llu seq:%llu hash:%llu to dma-fence table\n",
			context, seqno, *hash);
		ret = -EINVAL;
		goto error;
	}

	hw_dma_fence = to_hw_dma_fence(fence);
	/* internal_signal_callback does not take an additional hw-fence refcount */
	ret = dma_fence_add_callback(fence, &hw_dma_fence->signal_cb.fence_cb,
		msm_hw_fence_internal_signal_callback);
	if (ret)
		HWFNC_ERR("Failed to add signal callback ctx:%llu seq:%llu hash:%llu ret:%d\n",
			context, seqno, *hash, ret);

error:
	if (ret) {
		dma_fence_put(fence);
		return ERR_PTR(ret);
	}

	return fence;
}

struct dma_fence *hw_fence_dma_fence_find(struct hw_fence_driver_data *drv_data,
	u64 hw_fence_hash, bool incr_refcount)
{
	u32 dma_fence_key = hw_fence_hash % DMA_FENCE_HASH_TABLE_SIZE;
	struct hw_dma_fence *hw_dma_fence = NULL, *curr;
	struct dma_fence *fence = NULL;
	unsigned long flags;

	spin_lock_irqsave(&drv_data->dma_fence_table_lock, flags);
	hash_for_each_possible(drv_data->dma_fence_table, curr, node, dma_fence_key) {
		if (hw_fence_hash == curr->signal_cb.hash) {
			hw_dma_fence = curr;
			fence = &hw_dma_fence->base;
			if (incr_refcount)
				dma_fence_get(fence);
			break;
		}
	}
	spin_unlock_irqrestore(&drv_data->dma_fence_table_lock, flags);

	HWFNC_DBG_L("hw_dma_fence: %s:%pK ctx:%llu seqno:%llu key:%u dma_fence_ref:%u incr:%s\n",
		fence ? "found" : "not found", hw_dma_fence,
		fence ? fence->context : 0, fence ? fence->seqno : 0,
		dma_fence_key, fence ? kref_read(&fence->refcount) : -1,
		incr_refcount ? "true" : "false");

	return fence;
}

static int hw_fence_dma_fence_table_del(struct hw_fence_driver_data *drv_data, u64 hash,
	u64 flags, u32 error)
{
	struct hw_dma_fence *hw_dma_fence;
	struct dma_fence *fence;
	unsigned long lock_flags;
	int ret = 0;

	fence = hw_fence_dma_fence_find(drv_data, hash, false);
	if (IS_ERR_OR_NULL(fence))
		return PTR_ERR(fence);

	hw_dma_fence = to_hw_dma_fence(fence);

	HWFNC_DBG_L("removing dma_fence ctx:%llu seqno:%llu key:%u dma_fence_ref:%u\n",
		fence->context, fence->seqno, hw_dma_fence->dma_fence_key,
		kref_read(&fence->refcount));

	spin_lock_irqsave(&drv_data->dma_fence_table_lock, lock_flags);
	/* remove dma-fence from the internal hash table */
	if (hash_hashed(&hw_dma_fence->node))
		hash_del(&hw_dma_fence->node);
	else
		ret = -EINVAL;
	spin_unlock_irqrestore(&drv_data->dma_fence_table_lock, lock_flags);

	if (ret)
		HWFNC_ERR("internally owned dma-fence is not in table ctx:%llu seqno:%llu key:%u\n",
			fence->context, fence->seqno, hw_dma_fence->dma_fence_key);

	/* avoid signaling hw-fence when releasing hlos ref */
	dma_fence_remove_callback(fence, &hw_dma_fence->signal_cb.fence_cb);

	spin_lock_irqsave(fence->lock, lock_flags);
	if (!dma_fence_is_signaled(fence)) {
		if (!(flags & MSM_HW_FENCE_FLAG_SIGNAL))
			error = SYNX_STATE_SIGNALED_CANCEL;
		if (error)
			dma_fence_set_error(fence, -error);
		dma_fence_signal_locked(fence);
	}
	spin_unlock_irqrestore(fence->lock, lock_flags);
	dma_fence_put(fence);

	return ret;
}

int hw_fence_create(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client,
	u64 context, u64 seqno, u64 *hash)
{
	u32 client_id = hw_fence_client->client_id;
	struct msm_hw_fence *hw_fences_tbl = drv_data->hw_fences_tbl;
	int ret = 0;

	/* allocate hw fence in table */
	if (!_hw_fence_lookup_and_process(drv_data, hw_fences_tbl,
			context, seqno, client_id, 0, HW_FENCE_LOOKUP_OP_CREATE, hash, 0)) {
		HWFNC_ERR("Fail to create fence client:%u ctx:%llu seqno:%llu\n",
			client_id, context, seqno);
		ret = -EINVAL;
	}

	/**
	 * Note: This addresses any race conditions where clients may have been in progress
	 * creating hw-fences when soccp crashes
	 */
	if (!drv_data->fctl_ready) {
		HWFNC_ERR("unable to create hw-fence while fctl is not in valid state\n");
		hw_fence_destroy_refcount(drv_data, *hash, HW_FENCE_FCTL_REFCOUNT);
		hw_fence_destroy_with_hash(drv_data, hw_fence_client, *hash);
		return -EAGAIN;
	}

	if (hw_fence_client->skip_fctl_ref) {
		ret = hw_fence_destroy_refcount(drv_data, *hash, HW_FENCE_FCTL_REFCOUNT);
		if (ret)
			HWFNC_ERR("Can't remove fctl ref client:%u ctx:%llu seqno:%llu hash:%llu\n",
				client_id, context, seqno, *hash);
	}

	return ret;
}

int hw_fence_destroy(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client,
	u64 context, u64 seqno)
{
	u32 client_id = hw_fence_client->client_id;
	struct msm_hw_fence *hw_fences_tbl = drv_data->hw_fences_tbl;
	int ret = 0;
	u64 hash;

	/* decrement refcount on hw-fence */
	if (!_hw_fence_lookup_and_process(drv_data, hw_fences_tbl,
			context, seqno, client_id, 0, HW_FENCE_LOOKUP_OP_DESTROY, &hash, 0)) {
		HWFNC_ERR("Fail destroying fence client:%u ctx:%llu seqno:%llu\n",
			client_id, context, seqno);
		ret = -EINVAL;
	}

	return ret;
}

/*
 * This must be called while holding hw-fence lock; this releases hw-fence lock and (if needed)
 * associated dma-fence if necessary
 */
static int hw_fence_put_and_unlock(struct hw_fence_driver_data *drv_data, u32 client_id,
	struct msm_hw_fence *hw_fence, u64 hash)
{
	bool release_dma = false;
	int ret = 0;
	u64 flags;
	u32 error;

	if (hw_fence->refcount & HW_FENCE_HLOS_REFCOUNT_MASK) {
		hw_fence->refcount--;
	} else {
		ret = -EINVAL;
		goto end; /* keep hw-fence in table for debugging purposes */
	}

	if ((hw_fence->flags & MSM_HW_FENCE_FLAG_INTERNAL_OWNED) &&
			!(hw_fence->refcount & HW_FENCE_HLOS_REFCOUNT_MASK)) {
		hw_fence->flags &= ~MSM_HW_FENCE_FLAG_INTERNAL_OWNED;
		release_dma = true;
		flags = hw_fence->flags;
		error = hw_fence->error;
	}

	if (!hw_fence->refcount) {
		_cleanup_hw_fence(hw_fence);

		/* unreserve this HW fence */
		hw_fence->valid = 0;
	}

end:
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */

	if (ret) {
		HWFNC_ERR("fence client:%d ctx:%llu seq:%llu hash:%llu ref:0x%x before decr\n",
			client_id, hw_fence->ctx_id, hw_fence->seq_id, hash, hw_fence->refcount);
		return ret;
	}

	if (release_dma) {
		ret = hw_fence_dma_fence_table_del(drv_data, hash, flags, error);
		if (ret)
			HWFNC_ERR("Failed to delete internal dma-fence for hw-fence hash:%llu\n",
				hash);
	}

	return ret;
}

int hw_fence_destroy_with_hash(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, u64 hash)
{
	u32 client_id = hw_fence_client ? hw_fence_client->client_id : ~0;
	struct msm_hw_fence *hw_fences_tbl = drv_data->hw_fences_tbl;
	struct msm_hw_fence *hw_fence = NULL;

	hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, hw_fences_tbl, hash);
	if (!hw_fence) {
		HWFNC_ERR("bad hw fence hash:%llu client:%u\n", hash, client_id);
		return -EINVAL;
	}

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1); /* lock */
	return hw_fence_put_and_unlock(drv_data, client_id, hw_fence, hash);
}

static struct msm_hw_fence *_hw_fence_process_join_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client,
	struct dma_fence_array *array, u64 *hash, bool create)
{
	struct msm_hw_fence *hw_fences_tbl;
	struct msm_hw_fence *join_fence = NULL;
	u64 context, seqno;
	u32 client_id, pending_child_cnt;

	/*
	 * NOTE: For now we are allocating the join fences from the same table as all
	 * the other fences (i.e. drv_data->hw_fences_tbl), functionally this will work, however,
	 * this might impact the lookup algorithm, since the "join-fences" are created with the
	 * context and seqno of a fence-array, and those might not be changing by the client,
	 * so this will linearly increment the look-up and very likely impact the other fences if
	 * these join-fences start to fill-up a particular region of the fences global table.
	 * So we might have to allocate a different table altogether for these join fences.
	 * However, to do this, just alloc another table and change it here:
	 */
	hw_fences_tbl = drv_data->hw_fences_tbl;

	context = array->base.context;
	seqno = array->base.seqno;
	pending_child_cnt = array->num_fences;
	client_id = HW_FENCE_JOIN_FENCE_CLIENT_ID;

	if (create) {
		/* allocate the fence */
		join_fence = _hw_fence_lookup_and_process(drv_data, hw_fences_tbl, context,
			seqno, client_id, pending_child_cnt, HW_FENCE_LOOKUP_OP_CREATE_JOIN, hash,
			0);
		if (!join_fence)
			HWFNC_ERR("Fail to create join fence client:%u ctx:%llu seqno:%llu\n",
				client_id, context, seqno);
	} else if (hw_fence_destroy_refcount(drv_data, *hash, HW_FENCE_FCTL_REFCOUNT)) {
		HWFNC_ERR("Fail destroy join fence client:%u ctx:%llu seq:%llu hash:%llu\n",
			client_id, context, seqno, *hash);
	}

	return join_fence;
}

struct msm_hw_fence *msm_hw_fence_find(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client,
	u64 context, u64 seqno, u64 *hash)
{
	struct msm_hw_fence *hw_fences_tbl = drv_data->hw_fences_tbl;
	struct msm_hw_fence *hw_fence;
	u32 client_id = hw_fence_client ? hw_fence_client->client_id : ~0;

	/* find the hw fence */
	hw_fence = _hw_fence_lookup_and_process(drv_data, hw_fences_tbl, context,
		seqno, client_id, 0, HW_FENCE_LOOKUP_OP_FIND_FENCE, hash, 0);
	if (!hw_fence)
		HWFNC_ERR("Fail to find hw fence client:%u ctx:%llu seqno:%llu\n",
			client_id, context, seqno);

	return hw_fence;
}

static int _fence_ctl_signal(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct msm_hw_fence *hw_fence, u64 hash,
	u64 flags, u64 client_data, u32 error, bool signal_from_import)
{
	int ret = 0;
	u32 tx_client_id = drv_data->ipcc_client_pid; /* phys id for tx client */
	u32 rx_client_id = hw_fence_client->ipc_client_vid; /* virt id for rx client */

	HWFNC_DBG_H("We must signal the client now! hfence hash:%llu\n", hash);

	/* Call fence error callback */
	if (error && hw_fence_client->fence_error_cb) {
		ret = hw_fence_utils_fence_error_cb(hw_fence_client, hw_fence->ctx_id,
			hw_fence->seq_id, hash, flags, error);
	} else {
		/* Write to Rx queue */
		if (hw_fence_client->signaled_update_rxq ||
				(hw_fence_client->update_rxq && !signal_from_import)) {
			ret = hw_fence_update_queue(drv_data, hw_fence_client, hw_fence->ctx_id,
				hw_fence->seq_id, hash, flags, client_data, error,
				HW_FENCE_RX_QUEUE - 1);
			if (ret) {
				HWFNC_ERR("Can't update rxq clt:%d h:%llu ctx:%llu sq:%llu e:%d\n",
					hw_fence_client ? hw_fence_client->client_id : -1, hash,
					hw_fence->ctx_id, hw_fence->seq_id, error);
				return ret;
			}
		}

#if IS_ENABLED(CONFIG_DEBUG_FS)
		/* signal validation clients on targets with vm through custom mechanism */
		if (!drv_data->has_soccp && hw_fence_client->client_id >= HW_FENCE_CLIENT_ID_VAL0 &&
				hw_fence_client->client_id <= HW_FENCE_CLIENT_ID_VAL6) {
			ret = process_validation_client_loopback(drv_data,
				hw_fence_client->client_id);
			return ret;
		}
#endif /* CONFIG_DEBUG_FS */

		/* Signal the hw fence now */
		if (hw_fence_client->signaled_send_ipc || !signal_from_import)
			hw_fence_ipcc_trigger_signal(drv_data, tx_client_id, rx_client_id,
				hw_fence_client->ipc_signal_id);
	}

	return ret;
}

static void _cleanup_join_and_child_fences(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, int iteration, struct dma_fence_array *array,
	struct msm_hw_fence *join_fence, u64 hash_join_fence)
{
	struct dma_fence *child_fence;
	struct msm_hw_fence *hw_fence_child;
	bool child_is_signaled;
	int idx, j;
	u64 hash = 0;

	if (!array->fences)
		goto destroy_fence;

	/* cleanup the child-fences from the parent join-fence */
	for (idx = iteration; idx >= 0; idx--) {
		child_fence = array->fences[idx];
		if (!child_fence) {
			HWFNC_ERR("invalid child fence idx:%d\n", idx);
			continue;
		}

		hw_fence_child = hw_fence_find_with_dma_fence(drv_data, hw_fence_client,
			child_fence, &hash, &child_is_signaled, false);
		if (child_is_signaled) {
			continue;
		} else if (!hw_fence_child) {
			HWFNC_ERR("Cannot cleanup child fence context:%llu seqno:%llu hash:%llu\n",
				child_fence->context, child_fence->seqno, hash);

			/*
			 * ideally this should not have happened, but if it did, try to keep
			 * cleaning-up other fences after printing the error
			 */
			continue;
		}

		/* lock the child while we clean it up from the parent join-fence */
		GLOBAL_ATOMIC_STORE(drv_data, &hw_fence_child->lock, 1); /* lock */
		for (j = hw_fence_child->parents_cnt; j > 0; j--) {

			if (j > MSM_HW_FENCE_MAX_JOIN_PARENTS) {
				HWFNC_ERR("Invalid max parents_cnt:%d, will reset to max:%d\n",
					hw_fence_child->parents_cnt, MSM_HW_FENCE_MAX_JOIN_PARENTS);

				j = MSM_HW_FENCE_MAX_JOIN_PARENTS;
			}

			if (hw_fence_child->parent_list[j - 1] == hash_join_fence) {
				hw_fence_child->parent_list[j - 1] = HW_FENCE_INVALID_PARENT_FENCE;

				if (hw_fence_child->parents_cnt)
					hw_fence_child->parents_cnt--;

				/* update memory for the table update */
				wmb();
			}
		}
		/* decrement refcount acquired by finding fence */
		hw_fence_put_and_unlock(drv_data, hw_fence_client->client_id, hw_fence_child, hash);
	}

destroy_fence:
	/* destroy join fence */
	_hw_fence_process_join_fence(drv_data, hw_fence_client, array, &hash_join_fence,
		false);
}

/* update join fence for signaled child_fence and return if the join fence should be signaled */
bool _update_and_get_join_fence_signal_status(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *join_fence, u32 child_fence_error)
{
	bool signal_join_fence, error = false;

	/* child fence is already signaled */
	GLOBAL_ATOMIC_STORE(drv_data, &join_fence->lock, 1); /* lock */
	join_fence->error |= child_fence_error;
	if (join_fence->pending_child_cnt)
		join_fence->pending_child_cnt--;
	else
		error = true;
	signal_join_fence = !join_fence->pending_child_cnt;
	GLOBAL_ATOMIC_STORE(drv_data, &join_fence->lock, 0); /* unlock */

	/* update memory for the table update */
	wmb();

	if (error)
		HWFNC_ERR("join fence ctx:%llu seq:%llu pending_child_cnt==0 before decrement\n",
			join_fence->ctx_id, join_fence->seq_id);

	return signal_join_fence;
}

int hw_fence_process_fence_array(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct dma_fence_array *array,
	u64 *hash_join_fence, u64 client_data)
{
	struct msm_hw_fence *join_fence;
	struct msm_hw_fence *hw_fence_child;
	struct dma_fence *child_fence;
	bool child_is_signaled, signal_join_fence = false;
	u64 hash;
	int i, ret = 0;
	enum hw_fence_client_data_id data_id;

	if (client_data) {
		data_id = hw_fence_get_client_data_id(hw_fence_client->client_id_ext);
		if (data_id >= HW_FENCE_MAX_CLIENTS_WITH_DATA) {
			HWFNC_ERR("Populating client_data:%llu with invalid client_id_ext:%d\n",
				client_data, hw_fence_client->client_id_ext);
			return -EINVAL;
		}
	}

	/*
	 * Create join fence from the join-fences table,
	 * This function initializes:
	 * join_fence->pending_child_count = array->num_fences
	 */
	join_fence = _hw_fence_process_join_fence(drv_data, hw_fence_client, array,
		hash_join_fence, true);
	if (!join_fence) {
		HWFNC_ERR("cannot alloc hw fence for join fence array\n");
		return -EINVAL;
	}

	/* update this as waiting client of the join-fence */
	GLOBAL_ATOMIC_STORE(drv_data, &join_fence->lock, 1); /* lock */
	join_fence->wait_client_mask |= BIT(hw_fence_client->client_id);
	GLOBAL_ATOMIC_STORE(drv_data, &join_fence->lock, 0); /* unlock */

	/* Iterate through fences of the array */
	for (i = 0; i < array->num_fences; i++) {
		child_fence = array->fences[i];

		if (!child_fence) {
			HWFNC_ERR("NULL child fence at index:%d for fence array\n", i);
			ret = -EINVAL;
			goto error_array;
		}

		/* Nested fence-arrays are not supported */
		if (to_dma_fence_array(child_fence)) {
			HWFNC_ERR("This is a nested fence, fail!\n");
			ret = -EINVAL;
			goto error_array;
		}

		/* All elements in the fence-array must be hw-fences */
		if (!test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &child_fence->flags)) {
			HWFNC_ERR("DMA Fence in FenceArray is not a HW Fence\n");
			ret = -EINVAL;
			goto error_array;
		}

		/* Find the HW Fence in the Global Table */
		hw_fence_child = hw_fence_find_with_dma_fence(drv_data, hw_fence_client,
			child_fence, &hash, &child_is_signaled, false);
		if (child_is_signaled) {
			signal_join_fence = _update_and_get_join_fence_signal_status(drv_data,
				join_fence, child_fence->error);
			continue;
		} else if (!hw_fence_child) {
			HWFNC_ERR("Cannot find child fence context:%llu seqno:%llu hash:%llu\n",
				child_fence->context, child_fence->seqno, hash);
			ret = -EINVAL;
			goto error_array;
		}

		GLOBAL_ATOMIC_STORE(drv_data, &hw_fence_child->lock, 1); /* lock */
		if (hw_fence_child->flags & MSM_HW_FENCE_FLAG_SIGNAL) {

			/* child fence is already signaled */
			signal_join_fence = _update_and_get_join_fence_signal_status(drv_data,
				join_fence, hw_fence_child->error);
		} else {

			/* child fence is not signaled */
			hw_fence_child->parents_cnt++;

			if (hw_fence_child->parents_cnt >= MSM_HW_FENCE_MAX_JOIN_PARENTS
					|| hw_fence_child->parents_cnt < 1) {

				/* Max number of parents for a fence is exceeded */
				HWFNC_ERR("DMA Fence in FenceArray exceeds parents:%d\n",
					hw_fence_child->parents_cnt);
				hw_fence_child->parents_cnt--;

				/* decrement refcount acquired by finding fence */
				hw_fence_put_and_unlock(drv_data, hw_fence_client->client_id,
					hw_fence_child, hash);

				ret = -EINVAL;
				goto error_array;
			}

			hw_fence_child->parent_list[hw_fence_child->parents_cnt - 1] =
				*hash_join_fence;
		}
		/* decrement refcount acquired by finding fence */
		hw_fence_put_and_unlock(drv_data, hw_fence_client->client_id, hw_fence_child, hash);
	}

	if (client_data)
		join_fence->client_data[data_id] = client_data;

	/* all fences were signaled, signal client now */
	if (signal_join_fence) {

		/* signal the join hw fence */
		_fence_ctl_signal(drv_data, hw_fence_client, join_fence, *hash_join_fence, 0,
			client_data, join_fence->error, true);
		set_bit(MSM_HW_FENCE_FLAG_SIGNALED_BIT, &array->base.flags);

		/*
		 * job of the join-fence is finished since we already signaled,
		 * we can delete it now. This can happen when all the fences that
		 * are part of the join-fence are already signaled.
		 */
		_hw_fence_process_join_fence(drv_data, hw_fence_client, array, hash_join_fence,
			false);
	} else if (!array->num_fences) {
		/*
		 * if we didn't signal the join-fence and the number of fences is not set in
		 * the fence-array, then fail here, otherwise driver would create a join-fence
		 * with no-childs that won't be signaled at all or an incomplete join-fence
		 */
		HWFNC_ERR("invalid fence-array ctx:%llu seqno:%llu without fences\n",
			array->base.context, array->base.seqno);
		goto error_array;
	}

	return ret;

error_array:
	_cleanup_join_and_child_fences(drv_data, hw_fence_client, i, array, join_fence,
		*hash_join_fence);

	return -EINVAL;
}

/**
 * Registers the hw-fence client for wait on a hw-fence and keeps a reference on that hw-fence.
 * The hw-fence must be explicitly dereferenced following this function, e.g. by client
 * synx_release call.
 * This function does not register the fence_allocator as a waiting client.
 *
 * Note: This is the only place where the hw-fence refcount is retained for the client to release.
 * In all other places, the HW Fence Driver releases the refcount held for processing.
 */
int hw_fence_register_wait_client(struct hw_fence_driver_data *drv_data,
		struct dma_fence *fence, struct msm_hw_fence_client *hw_fence_client, u64 context,
		u64 seqno, u64 *hash, u64 client_data)
{
	struct msm_hw_fence *hw_fence;
	enum hw_fence_client_data_id data_id;
	bool is_signaled = false;
	int destroy_ret, ret = 0;

	if (client_data) {
		data_id = hw_fence_get_client_data_id(hw_fence_client->client_id_ext);
		if (data_id >= HW_FENCE_MAX_CLIENTS_WITH_DATA) {
			HWFNC_ERR("Populating client_data:%llu with invalid client_id_ext:%d\n",
				client_data, hw_fence_client->client_id);
			return -EINVAL;
		}
	}

	/* refcount from finding fence must be explicitly released outside this function call */
	if (fence)
		hw_fence = hw_fence_find_with_dma_fence(drv_data, hw_fence_client, fence, hash,
			&is_signaled, true);
	else
		hw_fence = msm_hw_fence_find(drv_data, hw_fence_client, context, seqno, hash);
	if (!hw_fence) {
		HWFNC_ERR("Cannot find fence!\n");
		return -EINVAL;
	}

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1); /* lock */

	/*
	 * If a creating client calls synx_import, then an additional hlos refcount is taken and a
	 * refcount is set for processing this fence in FenceCTL
	 */
	if (hw_fence->fence_allocator == hw_fence_client->client_id) {
		hw_fence->refcount |= HW_FENCE_FCTL_REFCOUNT;
	} else {
		/* register client in the hw fence */
		is_signaled = hw_fence->flags & MSM_HW_FENCE_FLAG_SIGNAL;
		hw_fence->wait_client_mask |= BIT(hw_fence_client->client_id);
		hw_fence->fence_wait_time = hw_fence_get_qtime(drv_data);
		if (client_data)
			hw_fence->client_data[data_id] = client_data;
	}

	/* update memory for the table update */
	wmb();

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */

	/* if hw fence already signaled, signal the client */
	if (is_signaled) {
		if (fence != NULL)
			set_bit(MSM_HW_FENCE_FLAG_SIGNALED_BIT, &fence->flags);
		ret = _fence_ctl_signal(drv_data, hw_fence_client, hw_fence, *hash, 0, client_data,
			hw_fence->error, true);
		if (ret) {
			HWFNC_ERR("failed to signal client:%d for import signaled fence h:%llu\n",
				hw_fence_client ? hw_fence_client->client_id : 0xff, *hash);
			destroy_ret = hw_fence_destroy_with_hash(drv_data, hw_fence_client, *hash);
			if (destroy_ret)
				HWFNC_ERR("failed destroy ref for failed import client:%d h:%llu\n",
					hw_fence_client ? hw_fence_client->client_id : 0xff, *hash);
		}
	}

	return ret;
}

int hw_fence_process_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client,
	struct dma_fence *fence, u64 *hash, u64 client_data)
{
	int ret = 0;

	if (!drv_data | !hw_fence_client | !fence) {
		HWFNC_ERR("Invalid Input!\n");
		return -EINVAL;
	}
	/* fence must be hw-fence */
	if (!test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags)) {
		HWFNC_ERR("DMA Fence in is not a HW Fence flags:0x%lx\n", fence->flags);
		return -EINVAL;
	}

	ret = hw_fence_register_wait_client(drv_data, fence, hw_fence_client, fence->context,
		fence->seqno, hash, client_data);
	if (ret)
		HWFNC_ERR("Error registering for wait client:%d\n", hw_fence_client->client_id);

	return ret;
}

static void _signal_all_wait_clients(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fence, u64 wait_client_mask, u64 hash, int error)
{
	enum hw_fence_client_id wait_client_id;
	enum hw_fence_client_data_id data_id;
	struct msm_hw_fence_client *hw_fence_wait_client;
	u64 client_data = 0;

	/* signal with an error all the waiting clients for this fence */
	for (wait_client_id = 0; wait_client_id <= drv_data->rxq_clients_num; wait_client_id++) {
		if (wait_client_mask & BIT(wait_client_id)) {
			hw_fence_wait_client = drv_data->clients[wait_client_id];

			if (!hw_fence_wait_client)
				continue;

			data_id = hw_fence_get_client_data_id(hw_fence_wait_client->client_id_ext);

			if (data_id < HW_FENCE_MAX_CLIENTS_WITH_DATA)
				client_data = hw_fence->client_data[data_id];

			_fence_ctl_signal(drv_data, hw_fence_wait_client, hw_fence,
				hash, 0, client_data, error, false);
		}
	}
}

/*
 * This function must be called with a signaled hw-fence; hw_fence->parents_cnt and
 * hw_fence->parent_list fields are only modified for unsignaled fences
 */
static void _signal_parent_fences(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fence, u32 parents_cnt, u64 hash, int error)
{
	struct msm_hw_fence *join_fence;
	u64 parent_hash;
	int i;

	if (parents_cnt > MSM_HW_FENCE_MAX_JOIN_PARENTS) {
		HWFNC_ERR("hw_fence hash:%llu has invalid parents_cnt:%u max:%u\n", hash,
			parents_cnt, MSM_HW_FENCE_MAX_JOIN_PARENTS);
		parents_cnt = MSM_HW_FENCE_MAX_JOIN_PARENTS;
	}

	for (i = 0; i < parents_cnt; i++) {
		parent_hash = hw_fence->parent_list[i];
		join_fence = _get_hw_fence(drv_data->hw_fence_table_entries,
			drv_data->hw_fences_tbl, parent_hash);
		if (!join_fence) {
			HWFNC_ERR("bad parent hash:%llu of child hash:%llu\n", parent_hash, hash);
			continue;
		}

		if (_update_and_get_join_fence_signal_status(drv_data, join_fence, error)) {
			/* no need to lock access to wait client mask for join fences */
			_signal_all_wait_clients(drv_data, join_fence, join_fence->wait_client_mask,
				parent_hash, join_fence->error);

			/* decrement refcount for signal on behalf of fence controller */
			hw_fence_destroy_refcount(drv_data, parent_hash, HW_FENCE_FCTL_REFCOUNT);
		}
	}
}

/*
 * Check fence signaling status. If unsignaled,
 * 1. signal waiting clients,
 * 2. signal parent fences (and waiting clients on parent fences)
 * 3. decrement refcount for signal on behalf of fence controller (if release_ref is true)
 */
static void _signal_fence_if_unsignaled(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fence, u64 hash, int error, bool release_ref)
{
	u64 wait_client_mask;
	u32 parents_cnt;

	/* check flags and error for signaling */
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1); /* lock */
	if (hw_fence->flags & MSM_HW_FENCE_FLAG_SIGNAL) {
		/* fence is already signaled so do nothing */
		GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0);
		return;
	}
	hw_fence->flags |= MSM_HW_FENCE_FLAG_SIGNAL;
	hw_fence->error = error;
	wait_client_mask = hw_fence->wait_client_mask;
	parents_cnt = hw_fence->parents_cnt;
	hw_fence->parents_cnt = 0;
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */

	/* fields used by the following are not modified for signaled fences */
	_signal_parent_fences(drv_data, hw_fence, parents_cnt, hash, error);
	_signal_all_wait_clients(drv_data, hw_fence, wait_client_mask, hash, error);

	/* remove ref held by fence controller to signal hw-fence */
	if (release_ref)
		hw_fence_destroy_refcount(drv_data, hash, HW_FENCE_FCTL_REFCOUNT);
}

struct msm_hw_fence *_create_signaled_hw_fence(struct hw_fence_driver_data *drv_data,
	u32 client_id, struct dma_fence *fence, u64 *hash)
{
	struct msm_hw_fence *hw_fences_tbl = drv_data->hw_fences_tbl;
	struct msm_hw_fence *hw_fence;

	/* create new hw-fence for signaled dma-fence */
	hw_fence = _hw_fence_lookup_and_process(drv_data, hw_fences_tbl,
		fence->context, fence->seqno, client_id, 0, HW_FENCE_LOOKUP_OP_CREATE, hash,
		MSM_HW_FENCE_FLAG_CREATE_SIGNALED);
	if (hw_fence) {
		_signal_fence_if_unsignaled(drv_data, hw_fence, *hash, fence->error, true);
		HWFNC_DBG_H("created hw-fence to back signaled fence client:%u ctx:%llu seq:%llu\n",
			client_id, fence->context, fence->seqno);
	} else {
		HWFNC_ERR("Fail to create signaled hfence client:%u ctx:%llu seq:%llu\n", client_id,
			fence->context, fence->seqno);
	}

	return hw_fence;
}

/* finds hw-fence in HW Fence table if present; if not and create==true, create a new hw-fence */
struct msm_hw_fence *hw_fence_find_with_dma_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct dma_fence *fence, u64 *hash,
	bool *is_signaled, bool create)
{
	u32 step, end_step, client_id = hw_fence_client ? hw_fence_client->client_id : 0xff;
	struct msm_hw_fence *hw_fences_tbl = drv_data->hw_fences_tbl;
	struct msm_hw_fence *hw_fence = NULL;

	if (!create && dma_fence_is_signaled(fence)) {
		/* signaled dma-fence may have been removed from table */
		*is_signaled = true;
		return NULL;
	}

	for (step = 0; step < drv_data->hw_fence_table_entries; step += HW_FENCE_FIND_THRESHOLD) {
		end_step = (step + HW_FENCE_FIND_THRESHOLD > drv_data->hw_fence_table_entries) ?
			drv_data->hw_fence_table_entries : step + HW_FENCE_FIND_THRESHOLD;
		hw_fence = _hw_fence_lookup_and_process_range(drv_data, hw_fences_tbl,
			fence->context, fence->seqno, client_id, 0, HW_FENCE_LOOKUP_OP_FIND_FENCE,
			hash, 0, step, end_step);
		if (hw_fence) {
			/* successfully found backing hw-fence*/
			*is_signaled = false;
			return hw_fence;
		}
		if (dma_fence_is_signaled(fence)) {
			/* signaled dma-fence may have been removed from table */
			*is_signaled = true;
			return create ? _create_signaled_hw_fence(drv_data, client_id, fence, hash)
				: NULL;
		}
	}

	/*
	 * The dma-fence signal callback holds a hw-fence refcount until dma-fence signal. If we hit
	 * this condition (unable to find unsignaled dma-fence with HW Fencing enabled), then the
	 * hw-fence has been incorrectly released early by someone who did not own the reference.
	 */
	HWFNC_ERR("Can't find backing hwfence for dma-fence client:%u ctx:%llu seq:%llu f:0x%lx\n",
		client_id, fence->context, fence->seqno, fence->flags);
	*is_signaled = false;
	return NULL;
}

void hw_fence_utils_reset_queues(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client)
{
	struct msm_hw_fence_queue *queue;
	u32 rd_idx, wr_idx, lock_idx;
	u32 *rd_idx_ptr, *wr_idx_ptr, *tx_wm_ptr;

	queue = &hw_fence_client->queues[HW_FENCE_TX_QUEUE - 1];
	hw_fence_get_queue_idx_ptrs(drv_data, queue->va_header, &rd_idx_ptr, &wr_idx_ptr,
		&tx_wm_ptr);

	/* For the client TxQ: set the read-index same as last write that was done by the client */
	mb(); /* make sure data is ready before read */
	wr_idx = readl_relaxed(wr_idx_ptr);
	if (queue->skip_wr_idx)
		*tx_wm_ptr = wr_idx;
	writel_relaxed(wr_idx, rd_idx_ptr);
	wmb(); /* make sure data is updated after write the index*/
	HWFNC_DBG_Q("update tx queue %s to match write_index:%u\n",
		queue->skip_wr_idx ? "read_index=tx_wm" : "read_index", wr_idx);

	/* For the client RxQ: set the write-index same as last read done by the client */
	if (hw_fence_client->update_rxq) {
		lock_idx = (hw_fence_client->client_id - 1) * HW_FENCE_LOCK_IDX_OFFSET;

		if (lock_idx >= drv_data->client_lock_tbl_cnt) {
			HWFNC_ERR("can't reset rxq, lock for client:%d lock_idx:%d exceed max:%d\n",
				hw_fence_client->client_id, lock_idx,
				drv_data->client_lock_tbl_cnt);
			return;
		}
		HWFNC_DBG_Q("Locking client id:%d: idx:%d\n", hw_fence_client->client_id, lock_idx);

		/* lock the client rx queue to update */
		GLOBAL_ATOMIC_STORE(drv_data, &drv_data->client_lock_tbl[lock_idx], 1);

		queue = &hw_fence_client->queues[HW_FENCE_RX_QUEUE - 1];
		hw_fence_get_queue_idx_ptrs(drv_data, queue->va_header, &rd_idx_ptr, &wr_idx_ptr,
			&tx_wm_ptr);

		mb(); /* make sure data is ready before read */
		rd_idx = readl_relaxed(rd_idx_ptr);
		writel_relaxed(rd_idx, wr_idx_ptr);
		wmb(); /* make sure data is updated after write the index */

		/* unlock */
		GLOBAL_ATOMIC_STORE(drv_data, &drv_data->client_lock_tbl[lock_idx], 0);
		HWFNC_DBG_Q("update rx queue write_index to match read_index:%u\n", rd_idx);
	}
}

int hw_fence_utils_cleanup_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_client *hw_fence_client, struct msm_hw_fence *hw_fence, u64 hash,
	u32 reset_flags)
{
	int ret = 0;
	int error = (reset_flags & MSM_HW_FENCE_RESET_WITHOUT_ERROR) ? 0 : MSM_HW_FENCE_ERROR_RESET;

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1); /* lock */
	if (hw_fence->wait_client_mask & BIT(hw_fence_client->client_id)) {
		HWFNC_DBG_H("clearing client:%d wait bit for fence: ctx:%llu seqno:%llu\n",
			hw_fence_client->client_id, hw_fence->ctx_id,
			hw_fence->seq_id);
		hw_fence->wait_client_mask &= ~BIT(hw_fence_client->client_id);

		/* remove reference held by waiting client */
		if (!(reset_flags & MSM_HW_FENCE_RESET_WITHOUT_DESTROY)) {
			hw_fence_put_and_unlock(drv_data, hw_fence_client->client_id, hw_fence,
				hash);
			return 0;
		}
	}
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */

	if (hw_fence->fence_allocator == hw_fence_client->client_id) {

		/* if fence is not signaled, signal with error all the waiting clients */
		_signal_fence_if_unsignaled(drv_data, hw_fence, hash, error, true);

		if (reset_flags & MSM_HW_FENCE_RESET_WITHOUT_DESTROY)
			goto skip_destroy;

		ret = hw_fence_destroy_with_hash(drv_data, hw_fence_client, hash);
		if (ret) {
			HWFNC_ERR("Error destroying HW fence: hash:%llu\n", hash);
		}
	}

skip_destroy:
	return ret;
}

enum hw_fence_client_data_id hw_fence_get_client_data_id(enum hw_fence_client_id client_id)
{
	enum hw_fence_client_data_id data_id;

	switch (client_id) {
	case HW_FENCE_CLIENT_ID_CTX0:
		data_id = HW_FENCE_CLIENT_DATA_ID_CTX0;
		break;
	default:
		data_id = HW_FENCE_MAX_CLIENTS_WITH_DATA;
		break;
	}

	return data_id;
}

int hw_fence_signal_fence(struct hw_fence_driver_data *drv_data, struct dma_fence *fence, u64 hash,
	u32 error, bool release_ref)
{
	struct msm_hw_fence *hw_fence;

	if (!drv_data) {
		HWFNC_ERR("bad drv_data\n");
		return -EINVAL;
	}

	hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, drv_data->hw_fences_tbl, hash);
	if (!hw_fence) {
		HWFNC_ERR("bad hw fence hash:%llu\n", hash);
		return -EINVAL;
	}

	if (fence && (hw_fence->ctx_id != fence->context || hw_fence->seq_id != fence->seqno)) {
		HWFNC_ERR("invalid hfence hash:%llu ctx:%llu seq:%llu expected ctx:%llu seq:%llu\n",
			hash, hw_fence->ctx_id, hw_fence->seq_id, fence->context, fence->seqno);
		return -EINVAL;
	}

	/* if unsignaled, signal but do not release ref held by FCTL */
	_signal_fence_if_unsignaled(drv_data, hw_fence, hash, error, release_ref);

	return 0;
}

static void msm_hw_fence_signal_callback(struct dma_fence *fence, struct dma_fence_cb *cb)
{
	struct hw_fence_signal_cb *signal_cb;
	int ret = 0;

	if (!fence || !cb) {
		HWFNC_ERR("Invalid params fence:0x%pK cb:0x%pK\n", fence, cb);
		return;
	}

	HWFNC_DBG_IRQ("dma-fence signal callback ctx:%llu seqno:%llu flags:%lx err:%d\n",
		fence->context, fence->seqno, fence->flags, fence->error);

	signal_cb = (struct hw_fence_signal_cb *)cb;
	ret = hw_fence_signal_fence(signal_cb->drv_data, fence, signal_cb->hash, fence->error,
		false);
	if (ret)
		HWFNC_ERR("failed to signal fence ctx:%llu seq:%llu hash:%llu err:%u\n",
			fence->context, fence->seqno, signal_cb->hash, fence->error);
	else
		/* release ref held by dma-fence signal */
		hw_fence_destroy_refcount(signal_cb->drv_data, signal_cb->hash,
			HW_FENCE_DMA_FENCE_REFCOUNT);

	kfree(signal_cb);
}

int hw_fence_add_callback(struct hw_fence_driver_data *drv_data, struct dma_fence *fence, u64 hash)
{
	struct hw_fence_signal_cb *signal_cb;
	struct msm_hw_fence *hw_fence;
	int ret;

	hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, drv_data->hw_fences_tbl, hash);
	if (!hw_fence) {
		HWFNC_ERR("Failed to find hw-fence for hash:%llu\n", hash);
		return -EINVAL;
	}

	signal_cb = kzalloc(sizeof(*signal_cb), GFP_ATOMIC);
	if (!signal_cb)
		return -ENOMEM;

	signal_cb->drv_data = drv_data;
	signal_cb->hash = hash;

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1);
	hw_fence->refcount |= HW_FENCE_DMA_FENCE_REFCOUNT;
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0);

	ret = dma_fence_add_callback(fence, &signal_cb->fence_cb, msm_hw_fence_signal_callback);
	if (ret) {
		if (dma_fence_is_signaled(fence)) {
			HWFNC_DBG_IRQ("dma_fence is signaled ctx:%llu seq:%llu flags:%lx err:%d\n",
				fence->context, fence->seqno, fence->flags, fence->error);
			msm_hw_fence_signal_callback(fence, &signal_cb->fence_cb);
			ret = 0;
		} else {
			HWFNC_ERR("failed to add signal_cb ctx:%llu seq:%llu f:%lx err:%d ret:%d\n",
				fence->context, fence->seqno, fence->flags, fence->error, ret);
			/* release ref held by dma-fence signal */
			hw_fence_destroy_refcount(signal_cb->drv_data, signal_cb->hash,
				HW_FENCE_DMA_FENCE_REFCOUNT);
			kfree(signal_cb);
		}
	}

	return ret;
}

int hw_fence_get_flags_error(struct hw_fence_driver_data *drv_data, u64 hash, u64 *flags,
	u32 *error)
{
	struct msm_hw_fence *hw_fence;

	if (!drv_data) {
		HWFNC_ERR("invalid drv_data\n");
		return -EINVAL;
	}

	hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, drv_data->hw_fences_tbl, hash);
	if (!hw_fence) {
		HWFNC_ERR("Failed to get hw-fence for hash:%llu\n", hash);
		return -EINVAL;
	}
	*flags = hw_fence->flags;
	*error = hw_fence->error;

	return 0;
}

int hw_fence_update_hsynx(struct hw_fence_driver_data *drv_data, u64 hash, u32 h_synx,
	bool wait_for)
{
	struct msm_hw_fence *hw_fence;
	int ret = 0;

	hw_fence = _get_hw_fence(drv_data->hw_fence_table_entries, drv_data->hw_fences_tbl, hash);
	if (!hw_fence) {
		HWFNC_ERR("Failed to get hw-fence for hash:%llu\n", hash);
		return -EINVAL;
	}

	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 1); /* lock */
	if (hw_fence->h_synx && hw_fence->h_synx != h_synx) {
		ret = -EINVAL;
		goto error;
	}
	hw_fence->h_synx = h_synx;
	if (wait_for)
		hw_fence->fence_wait_time = hw_fence_get_qtime(drv_data);
error:
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0); /* unlock */

	wmb(); /* update table */

	if (ret)
		HWFNC_ERR("setting h_synx:%u for hw-fence hash:%llu with existing h_synx:%u\n",
			h_synx, hash, hw_fence->h_synx);

	return ret;
}

int hw_fence_check_hw_fence_driver(struct hw_fence_driver_data *drv_data)
{
	if (IS_ERR_OR_NULL(drv_data) || !drv_data->resources_ready) {
		HWFNC_ERR("hw fence driver not ready\n");
		return -EINVAL;
	}

	return 0;
}

int hw_fence_check_valid_client(struct hw_fence_driver_data *drv_data, void *client_handle)
{
	int ret;

	ret = hw_fence_check_hw_fence_driver(drv_data);
	if (ret)
		return ret;

	if (IS_ERR_OR_NULL(client_handle)) {
		HWFNC_ERR("Invalid client\n");
		return -EINVAL;
	}

	return 0;
}

int hw_fence_check_valid_fctl(struct hw_fence_driver_data *drv_data, void *client_handle)
{
	int ret;

	ret = hw_fence_check_valid_client(drv_data, client_handle);
	if (ret)
		return ret;

	if (!drv_data->fctl_ready) {
		HWFNC_ERR("fctl in invalid state, cannot perform operation\n");
		return -EAGAIN;
	}

	return 0;
}

/* unlock the in-flight hw-fence and any locks taken on client rx queue for handling */
static void unlock_in_flight_fence(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fence, u64 hash, u64 in_flight_lock)
{
	u64 wait_client_mask;
	u32 wait_client_id, lock_idx;

	HWFNC_DBG_SSR("unlock in-flight fence locked as 0x%llx\n", hw_fence->lock);
	hw_fence_debug_dump_fence(HW_FENCE_SSR, hw_fence, hash, 0);
	wait_client_mask = hw_fence->wait_client_mask;
	GLOBAL_ATOMIC_STORE(drv_data, &hw_fence->lock, 0);

	for (wait_client_id = 0; wait_client_id <= drv_data->rxq_clients_num; wait_client_id++) {
		if (wait_client_mask & BIT(wait_client_id)) {
			lock_idx = (wait_client_id - 1) * HW_FENCE_LOCK_IDX_OFFSET;
			if (drv_data->client_lock_tbl[lock_idx] == in_flight_lock) {
				GLOBAL_ATOMIC_STORE(drv_data,
					&drv_data->client_lock_tbl[lock_idx], 0);
				HWFNC_DBG_SSR("unlock client rxq id:%d locked as 0x%llx\n",
					wait_client_id, in_flight_lock);
			}
		}
	}
}

int hw_fence_ssr_cleanup_table(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence *hw_fences_tbl, u32 table_total_entries, u64 in_flight_lock)
{
	struct msm_hw_fence *hw_fence;
	int i;

	if (!drv_data || !hw_fences_tbl || !in_flight_lock || in_flight_lock == BIT(0)) {
		HWFNC_ERR("invalid params drv_data:0x%pK table:0x%pK in_flight_lock:0x%llx",
			drv_data, hw_fences_tbl, in_flight_lock);
		return -EINVAL;
	}

	for (i = 0; i < table_total_entries; i++) {
		hw_fence = _get_hw_fence(table_total_entries, hw_fences_tbl, i);

		if (hw_fence->lock == in_flight_lock) {
			/* only one fence should be affected by this */
			unlock_in_flight_fence(drv_data, hw_fence, i, in_flight_lock);
		}
		_signal_fence_if_unsignaled(drv_data, hw_fence, i, MSM_HW_FENCE_ERROR_RESET, false);
	}

	return 0;
}
