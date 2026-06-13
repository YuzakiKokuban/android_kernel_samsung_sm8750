// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>

#include "camera_main.h"
#include "cam_debug_util.h"
#include "cam_hw.h"
#include "cam_hw_intf.h"
#include "cam_icp_hw_intf.h"
#include "cam_icp_v2_core.h"
#include "cam_icp_soc_common.h"
#include "cam_mem_mgr_api.h"
#include "cam_req_mgr_dev.h"

static int max_icp_v2_hw_idx = -1;
static struct cam_icp_hw_intf_data cam_icp_hw_list[CAM_ICP_HW_NUM_MAX];

struct cam_icp_v2_hw_info cam_icp_v2_hw_info[] = {
	{
		.ob_irq_status = 0xC,
		.ob_irq_mask   = 0x0,
		.ob_irq_clear  = 0x4,
		.ob_irq_set    = 0x8,
		.ob_irq_cmd    = 0x10,
		.host2icpint   = 0x124,
		.pfault_info   = 0x128,
	},
};

struct cam_icp_v2_hw_info cam_icp_v2_1_hw_info[] = {
	{
		.ob_irq_status = 0x20C,
		.ob_irq_mask   = 0x200,
		.ob_irq_clear  = 0x204,
		.ob_irq_set    = 0x208,
		.ob_irq_cmd    = 0x210,
		.host2icpint   = 0x300,
		.pfault_info   = 0x400,
	},
};

uint32_t cam_icp_v2_get_device_num(void)
{
	return max_icp_v2_hw_idx + 1;
}

static int cam_icp_v2_soc_info_init(struct cam_hw_soc_info *soc_info,
	struct platform_device *pdev)
{
	struct cam_icp_soc_info *icp_soc_info = NULL;

	icp_soc_info = CAM_MEM_ZALLOC(sizeof(*icp_soc_info), GFP_KERNEL);
	if (!icp_soc_info)
		return -ENOMEM;

	soc_info->pdev = pdev;
	soc_info->dev = &pdev->dev;
	soc_info->dev_name = pdev->name;
	soc_info->soc_private = icp_soc_info;

	icp_soc_info->dev_type = CAM_ICP_HW_ICP_V2;

	return 0;
}

static inline void cam_icp_v2_soc_info_deinit(struct cam_hw_soc_info *soc_info)
{
	struct cam_icp_soc_info *icp_soc_info = soc_info->soc_private;

	if (icp_soc_info->pid) {
		CAM_MEM_FREE(icp_soc_info->pid);
		icp_soc_info->pid = NULL;
	}

	CAM_MEM_FREE(soc_info->soc_private);
	soc_info->soc_private = NULL;
}

static int cam_icp_v2_component_bind(struct device *dev,
	struct device *mdev, void *data)
{
	int rc = 0, i;
	struct cam_hw_intf *icp_v2_intf = NULL;
	struct cam_hw_info *icp_v2_info = NULL;
	struct cam_icp_v2_core_info *core_info = NULL;
	const struct of_device_id *match_dev = NULL;
	struct platform_device *pdev = to_platform_device(dev);
	struct timespec64 ts_start, ts_end;
	long microsec = 0;
	struct cam_icp_soc_info *icp_soc_info;

	CAM_GET_TIMESTAMP(ts_start);
	match_dev = of_match_device(
		pdev->dev.driver->of_match_table, &pdev->dev);
	if (!match_dev) {
		CAM_DBG(CAM_ICP, "No ICP v2 hardware info");
		return -EINVAL;
	}

	icp_v2_intf = CAM_MEM_ZALLOC(sizeof(*icp_v2_intf), GFP_KERNEL);
	if (!icp_v2_intf)
		return -ENOMEM;

	icp_v2_info = CAM_MEM_ZALLOC(sizeof(*icp_v2_info), GFP_KERNEL);
	if (!icp_v2_info) {
		rc = -ENOMEM;
		goto free_hw_intf;
	}

	core_info = CAM_MEM_ZALLOC(sizeof(*core_info), GFP_KERNEL);
	if (!core_info) {
		rc = -ENOMEM;
		goto free_hw_info;
	}

	core_info->hw_info = (struct cam_icp_v2_hw_info *)match_dev->data;
	icp_v2_info->core_info = core_info;

	rc = cam_icp_v2_soc_info_init(&icp_v2_info->soc_info, pdev);
	if (rc)
		goto free_core_info;

	mutex_init(&icp_v2_info->hw_mutex);
	spin_lock_init(&icp_v2_info->hw_lock);
	init_completion(&icp_v2_info->hw_complete);

	rc = cam_icp_soc_resources_init(&icp_v2_info->soc_info,
		cam_icp_v2_handle_irq, icp_v2_info);
	if (rc) {
		CAM_ERR(CAM_ICP, "soc resources init failed rc=%d", rc);
		goto free_soc_info;
	}

	rc = cam_icp_v2_core_init(&icp_v2_info->soc_info, core_info);
	if (rc)
		goto free_soc_info;

	icp_v2_intf->hw_priv = icp_v2_info;
	icp_v2_intf->hw_type = CAM_ICP_HW_ICP_V2;
	icp_v2_intf->hw_idx = icp_v2_info->soc_info.index;
	icp_v2_intf->hw_ops.init = cam_icp_v2_hw_init;
	icp_v2_intf->hw_ops.deinit = cam_icp_v2_hw_deinit;
	icp_v2_intf->hw_ops.process_cmd = cam_icp_v2_process_cmd;
	icp_v2_intf->hw_ops.test_irq_line = cam_icp_v2_test_irq_line;

	icp_v2_info->soc_info.hw_id = CAM_HW_ID_ICP + icp_v2_info->soc_info.index;
	rc = cam_vmvm_populate_hw_instance_info(&icp_v2_info->soc_info, NULL, NULL);
	if (rc) {
		CAM_ERR(CAM_ICP, " hw instance populate failed: %d", rc);
		goto res_deinit;
	}

	rc = cam_icp_v2_cpas_register(icp_v2_intf);
	if (rc) {
		CAM_ERR(CAM_ICP, "cpas registration failed rc=%d", rc);
		goto res_deinit;
	}

	if ((int)(icp_v2_intf->hw_idx) > max_icp_v2_hw_idx)
		max_icp_v2_hw_idx = icp_v2_intf->hw_idx;

	icp_soc_info = icp_v2_info->soc_info.soc_private;
	cam_icp_hw_list[icp_v2_intf->hw_idx].pid =
		CAM_MEM_ZALLOC_ARRAY(icp_soc_info->num_pid, sizeof(uint32_t), GFP_KERNEL);
	if (!cam_icp_hw_list[icp_v2_intf->hw_idx].pid) {
		CAM_ERR(CAM_ICP, "Failed at allocating memory for icp hw list");
		rc = -ENOMEM;
		goto res_deinit;
	}

	cam_icp_hw_list[icp_v2_intf->hw_idx].hw_intf = icp_v2_intf;
	cam_icp_hw_list[icp_v2_intf->hw_idx].num_pid = icp_soc_info->num_pid;
	for (i = 0; i < icp_soc_info->num_pid; i++)
		cam_icp_hw_list[icp_v2_intf->hw_idx].pid[i] = icp_soc_info->pid[i];

	platform_set_drvdata(pdev, &cam_icp_hw_list[icp_v2_intf->hw_idx]);
	CAM_GET_TIMESTAMP(ts_end);
	CAM_GET_TIMESTAMP_DIFF_IN_MICRO(ts_start, ts_end, microsec);
	cam_record_bind_latency(pdev->name, microsec);

	return 0;

res_deinit:
	cam_icp_soc_resources_deinit(&icp_v2_info->soc_info);
free_soc_info:
	cam_icp_v2_soc_info_deinit(&icp_v2_info->soc_info);
free_core_info:
	CAM_MEM_FREE(core_info);
free_hw_info:
	CAM_MEM_FREE(icp_v2_info);
free_hw_intf:
	CAM_MEM_FREE(icp_v2_intf);

	return rc;
}

static void cam_icp_v2_component_unbind(struct device *dev,
	struct device *mdev, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct cam_hw_intf *icp_v2_intf = NULL;
	struct cam_hw_info *icp_v2_info = NULL;
	struct cam_icp_hw_intf_data *icp_intf_data;

	icp_intf_data = platform_get_drvdata(pdev);
	if (!icp_intf_data) {
		CAM_ERR(CAM_ICP, "Error No data in pdev");
		return;
	}

	icp_v2_intf = icp_intf_data->hw_intf;
	if (!icp_v2_intf) {
		CAM_ERR(CAM_ICP, "Error No ICP dev interface");
		return;
	}

	icp_v2_info = icp_v2_intf->hw_priv;

	cam_icp_v2_cpas_unregister(icp_v2_intf);
	cam_icp_soc_resources_deinit(&icp_v2_info->soc_info);
	if (icp_intf_data->pid)
		CAM_MEM_FREE(icp_intf_data->pid);
	cam_icp_v2_soc_info_deinit(&icp_v2_info->soc_info);

	max_icp_v2_hw_idx = -1;

	CAM_MEM_FREE(icp_v2_info->core_info);
	CAM_MEM_FREE(icp_v2_info);
	CAM_MEM_FREE(icp_v2_intf);
}

static const struct component_ops cam_icp_v2_component_ops = {
	.bind = cam_icp_v2_component_bind,
	.unbind = cam_icp_v2_component_unbind,
};

static const struct of_device_id cam_icp_v2_match[] = {
	{
		.compatible = "qcom,cam-icp_v2",
		.data = &cam_icp_v2_hw_info,
	},
	{
		.compatible = "qcom,cam-icp_v2_1",
		.data = &cam_icp_v2_1_hw_info,
	},
	{}
};
MODULE_DEVICE_TABLE(of, cam_icp_v2_match);

static int cam_icp_v2_driver_probe(struct platform_device *pdev)
{
	int rc;

	rc = component_add(&pdev->dev, &cam_icp_v2_component_ops);
	if (rc)
		CAM_ERR(CAM_ICP, "cam-icp_v2 component add failed rc=%d", rc);

	return rc;
}

static int cam_icp_v2_driver_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &cam_icp_v2_component_ops);

	return 0;
}

struct platform_driver cam_icp_v2_driver = {
	.probe = cam_icp_v2_driver_probe,
	.remove = cam_icp_v2_driver_remove,
	.driver = {
		.name = "cam-icp_v2",
		.of_match_table = cam_icp_v2_match,
		.suppress_bind_attrs = true,
	},
};

int cam_icp_v2_init_module(void)
{
	return platform_driver_register(&cam_icp_v2_driver);
}

void cam_icp_v2_exit_module(void)
{
	platform_driver_unregister(&cam_icp_v2_driver);
}

MODULE_DESCRIPTION("Camera ICP_V2 driver");
MODULE_LICENSE("GPL v2");
