// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/interconnect.h>
#include <dt-bindings/interconnect/qcom,icc.h>
#include "cam_soc_bus.h"
#include "cam_compat.h"
#include "cam_soc_util.h"
#include "cam_vmrm_interface.h"
#include "cam_mem_mgr_api.h"

extern bool clk_rgltr_bus_ops_profiling;

static inline struct icc_path *cam_wrapper_icc_get(struct device *dev,
	const int src_id, const int dst_id, const char *name, bool use_path_name)
{
	struct timespec64 ts1, ts2;
	long usec = 0;
	struct icc_path *temp;

	if (cam_vmrm_proxy_icc_voting_enable() || (debug_bypass_drivers & CAM_BYPASS_ICC)) {
		CAM_WARN(CAM_UTIL, "Bypass icc get for %d %d", src_id, dst_id);
		return (struct icc_path *)BYPASS_VALUE;
	}

	CAM_SAVE_START_TIMESTAMP_IF(ts1);

	temp = cam_icc_get_path(dev, src_id, dst_id, name, use_path_name);

	CAM_COMPUTE_TIME_TAKEN_IF(ts1, ts2, usec,
		"ClkRegBusOpsProfile", "icc_get (time taken in usec)");

	return temp;
}

static inline void cam_wrapper_icc_put(struct icc_path *path)
{
	struct timespec64 ts1, ts2;
	long usec = 0;

	if (cam_vmrm_proxy_icc_voting_enable() || (debug_bypass_drivers & CAM_BYPASS_ICC)) {
		CAM_WARN(CAM_UTIL, "Bypass icc put");
		return;
	}

	CAM_SAVE_START_TIMESTAMP_IF(ts1);

	icc_put(path);

	CAM_COMPUTE_TIME_TAKEN_IF(ts1, ts2, usec,
		"ClkRegBusOpsProfile", "icc_put (time taken in usec)");
}

static inline int cam_wrapper_icc_set_bw(struct icc_path *path,
	u32 avg_bw, u32 peak_bw)
{
	struct timespec64 ts1, ts2;
	long usec = 0;
	int temp;

	if (cam_vmrm_proxy_icc_voting_enable() || (debug_bypass_drivers & CAM_BYPASS_ICC)) {
		CAM_WARN(CAM_UTIL, "Bypass icc set bw");
		return 0;
	}

	CAM_SAVE_START_TIMESTAMP_IF(ts1);

	temp = icc_set_bw(path, avg_bw, peak_bw);

	CAM_COMPUTE_TIME_TAKEN_IF(ts1, ts2, usec,
		"ClkRegBusOpsProfile", "icc_set_bw (time taken in usec)");

	return temp;
}

static inline void cam_wrapper_icc_set_tag(struct icc_path *path,
	u32 tag)
{
	struct timespec64 ts1, ts2;
	long usec = 0;

	if (cam_vmrm_proxy_icc_voting_enable() || (debug_bypass_drivers & CAM_BYPASS_ICC)) {
		CAM_WARN(CAM_UTIL, "Bypass icc set tag");
		return;
	}

	CAM_SAVE_START_TIMESTAMP_IF(ts1);

	icc_set_tag(path, tag);

	CAM_COMPUTE_TIME_TAKEN_IF(ts1, ts2, usec,
		"ClkRegBusOpsProfile", "icc_set_tag (time taken in usec)");
}

/**
 * struct cam_soc_bus_client_data : Bus client data
 *
 * @icc_data: Bus icc path information
 */
struct cam_soc_bus_client_data {
	struct icc_path *icc_data[CAM_SOC_BUS_PATH_DATA_MAX];
};

const char *cam_soc_bus_path_data_to_str(enum cam_soc_bus_path_data bus_path_data)
{
	switch (bus_path_data) {
	case CAM_SOC_BUS_PATH_DATA_HLOS:
		return "BUS_PATH_HLOS";
	case CAM_SOC_BUS_PATH_DATA_DRV_HIGH:
		return "BUS_PATH_DRV_HIGH";
	case CAM_SOC_BUS_PATH_DATA_DRV_LOW:
		return "BUS_PATH_DRV_LOW";
	default:
		return "BUS_PATH_INVALID";
	}
}
int cam_soc_bus_client_update_request(void *client, unsigned int idx)
{
	int rc = 0;
	uint64_t ab = 0, ib = 0;
	struct cam_soc_bus_client *bus_client =
		(struct cam_soc_bus_client *) client;
	struct cam_soc_bus_client_data *bus_client_data =
		(struct cam_soc_bus_client_data *) bus_client->client_data;

	if (debug_bypass_drivers & CAM_BYPASS_ICC) {
		CAM_WARN(CAM_UTIL, "Bypass icc set bw");
		return rc;
	}

	if (idx >= bus_client->common_data->num_usecases) {
		CAM_ERR(CAM_UTIL, "Invalid vote level=%d, usecases=%d", idx,
			bus_client->common_data->num_usecases);
		rc = -EINVAL;
		goto end;
	}

	ab = bus_client->common_data->bw_pair[idx].ab;
	ib = bus_client->common_data->bw_pair[idx].ib;

	CAM_DBG(CAM_PERF, "Bus client=[%s] index[%d] ab[%llu] ib[%llu]",
		bus_client->common_data->name, idx, ab, ib);

	if (cam_vmrm_proxy_icc_voting_enable()) {
		rc = cam_vmrm_icc_vote(bus_client->common_data->name, ab, ib);
		if (rc)
			CAM_ERR(CAM_PERF,
				"Bus client=[%s] index[%d] ab[%llu] ib[%llu] vmrm icc vote failed",
				bus_client->common_data->name, idx, ab, ib);
		goto end;
	}

	rc = cam_wrapper_icc_set_bw(
		bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_HLOS],
		Bps_to_icc(ab),
		Bps_to_icc(ib));
	if (rc) {
		CAM_ERR(CAM_UTIL,
			"Update request failed, client[%s], idx: %d",
			bus_client->common_data->name, idx);
		goto end;
	}

end:
	return rc;
}

int cam_soc_bus_client_update_bw(void *client, uint64_t ab, uint64_t ib,
	enum cam_soc_bus_path_data bus_path_data)
{
	struct cam_soc_bus_client *bus_client =
		(struct cam_soc_bus_client *) client;
	struct cam_soc_bus_client_data *bus_client_data =
		(struct cam_soc_bus_client_data *) bus_client->client_data;
	int rc = 0;

	CAM_DBG(CAM_PERF, "Bus client=[%s] [%s] :ab[%llu] ib[%llu]",
		bus_client->common_data->name, cam_soc_bus_path_data_to_str(bus_path_data),
		ab, ib);

	if (cam_vmrm_proxy_icc_voting_enable()) {
		rc = cam_vmrm_icc_vote(bus_client->common_data->name, ab, ib);
		if (rc)
			CAM_ERR(CAM_PERF,
				"Bus client=[%s] [%s] :ab[%llu] ib[%llu] vmrm icc vote failed",
				bus_client->common_data->name,
				cam_soc_bus_path_data_to_str(bus_path_data), ab, ib);
		goto end;
	}

	rc = cam_wrapper_icc_set_bw(
		bus_client_data->icc_data[bus_path_data], Bps_to_icc(ab),
		Bps_to_icc(ib));
	if (rc) {
		CAM_ERR(CAM_UTIL, "Update request failed, client[%s]",
			bus_client->common_data->name);
		goto end;
	}

end:
	return rc;

}

int cam_soc_bus_client_register(struct platform_device *pdev,
	struct device_node *dev_node, void **client,
	struct cam_soc_bus_client_common_data *common_data, bool use_path_name)
{
	struct cam_soc_bus_client *bus_client = NULL;
	struct cam_soc_bus_client_data *bus_client_data = NULL;
	int rc = 0;

	bus_client = CAM_MEM_ZALLOC(sizeof(struct cam_soc_bus_client), GFP_KERNEL);
	if (!bus_client) {
		CAM_ERR(CAM_UTIL, "soc bus client is NULL");
		rc = -ENOMEM;
		goto end;
	}

	*client = bus_client;

	bus_client_data = CAM_MEM_ZALLOC(sizeof(struct cam_soc_bus_client_data), GFP_KERNEL);
	if (!bus_client_data) {
		CAM_MEM_FREE(bus_client);
		*client = NULL;
		rc = -ENOMEM;
		goto end;
	}

	bus_client->client_data = bus_client_data;
	bus_client->common_data = common_data;
	if (bus_client->common_data->is_drv_port) {
		bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_HIGH] =
			cam_wrapper_icc_get(&pdev->dev,
				bus_client->common_data->src_id, bus_client->common_data->dst_id,
				bus_client->common_data->name, use_path_name);
		if (IS_ERR_OR_NULL(bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_HIGH])) {
			CAM_ERR(CAM_UTIL,
				"Failed to register DRV bus client Bus Client=[%s] : src=%d, dst=%d bus_path:%d",
				bus_client->common_data->src_id, bus_client->common_data->dst_id,
				CAM_SOC_BUS_PATH_DATA_DRV_HIGH);
			rc = -EINVAL;
			goto error;
		}

		bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_LOW] =
		cam_wrapper_icc_get(&pdev->dev,
			bus_client->common_data->src_id, bus_client->common_data->dst_id,
			bus_client->common_data->name, use_path_name);
		if (IS_ERR_OR_NULL(bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_LOW])) {
			CAM_ERR(CAM_UTIL,
				"Failed to register DRV bus client Bus Client=[%s] : src=%d, dst=%d bus_path:%d",
				bus_client->common_data->src_id, bus_client->common_data->dst_id,
				CAM_SOC_BUS_PATH_DATA_DRV_LOW);
			rc = -EINVAL;
			goto error;
		}

		/* Set appropriate tags for HIGH and LOW vote paths */
		cam_wrapper_icc_set_tag(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_HIGH],
			QCOM_ICC_TAG_ACTIVE_ONLY);
		cam_wrapper_icc_set_tag(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_LOW],
			QCOM_ICC_TAG_SLEEP);

		rc = cam_wrapper_icc_set_bw(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_HIGH], 0, 0);
		if (rc) {
			CAM_ERR(CAM_UTIL, "Bus client[%s] update request failed, rc = %d",
				bus_client->common_data->name, rc);
			goto fail_unregister_client;
		}

		rc = cam_wrapper_icc_set_bw(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_LOW], 0, 0);
		if (rc) {
			CAM_ERR(CAM_UTIL, "Bus client[%s] update request failed, rc = %d",
				bus_client->common_data->name, rc);
			goto fail_unregister_client;
		}
	} else {
		bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_HLOS] =
			cam_wrapper_icc_get(&pdev->dev,
				bus_client->common_data->src_id, bus_client->common_data->dst_id,
				bus_client->common_data->name, use_path_name);
		if (IS_ERR_OR_NULL(bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_HLOS])) {
			CAM_ERR(CAM_UTIL, "failed to register HLOS bus client");
			rc = -EINVAL;
			goto error;
		}

		rc = cam_wrapper_icc_set_bw(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_HLOS], 0, 0);
		if (rc) {
			CAM_ERR(CAM_UTIL, "Bus client[%s] update request failed, rc = %d",
				bus_client->common_data->name, rc);
			goto fail_unregister_client;
		}
	}

	CAM_DBG(CAM_PERF, "Register Bus Client=[%s] : src=%d, dst=%d is_drv_port:%s",
		bus_client->common_data->name, bus_client->common_data->src_id,
		bus_client->common_data->dst_id,
		CAM_BOOL_TO_YESNO(bus_client->common_data->is_drv_port));

	return 0;

fail_unregister_client:
	if (bus_client->common_data->is_drv_port) {
		cam_wrapper_icc_put(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_HIGH]);
		cam_wrapper_icc_put(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_LOW]);
	} else {
		cam_wrapper_icc_put(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_HLOS]);
	}

error:
	CAM_MEM_FREE(bus_client_data);
	bus_client->client_data = NULL;
	CAM_MEM_FREE(bus_client);
	*client = NULL;
end:
	return rc;

}

void cam_soc_bus_client_unregister(void **client)
{
	struct cam_soc_bus_client *bus_client =
		(struct cam_soc_bus_client *) (*client);
	struct cam_soc_bus_client_data *bus_client_data =
		(struct cam_soc_bus_client_data *) bus_client->client_data;

	if (bus_client->common_data->is_drv_port) {
		cam_wrapper_icc_put(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_HIGH]);
		cam_wrapper_icc_put(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_DRV_LOW]);
	} else {
		cam_wrapper_icc_put(
			bus_client_data->icc_data[CAM_SOC_BUS_PATH_DATA_HLOS]);
	}

	CAM_MEM_FREE(bus_client_data);
	bus_client->client_data = NULL;
	CAM_MEM_FREE(bus_client);
	*client = NULL;

}
