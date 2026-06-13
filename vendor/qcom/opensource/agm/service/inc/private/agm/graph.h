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
 */

#ifndef GPH_OBJ_H
#define GPH_OBJ_H

#include <agm/agm_list.h>
#include <agm/device.h>
#include <agm/session_obj.h>
#include <agm/agm_priv.h>

#define ATTRIBUTES_DATA_MODE_MASK 0x3
#define DATA_MODE_FLAG_SHMEM 0x0 /**< shared memory mode */
#define DATA_MODE_FLAG_BLOCKING 0x1 /**< heap memory mode blocking */
#define DATA_MODE_FLAG_NON_BLOCKING 0x2 /**< heap memory mode non-blocking */

typedef enum state
{
    CLOSED = 0x0,
    OPENED = 0x01,
    PREPARED = 0x10,
    STARTED = 0x100,
    STOPPED = 0x1000,
} graph_state_t;


/**
 * \brief Callback function signature for events to client
 *
 * \param[in] event_params: holds all event related info
 * \param[in] client_data: client data that was provided during cb registration
 */
typedef void (*event_cb)(struct agm_event_cb_params *event_params,
                         void *client_data);

struct graph_obj;

/**
 *\brief Initialize graph handling module, this in turn initializes
 * GSL with correct ACDB data file. This should be triggered only once
 * during the lifetime of the service.
 *
 * retuns AR_EOK on success and error code on failure.
 */
int graph_init();

/**
 *\brief De initialize graph handling module,
 * this in turn deinitializes GSL.
 *
 * retuns AR_EOK on success and error code on failure.
 */
int graph_deinit();

/**
 *graph object specfici APIs
 */

/**
 *\brief Opens graph based on the shared gkvs and ckvs
 * All other subsequent operations on the graph are valid
 * only after a successfull open call. It is expected that
 * session and device objects should not go away till the
 * associated graph object is present.
 * The order in which the objects should be created is
 * session, device in any order then the last object to be
 * created is graph. Similary when closing the session, the
 * first object to be closed is the graph object and then
 * the session/device objects.
 *\param [in] meta_data_kv: composite graph and calibration
 *        key vectors needed to setup the complete graph.
 *\param [in] session_obj: session obj
 *\param [in] device_obj: Device to which the session is
 *        connected to.In case of SSMD/MSMD usecases, the
 *        second device would be added to the graph using
 *        graph_add api.
 *\param [in/out] graph_obj: Graph object created based on
 *        session and device objects.
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_open(struct agm_meta_data_gsl *meta_data_kv,
                   struct session_obj *ses_obj,
                   struct device_obj *dev_obj,
                   struct graph_obj **gph_obj);

/**
 *\brief prepare modules in the graph.
 *\param [in] graph_obj: associated graph obj
 *\param [in] cb: pointer to callback function implemented by
 *                the client
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_register_cb(struct graph_obj *gph_obj, event_cb cb,
                          void *client_data);

/**
  * \brief Register for events from Modules. Not needed for data path events.
  *
  * \param[in] graph_obj - Valid graph_obj
  * \param[out] evt_reg_info - event specific configuration.
  *
  * \return 0 on success, error code otherwise
  */
int graph_register_for_events(struct graph_obj *gph_obj,
                              struct agm_event_reg_cfg *evt_reg_cfg);


/**
 *\brief Set a custom config on the graph
 * the config is a self sustained payload which is passed as is
 * to GSL. Client is expected to ensure to form correct payload.
 * Client should ensure that custom configs and the state requirement
 * to configure modules with this custom config is adhered to.
 * Graph object wont be able to force this policy.
 *\param [in] graph_obj: associated graph obj
 *\param [in] config: payload to pass to SPF via GSL.
 *
 * return AR_EOK on success, or error code otherwise.
 */
int graph_set_config(struct graph_obj *gph_obj, void *payload,
                     size_t payload_size);

/**
 *\brief issue a prepare command to modules in the graph.
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on sucess or error code otherwise.
 */
int graph_prepare(struct graph_obj *gph_obj);

/**
 *\brief Issue start command to modules in the graph.
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_start(struct graph_obj *gph_obj);

/**
 *\brief Read audio data captured by the graph.
 *\param [in] graph_obj: associated graph obj
 *\param [in/out] size: size of data to be read.
 *\param [in/out] buffer: buffer where data is copied/filled to.
 *
 * return num of bytes read on success or error code on failure.
 */
int graph_read(struct graph_obj *gph_obj, struct agm_buff *buffer, size_t *size);

/**
 *\brief write audio data to be rendered
 *\param [in] graph_obj: associated graph obj
 *\param [in/out] size: size of data to be written
 *\param [in/out] buffer: buffer from where data is copied
 *
 * return zero on success or error code on failure.
 */
int graph_write(struct graph_obj *gph_obj, struct agm_buff *buffer, size_t *size);

/**
 *\brief pause an existing graph.
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_pause(struct graph_obj *gph_obj);

/**
 *\brief flush an existing graph.
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_flush(struct graph_obj *gph_obj);

/**
 *\brief resume an existing graph.
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_resume(struct graph_obj *gph_obj);

/**
 *\brief suspend an existing graph.
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_suspend(struct graph_obj *gph_obj);

/**
 *\brief add a new audio path (graph/subgraph) to an exisitng graph.
 * Assumption here is only the device leg would always be updated,
 * as a part of the passed GKV.
 *\param [in] graph_obj: associated graph obj
 *\param [in] meta_data_kv: composite graph and calibration key vector
 *        which describe only the new graph. In case of SSSD to SSMD
 *        transition, this argument would only represent the new device and
 *        stream connection.e.g S1D1 is moving to S1D1D2, then graph_add
 *        meta data would only have S1D2 related key vectors.
 *\param [in] dev_obj: This is an optional argument, pass the device obj
 *        if a new device is getting added to the current graph
 *        e.g. in case of SSSD to SSMD usecase transition. In cases
 *        such as EC_Ref or SideTone, graph_add would just add a new
 *        processing path into an existing graph and hence device object
 *        in such cases wont be needed.
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_add(struct graph_obj *gph_obj,
                  struct agm_meta_data_gsl *meta_data_kv,
                  struct device_obj *dev_obj);

/**
 *\brief change an exisitng graph.
 *\param [in] graph_obj: associated graph obj
 *\param [in] meta_data_kv: composite graph and calibration key vector
 *        which describes the new complete graph (new updated graph).
 *        Expectation here is that only the device leg of the graph
 *        gets updated/replaced. Session related gkv remains the same.
 *\param [in] dev_obj: Pass the device obj
 *        e.g. in case of device switch
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_change(struct graph_obj *gph_obj,
                     struct agm_meta_data_gsl *meta_data_kv,
                     struct device_obj *dev_obj);


/**
 *\brief remove certain graph modules from an exisitng graph.
 *\param [in] graph_obj: associated graph obj
 *\param [in] gkv: graph key vector
 *        which describes part of graph(subgraph) to be removed
 *        from the current graph
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_remove(struct graph_obj *gph_obj,
                 struct agm_meta_data_gsl *meta_data_kv);

/**
 *\brief Issue stop to the associated graph
 *\param [in] graph_obj: associated graph obj
 *\param [in] gkv: graph key vector
 *        which describes part of graph(subgraph) to be removed
 *        from the current graph
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_stop(struct graph_obj *gph_obj,
               struct agm_meta_data_gsl *meta_data);

/**
 *\brief close the graph, clears the graph object related
 * memory.
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_close(struct graph_obj *gph_obj);

/**
 *\brief return the no of buffers consumed/captured by the HW(SPF).
 * memory.
 *\param [in] graph_obj: associated graph obj
 *\param [in] dir      : specifies the path for which information is requested
 *                       e.g. RX/TX.
 * returns no of buffers consumed/captured.
 */
size_t graph_get_hw_processed_buff_cnt(struct graph_obj *gph_obj,
                                    enum direction dir);

int graph_get_tags_with_module_info(struct agm_key_vector_gsl *gkv,
                                    void *payload, size_t *size);

int graph_set_config_with_tag(struct graph_obj *gph_obj,
                              struct agm_key_vector_gsl *gkv,
                              struct agm_tag_config_gsl *tag_config);

int graph_get_config(struct graph_obj *graph_obj, void *payload,
                     size_t payload_size);

int graph_set_cal(struct graph_obj *gph_obj,
                              struct agm_meta_data_gsl *meta_data);

int graph_rw_acdb_param(void *payload, bool is_param_write);

/**
 *\brief Issue eos to the associated graph
 *\param [in] graph_obj: associated graph obj
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_eos(struct graph_obj *gph_obj);

/**
 *\brief Get timestamp of the associated running graph
 *\param [in] graph_obj: associated graph obj
 *\param [out] timestamp: updated the timestamp value if success
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_get_session_time(struct graph_obj *gph_obj, uint64_t *timestamp);

/**
 *\brief Get timestamp of the last read buffer
 *\param [in] graph_obj: associated graph obj
 *\param [out] timestamp: updated timestamp value if success
 *
 * return AR_EOK on success or error code otherwise.
 */
int graph_get_buffer_timestamp(struct graph_obj *gph_obj, uint64_t *timestamp);

/**
 *\brief Get buffer shared memory info
 *\param [in] graph_obj: associated graph obj
 *\param [out] buf_info: agm_buf_info containing dma_buf info
 *\param [in] flag: either DATA_BUF or POS_BUF or both
 *
 * return CASA_EOK on success or error code otherwise.
 */
int graph_get_buf_info(struct graph_obj *gph_obj,
    struct agm_buf_info *buf_info, uint32_t flag);

/**
 *\brief Set gapless metadata of the associated running graph
 *\param [in] graph_obj: associated graph obj
 *\param [in] type: Silence Type (Initial/Trailing)
 *\param [in] trailing_silence: Initial/Trailing silence samples to remove
 *
 * return CASA_EOK on success or error code otherwise.
 */
int graph_set_gapless_metadata(struct graph_obj *graph_obj,
                          enum agm_gapless_silence_type type,
                               uint32_t silence);

/**
 *\brief Set tagged parameter to acdb
 *\param [in] graph_key_vect: graph key vector list
 *\param [in] tag_id: tag for the module
 *\param [in] tag_key_vect: tag key vector list
 *\param [in] payload: parameter data
 *\param [in] payload_size: size of the payload
 *
 * return CASA_EOK on success or error code otherwise.
 */
int graph_set_tag_data_to_acdb(
    struct agm_key_vector_gsl *graph_key_vect, uint32_t tag_id,
    struct agm_key_vector_gsl *tag_key_vect, uint8_t *payload,
    uint32_t payload_size);

/**
 *\brief Set tagged parameter to acdb
 *\param [in] graph_key_vect: graph key vector list
 *\param [in] calibration_key_vect: calibration key vector list
 *\param [in] payload: parameter data
 *\param [in] payload_size: size of the payload
 *
 * return CASA_EOK on success or error code otherwise.
 */

int graph_set_cal_data_to_acdb(
    struct agm_key_vector_gsl *graph_key_vect,
    struct agm_key_vector_gsl *cal_key_vect, uint8_t *payload,
    uint32_t payload_size);

/**
 *\brief Get tagged parameter to acdb
 *\param [in] graph_key_vect: graph key vector list
 *\param [in] tag_id: tag for the module
 *\param [in] tag_key_vect: tag key vector list
 *\param [in/out] payload: query buffer for in and return buffer for out
 *\param [in/out] payload_size: size of the payload for query buffer at in and
 *                              return buffer at out
 *
 * return CASA_EOK on success or error code otherwise.
 */

int graph_get_tagged_data(const struct agm_key_vector_gsl *graph_key_vect,
    uint32_t tag, struct agm_key_vector_gsl *tag_key_vect, uint8_t *payload,
    size_t *payload_size);

/**
 *\brief Get TKV or CKV parameter from acdb
 *\param [in] graph_key_vect: graph key vector list
 *\param [in] tag: tag for the module. use 0 for CKV.
 *\param [in] tag_key_vect: tag key vector list
 *\param [in/out] payload: query buffer for in and return buffer for out
 *\param [in/out] payload_size: size of the payload for query buffer at in and
 *                              return buffer at out
 *
 * return CASA_EOK on success or error code otherwise.
 */
int graph_get_tckv_data_from_acdb(
    const struct agm_key_vector_gsl *graph_key_vect, uint32_t tag,
    struct agm_key_vector_gsl *tag_key_vect, uint8_t *payload,
    size_t *payload_size);

/**
 *\ brief Enable persistence on acdb
 *\ param [in] enable_flag: enable or disable
 *
 * return CASA_EOK on success or error code otherwise.
 */

int32_t graph_enable_acdb_persistence(uint8_t enable_flag);

int graph_set_media_config_datapath(struct graph_obj *gph_obj);
#endif /*GPH_OBJ_H*/
