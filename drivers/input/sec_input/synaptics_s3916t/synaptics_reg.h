/* SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_SYNAPTICS_TS_REG_H_
#define _LINUX_SYNAPTICS_TS_REG_H_

/* drivers/input/sec_input/stm/stm_reg.h
 *
 * Copyright (C) 2020 Samsung Electronics Co., Ltd.
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define USE_POR_AFTER_I2C_RETRY

#define BRUSH_Z_DATA			63 /* for ArtCanvas */

#define TOUCH_PRINT_INFO_DWORK_TIME	30000
#define TOUCH_RESET_DWORK_TIME		10
#define TOUCH_POWER_ON_DWORK_TIME	70

#define SYNA_TCM_CORE_LIB_VERSION 0x010B

#define FIRMWARE_IC			"SYNAPTICS"
#define SYNAPTICS_TS_MAX_FW_PATH			64
#define SYNAPTICS_TS_TS_DRV_NAME			"SYNAPTICS_TS_TOUCH"
#define SYNAPTICS_TS_TS_DRV_VERSION		"0100"


	/**
	 * @section: Data Comparison helpers
	 *
	 * @brief: MAX
	 *	   Find the maximum value between
	 *
	 * @brief: MIN:
	 *	   Find the minimum value between
	 *
	 * @brief: GET_BIT
	 *	   Return the value of target bit
	 */
#define MAX(a, b) \
		({__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a > _b ? _a : _b; })

#define MIN(a, b) \
		({__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a < _b ? _a : _b; })

#define GET_BIT(var, pos) \
		(((var) & (1 << (pos))) >> (pos))
#define GET_BIT_LSB(var, pos) \
		(((var) & (1 << (pos))) >> (pos))
#define GET_BIT_MSB(var, pos) \
		(((var) & (1 << (7 - pos))) >> (7 - pos))

	/**
	 * @section: C Atomic operations
	 *
	 * @brief: ATOMIC_SET
	 *	   Set an atomic data
	 *
	 * @brief: ATOMIC_GET:
	 *	   Get an atomic data
	 */
	typedef atomic_t syna_pal_atomic_t;

#define ATOMIC_SET(atomic, value) \
		atomic_set(&atomic, value)

#define ATOMIC_GET(atomic) \
		atomic_read(&atomic)


/**
 * @section: Parameters pre-defined
 *
 * @brief: MAX_NUM_OBJECTS
 *         Maximum number of objects being detected
 *
 * @brief: MAX_SIZE_GESTURE_DATA
 *         Maximum size of gesture data
 *
 * @brief: MAX_SIZE_CONFIG_ID
 *         Maximum size of customer configuration ID
 */
#define MAX_NUM_OBJECTS (10)

#define MAX_SIZE_GESTURE_DATA (8)

#define MAX_SIZE_CONFIG_ID (16)

/**
 * @section: Command-handling relevant definitions
 *
 * @brief: MESSAGE_HEADER_SIZE
 *         The size of message header
 *
 * @brief: CMD_RESPONSE_TIMEOUT_MS
 *         Time frame for a command execution
 *
 * @brief: CMD_RESPONSE_POLLING_DELAY_MS
 *         Generic time frame to check the response in polling
 *
 * @brief: RD_RETRY_US
 *         For retry reading, delay time range in microsecond
 *
 * @brief: WR_DELAY_US
 *         For continued writes, delay time range in microsecond
 *
 * @brief: TAT_DELAY_US
 *         For bus turn-around, delay time range in microsecond
 *
 * @brief: FW_MODE_SWITCH_DELAY_MS
 *         The default time for fw mode switching
 *
 * @brief: RESET_DELAY_MS
 *         The default time after reset in case it's not set properly
 *
 * @brief: FORCE_ATTN_DRIVEN
 *         Special flag to read in resp packet in ISR function
 */
#define SYNAPTICS_TS_MESSAGE_HEADER_SIZE (4)

#define CMD_RESPONSE_TIMEOUT_MS		(5000)
#define CMD_RESPONSE_SHORT_TIMEOUT_MS	(1000)

#define CMD_RESPONSE_POLLING_DELAY_MS (10)

#define RD_RETRY_US_MIN (5000)
#define RD_RETRY_US_MAX (10000)

#define WR_DELAY_US_MIN (1000)//(500)
#define WR_DELAY_US_MAX (1000)

#define MSG_DELAY_US_MIN (100)//(50)
#define MSG_DELAY_US_MAX (100)

#define FW_MODE_SWITCH_DELAY_MS (200)

#define RESET_DELAY_MS (300)

#define FORCE_ATTN_DRIVEN (~0)


/**
 * @section: Macro to show string in log
 */
#define STR(x) #x

/**
 * @section: Helpers to check the device mode
 */
#define IS_APP_FW_MODE(mode) \
	(mode == SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE)

#define IS_NOT_APP_FW_MODE(mode) \
	(!IS_APP_FW_MODE(mode))

#define IS_BOOTLOADER_MODE(mode) \
	((mode == SYNAPTICS_TS_MODE_BOOTLOADER) || \
	(mode == SYNAPTICS_TS_MODE_TDDI_BOOTLOADER)  || \
	(mode == SYNAPTICS_TS_MODE_TDDI_HDL_BOOTLOADER) || \
	(mode == SYNAPTICS_TS_MODE_MULTICHIP_TDDI_BOOTLOADER))

#define IS_ROM_BOOTLOADER_MODE(mode) \
	(mode == SYNAPTICS_TS_MODE_ROMBOOTLOADER)

#define SYNAPTICS_TS_BITS_IN_MESSAGE_HEADER (SYNAPTICS_TS_MESSAGE_HEADER_SIZE * 8)

#define HOST_PRIMARY (0)

#define COMMAND_RETRY_TIMES (5)


/**
 * @section: Types for lower-level bus being used
 */
enum bus_connection {
	BUS_TYPE_NONE,
	BUS_TYPE_I2C,
	BUS_TYPE_SPI,
	BUS_TYPE_I3C,
};

#define SYNAPTICS_TS_V1_MESSAGE_MARKER 0xa5
#define SYNAPTICS_TS_V1_MESSAGE_PADDING 0x5a


/**
 * @section: TouchComm Firmware Modes
 *
 * The current mode running is defined in Identify Info Packet.
 */
enum synaptics_ts_firmware_mode {
	SYNAPTICS_TS_MODE_UNKNOWN = 0x00,
	SYNAPTICS_TS_MODE_APPLICATION_FIRMWARE = 0x01,
	SYNAPTICS_TS_MODE_HOSTDOWNLOAD_FIRMWARE = 0x02,
	SYNAPTICS_TS_MODE_ROMBOOTLOADER = 0x04,
	SYNAPTICS_TS_MODE_BOOTLOADER = 0x0b,
	SYNAPTICS_TS_MODE_TDDI_BOOTLOADER = 0x0c,
	SYNAPTICS_TS_MODE_TDDI_HDL_BOOTLOADER = 0x0d,
	SYNAPTICS_TS_MODE_PRODUCTIONTEST_FIRMWARE = 0x0e,
	SYNAPTICS_TS_MODE_MULTICHIP_TDDI_BOOTLOADER = 0xab,
};

/**
 * @section: Status of Application Firmware
 *
 * The current status is defined in Application Info Packet.
 */
enum tcm_app_status {
	APP_STATUS_OK = 0x00,
	APP_STATUS_BOOTING = 0x01,
	APP_STATUS_UPDATING = 0x02,
	APP_STATUS_BAD_APP_CONFIG = 0xff,
};

/**
 * @section: Field IDs in Dynamic Configuration
 *
 * The codes specify the generic dynamic configuration options.
 */
enum dynamic_tcm_config_id {
	DC_UNKNOWN = 0x00,
	DC_DISABLE_NOISE_MITIGATION = 0x02,
	DC_DISABLE_FREQUENCY_SHIFT = 0x03,
	DC_REQUEST_FREQUENCY_INDEX = 0x04,
	DC_DISABLE_HSYNC = 0x05,
	DC_REZERO_ON_EXIT_DEEP_SLEEP = 0x06,
	DC_ENABLE_CHARGER_CONNECTED = 0x07,
	DC_DISABLE_BASELINE_RELAXATION = 0x08,
	DC_ENABLE_WAKEUP_GESTURE_MODE = 0x09,
	DC_REQUEST_TESTING_FINGERS = 0x0a,
	DC_ENABLE_GRIP_SUPPRESSION = 0x0b,
	DC_ENABLE_THICK_GLOVE = 0x0c,
	DC_ENABLE_GLOVE = 0x0d,
	DC_ENABLE_FACE_DETECTION = 0x0e,
	DC_INHIBIT_ACTIVE_GESTURE = 0x0f,
	DC_DISABLE_PROXIMITY = 0x11,
	DC_DISABLE_DOZE = 0xc4,
	DC_ENABLE_FAST_RESPONSE = 0xc5,
	DC_ENABLE_FOD_INT = 0xd2,
	DC_ENABLE_DEADZONE = 0xda,
	DC_ENABLE_COVER = 0xd7,
	DC_ENABLE_GAMEMODE = 0xd8,
	DC_ENABLE_SIPMODE = 0xd9,
	DC_ENABLE_HIGHSENSMODE = 0xdb,
	DC_ENABLE_NOTEMODE = 0xdc,
	DC_SET_SCANRATE = 0xdf,
	DC_ENABLE_COVERTYPE = 0xe0,
	DC_TSP_SNR_TEST_FRAMES = 0xe1,
	DC_ENABLE_WIRELESS_CHARGER = 0xe2,
	DC_TSP_SET_TEMP = 0xe4,
	DC_TSP_ENABLE_POCKET_MODE = 0xe5,
	DC_TSP_ENABLE_LOW_SENSITIVITY_MODE = 0xe6,
	DC_ENABLE_PEN_MODE = 0xe7,
	DC_ENABLE_EXTENDED_TOUCH_REPORT_CONTROL = 0xf9,
	DC_MAX = 0xFF,
};


/**
 * @section: TouchComm Commands
 *
 * List the generic commands supported in TouchComm command-response protocol.
 */
enum tcm_command {
	SYNAPTICS_TS_CMD_NONE = 0x00,
	SYNAPTICS_TS_CMD_CONTINUE_WRITE = 0x01,
	SYNAPTICS_TS_CMD_IDENTIFY = 0x02,
	SYNAPTICS_TS_CMD_RESET = 0x04,
	SYNAPTICS_TS_CMD_ENABLE_REPORT = 0x05,
	SYNAPTICS_TS_CMD_DISABLE_REPORT = 0x06,
	SYNAPTICS_TS_CMD_TCM2_ACK = 0x07,
	SYNAPTICS_TS_CMD_TCM2_RETRY = 0x08,
	SYNAPTICS_TS_CMD_TCM2_SET_MAX_READ_LENGTH = 0x09,
	SYNAPTICS_TS_CMD_TCM2_GET_REPORT = 0x0a,
	SYNAPTICS_TS_CMD_GET_BOOT_INFO = 0x10,
	SYNAPTICS_TS_CMD_ERASE_FLASH = 0x11,
	SYNAPTICS_TS_CMD_WRITE_FLASH = 0x12,
	SYNAPTICS_TS_CMD_READ_FLASH = 0x13,
	SYNAPTICS_TS_CMD_RUN_APPLICATION_FIRMWARE = 0x14,
	SYNAPTICS_TS_CMD_SPI_MASTER_WRITE_THEN_READ = 0x15,
	SYNAPTICS_TS_CMD_REBOOT_TO_ROM_BOOTLOADER = 0x16,
	SYNAPTICS_TS_CMD_RUN_BOOTLOADER_FIRMWARE = 0x1f,
	SYNAPTICS_TS_CMD_GET_APPLICATION_INFO = 0x20,
	SYNAPTICS_TS_CMD_GET_STATIC_CONFIG = 0x21,
	SYNAPTICS_TS_CMD_SET_STATIC_CONFIG = 0x22,
	SYNAPTICS_TS_CMD_GET_DYNAMIC_CONFIG = 0x23,
	SYNAPTICS_TS_CMD_SET_DYNAMIC_CONFIG = 0x24,
	SYNAPTICS_TS_CMD_GET_TOUCH_REPORT_CONFIG = 0x25,
	SYNAPTICS_TS_CMD_SET_TOUCH_REPORT_CONFIG = 0x26,
	SYNAPTICS_TS_CMD_REZERO = 0x27,
	SYNAPTICS_TS_CMD_COMMIT_CONFIG = 0x28,
	SYNAPTICS_TS_CMD_DESCRIBE_DYNAMIC_CONFIG = 0x29,
	SYNAPTICS_TS_CMD_PRODUCTION_TEST = 0x2a,
	SYNAPTICS_TS_CMD_SET_CONFIG_ID = 0x2b,
	SYNAPTICS_TS_CMD_ENTER_DEEP_SLEEP = 0x2c,
	SYNAPTICS_TS_CMD_EXIT_DEEP_SLEEP = 0x2d,
	SYNAPTICS_TS_CMD_GET_TOUCH_INFO = 0x2e,
	SYNAPTICS_TS_CMD_GET_DATA_LOCATION = 0x2f,
	SYNAPTICS_TS_CMD_DOWNLOAD_CONFIG = 0x30,
	SYNAPTICS_TS_CMD_ENTER_PRODUCTION_TEST_MODE = 0x31,
	SYNAPTICS_TS_CMD_GET_FEATURES = 0x32,
	SYNAPTICS_TS_CMD_GET_ROMBOOT_INFO = 0x40,
	SYNAPTICS_TS_CMD_WRITE_PROGRAM_RAM = 0x41,
	SYNAPTICS_TS_CMD_ROMBOOT_RUN_BOOTLOADER_FIRMWARE = 0x42,
	SYNAPTICS_TS_CMD_SPI_MASTER_WRITE_THEN_READ_EXTENDED = 0x43,
	SYNAPTICS_TS_CMD_ENTER_IO_BRIDGE_MODE = 0x44,
	SYNAPTICS_TS_CMD_ROMBOOT_DOWNLOAD = 0x45,
};

/**
 * @section: TouchComm Status Codes
 *
 * Define the following status codes for all command responses.
 *  0x00: (v1)      no commands are pending and no reports are available.
 *  0x01: (v1 & v2) the previous command succeeded.
 *  0x03: (v1 & v2) the payload continues a previous response.
 *  0x04: (v2)      command was written, but no reports were available.
 *  0x07: (v2)      the previous write was successfully received.
 *  0x08: (v2)      the previous write was corrupt. The host should resend.
 *  0x09: (v2)      the previous command failed.
 *  0x0c: (v1 & v2) write was larger than the device's receive buffer.
 *  0x0d: (v1 & v2) a command was sent before the previous command completed.
 *  0x0e: (v1 & v2) the requested command is not implemented.
 *  0x0f: (v1 & v2) generic communication error, probably incorrect payload.
 *
 *  0xfe: self-defined status for a corrupted packet.
 *  0xff: self-defined status for an invalid data.
 */
enum tcm_status_code {
	STATUS_IDLE = 0x00,
	STATUS_OK = 0x01,
	STATUS_CONTINUED_READ = 0x03,
	STATUS_NO_REPORT_AVAILABLE = 0x04,
	STATUS_BAD_COMMAND = 0x06,
	STATUS_ACK = 0x07,
	STATUS_RETRY_REQUESTED = 0x08,
	STATUS_CMD_FAILED = 0x09,
	STATUS_RECEIVE_BUFFER_OVERFLOW = 0x0c,
	STATUS_PREVIOUS_COMMAND_PENDING = 0x0d,
	STATUS_NOT_IMPLEMENTED = 0x0e,
	STATUS_ERROR = 0x0f,
	STATUS_PACKET_CORRUPTED = 0xfe,
	STATUS_INVALID = 0xff,
};

/**
 * @section: Types of Object Reported
 *
 * List the object classifications
 */
enum object_classification {
	LIFT = 0,
	FINGER = 1,
	GLOVED_OBJECT = 2,
	STYLUS = 3,
	ERASER = 4,
	SMALL_OBJECT = 5,
	PALM = 6,
	EDGE_TOUCHED = 8,
	HOVER_OBJECT = 9,
	NOP = -1,
};

/**
 * @section: Types of Gesture ID
 *
 * List the gesture ID assigned
 */
enum gesture_classification {
	GESTURE_ID_NONE = 0,
	GESTURE_ID_DOUBLE_TAP = 1,
	GESTURE_ID_SWIPE = 2,
	GESTURE_ID_ACTIVE_TAP_AND_HOLD = 7,
};


/**
 * @section: Codes for Touch Report Configuration
 *
 * Define the 8-bit codes for the touch report configuration
 */
enum touch_report_code {
	/* control flow codes */
	TOUCH_REPORT_END = 0x00,
	TOUCH_REPORT_FOREACH_ACTIVE_OBJECT = 0x01,
	TOUCH_REPORT_FOREACH_OBJECT = 0x02,
	TOUCH_REPORT_FOREACH_END = 0x03,
	TOUCH_REPORT_PAD_TO_NEXT_BYTE = 0x04,
	/* entity codes */
	TOUCH_REPORT_TIMESTAMP = 0x05,
	TOUCH_REPORT_OBJECT_N_INDEX = 0x06,
	TOUCH_REPORT_OBJECT_N_CLASSIFICATION = 0x07,
	TOUCH_REPORT_OBJECT_N_X_POSITION = 0x08,
	TOUCH_REPORT_OBJECT_N_Y_POSITION = 0x09,
	TOUCH_REPORT_OBJECT_N_Z = 0x0a,
	TOUCH_REPORT_OBJECT_N_X_WIDTH = 0x0b,
	TOUCH_REPORT_OBJECT_N_Y_WIDTH = 0x0c,
	TOUCH_REPORT_OBJECT_N_TX_POSITION_TIXELS = 0x0d,
	TOUCH_REPORT_OBJECT_N_RX_POSITION_TIXELS = 0x0e,
	TOUCH_REPORT_0D_BUTTONS_STATE = 0x0f,
	TOUCH_REPORT_GESTURE_ID = 0x10,
	TOUCH_REPORT_FRAME_RATE = 0x11,
	TOUCH_REPORT_POWER_IM = 0x12,
	TOUCH_REPORT_CID_IM = 0x13,
	TOUCH_REPORT_RAIL_IM = 0x14,
	TOUCH_REPORT_CID_VARIANCE_IM = 0x15,
	TOUCH_REPORT_NSM_FREQUENCY_INDEX = 0x16,
	TOUCH_REPORT_NSM_STATE = 0x17,
	TOUCH_REPORT_NUM_OF_ACTIVE_OBJECTS = 0x18,
	TOUCH_REPORT_CPU_CYCLES_USED_SINCE_LAST_FRAME = 0x19,
	TOUCH_REPORT_FACE_DETECT = 0x1a,
	TOUCH_REPORT_GESTURE_DATA = 0x1b,
	TOUCH_REPORT_FORCE_MEASUREMENT = 0x1c,
	TOUCH_REPORT_FINGERPRINT_AREA_MEET = 0x1d,
	TOUCH_REPORT_SENSING_MODE = 0x1e,
	TOUCH_REPORT_KNOB_DATA = 0x24,
};


/**
 * @section: TouchComm Report Codes
 *
 * Define the following report codes generated by TouchComm firmware.
 *  0x10:   Identify Info Packet
 *  0x11:   Touch Report
 *  0x12:   Delta Cap. Image
 *  0x13:   Raw Cap. Image
 */
enum tcm_report_type {
	REPORT_IDENTIFY = 0x10,
	REPORT_TOUCH = 0x11,
	REPORT_DELTA = 0x12,
	REPORT_RAW = 0x13,
};
/**
 * @brief: SEC Custom Commands, Reports, or Events
 */
enum custom_command {
	CMD_BSC_DO_CALIBRATION = 0x33,
	CMD_READ_SEC_SPONGE_REG = 0xc0,
	CMD_WRITE_SEC_SPONGE_REG = 0xc1,
	CMD_EVENT_BUFFER_CLEAR = 0xc2,
	CMD_GET_FACE_AREA = 0xc3,
	CMD_SET_AGING_MODE_ON = 0xc4,
	CMD_SET_AGING_MODE_OFF = 0xc5,
	CMD_DBG_ATTN = 0xc7,
	CMD_SET_GRIP = 0xe6,
	CMD_ACCESS_CALIB_DATA_FROM_NVM = 0xe7,
	CMD_SET_IMMEDIATE_DYNAMIC_CONFIG = 0xe8,
};

enum custom_report_type {
	REPORT_SEC_SPONGE_GESTURE = 0xc0,
	REPORT_SEC_COORDINATE_EVENT = 0xc1,
	REPORT_SEC_STATUS_EVENT = 0xc2,
	REPORT_SEC_COORDINATE_RAWDATA_EVENT = 0xc3,
};

/**
 * @section: States in Command Processing
 *
 * List the states in command processing.
 */
enum tcm_command_status {
	CMD_STATE_IDLE = 0,
	CMD_STATE_BUSY = 1,
	CMD_STATE_ERROR = -1,
};

/**
 * @section: Production Test Items
 *
 * List the generic production test items
 */
enum tcm_test_code {
	TEST_NOT_IMPLEMENTED = 0x00,
	TEST_PID01_TRX_SHORTS = 0x01,
	TEST_PID02_TRX_SENSOR_OPENS = 0x02,
	TEST_PID03_TRX_GROUND_SHORTS = 0x03,
	TEST_PID05_FULL_RAW_CAP = 0x05,
	TEST_PID06_EE_SHORT = 0x06,
	TEST_PID07_DYNAMIC_RANGE = 0x07,
	TEST_PID08_HIGH_RESISTANCE = 0x08,
	TEST_PID10_DELTA_NOISE = 0x0a,
	TEST_PID11_OPEN_DETECTION = 0x0b,
	TEST_PID12 = 0x0c,
	TEST_PID13 = 0x0d,
	TEST_PID14_DOZE_DYNAMIC_RANGE = 0x0e,
	TEST_PID15_DOZE_NOISE = 0x0f,
	TEST_PID16_SENSOR_SPEED = 0x10,
	TEST_PID17_ADC_RANGE = 0x11,
	TEST_PID18_HYBRID_ABS_RAW = 0x12,
	TEST_PID22_TRANS_RAW_CAP = 0x16,
	TEST_PID25_TRANS_TRX_SHORT = 0x19,
	TEST_PID29_HYBRID_ABS_NOISE = 0x1D,
	TEST_PID30_BSC_CALIB_TEST = 0x1E,
	TEST_PID48_EXTENDED_TRX_TEST = 0x30,
	TEST_PID52_JITTER_DELTA_TEST = 0x34,
	TEST_PID53_SRAM_TEST = 0x35,
	TEST_PID54_BSC_DIFF_TEST = 0x36,
	TEST_PID56_TSP_SNR_NONTOUCH = 0x38,
	TEST_PID57_TSP_SNR_TOUCH = 0x39,
	TEST_PID58_TRX_OPEN = 0x3A,
	TEST_PID59_PATTERN_SHORT = 0x3B,
	TEST_PID60_SENSITIVITY = 0x3C,
	TEST_PID61_MISCAL = 0x3D,
	TEST_PID65_MISCALDATA_NORMAL = 0x41,
	TEST_PID66_MISCALDATA_NOISE = 0x42,
	TEST_PID67_MISCALDATA_WET = 0x43,
	TEST_PID82_CUSTOMER_DATA_CHECK = 0x52,
	TEST_PID88_HSO_CHECK = 0x58,
	TEST_PID198_PROX_INTENSITY = 0xC6,
	TEST_PID_MAX,
};

/* SYNAPTICS_TS_GESTURE_TYPE */
#define SYNAPTICS_TS_GESTURE_CODE_SWIPE		0x00
#define SYNAPTICS_TS_GESTURE_CODE_DOUBLE_TAP		0x01
#define SYNAPTICS_TS_GESTURE_CODE_PRESS		0x03
#define SYNAPTICS_TS_GESTURE_CODE_SINGLE_TAP		0x04
#define SYNAPTICS_TS_GESTURE_CODE_DUMPFLUSH		0x05
#define SYNAPTICS_TS_GESTURE_CODE_AOD_NOTI_RECT	0x09

/* SYNAPTICS_TS_GESTURE_ID */
#define SYNAPTICS_TS_GESTURE_ID_AOD			0x00
#define SYNAPTICS_TS_GESTURE_ID_DOUBLETAP_TO_WAKEUP	0x01
#define SYNAPTICS_TS_GESTURE_ID_FOD_LONG		0
#define SYNAPTICS_TS_GESTURE_ID_FOD_NORMAL		1
#define SYNAPTICS_TS_GESTURE_ID_FOD_RELEASE		2
#define SYNAPTICS_TS_GESTURE_ID_FOD_OUT		3
#define SYNAPTICS_TS_GESTURE_ID_FOD_VI		4

#define SYNAPTICS_TS_TOUCHTYPE_NORMAL		0
#define SYNAPTICS_TS_TOUCHTYPE_HOVER		1
#define SYNAPTICS_TS_TOUCHTYPE_FLIPCOVER		2
#define SYNAPTICS_TS_TOUCHTYPE_GLOVE		3
#define SYNAPTICS_TS_TOUCHTYPE_STYLUS		4
#define SYNAPTICS_TS_TOUCHTYPE_PALM		5
#define SYNAPTICS_TS_TOUCHTYPE_WET			6
#define SYNAPTICS_TS_TOUCHTYPE_PROXIMITY		7
#define SYNAPTICS_TS_TOUCHTYPE_JIG			8

/* status event type */
#define SYNAPTICS_TS_STATUS_TYPE_ERROR			1
#define SYNAPTICS_TS_STATUS_TYPE_INFO			2
#define SYNAPTICS_TS_STATUS_TYPE_VENDOR			7

/* status event id: error */
#define SYNAPTICS_TS_STATUS_EVENT_ERROR_CORE		0
#define SYNAPTICS_TS_STATUS_EVENT_ERROR_OVERFLOW	1
#define SYNAPTICS_TS_STATUS_EVENT_ERROR_ESD		2

/* status event id: info */
#define SYNAPTICS_TS_STATUS_EVENT_INFO_READY_STATUS	0
#define SYNAPTICS_TS_STATUS_EVENT_INFO_WET		1
#define SYNAPTICS_TS_STATUS_EVENT_INFO_NOISE		2
#define SYNAPTICS_TS_STATUS_EVENT_INFO_JITTER		3
#define SYNAPTICS_TS_STATUS_EVENT_INFO_XENOSENSOR	4
#define SYNAPTICS_TS_STATUS_EVENT_INFO_LIGHTSENSOR	5

/* status event id: vendor */
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_PROXIMITY	0x6A
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_SYNC_STATUS	0x70
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_REZERO_REASON	0x71
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_NSM_STATUS	0x72
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_BASELINE_ERROR	0x73
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_NSM_FREQ	0x74
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_CHARGER_CON	0x75
#define SYNAPTICS_TS_STATUS_EVENT_VENDOR_MODE_CHANGED	0xE8

/* SEC_TS_DUMP_ID */
#define SYNAPTICS_TS_SPONGE_DUMP_0			0x00
#define SYNAPTICS_TS_SPONGE_DUMP_1			0x01

#endif
