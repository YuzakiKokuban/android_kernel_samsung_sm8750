/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *
 * Changes from Qualcomm Innovation Center, Inc.are provided under the following license:
 *
 * Copyright (c) 2022,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** \file pal_api.h
 *  \brief Function prototypes for the PAL(Platform Audio
 *   Layer).
 *
 *  This contains the prototypes for the PAL(Platform Audio
 *  layer) and  any macros, constants, or global variables
 *  needed.
 */

#ifndef PAL_API_H
#define PAL_API_H

#include "PalDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Get PAL version in the form of Major and Minor number
 *  seperated by period.
 *
 *  @return the version string in the form of Major and Minor
 *  e.g '1.0'
 */
const char* pal_get_version( );

/**
 *  Initialize PAL. Increases ref count.
 *
 *  @return 0 on success, error code on failure.
 */
int32_t pal_init( );

/**
 *  De-Initialize PAL. Decreases the ref count.
 */
void pal_deinit();

/**
  * \brief Open the stream with specified configuration.
  *
  * \param[in] attributes - Valid stream attributes obtained
  *       from pal_stream_open
  * \param[in] no_of_devices - no of audio devices that the
  *       stream should be initially started with.
  * \param[in] pal_device - an array of pal_devices. The size of
  *       the array is based on the no_of_devices specified by
  *       the client.
  *       If pal_media_config in pal_device is specified as NULL,
  *       PAL uses the default device configuration or appropriate
  *       configuration based on the usecases running.
  *       Clients can query the device configuration by using
  *       pal_get_device().
  * \param[in] no_of_modifiers - no of modifiers.
  * \param[in] modifiers - an array of modifiers. Modifiers are
  *       used to add additional key-value pairs. e.g to
  *       identify the topology of usecase from default set.
  * \param[in] cb - callback function associated with stream.
  *        Any event will notified through this callback
  *        function.
  * \param[in] cookie - client data associated with the stream.
  *       This cookie will be returned back in the callback
  *       function.
  * \param[out] stream_handle - Updated with valid stream handle
  *       if the operation is successful.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_open(struct pal_stream_attributes *attributes,
                        uint32_t no_of_devices, struct pal_device *devices,
                        uint32_t no_of_modifiers, struct modifier_kv *modifiers,
                        pal_stream_callback cb, uint64_t cookie,
                        pal_stream_handle_t **stream_handle);

/**
  * \brief Close the stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_close(pal_stream_handle_t *stream_handle);

/**
  * \brief Register for specific event on a given stream. Events
  *        will be notified via the callback function registered
  *        in pal_stream_open cmd.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] event_id - Valid event id that the client would like notificaiton.
  * \param[in] event_data - Event configuration data.
  *
  * \return 0 on success, error code otherwise
  */
//int32_t pal_stream_register_for_event(pal_stream_handle_t *stream_handle,
  //                               uint32_t event_id, pal_event_cfg_t *event_cfg);

/**
  * \brief Start the stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_start(pal_stream_handle_t *stream_handle);

/**
  * \brief Stop the stream. Stream must be in started/paused
  *        state before stoping.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_stop(pal_stream_handle_t *stream_handle);

/**
  * \brief Pause the stream. Stream must be in started state
  *        before resuming.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_pause(pal_stream_handle_t *stream_handle);

/**
  * \brief Resume the stream. Stream must be in paused state
  *        before resuming.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_resume(pal_stream_handle_t *stream_handle);

/**
  * \brief Flush accumlated data from the stream. Stream must be
  *        in paused state before flushing.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_flush(pal_stream_handle_t *stream_handle);

/**
  * Drain audio data buffered by the driver/hardware has been
  * played depending on the drain type specified. If stream is
  * opened with AUDIO_STREAM_FLAG_NON_BLOCKING and callback
  * function is set in pal_open_stream(), drain complete
  * notificaiton will be sent via the callback function
  * otherwise will block until drain is completed.
  *
  * Drain will return immediately on stop() and flush() call.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] type - drain type, DRAIN or DRAIN_PARTIAL.
  *
  * \return 0 on success, error code otherwise
  */

int32_t pal_stream_drain(pal_stream_handle_t *stream_handle, pal_drain_type_t type);

/**
  * \brief Suspend graph and stop processing data from the stream.
  * Stream must be in started state before suspending.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_suspend(pal_stream_handle_t *stream_handle);

/**
  * Get audio buffer size based on the direction of the stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open.
  * \param[out] in_buffer - filled if stream was opened with
  *       PAL_AUDIO_INPUT direction.
  * \param[out] out_buffer - filled if stream was opened with
  *       PAL_AUDIO_OUTPUT direction.
  * \param[out] in buffer and out_buffer - filled if stream was
  *       opened with PAL_AUDIO_OUTPUT|PAL_AUDIO_INPUT direction.
  *
  * \return - 0 on success, error code otherwise.
  */
int32_t pal_stream_get_buffer_size(pal_stream_handle_t *stream_handle,
                                   size_t *in_buffer, size_t *out_buffer);

/**
  * Gets all the tags and associated module iid and module_id
  * mapping associated with a Pal session handle
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open.
  * \param[in/out] size - size of the memory passed by the client. If it is not
                          enough to copy the tag_module_info a error is returned with
                          the size set to the expected size of the memory to be passed.
  * \param[out] payload - It is in the form of struct pal_tag_module_info
  *
  * \return - 0 on success, error code otherwise.
  */
int32_t pal_stream_get_tags_with_module_info(pal_stream_handle_t *stream_handle,
                                   size_t *size ,uint8_t *payload);

/**
  * Set audio buffer size based on the direction of the stream.
  * This overwrites the default buffer size configured for
  * certain stream types.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open.
  * \param[in] in_buff_cfg  - input buffers configuratoin when stream is
  *       opened with PAL_AUDIO_INPUT or PAL_AUDIO_INPUT_OUTPUT direction.
  * \param[in] output_buff_count - output buffers configuratoin when stream is
  *       opened with PAL_AUDIO_OUTPUT or PAL_AUDIO_INPUT_OUTPUT direction.
  *
  * \return - 0 on success, error code otherwise.
  */
int32_t pal_stream_set_buffer_size (pal_stream_handle_t *stream_handle,
                                    pal_buffer_config_t *in_buff_cfg,
                                    pal_buffer_config_t *out_buff_cfg);

/**
  * Read audio buffer captured from in the audio stream.
  * an error code.
  * Capture timestamps will be populated if session was
  * opened with timetamp flag.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] buf - pointer to pal_buffer containing audio
  *       samples and metadata.
  *
  * \return - number of bytes read or error code on failure
  */
ssize_t pal_stream_read(pal_stream_handle_t *stream_handle, struct pal_buffer *buf);

/**
  * Write audio buffer of a stream for rendering.If at least one
  * frame was written successfully prior to the error, PAL will
  * return number of bytes returned.
  *
  * Timestamp is honored if the stream was opened with timestamp
  * flag otherwise it is ignored.
  *
  * If the stream was opened with non-blocking mode, the write()
  * will operate in non-blocking mode. PAL will write only the
  * number of bytes that currently fit in the driver/hardware
  * buffer. If the callback function is set during
  * pal_stream_open, the callback function will be called when
  * more space is available in the driver/hardware buffer.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] buf - pointer to pal_buffer containing audio
  *       samples and metadata.
  *
  * \return number of bytes written or error code.
  */
ssize_t pal_stream_write(pal_stream_handle_t *stream_handle, struct pal_buffer *buf);

/**
  * \brief get current device on stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] no_of_devices - no of audio devices allocated  
  *       for below pal_device array.
  * \param[in] pal_device - Pointer to an array of pal_device. The size of
  *       the array is sent via the above no_of_devices param.
  *
  * \return number of devices on success, error code otherwise.
  */
int32_t pal_stream_get_device(pal_stream_handle_t *stream_handle,
                            uint32_t no_of_devices, struct pal_device *devices);

/**
  * \brief set new device on stream. This api will disable the
  *        existing device and set the new device. If the new
  *        device is a combo device and includes previously set
  *        device, it will retain the old device to avoid
  *        setting the same device again unless device
  *        configuration chagnes.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] no_of_devices - no of audio devices that the
  *       stream should be initially started with.
  * \param[in] pal_device - an array of pal_device. The size of
  *       the array is based on the no_of_devices specified by
  *       the client.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_set_device(pal_stream_handle_t *stream_handle,
                           uint32_t no_of_devices, struct pal_device *devices);

/**
  * \brief Get audio parameters specific to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] param_id - param id whose parameters are
  *       retrieved.
  * \param[out] param_payload - param data applicable to the
  *       param_id
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_get_param(pal_stream_handle_t *stream_handle,
                           uint32_t param_id, pal_param_payload **param_payload);

/**
  * \brief Set audio parameters specific to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] param_id - param id whose parameters are to be
  *       set.
  * \param[out] param_payload - param data applicable to the
  *       param_id
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_set_param(pal_stream_handle_t *stream_handle,
                           uint32_t param_id, pal_param_payload *param_payload);

/**
  * \brief Get audio volume specific to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] volume - volume data to be set on a stream.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_get_volume(pal_stream_handle_t *stream_handle,
                              struct pal_volume_data *volume);

/**
  * \brief Set audio volume specific to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[out] volume - volume data to be retrieved from the
  *       stream.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_set_volume(pal_stream_handle_t *stream_handle,
                              struct pal_volume_data *volume);

/**
  * \brief Get current audio audio mute state to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] mute - mute state to be retrieved from the
  *       stream.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_get_mute(pal_stream_handle_t *stream_handle, bool *state);

/**
  * \brief Set mute specific to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[out] mute - mute state to be set to the stream.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_set_mute(pal_stream_handle_t *stream, bool state);

/**
  * \brief Get microphone mute state.
  *
  *\param[out] mute - global mic mute flag to be retrieved.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_get_mic_mute(bool *state);

/**
  * \brief Set global mic mute state.
  *
  * \param[out] mute - global mic mute state
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_set_mic_mute(bool state);

/**
  * \brief Get time stamp.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[out] stime - time stamp data to be retrieved from the
  *       stream.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_get_timestamp(pal_stream_handle_t *stream_handle, struct pal_session_time *stime);

/**
  * \brief Add remove effects for Voip TX path.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] effect - effect to be enabled or disable
  * \param[in] enable - enable/disable
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_add_remove_effect(pal_stream_handle_t *stream_handle, pal_audio_effect_t effect, bool enable);

/**
  * \brief Set pal parameters
  *
  * \param[in] param_id - param id whose parameters are to be
  *       set.
  * \param[in] param_payload - param data applicable to the
  *       param_id
  * \param[in] payload_size - size of payload
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_set_param(uint32_t param_id, void *param_payload, size_t payload_size);

/**
  * \brief Get pal parameters
  *
  *
  * \param[in] param_id - param id whose parameters are
  *       retrieved.
  * \param[out] param_payload - param data applicable to the
  *       param_id
  * \param[out] payload_size - size of payload
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_get_param(uint32_t param_id, void **param_payload,
                        size_t *payload_size, void *query);

/**
  * \brief Set audio volume specific to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[in] min_size_frames - minimum frame size required.
  * \param[out] info - map buffer descriptor returned by
  *       stream.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_create_mmap_buffer(pal_stream_handle_t *stream_handle,
                              int32_t min_size_frames,
                              struct pal_mmap_buffer *info);

/**
  * \brief Set audio volume specific to a stream.
  *
  * \param[in] stream_handle - Valid stream handle obtained
  *       from pal_stream_open
  * \param[out] position - Mmap buffer read/write position returned.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_stream_get_mmap_position(pal_stream_handle_t *stream_handle,
                              struct pal_mmap_position *position);

/**
  * \brief Register global callback to pal.
  *        This can be used to inform client about any information
  *        needed even before stream is created.
  *
  * \param[in] cb - Valid callback.
  * \param[in] cookie - client data. This cookie will be
  *       returned back in the callback function.
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_register_global_callback(pal_global_callback cb, uint64_t cookie);

/**
  * \brief Set and get pal parameters for generic effect framework
  *
  * \param[in] param_id - param id whose parameters are to be
  *       set.
  * \param[in/out] param_payload - param data applicable to the
  *       param_id
  * \param[in] payload_size - size of payload
  *
  * \param[in] pal_stream_type - type of stream to apply the GEF param
  *
  * \param[in] pal_device_id - device id to apply the effect
  *
  * \param[i] pal_stream_type - stream type to apply the effect
  *
  * \param[in] dir - param read or write
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_gef_rw_param(uint32_t param_id, void *param_payload,
                      size_t payload_size, pal_device_id_t pal_device_id,
                      pal_stream_type_t pal_stream_type, unsigned int dir);

/**
  * \brief Set and get pal parameters for generic effect framework with ACDB
  *
  * \param[in] param_id - param id whose parameters are to be
  *       set.
  * \param[in/out] param_payload - param data applicable to the
  *       param_id
  * \param[in] payload_size - size of payload
  *
  * \param[in] pal_stream_type - type of stream to apply the GEF param
  *
  * \param[in] pal_device_id - device id to apply the effect
  *
  * \param[i] pal_stream_type - stream type to apply the effect
  *
  * \param[in] sample_rate - sample_rate value for CKV. 0 for default CKV
  *
  * \param[in] instance_id - instance id
  *
  * \param[in] dir - param read or write
  *
  * \param[in] is_play - stream direction. true for playback. false for recording.
  *
  * \return 0 on success, error code otherwise
  */

int32_t pal_gef_rw_param_acdb(uint32_t param_id, void *param_payload,
                      size_t payload_size, pal_device_id_t pal_device_id,
                      pal_stream_type_t pal_stream_type, uint32_t sample_rate,
                      uint32_t instance_id, uint32_t dir, bool is_play);

/**
  * \brief registers a callback function to get notification for audio events.
  *
  * \param[in] cb_event - Callback function to be called
  *
  * \return 0 on success, error code otherwise
  */
int32_t pal_register_for_events(pal_audio_event_callback cb_event);

#ifdef SEC_AUDIO_COMMON
int32_t pal_active_device_count(pal_device_id_t deviceId);
void pal_dump(int fd);
#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /*PAL_API_H*/
