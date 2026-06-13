/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include"PalUsecaseTest.h"
#include <errno.h>
#include <string.h>

static struct pal_stream_attributes *stream_attributes;
static struct pal_device *pal_devices;
static pal_stream_handle_t *pal_stream;

int32_t OpenAndStartUsecase(int usecase_type) {
     pal_stream_type_t usecase = (pal_stream_type_t)usecase_type;
     int32_t status = 0;
     fprintf(stdout, "Enter OpenAndStartUsecase\n");
     switch (usecase) {
          case PAL_STREAM_ULTRASOUND:
              status = setup_usecase_ultrasound();
              if (status) {
                  fprintf(stdout, "Error:Failed to Start UPD\n");
                  goto exit;
              }
              fprintf(stdout, "Stream started succefully\n");
              break;
         default :
              fprintf(stdout, "unkown uasecase\n");
              status = -EINVAL;
              break;
     }
exit:
     fprintf(stdout, "Exit OpenAndStartUsecase\n");
     return status;
}

int32_t StopAndCloseUsecase()
{
    int32_t status = 0;
    fprintf(stdout, "Enter StopAndCloseUsecase\n");

    if (pal_stream) {
        status = pal_stream_stop(pal_stream);
        if (status) {
           fprintf(stdout, "pal_stream_stop failed\n");
        }

        status = pal_stream_close(pal_stream);
        if (status) {
           fprintf(stdout, "pal_stream_close failed\n");
        }
        pal_stream = NULL;
        if (stream_attributes)
           free(stream_attributes);
        if (pal_devices)
           free(pal_devices);
    }

    fprintf(stdout, "Exit StopAndCloseUltrasound\n");
    return status;
}

int32_t setup_usecase_ultrasound()
{
     int32_t status = 0;
     pal_param_payload *param_payload = NULL;
     pal_param_upd_event_detection_t payload;
     int32_t no_of_devices = 2;

     //setting stream attributes
     stream_attributes = (struct pal_stream_attributes *)
                          calloc (1, sizeof(struct pal_stream_attributes));
     if (!stream_attributes)
        goto exit;

     stream_attributes->type = PAL_STREAM_ULTRASOUND;
     stream_attributes->direction = PAL_AUDIO_INPUT_OUTPUT;

     // setting device attriutes
     //  device attributes for UPD will be set based on BE used.
     pal_devices = (struct pal_device *) calloc(no_of_devices, sizeof(struct pal_device));

     status = pal_stream_open(stream_attributes, no_of_devices, pal_devices, 0, NULL,
                            (pal_stream_callback)&HandleCallbackForUPD, 0, &pal_stream);
     if (status) {
         fprintf(stdout, "Error:Failed to open UPD stream\n");
         goto exit;
     }
     fprintf(stdout, "Stream Opened succesfully\n");

     param_payload = (pal_param_payload *) calloc (1,
                                 sizeof(pal_param_payload) +
                                 sizeof(pal_param_upd_event_detection_t));
     if (!param_payload)
        goto exit;

     payload.register_status = 1;
     param_payload->payload_size = sizeof(pal_param_upd_event_detection_t);
     memcpy(param_payload->payload, &payload, param_payload->payload_size);
     status = pal_stream_set_param(pal_stream, PAL_PARAM_ID_UPD_REGISTER_FOR_EVENTS, param_payload);
     if (status) {
         fprintf(stdout, "setParams failed");
         goto close_stream;
     }

     status = pal_stream_start(pal_stream);
     if (status) {
         fprintf(stdout, "Error:Failed to Start UPD");
         goto close_stream;
     }
     goto exit;
close_stream:
     pal_stream_close(pal_stream);
     pal_stream = NULL;
exit:
     if (param_payload)
        free(param_payload);
     if (stream_attributes) {
        free(stream_attributes);
        stream_attributes = NULL;
     }
     return status;
}

static int32_t HandleCallbackForUPD(pal_stream_handle_t *stream_handle,
                                   uint32_t event_id, uint32_t *event_data,
                                   uint32_t event_size, uint64_t cookie)
{
     int32_t status = 0;

     if (event_id == EVENT_ID_GENERIC_US_DETECTION) {
        if (*event_data == 1)
            fprintf(stdout, "Event Detected : Near event received\n");
        else if (*event_data == 2)
            fprintf(stdout, "Event Detected : Far event received\n");
        else
           fprintf(stdout, "Event Detected : Invalid event %d\n", *event_data);
     }
     return status;
}
