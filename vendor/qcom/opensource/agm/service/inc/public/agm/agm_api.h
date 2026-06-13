/*
 * Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */

#ifndef _AGM_INTF_H_
#define _AGM_INTF_H_
/**
 *=============================================================================
 * \file agm_api.h
 *
 * \brief
 *      Defines public APIs for Audio Graph Manager (AGM)
 *=============================================================================
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

struct session_obj;

/**<
 * used to indicate a given buffer is the final buffer, client will get
 * notified once the buffer has been rendered
 */
#define AGM_BUFF_FLAG_EOS 0x1

/**< true if buffer has a valid timestamp */
#define AGM_BUFF_FLAG_TS_VALID 0x2

/**< true if buffer is marked as EOF */
#define AGM_BUFF_FLAG_EOF 0x4

/**< true if buffer contains media format */
#define AGM_BUFF_FLAG_MEDIA_FORMAT 0x8

/*Enables SRCM event in metadata on the read path*/
#define AGM_SESSION_FLAG_INBAND_SRCM 0x1

/**
 * A single entry of a Key Vector
 */
struct agm_key_value {
    uint32_t key; /**< key */
    uint32_t value; /**< value */
};

struct prop_data {
    uint32_t prop_id;
    uint32_t num_values;
uint32_t values[];
};

/**
 * Bit formats
 */
enum agm_media_format
{
    AGM_FORMAT_INVALID,
    AGM_FORMAT_PCM_S8,          /**< 8-bit signed */
    AGM_FORMAT_PCM_S16_LE,      /**< 16-bit signed */
    AGM_FORMAT_PCM_S24_LE,      /**< 24-bits in 4-bytes */
    AGM_FORMAT_PCM_S24_3LE,     /**< 24-bits in 3-bytes */
    AGM_FORMAT_PCM_S32_LE,      /**< 32-bit signed */
    AGM_FORMAT_MP3,             /**< MP3 codec */
    AGM_FORMAT_AAC,             /**< AAC codec */
    AGM_FORMAT_FLAC,            /**< FLAC codec */
    AGM_FORMAT_ALAC,            /**< ALAC codec */
    AGM_FORMAT_APE,             /**< APE codec */
    AGM_FORMAT_WMASTD,          /**< WMA codec */
    AGM_FORMAT_WMAPRO,          /**< WMA pro codec */
    AGM_FORMAT_VORBIS,          /**< Vorbis codec */
    AGM_FORMAT_AMR_NB,          /**< AMR NB codec */
    AGM_FORMAT_AMR_WB,          /**< AMR WB codec */
    AGM_FORMAT_AMR_WB_PLUS,     /**< AMR WB Plus codec */
    AGM_FORMAT_EVRC,            /**< EVRC codec */
    AGM_FORMAT_G711,            /**< G711 codec */
    AGM_FORMAT_QCELP,            /**< G711 codec */
    AGM_FORMAT_OPUS,            /**< OPUS codec */
    AGM_FORMAT_MAX,
};

/**
 * Data format
 * MUST stay in sync with media_fmt_api.h
 */
enum agm_data_format
{
    AGM_DATA_FORMAT_INVALID,
    AGM_DATA_FORMAT_FIXED_POINT,                    /**< fixed point */
    AGM_DATA_FORMAT_IEC61937_PACKETIZED,            /**< IEC61937 packetized stream */
    AGM_DATA_FORMAT_IEC60958_PACKETIZED,            /**< IEC60958 packetized stream for PCM only */
    AGM_DATA_FORMAT_DSD_OVER_PCM,                   /**< DSD over PCM stream */
    AGM_DATA_FORMAT_GENERIC_COMPRESSED,             /**< generic compressed stream */
    AGM_DATA_FORMAT_RAW_COMPRESSED,                 /**< raw compressed stream */
    AGM_DATA_FORMAT_COMPR_OVER_PCM_PACKETIZED,      /**< Compressed bitstreams packetized */
    AGM_DATA_FORMAT_IEC60958_PACKETIZED_NON_LINEAR, /**< IEC60958 packetized stream for compressed streams */
};

/**
 * AGM data modes
 */
enum agm_data_mode
{
    AGM_DATA_INVALID,
    AGM_DATA_BLOCKING,      /**< Blocking mode */
    AGM_DATA_NON_BLOCKING,  /**< Non blocking mode */
    AGM_DATA_PUSH_PULL,     /**< Push Pull mode */
    AGM_DATA_EXTERN_MEM,    /**< Push Pull mode */
    AGM_DATA_MODE_MAX,
};

/**
 *AGM session modes
 */
enum agm_session_mode
{
    AGM_SESSION_DEFAULT,         /**< Normal agm tunnel session*/
    AGM_SESSION_NO_HOST,         /**< Hostless mode */
    AGM_SESSION_NON_TUNNEL,      /**< Non tunnel mode */
    AGM_SESSION_NO_CONFIG,       /**< No Config mode*/
    AGM_SESSION_COMPRESS,        /**< Compress mode*/
};

struct agm_extern_alloc_buff_info{
    int      alloc_handle;/**< unique handle identifying extern mem allocation */
    uint32_t alloc_size;  /**< size of external allocation */
    uint32_t offset;      /**< offset of buffer within extern allocation */
};

struct agm_buff {
    uint64_t timestamp; /**< timestamp in micro-secs */
    uint32_t flags; /**< bitmasked flags for e.g. AGM_BUFF_FLAG_EOS */
    uint32_t size; /**< size of buffer in bytes */
    uint8_t *addr; /**< data buffer */
    uint32_t metadata_size; /**< size of metadata blob */
    uint8_t *metadata; /**< Blob for metadata */
    struct agm_extern_alloc_buff_info alloc_info; /**< holds info for extern buff */
};

/**
 *Gapless playback Silence type
 */
enum agm_gapless_silence_type
{
    INITIAL_SILENCE,            /**< Initial silence sample to be removed*/
    TRAILING_SILENCE,           /**< Trailing silence sample to be removed*/
};

/**
 * AAC decoder parameters
 */
struct agm_session_aac_dec {
    uint16_t aac_fmt_flag;          /**< AAC format flag */
    uint16_t audio_obj_type;        /**< AAC obj type */
    uint16_t num_channels;          /**< AAC obj type */
    uint16_t total_size_of_PCE_bits;/**< PCE bits size */
    uint32_t sample_rate;           /**< Sample rate */
};

/**
 * AAC encoder parameters
 */
struct agm_session_aac_enc_cfg {
    uint16_t aac_enc_mode; /**< AAC encoder mode */
    uint16_t aac_fmt_flag; /**< AAC format flag */
};

struct agm_session_aac_enc {
    uint32_t aac_bit_rate;
    uint32_t global_cutoff_freq;
    struct agm_session_aac_enc_cfg enc_cfg;
};

/**
 * FLAC decoder parameters
 */
struct agm_session_flac_dec {
    uint16_t num_channels;    /**< Number of channels */
    uint16_t sample_size;     /**< Sample size */
    uint16_t min_blk_size;    /**< Minimum block size */
    uint16_t max_blk_size;    /**< Maximum block size */
    uint32_t sample_rate;     /**< Sample rate */
    uint32_t min_frame_size;  /**< Minimum frame size */
    uint32_t max_frame_size;  /**< Maximum frame size */
};

/**
 * ALAC decoder parameters
 */
struct agm_session_alac_dec {
    uint32_t frame_length;        /**< Frame length */
    uint8_t compatible_version;   /**< Version */
    uint8_t bit_depth;            /**< bit depth */
    uint8_t pb;                   /**< tuning parameters */
    uint8_t mb;                   /**< tuning parameters */
    uint8_t kb;                   /**< tuning parameters */
    uint8_t num_channels;         /**< Number of channels */
    uint16_t max_run;             /**< Max run */
    uint32_t max_frame_bytes;     /**< Max Frame bytes */
    uint32_t avg_bit_rate;        /**< avg bit rate */
    uint32_t sample_rate;         /**< sample rate */
    uint32_t channel_layout_tag;  /**< Channel layout tag */
};

/**
 * APE decoder parameters
 */
struct agm_session_ape_dec {
    uint16_t compatible_version;   /**< Version */
    uint16_t compression_level;    /**< Compression Level */
    uint32_t format_flags;         /**< Format flags */
    uint32_t blocks_per_frame;     /**< Blocks per frame */
    uint32_t final_frame_blocks;   /**< Final frame blocks */
    uint32_t total_frames;         /**< Total frames */
    uint16_t bit_width;            /**< Bit width */
    uint16_t num_channels;         /**< Number of channels */
    uint32_t sample_rate;          /**< Sample rate */
    uint32_t seek_table_present;   /**< Seek table present */
};

/**
 * WMA decoder parameters
 */
struct agm_session_wma_dec {
    uint16_t fmt_tag;            /**< Format Tag */
    uint16_t num_channels;       /**< Number of channels */
    uint32_t sample_rate;        /**< Sample rate */
    uint32_t avg_bytes_per_sec;  /**< Avg bytes per sec */
    uint16_t blk_align;          /**< Block align */
    uint16_t bits_per_sample;    /**< Bits per sample */
    uint32_t channel_mask;       /**< Channel mask */
    uint16_t enc_options;        /**< Encoder options */
    uint16_t reserved;            /**< reserved */
};

/**
 * WMA Pro decoder parameters
 */
struct agm_session_wmapro_dec {
    uint16_t fmt_tag;            /**< Format Tag */
    uint16_t num_channels;       /**< Number of channels */
    uint32_t sample_rate;        /**< Sample rate */
    uint32_t avg_bytes_per_sec;  /**< Avg bytes per sec */
    uint16_t blk_align;          /**< Block align */
    uint16_t bits_per_sample;    /**< Bits per sample */
    uint32_t channel_mask;       /**< Channel mask */
    uint16_t enc_options;        /**< Encoder options */
    uint16_t advanced_enc_option; /**< Adv encoder options */
    uint32_t advanced_enc_option2; /**< Adv encoder options2 */
};

/**
 * OPUS decoder parameters
 */
struct agm_session_opus_dec {
    uint16_t bitstream_format;
    uint16_t payload_type;
    uint8_t version;
    uint8_t num_channels;
    uint16_t pre_skip;
    uint32_t sample_rate;
    uint16_t output_gain;
    uint8_t mapping_family;
    uint8_t stream_count;
    uint8_t coupled_count;
    uint8_t channel_map[8];
    uint8_t reserved[3];
};

/**
 * Session encoder/decoder parameters
 */
union agm_session_codec
{
    struct agm_session_aac_dec aac_dec;        /**< AAC decoder config */
    struct agm_session_aac_enc aac_enc;        /**< AAC encoder config */
    struct agm_session_flac_dec flac_dec;      /**< Flac decoder config */
    struct agm_session_alac_dec alac_dec;      /**< Alac decoder config */
    struct agm_session_ape_dec ape_dec;        /**< APE decoder config */
    struct agm_session_wma_dec wma_dec;        /**< WMA decoder config */
    struct agm_session_wmapro_dec wmapro_dec;  /**< WMAPro decoder config */
    struct agm_session_opus_dec opus_dec;      /**< OPUS decoder config */
};

/**
 * Flag to determine data buf or postion buf.
 * One bit is used for each type.
 */
enum buf_flag {
    DATA_BUF = 0x1,
    POS_BUF = 0x2,
};

/**
 * Shared memory buffer info 211
 */
struct agm_buf_info {
    int32_t data_buf_fd;
    int32_t data_buf_size;
    int32_t pos_buf_fd;
    int32_t pos_buf_size;
};

/**
 * Media Config
 */
struct agm_media_config {
    uint32_t rate;                 /**< sample rate */
    uint32_t channels;             /**< number of channels */
    enum agm_media_format format;  /**< format */
    uint32_t data_format;          /**< data format */
};

struct agm_group_media_config {
    struct agm_media_config config;
    uint32_t slot_mask;            /**< slot_mask for TDM*/
};

/**
 * Session Direction
 */
enum direction {
    RX = 1, /**< RX */
    TX,     /**< TX */
};

/**
 * MAX length of the AIF Name
 */
#define AIF_NAME_MAX_LEN 32

/**
 * AIF Info
 */
struct aif_info {
    char aif_name[AIF_NAME_MAX_LEN];  /**< AIF name  */
    enum direction dir;               /**< direction */
};

/**
 * Session Config
 */
struct agm_session_config {
    enum direction dir;        /**< TX or RX */
    enum agm_session_mode sess_mode;  /**< indicates mode of agm sesison, non-tunnel, or hostless */
    uint32_t start_threshold;  /**< start_threshold: number of buffers * buffer size */
    uint32_t stop_threshold;   /**< stop_th6reshold: number of buffers * buffer size */
    union agm_session_codec codec; /**< codec configuration */
    enum agm_data_mode data_mode; /**< compress format ID */
    uint32_t sess_flags; /**< pass session specific flags e.g enable inband SRCM event*/
};

/**
 * Buffer Config
 */
struct agm_buffer_config {
    uint32_t count; /**< number of buffers */
    size_t size;    /**< size of each buffer */
    size_t max_metadata_size; /**< max metadata size a client attaches to a buffer */
};

/**
 * Maps the modules instance id to module id for a single module
 */
struct agm_module_id_iid_map {
    uint32_t module_id;  /**< module id */
    uint32_t module_iid; /**< globally unique module instance id */
};

/**
 * Structure which holds tag and corresponding modules tagged with tag id
 */
struct agm_tag_info {
    uint32_t tag_id;             /**< tag id */
    uint32_t num_modules;        /**< number of modules matching the tag_id */
    struct agm_module_id_iid_map mid_iid_list[0]; /**< agm_module_id_iid_map list */
};

/**
 * Structure which holds tag info of a given graph.
 */
struct agm_tag_module_info_list {
    uint32_t num_tags;          /**< number of tags */
    uint8_t tag_info_list[];/**< variable payload of type struct agm_tag_module_info */
};

/**
 * Structure which holds tag config for setparams
 */
struct agm_tag_config {
    uint32_t tag;                /**< tag id */
    uint32_t num_tkvs;           /**< num  of tag key values*/
    struct agm_key_value kv[];   /**< tag key vector*/
};

struct agm_cal_config {
    uint32_t num_ckvs;         /**< num  of tag key values*/
    struct agm_key_value kv[]; /**< tag key vector*/
};

struct agm_acdb_param {
    bool isTKV;
    uint32_t tag;
    uint32_t num_kvs;          /**< number of ckv or tkv*/
    uint32_t blob_size;        /**< kv size + payload size*/
    uint8_t blob[];            /**< kv + payload */
};

struct agm_acdb_tunnel_param {
    bool isTKV;
    uint32_t tag;
    uint32_t num_gkvs;         /**< number of gkv*/
    uint32_t num_kvs;          /**< number of ckv or tkv*/
    uint32_t blob_size;        /**< gkv size + t/ckv size + payload size*/
    uint8_t blob[];            /**< gkv + t/ckv + payload */
};

/**
 * Event types
 */
enum event_type
{
    AGM_EVENT_DATA_PATH = 1,/**< Events on the Data path, READ_DONE or WRITE_DONE */
    AGM_EVENT_MODULE,       /**< Events raised by modules */
};

struct agm_event_read_write_done_payload {
    uint32_t tag; /**< tag that was used to read/write this buffer */
    uint32_t status; /**< data buffer status as defined in ar_osal_error.h */
    uint32_t md_status; /**< meta-data status as defined in ar_osal_error.h */
    struct agm_buff buff; /**< buffer that was passed to agm_read/agm_write */
};

/**
 * Event registration structure.
 */
struct agm_event_reg_cfg {
    /** Valid instance ID of module */
    uint32_t module_instance_id;

    /** Valid event ID of the module */
    uint32_t event_id;

    /**
     * Size of the event config data based upon the module_instance_id/event_id
     * combination.    @values > 0 bytes, in multiples of 4 bytes atleast
     */
    uint32_t event_config_payload_size;

    /**
     * 1 - to register the event
     * 0 - to de-register the event
     */
    uint8_t is_register;

    /**
     * module specifc event registration payload
     */
    uint8_t event_config_payload[];
};

/** Data Events that will be notified to client from AGM */
enum agm_event_id {
   /**
    * Indicates EOS rendered event
    */
    AGM_EVENT_EOS_RENDERED = 0x0,

   /**
    * Indicates buffer provided as part of read call has been filled.
    */

    AGM_EVENT_READ_DONE = 0x1,
   /**
    * Indicates buffer provided as part of write has been consumed
    */

    AGM_EVENT_WRITE_DONE = 0x2,
    /**
    * Indicates early EOS event flagged internally by AGM in case
    * flush
    */

    AGM_EVENT_EARLY_EOS_INTERNAL = 0x5,
   /**
    * Indicates early EOS event
    */

    AGM_EVENT_EARLY_EOS = 0x08001126,

    AGM_EVENT_ID_MAX
};

/** data that will be passed to client in the event callback */
struct agm_event_cb_params {
/**< identifies the module which generated event */
    uint32_t source_module_id;

/**< identifies the event */
    uint32_t event_id;

/**< size of payload below */
    uint32_t event_payload_size;

/**< payload associated with the event if any */
    uint8_t event_payload[];
};

/** Data passed by the client when requesting dump */
struct agm_dump_info {
    /**< identifies the signal received by client */
    int32_t signal;
    /**< identifies client pid */
    uint32_t pid;
    /**< identifies client uid */
    uint32_t uid;
};

/**
 * \brief Callback function signature for events to client
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] event_params - holds all event related info
 * \param[in] client_data - client data set during callback registration.
 */
typedef void (*agm_event_cb)(uint32_t session_id,
              struct agm_event_cb_params *event_params, void *client_data);

/**
 * \brief Callback function signature for crash notification to clients
 *
 * \param[in] cookie - cookie set during callback registration
 */
typedef void (*agm_service_crash_cb)(uint64_t cookie);

/**
 *  \brief Initialize agm.
 *
 *  \return 0 on success, error code on failure.
 */
int agm_init( );

/**
 *  \brief De-Initialize agm.
 *
 *  \return 0 on success, error code on failure.
 */
int agm_deinit();

 /**
  * \brief Set media configuration for an audio interface.
  *
  * \param[in] aif_id - Valid audio interface id
  * \param[in] media_config - valid media configuration for the
  *       audio interafce.
  *
  *  \return 0 on success, error code on failure.
  *       If the audio interface is already in use and the
  *       new media_config is different from previous, api will return
  *       failure.
  */
int agm_aif_set_media_config(uint32_t aif_id,
                             struct agm_media_config *media_config);


 /**
  * \brief Set metadata for an audio interface.
  *
  * \param[in] aif_id - Valid audio interface id
  * \param[in] size -  size of metadata in bytes
  * \param[in] metadata - valid metadata for the audio
  *       interface.
  *
  *  \return 0 on success, error code on failure.
  *       If the audio interface is already in use and the
  *       new meta data is set, api will return
  *       failure.
  */
int agm_aif_set_metadata(uint32_t aif_id,
                         uint32_t size, uint8_t *metadata);

/**
  * \brief Set metadata for the session.
  *
  * \param[in] session_id - Valid audio session id
  * \param[in] size -  size of metadata in bytes
  * \param[in] metadata - valid metadata for the session.
  *
  *  \return 0 on success, error code on failure.
  *       If the session is already opened and the new
  *       meta data is set, api will return failure.
  */
int agm_session_set_metadata(uint32_t session_id,
                             uint32_t size, uint8_t *metadata);

 /**
  * \brief Set metadata for the session, audio intf pair.
  *
  * \param[in] session_id - Valid session id
  * \param[in] aif_id - Valid audio interface id
  * \param[in] size -  size of metadata in bytes
  * \param[in] metadata - valid metadata for the session and
  *            audio_intf.
  *
  *  \return 0 on success, error code on failure.
  *       If the session is already opened and the new
  *       meta data is set, api will return failure.
  */
int agm_session_aif_set_metadata(uint32_t session_id,
                                 uint32_t aif_id,
                                 uint32_t size, uint8_t *metadata);

/**
 * \brief Set metadata for the session, audio intf pair.
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] aif_id - Valid audio interface id
 * \param[in] state - Connect or Disconnect AIF to Session
 *
 *  \return 0 on success, error code on failure.
 *       If the session is already opened and the new
 *       meta data is set, api will return failure.
 */
int agm_session_aif_connect(uint32_t session_id,
                            uint32_t aif_id,
                            bool state);

/**
 * \brief Set metadata for the session, audio intf pair.
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] aif_id - Valid audio interface id
 * \param[in] payload - payload containing tag module info list in the graph.
 *           The memory for this pointer is allocated by client.
 * \param [in,out] size: size of the payload.
 *      if the value of size is zero, AGM will update required module
 *          info list of a given graph.
 *      if size equal or greater than the required size,
 *          AGM will copy the module info.
 *
 *  \return 0 on success, error code on failure.
 */
int agm_session_aif_get_tag_module_info(uint32_t session_id,
                                        uint32_t aif_id, void *payload,
                                        size_t *size);

/**
 * \brief Set parameters for modules in audio interface
 *
 * \param[in] aif_id - Valid audio interface id
 * \param[in] payload - payload
 * \param[in] size - payload size in bytes
 *
 *  \return 0 on success, error code on failure.
 */
int agm_aif_set_params(uint32_t aif_id,
                        void* payload, size_t size);

/**
 * \brief Set parameters for modules in b/w stream and audio interface
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] aif_id - Valid audio interface id
 * \param[in] payload - payload
 * \param[in] size - payload size in bytes
 *
 *  \return 0 on success, error code on failure.
 *       If the session is already opened and the new
 *       meta data is set, api will return failure.
 */
int agm_session_aif_set_params(uint32_t session_id,
                               uint32_t aif_id,
                               void* payload, size_t size);

/**
 * \brief Set calibration for modules in b/w stream and audio interface
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] aif_id - Valid audio interface id
 * \param[in] cal_config - calibration key vector
 *
 *  \return 0 on success, error code on failure.
 */
int agm_session_aif_set_cal(uint32_t session_id,
                            uint32_t aif_id,
                            struct agm_cal_config *cal_config);

/**
 * \brief Set parameters for modules in stream
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] payload - payload
 * \param[in] size - payload size in bytes
 *
 *  \return 0 on success, error code on failure.
 *       If the session is already opened and the new
 *       meta data is set, api will return failure.
 */
int agm_session_set_params(uint32_t session_id,
                           void* payload, size_t size);

/**
 * \brief Get parameters of the modules of a given session
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] payload - payload
 * \param[in] size - payload size in bytes
 *
 *  \return 0 on success, error code on failure.
 *       If the session is not opened,
 *       api will return failure.
 */
int agm_session_get_params(uint32_t session_id,
    void* payload, size_t size);

/**
 * \brief Get parameters for modules at acdb without session
 *

 * \param[in] payload - payload with tag and calibration date
 * \param[in] size - size of payload
 *
 *  \return 0 on success, error code on failure.
 */
int agm_get_params_from_acdb_tunnel(void *payload, size_t *size);

/**
 * \brief Set parameters for modules in b/w stream and audio interface
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] aif_id - valid audio interface id
 * \param[in] tag_config - tag config structure with tag id and tag key vector
 *
 *  \return 0 on success, error code on failure.
 *       If the session is already opened and the new
 *       meta data is set, api will return failure.
 */

int agm_set_params_with_tag(uint32_t session_id, uint32_t aif_id,
                              struct agm_tag_config *tag_config);

/**
 * \brief Set parameters for modules in b/w stream and audio interface
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] aif_id - valid audio interface id
 * \param[in] payload - payload with tag and calibration date
 * \param[in] size - size of payload
 *
 *  \return 0 on success, error code on failure.
 *       If the session is already opened and the new
 *       meta data is set, api will return failure.
 */

int agm_set_params_with_tag_to_acdb(uint32_t session_id, uint32_t aif_id,
                                void *payload, size_t size);

/**
 * \brief Set parameters for modules at acdb without session
 *

 * \param[in] payload - payload with tag and calibration date
 * \param[in] size - size of payload
 *
 *  \return 0 on success, error code on failure.
 */

int agm_set_params_to_acdb_tunnel(void *payload, size_t size);

/**
  * \brief Open the session with specified session id.
  *
  * \param[in] session_id - Valid audio session id
  * \param[in] cb - callback function to be invoked when an event occurs.
  * \param[in] evt_type - event type that
  * \param[in] client_data - client data.
  *
  * \return 0 on success, error code otherwise
  */

int agm_session_register_cb(uint32_t session_id, agm_event_cb cb,
                    enum event_type evt_type, void *client_data);

/**
  * \brief Register for events from Modules. Not needed for data path events.
  *
  * \param[in] session_id - Valid audio session id
  * \param[out] evt_reg_info - event specific configuration.
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_register_for_events(uint32_t session_id,
                 struct agm_event_reg_cfg *evt_reg_cfg);

/**
  * \brief Open the session with specified session id.
  *
  * \param[in] session_id - Valid audio session id
  * \param[in] sess_mode - Mode in which this agm session is to be opened
  * \param[out] handle - updated with valid session
  *       handle if the operation is successful.
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_open(uint32_t session_id,
                     enum agm_session_mode sess_mode,
                     uint64_t *handle);

/**
  * \brief Set Session config
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  * \param[in] session_config - valid stream configuration of the
  *       sessions
  * \param[in] media_config - valid media configuration of the
  *       session.
  * \param[in] buffer_config - buffer configuration for the
  *       session. Null if hostless
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_set_config(uint64_t hndl,
                           struct agm_session_config *session_config,
                           struct agm_media_config *media_config,
                           struct agm_buffer_config *buffer_config);

/**
  * \brief Close the session.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_close(uint64_t hndl);

/**
  * \brief prepare the session.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_prepare(uint64_t hndl);

/**
  * \brief Start the session.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */

int agm_session_start(uint64_t hndl);

/**
  * \brief Stop the session. session must be in started/paused
  *        state before stopping.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_stop(uint64_t hndl);

/**
  * \brief Pause the session. session must be in started state
  *        before resuming.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_pause(uint64_t hndl);

/**
  * \brief flush the session. session must be in pause state
  *        before flushing.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_flush(uint64_t hndl);

/**
  * \brief flush the session. session must be in pause state
  *        before flushing.
  *
  * \param[in] session_id - Valid audio session id
  *
  * \return 0 on success, error code otherwise
  */
int agm_sessionid_flush(uint32_t session_id);

/**
  * \brief Resume the session. session must be in paused state
  *        before resuming.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_resume(uint64_t hndl);

/**
  * \brief suspend the session. session must be in started state
  *        before suspending.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_suspend(uint64_t hndl);

/**
  * \brief Read data buffers.from session
  *
  * \param[in] handle: session handle returned from
  *       agm_session_open
  * \param[in,out] buff: buffer where data will be copied to
  *  \param[in,out] count: number of bytes requested to fill into
  *      the buffer. AGM will update the count with actual
  *      number of bytes filled.
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_read(uint64_t handle, void *buff, size_t *count);

/**
 * \brief Write data buffers.to session
 *
 * \param[in] handle: session handle returned from
 *       agm_session_open
 * \param[in] buff: buffer where data will be copied from
 * \param[in] count: actual number of bytes in the buffer. AGM
 *       will update the count with number of bytes
 *       consumed/written.
 *
 * \return 0 on success, error code otherwise
 */
int agm_session_write(uint64_t hndl, void *buff, size_t *count);

/**
  * \brief Get count of Buffer processed by h/w
  *
  * \param[in] handle: session handle returned from
  *       agm_session_open
  * \param[in] dir: indicates whether to return the write or
  *       read buffer count
  *
  * \return:  An increasing count of buffers, value wraps back to zero
  * once it reaches SIZE_MAX
  */
size_t agm_get_hw_processed_buff_cnt(uint64_t hndl, enum direction dir);

/**
  * \brief Get list of AIF info objects.
  *
  * \param [in] aif_list: list of aif_info objects
  * \param [in,out] num_aif_info: number of aif info items in the list.
  *     if num_aif_info value is listed as zero, AGM will update num_aif_info with
  *     the number of aif info items in AGM.
  *     if num_aif_info is greater than zero,
  *     AGM will copy client specified num_aif_info of items into aif_list.
  *
  * \return: 0 on success, error code otherwise
  */
int agm_get_aif_info_list(struct aif_info *aif_list, size_t *num_aif_info);

/**
  * \brief Set loopback between capture and playback sessions
  *
  * \param[in] capture_session_id : a non zero capture session id
  * \param[in] playback_session_id: playback session id.
  * \param[in] state : flag to indicate to enable(true) or disable(false) loopback
  *
  * \return: 0 on success, error code otherwise
  */
int agm_session_set_loopback(uint32_t capture_session_id,
               uint32_t playback_session_id, bool state);

/**
  * \brief Set ec reference on capture session
  *
  * \param[in] capture_session_id : a non zero capture session id
  * \param[in] aif_id: aif_id on RX path.
  * \param[in] state : flag to indicate to enable(true) or disable(false) ec_ref
  *
  * \return: 0 on success, error code otherwise
  */
int agm_session_set_ec_ref(uint32_t capture_session_id, uint32_t aif_id,
                                                            bool state);

/**
  * \brief send eos of the session.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  *
  * \return 0 on success, error code otherwise
  */
int agm_session_eos(uint64_t handle);

/**
  * \brief get timestamp of the session.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  * \param[out] timestamp - updated with valid timestamp if the
  *     operation is successful.
  *
  * \return 0 on success, error code otherwise
  */
int agm_get_session_time(uint64_t handle, uint64_t *timestamp);

/**
  * \brief get timestamp of last read buffer.
  *
  * \param[in] session_id - Valid audio session id
  * \param[out] timestamp - updated with valid timestamp if the
  *       operation is successful.
  *
  * \return 0 on success, error code otherwise
  */
int agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp);

/**
 * \brief Get shared memory buf_info of a given session
 *
 * \param[in] session_id - Valid audio session id
 * \param[out] buf_info - agm_buf_info structure with dma_buf_fd
 * \param[in] buf_info - flag to determine data buf/pos buf
 *
 * \return 0 on success, error code on failure.
 * If the session is not opened,
 * api will return failure.
 */
int agm_session_get_buf_info(uint32_t session_id, struct agm_buf_info *buf_info, uint32_t flag);

/**
  * \brief This api is a no-op if agm runs in clients context.
  *        In scenarios where AGM runs in its own process context
  *        and clients talk to AGM over IPC, this api could be used
  *        by clients to register a callback with the agm client wrapper
  *        implentation, which is then responsible to trigger this callback
  *        when the agm service crashes. Once the callback is invoked clients
  *        then can decide on the approach they want to take with respect to
  *        clean up of the agm sesisons

  * \param[in] agm_service_crash_cb - Callback passed by clients
  * \param[in] cookie - Cookie that client expect to be passed during the invocation
  *                     of the callback.
  *
  * \return 0 on success, error code otherwise
  */

int agm_register_service_crash_callback(agm_service_crash_cb cb,
                                         uint64_t cookie);

/**
  * \brief set gapless metadata of the session.
  *
  * \param[in] handle - Valid session handle obtained
  *       from agm_session_open
  * \param[in] type - Silence Type (Initial or Trailing)
  * \param[in] silence - Initial/Trailing silence samples to be
  *       removed
  *
  * \return 0 on success, error code otherwise
  */
int agm_set_gapless_session_metadata(uint64_t handle, enum agm_gapless_silence_type type,
                                     uint32_t silence);

/**
 * \brief Write data buffers with metadata to session
 *
 * \param[in] handle: session handle returned from
 *       agm_session_open
 * \param[in] buff: agm_buffer where data will be copied from
 * \param[in] consumed size: Actual number of bytes that were consumed by AGM
 *
 * \return 0 on success, error code otherwise
 */
int agm_session_write_with_metadata(uint64_t hndl, struct agm_buff *buff,
                                    size_t *consumed_size);

/**
 * \brief Read data buffers with metadata to session
 *
 * \param[in] handle: session handle returned from
 *       agm_session_open
 * \param[in] buff: agm_buffer where data will be copied to
 * \param[in] captured_size: Actual number of bytes that were captured
 *
 * \return 0 on success, error code otherwise
 */
int agm_session_read_with_metadata(uint64_t hndl, struct agm_buff *buff,
                                    uint32_t *captured_size);

/**
 * \brief Helps set config for non tunnel mode (rx and tx path)
 *
 * \param[in] handle: session handle returned from
 *       agm_session_open
 * \param[in] session_config - valid stream configuration of the
 *       sessions
 * \param[in] in_media_config - valid media configuration of the
 *       input data.
 * \param[in] in_buffer_config - buffer configuration for the
 *       input data path.
 * \param[in] out_media_config - valid media configuration of the
 *       output data.
 * \param[in] out_buffer_config - buffer configuration for the
 *       output data path.
 *
 * \return 0 on success, error code otherwise
 */
int agm_session_set_non_tunnel_mode_config(uint64_t hndl,
                                       struct agm_session_config *session_config,
                                       struct agm_media_config *in_media_config,
                                       struct agm_media_config *out_media_config,
                                       struct agm_buffer_config *in_buffer_config,
                                       struct agm_buffer_config *out_buffer_config);


/**
  * \brief Get list of group AIF objects.
  *
  * \param [in] aif_list: list of group aif_info objects
  * \param [in,out] num_groups: number of group aif items in the list.
  *     if num_groups value is listed as zero, AGM will update num_groups with
  *     the number of group aif items in AGM.
  *     if num_groups is greater than zero,
  *     AGM will copy client specified num_groups of items into aif_list.
  *
  * \return: 0 on success, error code otherwise
  */
int agm_get_group_aif_info_list(struct aif_info *aif_list, size_t *num_groups);

 /**
  * \brief Set media configuration for a group AIF.
  *
  * \param[in] aif_group_id - Valid group id
  * \param[in] media_config - valid media configuration for the
  *       audio interafce.
  *
  *  \return 0 on success, error code on failure.
  *       If the audio interface is already in use and the
  *       new media_config is different from previous, api will return
  *       failure.
  */
int agm_aif_group_set_media_config(uint32_t aif_group_id,
                          struct agm_group_media_config *media_config);

/**
 * \brief Write buffers containing codec params to session on datapath
 *
 * \param[in] session_id - Valid audio session id
 * \param[in] buff: agm_buffer where data will be copied from
 *
 * \return 0 on success, error code otherwise
 */
int agm_session_write_datapath_params(uint32_t session_id, struct agm_buff *buff);

/**
  * \brief Dump AGM information based on client
  *
  * \param[in] dump_info - dump information populated by
  *       the client to facilitate dump
  *
  *  \return 0 on success, error code on failure.
  */
int agm_dump(struct agm_dump_info *dump_info);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* _AGM_INTF_H_ */
