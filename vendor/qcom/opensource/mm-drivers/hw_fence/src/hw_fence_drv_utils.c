// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/gunyah/gh_rm_drv.h>
#include <linux/gunyah/gh_dbl.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <linux/firmware/qcom/qcom_scm.h>
#else
#include <linux/qcom_scm.h>
#endif
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
#include <linux/gh_cpusys_vm_mem_access.h>
#endif
#include <soc/qcom/secure_buffer.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#if (KERNEL_VERSION(6, 1, 25) <= LINUX_VERSION_CODE)
#include <linux/remoteproc/qcom_rproc.h>
#endif
#include <linux/remoteproc.h>
#include <linux/platform_device.h>

#include "hw_fence_drv_priv.h"
#include "hw_fence_drv_utils.h"
#include "hw_fence_drv_ipc.h"
#include "hw_fence_drv_debug.h"

/**
 * MAX_CLIENT_QUEUE_MEM_SIZE:
 * Maximum memory size for client queues of a hw fence client.
 */
#define MAX_CLIENT_QUEUE_MEM_SIZE 0x100000

/**
 * HW_FENCE_MAX_CLIENT_TYPE:
 * Total number of client types with and without configurable number of sub-clients
 */
#define HW_FENCE_MAX_CLIENT_TYPE (HW_FENCE_MAX_CLIENT_TYPE_STATIC + \
	HW_FENCE_MAX_CLIENT_TYPE_CONFIGURABLE)

/**
 * HW_FENCE_MIN_RXQ_CLIENTS:
 * Minimum number of static hw fence clients with rxq
 */
#define HW_FENCE_MIN_RXQ_CLIENTS HW_FENCE_CLIENT_ID_VAL6

/**
 * HW_FENCE_MIN_RXQ_CLIENT_TYPE:
 * Minimum number of static hw fence client types with rxq (GFX, DPU, VAL)
 */
#define HW_FENCE_MIN_RXQ_CLIENT_TYPE 3

/* Maximum number of clients for each client type */
#define HW_FENCE_CLIENT_TYPE_MAX_GPU 1
#define HW_FENCE_CLIENT_TYPE_MAX_DPU 6
#define HW_FENCE_CLIENT_TYPE_MAX_VAL 7
#define HW_FENCE_CLIENT_TYPE_MAX_IPE 32
#define HW_FENCE_CLIENT_TYPE_MAX_VPU 32
#define HW_FENCE_CLIENT_TYPE_MAX_IFE 32
#define HW_FENCE_CLIENT_TYPE_MAX_IPA 32

/**
 * HW_FENCE_CLIENT_ID_CTRL_QUEUE:
 * Bit set in signaled clients mask if hw fence driver should read ctrl rx queue
 */
#define HW_FENCE_CLIENT_ID_CTRL_QUEUE 0

/**
 * HW_FENCE_SIGNALED_CLIENTS_LAST:
 * Last signaled clients id for which HW Fence Driver can receive doorbell
 */
#if IS_ENABLED(CONFIG_DEBUG_FS)
#define HW_FENCE_SIGNALED_CLIENTS_LAST HW_FENCE_CLIENT_ID_VAL6
#else
#define HW_FENCE_SIGNALED_CLIENTS_LAST HW_FENCE_CLIENT_ID_CTRL_QUEUE
#endif /* CONFIG_DEBUG_FS */

/**
 * HW_FENCE_ALL_SIGNALED_CLIENTS_MASK:
 * Each bit in this mask represents possible signaled client ids for which hw fence driver can
 * receive
 */
#define HW_FENCE_ALL_SIGNALED_CLIENTS_MASK \
	GENMASK(HW_FENCE_SIGNALED_CLIENTS_LAST, HW_FENCE_CLIENT_ID_CTRL_QUEUE)

/**
 * HW_FENCE_MAX_ITER_READ:
 * Maximum number of iterations when reading queue
 */
#define HW_FENCE_MAX_ITER_READ 100

/**
 * HW_FENCE_SOCCP_INIT_TIMEOUT_MS:
 * Timeout in ms for hw-fence driver delay of ssr callback while waiting for soccp response message
 */
#define HW_FENCE_SOCCP_INIT_TIMEOUT_MS 50

/**
 * HW_FENCE_FCTL_LOCK_VALUE:
 * Fence controller sets the hw-fence lock value to this when locking a given fence.
 */
#define HW_FENCE_FCTL_LOCK_VALUE BIT(1)

/**
 * HW_FENCE_MAX_EVENTS:
 * Maximum number of HW Fence debug events
 */
#define HW_FENCE_MAX_EVENTS 1000

/**
 * DT_PROPS_CLIENT_NAME_SIZE:
 * Maximum number of characters in client name used in device-tree properties
 */
#define DT_PROPS_CLIENT_NAME_SIZE 10

/**
 * DT_PROPS_CLIENT_PROPS_SIZE:
 * Maximum number of characters in property name for base client queue properties.
 */
#define DT_PROPS_CLIENT_PROPS_SIZE (DT_PROPS_CLIENT_NAME_SIZE + 27)

/**
 * DT_PROPS_CLIENT_EXTRA_PROPS_SIZE:
 * Maximum number of characters in property name for extra client queue properties.
 */
#define DT_PROPS_CLIENT_EXTRA_PROPS_SIZE (DT_PROPS_CLIENT_NAME_SIZE + 33)

/**
 * struct hw_fence_client_types - Table describing all supported client types, used to parse
 *                                device-tree properties related to client queue size.
 *
 * The fields name, init_id, and max_clients_num are constants. Default values for clients_num,
 * queues_num, and skip_txq_wr_idx are provided in this table, and clients_num, queues_num,
 * queue_entries, and skip_txq_wr_idx can be read from device-tree.
 *
 * If a value for queue entries is not parsed for the client type, then the default number of client
 * queue entries (parsed from device-tree) is used.
 *
 * Notes:
 * 1. Client types must be in the same order as client_ids within the enum 'hw_fence_client_id'.
 * 2. Each HW Fence client ID must be described by one of the client types in this table.
 * 3. A new client type must set: name, init_id, max_clients_num, clients_num, queues_num, and
 *    skip_txq_wr_idx.
 * 4. Either constant HW_FENCE_MAX_CLIENT_TYPE_CONFIGURABLE or HW_FENCE_MAX_CLIENT_TYPE_STATIC must
 *    be incremented as appropriate for new client types.
 */
struct hw_fence_client_type_desc hw_fence_client_types[HW_FENCE_MAX_CLIENT_TYPE] = {
	{"gpu", HW_FENCE_CLIENT_ID_CTX0, HW_FENCE_CLIENT_TYPE_MAX_GPU, HW_FENCE_CLIENT_TYPE_MAX_GPU,
		HW_FENCE_CLIENT_QUEUES, 0, 0, 0, 0, 0, 0, false, false},
	{"dpu", HW_FENCE_CLIENT_ID_CTL0, HW_FENCE_CLIENT_TYPE_MAX_DPU, HW_FENCE_CLIENT_TYPE_MAX_DPU,
		HW_FENCE_CLIENT_QUEUES, 0, 0, 0, 0, 0, 0, false, false},
	{"val", HW_FENCE_CLIENT_ID_VAL0, HW_FENCE_CLIENT_TYPE_MAX_VAL, HW_FENCE_CLIENT_TYPE_MAX_VAL,
		HW_FENCE_CLIENT_QUEUES, 0, 0, 0, 0, 0, 0, false, false},
	{"ipe", HW_FENCE_CLIENT_ID_IPE, HW_FENCE_CLIENT_TYPE_MAX_IPE, 0, HW_FENCE_CLIENT_QUEUES,
		0, 0, 0, 0, 0, 0, false, false},
	{"vpu", HW_FENCE_CLIENT_ID_VPU, HW_FENCE_CLIENT_TYPE_MAX_VPU, 0, HW_FENCE_CLIENT_QUEUES,
		0, 0, 0, 0, 0, 0, false, false},
	{"ipa", HW_FENCE_CLIENT_ID_IPA, HW_FENCE_CLIENT_TYPE_MAX_IPA, 0, 1, 0, 0, 0, 0, 0, 0,
		false, false},
	{"ife0", HW_FENCE_CLIENT_ID_IFE0, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife1", HW_FENCE_CLIENT_ID_IFE1, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife2", HW_FENCE_CLIENT_ID_IFE2, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife3", HW_FENCE_CLIENT_ID_IFE3, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife4", HW_FENCE_CLIENT_ID_IFE4, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife5", HW_FENCE_CLIENT_ID_IFE5, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife6", HW_FENCE_CLIENT_ID_IFE6, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife7", HW_FENCE_CLIENT_ID_IFE7, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife8", HW_FENCE_CLIENT_ID_IFE8, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife9", HW_FENCE_CLIENT_ID_IFE9, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife10", HW_FENCE_CLIENT_ID_IFE10, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
	{"ife11", HW_FENCE_CLIENT_ID_IFE11, HW_FENCE_CLIENT_TYPE_MAX_IFE, 0, 1, 0, 0, 0, 0, 0, 0,
		true, false},
};

#define hw_fence_wait_event_timeout(waitq, cond, timeout_ms, ret)	\
	do {								\
		ktime_t cur_ktime;					\
		ktime_t exp_ktime;					\
		s64 wait_time_jiffies = msecs_to_jiffies(timeout_ms);	\
\
		exp_ktime = ktime_add_ms(ktime_get(), timeout_ms);	\
		do {							\
			ret = wait_event_timeout(waitq, cond,		\
					wait_time_jiffies);		\
			cur_ktime = ktime_get();			\
		} while ((!cond) && (ret == 0) &&			\
			(ktime_compare(ktime_sub(exp_ktime, cur_ktime), ktime_set(0, 0)) > 0));\
	} while (0)

static void _lock(uint64_t *wait)
{
#if defined(__aarch64__)
	__asm__(
		// Sequence to wait for lock to be free (i.e. zero)
		"PRFM PSTL1KEEP, [%x[i_lock]]\n\t"
		"1:\n\t"
		"LDAXR W5, [%x[i_lock]]\n\t"
		"CBNZ W5, 1b\n\t"
		// Sequence to set PVM BIT0
		"LDR W7, =0x1\n\t"              // Load BIT0 (0x1) into W7
		"STXR W5, W7, [%x[i_lock]]\n\t" // Atomic Store exclusive BIT0 (lock = 0x1)
		"CBNZ W5, 1b\n\t"               // If cannot set it, goto 1
		:
		: [i_lock] "r" (wait)
		: "memory");
#elif
	HWFNC_ERR("cannot lock\n");
#endif
}

static void _unlock_vm(struct hw_fence_driver_data *drv_data, uint64_t *lock)
{
	uint64_t lock_val;

#if defined(__aarch64__)
	__asm__(
		// Sequence to clear PVM BIT0
		"2:\n\t"
		"LDAXR W5, [%x[i_out]]\n\t"             // Atomic Fetch Lock
		"AND W6, W5, #0xFFFFFFFFFFFFFFFE\n\t"   // AND to clear BIT0 (lock &= ~0x1))
		"STXR W5, W6, [%x[i_out]]\n\t"          // Store exclusive result
		"CBNZ W5, 2b\n\t"                       // If cannot store exclusive, goto 2
		:
		: [i_out] "r" (lock)
		: "memory");
#elif
	HWFNC_ERR("cannot unlock\n");
#endif
	mb(); /* Make sure the memory is updated */

	lock_val = *lock; /* Read the lock value */
	HWFNC_DBG_LOCK("unlock: lock_val after:0x%llx\n", lock_val);
	if (lock_val & HW_FENCE_FCTL_LOCK_VALUE) { /* check if SVM BIT1 is set*/
		/*
		 * SVM is in WFI state, since SVM acquire bit is set
		 * Trigger IRQ to Wake-Up SVM Client
		 */
#if IS_ENABLED(CONFIG_DEBUG_FS)
		drv_data->debugfs_data.lock_wake_cnt++;
		HWFNC_DBG_LOCK("triggering ipc to unblock SVM lock_val:%llu cnt:%llu\n", lock_val,
			drv_data->debugfs_data.lock_wake_cnt);
#endif
		hw_fence_ipcc_trigger_signal(drv_data,
			drv_data->ipcc_client_pid,
			drv_data->ipcc_fctl_vid, 30); /* Trigger APPS Signal 30 */
	}
}

static void _unlock_soccp(uint64_t *lock)
{
	/* Signal Client */
#if defined(__aarch64__)
	__asm__("STLR WZR, [%x[i_out]]\n\t"
		"SEV\n"
		:
		: [i_out] "r" (lock)
		: "memory");
#elif
	HWFNC_ERR("cannot unlock\n");
#endif
}

void global_atomic_store(struct hw_fence_driver_data *drv_data, uint64_t *lock, bool val)
{
	if (val) {
		preempt_disable();
		_lock(lock);
	} else {
		if (drv_data->has_soccp)
			_unlock_soccp(lock);
		else
			_unlock_vm(drv_data, lock);
		preempt_enable();
	}
}

int hw_fence_utils_fence_error_cb(struct msm_hw_fence_client *hw_fence_client, u64 ctxt_id,
	u64 seqno, u64 hash, u64 flags, u32 error)
{
	struct msm_hw_fence_cb_data cb_data;
	struct dma_fence fence;
	int ret = 0;

	if (IS_ERR_OR_NULL(hw_fence_client)) {
		HWFNC_ERR("Invalid client:0x%pK\n", hw_fence_client);
		return -EINVAL;
	}

	mutex_lock(&hw_fence_client->error_cb_lock);
	if (!error || !hw_fence_client->fence_error_cb) {
		HWFNC_ERR("Invalid error:%d fence_error_cb:0x%pK\n", error,
			hw_fence_client->fence_error_cb);
		ret = -EINVAL;
		goto exit;
	}

	/* initialize cb_data info */
	fence.context = ctxt_id;
	fence.seqno = seqno;
	fence.flags = flags;
	fence.error = error;
	cb_data.fence = &fence;
	cb_data.data = hw_fence_client->fence_error_cb_userdata;

	HWFNC_DBG_L("invoking cb for client:%d ctx:%llu seq:%llu flags:%llu e:%u data:0x%pK\n",
		hw_fence_client->client_id, ctxt_id, seqno, flags, error,
		hw_fence_client->fence_error_cb_userdata);

	hw_fence_client->fence_error_cb(hash, error, &cb_data);

exit:
	mutex_unlock(&hw_fence_client->error_cb_lock);

	return ret;
}

static int _process_fence_error_payload(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_queue_payload *payload)
{
	struct msm_hw_fence_client *hw_fence_client;
	u32 client_id;
	int ret;

	if (!drv_data || !payload || payload->type != HW_FENCE_PAYLOAD_TYPE_2) {
		HWFNC_ERR("invalid drv_data:0x%pK payload:0x%pK type:%d expected type:%d\n",
			drv_data, payload, payload ? payload->type : -1, HW_FENCE_PAYLOAD_TYPE_2);
		return -EINVAL;
	}

	if (payload->client_data < HW_FENCE_CLIENT_ID_CTX0 ||
			payload->client_data >= drv_data->clients_num) {
		HWFNC_ERR("read invalid client_id:%llu from ctrl rxq min:%u max:%u\n",
			payload->client_data, HW_FENCE_CLIENT_ID_CTX0,
			drv_data->clients_num);
		return -EINVAL;
	}

	client_id = payload->client_data;
	HWFNC_DBG_Q("ctrl rxq rd: h:%llu ctx:%llu seq:%llu f:%llu e:%u client:%u\n", payload->hash,
		payload->ctxt_id, payload->seqno, payload->flags, payload->error, client_id);

	hw_fence_client = drv_data->clients[client_id];
	if (!hw_fence_client) {
		HWFNC_ERR("processing fence error cb for unregistered client_id:%u\n",
			client_id);
		return -EINVAL;
	}

	ret = hw_fence_utils_fence_error_cb(hw_fence_client, payload->ctxt_id,
		payload->seqno, payload->hash, payload->flags, payload->error);
	if (ret)
		HWFNC_ERR("fence_error_cb failed for client:%u ctx:%llu seq:%llu err:%u\n",
			client_id, payload->ctxt_id, payload->seqno, payload->error);

	return ret;
}

static int _process_init_soccp_payload(struct hw_fence_driver_data *drv_data,
	struct msm_hw_fence_queue_payload *payload)
{
	struct hw_fence_soccp *soccp_props;

	if (!drv_data || !drv_data->has_soccp || !payload ||
			!(payload->type == HW_FENCE_PAYLOAD_TYPE_3 ||
			payload->type == HW_FENCE_PAYLOAD_TYPE_4)) {
		HWFNC_ERR("invalid drv_data:0x%pK has_soccp:%d payload:0x%pK type:%d expected:%d\n",
			drv_data, drv_data ? drv_data->has_soccp : -1, payload,
			payload ? payload->type : -1, HW_FENCE_PAYLOAD_TYPE_3);
		return -EINVAL;
	}

	soccp_props = &drv_data->soccp_props;
	if (payload->type == HW_FENCE_PAYLOAD_TYPE_4 && !soccp_props->ssr_cnt) {
		HWFNC_ERR("incorrectly received type:%d when ssr error is not happening\n",
			payload->type);
		return -EINVAL;
	}

	HWFNC_DBG_INIT("Received ctrlq msg type:%d that soccp is initialized\n", payload->type);
	drv_data->fctl_ready = true;
	wake_up_all(&soccp_props->ssr_wait_queue);

	return 0;
}

static int _process_ctrl_rx_queue(struct hw_fence_driver_data *drv_data)
{
	struct msm_hw_fence_queue_payload payload;
	int i, ret = 0, read = 1;

	for (i = 0; read && i < HW_FENCE_MAX_ITER_READ; i++) {
		read = hw_fence_read_queue_helper(drv_data,
			&drv_data->ctrl_queues[HW_FENCE_RX_QUEUE - 1], &payload);
		if (read < 0) {
			HWFNC_DBG_Q("unable to read ctrl rxq\n");
			return read;
		}
		switch (payload.type) {
		case HW_FENCE_PAYLOAD_TYPE_2:
			ret = _process_fence_error_payload(drv_data, &payload);
			break;
		case HW_FENCE_PAYLOAD_TYPE_3:
		case HW_FENCE_PAYLOAD_TYPE_4:
			ret = _process_init_soccp_payload(drv_data, &payload);
			break;
		default:
			HWFNC_ERR("received unexpected ctrl queue payload type:%d\n", payload.type);
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}

static int _process_signaled_client_id(struct hw_fence_driver_data *drv_data, int client_id)
{
	int ret;

	HWFNC_DBG_H("Processing signaled client mask id:%d\n", client_id);
	switch (client_id) {
	case HW_FENCE_CLIENT_ID_CTRL_QUEUE:
		ret = _process_ctrl_rx_queue(drv_data);
		break;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	case HW_FENCE_CLIENT_ID_VAL0:
	case HW_FENCE_CLIENT_ID_VAL1:
	case HW_FENCE_CLIENT_ID_VAL2:
	case HW_FENCE_CLIENT_ID_VAL3:
	case HW_FENCE_CLIENT_ID_VAL4:
	case HW_FENCE_CLIENT_ID_VAL5:
	case HW_FENCE_CLIENT_ID_VAL6:
		ret = process_validation_client_loopback(drv_data, client_id);
		break;
#endif /* CONFIG_DEBUG_FS */
	default:
		HWFNC_ERR("unknown mask id:%d\n", client_id);
		ret = -EINVAL;
	}

	return ret;
}

void hw_fence_utils_process_signaled_clients_mask(struct hw_fence_driver_data *drv_data,
	u64 signaled_clients_mask)
{
	int signaled_client_id;
	u64 mask;

	for (signaled_client_id = HW_FENCE_CLIENT_ID_CTRL_QUEUE;
			signaled_client_id <= HW_FENCE_SIGNALED_CLIENTS_LAST;
			signaled_client_id++) {
		mask = 1 << signaled_client_id;
		if (mask & signaled_clients_mask) {
			HWFNC_DBG_H("received signaled_client:%d mask:0x%llx\n", signaled_client_id,
				signaled_clients_mask);

			if (_process_signaled_client_id(drv_data, signaled_client_id))
				HWFNC_ERR("Failed to process signaled_client:%d\n",
					signaled_client_id);

			/* clear mask for this flag id if nothing else pending finish */
			signaled_clients_mask = signaled_clients_mask & ~(mask);
			HWFNC_DBG_H("signaled_client:%d cleared flags:0x%llx mask:0x%llx\n",
				signaled_client_id, signaled_clients_mask, mask);
			if (!signaled_clients_mask)
				break;
		}
	}
}

/* doorbell callback */
static void _hw_fence_cb(int irq, void *data)
{
	struct hw_fence_driver_data *drv_data = (struct hw_fence_driver_data *)data;
	gh_dbl_flags_t clear_flags = HW_FENCE_ALL_SIGNALED_CLIENTS_MASK;
	int ret;

	if (!drv_data)
		return;

	ret = gh_dbl_read_and_clean(drv_data->rx_dbl, &clear_flags, 0);
	if (ret) {
		HWFNC_ERR("hw_fence db callback, retrieve flags fail ret:%d\n", ret);
		return;
	}

	HWFNC_DBG_IRQ("db callback label:%d irq:%d flags:0x%llx qtime:%llu\n", drv_data->db_label,
		irq, clear_flags, hw_fence_get_qtime(drv_data));

	hw_fence_utils_process_signaled_clients_mask(drv_data, clear_flags);
}

int hw_fence_utils_init_virq(struct hw_fence_driver_data *drv_data)
{
	struct device_node *node = drv_data->dev->of_node;
	struct device_node *node_compat;
	const char *compat = "qcom,msm-hw-fence-db";
	int ret;

	node_compat = of_find_compatible_node(node, NULL, compat);
	if (!node_compat) {
		HWFNC_ERR("Failed to find dev node with compat:%s\n", compat);
		return -EINVAL;
	}

	ret = of_property_read_u32(node_compat, "gunyah-label", &drv_data->db_label);
	if (ret) {
		HWFNC_ERR("failed to find label info %d\n", ret);
		return ret;
	}

	HWFNC_DBG_IRQ("registering doorbell db_label:%d\n", drv_data->db_label);
	drv_data->rx_dbl = gh_dbl_rx_register(drv_data->db_label, _hw_fence_cb, drv_data);
	if (IS_ERR_OR_NULL(drv_data->rx_dbl)) {
		ret = PTR_ERR(drv_data->rx_dbl);
		HWFNC_ERR("Failed to register doorbell\n");
		return ret;
	}

	return 0;
}

static irqreturn_t hw_fence_soccp_irq_handler(int irq, void *data)
{
	struct hw_fence_driver_data *drv_data = (struct hw_fence_driver_data *)data;
	u32 mask;

	mask = hw_fence_ipcc_get_signaled_clients_mask(drv_data);
	atomic_or(mask, &drv_data->signaled_clients_mask);
	wake_up_all(&drv_data->soccp_wait_queue);

	return IRQ_HANDLED;
}

static int hw_fence_soccp_listener(void *data)
{
	struct hw_fence_driver_data *drv_data = (struct hw_fence_driver_data *)data;
	u32 mask;

	while (drv_data->has_soccp) {
		wait_event(drv_data->soccp_wait_queue,
			atomic_read(&drv_data->signaled_clients_mask) != 0);
		mask = atomic_xchg(&drv_data->signaled_clients_mask, 0);
		if (mask)
			hw_fence_utils_process_signaled_clients_mask(drv_data, mask);
	}

	return 0;
}

static int _send_bootup_ctrl_txq_msg(struct hw_fence_driver_data *drv_data, u32 payload_type)
{
	struct msm_hw_fence_queue *queue;
	int ret;

	if (drv_data->fctl_ready)
		return 0;

	ret = hw_fence_utils_set_power_vote(drv_data, true);
	if (ret) {
		HWFNC_ERR("failed to set power vote to send ctrlq message ret:%d\n", ret);
		return -EINVAL;
	}

	/* soccp may fail to wake up during hw-fence driver probe */
	if (!drv_data->soccp_props.is_awake) {
		HWFNC_DBG_INFO("rproc_set_state call failed to wake up soccp\n");
		ret = hw_fence_utils_set_power_vote(drv_data, false);
		if (ret)
			HWFNC_ERR("failed to remove power vote for ctrlq msg ret:%d\n", ret);

		return -EINVAL;
	}

	queue = &drv_data->ctrl_queues[HW_FENCE_TX_QUEUE - 1];
	ret = hw_fence_update_queue_helper(drv_data, 0, queue, payload_type, 0, 0, 0,
		0, 0, 0, HW_FENCE_TX_QUEUE - 1);
	if (ret) {
		HWFNC_ERR("unable to update ctrl txq message\n");
		return ret;
	}

	hw_fence_ipcc_trigger_signal(drv_data, drv_data->ipcc_client_pid, drv_data->ipcc_fctl_vid,
		hw_fence_ipcc_get_signal_id(drv_data, 0));

	/* wait for communication back from soccp with timeout */
	hw_fence_wait_event_timeout(drv_data->soccp_props.ssr_wait_queue, drv_data->fctl_ready,
		HW_FENCE_SOCCP_INIT_TIMEOUT_MS, ret);

	ret = hw_fence_utils_set_power_vote(drv_data, false);
	if (ret)
		HWFNC_ERR("failed to remove power vote for ctrlq msg ret:%d\n", ret);

	if (!drv_data->fctl_ready) {
		HWFNC_ERR("failed to receive ctrlq message for bootup event ret:%d\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int hw_fence_utils_init_soccp_irq(struct hw_fence_driver_data *drv_data)
{
	struct platform_device *pdev;
	struct task_struct *thread;
	int irq, ret;

	if (!drv_data || !drv_data->dev || !drv_data->has_soccp) {
		HWFNC_ERR("invalid drv_data:0x%pK dev:0x%pK has_soccp:%d\n", drv_data,
			drv_data ? drv_data->dev : NULL, drv_data ? drv_data->has_soccp : -1);
		return -EINVAL;
	}

	init_waitqueue_head(&drv_data->soccp_wait_queue);

	pdev = to_platform_device(drv_data->dev);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		HWFNC_ERR("failed to get the irq\n");
		return irq;
	}
	HWFNC_DBG_INIT("Registering irq:%d\n", irq);

	ret = devm_request_irq(drv_data->dev, irq, hw_fence_soccp_irq_handler, IRQF_TRIGGER_HIGH,
		"hwfence-driver", drv_data);
	if (ret < 0) {
		HWFNC_ERR("failed to register irq:%d ret:%d\n", irq, ret);
		return ret;
	}

	thread = kthread_run(hw_fence_soccp_listener, (void *)drv_data,
		"msm_hw_fence_soccp_listener");
	if (IS_ERR(thread)) {
		HWFNC_ERR("failed to create thread to process signals received from soccp\n");
		return PTR_ERR(thread);
	}
	drv_data->soccp_listener_thread = thread;

	return ret;
}

#if (KERNEL_VERSION(6, 1, 25) <= LINUX_VERSION_CODE)
/*
 * This is called to set soccp power vote based off internal counter of soccp power votes.
 * This must be called with rproc_lock held
 */
static int _set_intended_soccp_state(struct hw_fence_soccp *soccp_props)
{
	bool intended_state;
	int ret;

	intended_state = (refcount_read(&soccp_props->usage_cnt) > 1);

	if (intended_state == soccp_props->is_awake)
		return 0;

	/* cannot call soccp power vote because soccp has crashed */
	if (IS_ERR_OR_NULL(soccp_props->rproc)) {
		HWFNC_DBG_SSR("Cannot set power vote before after_powerup notification\n");
		return -EINVAL;
	}

	ret = rproc_set_state(soccp_props->rproc, intended_state);
	if (!ret)
		soccp_props->is_awake = intended_state;

	return ret;
}

int hw_fence_utils_set_power_vote(struct hw_fence_driver_data *drv_data, bool state)
{
	struct hw_fence_soccp *soccp_props;
	bool prev_state, cur_state;
	int ret;

	if (!drv_data || !drv_data->has_soccp) {
		HWFNC_ERR("invalid params: drv_data:0x%pK has_soccp:%d state:%d\n", drv_data,
			drv_data ? drv_data->has_soccp : -1, state);
		return -EINVAL;
	}

	soccp_props = &drv_data->soccp_props;
	mutex_lock(&soccp_props->rproc_lock);
	if (state) {
		refcount_inc(&soccp_props->usage_cnt);
	} else {
		if (refcount_read(&soccp_props->usage_cnt) == 1) {
			mutex_unlock(&soccp_props->rproc_lock);
			HWFNC_ERR("removing usage cnt that was never set\n");

			return -EINVAL;
		}
		refcount_dec(&soccp_props->usage_cnt);
	}

	prev_state = soccp_props->is_awake;
	ret = _set_intended_soccp_state(soccp_props);
	cur_state = soccp_props->is_awake;

	mutex_unlock(&soccp_props->rproc_lock);

	HWFNC_DBG_L("Set power vote prev:%d curr:%d req_state:%d votes:0x%x ret:%d\n",
		prev_state, cur_state, state, refcount_read(&soccp_props->usage_cnt), ret);

	return 0; /* do not expose failures of power vote to client */
}
#else
int hw_fence_utils_set_power_vote(struct hw_fence_driver_data *drv_data, bool state)
{
	HWFNC_ERR("Kernel version does not support SOCCP power votes\n");
	return -EINVAL;
}
#endif

static int _set_soccp_rproc(struct hw_fence_soccp *soccp_props, phandle ph)
{
	int ret = 0;

	mutex_lock(&soccp_props->rproc_lock);
	if (IS_ERR_OR_NULL(soccp_props->rproc))
		soccp_props->rproc = rproc_get_by_phandle(ph);
	if (IS_ERR_OR_NULL(soccp_props->rproc)) {
		ret = PTR_ERR(soccp_props->rproc);
		if (!ret)
			ret = -EINVAL;
		soccp_props->rproc = NULL;
	}
	mutex_unlock(&soccp_props->rproc_lock);

	return ret;
}

static int hw_fence_notify_ssr(struct notifier_block *nb, unsigned long action, void *data)
{
	struct hw_fence_soccp *soccp_props = container_of(nb, struct hw_fence_soccp, ssr_nb);
	struct hw_fence_driver_data *drv_data = container_of(soccp_props,
		struct hw_fence_driver_data, soccp_props);
	struct qcom_ssr_notify_data *notify_data = data;
	u32 payload_type;
	int ret = 0;

	switch (action) {
	case QCOM_SSR_BEFORE_POWERUP:
		HWFNC_DBG_SSR("received soccp starting event\n");
		break;
	case QCOM_SSR_AFTER_POWERUP:
		HWFNC_DBG_SSR("received soccp running event\n");
		/* rproc must be available after power up notification */
		ret = _set_soccp_rproc(soccp_props, soccp_props->rproc_ph);
		if (ret)
			HWFNC_ERR("failed getting soccp_rproc:0x%pK ph:%d usage_cnt:0x%x ret:%d\n",
				soccp_props->rproc, soccp_props->rproc_ph,
				refcount_read(&soccp_props->usage_cnt), ret);
		/* inform soccp of ctrl queue updates once it is up; this will set a power vote */
		payload_type = (soccp_props->ssr_cnt) ? HW_FENCE_PAYLOAD_TYPE_4 :
			HW_FENCE_PAYLOAD_TYPE_3;
		ret = _send_bootup_ctrl_txq_msg(drv_data, payload_type);
		if (ret) {
			HWFNC_ERR("failed to send ctrlq message for bootup event\n");
			goto end;
		}
		break;
	case QCOM_SSR_BEFORE_SHUTDOWN:
		HWFNC_DBG_SSR("received soccp %s event ssr_cnt:%d\n", notify_data->crashed ?
			"crashed" : "stopping", soccp_props->ssr_cnt);
		/* disallow fence creation, signaling, etc. when soccp is going to stop or crash */
		drv_data->fctl_ready = false;
		soccp_props->ssr_cnt++;
		break;
	case QCOM_SSR_AFTER_SHUTDOWN:
		HWFNC_DBG_SSR("received soccp offline event\n");
		mutex_lock(&soccp_props->rproc_lock);
		if (!IS_ERR_OR_NULL(soccp_props->rproc))
			rproc_put(soccp_props->rproc);
		soccp_props->rproc = NULL;
		soccp_props->is_awake = false;
		mutex_unlock(&soccp_props->rproc_lock);
		ret = hw_fence_ssr_cleanup_table(drv_data, drv_data->hw_fences_tbl,
			drv_data->hw_fence_table_entries, HW_FENCE_FCTL_LOCK_VALUE);
		if (ret)
			HWFNC_ERR("failed to cleanup hw-fence table for soccp ssr\n");
		break;
	default:
		HWFNC_ERR("received unrecognized event %lu\n", action);
		break;
	}

end:
	return ret ? NOTIFY_BAD : NOTIFY_OK;
}

int hw_fence_utils_register_soccp_ssr_notifier(struct hw_fence_driver_data *drv_data)
{
	void *notifier;
	struct hw_fence_soccp *soccp_props;
	int ret;

	if (!drv_data || !drv_data->has_soccp) {
		HWFNC_ERR("invalid drv_data:0x%pK has_soccp:%d\n", drv_data,
			drv_data ? drv_data->has_soccp : -1);
		return -EINVAL;
	}
	soccp_props = &drv_data->soccp_props;

	mutex_init(&soccp_props->rproc_lock);
	refcount_set(&soccp_props->usage_cnt, 1);
	init_waitqueue_head(&soccp_props->ssr_wait_queue);
	soccp_props->ssr_nb.priority = 1; /* higher value indicates higher priority */
	soccp_props->ssr_nb.notifier_call = hw_fence_notify_ssr;
	notifier = qcom_register_ssr_notifier("soccp", &soccp_props->ssr_nb);
	if (IS_ERR(notifier)) {
		HWFNC_ERR("failed to register soccp ssr notifier\n");
		return PTR_ERR(notifier);
	}
	soccp_props->ssr_notifier = notifier;
	HWFNC_DBG_SSR("registered for soccp ssr notification notifier:0x%pK\n", notifier);

	/* if soccp is already up, do initial bootup here; this first attempt may fail */
	ret = _set_soccp_rproc(soccp_props, soccp_props->rproc_ph);
	if (ret) {
		HWFNC_DBG_INFO("failed getting soccp_rproc:0x%pK ph:%d at probe time ret:%d\n",
			soccp_props->rproc, soccp_props->rproc_ph, ret);
		return 0;
	}

	ret = _send_bootup_ctrl_txq_msg(drv_data, HW_FENCE_PAYLOAD_TYPE_3);
	if (ret)
		HWFNC_DBG_INFO("can't send ctrl tx queue msg to inform soccp of mem map\n");

	return 0;
}

static int hw_fence_gunyah_share_mem(struct hw_fence_driver_data *drv_data,
				gh_vmid_t self, gh_vmid_t peer)
{
	struct qcom_scm_vmperm src_vmlist[] = {{self, PERM_READ | PERM_WRITE | PERM_EXEC}};
	struct qcom_scm_vmperm dst_vmlist[] = {{self, PERM_READ | PERM_WRITE},
					       {peer, PERM_READ | PERM_WRITE}};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	u64 srcvmids, dstvmids;
#else
	unsigned int srcvmids, dstvmids;
#endif
	struct gh_acl_desc *acl;
	struct gh_sgl_desc *sgl;
	int ret;

	srcvmids = BIT(src_vmlist[0].vmid);
	dstvmids = BIT(dst_vmlist[0].vmid) | BIT(dst_vmlist[1].vmid);
	ret = qcom_scm_assign_mem(drv_data->res.start, resource_size(&drv_data->res), &srcvmids,
		dst_vmlist, ARRAY_SIZE(dst_vmlist));
	if (ret) {
		HWFNC_ERR("%s: qcom_scm_assign_mem failed addr=0x%llx size=%lu err=%d\n",
			__func__, drv_data->res.start, drv_data->size, ret);
		return ret;
	}

	acl = kzalloc(offsetof(struct gh_acl_desc, acl_entries[2]), GFP_KERNEL);
	if (!acl)
		return -ENOMEM;
	sgl = kzalloc(offsetof(struct gh_sgl_desc, sgl_entries[1]), GFP_KERNEL);
	if (!sgl) {
		kfree(acl);
		return -ENOMEM;
	}
	acl->n_acl_entries = 2;
	acl->acl_entries[0].vmid = (u16)self;
	acl->acl_entries[0].perms = GH_RM_ACL_R | GH_RM_ACL_W;
	acl->acl_entries[1].vmid = (u16)peer;
	acl->acl_entries[1].perms = GH_RM_ACL_R | GH_RM_ACL_W;

	sgl->n_sgl_entries = 1;
	sgl->sgl_entries[0].ipa_base = drv_data->res.start;
	sgl->sgl_entries[0].size = resource_size(&drv_data->res);

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	ret = ghd_rm_mem_share(GH_RM_MEM_TYPE_NORMAL, 0, drv_data->label,
			acl, sgl, NULL, &drv_data->memparcel);
#else
	ret = gh_rm_mem_share(GH_RM_MEM_TYPE_NORMAL, 0, drv_data->label,
			acl, sgl, NULL, &drv_data->memparcel);
#endif
	if (ret) {
		HWFNC_ERR("%s: gh_rm_mem_share failed addr=%llx size=%lu err=%d\n",
			__func__, drv_data->res.start, drv_data->size, ret);
		/* Attempt to give resource back to HLOS */
		qcom_scm_assign_mem(drv_data->res.start, resource_size(&drv_data->res), &dstvmids,
			src_vmlist, ARRAY_SIZE(src_vmlist));
		ret = -EPROBE_DEFER;
	}

	kfree(acl);
	kfree(sgl);

	return ret;
}

static int _is_mem_shared(struct resource *res)
{
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	return gh_cpusys_vm_get_share_mem_info(res);
#else
	return -EINVAL;
#endif
}

static int hw_fence_rm_cb(struct notifier_block *nb, unsigned long cmd, void *data)
{
	struct gh_rm_notif_vm_status_payload *vm_status_payload;
	struct hw_fence_driver_data *drv_data;
	struct resource res;
	gh_vmid_t peer_vmid;
	gh_vmid_t self_vmid;
	int ret;

	drv_data = container_of(nb, struct hw_fence_driver_data, rm_nb);

	HWFNC_DBG_INIT("cmd:0x%lx ++\n", cmd);
	if (cmd != GH_RM_NOTIF_VM_STATUS)
		goto end;

	vm_status_payload = data;
	HWFNC_DBG_INIT("payload vm_status:%d\n", vm_status_payload->vm_status);
	if (vm_status_payload->vm_status != GH_RM_VM_STATUS_READY &&
	    vm_status_payload->vm_status != GH_RM_VM_STATUS_RESET)
		goto end;

#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	if (ghd_rm_get_vmid(drv_data->peer_name, &peer_vmid))
		goto end;

	if (ghd_rm_get_vmid(GH_PRIMARY_VM, &self_vmid))
		goto end;
#else
	if (gh_rm_get_vmid(drv_data->peer_name, &peer_vmid))
		goto end;

	if (gh_rm_get_vmid(GH_PRIMARY_VM, &self_vmid))
		goto end;
#endif

	if (peer_vmid != vm_status_payload->vmid)
		goto end;

	switch (vm_status_payload->vm_status) {
	case GH_RM_VM_STATUS_READY:
		ret = _is_mem_shared(&res);
		if (ret) {
			HWFNC_DBG_INIT("mem not shared ret:%d, attempt share\n", ret);
			if (hw_fence_gunyah_share_mem(drv_data, self_vmid, peer_vmid))
				HWFNC_ERR("failed to share memory\n");
			else
				drv_data->fctl_ready = true;
		} else {
			if (drv_data->res.start == res.start &&
					resource_size(&drv_data->res) == resource_size(&res)) {
				drv_data->fctl_ready = true;
				HWFNC_DBG_INIT("mem_ready: add:0x%llx size:%llu ret:%d\n",
					res.start, resource_size(&res), ret);
			} else {
				HWFNC_ERR("mem-shared:[0x%llx,%llu] expected:[0x%llx,%llu]\n",
					res.start, resource_size(&res), drv_data->res.start,
					resource_size(&drv_data->res));
			}
		}
		break;
	case GH_RM_VM_STATUS_RESET:
		HWFNC_DBG_INIT("reset\n");
		break;
	}

end:
	return NOTIFY_DONE;
}

static int _register_vm_mem_with_hyp(struct hw_fence_driver_data *drv_data,
	struct device_node *node_compat)
{
	int ret, notifier_ret;

	if (!drv_data || !node_compat) {
		HWFNC_ERR("invalid params drv_data:0x%pK node_compat:0x%pK\n", drv_data,
			node_compat);
		return -EINVAL;
	}

	ret = of_property_read_u32(node_compat, "gunyah-label", &drv_data->label);
	if (ret) {
		HWFNC_ERR("failed to find label info %d\n", ret);
		return ret;
	}

	/* Register memory with HYP for vm */
	ret = of_property_read_u32(node_compat, "peer-name", &drv_data->peer_name);
	if (ret)
		drv_data->peer_name = GH_SELF_VM;

	drv_data->rm_nb.notifier_call = hw_fence_rm_cb;
	drv_data->rm_nb.priority = INT_MAX;
	notifier_ret = gh_rm_register_notifier(&drv_data->rm_nb);
	HWFNC_DBG_INIT("notifier: ret:%d peer_name:%d notifier_ret:%d\n", ret,
		drv_data->peer_name, notifier_ret);
	if (notifier_ret) {
		HWFNC_ERR_ONCE("fail to register notifier ret:%d\n", ret);
		return -EPROBE_DEFER;
	}

	return 0;
}

static int _init_soccp_mem(struct hw_fence_driver_data *drv_data)
{
	struct iommu_domain *domain;
	u32 shbuf_soccp_va;
	int ret;

	if (!drv_data) {
		HWFNC_ERR("invalid params drv_data:0x%pK\n", drv_data);
		return -EINVAL;
	}

	ret = of_property_read_u32(drv_data->dev->of_node, "shbuf_soccp_va", &shbuf_soccp_va);
	if (ret || !shbuf_soccp_va) {
		if (drv_data->cpu_addr_cookie) {
			HWFNC_ERR("non-static mem allocation w/out soccp_va dt ret:%d val:%d\n",
				ret, shbuf_soccp_va);
			return -EINVAL;
		}
		/* use one to one memory mapping if virtual address is not in dt */
		shbuf_soccp_va = drv_data->res.start;
	}

	domain = iommu_get_domain_for_dev(drv_data->dev);
	if (IS_ERR_OR_NULL(domain)) {
		HWFNC_ERR("failed to get iommu domain for device ret:%ld\n", PTR_ERR(domain));
		return PTR_ERR(domain);
	}

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
	ret = iommu_map(domain, shbuf_soccp_va, drv_data->res.start, drv_data->size,
		IOMMU_READ | IOMMU_WRITE | IOMMU_CACHE, GFP_KERNEL);
#else
	ret = iommu_map(domain, shbuf_soccp_va, drv_data->res.start, drv_data->size,
		IOMMU_READ | IOMMU_WRITE | IOMMU_CACHE);
#endif
	if (ret)
		HWFNC_ERR("failed to map for soccp smmu phys_addr:0x%llx va:0x%x sz:%lx ret:%d\n",
			drv_data->res.start, shbuf_soccp_va, drv_data->size, ret);
	else
		HWFNC_DBG_INIT("mapped for soccp smmu phys_addr:0x%llx va:0x%x sz:%lx ret:%d\n",
			drv_data->res.start, shbuf_soccp_va, drv_data->size, ret);

	return ret;
}

/* Allocates carved-out mapped memory from device-tree */
static int _alloc_mem_static(struct hw_fence_driver_data *drv_data, struct device_node *node_compat)
{
	struct device_node *np;
	int ret;

	if (!drv_data || !node_compat) {
		HWFNC_ERR("invalid drv_data:0x%pK node_compat:0x%pK\n", drv_data, node_compat);
		return -EINVAL;
	}

	np = of_parse_phandle(node_compat, "shared-buffer", 0);
	if (!np) {
		HWFNC_ERR("failed to read shared-buffer info\n");
		return -ENOMEM;
	}

	ret = of_address_to_resource(np, 0, &drv_data->res);
	of_node_put(np);
	if (ret) {
		HWFNC_ERR("of_address_to_resource failed %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

/* Allocates memory dynamically */
static int _alloc_mem_dynamic(struct hw_fence_driver_data *drv_data)
{
	u32 events_size, size;

	if (!drv_data || !drv_data->has_soccp) {
		HWFNC_ERR("invalid drv_data:0x%pK has_soccp:%d\n", drv_data,
			drv_data ? drv_data->has_soccp : -1);
		return -EINVAL;
	}

	events_size = HW_FENCE_MAX_EVENTS * sizeof(struct msm_hw_fence_event);
	if (drv_data->used_mem_size >= U32_MAX - events_size) {
		HWFNC_ERR("invalid used_mem_size:%u events_size:%u\n", drv_data->used_mem_size,
			events_size);
		return -EINVAL;
	}

	size = PAGE_ALIGN(drv_data->used_mem_size + events_size);
	drv_data->cpu_addr_cookie = dma_alloc_attrs(drv_data->dev, size, &drv_data->res.start,
		GFP_KERNEL, DMA_ATTR_NO_KERNEL_MAPPING);
	if (!drv_data->cpu_addr_cookie) {
		HWFNC_ERR("memory allocation failed!\n");
		return -ENOMEM;
	}

	drv_data->res.end = drv_data->res.start + size - 1;
	drv_data->res.name = "hwfence_shbuf";
	HWFNC_DBG_INIT("allocated memory start:0x%llx end:0x%llx size:0x%x\n", drv_data->res.start,
		drv_data->res.end, size);

	return 0;
}

/* Allocates carved-out mapped memory */
int hw_fence_utils_alloc_mem(struct hw_fence_driver_data *drv_data)
{
	struct device_node *node = drv_data->dev->of_node;
	struct device_node *node_compat;
	const char *compat = "qcom,msm-hw-fence-mem";
	int ret;

	node_compat = of_find_compatible_node(node, NULL, compat);
	if (!node_compat && !drv_data->has_soccp) {
		HWFNC_ERR("Failed to find dev node with compat:%s\n", compat);
		return -EINVAL;
	}

	if (node_compat)
		ret = _alloc_mem_static(drv_data, node_compat);
	else
		ret = _alloc_mem_dynamic(drv_data);

	if (ret) {
		HWFNC_ERR("failed to allocate static or dynamic memory ret:%d\n", ret);
		return ret;
	}

	if (drv_data->has_soccp)
		drv_data->io_mem_base = memremap(drv_data->res.start, resource_size(&drv_data->res),
			MEMREMAP_WB);
	else
		drv_data->io_mem_base = devm_ioremap_wc(drv_data->dev, drv_data->res.start,
			resource_size(&drv_data->res));

	if (!drv_data->io_mem_base) {
		HWFNC_ERR("ioremap failed!\n");
		return -ENXIO;
	}
	drv_data->size = resource_size(&drv_data->res);
	if (drv_data->size < drv_data->used_mem_size) {
		HWFNC_ERR("0x%lx size of carved-out memory region less than required size:0x%x\n",
			drv_data->size, drv_data->used_mem_size);
		return -ENOMEM;
	}

	memset_io(drv_data->io_mem_base, 0x0, drv_data->size);

	HWFNC_DBG_INIT("va:0x%pK start:0x%llx sz:0x%lx name:%s cookie:0x%pK has_soccp:%s\n",
		drv_data->io_mem_base, drv_data->res.start, drv_data->size, drv_data->res.name,
		drv_data->cpu_addr_cookie, drv_data->has_soccp ? "true" : "false");

	if (drv_data->has_soccp)
		ret = _init_soccp_mem(drv_data);
	else
		ret = _register_vm_mem_with_hyp(drv_data, node_compat);

	if (ret)
		HWFNC_ERR("failed to share mem with %s cpu_va:0x%pK pa:0x%llx sz:0x%lx name:%s\n",
			drv_data->has_soccp ? "soccp" : "vm", drv_data->io_mem_base,
			drv_data->res.start, drv_data->size, drv_data->res.name);

	return ret;
}

char *_get_mem_reserve_type(enum hw_fence_mem_reserve type)
{
	switch (type) {
	case HW_FENCE_MEM_RESERVE_CTRL_QUEUE:
		return "HW_FENCE_MEM_RESERVE_CTRL_QUEUE";
	case HW_FENCE_MEM_RESERVE_LOCKS_REGION:
		return "HW_FENCE_MEM_RESERVE_LOCKS_REGION";
	case HW_FENCE_MEM_RESERVE_TABLE:
		return "HW_FENCE_MEM_RESERVE_TABLE";
	case HW_FENCE_MEM_RESERVE_CLIENT_QUEUE:
		return "HW_FENCE_MEM_RESERVE_CLIENT_QUEUE";
	case HW_FENCE_MEM_RESERVE_EVENTS_BUFF:
		return "HW_FENCE_MEM_RESERVE_EVENTS_BUFF";
	}

	return "Unknown";
}

/* Calculates the memory range for each of the elements in the carved-out memory */
int hw_fence_utils_reserve_mem(struct hw_fence_driver_data *drv_data,
	enum hw_fence_mem_reserve type, phys_addr_t *phys, void **pa, u32 *size, int client_id)
{
	int ret = 0;
	u32 start_offset = 0;
	u32 remaining_size_bytes;
	u32 total_events;

	switch (type) {
	case HW_FENCE_MEM_RESERVE_CTRL_QUEUE:
		start_offset = 0;
		*size = drv_data->hw_fence_mem_ctrl_queues_size;
		break;
	case HW_FENCE_MEM_RESERVE_LOCKS_REGION:
		/* Locks region starts at the end of the ctrl queues */
		start_offset = drv_data->hw_fence_mem_ctrl_queues_size;
		*size = HW_FENCE_MEM_LOCKS_SIZE(drv_data->rxq_clients_num);
		break;
	case HW_FENCE_MEM_RESERVE_TABLE:
		/* HW Fence table starts at the end of the Locks region */
		start_offset = drv_data->hw_fence_mem_ctrl_queues_size +
			HW_FENCE_MEM_LOCKS_SIZE(drv_data->rxq_clients_num);
		*size = drv_data->hw_fence_mem_fences_table_size;
		break;
	case HW_FENCE_MEM_RESERVE_CLIENT_QUEUE:
		if (client_id >= drv_data->clients_num ||
				!drv_data->hw_fence_client_queue_size[client_id].type) {
			HWFNC_ERR("unexpected client_id:%d for clients_num:%d\n", client_id,
				drv_data->clients_num);
			ret = -EINVAL;
			goto exit;
		}

		start_offset = drv_data->hw_fence_client_queue_size[client_id].start_offset;
		*size = drv_data->hw_fence_client_queue_size[client_id].type->mem_size;
		break;
	case HW_FENCE_MEM_RESERVE_EVENTS_BUFF:
		start_offset = drv_data->used_mem_size;
		remaining_size_bytes = drv_data->size - start_offset;
		if (start_offset >= drv_data->size ||
				remaining_size_bytes < sizeof(struct msm_hw_fence_event)) {
			HWFNC_DBG_INFO("no space for events total_sz:%lu offset:%u evt_sz:%lu\n",
				drv_data->size, start_offset, sizeof(struct msm_hw_fence_event));
			ret = -ENOMEM;
			goto exit;
		}

		total_events = remaining_size_bytes / sizeof(struct msm_hw_fence_event);
		if (total_events > HW_FENCE_MAX_EVENTS)
			total_events = HW_FENCE_MAX_EVENTS;
		*size = total_events * sizeof(struct msm_hw_fence_event);
		break;
	default:
		HWFNC_ERR("Invalid mem reserve type:%d\n", type);
		ret = -EINVAL;
		break;
	}

	if (start_offset + *size > drv_data->size) {
		HWFNC_ERR("reservation request exceeds total size:%lu\n",
			drv_data->size);
		return -ENOMEM;
	}

	HWFNC_DBG_INIT("type:%s (%d) start:0x%llx start_offset:%u size:0x%x\n",
		_get_mem_reserve_type(type), type, drv_data->res.start,
		start_offset, *size);


	*phys = drv_data->res.start + (phys_addr_t)start_offset;
	*pa = (drv_data->io_mem_base + start_offset); /* offset is in bytes */
	HWFNC_DBG_H("phys:0x%llx pa:0x%pK\n", *phys, *pa);

exit:
	return ret;
}

static int _parse_client_queue_dt_props_extra(struct hw_fence_driver_data *drv_data,
	struct hw_fence_client_type_desc *desc)
{
	u32 max_idx_from_zero, payload_size_u32 = HW_FENCE_CLIENT_QUEUE_PAYLOAD / sizeof(u32);
	char name[DT_PROPS_CLIENT_EXTRA_PROPS_SIZE];
	u32 tmp[5];
	bool idx_by_payload = false;
	int count, ret;

	snprintf(name, sizeof(name), "qcom,hw-fence-client-type-%s-extra", desc->name);

	/* check if property is present */
	ret = of_property_read_bool(drv_data->dev->of_node, name);
	if (!ret)
		return 0;

	count = of_property_count_u32_elems(drv_data->dev->of_node, name);
	if (count <= 0 || count > 5) {
		HWFNC_ERR("invalid %s extra dt props count:%d\n", desc->name, count);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(drv_data->dev->of_node, name, tmp, count);
	if (ret) {
		HWFNC_ERR("Failed to read %s extra dt properties ret=%d count=%d\n", desc->name,
			ret, count);
		ret = -EINVAL;
		goto exit;
	}

	desc->start_padding = tmp[0];
	if (count >= 2)
		desc->end_padding = tmp[1];
	if (count >= 3)
		desc->txq_idx_start = tmp[2];
	if (count >= 4) {
		if (tmp[3] > 1) {
			HWFNC_ERR("%s invalid txq_idx_by_payload prop:%u\n", desc->name, tmp[3]);
			ret = -EINVAL;
			goto exit;
		}
		idx_by_payload = tmp[3];
		desc->txq_idx_factor = idx_by_payload ? payload_size_u32 : 1;
	}
	if (count >= 5) {
		if (tmp[4] > 1) {
			HWFNC_ERR("%s invalid skip_fctl_ref prop:%u\n", desc->name, tmp[4]);
			ret = -EINVAL;
			goto exit;
		}
		desc->skip_fctl_ref = 1;
	}

	if (desc->start_padding % sizeof(u32) || desc->end_padding % sizeof(u32) ||
			(desc->start_padding + desc->end_padding) % sizeof(u64)) {
		HWFNC_ERR("%s start_padding:%u end_padding:%u violates mem alignment\n",
			desc->name, desc->start_padding, desc->end_padding);
		ret = -EINVAL;
		goto exit;
	}

	if (desc->start_padding >= U32_MAX - HW_FENCE_HFI_CLIENT_HEADERS_SIZE(desc->queues_num,
			drv_data->has_soccp)) {
		HWFNC_ERR("%s client queues_num:%u start_padding:%u will overflow mem_size\n",
			desc->name, desc->queues_num, desc->start_padding);
		ret = -EINVAL;
		goto exit;
	}

	if (desc->end_padding >= U32_MAX - HW_FENCE_HFI_CLIENT_HEADERS_SIZE(desc->queues_num,
			drv_data->has_soccp) - desc->start_padding) {
		HWFNC_ERR("%s client q_num:%u start_p:%u end_p:%u will overflow mem_size\n",
			desc->name, desc->queues_num, desc->start_padding, desc->end_padding);
		ret = -EINVAL;
		goto exit;
	}

	max_idx_from_zero = idx_by_payload ? desc->queue_entries :
		desc->queue_entries * payload_size_u32;
	if (desc->txq_idx_start >= U32_MAX - max_idx_from_zero) {
		HWFNC_ERR("%s txq_idx start:%u by_payload:%s q_entries:%u will overflow txq_idx\n",
			desc->name, desc->txq_idx_start, idx_by_payload ? "true" : "false",
			desc->queue_entries);
		ret = -EINVAL;
		goto exit;
	}

	HWFNC_DBG_INIT("%s: start_p=%u end_p=%u txq_idx_start:%u idx_by_payload:%s skip_ref:%s\n",
		desc->name, desc->start_padding, desc->end_padding, desc->txq_idx_start,
		idx_by_payload ? "true" : "false", desc->skip_fctl_ref ? "true" : "false");

exit:
	return ret;
}

static int _parse_client_queue_dt_props_indv(struct hw_fence_driver_data *drv_data,
	struct hw_fence_client_type_desc *desc)
{
	char name[DT_PROPS_CLIENT_PROPS_SIZE];
	u32 tmp[4];
	u32 queue_size;
	int ret;

	/* parse client queue properties from device-tree */
	snprintf(name, sizeof(name), "qcom,hw-fence-client-type-%s", desc->name);
	ret = of_property_read_u32_array(drv_data->dev->of_node, name, tmp, 4);
	if (ret) {
		HWFNC_DBG_INIT("missing %s client queue entry or invalid ret:%d\n", desc->name,
			ret);
		desc->queue_entries = drv_data->hw_fence_queue_entries;
	} else {
		desc->clients_num = tmp[0];
		desc->queues_num = tmp[1];
		desc->queue_entries = tmp[2];

		if (tmp[3] > 1) {
			HWFNC_ERR("%s invalid skip_txq_wr_idx prop:%u\n", desc->name, tmp[3]);
			return -EINVAL;
		}
		desc->skip_txq_wr_idx = tmp[3];
	}

	if (desc->clients_num > desc->max_clients_num || !desc->queues_num ||
			desc->queues_num > HW_FENCE_CLIENT_QUEUES || !desc->queue_entries) {
		HWFNC_ERR("%s invalid dt: clients_num:%u queues_num:%u, queue_entries:%u\n",
			desc->name, desc->clients_num, desc->queues_num, desc->queue_entries);
		return -EINVAL;
	}

	/* parse extra client queue properties from device-tree */
	ret = _parse_client_queue_dt_props_extra(drv_data, desc);
	if (ret) {
		HWFNC_ERR("%s failed to parse extra dt props\n", desc->name);
		return -EINVAL;
	}

	/* compute mem_size */
	if (desc->queue_entries >= U32_MAX / HW_FENCE_CLIENT_QUEUE_PAYLOAD) {
		HWFNC_ERR("%s client queue entries:%u will overflow client queue size\n",
			desc->name, desc->queue_entries);
		return -EINVAL;
	}

	queue_size = HW_FENCE_CLIENT_QUEUE_PAYLOAD * desc->queue_entries;
	if (queue_size >= ((U32_MAX & PAGE_MASK) -
			(HW_FENCE_HFI_CLIENT_HEADERS_SIZE(desc->queues_num, drv_data->has_soccp) +
			desc->start_padding + desc->end_padding)) / desc->queues_num) {
		HWFNC_ERR("%s client queue_sz:%u start_p:%u end_p:%u will overflow mem size\n",
			desc->name, queue_size, desc->start_padding, desc->end_padding);
		return -EINVAL;
	}

	desc->mem_size = PAGE_ALIGN(HW_FENCE_HFI_CLIENT_HEADERS_SIZE(desc->queues_num,
		drv_data->has_soccp) + (queue_size * desc->queues_num) + desc->start_padding +
		desc->end_padding);

	if (desc->mem_size > MAX_CLIENT_QUEUE_MEM_SIZE) {
		HWFNC_ERR("%s client queue mem_size:%u greater than max mem size:%d\n",
			desc->name, desc->mem_size, MAX_CLIENT_QUEUE_MEM_SIZE);
		return -EINVAL;
	}

	HWFNC_DBG_INIT("%s: clients=%u q_num=%u q_entries=%u mem_sz=%u skips_wr_ptr:%s\n",
		desc->name, desc->clients_num, desc->queues_num, desc->queue_entries,
		desc->mem_size, desc->skip_txq_wr_idx ? "true" : "false");

	return 0;
}

static int _parse_client_queue_dt_props(struct hw_fence_driver_data *drv_data)
{
	struct hw_fence_client_type_desc *desc;
	int i, j, ret;
	u32 start_offset;
	size_t size;
	int configurable_clients_num = 0;

	drv_data->rxq_clients_num = HW_FENCE_MIN_RXQ_CLIENTS;
	for (i = 0; i < HW_FENCE_MAX_CLIENT_TYPE; i++) {
		desc = &hw_fence_client_types[i];
		ret = _parse_client_queue_dt_props_indv(drv_data, desc);
		if (ret) {
			HWFNC_ERR("failed to initialize %s client queue size properties\n",
				desc->name);
			return ret;
		}

		if (i >= HW_FENCE_MIN_RXQ_CLIENT_TYPE &&
				desc->queues_num == HW_FENCE_CLIENT_QUEUES)
			drv_data->rxq_clients_num += desc->clients_num;

		if (i >= HW_FENCE_MAX_CLIENT_TYPE_STATIC)
			configurable_clients_num += desc->clients_num;
	}

	/* store client type descriptors for configurable client indexing logic */
	drv_data->hw_fence_client_types = hw_fence_client_types;

	/* clients and size desc are allocated for all static clients regardless of device-tree */
	drv_data->clients_num = HW_FENCE_MAX_STATIC_CLIENTS_INDEX + configurable_clients_num;

	/* allocate memory for client queue size descriptors */
	size = drv_data->clients_num * sizeof(struct hw_fence_client_queue_desc);
	drv_data->hw_fence_client_queue_size = kzalloc(size, GFP_KERNEL);
	if (!drv_data->hw_fence_client_queue_size)
		return -ENOMEM;

	/* initialize client queue size desc for each client */
	start_offset = PAGE_ALIGN(drv_data->hw_fence_mem_ctrl_queues_size +
		HW_FENCE_MEM_LOCKS_SIZE(drv_data->rxq_clients_num) +
		drv_data->hw_fence_mem_fences_table_size);
	for (i = 0; i < HW_FENCE_MAX_CLIENT_TYPE; i++) {
		desc = &hw_fence_client_types[i];
		for (j = 0; j < desc->clients_num; j++) {
			enum hw_fence_client_id client_id_ext = desc->init_id + j;
			enum hw_fence_client_id client_id =
				hw_fence_utils_get_client_id_priv(drv_data, client_id_ext);

			drv_data->hw_fence_client_queue_size[client_id] =
				(struct hw_fence_client_queue_desc){desc, start_offset};
			HWFNC_DBG_INIT("%s client_id_ext:%u client_id:%u start_offset:%u\n",
				desc->name, client_id_ext, client_id, start_offset);
			start_offset += desc->mem_size;
		}
	}
	drv_data->used_mem_size = start_offset;

	return 0;
}

int hw_fence_utils_parse_dt_props(struct hw_fence_driver_data *drv_data)
{
	int ret;
	size_t size;
	u32 val = 0;
	struct hw_fence_soccp *soccp_props = &drv_data->soccp_props;

	/* check presence of soccp */
	ret = of_property_read_u32(drv_data->dev->of_node, "soccp_controller",
		&soccp_props->rproc_ph);
	if (!ret && soccp_props->rproc_ph)
		drv_data->has_soccp = true;

	ret = of_property_read_u32(drv_data->dev->of_node, "qcom,hw-fence-table-entries", &val);
	if (ret || !val) {
		HWFNC_ERR("missing hw fences table entry or invalid ret:%d val:%d\n", ret, val);
		return ret;
	}
	drv_data->hw_fence_table_entries = val;

	if (drv_data->hw_fence_table_entries >= U32_MAX / sizeof(struct msm_hw_fence)) {
		HWFNC_ERR("table entries:%u will overflow table size\n",
			drv_data->hw_fence_table_entries);
		return -EINVAL;
	}
	drv_data->hw_fence_mem_fences_table_size = (sizeof(struct msm_hw_fence) *
		drv_data->hw_fence_table_entries);

	ret = of_property_read_u32(drv_data->dev->of_node, "qcom,hw-fence-queue-entries", &val);
	if (ret || !val) {
		HWFNC_ERR("missing queue entries table entry or invalid ret:%d val:%d\n", ret, val);
		return ret;
	}
	drv_data->hw_fence_queue_entries = val;

	/* ctrl queues init */

	if (drv_data->hw_fence_queue_entries >= U32_MAX / HW_FENCE_CTRL_QUEUE_PAYLOAD) {
		HWFNC_ERR("queue entries:%u will overflow ctrl queue size\n",
			drv_data->hw_fence_queue_entries);
		return -EINVAL;
	}
	drv_data->hw_fence_ctrl_queue_size = HW_FENCE_CTRL_QUEUE_PAYLOAD *
		HW_FENCE_CTRL_QUEUE_ENTRIES;

	if (drv_data->hw_fence_ctrl_queue_size >= (U32_MAX -
			HW_FENCE_HFI_CTRL_HEADERS_SIZE(drv_data->has_soccp)) /
			HW_FENCE_CTRL_QUEUES) {
		HWFNC_ERR("queue size:%u will overflow ctrl queue mem size\n",
			drv_data->hw_fence_ctrl_queue_size);
		return -EINVAL;
	}
	drv_data->hw_fence_mem_ctrl_queues_size =
		HW_FENCE_HFI_CTRL_HEADERS_SIZE(drv_data->has_soccp) +
		(HW_FENCE_CTRL_QUEUES * drv_data->hw_fence_ctrl_queue_size);

	/* clients queues init */

	ret = _parse_client_queue_dt_props(drv_data);
	if (ret) {
		HWFNC_ERR("failed to parse client queue properties\n");
		return -EINVAL;
	}

	/* allocate clients */

	size = drv_data->clients_num * sizeof(struct msm_hw_fence_client *);
	drv_data->clients = kzalloc(size, GFP_KERNEL);
	if (!drv_data->clients)
		return -ENOMEM;

	HWFNC_DBG_INIT("table: entries=%u mem_size=%u queue: entries=%u\b",
		drv_data->hw_fence_table_entries, drv_data->hw_fence_mem_fences_table_size,
		drv_data->hw_fence_queue_entries);
	HWFNC_DBG_INIT("ctrl queue: size=%u mem_size=%u\b",
		drv_data->hw_fence_ctrl_queue_size, drv_data->hw_fence_mem_ctrl_queues_size);
	HWFNC_DBG_INIT("clients_num: %u, total_mem_size:%u\n", drv_data->clients_num,
		drv_data->used_mem_size);
	HWFNC_DBG_INIT("has_soccp:%s\n", drv_data->has_soccp ? "true" : "false");

	return 0;
}

int hw_fence_utils_map_ipcc(struct hw_fence_driver_data *drv_data)
{
	int ret;
	u32 reg_config[2];
	void __iomem *ptr;

	/* Get ipcc memory range */
	ret = of_property_read_u32_array(drv_data->dev->of_node, "qcom,ipcc-reg",
				reg_config, 2);
	if (ret) {
		HWFNC_ERR("failed to read ipcc reg: %d\n", ret);
		return ret;
	}
	drv_data->ipcc_reg_base = reg_config[0];
	drv_data->ipcc_size = reg_config[1];

	/* Mmap ipcc registers */
	ptr = devm_ioremap(drv_data->dev, drv_data->ipcc_reg_base, drv_data->ipcc_size);
	if (!ptr) {
		HWFNC_ERR("failed to ioremap ipcc regs\n");
		return -ENOMEM;
	}
	drv_data->ipcc_io_mem = ptr;

	HWFNC_DBG_H("mapped address:0x%llx size:0x%x io_mem:0x%pK\n",
		drv_data->ipcc_reg_base, drv_data->ipcc_size,
		drv_data->ipcc_io_mem);

	hw_fence_ipcc_enable_signaling(drv_data);

	return ret;
}

int hw_fence_utils_map_qtime(struct hw_fence_driver_data *drv_data)
{
	int ret = 0;
	unsigned int reg_config[2];
	void __iomem *ptr;

	ret = of_property_read_u32_array(drv_data->dev->of_node, "qcom,qtime-reg",
			reg_config, 2);
	if (ret) {
		HWFNC_ERR("failed to read qtimer reg: %d\n", ret);
		return ret;
	}

	drv_data->qtime_reg_base = reg_config[0];
	drv_data->qtime_size = reg_config[1];

	ptr = devm_ioremap(drv_data->dev, drv_data->qtime_reg_base, drv_data->qtime_size);
	if (!ptr) {
		HWFNC_ERR("failed to ioremap qtime regs\n");
		return -ENOMEM;
	}

	drv_data->qtime_io_mem = ptr;

	return ret;
}

enum hw_fence_client_id hw_fence_utils_get_client_id_priv(struct hw_fence_driver_data *drv_data,
	enum hw_fence_client_id client_id)
{
	int i, client_type, offset;
	enum hw_fence_client_id client_id_priv;

	if (client_id < HW_FENCE_MAX_STATIC_CLIENTS_INDEX)
		return client_id;

	/* consolidate external 'hw_fence_client_id' enum into consecutive internal client IDs */
	client_type = HW_FENCE_MAX_CLIENT_TYPE_STATIC +
		(client_id - HW_FENCE_MAX_STATIC_CLIENTS_INDEX) /
		MSM_HW_FENCE_MAX_SIGNAL_PER_CLIENT;
	offset = (client_id - HW_FENCE_MAX_STATIC_CLIENTS_INDEX) %
		MSM_HW_FENCE_MAX_SIGNAL_PER_CLIENT;

	/* invalid client id out of range of supported configurable sub-clients */
	if (offset >= drv_data->hw_fence_client_types[client_type].clients_num)
		return HW_FENCE_CLIENT_MAX;

	client_id_priv = HW_FENCE_MAX_STATIC_CLIENTS_INDEX + offset;

	for (i = HW_FENCE_MAX_CLIENT_TYPE_STATIC; i < client_type; i++)
		client_id_priv += drv_data->hw_fence_client_types[i].clients_num;

	return client_id_priv;
}

int hw_fence_utils_get_queues_num(struct hw_fence_driver_data *drv_data, int client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num ||
			!drv_data->hw_fence_client_queue_size[client_id].type) {
		HWFNC_ERR("invalid access to client:%d queues_num\n", client_id);
		return 0;
	}

	return drv_data->hw_fence_client_queue_size[client_id].type->queues_num;
}

int hw_fence_utils_get_skip_fctl_ref(struct hw_fence_driver_data *drv_data, int client_id)
{
	if (!drv_data || client_id >= drv_data->clients_num ||
			!drv_data->hw_fence_client_queue_size[client_id].type) {
		HWFNC_ERR("invalid access to client:%d skip_fctl_ref\n", client_id);
		return 0;
	}

	return drv_data->hw_fence_client_queue_size[client_id].type->skip_fctl_ref;
}
