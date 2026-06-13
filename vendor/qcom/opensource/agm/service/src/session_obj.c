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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#define LOG_TAG "AGM: session"

#include <malloc.h>
#include <string.h>
#include <agm/session_obj.h>
#include <agm/utils.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_SESSION_OBJ
#include <log_utils.h>
#endif

#ifdef AGM_USE_CUTILS
#include <cutils/trace.h>
#include <cutils/properties.h>
static uint32_t TRACE_TAG = ATRACE_TAG_NEVER;
char tracestr[100] = "agm: ";
#define agm_trace_begin(str)  \
        strlcat(tracestr, str, sizeof(tracestr));\
        atrace_begin(TRACE_TAG, tracestr);\
        strlcpy(tracestr, "agm: ", sizeof(tracestr));

#define agm_trace_end() \
        atrace_end(TRACE_TAG);

#define check_and_enable_traces() \
    char val[PROPERTY_VALUE_MAX] = {0}; \
    if (property_get("vendor.audio.agm.enable.traces", val, "") && !strncmp(val, "true", 4)) { \
        TRACE_TAG = ATRACE_TAG_AUDIO | ATRACE_TAG_HAL; \
    }
#else
#define agm_trace_begin(str)
#define agm_trace_end()
#define check_and_enable_traces()
#endif

#define GSL_EVENT_SRC_MODULE_ID_GSL 0x2001 // DO NOT CHANGE

//forward declarations
static int session_close(struct session_obj *sess_obj);
static int session_set_loopback(struct session_obj *sess_obj,
                           uint32_t session_id, bool enable);
static pthread_mutex_t hwep_lock;
static struct aif *aif_obj_get_from_pool(struct session_obj *sess_obj,
                                      uint32_t aif)
{
    struct listnode *node;
    struct aif *aif_node;

    list_for_each(node, &sess_obj->aif_pool) {
        aif_node = node_to_item(node, struct aif, node);
        if (aif_node->aif_id == aif)
            return aif_node;
    }

    return NULL;
}

static struct aif* aif_obj_create(struct session_obj *sess_obj __unused, int aif_id)
{
    struct aif *aif_obj = NULL;
    struct device_obj *dev_obj = NULL;
    int ret = 0;

    aif_obj = calloc(1, sizeof(struct aif));
    if (!aif_obj) {
        AGM_LOGE("Memory allocation failed for aif object\n");
        return aif_obj;
    }

    ret = device_get_obj(aif_id, &dev_obj);
    if (ret || !dev_obj) {
        AGM_LOGE("Error:%d retrieving device object with id:%d \n",
                                       ret, aif_obj->aif_id);
        goto done;
    }
    aif_obj->aif_id = aif_id;
    aif_obj->dev_obj = dev_obj;

done:
    return aif_obj;
}

/* returns aif_obj associated with aif id in the session obj */
int aif_obj_get(struct session_obj *sess_obj, int aif_id, struct aif **aif_obj)
{
    // return from list, if not there create, add to list and return
    struct aif *tobj = NULL;
    int ret = 0;

    tobj = aif_obj_get_from_pool(sess_obj, aif_id);
    if (!tobj) {
        //AGM_LOGE("Couldnt find a aif object in the list, creating one\n");

        tobj = aif_obj_create(sess_obj, aif_id);
        if (!tobj || !tobj->dev_obj) {
            AGM_LOGE("Couldnt create an aif object\n");
            ret = -ENOMEM;
            return ret;
        }
        list_add_tail(&sess_obj->aif_pool, &tobj->node);
    }

    *aif_obj = tobj;

    return ret;
}

/* returns aif_obj associated with aif id in the session obj with
 * state not as specified in the argument */
uint32_t aif_obj_get_count_with_state(struct session_obj *sess_obj,
                     enum aif_state state, bool exact_state_match)
{
    uint32_t count = 0;
    struct listnode *node;
    struct aif *temp = NULL;

    //check how many devices in connected state
    list_for_each(node, &sess_obj->aif_pool) {
        temp = node_to_item(node, struct aif, node);
        if (!temp) {
            AGM_LOGE("Error could not find aif node\n");
            continue;
        }

        if ((exact_state_match  && temp->state == state) ||
            (!exact_state_match && temp->state >= state)) {
            count++;
        }
    }

    return count;
}

static struct agm_meta_data_gsl* session_get_merged_metadata(struct session_obj *sess_obj)
{
    struct agm_meta_data_gsl *merged = NULL;
    struct agm_meta_data_gsl *temp = NULL;
    enum agm_session_mode sess_mode = sess_obj->stream_config.sess_mode;
    struct listnode *node;
    struct aif *aif_node;

    if (sess_mode != AGM_SESSION_NON_TUNNEL) {
        list_for_each(node, &sess_obj->aif_pool) {
            aif_node = node_to_item(node, struct aif, node);
            if (aif_node->state == AIF_CLOSED) {
                AGM_LOGD("ignore closed AIF node");
                continue;
            }
            pthread_mutex_lock(&aif_node->dev_obj->lock);
            merged = metadata_merge(4, temp, &sess_obj->sess_meta,
                           &aif_node->sess_aif_meta, &aif_node->dev_obj->metadata);
            pthread_mutex_unlock(&aif_node->dev_obj->lock);
            if (temp) {
                metadata_free(temp);
                free(temp);
            }
            temp = merged;
        }
    } else {
        merged = &sess_obj->sess_meta;
    }

    return merged;
}

static struct agm_meta_data_gsl* session_get_merged_metadata_without_aif(struct session_obj *sess_obj)
{
    struct agm_meta_data_gsl *merged = NULL;
    struct agm_meta_data_gsl *temp = NULL;
    struct listnode *node;
    struct aif *aif_node;

    list_for_each(node, &sess_obj->aif_pool) {
        aif_node = node_to_item(node, struct aif, node);
        if (aif_node->state == AIF_CLOSED) {
            AGM_LOGD("ignore closed AIF node");
            continue;
        }
        merged = metadata_merge(3, temp, &sess_obj->sess_meta,
                                    &aif_node->sess_aif_meta);
        if (temp) {
            metadata_free(temp);
            free(temp);
        }
        temp = merged;
    }

    return merged;
}

static int session_pool_init()
{
    int ret = 0;
    sess_pool = calloc(1, sizeof(struct session_pool));
    if (!sess_pool) {
        AGM_LOGE("No Memory to create sess_pool\n");
        ret = -ENOMEM;
        goto done;
    }
    list_init(&sess_pool->session_list);
    pthread_mutex_init(&sess_pool->lock, (const pthread_mutexattr_t *) NULL);

done:
    return ret;
}

static void aif_free(struct aif *aif_obj)
{
    metadata_free(&aif_obj->sess_aif_meta);
    free(aif_obj->params);
    free(aif_obj);
}

static void aif_pool_free(struct session_obj *sess_obj)
{
    struct aif *aif_obj;
    struct listnode *node, *next;

    list_for_each_safe(node, next, &sess_obj->aif_pool) {
        aif_obj = node_to_item(node, struct aif, node);
        list_remove(&aif_obj->node);
        aif_free(aif_obj);
    }
}

static void session_cb_pool_free(struct session_obj *sess_obj)
{
    struct session_cb *sess_cb;
    struct listnode *node, *next;

    list_for_each_safe(node, next, &sess_obj->cb_pool) {
        sess_cb = node_to_item(node, struct session_cb, node);
        list_remove(&sess_cb->node);
        free(sess_cb);
    }

}

static void sess_obj_free(struct session_obj *sess_obj)
{
    aif_pool_free(sess_obj);
    session_cb_pool_free(sess_obj);
    metadata_free(&sess_obj->sess_meta);
    free(sess_obj->params);
    free(sess_obj);
}

static void session_pool_free()
{
    struct session_obj *sess_obj;
    struct listnode *node, *next;
    int ret = 0;

    pthread_mutex_lock(&sess_pool->lock);
    list_for_each_safe(node, next, &sess_pool->session_list) {
        sess_obj = node_to_item(node, struct session_obj, node);
        pthread_mutex_lock(&sess_obj->lock);
        ret = session_close(sess_obj);
        if (ret) {
            AGM_LOGE("Error:%d closing session with session id:%d\n",
                                       ret, sess_obj->sess_id);
        }
        pthread_mutex_unlock(&sess_obj->lock);

        //cleanup aif pool from session_object
        list_remove(&sess_obj->node);
        sess_obj_free(sess_obj);
    }
    pthread_mutex_unlock(&sess_pool->lock);
    free(sess_pool);
}

static struct session_obj* session_obj_create(int session_id)
{
    struct session_obj *obj = NULL;

    obj = calloc(1, sizeof(struct session_obj));
    if (!obj) {
        AGM_LOGE("Memory allocation failed for sesssion object\n");
        return obj;
    }

    obj->sess_id = session_id;
    list_init(&obj->aif_pool);
    list_init(&obj->cb_pool);
    pthread_mutex_init(&obj->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&obj->cb_pool_lock, (const pthread_mutexattr_t *) NULL);

    return obj;
}

struct session_obj *session_obj_retrieve_from_pool(uint32_t session_id)
{
    struct session_obj *obj = NULL;
    struct listnode *node;

    pthread_mutex_lock(&sess_pool->lock);
    list_for_each(node, &sess_pool->session_list) {
        obj = node_to_item(node, struct session_obj, node);
        if (obj->sess_id == session_id)
            break;
        else
            obj = NULL;
    }
    pthread_mutex_unlock(&sess_pool->lock);

    return obj;
}

struct session_obj *session_obj_get_from_pool(uint32_t session_id)
{
    struct session_obj *obj = NULL;
    struct listnode *node;

    pthread_mutex_lock(&sess_pool->lock);
    list_for_each(node, &sess_pool->session_list) {
        obj = node_to_item(node, struct session_obj, node);
        if (obj->sess_id == session_id)
            break;
        else
            obj = NULL;
    }

    if (!obj) {
        //AGM_LOGE("Couldnt find a session object in the list,
        //                             creating one\n");
        obj = session_obj_create(session_id);
        if (!obj) {
            AGM_LOGE("Couldnt create a session object\n");
            goto done;
        }
        list_add_tail(&sess_pool->session_list, &obj->node);
    }

done:
    pthread_mutex_unlock(&sess_pool->lock);
    return obj;
}
int session_obj_valid_check(uint64_t hndl)
{

    struct session_obj *obj = NULL;
    struct listnode *node;

    pthread_mutex_lock(&sess_pool->lock);
    list_for_each(node, &sess_pool->session_list) {
        obj = node_to_item(node, struct session_obj, node);
        if (obj == hndl) {
            pthread_mutex_unlock(&sess_pool->lock);
            return  1;
        }
    }
    pthread_mutex_unlock(&sess_pool->lock);
    return 0;
}

/* returns session_obj associated with session id */
int session_obj_get(int session_id, struct session_obj **obj)
{
    // return from list, if not there create, add to list and return
    struct session_obj *tobj = NULL;
    int ret = 0;

    tobj = session_obj_get_from_pool(session_id);
    if (!tobj) {
        AGM_LOGE("Couldnt find or create session_obj\n");
        ret = -ENOMEM;
    }

    *obj = tobj;
    return ret;
}

static int session_set_loopback(struct session_obj *sess_obj,
                                uint32_t pb_id, bool enable)
{
    int ret = 0;
    struct session_obj *pb_obj = NULL;
    struct agm_meta_data_gsl *capture_metadata = NULL;
    struct agm_meta_data_gsl *playback_metadata = NULL;
    struct agm_meta_data_gsl *merged_metadata = NULL;

    /*
     * 1. merged metadata of pb session + cap session
     * 2. call graph_add
     * 3. call start (prepare doesnt achieve anything so skip)
     * 4. Expectation for loopback is that its establishing an edge b/w TX and RX session
     *    and no new subgraphs are added and hence no gsl_start/prepare.
     *    So no new modules/subgraphs which require configuration is expected and hence
     *    no separate setparams() for loopback for now.
     */
    ret = session_obj_get(pb_id, &pb_obj);
    if (ret) {
        AGM_LOGE("Error:%d getting session object with session id:%d\n",
                                                      ret, pb_id);
        goto done;
    }

    capture_metadata = session_get_merged_metadata(sess_obj);
    if (!capture_metadata) {
        ret = -ENOMEM;
        AGM_LOGE("Error:%d, merging metadata with session id=%d\n",
                                         ret, sess_obj->sess_id);
        goto done;
    }

    playback_metadata = session_get_merged_metadata(pb_obj);
    if (!playback_metadata) {
        ret = -ENOMEM;
        AGM_LOGE("Error:%d, merging metadata with session id=%d\n",
                                                    ret, pb_id);
        goto done;
    }

    merged_metadata = metadata_merge(2, capture_metadata, playback_metadata);
    if (!merged_metadata) {
        ret = -ENOMEM;
        AGM_LOGE("Error:%d, merging metadata with playback"
                   "session id=%d and capture session id=%d\n",
                     ret, pb_id, sess_obj->sess_id);
        goto done;
    }

    if (enable)
        ret = graph_add(sess_obj->graph, merged_metadata, NULL);
    else
        ret = graph_remove(sess_obj->graph, merged_metadata);

    if (ret) {
        AGM_LOGE("Error:%d graph %s failed for session_id: %d\n",
         ret, ((enable == true) ? "add":"remove"), sess_obj->sess_id);
        goto done;
    }

done:
    if (capture_metadata) {
        metadata_free(capture_metadata);
        free(capture_metadata);
    }
    if (playback_metadata) {
        metadata_free(playback_metadata);
        free(playback_metadata);
    }
    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }
    return ret;
}

static int session_set_ec_ref(struct session_obj *sess_obj, uint32_t aif_id,
                                                                bool enable)
{
    int ret = 0;
    struct agm_meta_data_gsl *capture_metadata = NULL;
    struct agm_meta_data_gsl *merged_metadata = NULL;
    struct device_obj *dev_obj = NULL;


    ret = device_get_obj(aif_id, &dev_obj);
    if (ret) {
        AGM_LOGE("Error:%d, unable to get dev_obj with aif_id=%d\n",
                                                 ret, aif_id);
        goto done;
    }

    capture_metadata = session_get_merged_metadata_without_aif(sess_obj);
    if (!capture_metadata) {
        ret = -ENOMEM;
        AGM_LOGE("Error:%d, merging metadata with session id=%d\n",
                                     ret, sess_obj->sess_id);
        goto done;
    }

    pthread_mutex_lock(&dev_obj->lock);
    merged_metadata = metadata_merge(2, capture_metadata, &dev_obj->metadata);
    pthread_mutex_unlock(&dev_obj->lock);
    if (!merged_metadata) {
        ret = -ENOMEM;
        AGM_LOGE("Error:%d, merging metadata with capture \
                   session id=%d aif_id:%d \n",
                   ret, sess_obj->sess_id, aif_id);
        goto done;
    }

    if (enable)
        ret = graph_add(sess_obj->graph, merged_metadata, NULL);
    else
        ret = graph_remove(sess_obj->graph, merged_metadata);

    if (ret) {
        AGM_LOGE("Error:%d graph %s failed for session_id: %d\n",
            ret, ((enable == true) ? "add":"remove"),
                                            sess_obj->sess_id);
        goto done;
    }

done:
    if (capture_metadata) {
        metadata_free(capture_metadata);
        free(capture_metadata);
    }
    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }
    return ret;
}

static int session_disconnect_aif(struct session_obj *sess_obj,
                    struct aif *aif_obj, uint32_t opened_count)
{
    int ret = 0;
    struct agm_meta_data_gsl *merged_metadata = NULL;
    struct agm_meta_data_gsl *merged_meta_sess_aif = NULL;
    struct agm_meta_data_gsl temp = {0};
    struct graph_obj *graph = sess_obj->graph;

    pthread_mutex_lock(&aif_obj->dev_obj->lock);
    merged_metadata = metadata_merge(3, &sess_obj->sess_meta,
                      &aif_obj->sess_aif_meta, &aif_obj->dev_obj->metadata);
    pthread_mutex_unlock(&aif_obj->dev_obj->lock);
    if (!merged_metadata) {
        AGM_LOGE("No memory to create merged_metadata session_id: %d, \
                      audio interface id:%d \n",
                       sess_obj->sess_id, aif_obj->aif_id);
        ret = -ENOMEM;
        goto done;
    }

    pthread_mutex_lock(&hwep_lock);
    if (opened_count == 1) {
        //this is SSSD condition, hence stop just the stream/stream-device,
        //merged only sess-aif, aif
        pthread_mutex_lock(&aif_obj->dev_obj->lock);
        merged_meta_sess_aif = metadata_merge(2, &aif_obj->sess_aif_meta,
                                            &aif_obj->dev_obj->metadata);
        pthread_mutex_unlock(&aif_obj->dev_obj->lock);
        if (!merged_meta_sess_aif) {
            AGM_LOGE("No memory to create merged_metadata session_id: %d, \
                          audio interface id:%d \n",
                          sess_obj->sess_id, aif_obj->aif_id);
            ret = -ENOMEM;
            pthread_mutex_unlock(&hwep_lock);
            goto done;
        }

        temp.gkv = merged_metadata->gkv;
        temp.ckv = merged_metadata->ckv;
        temp.sg_props = merged_meta_sess_aif->sg_props;
        agm_trace_begin("graph_stop");
        ret = graph_stop(graph, &temp);
        agm_trace_end();
        if (ret) {
            AGM_LOGE("Error:%d graph stop failed session_id: %d, \
                      audio interface id:%d \n",
                      ret, sess_obj->sess_id, aif_obj->aif_id);
        }
    } else {
        ret = graph_remove(graph, merged_metadata);
        if (ret) {
            AGM_LOGE("Error:%d graph remove failed session_id: %d, \
                      audio interface id:%d \n",
                      ret, sess_obj->sess_id, aif_obj->aif_id);
        }
    }
    agm_trace_begin("device_stop");
    if (sess_obj->state == SESSION_STARTED)
        device_stop(aif_obj->dev_obj);
    agm_trace_end();

    agm_trace_begin("device_close");
    ret = device_close(aif_obj->dev_obj);
    agm_trace_end();
    if (ret) {
        AGM_LOGE("Error:%d closing device object with id:%d \n",
            ret, aif_obj->aif_id);
    }
    pthread_mutex_unlock(&hwep_lock);

done:
    if (merged_meta_sess_aif) {
        metadata_free(merged_meta_sess_aif);
        free(merged_meta_sess_aif);
    }

    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }
    return ret;
}

static void graph_event_cb(struct agm_event_cb_params *event_params,
                         void *client_data)
{
    struct session_obj *sess_obj = NULL;
    struct session_cb *sess_cb;
    struct listnode *node, *next;
    uint32_t session_id = (uint32_t)((uintptr_t)client_data);

    if (!event_params) {
        AGM_LOGE("event_parms is NULL");
        return;
    }

    sess_obj = session_obj_retrieve_from_pool(session_id);
    if (!sess_obj) {
        AGM_LOGE("Incorrect client_data:%d, doesn't match sess_obj from pool",
                                        session_id);
        return;
    }

    pthread_mutex_lock(&sess_obj->cb_pool_lock);
    list_for_each_safe(node, next, &sess_obj->cb_pool) {
        sess_cb = node_to_item(node, struct session_cb, node);
        if (sess_cb && sess_cb->cb) {
            /* Filter callbacks based on event_id and event_type */
            if (sess_cb->evt_type == AGM_EVENT_DATA_PATH &&
                event_params->source_module_id == GSL_EVENT_SRC_MODULE_ID_GSL &&
                (event_params->event_id == AGM_EVENT_EOS_RENDERED ||
                event_params->event_id == AGM_EVENT_READ_DONE ||
                event_params->event_id == AGM_EVENT_WRITE_DONE)) {
                sess_cb->cb(sess_obj->sess_id,
                                   (struct agm_event_cb_params *)event_params,
                                    sess_cb->client_data);
            } else if (sess_cb->evt_type == AGM_EVENT_MODULE &&
                       event_params->source_module_id != GSL_EVENT_SRC_MODULE_ID_GSL) {
                sess_cb->cb(sess_obj->sess_id,
                                   (struct agm_event_cb_params *)event_params,
                                    sess_cb->client_data);
            }
        }
    }
    pthread_mutex_unlock(&sess_obj->cb_pool_lock);
}

static int session_apply_aif_tag_params(struct session_obj *sess_obj,
        struct agm_meta_data_gsl *merged_metadata, struct aif *aif_obj)
{
    int ret = 0;
    struct agm_tag_config_gsl tag_config_gsl;
    struct agm_tag_config *tag_config = NULL;

    if (aif_obj->tag_config == NULL)
        return ret;

    tag_config = aif_obj->tag_config;

    tag_config_gsl.tag_id = tag_config->tag;
    tag_config_gsl.tkv.num_kvs = tag_config->num_tkvs;
    tag_config_gsl.tkv.kv = tag_config->kv;

    ret = graph_set_config_with_tag(sess_obj->graph, &merged_metadata->gkv,
                                                &tag_config_gsl);
    if (ret)
        AGM_LOGE("Error:%d setting for sess_aif params with tags \
                  on sess_id:%d, aif_id:%d\n",
                  ret, sess_obj->sess_id, aif_obj->aif_id);

    // free aif tag param after setting.
    // Client need to resend for configuring on next usecase
    free(aif_obj->tag_config);
    aif_obj->tag_config = NULL;

    return ret;
}

static int session_apply_aif_device_params(struct session_obj *sess_obj,
        struct device_obj *dev_obj)
{
    int ret = 0;

    if (!dev_obj)
        return ret;

    pthread_mutex_lock(&dev_obj->lock);

    if ((device_get_state(dev_obj) != DEV_CLOSED) && dev_obj->params != NULL) {
        ret = graph_set_config(sess_obj->graph, dev_obj->params,
                dev_obj->params_size);
        if (ret)
            AGM_LOGE("Error:%d setting device cached params: %d\n",
                    ret, dev_obj->pcm_id);

        // free dev_obj params after setting.
        // Client need to resend for configuring on next usecase
        free(dev_obj->params);
        dev_obj->params = NULL;
        dev_obj->params_size = 0;
    }
    pthread_mutex_unlock(&dev_obj->lock);

    return ret;
}

static int session_connect_aif(struct session_obj *sess_obj,
                               struct aif *aif_obj, uint32_t opened_count)
{
    int ret = 0;
    struct agm_meta_data_gsl *merged_metadata = NULL;
    struct graph_obj *graph = sess_obj->graph;

    //step 2.a  merge metadata
    pthread_mutex_lock(&aif_obj->dev_obj->lock);
    merged_metadata = metadata_merge(3, &sess_obj->sess_meta,
                         &aif_obj->sess_aif_meta, &aif_obj->dev_obj->metadata);
    pthread_mutex_unlock(&aif_obj->dev_obj->lock);
    if (!merged_metadata) {
        AGM_LOGE("Error merging metadata session_id:%d aif_id:%d\n",
            sess_obj->sess_id, aif_obj->aif_id);
        ret = -ENOMEM;
        goto done;
    }
    agm_trace_begin("session_connect_aif");

    agm_trace_begin("device_open");
    ret = device_open(aif_obj->dev_obj);
    agm_trace_end();
    if (ret) {
        AGM_LOGE("Error:%d opening device object with id:%d \n",
            ret, aif_obj->aif_id);
        goto done;
    }

    //step 2.b
    if (opened_count == 0) {
        if (sess_obj->state == SESSION_CLOSED) {
            agm_trace_begin("graph_open");
            ret = graph_open(merged_metadata, sess_obj, aif_obj->dev_obj,
                                                       &sess_obj->graph);
            agm_trace_end();
            graph = sess_obj->graph;
            if (ret) {
                AGM_LOGE("Error:%d graph open failed session_id: %d, \
                          audio interface id:%d \n",
                          ret, sess_obj->sess_id, aif_obj->aif_id);
                goto close_device;
            }

            //register callback
            ret = graph_register_cb(graph, graph_event_cb,
                                    (void *)((uintptr_t) sess_obj->sess_id));
            if (ret) {
                AGM_LOGE("Error:%d graph callback registration failed \
                         session_id: %d\n", ret, sess_obj->sess_id);
                goto graph_cleanup;
            }
        } else {
            agm_trace_begin("graph_change");
            ret = graph_change(graph, merged_metadata, aif_obj->dev_obj);
            agm_trace_end();
            if (ret) {
                AGM_LOGE("Error:%d graph change failed session_id: %d, \
                         audio interface id:%d \n",
                         ret, sess_obj->sess_id, aif_obj->aif_id);
                goto close_device;
            }
        }
    } else {
            agm_trace_begin("graph_add");
            ret = graph_add(graph, merged_metadata, aif_obj->dev_obj);
            agm_trace_end();
            if (ret) {
                AGM_LOGE("Error:%d graph add failed session_id: %d, \
                          audio interface id:%d \n",
                          ret, sess_obj->sess_id, aif_obj->aif_id);
                goto close_device;
            }
    }

    //step 2.c set cached params for stream only in closed
    if (sess_obj->state == SESSION_CLOSED && sess_obj->params != NULL) {
        ret = graph_set_config(graph, sess_obj->params, sess_obj->params_size);
        /* clean up params irrespective of success or failure to avoid
         * impact to next usecase */
        free(sess_obj->params);
        sess_obj->params = NULL;
        sess_obj->params_size = 0;
        if (ret) {
            AGM_LOGE("Error:%d setting session cached params: %d\n",
                ret, sess_obj->sess_id);
            goto graph_cleanup;
        }
    }

    //step 2.d set cached streamdevice params
    if (aif_obj->params != NULL) {
        ret = graph_set_config(graph, aif_obj->params, aif_obj->params_size);
        if (ret) {
            AGM_LOGE("Error:%d setting session cached params: %d\n",
                ret, sess_obj->sess_id);
            goto graph_cleanup;
        }
        free(aif_obj->params);
        aif_obj->params = NULL;
        aif_obj->params_size = 0;
    }

    //step 2.e set cached device params
    ret = session_apply_aif_device_params(sess_obj, aif_obj->dev_obj);
    if (ret)
        goto graph_cleanup;

    //step 2.f set cached streamdevice tagparams
    ret = session_apply_aif_tag_params(sess_obj, merged_metadata, aif_obj);
    if (ret)
        goto graph_cleanup;

    goto done;

graph_cleanup:
    if (opened_count == 0) {
        graph_close(sess_obj->graph);
        sess_obj->graph = NULL;
    } else {
        graph_remove(sess_obj->graph, merged_metadata);
    }

close_device:
    if (aif_obj->params) {
        free(aif_obj->params);
        aif_obj->params = NULL;
        aif_obj->params_size = 0;
    }
    device_close(aif_obj->dev_obj);

done:
    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }
    agm_trace_end();
    return ret;
}

static int session_open_with_first_device(struct session_obj *sess_obj)
{
    int ret = 0;
    struct aif *aif_obj = NULL, *temp = NULL;
    struct listnode *node;

    list_for_each(node, &sess_obj->aif_pool) {
        temp = node_to_item(node, struct aif, node);
        if (temp->state == AIF_OPEN) {
            aif_obj = temp;
            break;
        }
    }

    if (!aif_obj) {
        AGM_LOGE("No Audio interface(Backend) set on session(Frontend):%d\n",
                sess_obj->sess_id);
        ret = -EPIPE;
        goto done;
    }

    ret = session_connect_aif(sess_obj, aif_obj, 0);
    if (ret) {
        AGM_LOGE("Audio interface(Backend):%d <-> session(Frontend):%d \
                   Connect failed error:%d\n",
                     aif_obj->aif_id, sess_obj->sess_id, ret);
        goto done;
    }
    aif_obj->state = AIF_OPENED;

done:
    return ret;
}

static int session_connect_reminder_devices(struct session_obj *sess_obj)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    struct listnode *node;
    uint32_t opened_count = 0;

    // opened_count is 1 because this function is being called
    //after connecting with 1 device
    opened_count = 1;

    list_for_each(node, &sess_obj->aif_pool) {
        aif_obj = node_to_item(node, struct aif, node);
        if (aif_obj && aif_obj->state == AIF_OPEN) {
            ret = session_connect_aif(sess_obj, aif_obj, opened_count);
            if (ret) {
                AGM_LOGE("Audio interface(Backend): %d <-> \
                          session(Frontend):%d Connect failed error:%d\n",
                          aif_obj->aif_id, sess_obj->sess_id, ret);
                goto unwind;
            }

            aif_obj->state = AIF_OPENED;
            opened_count++;
        }
    }

    return 0;

unwind:
    list_for_each(node, &sess_obj->aif_pool) {
        aif_obj = node_to_item(node, struct aif, node);
        if (aif_obj && aif_obj->state == AIF_OPENED) {
            /*TODO: fix the 3rd argument to provide correct count*/
            ret = session_disconnect_aif(sess_obj, aif_obj, 1);
            if (ret) {
                AGM_LOGE("Error:%d initializing session_pool\n",
                                                     ret);
            }
            aif_obj->state = AIF_OPEN;
            opened_count--;
        }
    }
    ret = graph_close(sess_obj->graph);
    if (ret) {
        AGM_LOGE("Error:%d initializing session_pool\n", ret);
    }
    sess_obj->graph = NULL;

    return ret;
}

static int session_open_without_device(struct session_obj *sess_obj)
{
    int ret = 0;
    struct graph_obj *graph = sess_obj->graph;

    if (sess_obj->state == SESSION_CLOSED) {
        ret = graph_open(&sess_obj->sess_meta, sess_obj, NULL, &sess_obj->graph);
        graph = sess_obj->graph;
        if (ret) {
                AGM_LOGE("Error:%d graph open failed session_id: %d\n",
                          ret, sess_obj->sess_id);
                goto done;
            }
            /*register callback*/
            ret = graph_register_cb(graph, graph_event_cb,
                                    (void *)((uintptr_t) sess_obj->sess_id));
            if (ret) {
                AGM_LOGE("Error:%d graph callback registration failed \
                         session_id: %d\n", ret, sess_obj->sess_id);
                goto graph_cleanup;
            }
    }
    //step 2.c set cached params for stream only in closed
    if (sess_obj->state == SESSION_CLOSED && sess_obj->params != NULL) {
        ret = graph_set_config(graph, sess_obj->params, sess_obj->params_size);
        if (ret) {
            AGM_LOGE("Error:%d setting session cached params: %d\n",
                ret, sess_obj->sess_id);
            goto graph_cleanup;
        }
        free(sess_obj->params);
        sess_obj->params = NULL;
        sess_obj->params_size = 0;
    }

    goto done;

graph_cleanup:
        graph_close(sess_obj->graph);
        sess_obj->graph = NULL;
done:
    return ret;
}

static int session_prepare(struct session_obj *sess_obj)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    enum agm_session_mode sess_mode = sess_obj->stream_config.sess_mode;
    struct listnode *node = NULL;
    uint32_t count = 0;

    if (sess_mode != AGM_SESSION_NON_TUNNEL  && sess_mode != AGM_SESSION_NO_CONFIG) {
        count = aif_obj_get_count_with_state(sess_obj, AIF_OPENED, false);
        if (count == 0) {
            AGM_LOGE("Error:%d No aif in right state to proceed with \
                       session start for sessionid :%d\n",
                       ret, sess_obj->sess_id);
           ret = -EINVAL;
           goto done;
        }

        list_for_each(node, &sess_obj->aif_pool) {
            aif_obj = node_to_item(node, struct aif, node);
            if (!aif_obj) {
                AGM_LOGE("Error:%d could not find aif node\n", ret);
                goto done;
            }
            ret = session_apply_aif_device_params(sess_obj, aif_obj->dev_obj);
            if (ret)
                goto done;
        }

        if ((sess_obj->state != SESSION_STARTED)) {
            pthread_mutex_lock(&hwep_lock);
            agm_trace_begin("graph_prepare");
            ret = graph_prepare(sess_obj->graph);
            agm_trace_end();
            pthread_mutex_unlock(&hwep_lock);
            if (ret) {
                AGM_LOGE("Error:%d preparing graph\n", ret);
                goto done;
            } else {
                sess_obj->state = SESSION_PREPARED;
            }
        }
    } else if(sess_obj->state != SESSION_STARTED) {
        agm_trace_begin("graph_prepare");
        ret = graph_prepare(sess_obj->graph);
        agm_trace_end();
        if (ret) {
             AGM_LOGE("Error:%d preparing graph\n", ret);
             goto done;
        } else {
             sess_obj->state = SESSION_PREPARED;
        }
    }

done:
    return ret;
}

static int session_start(struct session_obj *sess_obj)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    enum direction dir = sess_obj->stream_config.dir;
    enum agm_session_mode sess_mode = sess_obj->stream_config.sess_mode;
    struct listnode *node = NULL;
    uint32_t count = 0;
    struct session_obj *pb_obj = NULL;
    struct device_obj *ec_ref_dev_obj = NULL;

    if (sess_mode != AGM_SESSION_NON_TUNNEL && sess_mode != AGM_SESSION_NO_CONFIG) {
        count = aif_obj_get_count_with_state(sess_obj, AIF_OPENED, false);
        if (count == 0) {
            AGM_LOGE("Error:%d No aif in right state to proceed with \
                      session start for session id :%d\n",
                     ret, sess_obj->sess_id);
            ret = -EINVAL;
            goto done;
        }

        if (dir == TX) {
            // For loopback, check if the playback session is in STARTED state,
            //otherwise return failure
            if (sess_obj->loopback_state == true) {
                ret = session_obj_get(sess_obj->loopback_sess_id, &pb_obj);
                if (ret) {
                    AGM_LOGE("Error:%d getting session object with \
                              session id:%d\n",
                              ret, sess_obj->loopback_sess_id);
                    goto done;
                }

                if (pb_obj->state != SESSION_STARTED) {
                    AGM_LOGE("Error:%d Playback session with session id:%d\n"
                              "not in STARTED state, current state:%d\n",
                              ret, pb_obj->sess_id, pb_obj->state);
                    ret = -EINVAL;
                    goto done;
                }
            }

            /*
             * For ec ref, check if the device object is in STARTED,
             * otherwise return failure.
             * The RX(EC) device EP should be in started state, this ensures
             * the RX EP is configured and hence capture session start() succeeds
             * if RX(EC) device EP is not started, graph_start of capture will fail.
             */
            if (sess_obj->ec_ref_state == true) {
                ret = device_get_obj(sess_obj->ec_ref_aif_id, &ec_ref_dev_obj);
                if (ret) {
                    AGM_LOGE("Error:%d getting device object with aif id:%d\n",
                            ret, sess_obj->ec_ref_aif_id);
                    goto done;
                }

                if (device_get_state(ec_ref_dev_obj) != DEV_STARTED) {
                    AGM_LOGE("Error:%d Device object with aif id:%d\n"
                              "not in STARTED state, current state:%d\n",
                              ret, sess_obj->ec_ref_aif_id,
                            ec_ref_dev_obj->state);
                    ret = -EINVAL;
                    goto done;
                }
            }
        }

        pthread_mutex_lock(&hwep_lock);

        //For Slimbus/CP EP - First configure the slave ports via device_prepare/start
        //and then start the master side via graph_start.
        list_for_each(node, &sess_obj->aif_pool) {
            aif_obj = node_to_item(node, struct aif, node);
            if (!aif_obj) {
                AGM_LOGE("Error:%d could not find aif node\n", ret);
                goto device_stop;
            }

            if ((aif_obj->dev_obj->hw_ep_info.intf == SLIMBUS) ||
                (aif_obj->dev_obj->hw_ep_info.intf == BTFM_PROXY)) {
                AGM_LOGD("configuring device early - for SLIMBUS/Connectivity Proxy EPs\n");
                if (aif_obj->state == AIF_OPENED || aif_obj->state == AIF_STOPPED) {
                    agm_trace_begin("device_prepare");
                    ret = device_prepare(aif_obj->dev_obj);
                    agm_trace_end();
                    if (ret) {
                        AGM_LOGE("Error:%d preparing device\n", ret);
                        goto device_stop;
                    }
                    aif_obj->state = AIF_PREPARED;
                }

                if (aif_obj->state == AIF_OPENED || aif_obj->state == AIF_PREPARED ||
                                                     aif_obj->state == AIF_STOPPED ) {
                    agm_trace_begin("device_start");
                    ret = device_start(aif_obj->dev_obj);
                    agm_trace_end();
                    if (ret) {
                        AGM_LOGE("Error:%d starting device id:%d\n",
                                       ret, aif_obj->aif_id);
                        goto device_stop;
                    }
                    aif_obj->state = AIF_STARTED;
                }
            }
        }
        agm_trace_begin("graph_start");
        ret = graph_start(sess_obj->graph);
        agm_trace_end();
        if (ret) {
            AGM_LOGE("Error:%d starting graph\n", ret);
            goto device_stop;
        }

        list_for_each(node, &sess_obj->aif_pool) {
            aif_obj = node_to_item(node, struct aif, node);
            if (!aif_obj) {
                AGM_LOGE("Error:%d could not find aif node\n", ret);
                pthread_mutex_unlock(&hwep_lock);
                goto unwind;
            }

            //Continue/SKIP for SLIMBUS/Connectivity Proxy EP as they are started early.
            if ((aif_obj->dev_obj->hw_ep_info.intf == SLIMBUS) ||
                (aif_obj->dev_obj->hw_ep_info.intf == BTFM_PROXY)) {
                continue;
            }

            if (aif_obj->state == AIF_OPENED || aif_obj->state == AIF_STOPPED) {
                agm_trace_begin("device_prepare");
                ret = device_prepare(aif_obj->dev_obj);
                agm_trace_end();
                if (ret) {
                    AGM_LOGE("Error:%d preparing device\n", ret);
                    pthread_mutex_unlock(&hwep_lock);
                    goto unwind;
                }
                aif_obj->state = AIF_PREPARED;
            }

            if (aif_obj->state == AIF_OPENED || aif_obj->state == AIF_PREPARED ||
                                                 aif_obj->state == AIF_STOPPED ) {
                agm_trace_begin( "device_start");
                ret = device_start(aif_obj->dev_obj);
                agm_trace_end();
                if (ret) {
                    AGM_LOGE("Error:%d starting device id:%d\n",
                                   ret, aif_obj->aif_id);
                    pthread_mutex_unlock(&hwep_lock);
                    goto unwind;
                }
                aif_obj->state = AIF_STARTED;
            }
        }
        pthread_mutex_unlock(&hwep_lock);
    } else {
        ret = graph_start(sess_obj->graph);
        if (ret) {
            AGM_LOGE("Error:%d starting graph\n", ret);
            goto unwind;
        }
    }

    sess_obj->state = SESSION_STARTED;
    goto done;

unwind:
    pthread_mutex_lock(&hwep_lock);
    graph_stop(sess_obj->graph, NULL);
device_stop:
    if (sess_mode != AGM_SESSION_NON_TUNNEL  && sess_mode != AGM_SESSION_NO_CONFIG) {
        list_for_each(node, &sess_obj->aif_pool) {
            aif_obj = node_to_item(node, struct aif, node);
            if (aif_obj && (aif_obj->state == AIF_STARTED)) {
                device_stop(aif_obj->dev_obj);
                //If start fails, client will retry with a prepare call,
                //so moving to opened state will allow prepare to go through
                aif_obj->state = AIF_OPENED;
            }
        }
    }
    pthread_mutex_unlock(&hwep_lock);
done:
    return ret;
}

static int session_stop(struct session_obj *sess_obj)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    enum direction dir = sess_obj->stream_config.dir;
    enum agm_session_mode sess_mode = sess_obj->stream_config.sess_mode;
    struct listnode *node = NULL;

    if (sess_obj->state != SESSION_STARTED) {
        AGM_LOGE("session not in STARTED state, current state:%d\n",
                                           sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    if (sess_mode != AGM_SESSION_NON_TUNNEL  && sess_mode != AGM_SESSION_NO_CONFIG) {
        pthread_mutex_lock(&hwep_lock);
        if (dir == RX) {
            agm_trace_begin("graph_stop");
            ret = graph_stop(sess_obj->graph, NULL);
            agm_trace_end();
            if (ret) {
                AGM_LOGE("Error:%d stopping graph\n", ret);
                pthread_mutex_unlock(&hwep_lock);
                goto done;
            }
        }

        list_for_each(node, &sess_obj->aif_pool) {
            aif_obj = node_to_item(node, struct aif, node);
            if (!aif_obj) {
                AGM_LOGE("Error:%d could not find aif node\n", ret);
                continue;
            }

            if (aif_obj->state == AIF_STARTED) {
                agm_trace_begin("device_stop");
                ret = device_stop(aif_obj->dev_obj);
                agm_trace_end();
                if (ret) {
                    AGM_LOGE("Error:%d stopping device id:%d\n",
                                   ret, aif_obj->aif_id);
                }
                aif_obj->state = AIF_STOPPED;
            }
        }

        if (dir == TX) {
            agm_trace_begin("graph_stop");
            ret = graph_stop(sess_obj->graph, NULL);
            if (ret) {
                AGM_LOGE("Error:%d stopping graph\n", ret);
            }
            agm_trace_end();
        }
        pthread_mutex_unlock(&hwep_lock);
    } else {
            agm_trace_begin("graph_stop");
            ret = graph_stop(sess_obj->graph, NULL);
            agm_trace_end();
            if (ret) {
                AGM_LOGE("Error:%d stopping graph\n", ret);
            }
    }
    sess_obj->state = SESSION_STOPPED;

done:
    return ret;
}

static int session_close(struct session_obj *sess_obj)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    enum agm_session_mode sess_mode = sess_obj->stream_config.sess_mode;
    struct listnode *node = NULL;
    struct listnode *next = NULL;

    AGM_LOGD("enter");
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("session already in CLOSED state\n");
        ret = -EALREADY;
        goto done;
    }

    pthread_mutex_lock(&hwep_lock);
    if (sess_obj->state == SESSION_STARTED) {
        ret = graph_stop(sess_obj->graph, NULL);
        if (ret) {
           AGM_LOGE("Error:%d closing graph\n", ret);
        }
    }

    ret = graph_close(sess_obj->graph);
    if (ret) {
        AGM_LOGE("Error:%d closing graph\n", ret);
    }
    sess_obj->graph = NULL;
    sess_obj->ec_ref_state = false;
    sess_obj->loopback_state = false;

    if (sess_mode != AGM_SESSION_NON_TUNNEL  && sess_mode != AGM_SESSION_NO_CONFIG) {
        list_for_each_safe(node, next, &sess_obj->aif_pool) {
            aif_obj = node_to_item(node, struct aif, node);
            if (!aif_obj) {
                AGM_LOGE("Error:%d could not find aif node\n", ret);
                continue;
            }

            if (aif_obj->state >= AIF_OPENED) {
                ret = device_close(aif_obj->dev_obj);
                if (ret) {
                    AGM_LOGE("Error:%d stopping device id:%d\n",
                                   ret, aif_obj->aif_id);
                }
// { SEC_AUDIO_ADD_FOR_DEBUG
                else {
                    AGM_LOGI("stopping device id:%d\n",aif_obj->aif_id);
                }
// } SEC_AUDIO_ADD_FOR_DEBUG
                aif_obj->state = AIF_CLOSED;
            }

            if (aif_obj->tag_config) {
                free(aif_obj->tag_config);
                aif_obj->tag_config = NULL;
            }
            list_remove(&aif_obj->node);
            aif_free(aif_obj);
        }
    }
    pthread_mutex_unlock(&hwep_lock);
    sess_obj->state = SESSION_CLOSED;
done:
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}

int session_obj_deinit()
{
    session_pool_free();
    device_deinit();
    graph_deinit();
    return 0;
}

/* Initializes session_obj, enumerate and fill session related information */
int session_obj_init()
{
    int ret = 0;

    ret = device_init();
    if (ret) {
        AGM_LOGE("Error:%d initializing device\n", ret);
        goto done;
    }

    ret = graph_init();
    if (ret) {
        AGM_LOGE("Error:%d initializing graph\n", ret);
        goto device_deinit;
    }

    ret = session_pool_init();
    if (ret) {
        AGM_LOGE("Error:%d initializing session_pool\n", ret);
        goto graph_deinit;
    }
    pthread_mutex_init(&hwep_lock, (const pthread_mutexattr_t *) NULL);
    check_and_enable_traces();
    goto done;

graph_deinit:
    graph_deinit();

device_deinit:
    device_deinit();

done:
    return ret;
}

int session_obj_set_sess_metadata(struct session_obj *sess_obj,
                              uint32_t size, uint8_t *metadata)
{

    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    metadata_free(&(sess_obj->sess_meta));
    ret = metadata_copy(&(sess_obj->sess_meta), size, metadata);
    pthread_mutex_unlock(&sess_obj->lock);

    return ret;
}

int session_obj_set_sess_params(struct session_obj *sess_obj,
    void *payload, size_t size)
{
   int ret = 0;

   pthread_mutex_lock(&sess_obj->lock);

   if (sess_obj->params) {
       free(sess_obj->params);
       sess_obj->params = NULL;
       sess_obj->params_size = 0;
   }

   if ((size == 0) ||(payload == NULL))
       goto done;

   sess_obj->params = calloc(1, size);
   if (!sess_obj->params) {
       AGM_LOGE("No memory for sess params on sess_id:%d\n",
                                   sess_obj->sess_id);
       ret = -EINVAL;
       goto done;
   }

   memcpy(sess_obj->params, payload, size);
   sess_obj->params_size = size;

   if (sess_obj->state != SESSION_CLOSED) {
       ret = graph_set_config(sess_obj->graph, sess_obj->params,
                                         sess_obj->params_size);
       if (ret) {
           AGM_LOGE("Error:%d setting for sess params on sess_id:%d\n",
                   ret, sess_obj->sess_id);
       }
       free(sess_obj->params);
       sess_obj->params = NULL;
       sess_obj->params_size = 0;
   }

done:
   pthread_mutex_unlock(&sess_obj->lock);
   return ret;
}

int session_obj_set_sess_aif_params(struct session_obj *sess_obj,
    uint32_t aif_id,
    void* payload, size_t size)
{
    int ret = 0;
    struct aif *aif_obj = NULL;

    pthread_mutex_lock(&sess_obj->lock);
    ret = aif_obj_get(sess_obj, aif_id, &aif_obj);
    if (ret) {
        AGM_LOGE("Error obtaining aif object with sess_id:%d,  aif id:%d\n",
            sess_obj->sess_id, aif_id);
        goto done;
    }

   if (aif_obj->params) {
       free(aif_obj->params);
       aif_obj->params = NULL;
       aif_obj->params_size = 0;
   }

   if ((size == 0) || (payload == NULL))
       goto done;

   aif_obj->params = calloc(1, size);
   if (!aif_obj->params) {
       AGM_LOGE("No memory for sess_aif params on sess_id:%d, aif_id:%d\n",
                                 sess_obj->sess_id, aif_obj->aif_id);
       ret = -EINVAL;
       goto done;
   }

   memcpy(aif_obj->params, payload, size);
   aif_obj->params_size = size;

   if (sess_obj->state != SESSION_CLOSED && aif_obj->state >= AIF_OPENED) {
       ret = graph_set_config(sess_obj->graph, aif_obj->params, aif_obj->params_size);
       if (ret) {
           AGM_LOGE("Error:%d setting for sess_aif params on sess_id:%d, \
                     aif_id:%d\n", ret,
                     sess_obj->sess_id, aif_obj->aif_id);
       }
       free(aif_obj->params);
       aif_obj->params = NULL;
       aif_obj->params_size = 0;
   }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;

}

int session_obj_set_sess_aif_params_with_tag(struct session_obj *sess_obj,
    uint32_t aif_id,
    struct agm_tag_config *tag_config)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    struct agm_meta_data_gsl *merged_metadata = NULL;
    struct agm_tag_config_gsl tag_config_gsl;
    size_t tkv_payload_size = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (aif_id < UINT_MAX) {
        ret = aif_obj_get(sess_obj, aif_id, &aif_obj);
        if (ret) {
            AGM_LOGE("Error obtaining aif object with sess_id:%d,  aif id:%d\n",
                sess_obj->sess_id, aif_id);
            goto done;
        }

        if (sess_obj->state == SESSION_STARTED && aif_obj->state < AIF_OPENED) {
            AGM_LOGE("AIF not opened on sess_id:%d, aif_id:%d, caching tkv\n",
                     sess_obj->sess_id, aif_obj->aif_id);

            if (aif_obj->tag_config) {
                free(aif_obj->tag_config);
                aif_obj->tag_config = NULL;
            }

            tkv_payload_size = sizeof(struct agm_tag_config) +
                               (tag_config->num_tkvs * sizeof(struct agm_key_value));
            aif_obj->tag_config = (struct agm_tag_config *)calloc(1, tkv_payload_size);
            if (!aif_obj->tag_config) {
                AGM_LOGE("Tag_config memory allocation failed for sess_id:%d, aif_id:%d",
                          sess_obj->sess_id, aif_obj->aif_id);
                ret = -ENOMEM;
                goto done;
            }
            memcpy(aif_obj->tag_config, tag_config, tkv_payload_size);
            goto done;
        }

        if (sess_obj->state == SESSION_CLOSED && aif_obj->state < AIF_OPENED) {
            AGM_LOGE("Invalid state on sess_id:%d, aif_id:%d\n",
                     sess_obj->sess_id, aif_obj->aif_id);
            ret = -EINVAL;
            goto done;
        }

        pthread_mutex_lock(&aif_obj->dev_obj->lock);
        merged_metadata = metadata_merge(3, &sess_obj->sess_meta,
                          &aif_obj->sess_aif_meta, &aif_obj->dev_obj->metadata);
        pthread_mutex_unlock(&aif_obj->dev_obj->lock);
        if (!merged_metadata) {
            AGM_LOGE("Error merging metadata session_id:%d aif_id:%d\n",
                sess_obj->sess_id, aif_obj->aif_id);
            ret = -ENOMEM;
            goto done;
        }

        tag_config_gsl.tag_id = tag_config->tag;
        tag_config_gsl.tkv.num_kvs = tag_config->num_tkvs;
        tag_config_gsl.tkv.kv = tag_config->kv;

        ret = graph_set_config_with_tag(sess_obj->graph, &merged_metadata->gkv,
                                                    &tag_config_gsl);
        if (ret) {
            AGM_LOGE("Error:%d setting for sess_aif params with tags \
                      on sess_id:%d, aif_id:%d\n",
                      ret, sess_obj->sess_id, aif_obj->aif_id);
        }
    } else {
        if (sess_obj->state == SESSION_CLOSED) {
            AGM_LOGE("Invalid state on sess_id:%d\n", sess_obj->sess_id);
            ret = -EINVAL;
            goto done;
        }
        tag_config_gsl.tag_id = tag_config->tag;
        tag_config_gsl.tkv.num_kvs = tag_config->num_tkvs;
        tag_config_gsl.tkv.kv = tag_config->kv;

        ret = graph_set_config_with_tag(sess_obj->graph, &sess_obj->sess_meta.gkv,
                                                    &tag_config_gsl);
        if (ret) {
            AGM_LOGE("Error:%d setting for sess params with tags \
                      on sess_id:%d\n",
                      ret, sess_obj->sess_id);
        }
    }

done:
    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }

    pthread_mutex_unlock(&sess_obj->lock);

    return ret;
}

int session_obj_rw_acdb_params_with_tag(
    struct session_obj *sess_obj, uint32_t aif_id,
    struct agm_acdb_param *acdb_param, bool is_set)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    struct agm_meta_data_gsl *merged_metadata = NULL;
    struct agm_key_vector_gsl tckv;
    uint8_t *ptr = NULL;
    uint8_t enable_flag = 1;
    uint32_t actual_size = 0;

    pthread_mutex_lock(&sess_obj->lock);
    ret = graph_enable_acdb_persistence(enable_flag);
    if (ret) {
        AGM_LOGE("Error: graph_enable_acdb_persistence failed. ret = %d\n", ret);
        goto error;
    }

    ret = aif_obj_get(sess_obj, aif_id, &aif_obj);
    if (ret) {
        AGM_LOGE("Error obtaining aif object with sess_id:%d,  aif id:%d\n",
            sess_obj->sess_id, aif_id);
        goto error;
    }

    pthread_mutex_lock(&aif_obj->dev_obj->lock);
    merged_metadata = metadata_merge(3, &sess_obj->sess_meta,
                        &aif_obj->sess_aif_meta, &aif_obj->dev_obj->metadata);
    pthread_mutex_unlock(&aif_obj->dev_obj->lock);

    if (!merged_metadata) {
        AGM_LOGE("Error merging metadata session_id:%d aif_id:%d\n",
            sess_obj->sess_id, aif_obj->aif_id);
        ret = -ENOMEM;
        goto error;
    }

    tckv.num_kvs= acdb_param->num_kvs;
    tckv.kv = (struct agm_key_value *)calloc(1,
                    tckv.num_kvs * sizeof(struct agm_key_value));
    if (!tckv.kv) {
        ret = -ENOMEM;
        goto free_metadata;
    }

    memcpy((uint8_t *)tckv.kv, acdb_param->blob,
                tckv.num_kvs*sizeof(struct agm_key_value));
    ptr = acdb_param->blob + tckv.num_kvs * sizeof(struct agm_key_value);
    AGM_LOGV("blob_size = %d", acdb_param->blob_size);
    actual_size = acdb_param->blob_size -
                        acdb_param->num_kvs * sizeof(struct agm_key_value);
    for (int f = 0; f < actual_size; f++) {
        AGM_LOGV("%d blob data is 0x%x", f, ptr[f]);
    }
    if (acdb_param->isTKV) {
        AGM_LOGI("%s: TKV param to ACDB.\n", __func__);
        ret = graph_set_tag_data_to_acdb(&merged_metadata->gkv,
                                acdb_param->tag, &tckv,
                                ptr, actual_size);
    } else {
        AGM_LOGI("%s: CKV param to ACDB.\n", __func__);
        ret = graph_set_cal_data_to_acdb(&merged_metadata->gkv,
                                    &tckv, ptr, actual_size);
    }
    free(tckv.kv);

free_metadata:
    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }
error:
    pthread_mutex_unlock(&sess_obj->lock);

    return ret;
}

int session_dummy_rw_acdb_tunnel(
                             void *payload, bool is_param_set)
{
    int ret = 0;
    uint8_t enable_flag = 1;

    AGM_LOGD("enter");
    ret = graph_enable_acdb_persistence(enable_flag);
    if (ret) {
        AGM_LOGE("Error: graph_enable_acdb_persistence failed. ret = %d\n", ret);
        return ret;
    }

    ret = graph_rw_acdb_param(payload, is_param_set);

    AGM_LOGD("exit status=%d", ret);

    return ret;
}

int session_obj_set_sess_aif_cal(struct session_obj *sess_obj,
    uint32_t aif_id,
    struct agm_cal_config *cal_config)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    struct agm_meta_data_gsl *merged_metadata = NULL;
    struct agm_meta_data_gsl *capture_metadata = NULL;
    struct agm_key_vector_gsl ckv;
    struct device_obj *ec_ref_dev_obj = NULL;

    pthread_mutex_lock(&sess_obj->lock);
    if (aif_id < UINT_MAX) {
        ret = aif_obj_get(sess_obj, aif_id, &aif_obj);
        if (ret) {
            AGM_LOGE("Error obtaining aif object with sess_id:%d,  aif id:%d\n",
                sess_obj->sess_id, aif_id);
            goto done;
        }

        if (sess_obj->state == SESSION_CLOSED || aif_obj->state < AIF_OPENED) {
            AGM_LOGE("Invalid state on sess_id:%d, aif_id:%d\n",
                     sess_obj->sess_id, aif_obj->aif_id);
            ret = -EINVAL;
            goto done;
        }

        ckv.kv = cal_config->kv;
        ckv.num_kvs = cal_config->num_ckvs;
        metadata_update_cal(&sess_obj->sess_meta, &ckv);
        metadata_update_cal(&aif_obj->sess_aif_meta, &ckv);
        pthread_mutex_lock(&aif_obj->dev_obj->lock);
        metadata_update_cal(&aif_obj->dev_obj->metadata, &ckv);

        merged_metadata = metadata_merge(3, &sess_obj->sess_meta,
                          &aif_obj->sess_aif_meta, &aif_obj->dev_obj->metadata);
        pthread_mutex_unlock(&aif_obj->dev_obj->lock);
        if (!merged_metadata) {
            AGM_LOGE("Error merging metadata session_id:%d aif_id:%d\n",
                sess_obj->sess_id, aif_obj->aif_id);
            ret = -ENOMEM;
            goto done;
        }

        ret = graph_set_cal(sess_obj->graph, merged_metadata);
        if (ret) {
            AGM_LOGE("Error:%d setting calibration on sess_id:%d, aif_id:%d\n",
                    ret, sess_obj->sess_id, aif_obj->aif_id);
        }
        // update CKV values for EC path as well
        if (sess_obj->ec_ref_state == true) {
            struct device_obj *ec_ref_dev_obj = NULL;
            ret = device_get_obj(sess_obj->ec_ref_aif_id, &ec_ref_dev_obj);
            if (ret) {
                AGM_LOGE("Error:%d getting device object with aif id:%d\n",
                        ret, sess_obj->ec_ref_aif_id);
                goto done;
            }
            capture_metadata = session_get_merged_metadata_without_aif(sess_obj);
            if (!capture_metadata) {
                ret = -ENOMEM;
                AGM_LOGE("Error:%d, merging metadata with session id=%d\n",
                                             ret, sess_obj->sess_id);
                goto done;
            }
            metadata_free(merged_metadata);
            free(merged_metadata);
            pthread_mutex_lock(&ec_ref_dev_obj->lock);
            merged_metadata = metadata_merge(2, capture_metadata, &ec_ref_dev_obj->metadata);
            pthread_mutex_unlock(&ec_ref_dev_obj->lock);
            if (!merged_metadata) {
                ret = -ENOMEM;
                AGM_LOGE("Error:%d, merging metadata with capture \
                           session id=%d ec aif_id:%d \n",
                           ret, sess_obj->sess_id, sess_obj->ec_ref_aif_id);
                goto done;
            }
            ret = graph_set_cal(sess_obj->graph, merged_metadata);
            if (ret) {
                AGM_LOGE("Error:%d setting EC calibration on sess_id:%d, aif_id:%d\n",
                        ret, sess_obj->sess_id, sess_obj->ec_ref_aif_id);
            }
        }
    } else {
        if (sess_obj->state == SESSION_CLOSED) {
            AGM_LOGE("Invalid state on sess_id:%d\n", sess_obj->sess_id);
            ret = -EINVAL;
            goto done;
        }

        ckv.kv = cal_config->kv;
        ckv.num_kvs = cal_config->num_ckvs;
        metadata_update_cal(&sess_obj->sess_meta, &ckv);

        ret = graph_set_cal(sess_obj->graph, &sess_obj->sess_meta);
        if (ret) {
            AGM_LOGE("Error:%d setting calibration on sess_id:%d\n",
                    ret, sess_obj->sess_id);
        }
    }

done:
    if (capture_metadata) {
        metadata_free(capture_metadata);
        free(capture_metadata);
    }
    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }

    pthread_mutex_unlock(&sess_obj->lock);

    return ret;
}


int session_obj_set_sess_aif_metadata(struct session_obj *sess_obj,
    uint32_t aif_id,  uint32_t size, uint8_t *metadata)
{

    int ret = 0;
    struct aif *aif_obj = NULL;

    pthread_mutex_lock(&sess_obj->lock);
    ret = aif_obj_get(sess_obj, aif_id, &aif_obj);
    if (ret) {
        AGM_LOGE("Error obtaining aif object with sess_id:%d,  aif id:%d\n",
            sess_obj->sess_id, aif_id);
        goto done;
    }

    metadata_free(&(aif_obj->sess_aif_meta));
    ret = metadata_copy(&(aif_obj->sess_aif_meta), size, metadata);
    if (ret) {
        AGM_LOGE("Error copying session audio interface metadata \
                  sess_id:%d, aif_id:%d \n",
                  sess_obj->sess_id, aif_obj->aif_id);
    }
#ifdef AGM_DEBUG_METADATA
    AGM_LOGI("Setting metadata for sess_id %d, aif id %d\n", sess_obj->sess_id, aif_id);
    metadata_print(&(aif_obj->sess_aif_meta));
#endif
done:
    pthread_mutex_unlock(&sess_obj->lock);
    AGM_LOGI("Exit");
    return ret;
}

int session_obj_get_sess_params(struct session_obj *sess_obj,
        void *payload, size_t size)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);

    if (sess_obj->state != SESSION_CLOSED) {
        ret = graph_get_config(sess_obj->graph, payload, size);
            if (ret)
                AGM_LOGE("Error:%d get sess params on sess_id:%d\n",
                              ret, sess_obj->sess_id);
    }


    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_get_tag_with_module_info(struct session_obj *sess_obj,
                                         uint32_t aif_id, void *payload,
                                         size_t *size)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    struct agm_meta_data_gsl *merged_metadata = NULL;
    enum agm_session_mode sess_mode = sess_obj->stream_config.sess_mode;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_mode != AGM_SESSION_NON_TUNNEL) {
        if (aif_id < UINT_MAX) {
            ret = aif_obj_get(sess_obj, aif_id, &aif_obj);
            if (ret) {
                AGM_LOGE("Error obtaining aif object with sess_id:%d,  aif id:%d\n",
                        sess_obj->sess_id, aif_id);
                goto done;
            }

            pthread_mutex_lock(&aif_obj->dev_obj->lock);
            merged_metadata = metadata_merge(3, &sess_obj->sess_meta,
                                &aif_obj->sess_aif_meta, &aif_obj->dev_obj->metadata);
            pthread_mutex_unlock(&aif_obj->dev_obj->lock);
            if (!merged_metadata) {
                AGM_LOGE("Error merging metadata session_id:%d aif_id:%d\n",
                    sess_obj->sess_id, aif_obj->aif_id);
                ret = -ENOMEM;
                goto done;
            }
        } else {
            ret = -EINVAL;
            goto done;
        }
    } else {
        merged_metadata = metadata_merge(1, &sess_obj->sess_meta);
        if (!merged_metadata) {
            AGM_LOGE("Error merging metadata session_id:%d aif_id:%d\n",
                     sess_obj->sess_id, aif_id);
            ret = -ENOMEM;
            goto done;
        }
    }

    ret = graph_get_tags_with_module_info(&merged_metadata->gkv, payload, size);
    if (ret) {
        AGM_LOGE("Error getting tag with module info from graph for \
                 session_id:%d aif_id:%d\n",
                 sess_obj->sess_id, aif_id);
        goto done;
    }

done:
    if (merged_metadata) {
        metadata_free(merged_metadata);
        free(merged_metadata);
    }

    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_dummy_get_tag_with_module_info(struct agm_key_vector_gsl *gkv,
                                         void *payload, size_t *size)
{
    int ret = 0;

    ret = graph_get_tags_with_module_info(gkv, payload, size);
    if (ret) {
        AGM_LOGE("Error getting tag with module info from graph");
    }

    return ret;
}

int session_obj_register_cb(struct session_obj *sess_obj, agm_event_cb cb,
                              enum event_type evt_type, void *client_data)
{
    int ret = 0;
    struct session_cb *sess_cb = NULL;

    pthread_mutex_lock(&sess_obj->cb_pool_lock);
    if (cb != NULL) {
        sess_cb = calloc(1, sizeof(struct session_cb));
        if (!sess_cb) {
            AGM_LOGE("Error creating session_cb object with sess_id:%d\n",
                                             sess_obj->sess_id);
            ret = -ENOMEM;
            goto done;
        }

        sess_cb->cb = cb;
        sess_cb->client_data = client_data;
        sess_cb->evt_type = evt_type;
        AGM_LOGV("sess_cb %p client_data %p evt_type %d", sess_cb,
                                           client_data, evt_type);
        list_add_tail(&sess_obj->cb_pool, &sess_cb->node);
    } else {
        struct listnode *node, *next;
        list_for_each_safe(node, next, &sess_obj->cb_pool) {
            sess_cb = node_to_item(node, struct session_cb, node);
            if (sess_cb->evt_type == evt_type &&
                sess_cb->client_data == client_data) {
                AGM_LOGV("remove sess_cb %p client_data %p evt_type %d",
                        sess_cb, sess_cb->client_data,
                        sess_cb->evt_type);
                list_remove(&sess_cb->node);
                free(sess_cb);
            }
        }
    }
done:
    pthread_mutex_unlock(&sess_obj->cb_pool_lock);
    return ret;
}

int session_obj_register_for_events(struct session_obj *sess_obj,
                           struct agm_event_reg_cfg *evt_reg_cfg)
{

    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);

    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Error registering for events, Session with sess_id:%d \
                  in invalid state:%d\n",
                   sess_obj->sess_id, sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_register_for_events(sess_obj->graph, evt_reg_cfg);
    if (ret) {
        AGM_LOGE("Error:%d registering events with graph for sess_id:%d\n",
                ret, sess_obj->sess_id);
        goto done;
    }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_sess_aif_connect(struct session_obj *sess_obj,
    uint32_t aif_id, bool aif_state)
{
    int ret = 0;
    struct aif *aif_obj = NULL;
    uint32_t opened_count = 0;

    pthread_mutex_lock(&sess_obj->lock);
    ret = aif_obj_get(sess_obj, aif_id, &aif_obj);
    if (ret) {
        AGM_LOGE("Error obtaining aif object with sess_id:%d,  aif id:%d\n",
             sess_obj->sess_id, aif_id);
        goto done;
    }

    if (((aif_state == true) && (aif_obj->state > AIF_OPENED)) ||
        ((aif_state == false) && (aif_obj->state < AIF_OPEN))) {
        AGM_LOGE("AIF already in state %d\n", aif_obj->state);
        ret = -EALREADY;
        goto done;
    }

    opened_count = aif_obj_get_count_with_state(sess_obj, AIF_OPENED, false);

    if (aif_state == true) {
        //TODO: check if the assumption is correct
        //Assumption: Each of the following state assumes that there was
        //an Audio Interface in Connect state.
        switch (sess_obj->state) {
        case SESSION_OPENED:
            ret = session_connect_aif(sess_obj, aif_obj, opened_count);
            if (ret) {
                AGM_LOGE("Error:%d, Unable to Connect device\n",
                                                    ret);
                goto done;
            }
            aif_obj->state = AIF_OPENED;
            opened_count++;
            break;
        case SESSION_PREPARED:
        case SESSION_STOPPED:
            ret = session_connect_aif(sess_obj, aif_obj, opened_count);
            if (ret) {
                AGM_LOGE("Error:%d, Unable to Connect device\n",
                                                    ret);
                goto done;
            }
            aif_obj->state = AIF_OPENED;
            opened_count++;

            ret = session_prepare(sess_obj);
            if (ret) {
                AGM_LOGE("Error:%d, Unable to prepare device\n",
                                                    ret);
                goto unwind;
            }

            break;
        case SESSION_STARTED:
            ret = session_connect_aif(sess_obj, aif_obj, opened_count);
            if (ret) {
                AGM_LOGE("Error:%d, Unable to Connect device\n",
                                                    ret);
                goto done;
            }
            aif_obj->state = AIF_OPENED;
            opened_count++;

            ret = session_prepare(sess_obj);
            if (ret) {
                AGM_LOGE("Error:%d, Unable to prepare device\n",
                                                    ret);
                goto unwind;
            }
            ret = session_start(sess_obj);
            if (ret) {
                AGM_LOGE("Error:%d, Unable to start device\n",
                                                   ret);
                goto unwind;
            }
            break;

        case SESSION_CLOSED:
            aif_obj->state = AIF_OPEN;
            break;

        }

    } else {
        aif_obj->state = AIF_CLOSE;

       //if session is in started state and more than 1 device is connect,
       //call remove, if only 1 device is connected, do graph stop
        switch (sess_obj->state) {
        case SESSION_OPENED:
        case SESSION_PREPARED:
        case SESSION_STARTED:
        case SESSION_STOPPED:
            ret = session_disconnect_aif(sess_obj, aif_obj, opened_count);
            if (ret) {
                AGM_LOGE("Error:%d, Unable to Connect device\n",
                                                    ret);
                goto done;
            }
        default:
            break;
        }
        aif_obj->state = AIF_CLOSED;
    }
    goto done;

unwind:
    aif_obj->state = AIF_CLOSE;
    session_disconnect_aif(sess_obj, aif_obj, opened_count);
    opened_count--;

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_open(uint32_t session_id,
                     enum agm_session_mode sess_mode,
                     struct session_obj **session)
{

    struct session_obj *sess_obj = NULL;
    int ret = 0;
    int ret_unwind = 0;
    struct listnode *node;
    struct aif *aif_obj = NULL;

    ret = session_obj_get(session_id, &sess_obj);
    if (ret) {
        AGM_LOGE("Error getting session object\n");
        return ret;
    }

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state != SESSION_CLOSED) {
        AGM_LOGE("Session already Opened, session_state:%d\n",
                                       sess_obj->state);
        ret = -EALREADY;
        goto done;
    }
    sess_obj->stream_config.sess_mode = sess_mode;

    if (sess_mode == AGM_SESSION_NON_TUNNEL || sess_mode == AGM_SESSION_NO_CONFIG) {
        /**
         *AGM session can be opened in any one of the agm_session_modes
         *If it is a AGM_SESSUION_NON_TUNNEL mode, then that indicates
         *there is no device leg associated with this session, and hence
         *we skip setting up the same.
         */
        ret = session_open_without_device(sess_obj);
        if (ret) {
            AGM_LOGE("Unable to open a session with Session ID:%d\n",
                                            sess_obj->sess_id);
            goto done;
        }
    } else {
        /**
         *1. get first device obj from the list for the session
         *2. concatenate stream+dev metadata
         *3. for playback, open alsa first, open graph
         *4. get rest of the devices, call add graph
         *5. update state as opened
         **/
        ret = session_open_with_first_device(sess_obj);
        if (ret) {
            AGM_LOGE("Unable to open a session with Session ID:%d\n",
                                            sess_obj->sess_id);
            goto done;
        }

        ret = session_connect_reminder_devices(sess_obj);
        if (ret) {
            AGM_LOGE("Unable to open a session with Session ID:%d\n",
                                            sess_obj->sess_id);
            goto done;
        }


        //configure ecref if valid session id has been set
        if (sess_obj->ec_ref_state == true) {
            ret = session_set_ec_ref(sess_obj, sess_obj->ec_ref_aif_id,
                                               sess_obj->ec_ref_state);
            if (ret) {
                sess_obj->ec_ref_state = false;
                sess_obj->ec_ref_aif_id = 0;
                goto unwind;
            }
        }
    }
    //configure loopback if loopback state is true
    if (sess_obj->loopback_state == true) {
        ret = session_set_loopback(sess_obj, sess_obj->loopback_sess_id,
                                              sess_obj->loopback_state);
        if (ret) {
            sess_obj->loopback_state = false;
            sess_obj->loopback_sess_id = 0;
            goto unwind;
        }
    }

    sess_obj->state = SESSION_OPENED;
    *session = sess_obj;
    goto done;

unwind:
    list_for_each(node, &sess_obj->aif_pool) {
        aif_obj = node_to_item(node, struct aif, node);
        if (aif_obj && aif_obj->state == AIF_OPENED) {
            /*TODO: fix the 3rd argument to provide correct count*/
            ret_unwind = session_disconnect_aif(sess_obj, aif_obj, 1);
            if (ret_unwind) {
                AGM_LOGE("Error:%d Failed to disconnect device\n",
                                                     ret_unwind);
            }
            aif_obj->state = AIF_OPEN;
        }
    }
    ret_unwind = graph_close(sess_obj->graph);
    if (ret_unwind) {
        AGM_LOGE("Error:%d Failed to close graph\n", ret_unwind);
    }
    sess_obj->graph = NULL;

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_set_config(struct session_obj *sess_obj,
                 struct agm_session_config *stream_config,
                 struct agm_media_config *media_config,
                 struct agm_buffer_config *buffer_config)
{
    int ret = 0;
    pthread_mutex_lock(&sess_obj->lock);

    sess_obj->stream_config = *stream_config;

    if (sess_obj->stream_config.dir == TX) {
        /*Capture session config*/
        sess_obj->in_media_config = *media_config;
        sess_obj->in_buffer_config = *buffer_config;
    } else {
        /*Playback session config*/
        sess_obj->out_media_config = *media_config;
        sess_obj->out_buffer_config = *buffer_config;

        /* During gapless playback, when clips are switched from
         * in between, pause, flush and resume gets called from client,
         * flush makes session state as SESSION_STOPPED, so codec param
         * needs to be sent to ADSP in this state as well.
         */
        if (sess_obj->state == SESSION_STARTED || sess_obj->state == SESSION_STOPPED) {
            ret = graph_set_media_config_datapath(sess_obj->graph);
            if (ret < 0)
                AGM_LOGE("Failed to set media config on datapath ret %d", ret);
        }
    }

    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_prepare(struct session_obj *sess_obj)
{
    int ret = 0;

    agm_trace_begin("session_prepare");
    pthread_mutex_lock(&sess_obj->lock);
    ret = session_prepare(sess_obj);
    pthread_mutex_unlock(&sess_obj->lock);
    agm_trace_end();
    return ret;
}

int session_obj_start(struct session_obj *sess_obj)
{
    int ret = 0;
    agm_trace_begin( "session_start");
    pthread_mutex_lock(&sess_obj->lock);
    ret = session_start(sess_obj);
    pthread_mutex_unlock(&sess_obj->lock);
    agm_trace_end();

    return ret;
}

int session_obj_stop(struct session_obj *sess_obj)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    ret = session_stop(sess_obj);
    pthread_mutex_unlock(&sess_obj->lock);

    return ret;
}


int session_obj_close(struct session_obj *sess_obj)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    ret = session_close(sess_obj);
    pthread_mutex_unlock(&sess_obj->lock);

    return ret;
}

int session_obj_pause(struct session_obj *sess_obj)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    /* TODO: should pause be issued in specific state,
       for now ensure its in started state */
    if (sess_obj->state != SESSION_STARTED) {
        AGM_LOGE("Cannot issue pause in state:%d\n",
                            sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_pause(sess_obj->graph);
    if (ret) {
        AGM_LOGE("Error:%d pausing graph\n", ret);
    }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_flush(struct session_obj *sess_obj)
{
    int ret = 0;
    struct session_cb *sess_cb;
    struct listnode *node, *next;
    struct agm_event_cb_params *event_params = NULL;

    pthread_mutex_lock(&sess_obj->lock);

    ret = graph_flush(sess_obj->graph);
    if (ret) {
        AGM_LOGE("Error:%d flushing graph\n", ret);
        goto done;
    }

    // Unblock the call waiting for EARLY_EOS callback
    event_params = (struct agm_event_cb_params*) calloc(1,
                   (sizeof(struct agm_event_cb_params)));
    if (!event_params) {
        AGM_LOGE("Not enough memory for event_params");
        goto done;
    }

    pthread_mutex_lock(&sess_obj->cb_pool_lock);
    list_for_each_safe(node, next, &sess_obj->cb_pool) {
        sess_cb = node_to_item(node, struct session_cb, node);
        if (sess_cb && sess_cb->cb) {
            event_params->event_id = AGM_EVENT_EARLY_EOS_INTERNAL;
            sess_cb->cb(sess_obj->sess_id,
                        (struct agm_event_cb_params *)event_params,
                        sess_cb->client_data);
        }
    }
    pthread_mutex_unlock(&sess_obj->cb_pool_lock);
    if (event_params)
        free(event_params);

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}


int session_obj_resume(struct session_obj *sess_obj)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);

    ret = graph_resume(sess_obj->graph);
    if (ret) {
        AGM_LOGE("Error:%d resuming graph\n", ret);
    }


    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_suspend(struct session_obj *sess_obj)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);

    ret = graph_suspend(sess_obj->graph);
    if (ret) {
        AGM_LOGE("Error:%d suspending graph\n", ret);
    }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_read(struct session_obj *sess_obj, void *buff, size_t *count)
{
    int ret = 0;
    struct agm_buff buffer = {0};

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot issue read in state:%d\n",
                           sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    buffer.timestamp = 0x0;
    buffer.flags = 0;
    buffer.size = *count;
    buffer.addr = (uint8_t *)(buff);

    ret = graph_read(sess_obj->graph, &buffer, count);
    if (ret) {
        AGM_LOGE("Error:%d reading from graph\n", ret);
    }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_write(struct session_obj *sess_obj, void *buff, size_t *count)
{
    int ret = 0;
    struct agm_buff buffer = {0};

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot issue write in state:%d\n",
                            sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    buffer.timestamp = 0x0;
    buffer.flags = 0;
    buffer.size = *count;
    buffer.addr = (uint8_t *)(buff);

    ret = graph_write(sess_obj->graph, &buffer, count);
    if (ret) {
        AGM_LOGE("Error:%d writing to graph\n", ret);
    }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

size_t session_obj_hw_processed_buff_cnt(struct session_obj *sess_obj,
                                                   enum direction dir)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot issue resume in state:%d\n",
                             sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = (int)graph_get_hw_processed_buff_cnt(sess_obj->graph, dir);


done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_set_loopback(struct session_obj *sess_obj,
                    uint32_t playback_sess_id, bool state)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (playback_sess_id == sess_obj->loopback_sess_id &&
                            state == sess_obj->loopback_state) {
        AGM_LOGE("loopback already in %s state for session:%d\n",
                ((state != false) ? "enabled":"disabled"),
                sess_obj->sess_id);
        ret = -EALREADY;
        goto done;
    }

    /*
     * loopback enables just the edge b/w capture and playback sessions.
     * There is no need to call prepare/start on added graphs as their states are
     * are updated as each of these session states are updated.
     */

    switch(sess_obj->state) {
    case SESSION_OPENED:
    case SESSION_PREPARED:
    case SESSION_STARTED:
    case SESSION_STOPPED:
        if (state == true)
            ret = session_set_loopback(sess_obj, playback_sess_id, state);
        else
            ret = session_set_loopback(sess_obj, sess_obj->loopback_sess_id,
                                                                     state);

        if (ret) {
            AGM_LOGE("Error:%d setting loopback state:%s for session:%d\n",
                      ret, ((state != false) ? "enable":"disable"),
                      sess_obj->sess_id);
            goto done;
        }

        break;

    case SESSION_CLOSED:
        break;
    }
    sess_obj->loopback_sess_id = playback_sess_id;
    sess_obj->loopback_state = state;

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_set_ec_ref(struct session_obj *sess_obj, uint32_t aif_id,
                                        bool state)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (aif_id == sess_obj->ec_ref_aif_id && state == sess_obj->ec_ref_state) {
        AGM_LOGE("ec_ref already in %s state for session:%d\n",
                   ((state != false) ? "enabled":"disabled"),
                  sess_obj->sess_id);
        ret = -EALREADY;
        goto done;
    }

    /*
     * ec_ref enables just the edge b/w capture and playback aif id.
     * There is no need to call prepare/start on added graphs as their states
     * are updated as each of these session states are updated.
     */

    switch (sess_obj->state) {
    case SESSION_OPENED:
    case SESSION_PREPARED:
    case SESSION_STARTED:
    case SESSION_STOPPED:
        if (state == true)
            ret = session_set_ec_ref(sess_obj, aif_id, state);
        else
            ret = session_set_ec_ref(sess_obj, sess_obj->ec_ref_aif_id, state);

        if (ret) {
            AGM_LOGE("Error:%d setting ec_ref state:%s for session:%d\n",
                      ret, ((state != false) ? "enable":"disable"),
                     sess_obj->sess_id);
            goto done;
        }

        break;

    case SESSION_CLOSED:
        break;
    }
    sess_obj->ec_ref_aif_id = aif_id;
    sess_obj->ec_ref_state = state;

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_eos(struct session_obj *sess_obj)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot issue EOS in state:%d\n",
                          sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_eos(sess_obj->graph);
    if (ret) {
        AGM_LOGE("Error:%d sending EOS cmd \n", ret);
    }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}


int session_obj_get_timestamp(struct session_obj *sess_obj,
                                       uint64_t *timestamp)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot get timestamp in state:%d\n",
                              sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_get_session_time(sess_obj->graph, timestamp);
    if (ret)
        AGM_LOGE("Error:%d for get_timestamp \n", ret);

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_buffer_timestamp(struct session_obj *sess_obj, uint64_t *timestamp)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state != SESSION_STARTED) {
        AGM_LOGE("Cannot get timestamp in state:%d\n", sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_get_buffer_timestamp(sess_obj->graph, timestamp);
    if (ret)
        AGM_LOGE("Error:%d cannot get buffer timestamp\n",
                  ret);

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_get_sess_buf_info(struct session_obj *sess_obj, struct agm_buf_info *buf_info, uint32_t flag)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot get timestamp in state:%d\n", sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_get_buf_info(sess_obj->graph, buf_info, flag);
    if (ret)
        AGM_LOGE("graph_get_buf_info failed %d\n", ret);

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_set_gapless_metadata(struct session_obj *sess_obj,
                                     enum agm_gapless_silence_type type,
                                     uint32_t silence)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot set gapless data in state:%d\n", sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_set_gapless_metadata(sess_obj->graph, type,
                                     silence);
    if (ret)
        AGM_LOGE("failed to set gapless metadata :%d\n", ret);
done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_write_with_metadata(struct session_obj *sess_obj,
                                    struct agm_buff *buffer,
                                    size_t *consumed_size)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot issue write in state:%d\n",
                            sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    ret = graph_write(sess_obj->graph, buffer, consumed_size);
    if (ret) {
        AGM_LOGE("Error:%d writing to graph\n", ret);
    }

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_read_with_metadata(struct session_obj *sess_obj,
                                   struct agm_buff *buffer,
                                   uint32_t *captured_size)
{
    int ret = 0;
    pthread_mutex_lock(&sess_obj->lock);
    if (sess_obj->state == SESSION_CLOSED) {
        AGM_LOGE("Cannot issue read in state:%d\n",
                           sess_obj->state);
        ret = -EINVAL;
        goto done;
    }

    size_t read_size;
    ret = graph_read(sess_obj->graph, buffer, &read_size);
    if (ret) {
        AGM_LOGE("Error:%d reading from graph\n", ret);
    }

    *captured_size = (uint32_t)read_size;

done:
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}

int session_obj_set_non_tunnel_mode_config(struct session_obj *sess_obj,
                                    struct agm_session_config *session_config,
                                    struct agm_media_config *in_media_config,
                                    struct agm_media_config *out_media_config,
                                    struct agm_buffer_config *in_buffer_config,
                                    struct agm_buffer_config *out_buffer_config)
{
    int ret = 0;

    pthread_mutex_lock(&sess_obj->lock);
    sess_obj->stream_config = *session_config;
    sess_obj->in_media_config = *in_media_config;
    sess_obj->out_media_config = *out_media_config;
    sess_obj->in_buffer_config = *in_buffer_config;
    sess_obj->out_buffer_config = *out_buffer_config;
    pthread_mutex_unlock(&sess_obj->lock);
    return ret;
}
