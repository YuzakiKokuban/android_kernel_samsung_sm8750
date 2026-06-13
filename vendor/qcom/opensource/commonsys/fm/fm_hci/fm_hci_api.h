/*
 * Copyright (c) 2015-2016 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above
 *            copyright notice, this list of conditions and the following
 *            disclaimer in the documentation and/or other materials provided
 *            with the distribution.
 *        * Neither the name of The Linux Foundation nor the names of its
 *            contributors may be used to endorse or promote products derived
 *            from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __FM_HCI_API__
#define __FM_HCI_API__


/** Host/Controller Library Return Status */
typedef enum {
    FM_HC_STATUS_SUCCESS,
    FM_HC_STATUS_FAIL,
    FM_HC_STATUS_NOT_READY,
    FM_HC_STATUS_NOMEM,
    FM_HC_STATUS_BUSY,
    FM_HC_STATUS_CORRUPTED_BUFFER,
    FM_HC_STATUS_NULL_POINTER,
} fm_hc_status_t;

static char *status_s[] = {
    "Success",
    "Failed, generic error",
    "Not ready",
    "Memory not available",
    "Resource busy",
    "Buffer is corrupted",
    "NULL pointer dereference",
};

static inline char *fm_hci_status(int status) {
    return status_s[status];
}

typedef enum {
    FM_RADIO_DISABLED,
    FM_RADIO_DISABLING,
    FM_RADIO_ENABLED,
    FM_RADIO_ENABLING
} fm_power_state_t;

typedef int (*event_notification_cb_t)(unsigned char *buf);
typedef int (*hci_close_done_cb_t)(void);


struct fm_hci_callbacks_t {
    event_notification_cb_t process_event;
    hci_close_done_cb_t fm_hci_close_done;
};

typedef struct {
    void *hci;
    void *hal;
    struct fm_hci_callbacks_t *cb;
}fm_hci_hal_t;

struct fm_command_header_t {
    uint16_t opcode;
    uint8_t  len;
    uint8_t  params[];
}__attribute__((packed));

struct fm_event_header_t {
    uint8_t evt_code;
    uint8_t evt_len;
    uint8_t params[];
}__attribute__((packed));
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         fm_hci_init
**
** Description      This function is used to intialize fm hci
**
** Parameters:     hci_hal: contains the fm helium hal hci pointer
**
**
** Returns          void
**
*******************************************************************************/
int fm_hci_init(fm_hci_hal_t *hal_hci);

/*******************************************************************************
**
** Function         fm_hci_transmit
**
** Description      This function is called by helium hal & is used enqueue the
**                      tx commands in tx queue.
**
** Parameters:     p_hci - contains the fm helium hal hci pointer
**                      hdr - contains the fm command header pointer
**
** Returns          void
**
*******************************************************************************/
int fm_hci_transmit(void *p_hci, struct fm_command_header_t *hdr);
/*******************************************************************************
**
** Function         fm_hci_close
**
** Description      This function is used to close & cleanup hci
**
** Parameters:      p_hci: contains the fm hci pointer
**
**
** Returns          void
**
*******************************************************************************/
void fm_hci_close(void *p_hci);

#ifdef __cplusplus
}
#endif

#endif
