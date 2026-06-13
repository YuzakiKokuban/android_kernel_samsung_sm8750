/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_SUBDEV_H_
#define _CAM_SUBDEV_H_

#include <linux/types.h>
#include <linux/platform_device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>

#define CAM_SUBDEVICE_EVENT_MAX 30


/* Enum for indicating CSID event */
enum cam_subdev_phy_csid_state {
	CAM_SUBDEV_PHY_CSID_HALT = 1,
	CAM_SUBDEV_PHY_CSID_RESUME,
};

/**
 * struct cam_subdev_msg_phy_conn_csid_info: Contains phy num and connected CSID info
 *
 * @phy_idx:          Phy idx value indicating which phy is connected to csid core
 * @lane_cfg:         This value is similar to lane_assign in the PHY
 *                    driver, and is used to identify the particular
 *                    PHY instance with which this IFE session is
 *                    connected to.
 * @core_idx:         Primary(in case of dual ife) core idx for the csid to which the phy
 *                    is connected, -1 while disconnecting
 */
struct cam_subdev_msg_phy_conn_csid_info {
	uint32_t phy_idx;
	uint32_t lane_cfg;
	int32_t core_idx;
};

/**
 * struct cam_subdev_msg_phy_drv_info: Contains drv config info
 *
 * @phy_idx:               Phy idx value indicating which phy is connected to csid core
 * @lane_cfg:              This value is similar to lane_assign in the PHY
 *                         driver, and is used to identify the particular
 *                         PHY instance with which this IFE session is
 *                         connected to.
 * @use_hw_client_voting:  Whether to use hw client voting for csiphy clk
 * @is_drv_config_en:      If DRV config is enabled in CSID
 */
struct cam_subdev_msg_phy_drv_info {
	uint32_t phy_idx;
	uint32_t lane_cfg;
	bool     use_hw_client_voting;
	bool     is_drv_config_en;
};

/**
 * struct cam_subdev_msg_phy_halt_resume_info: Contains csid halt resume info
 *
 * @phy_idx:     Phy idx value indicating which phy is connected to csid core
 * @lane_cfg:    This value is similar to lane_assign in the PHY
 *               driver, and is used to identify the particular
 *               PHY instance with which this IFE session is
 *               connected to.
 * @csid_state:  Notification of CSID state
 * @do_drv_ops:  Only if set perform the drv related ops
 * @reset_phy:   Reset PHY
 */
struct cam_subdev_msg_phy_halt_resume_info {
	uint32_t                       phy_idx;
	uint32_t                       lane_cfg;
	enum cam_subdev_phy_csid_state csid_state;
	bool                           do_drv_ops;
	bool                           reset_resume_phy;
};

/**
 * struct cam_subdev_msg_cdr_sweep_info: Contains all relevant info wrt
 *                                       CSID register values needed as
 *                                       part of CSIPHY CDR tuning
 *
 * @phy_idx:                 Phy idx value indicating which phy is connected to csid core
 * @lane_cfg:                This value is similar to lane_assign in the PHY
 *                           driver, and is used to identify the particular
 *                           PHY instance with which this IFE session is
 *                           connected to
 * @csi2_rx_status:          Rx irq status register value
 * @csi2_rx_total_crc_err:   Rx total crc err register value
 * @csi2_rx_total_pkts_rcvd: Rx total pkts rcvd register value
 * @csi2_err_seen:           If CSI RX has an error
 * @epd_enabled:             If sensor streaming on this CSI is
 *                           EPD enabled
 */
struct cam_subdev_msg_cdr_sweep_info {
	uint32_t phy_idx;
	uint32_t lane_cfg;
	uint32_t csi2_rx_status;
	uint32_t csi2_rx_total_crc_err;
	uint32_t csi2_rx_total_pkts_rcvd;
	bool     csi2_err_seen;
	bool     epd_enabled;
};

enum cam_subdev_message_type_t {
	CAM_SUBDEV_MESSAGE_REG_DUMP = 0x1,
	CAM_SUBDEV_MESSAGE_APPLY_CSIPHY_AUX,
	CAM_SUBDEV_MESSAGE_DOMAIN_ID_SECURE_PARAMS,
	CAM_SUBDEV_MESSAGE_CONN_CSID_INFO,
	CAM_SUBDEV_MESSAGE_DRV_INFO,
	CAM_SUBDEV_MESSAGE_NOTIFY_HALT_RESUME,
	CAM_SUBDEV_MESSAGE_CLOCK_UPDATE,
	CAM_SUBDEV_MESSAGE_CDR_SWEEP
};

/* Enum for close sequence priority */
enum cam_subdev_close_seq_priority {
	CAM_SD_CLOSE_HIGH_PRIORITY,
	CAM_SD_CLOSE_MEDIUM_PRIORITY,
	CAM_SD_CLOSE_MEDIUM_LOW_PRIORITY,
	CAM_SD_CLOSE_LOW_PRIORITY
};

enum cam_subdev_rwsem {
	CAM_SUBDEV_LOCK = 1,
	CAM_SUBDEV_UNLOCK,
};

/**
 * struct cam_subdev - describes a camera sub-device
 *
 * @pdev:                  Pointer to the platform device
 * @sd:                    V4l2 subdevice
 * @ops:                   V4l2 subdecie operations
 * @internal_ops:          V4l2 subdevice internal operations
 * @name:                  Name of the sub-device. Please notice that the name
 *                             must be unique.
 * @sd_flags:              Subdev flags. Can be:
 *                             %V4L2_SUBDEV_FL_HAS_DEVNODE - Set this flag if
 *                                 this subdev needs a device node.
 *                             %V4L2_SUBDEV_FL_HAS_EVENTS -  Set this flag if
 *                                 this subdev generates events.
 * @token:                 Pointer to cookie of the client driver
 * @ent_function:          Media entity function type. Can be:
 *                             %CAM_IFE_DEVICE_TYPE - identifies as IFE device.
 *                             %CAM_ICP_DEVICE_TYPE - identifies as ICP device.
 * @list:                  list pointer
 * @close_seq_prior:         cam_subdev_close_seq_priority type
 *
 * Each instance of a subdev driver should create this struct, either
 * stand-alone or embedded in a larger struct. This structure should be
 * initialized/registered by cam_register_subdev
 *
 */
struct cam_subdev {
	struct platform_device                *pdev;
	struct v4l2_subdev                     sd;
	const struct v4l2_subdev_ops          *ops;
	const struct v4l2_subdev_internal_ops *internal_ops;
	char                                  *name;
	u32                                    sd_flags;
	void                                  *token;
	u32                                    ent_function;
	void                                  (*msg_cb)(
					struct v4l2_subdev *sd,
					enum cam_subdev_message_type_t msg_type,
					void *data);
	struct list_head                       list;
	enum cam_subdev_close_seq_priority     close_seq_prior;
};

/**
 * cam_subdev_notify_message()
 *
 * @brief:  Notify message to a subdevs of specific type
 *
 * @subdev_type:           Subdev type
 * @message_type:          message type
 * @data:                  data to be delivered.
 *
 */
void cam_subdev_notify_message(u32 subdev_type,
		enum cam_subdev_message_type_t message_type,
		void *data);

/**
 * cam_subdev_probe()
 *
 * @brief:      Camera Subdevice node probe function for v4l2 setup
 *
 * @sd:                    Camera subdevice object
 * @name:                  Name of the subdevice node
 * @dev_type:              Subdevice node type
 *
 */
int cam_subdev_probe(struct cam_subdev *sd, struct platform_device *pdev,
	char *name, uint32_t dev_type);

/**
 * cam_subdev_remove()
 *
 * @brief:      Called when subdevice node is unloaded
 *
 * @sd:                    Camera subdevice node object
 *
 */
int cam_subdev_remove(struct cam_subdev *sd);

/**
 * cam_register_subdev()
 *
 * @brief:   This is the common utility function to be called by each camera
 *           subdevice node when it tries to register itself to the camera
 *           request manager
 *
 * @sd:                    Pointer to struct cam_subdev.
 */
int cam_register_subdev(struct cam_subdev *sd);

/**
 * cam_unregister_subdev()
 *
 * @brief:    This is the common utility function to be called by each camera
 *            subdevice node when it tries to unregister itself from the
 *            camera request manger
 *
 * @sd:                    Pointer to struct cam_subdev.
 */
int cam_unregister_subdev(struct cam_subdev *sd);

/**
 * cam_req_mgr_rwsem_read_op()
 *
 * @brief : API to acquire read semaphore lock to platform framework.
 *
 * @lock  : value indicates to lock or unlock the read lock
 */
void cam_req_mgr_rwsem_read_op(enum cam_subdev_rwsem lock);

/**
 * cam_req_mgr_is_open()
 *
 * @brief:    This common utility function returns the crm active status
 *
 */
bool  cam_req_mgr_is_open(void);

/**
 * cam_req_mgr_is_shutdown()
 *
 * @brief:    This common utility function returns the shutdown state
 */
bool cam_req_mgr_is_shutdown(void);

#endif /* _CAM_SUBDEV_H_ */
