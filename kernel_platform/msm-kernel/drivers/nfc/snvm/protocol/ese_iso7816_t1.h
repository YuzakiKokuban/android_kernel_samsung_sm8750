/*
 * Copyright (C) 2020 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/********************* Definitions and structures *****************************/

#ifndef __ESE_PROTOCOL_T1__H
#define __ESE_PROTOCOL_T1__H

#include <linux/delay.h>
#include <linux/slab.h>

#include "ese_protocol.h"
#include "ese_data.h"

typedef enum sframe_type_e {
	SFRAME_RESYNCH_REQ = 0x00,
	SFRAME_RESYNCH_RSP = 0x20,
	SFRAME_IFSC_REQ = 0x01,
	SFRAME_IFSC_RES = 0x21,
	SFRAME_ABORT_REQ = 0x02,
	SFRAME_ABORT_RES = 0x22,
	SFRAME_WTX_REQ = 0x03,
	SFRAME_WTX_RSP = 0x23,
	SFRAME_INVALID_REQ_RES
} sframe_type_t;

typedef enum rframe_type_e {
	RFRAME_ACK = 0x01,
	RFRAME_NACK = 0x02
} rframe_type_t;

typedef enum rframe_error_e {
	RFRAME_ERROR_NO,
	RFRAME_ERROR_PARITY,
	RFRAME_ERROR_OTHER,
	RFRAME_ERROR_SOF_MISSED,
	RFRAME_ERROR_UNDEFINED
} rframe_error_t;

typedef enum frame_type_e {
	FRAME_IFRAME,
	FRAME_SFRAME,
	FRAME_RFRAME,
	FRAME_INVALID,
	FRAME_UNKNOWN
} frame_type_t;

typedef struct iframe_info_s {
	uint8_t *data;
	uint32_t offset;
	uint32_t total_size;
	uint32_t send_size;
	uint8_t chain;
	uint8_t seq_no;
} iframe_info_t;

typedef struct sframe_info_s {
	uint8_t *data;
	uint32_t send_size;
	sframe_type_t type;
} sframe_info_t;

typedef struct rframe_info_s {
	uint8_t seq_no;
	rframe_error_t error;
} rframe_info_t;

typedef struct frame_info_s {
	iframe_info_t iframe;
	rframe_info_t rframe;
	sframe_info_t sframe;
	frame_type_t type;
} frame_info_t;

typedef enum state_e {
	STATE_IDLE,
	STATE_SEND_IFRAME,
	STATE_SEND_RFRAME,
	STATE_SEND_SFRAME,
} state_t;

typedef struct iso7816_t1_s {
	frame_info_t last_tx;
	frame_info_t next_tx;
	frame_info_t last_rx;
	data_list_t recv_data;
	uint8_t *proc_buf;
	uint8_t *cur_buf;
	uint32_t cur_size;
	uint32_t wtx_counter;
	uint32_t timeout_counter;
	uint32_t rnack_counter;
	uint32_t recovery_counter;
	frame_type_t last_frame;
	state_t state;
	uint8_t send_address;
	uint8_t receive_address;
	void *hal;
} iso7816_t1_t;

#define PROTOCOL_HEADER_SIZE			0x03
#define PROTOCOL_LRC_SIZE				0x01
#define PROTOCOL_ZERO					0x00
#define PROTOCOL_ADDRESS_POLLING_COUNT	100
#define PROTOCOL_TIMEOUT_RETRY_COUNT	3
#define PROTOCOL_PCB_OFFSET				1
#define PROTOCOL_SEND_SIZE				254
#define PROTOCOL_CHAINING				0x20
#define PROTOCOL_S_BLOCK_REQ			0xC0
#define PROTOCOL_S_BLOCK_RSP			0xE0
#define PROTOCOL_FRAME_RETRY_COUNT		3
#define PROTOCOL_MAX_RNACK_RETRY_LIMIT	2
#define PROTOCOL_UDELAY(x)				usleep_range(x, (x + 100))

#define SFRAME_MAX_INF_SIZE				(4)
#define RFRAME_PROTOCOL_SIZE			(PROTOCOL_HEADER_SIZE + PROTOCOL_LRC_SIZE)

#define APDU_HEADER_SIZE				(5)
#define APDU_P3_OFFSET					(4)

#define ESE_BIT(val, bit)				((val >> bit) & 0x1)

ESE_STATUS iso7816_t1_send(void *ctx, ese_data_t *data);
ESE_STATUS iso7816_t1_receive(void *ctx, ese_data_t *data);
ESE_STATUS iso7816_t1_resync_request(void *ctx);
ESE_STATUS iso7816_t1_resync_response(void *ctx);
ESE_STATUS iso7816_t1_wtx_request(void *ctx, uint32_t time);
ESE_STATUS iso7816_t1_wtx_response(void *ctx);
ESE_STATUS iso7816_t1_get_data(void *ctx, ese_data_t *data, uint8_t *le);
void *iso7816_t1_init(uint8_t send_address, uint8_t receive_address, void *hal);
void iso7816_t1_deinit(void *ctx);
void iso7816_t1_reset(void *ctx);

#endif
