/* SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_SYNAPTICS_TS_H_
#define _LINUX_SYNAPTICS_TS_H_

#include <linux/cdev.h>
#include <linux/crc32.h>
#include <linux/spi/spi.h>
#include <linux/input/sec_input.h>

#if IS_ENABLED(CONFIG_SEC_INPUT_RAWDATA)
#include <linux/input/sec_input_rawdata.h>
#endif

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
#include <linux/input/stui_inf.h>
#endif

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
#define SECURE_TOUCH_ENABLE	1
#define SECURE_TOUCH_DISABLE	0
#endif

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
#include <linux/sec_panel_notifier_v2.h>
#define SYNAPTICS_PANEL_DETACHED	0
#define SYNAPTICS_PANEL_ATTACHED	1
#endif

#if IS_ENABLED(CONFIG_INPUT_TOUCHSCREEN_TCLMV2)
#define TCLM_CONCEPT
#endif


#ifndef I2C_M_DMA_SAFE
#define I2C_M_DMA_SAFE		0
#endif

#include "synaptics_reg.h"

#define SYNAPTICS_TS_NAME		"synaptics_ts"
#define SYNAPTICS_TS_I2C_NAME		"synaptics_ts_i2c"
#define SYNAPTICS_TCM_DRV_DEFAULT	"synaptics_tcm"
#define SYNAPTICS_TCM_DEV_DEFAULT	"tcm"

#define PRINT_SIZE	10

extern struct device *ptsp;

#define SYNAPTICS_TS_I2C_RETRY_CNT	5
#define SYNAPTICS_TS_TIMEOUT_RETRY_CNT	3

#define SYNAPTICS_TS_CHK_UPDATE_FAIL		(1 << 0)
#define SYNAPTICS_TS_CHK_BLMODE_ON_PROBE	(1 << 1)
#define SYNAPTICS_TS_CHK_BLMODE_ON_START	(1 << 2)

/* if it is not ship build, disable reset on reinit for debugging */
#if IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
#define SYNAPTICS_TS_RESET_ON_REINIT_TIMEOUT
#endif

#define ERASE_DELAY (800)
#define ERASE_DELAY_EX (15000)
#define WRITE_FLASH_DELAY (50)

#define SYNAPTICS_TS_CHECK_MODE_DELAY		300

/**
 * @section: Blocks to be updated
 */
enum update_area {
	UPDATE_NONE = 0,
	UPDATE_FIRMWARE_AND_CONFIG,
	UPDATE_CONFIG,
	UPDATE_ALL_BLOCKS,
};

/**
 * @section: Data Type in flash memory
 */
enum flash_data {
	FLASH_LCM_DATA = 1,
	FLASH_OEM_DATA,
	FLASH_PPDT_DATA,
	FLASH_FORCE_CALIB_DATA,
	FLASH_OPEN_SHORT_TUNING_DATA,
};

/**
 * @section: Some specific definition in the firmware file
 */
#define ID_STRING_SIZE (32)

#define SIZE_WORDS (8)

#define IHEX_RECORD_SIZE (14)

#define IHEX_MAX_BLOCKS (64)

#define IMAGE_FILE_MAGIC_VALUE (0x4818472b)

#define FLASH_AREA_MAGIC_VALUE (0x7c05e516)

#define SYNAPTICS_TS_NVM_CALIB_DATA_SIZE (30)

#define SEC_NVM_CALIB_DATA_SIZE (28)

/**
 * @section: Helper macros for firmware file parsing
 */
#define CRC32(data, length) \
	(crc32(~0, data, length) ^ ~0)

#define VALUE(value) \
	(synaptics_ts_pal_le2_to_uint(value))

#define AREA_ID_STR(area) \
	(synaptics_ts_get_flash_area_string(area))

/**
 * @section: Area Partitions in firmware
 */
enum flash_area {
	AREA_NONE = 0,
	/* please add the declarations below */

	AREA_BOOT_CODE,
	AREA_BOOT_CONFIG,
	AREA_APP_CODE,
	AREA_APP_CODE_COPRO,
	AREA_APP_CONFIG,
	AREA_PROD_TEST,
	AREA_DISP_CONFIG,
	AREA_F35_APP_CODE,
	AREA_FORCE_TUNING,
	AREA_GAMMA_TUNING,
	AREA_TEMPERATURE_GAMM_TUNING,
	AREA_CUSTOM_LCM,
	AREA_LOOKUP,
	AREA_CUSTOM_OEM,
	AREA_OPEN_SHORT_TUNING,
	AREA_CUSTOM_OTP,
	AREA_PPDT,
	AREA_ROMBOOT_APP_CODE,
	AREA_TOOL_BOOT_CONFIG,

	/* please add the declarations above */
	AREA_MAX,
};
/**
 * @section: String of Area Partitions in firmware
 */
static char *flash_area_str[] = {
	NULL,
	/* please add the declarations below */

	"BOOT_CODE",       /* AREA_BOOT_CODE */
	"BOOT_CONFIG",     /* AREA_BOOT_CONFIG */
	"APP_CODE",        /* AREA_APP_CODE  */
	"APP_CODE_COPRO",  /* AREA_APP_CODE_COPRO */
	"APP_CONFIG",      /* AREA_APP_CONFIG */
	"APP_PROD_TEST",   /* AREA_PROD_TEST */
	"DISPLAY",         /* AREA_DISP_CONFIG  */
	"F35_APP_CODE",    /* AREA_F35_APP_CODE  */
	"FORCE",           /* AREA_FORCE_TUNING */
	"GAMMA",           /* AREA_GAMMA_TUNING */
	"TEMPERATURE_GAMM",/* AREA_TEMPERATURE_GAMM_TUNING */
	"LCM",             /* AREA_CUSTOM_LCM */
	"LOOKUP",          /* AREA_LOOKUP */
	"OEM",             /* AREA_CUSTOM_OEM */
	"OPEN_SHORT",      /* AREA_OPEN_SHORT_TUNING */
	"OTP",             /* AREA_CUSTOM_OTP */
	"PPDT",            /* AREA_PPDT */
	"ROMBOOT_APP_CODE",/* AREA_ROMBOOT_APP_CODE */
	"TOOL_BOOT_CONFIG",/* AREA_TOOL_BOOT_CONFIG */

	/* please add the declarations above */
	NULL
};

typedef struct completion synaptics_ts_pal_completion_t;
typedef struct mutex synaptics_ts_pal_mutex_t;

/**
 * @section: Internal Buffer Structure
 *
 * This structure is taken as the internal common buffer.
 */
struct synaptics_ts_buffer {
	unsigned char *buf;
	unsigned int buf_size;
	unsigned int data_length;
	synaptics_ts_pal_mutex_t buf_mutex;
	unsigned char ref_cnt;
};

/**
 * @section: Header Content of app config defined
 *           in firmware file
 */
struct app_config_header {
	unsigned short magic_value[4];
	unsigned char checksum[4];
	unsigned char length[2];
	unsigned char build_id[4];
	unsigned char customer_config_id[16];
};
/**
 * @section: The Partition Descriptor defined
 *           in firmware file
 */
struct area_descriptor {
	unsigned char magic_value[4];
	unsigned char id_string[16];
	unsigned char flags[4];
	unsigned char flash_addr_words[4];
	unsigned char length[4];
	unsigned char checksum[4];
};
/**
 * @section: Structure for the Data Block defined
 *           in firmware file
 */
struct block_data {
	bool available;
	const unsigned char *data;
	unsigned int size;
	unsigned int flash_addr;
	unsigned char id;
};
/**
 * @section: Structure for the Parsed Image File
 */
struct image_info {
	struct block_data data[AREA_MAX];
};
/**
 * @section: Header of Image File
 *
 * Define the header of firmware image file
 */
struct image_header {
	unsigned char magic_value[4];
	unsigned char num_of_areas[4];
};
/**
 * @section: Structure for the Parsed iHex File
 */
struct ihex_info {
	unsigned int records;
	unsigned char *bin;
	unsigned int bin_size;
	struct block_data block[IHEX_MAX_BLOCKS];
};

/* factory test mode */
struct synaptics_ts_test_mode {
	u8 type;
	int min;
	int max;
	bool allnode;
	bool frame_channel;
};

/**
 * @section: Specific data blob for reflash
 *
 * The structure contains various parameters being used in reflash
 */
struct synaptics_ts_reflash_data_blob {
	/* binary data of an image file */
	const unsigned char *image;
	unsigned int image_size;
	/* parsed data based on given image file */
	struct image_info image_info;
	/* standard information for flash access */
	unsigned int page_size;
	unsigned int write_block_size;
	unsigned int max_write_payload_size;
	struct synaptics_ts_buffer out;
};

/**
 * @section: Header of TouchComm v1 Message Packet
 *
 * The 4-byte header in the TouchComm v1 packet
 */
struct synaptics_ts_v1_message_header {
	union {
		struct {
			unsigned char marker;
			unsigned char code;
			unsigned char length[2];
		};
		unsigned char data[SYNAPTICS_TS_MESSAGE_HEADER_SIZE];
	};
};


/**
 * @section: TouchComm Identify Info Packet
 *           Ver.1: size is 24 (0x18) bytes
 *           Ver.2: size is extended to 32 (0x20) bytes
 *
 * The identify packet provides the basic TouchComm information and indicate
 * that the device is ready to receive commands.
 *
 * The report is received whenever the device initially powers up, resets,
 * or switches fw between bootloader and application modes.
 */
struct synaptics_ts_identification_info {
	unsigned char version;
	unsigned char mode;
	unsigned char part_number[16];
	unsigned char build_id[4];
	unsigned char max_write_size[2];
	/* extension in ver.2 */
	unsigned char max_read_size[2];
	unsigned char reserved[6];
};

/**
 * @section: TouchComm Application Information Packet
 *
 * The application info packet provides the information about the application
 * firmware as well as the touch controller.
 */
struct synaptics_ts_application_info {
	unsigned char version[2];
	unsigned char status[2];
	unsigned char static_config_size[2];
	unsigned char dynamic_config_size[2];
	unsigned char app_config_start_write_block[2];
	unsigned char app_config_size[2];
	unsigned char max_touch_report_config_size[2];
	unsigned char max_touch_report_payload_size[2];
	unsigned char customer_config_id[MAX_SIZE_CONFIG_ID];
	unsigned char max_x[2];
	unsigned char max_y[2];
	unsigned char max_objects[2];
	unsigned char num_of_buttons[2];
	unsigned char num_of_image_rows[2];
	unsigned char num_of_image_cols[2];
	unsigned char has_hybrid_data[2];
	unsigned char num_of_force_elecs[2];
};

/**
 * @section: TouchComm boot information packet
 *
 * The boot info packet provides the information of TouchBoot.
 */
struct synaptics_ts_boot_info {
	unsigned char version;
	unsigned char status;
	unsigned char asic_id[2];
	unsigned char write_block_size_words;
	unsigned char erase_page_size_words[2];
	unsigned char max_write_payload_size[2];
	unsigned char last_reset_reason;
	unsigned char pc_at_time_of_last_reset[2];
	unsigned char boot_config_start_block[2];
	unsigned char boot_config_size_blocks[2];
	/* extension in ver.2 */
	unsigned char display_config_start_block[4];
	unsigned char display_config_length_blocks[2];
	unsigned char backup_display_config_start_block[4];
	unsigned char backup_display_config_length_blocks[2];
	unsigned char custom_otp_start_block[2];
	unsigned char custom_otp_length_blocks[2];
};

/* @section: Data blob for touch data reported
 *
 * Once receiving a touch report generated by firmware, the touched data
 * will be parsed and converted to touch_data_blob structure as a data blob.
 *
 * @subsection: tcm_touch_data_blob
 *              The touch_data_blob contains all sorts of touched data entities.
 *
 * @subsection tcm_objects_data_blob
 *             The objects_data_blob includes the data for each active objects.
 *
 * @subsection tcm_gesture_data_blob
 *             The gesture_data_blob contains the gesture data if detected.
 */
struct synaptics_ts_objects_data_blob {
	unsigned char status;
	unsigned int x_pos;
	unsigned int y_pos;
	unsigned int x_width;
	unsigned int y_width;
	unsigned int z;
	unsigned int tx_pos;
	unsigned int rx_pos;
};

struct synaptics_ts_gesture_data_blob {
	union {
		struct {
			unsigned char tap_x[2];
			unsigned char tap_y[2];
		};
		struct {
			unsigned char swipe_x[2];
			unsigned char swipe_y[2];
			unsigned char swipe_direction[2];
		};
		unsigned char data[MAX_SIZE_GESTURE_DATA];
	};
};

struct synaptics_ts_touch_data_blob {

	/* for each active objects */
	unsigned int num_of_active_objects;
	struct synaptics_ts_objects_data_blob object_data[MAX_NUM_OBJECTS];

	/* for gesture */
	unsigned int gesture_id;
	struct synaptics_ts_gesture_data_blob gesture_data;

	/* various data */
	unsigned int timestamp;
	unsigned int buttons_state;
	unsigned int frame_rate;
	unsigned int power_im;
	unsigned int cid_im;
	unsigned int rail_im;
	unsigned int cid_variance_im;
	unsigned int nsm_frequency;
	unsigned int nsm_state;
	unsigned int num_of_cpu_cycles;
	unsigned int fd_data;
	unsigned int force_data;
	unsigned int fingerprint_area_meet;
	unsigned int sensing_mode;
};

/**
 * @section: TouchComm Message Handling Wrapper
 *
 * The structure contains the essential buffers and parameters to implement
 * the command-response protocol for both TouchCom ver.1 and TouchCom ver.2.
 */
struct synaptics_ts_message_data_blob {
	/* parameters for command processing */
	syna_pal_atomic_t command_status;
	unsigned char command;
	unsigned char status_report_code;
	unsigned int payload_length;
	unsigned char response_code;
	unsigned char report_code;
	unsigned char seq_toggle;

	/* completion event command processing */
	synaptics_ts_pal_completion_t cmd_completion;

	/* internal buffers
	 *   in  : buffer storing the data being read 'in'
	 *   out : buffer storing the data being sent 'out'
	 *   temp: 'temp' buffer used for continued read operation
	 */
	struct synaptics_ts_buffer in;
	struct synaptics_ts_buffer out;
	struct synaptics_ts_buffer temp;

	/* mutex for the protection of command processing */
	synaptics_ts_pal_mutex_t cmd_mutex;

	/* mutex for the read/write protection */
	synaptics_ts_pal_mutex_t rw_mutex;

};

#define UEVENT_OPEN_SHORT_PASS		1
#define UEVENT_OPEN_SHORT_FAIL		2

/**
 * @brief: context of SEC touch/coordinate event data
 */
struct sec_touch_event_data {
	union {
		struct {
			unsigned char eid:2;
			unsigned char tid:4;
			unsigned char tchsta:2;
			unsigned char x_11_4;
			unsigned char y_11_4;
			unsigned char y_3_0:4;
			unsigned char x_3_0:4;
			unsigned char major;
			unsigned char minor;
			unsigned char z:6;
			unsigned char ttype_3_2:2;
			unsigned char left_event:5;
			unsigned char max_energy_flag:1;
			unsigned char ttype_1_0:2;
			unsigned char noise_level;
			unsigned char max_strength;
			unsigned char hover_id_num:4;
			unsigned char noise_status:2;
			unsigned char reserved10:1;
			unsigned char game_mode:1;
			unsigned char freq_id:4;
			unsigned char fod_debug:4;
			unsigned char call:1;
			unsigned char pocket:1;
			unsigned char lowsen:1;
			unsigned char sip:1;
			unsigned char wireless:1;
			unsigned char ta:1;
			unsigned char note:1;
			unsigned char cover:1;
			unsigned char reserved_byte13;
			unsigned char reserved_byte14;
			unsigned char reserved_byte15;
		} __packed;
		unsigned char data[16];
	};
};

/**
 * @brief: context of SEC status event data
 */
struct sec_status_event_data {
	union {
		struct {
			unsigned char eid:2;
			unsigned char stype:4;
			unsigned char sf:2;
			unsigned char status_id;
			unsigned char status_data_1;
			unsigned char status_data_2;
			unsigned char status_data_3;
			unsigned char status_data_4;
			unsigned char status_data_5;
			unsigned char left_event_5_0:6;
			unsigned char reserved_byte07_7_6:2;
			unsigned char reserved_byte08;
			unsigned char reserved_byte09;
			unsigned char reserved_byte10;
			unsigned char reserved_byte11;
			unsigned char reserved_byte12;
			unsigned char reserved_byte13;
			unsigned char reserved_byte14;
			unsigned char reserved_byte15;
		} __packed;
		unsigned char data[16];
	};
};

/**
 * @brief: context of SEC gesture event data
 */
struct sec_gesture_event_data {
	union {
		struct {
			unsigned char eid:2;
			unsigned char stype:4;
			unsigned char sf:2;
			unsigned char gesture_id;
			unsigned char gesture_data_1;
			unsigned char gesture_data_2;
			unsigned char gesture_data_3;
			unsigned char gesture_data_4;
			unsigned char gesture_data_5;
			unsigned char left_event_5_0:6;
			unsigned char reserved_byte07_7_6:2;
			unsigned char reserved_byte08;
			unsigned char reserved_byte09;
			unsigned char reserved_byte10;
			unsigned char reserved_byte11;
			unsigned char reserved_byte12;
			unsigned char reserved_byte13;
			unsigned char reserved_byte14;
			unsigned char reserved_byte15;
		} __packed;
		unsigned char data[16];
	};
};

struct tsp_snr_result_of_point {
	s16	max;
	s16	min;
	s16	average;
	s16	nontouch_peak_noise;
	s16	touch_peak_noise;
	s16	snr1;
	s16	snr2;
} __packed;

/* This Flash Meory Map is FIXED by SYNAPTICS firmware
 * Do not change MAP.
 */
#define SYNAPTICS_TS_NVM_OFFSET_ALL	31

#define ENABLE_EXTERNAL_FRAME_PROCESS
#define REPORT_TYPES (256)
#define EFP_ENABLE	(1)
#define EFP_DISABLE (0)

struct synaptics_ts_sysfs {
	/* cdev and sysfs nodes creation */
	struct cdev char_dev;
	dev_t char_dev_num;
	int char_dev_ref_count;

	struct class *device_class;
	struct device *device;

	struct kobject *sysfs_dir;

	/* IOCTL-related variables */
	pid_t proc_pid;
	struct task_struct *proc_task;

	/* fifo to pass the report to userspace */
	unsigned int fifo_remaining_frame;
	struct list_head frame_fifo_queue;
	wait_queue_head_t wait_frame;
	unsigned char report_to_queue[REPORT_TYPES];

	bool is_attn_redirecting;
};

struct synaptics_ts_mode_state {
	unsigned short drv_setting;
	unsigned short ic_state;
	bool is_waiting;
};
#define SYNAPTICS_TS_MODE_STATE_SIZE (sizeof(struct synaptics_ts_mode_state) * DC_MAX)

struct synaptics_ts_data {
	void *client;
	struct device *dev;
	struct mutex transfer_mutex;
	unsigned int spi_mode;
	int irq;
	int irq_empty_count;
	struct sec_ts_plat_data *plat_data;
	struct sec_input_multi_device *multi_dev;
	struct synaptics_ts_identification_info id_info;
	struct synaptics_ts_application_info app_info;
	struct synaptics_ts_boot_info boot_info;

	struct synaptics_ts_touch_data_blob tp_data;
	/* internal buffers
	 *   resp  : record the command response to caller
	 */
	struct synaptics_ts_buffer resp_buf;
	struct synaptics_ts_buffer external_buf;

	/* touch report configuration */
	struct synaptics_ts_buffer touch_config;
	struct synaptics_ts_buffer report_buf;

	/* Buffer stored the irq event data */
	struct synaptics_ts_buffer event_data;
	struct synaptics_ts_v1_message_header *header;

	/* TouchComm message handling wrapper */
	struct synaptics_ts_message_data_blob msg_data;

	/* for vendor sysfs */
	struct synaptics_ts_sysfs *vendor_data;

	/* indicate that fw update is on-going */
	syna_pal_atomic_t firmware_flashing;

	/* basic device information */
	unsigned char dev_mode;
	unsigned int packrat_number;
	unsigned int max_x;
	unsigned int max_y;
	unsigned int max_objects;
	unsigned int rows;
	unsigned int cols;
	unsigned char config_id[MAX_SIZE_CONFIG_ID];

	struct mutex lock;
	bool probe_done;
	struct sec_cmd_data sec;

	/* capability of read/write transferred
	 * being assigned through syna_hw_interface
	 */
	unsigned int max_wr_size;
	unsigned int max_rd_size;

	int *pFrame;
	u8 miscal_result;
	u8 *cx_data;
	u8 *ito_result;
	u8 disassemble_count;
	u8 fac_nv;

	struct sec_tclm_data *tdata;
	bool is_cal_done;
	int firmware_update_done;

	bool fw_corruption;

	int glove_mode;
	int game_mode;
	int fast_response;
	int sip_mode;
	int note_mode;
	int scan_rate;
	int refresh_rate;
	int dead_zone;
	bool cover_closed;
	int fix_active_mode;
	int pen_hover_in;
	struct synaptics_ts_mode_state *mode;

	int resolution_x;
	int resolution_y;

	bool support_immediate_cmd;
	bool support_miscal_wet;
	const char *tcm_drv_name;
	const char *tcm_dev_name;
	int fw_erase_delay;
	int fw_write_block_delay;
	int power_on_delay;

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	u8 panel_attached;
	struct notifier_block lcd_nb;
#endif
	struct notifier_block synaptics_input_nb;
	struct delayed_work work_print_info;
	struct delayed_work work_read_functions;
	struct delayed_work reset_work;
	struct delayed_work work_read_info;
	struct delayed_work debug_work;
	struct delayed_work check_rawdata;
	struct delayed_work set_temperature_work;
	struct delayed_work work_notify_pen_hover;
	struct delayed_work work_check_mode;

	atomic_t reset_is_on_going;

	u8 prev_event_id;

	int debug_flag;
	struct mutex device_mutex;
	struct mutex eventlock;
	struct mutex sponge_mutex;
	struct mutex fn_mutex;
	bool info_work_done;

	bool sponge_inf_dump;
	u8 sponge_dump_format;
	u8 sponge_dump_event;
	u8 sponge_dump_border_msb;
	u8 sponge_dump_border_lsb;
	bool sponge_dump_delayed_flag;
	u8 sponge_dump_delayed_area;
	u16 sponge_dump_border;

	bool rear_selfie_mode;

	u8 hover_event;

	bool tsp_dump_lock;

	bool touch_aging_mode;
	int sensitivity_mode;

	s16 *raw;
	int raw_mode;
	bool rawcap_lock;
	int rawcap_max;
	int rawcap_max_tx;
	int rawcap_max_rx;
	int rawcap_min;
	int rawcap_min_tx;
	int rawcap_min_rx;
	u8 factory_position;

	int (*stop_device)(struct synaptics_ts_data *ts);
	int (*start_device)(struct synaptics_ts_data *ts);

	int (*synaptics_ts_write_data)(struct synaptics_ts_data *ts, u8 *reg, int cunum, u8 *data, int len);
	int (*synaptics_ts_read_data)(struct synaptics_ts_data *ts, u8 *reg, int cnum, u8 *data, int len);
	int (*synaptics_ts_read_data_only)(struct synaptics_ts_data *ts, u8 *data, int len);
	int (*synaptics_ts_read_sponge)(struct synaptics_ts_data *ts, unsigned char *rd_buf, int size_buf,
		unsigned short offset, unsigned int rd_len);
	int (*synaptics_ts_write_sponge)(struct synaptics_ts_data *ts, unsigned char *wr_buf, int size_buf,
		unsigned short offset, unsigned int wr_len);
	int (*synaptics_ts_systemreset)(struct synaptics_ts_data *ts, unsigned int msec);
	int (*synaptics_ts_get_baredata)(struct synaptics_ts_data *ts, u8 *data, int len);
	int (*write_message)(struct synaptics_ts_data *ts,
	unsigned char command, unsigned char *payload,
	unsigned int payload_len, unsigned char *resp_code,
	unsigned int delay_ms_resp);
	int (*read_message)(struct synaptics_ts_data *ts,
		unsigned char *status_report_code);
	int (*write_immediate_message)(struct synaptics_ts_data *ts,
		unsigned char command, unsigned char *payload,
		unsigned int payload_len);
};

//temporily

/**
 * @section: C Integer Calculation helpers
 *
 * @brief: synaptics_ts_pal_le2_to_uint
 *         Convert 2-byte data to an unsigned integer
 *
 * @brief: synaptics_ts_pal_le4_to_uint
 *         Convert 4-byte data to an unsigned integer
 *
 * @brief: synaptics_ts_pal_ceil_div
 *         Calculate the ceiling of the integer division
 */

/**
 * synaptics_ts_pal_le2_to_uint()
 *
 * Convert 2-byte data in little-endianness to an unsigned integer
 *
 * @param
 *    [ in] src: 2-byte data in little-endianness
 *
 * @return
 *    an unsigned integer converted
 */
static inline unsigned int synaptics_ts_pal_le2_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
		(unsigned int)src[1] * 0x100;
}
/**
 * synaptics_ts_pal_le4_to_uint()
 *
 * Convert 4-byte data in little-endianness to an unsigned integer
 *
 * @param
 *    [ in] src: 4-byte data in little-endianness
 *
 * @return
 *    an unsigned integer converted
 */
static inline unsigned int synaptics_ts_pal_le4_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
		(unsigned int)src[1] * 0x100 +
		(unsigned int)src[2] * 0x10000 +
		(unsigned int)src[3] * 0x1000000;
}
/**
 * synaptics_ts_pal_ceil_div()
 *
 * Calculate the ceiling of the integer division
 *
 * @param
 *    [ in] dividend: the dividend value
 *    [ in] divisor:  the divisor value
 *
 * @return
 *    the ceiling of the integer division
 */
static inline unsigned int synaptics_ts_pal_ceil_div(unsigned int dividend,
		unsigned int divisor)
{
	return (dividend + divisor - 1) / divisor;
}


/**
 * @section: C Runtime for Memory Management helpers
 *
 * @brief: synaptics_ts_pal_mem_calloc
 *         Allocate a block of memory space
 *
 * @brief: synaptics_ts_pal_mem_free
 *         Deallocate a block of memory previously allocated
 *
 * @brief: synaptics_ts_pal_mem_set
 *         Fill memory with a constant byte
 *
 * @brief: synaptics_ts_pal_mem_cpy
 *         Ensure the safe size before doing memory copy
 */

/**
 * synaptics_ts_pal_mem_calloc()
 *
 * Allocates a block of memory for an array of 'num' elements,
 * each of them has 'size' bytes long, and initializes all its bits to zero.
 *
 * @param
 *    [ in] num:  number of elements for an array
 *    [ in] size: number of bytes for each elements
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 */
static inline void *synaptics_ts_pal_mem_alloc(unsigned int num, unsigned int size)
{
#ifdef DEV_MANAGED_API
	struct device *dev = synaptics_ts_request_managed_device();

	if (!dev) {
		pr_err("[sec_input]Invalid managed device\n");
		return NULL;
	}
#endif

	if ((int)(num * size) <= 0) {
		pr_err("[sec_input]Invalid parameter\n");
		return NULL;
	}

#ifdef DEV_MANAGED_API
	return devm_kcalloc(dev, num, size, GFP_KERNEL);
#else /* Legacy API */
	return kcalloc(num, size, GFP_KERNEL);
#endif
}
/**
 * synaptics_ts_pal_mem_free()
 *
 * Deallocate a block of memory previously allocated.
 *
 * @param
 *    [ in] ptr: a memory block  previously allocated
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_mem_free(void *ptr)
{
#ifdef DEV_MANAGED_API
	struct device *dev = synaptics_ts_request_managed_device();

	if (!dev) {
		pr_err("[sec_input]Invalid managed device\n");
		return;
	}

	if (ptr)
		devm_kfree(dev, ptr);
#else /* Legacy API */
	if (ptr) {
		kfree(ptr);
		ptr = NULL;
	}
#endif
}
/**
 * synaptics_ts_pal_mem_set()
 *
 * Fill memory with a constant byte
 *
 * @param
 *    [ in] ptr: pointer to a memory block
 *    [ in] c:   the constant value
 *    [ in] n:   number of byte being set
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_mem_set(void *ptr, int c, unsigned int n)
{
	memset(ptr, c, n);
}
/**
 * synaptics_ts_pal_mem_cpy()
 *
 * Ensure the safe size before copying the values of num bytes from the
 * location to the memory block pointed to by destination.
 *
 * @param
 *    [out] dest:      pointer to the destination space
 *    [ in] dest_size: size of destination array
 *    [ in] src:       pointer to the source of data to be copied
 *    [ in] src_size:  size of source array
 *    [ in] num:       number of bytes to copy
 *
 * @return
 *    0 on success; otherwise, on error.
 */
static inline int synaptics_ts_pal_mem_cpy(void *dest, unsigned int dest_size,
		const void *src, unsigned int src_size, unsigned int num)
{
	if (dest == NULL || src == NULL)
		return -1;

	if (num > dest_size || num > src_size) {
		pr_err("[sec_input]Invalid size. src:%d, dest:%d, num:%d\n",
			src_size, dest_size, num);
		return -1;
	}

	memcpy((void *)dest, (const void *)src, num);

	return 0;
}


/**
 * synaptics_ts_pal_mutex_alloc()
 *
 * Create a mutex object.
 *
 * @param
 *    [out] ptr: pointer to the mutex handle being allocated
 *
 * @return
 *    0 on success; otherwise, on error.
 */
static inline int synaptics_ts_pal_mutex_alloc(synaptics_ts_pal_mutex_t *ptr)
{
	mutex_init((struct mutex *)ptr);
	return 0;
}
/**
 * synaptics_ts_pal_mutex_free()
 *
 * Release the mutex object previously allocated.
 *
 * @param
 *    [ in] ptr: mutex handle previously allocated
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_mutex_free(synaptics_ts_pal_mutex_t *ptr)
{
	/* do nothing */
}
/**
 * synaptics_ts_pal_mutex_lock()
 *
 * Acquire/lock the mutex.
 *
 * @param
 *    [ in] ptr: a mutex handle
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_mutex_lock(synaptics_ts_pal_mutex_t *ptr)
{
	mutex_lock((struct mutex *)ptr);
}
/**
 * synaptics_ts_pal_mutex_unlock()
 *
 * Unlock the locked mutex.
 *
 * @param
 *    [ in] ptr: a mutex handle
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_mutex_unlock(synaptics_ts_pal_mutex_t *ptr)
{
	mutex_unlock((struct mutex *)ptr);
}


/**
 * synaptics_ts_pal_completion_alloc()
 *
 * Allocate a completion event, and the default state is not set.
 * Caller must reset the event before each use.
 *
 * @param
 *    [out] ptr: pointer to the completion handle being allocated
 *
 * @return
 *    0 on success; otherwise, on error.
 */
static inline int synaptics_ts_pal_completion_alloc(synaptics_ts_pal_completion_t *ptr)
{
	init_completion((struct completion *)ptr);
	return 0;
}
/**
 * synaptics_ts_pal_completion_free()
 *
 * Release the completion event previously allocated
 *
 * @param
 *    [ in] ptr: the completion event previously allocated
 event
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_completion_free(synaptics_ts_pal_completion_t *ptr)
{
	/* do nothing */
}
/**
 * synaptics_ts_pal_completion_complete()
 *
 * Complete the completion event being waiting for
 *
 * @param
 *    [ in] ptr: the completion event
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_completion_complete(synaptics_ts_pal_completion_t *ptr)
{
	if (!completion_done((struct completion *)ptr))
		complete((struct completion *)ptr);
}
/**
 * synaptics_ts_pal_completion_reset()
 *
 * Reset or reinitialize the completion event
 *
 * @param
 *    [ in] ptr: the completion event
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_completion_reset(synaptics_ts_pal_completion_t *ptr)
{
#if (KERNEL_VERSION(3, 13, 0) > LINUX_VERSION_CODE)
		init_completion((struct completion *)ptr);
#else
		reinit_completion((struct completion *)ptr);
#endif
}
/**
 * synaptics_ts_pal_completion_wait_for()
 *
 * Wait for the completion event during the given time slot
 *
 * @param
 *    [ in] ptr:        the completion event
 *    [ in] timeout_ms: time frame in milliseconds
 *
 * @return
 *    0 if a signal is received; otherwise, on timeout or error occurs.
 */
static inline int synaptics_ts_pal_completion_wait_for(synaptics_ts_pal_completion_t *ptr,
		unsigned int timeout_ms)
{
	int retval;

	retval = wait_for_completion_timeout((struct completion *)ptr,
			msecs_to_jiffies(timeout_ms));
	if (retval == 0) /* timeout occurs */
		return -1;

	return 0;
}


/**
 * @section: C Runtime to Pause the Execution
 *
 * @brief: synaptics_ts_pal_sleep_ms
 *         Sleep for a fixed amount of time in milliseconds
 *
 * @brief: synaptics_ts_pal_sleep_us
 *         Sleep for a range of time in microseconds
 *
 * @brief: synaptics_ts_pal_busy_delay_ms
 *         Busy wait for a fixed amount of time in milliseconds
 */

/**
 * synaptics_ts_pal_sleep_ms()
 *
 * Sleep for a fixed amount of time in milliseconds
 *
 * @param
 *    [ in] time_ms: time frame in milliseconds
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_sleep_ms(int time_ms)
{
	msleep(time_ms);
}
/**
 * synaptics_ts_pal_sleep_us()
 *
 * Sleep for a range of time in microseconds
 *
 * @param
 *    [ in] time_us_min: the min. time frame in microseconds
 *    [ in] time_us_max: the max. time frame in microseconds
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_sleep_us(int time_us_min, int time_us_max)
{
	usleep_range(time_us_min, time_us_max);
}
/**
 * synaptics_ts_pal_busy_delay_ms()
 *
 * Busy wait for a fixed amount of time in milliseconds
 *
 * @param
 *    [ in] time_ms: time frame in milliseconds
 *
 * @return
 *    none.
 */
static inline void synaptics_ts_pal_busy_delay_ms(int time_ms)
{
	mdelay(time_ms);
}


/**
 * @section: C Runtime for String operations
 *
 * @brief: synaptics_ts_pal_str_len
 *         Return the length of C string
 *
 * @brief: synaptics_ts_pal_str_cpy:
 *         Ensure the safe size before doing C string copy
 *
 * @brief: synaptics_ts_pal_str_cmp:
 *         Compare whether the given C strings are equal or not
 */

/**
 * synaptics_ts_pal_str_len()
 *
 * Return the length of C string
 *
 * @param
 *    [ in] str:  an array of characters
 *
 * @return
 *    the length of given string
 */
static inline unsigned int synaptics_ts_pal_str_len(const char *str)
{
	return (unsigned int)strlen(str);
}
/**
 * synaptics_ts_pal_str_cpy()
 *
 * Copy the C string pointed by source into the array pointed by destination.
 *
 * @param
 *    [ in] dest:      pointer to the destination C string
 *    [ in] dest_size: size of destination C string
 *    [out] src:       pointer to the source of C string to be copied
 *    [ in] src_size:  size of source C string
 *    [ in] num:       number of bytes to copy
 *
 * @return
 *    0 on success; otherwise, on error.
 */
static inline int synaptics_ts_pal_str_cpy(char *dest, unsigned int dest_size,
		const char *src, unsigned int src_size, unsigned int num)
{
	if (dest == NULL || src == NULL)
		return -1;

	if (num > dest_size || num > src_size) {
		pr_err("[sec_input]Invalid size. src:%d, dest:%d, num:%d\n",
			src_size, dest_size, num);
		return -1;
	}

	strncpy(dest, src, num);

	return 0;
}
/**
 * synaptics_ts_pal_str_cmp()
 *
 * Compares up to num characters of the C string str1 to those of the
 * C string str2.
 *
 * @param
 *    [ in] str1: C string to be compared
 *    [ in] str2: C string to be compared
 *    [ in] num:  number of characters to compare
 *
 * @return
 *    0 if both strings are equal; otherwise, not equal.
 */
static inline int synaptics_ts_pal_str_cmp(const char *str1, const char *str2,
		unsigned int num)
{
	return strncmp(str1, str2, num);
}
/**
 * synaptics_ts_pal_hex_to_uint()
 *
 * Convert the given string in hex to an integer returned
 *
 * @param
 *    [ in] str:    C string to be converted
 *    [ in] length: target length
 *
 * @return
 *    An integer converted
 */
static inline unsigned int synaptics_ts_pal_hex_to_uint(char *str, int length)
{
	unsigned int result = 0;
	char *ptr = NULL;

	for (ptr = str; ptr != str + length; ++ptr) {
		result <<= 4;
		if (*ptr >= 'A')
			result += *ptr - 'A' + 10;
		else
			result += *ptr - '0';
	}

	return result;
}

/**
 * @section: C Runtime for Checksum Calculation
 *
 * @brief: synaptics_ts_pal_crc32
 *         Calculates the CRC32 value
 */

/**
 * synaptics_ts_pal_crc32()
 *
 * Calculates the CRC32 value of the data
 *
 * @param
 *    [ in] seed: the previous crc32 value
 *    [ in] data: byte data for the calculation
 *    [ in] len:  the byte length of the data.
 *
 * @return
 *    0 if both strings are equal; otherwise, not equal.
 */
static inline unsigned int synaptics_ts_pal_crc32(unsigned int seed,
		const char *data, unsigned int len)
{
	return crc32(seed, data, len);
}

static inline int synaptics_ts_buf_alloc(struct synaptics_ts_buffer *pbuf,
		unsigned int size)
{
	if (!pbuf) {
		pr_err("[sec_input]Invalid buffer structure\n");
		return -1;
	}

	if (size > pbuf->buf_size) {
		if (pbuf->buf)
			synaptics_ts_pal_mem_free((void *)pbuf->buf);

		pbuf->buf = synaptics_ts_pal_mem_alloc(size, sizeof(unsigned char));
		if (!(pbuf->buf)) {
			pr_err("[sec_input]Fail to allocate memory (size = %d)\n",
				(int)(size*sizeof(unsigned char)));
			pbuf->buf_size = 0;
			pbuf->data_length = 0;
			return -1;
		}
		pbuf->buf_size = size;
	}

	synaptics_ts_pal_mem_set(pbuf->buf, 0x00, pbuf->buf_size);
	pbuf->data_length = 0;

	return 0;
}

/**
 * synaptics_ts_buf_realloc()
 *
 * Extend the requested memory space for the given buffer only if
 * the existed buffer is not enough for the requirement.
 * Then, move the content to the new memory space.
 *
 * @param
 *    [ in] pbuf: pointer to an internal buffer
 *    [ in] size: required size to be extended
 *
 * @return
 *     0 or positive value on success; otherwise, on error.
 */
static inline int synaptics_ts_buf_realloc(struct synaptics_ts_buffer *pbuf,
		unsigned int size)
{
	int retval;
	unsigned char *temp_src;
	unsigned int temp_size = 0;

	if (!pbuf) {
		pr_err("[sec_input]Invalid buffer structure\n");
		return -1;
	}

	if (size > pbuf->buf_size) {
		temp_src = pbuf->buf;
		temp_size = pbuf->buf_size;

		pbuf->buf = synaptics_ts_pal_mem_alloc(size, sizeof(unsigned char));
		if (!(pbuf->buf)) {
			pr_err("[sec_input]Fail to allocate memory (size = %d)\n",
				(int)(size * sizeof(unsigned char)));
			synaptics_ts_pal_mem_free((void *)temp_src);
			pbuf->buf_size = 0;
			return -1;
		}

		retval = synaptics_ts_pal_mem_cpy(pbuf->buf,
				size,
				temp_src,
				temp_size,
				temp_size);
		if (retval < 0) {
			pr_err("[sec_input]Fail to copy data\n");
			synaptics_ts_pal_mem_free((void *)temp_src);
			synaptics_ts_pal_mem_free((void *)pbuf->buf);
			pbuf->buf_size = 0;
			return retval;
		}

		synaptics_ts_pal_mem_free((void *)temp_src);
		pbuf->buf_size = size;
	}

	return 0;
}
/**
 * synaptics_ts_buf_init()
 *
 * Initialize the buffer structure.
 *
 * @param
 *    [ in] pbuf: pointer to an internal buffer
 *
 * @return
 *     none
 */
static inline void synaptics_ts_buf_init(struct synaptics_ts_buffer *pbuf)
{
	pbuf->buf_size = 0;
	pbuf->data_length = 0;
	pbuf->ref_cnt = 0;
	pbuf->buf = NULL;
	synaptics_ts_pal_mutex_alloc(&pbuf->buf_mutex);
}
/**
 * synaptics_ts_buf_lock()
 *
 * Protect the access of current buffer structure.
 *
 * @param
 *    [ in] pbuf: pointer to an internal buffer
 *
 * @return
 *     none
 */
static inline void synaptics_ts_buf_lock(struct synaptics_ts_buffer *pbuf)
{
	if (pbuf->ref_cnt != 0) {
		pr_err("[sec_input]Buffer access out-of balance, %d\n", pbuf->ref_cnt);
		return;
	}

	synaptics_ts_pal_mutex_lock(&pbuf->buf_mutex);
	pbuf->ref_cnt++;
}
/**
 * synaptics_ts_buf_unlock()
 *
 * Open the access of current buffer structure.
 *
 * @param
 *    [ in] pbuf: pointer to an internal buffer
 *
 * @return
 *     none
 */
static inline void synaptics_ts_buf_unlock(struct synaptics_ts_buffer *pbuf)
{
	if (pbuf->ref_cnt != 1) {
		pr_err("[sec_input]Buffer access out-of balance, %d\n", pbuf->ref_cnt);
		return;
	}

	pbuf->ref_cnt--;
	synaptics_ts_pal_mutex_unlock(&pbuf->buf_mutex);
}
/**
 * synaptics_ts_buf_release()
 *
 * Release the buffer structure.
 *
 * @param
 *    [ in] pbuf: pointer to an internal buffer
 *
 * @return
 *     none
 */
static inline void synaptics_ts_buf_release(struct synaptics_ts_buffer *pbuf)
{
	if (pbuf->ref_cnt != 0)
		pr_err("[sec_input]Buffer access hold, %d\n", pbuf->ref_cnt);

	synaptics_ts_pal_mutex_free(&pbuf->buf_mutex);
	synaptics_ts_pal_mem_free((void *)pbuf->buf);
	pbuf->buf_size = 0;
	pbuf->data_length = 0;
	pbuf->ref_cnt = 0;
}
/**
 * synaptics_ts_buf_copy()
 *
 * Helper to copy data from the source buffer to the destination buffer.
 * The size of destination buffer may be reallocated, if the size is
 * smaller than the actual data size to be copied.
 *
 * @param
 *    [out] dest: pointer to an internal buffer
 *    [ in] src:  required size to be extended
 *
 * @return
 *     0 or positive value on success; otherwise, on error.
 */
static inline int synaptics_ts_buf_copy(struct synaptics_ts_buffer *dest,
		struct synaptics_ts_buffer *src)
{
	int retval = 0;

	if (dest->buf_size < src->data_length) {
		retval = synaptics_ts_buf_alloc(dest, src->data_length + 1);
		if (retval < 0) {
			pr_err("[sec_input]Fail to reallocate the given buffer, size: %d\n",
				src->data_length + 1);
			return retval;
		}
	}

	/* copy data content to the destination */
	retval = synaptics_ts_pal_mem_cpy(dest->buf,
			dest->buf_size,
			src->buf,
			src->buf_size,
			src->data_length);
	if (retval < 0) {
		pr_err("[sec_input]Fail to copy data to caller, size: %d\n",
			src->data_length);
		return retval;
	}

	dest->data_length = src->data_length;

	return retval;
}


//core
int synaptics_ts_init(struct synaptics_ts_data *ts);
int synaptics_ts_probe(struct device *dev);
int synaptics_ts_remove(struct synaptics_ts_data *ts);
void synaptics_ts_shutdown(struct synaptics_ts_data *ts);
#if IS_ENABLED(CONFIG_PM)
int synaptics_ts_pm_suspend(struct device *dev);
int synaptics_ts_pm_resume(struct device *dev);
#endif
int synaptics_ts_stop_device(void *data);
int synaptics_ts_start_device(void *data);
irqreturn_t synaptics_ts_irq_thread(int irq, void *ptr);


void synaptics_ts_reinit(void *data);
int synaptics_ts_execute_autotune(struct synaptics_ts_data *ts, bool IsSaving);
int synaptics_ts_get_tsp_test_result(struct synaptics_ts_data *ts);
void synaptics_ts_release_all_finger(struct synaptics_ts_data *ts);
void synaptics_ts_locked_release_all_finger(struct synaptics_ts_data *ts);

int synaptics_ts_set_external_noise_mode(struct synaptics_ts_data *ts, u8 mode);
int synaptics_ts_get_version_info(struct synaptics_ts_data *ts);

#if IS_ENABLED(CONFIG_INPUT_SEC_SECURE_TOUCH)
irqreturn_t synaptics_secure_filter_interrupt(struct synaptics_ts_data *ts);
#endif

//fn
int synaptics_ts_read_from_sponge(struct synaptics_ts_data *ts, unsigned char *rd_buf, int size_buf,
		unsigned short offset, unsigned int rd_len);
int synaptics_ts_write_to_sponge(struct synaptics_ts_data *ts, unsigned char *wr_buf, int size_buf,
		unsigned short offset, unsigned int wr_len);
void synaptics_set_grip_data_to_ic(struct device *dev, u8 flag);
int synaptics_ts_set_temperature(struct device *dev, u8 temperature_data);
int synaptics_ts_set_lowpowermode(void *data, u8 mode);
int synaptics_ts_set_aod_rect(struct synaptics_ts_data *ts);
int synaptics_ts_set_aod_noti_rect(struct synaptics_ts_data *ts);
int synaptics_ts_setup(struct synaptics_ts_data *ts);
void synaptics_ts_hw_reset(struct synaptics_ts_data *ts);
void synaptics_ts_reset_work(struct work_struct *work);
void synaptics_ts_read_info_work(struct work_struct *work);
void synaptics_ts_print_info_work(struct work_struct *work);
void synaptics_ts_notify_pen_hover_work(struct work_struct *work);
void synatpics_ts_check_mode_work(struct work_struct *work);
int get_nvm_data_by_size(struct synaptics_ts_data *ts, u8 offset, int length, u8 *nvdata);
int get_nvm_data(struct synaptics_ts_data *ts, int type, u8 *nvdata);
int synaptics_ts_set_custom_library(struct synaptics_ts_data *ts);
void synaptics_ts_get_custom_library(struct synaptics_ts_data *ts);
void synaptics_ts_set_fod_finger_merge(struct synaptics_ts_data *ts);
int synaptics_ts_set_fod_rect(struct synaptics_ts_data *ts);
int synaptics_ts_set_touchable_area(struct synaptics_ts_data *ts);
int synaptics_ts_ear_detect_enable(struct synaptics_ts_data *ts, u8 enable);
int synaptics_ts_pocket_mode_enable(struct synaptics_ts_data *ts, u8 enable);
int synaptics_ts_low_sensitivity_mode_enable(struct synaptics_ts_data *ts, u16 enable);
int synaptics_ts_set_charger_mode(struct device *dev, bool on);
void synaptics_ts_set_cover_type(struct synaptics_ts_data *ts, bool enable);
int synaptics_ts_set_press_property(struct synaptics_ts_data *ts);
int get_nvm_data(struct synaptics_ts_data *ts, int type, u8 *nvdata);
int set_nvm_data(struct synaptics_ts_data *ts, u8 type, u8 *buf);
int get_nvm_data_by_size(struct synaptics_ts_data *ts, u8 offset, int length, u8 *nvdata);
int set_nvm_data_by_size(struct synaptics_ts_data *ts, u8 offset, int length, u8 *buf);
int synaptics_ts_set_touch_function(struct synaptics_ts_data *ts);
void synaptics_ts_get_touch_function(struct work_struct *work);
int synaptics_ts_detect_protocol(struct synaptics_ts_data *ts);
int synaptics_ts_set_up_app_fw(struct synaptics_ts_data *ts);
inline unsigned int synaptics_ts_pal_le2_to_uint(const unsigned char *src);
inline unsigned int synaptics_ts_pal_le4_to_uint(const unsigned char *src);
int synaptics_ts_switch_fw_mode(struct synaptics_ts_data *ts, unsigned char mode);
int synaptics_ts_get_boot_info(struct synaptics_ts_data *ts, struct synaptics_ts_boot_info *boot_info);
inline int synaptics_ts_memcpy(void *dest, unsigned int dest_size,
		const void *src, unsigned int src_size, unsigned int num);
int synaptics_ts_v1_parse_idinfo(struct synaptics_ts_data *ts,
		unsigned char *data, unsigned int size, unsigned int data_len);
inline void synaptics_ts_buffer_size_set(struct synaptics_ts_buffer *pbuf, unsigned int size);
int synaptics_ts_soft_reset(struct synaptics_ts_data *ts);
int synaptics_ts_rezero(struct synaptics_ts_data *ts);
int synaptics_ts_get_app_info(struct synaptics_ts_data *ts, struct synaptics_ts_application_info *app_info);
int synaptics_ts_send_command(struct synaptics_ts_data *ts,
			unsigned char command, unsigned char *payload,
			unsigned int payload_length, unsigned char *resp_code,
			struct synaptics_ts_buffer *resp, unsigned int delay_ms_resp);
int synaptics_ts_send_immediate_command(struct synaptics_ts_data *ts,
			unsigned char command, unsigned char *payload,
			unsigned int payload_length);
int synaptics_ts_calibration(struct synaptics_ts_data *ts);
int synaptics_ts_set_dynamic_config(struct synaptics_ts_data *ts,
		unsigned char id, unsigned short value);
int synaptics_ts_set_immediate_dynamic_config(struct synaptics_ts_data *ts,
		unsigned char id, unsigned short value);
int synaptics_ts_clear_buffer(struct synaptics_ts_data *ts);
int synaptics_ts_tclm_read(struct device *dev, int address);
int synaptics_ts_tclm_write(struct device *dev, int address);
int synaptics_ts_tclm_execute_force_calibration(struct device *dev, int cal_mode);
int synaptics_ts_set_up_rawdata_report_type(struct synaptics_ts_data *ts);
int synaptics_ts_enable_report(struct synaptics_ts_data *ts, unsigned char report_code, bool en);
void synaptics_ts_external_func_work(struct work_struct *work);

//cmd
void synaptics_ts_fn_remove(struct synaptics_ts_data *ts);
int synaptics_ts_fn_init(struct synaptics_ts_data *ts);
int synaptics_ts_panel_ito_test(struct synaptics_ts_data *ts, int tesynapticsode);
void synaptics_ts_run_rawdata_all(struct synaptics_ts_data *ts);
int synaptics_ts_run_production_test(struct synaptics_ts_data *ts,
		unsigned char test_item, struct synaptics_ts_buffer *tdata);
void synaptics_ts_rawdata_read_all(struct synaptics_ts_data *ts);


//dump
#if IS_ENABLED(CONFIG_TOUCHSCREEN_DUMP_MODE)
void synaptics_ts_check_rawdata(struct work_struct *work);
void synaptics_ts_dump_tsp_log(struct device *dev);
void synaptics_ts_sponge_dump_flush(struct synaptics_ts_data *ts, int dump_area);
#endif
ssize_t get_lp_dump_show(struct device *dev, struct device_attribute *attr, char *buf);

//fw
int synaptics_ts_fw_update_on_probe(struct synaptics_ts_data *ts);
int synaptics_ts_fw_update_on_hidden_menu(struct synaptics_ts_data *ts, int update_type);
int synaptics_ts_wait_for_echo_event(struct synaptics_ts_data *ts, u8 *cmd, u8 cmd_cnt, int delay);
int synaptics_ts_fw_wait_for_event(struct synaptics_ts_data *ts, u8 *result, u8 result_cnt);
void synaptics_ts_checking_miscal(struct synaptics_ts_data *ts);
void synaptics_ts_external_func(struct synaptics_ts_data *ts);

/* for vendor */
#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
int syna_cdev_create_sysfs(struct synaptics_ts_data *ts);
void syna_cdev_remove_sysfs(struct synaptics_ts_data *ts);
void syna_cdev_redirect_attn(struct synaptics_ts_data *ts);
#ifdef ENABLE_EXTERNAL_FRAME_PROCESS
void syna_cdev_update_report_queue(struct synaptics_ts_data *ts,
		unsigned char code, struct synaptics_ts_buffer *pevent_data);
#endif
#endif

//ic_setting
void synaptics_ts_fw_delay_setting(struct synaptics_ts_data *ts);
int synaptics_ts_fw_resp_delay_ms(struct synaptics_ts_data *ts,
	unsigned int xfer_length, unsigned int wr_delay_ms, unsigned int num_blocks);
int synaptics_ts_fw_set_page_count(struct synaptics_ts_data *ts,
			struct synaptics_ts_reflash_data_blob *reflash_data, unsigned int size);
int synaptics_ts_fw_set_erase_delay(struct synaptics_ts_data *ts,
		unsigned int erase_delay_ms, unsigned int page_count);

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
extern int panel_notifier_register(struct notifier_block *nb);
extern int panel_notifier_unregister(struct notifier_block *nb);
#endif

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
extern int stui_spi_lock(struct spi_master *spi);
extern int stui_spi_unlock(struct spi_master *spi);
#endif

#endif /* _LINUX_synaptics_ts_H_ */

