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

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/printk.h>

#include "../hal/ese_hal.h"
#include "ese_log.h"
#include "ese_memory.h"
#include "ese_error.h"
#include "ese_protocol.h"
#include "ese_iso7816_t1.h"

static void iso7816_t1_print_buffer(const char *buf_tag, uint8_t *buffer, uint32_t buffer_size)
{
	print_hex_dump(KERN_DEBUG, buf_tag, DUMP_PREFIX_NONE, 16, 1, buffer, buffer_size, 0);
}

static uint8_t iso7816_t1_compute_lrc(uint8_t *data, uint32_t offset, uint32_t size)
{
	uint32_t lrc = 0, i = 0;

	for (i = offset; i < offset + size; i++) {
		lrc = lrc ^ data[i];
	}

	return (uint8_t) lrc;
}

static ESE_STATUS iso7816_t1_check_lrc(uint8_t* data, uint32_t data_size)
{
	uint8_t calc_crc = 0;
	uint8_t recv_crc = 0;

	recv_crc = data[data_size - 1];
	calc_crc = iso7816_t1_compute_lrc(data, 0, (data_size - 1));
	if (recv_crc != calc_crc) {
		return ESESTATUS_FAILED;
	}
	return ESESTATUS_SUCCESS;
}

static void iso7816_t1_reset_params(iso7816_t1_t *protocol)
{
	protocol->state = STATE_IDLE;
	protocol->last_rx.type = FRAME_INVALID;
	protocol->next_tx.type = FRAME_INVALID;
	protocol->next_tx.iframe.data = NULL;
	protocol->last_tx.type = FRAME_INVALID;
	protocol->last_tx.iframe.data = NULL;
	protocol->next_tx.iframe.seq_no = 0x01;
	protocol->last_rx.iframe.seq_no = 0x01;
	protocol->last_tx.iframe.seq_no = 0x01;
	protocol->next_tx.sframe.data = NULL;
	protocol->next_tx.sframe.send_size = 0;
	protocol->recovery_counter = 0;
	protocol->timeout_counter = 0;
	protocol->wtx_counter = 0;
	protocol->last_frame = FRAME_UNKNOWN;
	protocol->rnack_counter = 0;
}

static void iso7816_t1_set_iframe(iso7816_t1_t *protocol)
{
	protocol->next_tx.iframe.offset = 0;
	protocol->next_tx.type = FRAME_IFRAME;
	protocol->next_tx.iframe.seq_no = protocol->last_tx.iframe.seq_no ^ 1;
	protocol->state = STATE_SEND_IFRAME;

	if (protocol->next_tx.iframe.total_size > PROTOCOL_SEND_SIZE) {
		protocol->next_tx.iframe.chain = 1;
		protocol->next_tx.iframe.send_size = PROTOCOL_SEND_SIZE;
		protocol->next_tx.iframe.total_size =
			protocol->next_tx.iframe.total_size - PROTOCOL_SEND_SIZE;
	} else {
		protocol->next_tx.iframe.send_size = protocol->next_tx.iframe.total_size;
		protocol->next_tx.iframe.chain = 0;
	}
}

static void iso7816_t1_next_iframe(iso7816_t1_t *protocol)
{
	protocol->next_tx.type = FRAME_IFRAME;
	protocol->state = STATE_SEND_IFRAME;

	protocol->next_tx.iframe.seq_no = protocol->last_tx.iframe.seq_no ^ 1;
	protocol->next_tx.iframe.offset =
		protocol->last_tx.iframe.offset + PROTOCOL_SEND_SIZE;
	protocol->next_tx.iframe.data = protocol->last_tx.iframe.data;

	if (protocol->last_tx.iframe.total_size > PROTOCOL_SEND_SIZE) {
		protocol->next_tx.iframe.chain = 1;
		protocol->next_tx.iframe.send_size = PROTOCOL_SEND_SIZE;
		protocol->next_tx.iframe.total_size =
			protocol->last_tx.iframe.total_size - PROTOCOL_SEND_SIZE;
	} else {
		protocol->next_tx.iframe.chain = 0;
		protocol->next_tx.iframe.send_size = protocol->last_tx.iframe.total_size;
	}
}

static void iso7816_t1_finish_recovery(iso7816_t1_t *protocol)
{
	/*
	 * ToDo : need to reset interface
	 */
	LOG_E("finish recovery, set protocol state to IDLE");
	protocol->state = STATE_IDLE;
	protocol->recovery_counter = PROTOCOL_ZERO;
}

static ESE_STATUS iso7816_t1_send_sframe(iso7816_t1_t *protocol)
{
	uint32_t frame_size = (PROTOCOL_HEADER_SIZE + PROTOCOL_LRC_SIZE);
	uint8_t buf[PROTOCOL_HEADER_SIZE + PROTOCOL_LRC_SIZE + SFRAME_MAX_INF_SIZE];
	uint8_t pcb_byte = 0;

	protocol->last_frame = FRAME_SFRAME;
	if ((protocol->next_tx.sframe.type & 0x20 )== 0) {
		buf[2] = 0x00;

		pcb_byte = PROTOCOL_S_BLOCK_REQ | protocol->next_tx.sframe.type;

		if (protocol->next_tx.sframe.data && protocol->next_tx.sframe.send_size > 0) {
			if (protocol->next_tx.sframe.send_size > 4)
				protocol->next_tx.sframe.send_size = 4;
			buf[2] = protocol->next_tx.sframe.send_size;
			memcpy(buf + PROTOCOL_HEADER_SIZE,
					protocol->next_tx.sframe.data, protocol->next_tx.sframe.send_size);
			frame_size += protocol->next_tx.sframe.send_size;
			protocol->next_tx.sframe.send_size = 0;
		}
	} else if ((protocol->next_tx.sframe.type & 0x20) == 0x20) {
		buf[2] = 0x00;

		pcb_byte = PROTOCOL_S_BLOCK_RSP | (protocol->next_tx.sframe.type & 0x0F);

		if (protocol->next_tx.sframe.data && protocol->next_tx.sframe.send_size > 0) {
			if (protocol->next_tx.sframe.send_size > 4)
				protocol->next_tx.sframe.send_size = 4;
			buf[2] = protocol->next_tx.sframe.send_size;
			memcpy(buf + PROTOCOL_HEADER_SIZE,
					protocol->next_tx.sframe.data, protocol->next_tx.sframe.send_size);
			frame_size += protocol->next_tx.sframe.send_size;
			protocol->next_tx.sframe.send_size = 0;
		}
	} else {
		LOG_E("Invalid SFrame");
	}

	buf[0] = protocol->send_address;
	buf[1] = pcb_byte;
	buf[frame_size - 1] = iso7816_t1_compute_lrc(buf, 0, (frame_size - 1));

	iso7816_t1_print_buffer("[star-protocol] S_SFRAME: ", buf, frame_size);
	if ((uint32_t)ese_hal_send(protocol->hal, buf, frame_size) != frame_size) {
		return ESESTATUS_SEND_FAILED;
	}
	return ESESTATUS_SUCCESS;
}

static ESE_STATUS iso7816_t1_send_rframe(iso7816_t1_t *protocol)
{
	uint8_t buf[RFRAME_PROTOCOL_SIZE] = {0x00, 0x80, 0x00, 0x00};

	if (protocol->next_tx.rframe.error != RFRAME_ERROR_NO) {
		buf[1] |= protocol->next_tx.rframe.error;
	} else {
		buf[1] = 0x80;
		protocol->last_frame = FRAME_RFRAME;
	}

	buf[0] = protocol->send_address;
	buf[1] |= ((protocol->last_rx.iframe.seq_no ^ 1) << 4);
	buf[2] = 0x00;
	buf[3] = iso7816_t1_compute_lrc(buf, 0x00, RFRAME_PROTOCOL_SIZE - 1);

	iso7816_t1_print_buffer("[star-protocol] S_RFRAME: ", buf, RFRAME_PROTOCOL_SIZE);
	if ((uint32_t)ese_hal_send(protocol->hal, buf, RFRAME_PROTOCOL_SIZE) != RFRAME_PROTOCOL_SIZE) {
		return ESESTATUS_SEND_FAILED;
	}

	return ESESTATUS_SUCCESS;
}

static ESE_STATUS iso7816_t1_send_iframe(iso7816_t1_t *protocol)
{
	uint32_t frame_size = 0;
	uint8_t pcb_byte = 0;

	if (protocol->proc_buf == NULL) {
		LOG_E("Process Buffer is NULL, INVALID");
		return ESESTATUS_NOT_ENOUGH_MEMORY;
	}

	if (protocol->next_tx.iframe.send_size == 0) {
		LOG_E("Iframe Len is 0, INVALID");
		return ESESTATUS_INVALID_SEND_LENGTH;
	}

	if (protocol->next_tx.iframe.data == NULL) {
		LOG_E("Iframe Buffer is NULL, INVALID");
		return ESESTATUS_NOT_ENOUGH_MEMORY;
	}

	protocol->last_frame = FRAME_IFRAME;
	frame_size = (protocol->next_tx.iframe.send_size + PROTOCOL_HEADER_SIZE + PROTOCOL_LRC_SIZE);

	(protocol->proc_buf)[0] = protocol->send_address;
	if (protocol->next_tx.iframe.chain) {
		pcb_byte |= PROTOCOL_CHAINING;
	}

	pcb_byte |= (protocol->next_tx.iframe.seq_no << 6);
	(protocol->proc_buf)[1] = pcb_byte;
	(protocol->proc_buf)[2] = protocol->next_tx.iframe.send_size;
	memcpy(protocol->proc_buf + 3,
			protocol->next_tx.iframe.data + protocol->next_tx.iframe.offset,
			protocol->next_tx.iframe.send_size);
	(protocol->proc_buf)[frame_size - 1] = iso7816_t1_compute_lrc(protocol->proc_buf, 0, (frame_size - 1));

	iso7816_t1_print_buffer("[star-protocol] S_IFRAME: ", protocol->proc_buf, PROTOCOL_HEADER_SIZE);
	if ((uint32_t)ese_hal_send(protocol->hal, protocol->proc_buf, frame_size) != frame_size) {
		return ESESTATUS_SEND_FAILED;
	}

	return ESESTATUS_SUCCESS;
}

static ESE_STATUS iso7816_t1_decode_frame(iso7816_t1_t *protocol, uint8_t* data, uint32_t data_size)
{
	ESE_STATUS status = ESESTATUS_FAILED;
	uint8_t pcb_byte;
	int32_t frame_type;
	uint32_t wait_time = 0;

	pcb_byte = data[PROTOCOL_PCB_OFFSET];

	if (ESE_BIT(pcb_byte, 7) == 0x00) {
		iso7816_t1_print_buffer("[star-protocol] R_IFRAME: ", data, PROTOCOL_HEADER_SIZE);
		protocol->wtx_counter = 0;
		protocol->last_rx.type = FRAME_IFRAME;
		if (protocol->last_rx.iframe.seq_no != ESE_BIT(pcb_byte, 6)) {
			protocol->recovery_counter = 0;
			protocol->last_rx.iframe.seq_no = 0x00;
			protocol->last_rx.iframe.seq_no |= ESE_BIT(pcb_byte, 6);

			if (ESE_BIT(pcb_byte, 5)) {
				protocol->last_rx.iframe.chain = 1;
				protocol->next_tx.type = FRAME_RFRAME;
				protocol->next_tx.rframe.error = RFRAME_ERROR_NO;
				protocol->state = STATE_SEND_RFRAME;
			} else {
				protocol->last_rx.iframe.chain = 0;
				protocol->state = STATE_IDLE;
			}

			status = ese_data_store(&(protocol->recv_data), &data[3], data_size - 4, 1);
			if (status != ESESTATUS_SUCCESS) {
				return status;
			}
		} else {
			if (protocol->recovery_counter < PROTOCOL_FRAME_RETRY_COUNT) {
				protocol->next_tx.type = FRAME_RFRAME;
				protocol->next_tx.rframe.error = RFRAME_ERROR_OTHER;
				protocol->state = STATE_SEND_RFRAME;
				protocol->recovery_counter++;
			} else {
				iso7816_t1_finish_recovery(protocol);
			}
		}
	} else if ((ESE_BIT(pcb_byte, 7) == 0x01) && (ESE_BIT(pcb_byte, 6) == 0x00)) {
		iso7816_t1_print_buffer("[star-protocol] R_RFRAME: ", data, data_size);
		protocol->wtx_counter = 0;
		protocol->last_rx.type = FRAME_RFRAME;
		protocol->last_rx.rframe.seq_no = ESE_BIT(pcb_byte, 4);

		if ((ESE_BIT(pcb_byte, 0) == 0x00) && (ESE_BIT(pcb_byte, 1) == 0x00)) {
			protocol->last_rx.rframe.error = RFRAME_ERROR_NO;
			protocol->recovery_counter = 0;
			if (protocol->last_rx.rframe.seq_no !=
				protocol->last_tx.iframe.seq_no) {
				protocol->state = STATE_SEND_IFRAME;
				iso7816_t1_next_iframe(protocol);
			} else {
				/*
				 * ToDo : error handling.
				 */
			}
		} else if (((ESE_BIT(pcb_byte, 0) == 0x01) && (ESE_BIT(pcb_byte, 1) == 0x00)) ||
			((ESE_BIT(pcb_byte, 0) == 0x00) && (ESE_BIT(pcb_byte, 1) == 0x01))) {
			if ((ESE_BIT(pcb_byte, 0) == 0x00) && (ESE_BIT(pcb_byte, 1) == 0x01))
				protocol->last_rx.rframe.error = RFRAME_ERROR_OTHER;
			else
				protocol->last_rx.rframe.error = RFRAME_ERROR_PARITY;
			if (protocol->recovery_counter < PROTOCOL_FRAME_RETRY_COUNT) {
				if (protocol->last_tx.type == FRAME_IFRAME) {
					memcpy((uint8_t *)&protocol->next_tx,
						(uint8_t *)&protocol->last_tx, sizeof(frame_info_t));
					protocol->state = STATE_SEND_IFRAME;
					protocol->next_tx.type = FRAME_IFRAME;
				} else if (protocol->last_tx.type == FRAME_RFRAME) {
					if ((protocol->last_rx.rframe.seq_no ==
						protocol->last_tx.iframe.seq_no) &&
						(protocol->last_frame == FRAME_IFRAME)) {
						/*
						 * Usecase to reach the below case:
						 * I-frame sent first, followed by R-NACK and we receive a R-NACK with
						 * last sent I-frame sequence number
						 */
						memcpy((uint8_t *)&protocol->next_tx,
								(uint8_t *)&protocol->last_tx, sizeof(frame_info_t));
						protocol->state = STATE_SEND_IFRAME;
						protocol->next_tx.type = FRAME_IFRAME;
					} else if ((protocol->last_rx.rframe.seq_no !=
							protocol->last_tx.iframe.seq_no) &&
							(protocol->last_frame == FRAME_RFRAME)) {
						/*
						 * Usecase to reach the below case:
						 * R-frame sent first, followed by R-NACK and we receive a R-NACK with
						 * next expected I-frame sequence number
						 */
						protocol->next_tx.type = FRAME_RFRAME;
						protocol->next_tx.rframe.error = RFRAME_ERROR_NO;
						protocol->state = STATE_SEND_RFRAME;
					} else {
						/*
						 * Usecase to reach the below case:
						 * I-frame sent first, followed by R-NACK and we receive a R-NACK with
						 * next expected I-frame sequence number + all the other unexpected
						 * scenarios
						 */
						protocol->next_tx.type = FRAME_RFRAME;
						protocol->next_tx.rframe.error = RFRAME_ERROR_OTHER;
						protocol->state = STATE_SEND_RFRAME;
					}
				} else if (protocol->last_tx.type == FRAME_SFRAME) {
					memcpy((uint8_t *)&protocol->next_tx,
						(uint8_t *)&protocol->last_tx, sizeof(frame_info_t));
				}
				protocol->recovery_counter++;
			} else {
				iso7816_t1_finish_recovery(protocol);
			}
		} else if ((ESE_BIT(pcb_byte, 0) == 0x01) && (ESE_BIT(pcb_byte, 1) == 0x01)) {
			if (protocol->recovery_counter < PROTOCOL_FRAME_RETRY_COUNT) {
				protocol->last_rx.rframe.error = RFRAME_ERROR_SOF_MISSED;
				memcpy((uint8_t *)&protocol->next_tx,
					(uint8_t *)&protocol->last_tx, sizeof(frame_info_t));
				protocol->recovery_counter++;
			} else {
				iso7816_t1_finish_recovery(protocol);
			}
		} else {
			if (protocol->recovery_counter < PROTOCOL_FRAME_RETRY_COUNT) {
				protocol->last_rx.rframe.error = RFRAME_ERROR_UNDEFINED;
				memcpy((uint8_t *)&protocol->next_tx,
					(uint8_t *)&protocol->last_tx, sizeof(frame_info_t));
				protocol->recovery_counter++;
			} else {
				iso7816_t1_finish_recovery(protocol);
			}
		}
	} else if ((ESE_BIT(pcb_byte, 7) == 0x01) && (ESE_BIT(pcb_byte, 6) == 0x01)) {
		iso7816_t1_print_buffer("[star-protocol] R_SFRAME: ", data, data_size);
		frame_type = (int32_t)(pcb_byte & 0x3F);
		protocol->last_rx.type = FRAME_SFRAME;
		if (frame_type != SFRAME_WTX_REQ) {
			protocol->wtx_counter = 0;
		}
		switch (frame_type) {
		case SFRAME_RESYNCH_REQ:
			iso7816_t1_reset_params(protocol);
			protocol->last_rx.sframe.type = SFRAME_RESYNCH_REQ;
			protocol->next_tx.type = FRAME_SFRAME;
			protocol->next_tx.sframe.type = SFRAME_RESYNCH_RSP;
			protocol->state = STATE_SEND_SFRAME;
			protocol->next_tx.sframe.data = NULL;
			protocol->next_tx.sframe.send_size = 0;
			break;
		case SFRAME_RESYNCH_RSP:
			protocol->last_rx.sframe.type = SFRAME_RESYNCH_RSP;
			protocol->next_tx.type = FRAME_UNKNOWN;
			protocol->state = STATE_IDLE;
			break;
		case SFRAME_ABORT_REQ:
			protocol->last_rx.sframe.type = SFRAME_ABORT_REQ;
			break;
		case SFRAME_ABORT_RES:
			protocol->last_rx.sframe.type = SFRAME_ABORT_RES;
			protocol->next_tx.type = FRAME_UNKNOWN;
			protocol->state = STATE_IDLE;
			break;
		case SFRAME_WTX_REQ:
			if (protocol->last_tx.type == FRAME_SFRAME &&
					protocol->last_tx.sframe.type != SFRAME_WTX_RSP) {
				if (protocol->recovery_counter < PROTOCOL_FRAME_RETRY_COUNT) {
					memcpy((uint8_t *)&protocol->next_tx,
							(uint8_t *)&protocol->last_tx, sizeof(frame_info_t));
					protocol->recovery_counter++;
				} else {
					iso7816_t1_finish_recovery(protocol);
				}
			} else {
				protocol->wtx_counter++;
				if (data_size == 8) {
					wait_time = data[3] << 24 | data[4] << 16 | data[5] << 8 | data[6];
					wait_time = wait_time * 1000;
				}
				LOG_I("wtx_counter : %u, wait time : %u us", protocol->wtx_counter, wait_time);
				PROTOCOL_UDELAY(wait_time);
				protocol->last_rx.sframe.type = SFRAME_WTX_REQ;
				protocol->next_tx.type = FRAME_SFRAME;
				protocol->next_tx.sframe.type = SFRAME_WTX_RSP;
				protocol->next_tx.sframe.data = NULL;
				protocol->next_tx.sframe.send_size = 0;
				protocol->state = STATE_SEND_SFRAME;
			}
			break;
		case SFRAME_WTX_RSP:
			protocol->last_rx.sframe.type = SFRAME_WTX_RSP;
			break;
		default:
			break;
		}
	} else {
		LOG_E("Wrong-Frame Received");
		return ESESTATUS_INVALID_FORMAT;
	}

	return ESESTATUS_SUCCESS;
}

static ESE_STATUS iso7816_t1_process(iso7816_t1_t *protocol)
{
	uint32_t data_size = 0, rec_size = 0, ret_size = 0;
	ESE_STATUS status = ESESTATUS_FAILED;
	int32_t i = 0;

	PROTOCOL_UDELAY(100);
	for (i = 0; i < PROTOCOL_ADDRESS_POLLING_COUNT; i++) {
		if (ese_hal_receive(protocol->hal, protocol->proc_buf, 1) != 1) {
			status = ESESTATUS_INVALID_RECEIVE_LENGTH;
			goto error;
		}

		if ((protocol->proc_buf)[data_size] == protocol->receive_address) {
			break;
		}

		if ((protocol->proc_buf)[data_size] != 0x0) {
			LOG_E("invalid address, expected : %d, received : %d",
				protocol->receive_address, (protocol->proc_buf)[data_size]);
			status = ESESTATUS_INVALID_FRAME;
			goto error;
		}
		PROTOCOL_UDELAY(1500);
	}

	if (i == PROTOCOL_ADDRESS_POLLING_COUNT) {
		LOG_E("finish polling address, limit : %d", PROTOCOL_ADDRESS_POLLING_COUNT);
		status = ESESTATUS_INVALID_FRAME;
		goto error;
	}

	data_size ++;

	ret_size = (uint32_t)ese_hal_receive(protocol->hal, protocol->proc_buf + data_size, 2);
	if (ret_size != 2) {
		LOG_E("mismatch receive size %u, %u", 2, ret_size);
		status = ESESTATUS_INVALID_RECEIVE_LENGTH;
		goto error;
	}
	data_size += ret_size;

	rec_size = (protocol->proc_buf)[2] + 1;
	ret_size = (uint32_t)ese_hal_receive(protocol->hal, protocol->proc_buf + data_size, rec_size);
	if (ret_size != rec_size) {
		LOG_E("mismatch receive size %u, %u", rec_size, ret_size);
		status = ESESTATUS_INVALID_RECEIVE_LENGTH;
		goto error;
	}
	data_size += ret_size;

error:
	if (data_size > 0) {
		protocol->timeout_counter = PROTOCOL_ZERO;
		status = iso7816_t1_check_lrc(protocol->proc_buf, data_size);
		if (status == ESESTATUS_SUCCESS) {
			protocol->rnack_counter = PROTOCOL_ZERO;
			iso7816_t1_decode_frame(protocol, protocol->proc_buf, data_size);
		} else {
			LOG_E("LRC Check failed");
			if (protocol->rnack_counter < PROTOCOL_MAX_RNACK_RETRY_LIMIT) {
				protocol->last_rx.type = FRAME_INVALID;
				protocol->next_tx.type = FRAME_RFRAME;
				protocol->next_tx.rframe.error = RFRAME_ERROR_PARITY;
				protocol->next_tx.rframe.seq_no =
					(!protocol->last_rx.iframe.seq_no) << 4;
				protocol->state = STATE_SEND_RFRAME;
				protocol->rnack_counter++;
			} else {
				protocol->rnack_counter = PROTOCOL_ZERO;
				protocol->state = STATE_IDLE;
			}
		}
	} else {
		LOG_E("ese_hal_receive failed");
		if ((protocol->last_tx.type == FRAME_SFRAME)
			&& ((protocol->last_tx.sframe.type == SFRAME_WTX_RSP) ||
			(protocol->last_tx.sframe.type == SFRAME_RESYNCH_RSP))) {
			if (protocol->rnack_counter < PROTOCOL_MAX_RNACK_RETRY_LIMIT) {
				protocol->last_rx.type = FRAME_INVALID;
				protocol->next_tx.type = FRAME_RFRAME;
				protocol->next_tx.rframe.error = RFRAME_ERROR_OTHER;
				protocol->next_tx.rframe.seq_no =
					(!protocol->last_rx.iframe.seq_no) << 4;
				protocol->state = STATE_SEND_RFRAME;
				protocol->rnack_counter++;
			} else {
				protocol->rnack_counter = PROTOCOL_ZERO;
				protocol->state = STATE_IDLE;
				protocol->timeout_counter = PROTOCOL_ZERO;
			}
		} else {
			PROTOCOL_UDELAY(50 * 1000);
			if (status == ESESTATUS_INVALID_FRAME) {
				if (protocol->rnack_counter < PROTOCOL_MAX_RNACK_RETRY_LIMIT) {
					protocol->last_rx.type = FRAME_INVALID;
					protocol->next_tx.type = FRAME_RFRAME;
					protocol->next_tx.rframe.error = RFRAME_ERROR_OTHER;
					protocol->next_tx.rframe.seq_no =
						(!protocol->last_rx.iframe.seq_no) << 4;
					protocol->state = STATE_SEND_RFRAME;
					protocol->rnack_counter++;
				} else {
					protocol->rnack_counter = PROTOCOL_ZERO;
					protocol->state = STATE_IDLE;
					protocol->timeout_counter = PROTOCOL_ZERO;
				}
			} else {
				if (protocol->timeout_counter < PROTOCOL_TIMEOUT_RETRY_COUNT) {
					LOG_E("re-transmitting the previous frame");
					protocol->timeout_counter++;
					memcpy((uint8_t *)&protocol->next_tx,
							(uint8_t *)&protocol->last_tx, sizeof(frame_info_t));
				} else {
					LOG_E("finish timeout recovery, set protocol state to IDLE");
					protocol->state = STATE_IDLE;
					protocol->timeout_counter = PROTOCOL_ZERO;
				}
			}
		}
	}

	return status;
}

ESE_STATUS iso7816_t1_send(void *ctx, ese_data_t *data)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;
	ESE_STATUS status = ESESTATUS_FAILED;

	if (protocol->state == STATE_IDLE && data != NULL) {
		protocol->next_tx.iframe.data = data->data;
		protocol->next_tx.iframe.total_size = data->size;
		iso7816_t1_set_iframe(protocol);
	}

	memcpy((uint8_t *)&protocol->last_tx, (uint8_t *)&protocol->next_tx, sizeof(frame_info_t));

	switch (protocol->state) {
		case STATE_SEND_IFRAME:
			status = iso7816_t1_send_iframe(protocol);
			break;
		case STATE_SEND_RFRAME:
			status = iso7816_t1_send_rframe(protocol);
			break;
		case STATE_SEND_SFRAME:
			status = iso7816_t1_send_sframe(protocol);
			break;
		default:
			LOG_E("invalid state");
			protocol->state = STATE_IDLE;
			break;
	}

	return status;
}

ESE_STATUS iso7816_t1_receive(void *ctx, ese_data_t *data)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;
	ESE_STATUS status = ESESTATUS_FAILED;
	ESE_STATUS wstatus = ESESTATUS_FAILED;

	do {
		status = iso7816_t1_process(protocol);
		if (status == ESESTATUS_INVALID_DEVICE) {
			return status;
		}

		if (protocol->state != STATE_IDLE) {
			status = iso7816_t1_send(protocol, NULL);
			if (status != ESESTATUS_SUCCESS) {
				LOG_E("send failed, going to recovery!");
				protocol->state = STATE_IDLE;
			}
		}
	} while (protocol->state != STATE_IDLE);
	wstatus = ese_data_get(&(protocol->recv_data), &data->data, &data->size);
	if (wstatus != ESESTATUS_SUCCESS) {
		status = wstatus;
	}

	protocol->cur_buf = data->data;
	protocol->cur_size = data->size;
	return status;
}

ESE_STATUS iso7816_t1_resync_request(void *ctx)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;

	protocol->state = STATE_SEND_SFRAME;
	protocol->next_tx.type = FRAME_SFRAME;
	protocol->next_tx.sframe.type = SFRAME_RESYNCH_REQ;
	protocol->next_tx.sframe.data = NULL;
	protocol->next_tx.sframe.send_size = 0;
	return iso7816_t1_send(protocol, NULL);
}

ESE_STATUS iso7816_t1_resync_response(void *ctx)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;
	ESE_STATUS status = ESESTATUS_FAILED;

	status = iso7816_t1_process(protocol);
	protocol->state = STATE_IDLE;
	if (protocol->last_rx.type != FRAME_SFRAME &&
			protocol->last_rx.sframe.type != SFRAME_RESYNCH_RSP) {
		return ESESTATUS_INVALID_FRAME;
	}
	return status;
}

ESE_STATUS iso7816_t1_wtx_request(void *ctx, uint32_t time)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;

	protocol->state = STATE_SEND_SFRAME;
	protocol->next_tx.type = FRAME_SFRAME;
	protocol->next_tx.sframe.type = SFRAME_WTX_REQ;
	protocol->next_tx.sframe.data = (uint8_t *)&time;
	protocol->next_tx.sframe.send_size = 4;
	return iso7816_t1_send(protocol, NULL);
}

ESE_STATUS iso7816_t1_wtx_response(void *ctx)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;
	ESE_STATUS status = ESESTATUS_FAILED;

	status = iso7816_t1_process(protocol);
	protocol->state = STATE_IDLE;
	if (protocol->last_rx.type != FRAME_SFRAME &&
			protocol->last_rx.sframe.type != SFRAME_WTX_RSP) {
		return ESESTATUS_INVALID_FRAME;
	}
	return status;
}

ESE_STATUS iso7816_t1_get_data(void *ctx, ese_data_t *data, uint8_t *le)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;
	unsigned int data_size = 0;

	if (data != NULL) {
		data_size = protocol->cur_buf[APDU_P3_OFFSET];
		data->data = protocol->cur_buf + APDU_HEADER_SIZE;
		data->size = data_size;
	}

	if (le != NULL) {
		if (data_size == 0) {
			*le = protocol->cur_buf[APDU_P3_OFFSET];
		} else {
			*le = protocol->cur_buf[APDU_HEADER_SIZE + data_size];
		}
	}

	return ESESTATUS_SUCCESS;
}

void *iso7816_t1_init(uint8_t send_address, uint8_t receive_address, void *hal)
{
	iso7816_t1_t *protocol = NULL;

	protocol = ESE_MALLOC(sizeof(iso7816_t1_t));
	if (protocol == NULL) {
		return NULL;
	}

	iso7816_t1_reset_params(protocol);
	ese_data_init(&(protocol->recv_data));
/*
 * T1 Header + Information Field + LRC (3 + 256 + 1)
 */
	protocol->proc_buf = ESE_MALLOC(260);
	if (protocol->proc_buf == NULL) {
		ESE_FREE(protocol);
		return NULL;
	}
	protocol->send_address = send_address;
	protocol->receive_address = receive_address;
	protocol->hal = hal;

	return (void *)protocol;
}

void iso7816_t1_deinit(void *ctx)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;

	ESE_FREE(protocol->proc_buf);
	ESE_FREE(protocol);
}

void iso7816_t1_reset(void *ctx)
{
	iso7816_t1_t *protocol = (iso7816_t1_t *)ctx;

	iso7816_t1_reset_params(protocol);
}
