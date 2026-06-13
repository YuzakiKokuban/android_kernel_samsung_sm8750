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
#define LOG_TAG "AGM: metadata"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <agm/metadata.h>
#include <agm/utils.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_METADATA
#include <log_utils.h>
#endif

/*
 * Payload for metadata is expected in the following form
 *
 * struct {
 *     uint32_t num_gkv;
 *     struct agm_key_value gkv[num_gkv];
 *     uint32_t num_ckv;
 *     struct agm_key_value ckv[num_ckv];
 *     uint32_t prop_id;
 *     uint32_t num_props;
 *     uint32_t props[num_props];
 *
 * }
 *
 * */

#define PTR_TO_NUM_GKV(x)               (x)
#define NUM_GKV(x)                     *((uint32_t *) x)
#define PTR_TO_GKV(x)                   (x + sizeof(uint32_t))

#define PTR_TO_NUM_CKV(x)               (x + sizeof(uint32_t) + (NUM_GKV(x) * (sizeof(struct agm_key_value))))
#define NUM_CKV(x)                      *((uint32_t *) PTR_TO_NUM_CKV(x))
#define PTR_TO_CKV(x)                   (PTR_TO_NUM_CKV(x) + sizeof(uint32_t))

#define PTR_TO_PROP_ID(x)               (PTR_TO_CKV(x) + (NUM_CKV(x) * (sizeof(struct agm_key_value))))
#define PROP_ID(x)                     *((uint32_t *) PTR_TO_PROP_ID(x))

#define PTR_TO_NUM_PROPS(x)             (PTR_TO_PROP_ID(x) + sizeof(uint32_t))
#define NUM_PROPS(x)                    *((uint32_t *) PTR_TO_NUM_PROPS(x))
#define PTR_TO_PROPS(x)                 (PTR_TO_NUM_PROPS(x) + sizeof(uint32_t))

#define MAX_KVPAIR_PROPS 48

void metadata_print(struct agm_meta_data_gsl* metadata)
{
    int i, count = metadata->gkv.num_kvs;
    AGM_LOGD("*************************Metadata*************************\n");
    AGM_LOGD("GKV size:%d\n", count);
    for (i = 0; i < count; i++) {
        AGM_LOGD("key:0x%x, value:0x%x ", metadata->gkv.kv[i].key,
                                       metadata->gkv.kv[i].value);
    }

    count = metadata->ckv.num_kvs;
    AGM_LOGD("\nCKV count:%d\n", count);
    for (i = 0; i < count; i++) {
        AGM_LOGD("key:0x%x, value:0x%x ", metadata->ckv.kv[i].key,
                                       metadata->ckv.kv[i].value);
    }
    AGM_LOGD("\n");

    AGM_LOGD("Property ID:%d\n", metadata->sg_props.prop_id);
    AGM_LOGD("property count:%d\n", metadata->sg_props.num_values);

    count = metadata->sg_props.num_values;
    AGM_LOGD("Property Values: ");
    for (i = 0; i < count; i++) {
        AGM_LOGD("0x%x ", metadata->sg_props.values[i]);
    }
    AGM_LOGD("\n");
    AGM_LOGD("****************Metadata Done***************************\n\n");

}

static void metadata_remove_dup(
       struct agm_meta_data_gsl* meta_data) {

    int i, j, k, count;

    //remove duplicate gkvs
    count = meta_data->gkv.num_kvs;
    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (meta_data->gkv.kv[i].key == meta_data->gkv.kv[j].key) {
                for (k = j; k < count-1; k++) {
                    meta_data->gkv.kv[k].key = meta_data->gkv.kv[k + 1].key;
                    meta_data->gkv.kv[k].value = meta_data->gkv.kv[k + 1].value;
                }
                count--;
                j--;
            }
        }
    }
    meta_data->gkv.num_kvs = count;

    //remove duplicate ckvs
    count = meta_data->ckv.num_kvs;
    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (meta_data->ckv.kv[i].key == meta_data->ckv.kv[j].key) {
                for (k = j; k < count-1; k++) {
                    meta_data->ckv.kv[k].key = meta_data->ckv.kv[k + 1].key;
                    meta_data->ckv.kv[k].value = meta_data->ckv.kv[k + 1].value;
                }
                count--;
                j--;
            }
        }
    }
    meta_data->ckv.num_kvs = count;

    //remove duplicate props
    count = meta_data->sg_props.num_values;
    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (meta_data->sg_props.values[i] == meta_data->sg_props.values[j]) {
                for (k = j; k < count-1; k++) {
                    meta_data->sg_props.values[k] =
                        meta_data->sg_props.values[k+1];
                }
                count--;
                j--;
            }
        }
    }
    meta_data->sg_props.num_values = count;

    //metadata_print(meta_data);
}

void metadata_update_cal(struct agm_meta_data_gsl *meta_data,
                                     struct agm_key_vector_gsl *ckv)
{
    int i, j;

    if (!meta_data || !ckv) {
        AGM_LOGE("Invalid params\n");
        return;
    }

    if (!(meta_data->ckv.kv)) {
        AGM_LOGE("metadata->ckv.kv is NULL, num_kvs=%d\n",
                                    meta_data->ckv.num_kvs);
        return;
    }
    if (!(ckv->kv)) {
        AGM_LOGE("ckv->kv is NULL, num_kvs=%d\n",
                                    ckv->num_kvs);
        return;
    }

    for (i = 0; i < meta_data->ckv.num_kvs; i++) {
        for (j = 0; j < ckv->num_kvs; j++) {
            if (meta_data->ckv.kv[i].key == ckv->kv[j].key) {
                meta_data->ckv.kv[i].value = ckv->kv[j].value;
            }
        }
    }
}

struct agm_meta_data_gsl* metadata_merge(int num, ...)
{
    struct agm_key_value *gkv_offset;
    struct agm_key_value *ckv_offset;
    uint32_t *prop_offset;
    struct agm_meta_data_gsl *temp, *merged = NULL;

    va_list valist;
    int i = 0;


    merged = calloc(1, sizeof(struct agm_meta_data_gsl));
    if (!merged) {
        AGM_LOGE("No memory to create merged metadata\n");
        return NULL;
    }


    va_start(valist, num);
    for (i = 0; i < num; i++) {
        temp = va_arg(valist, struct agm_meta_data_gsl*);
        if (temp) {
            merged->gkv.num_kvs += temp->gkv.num_kvs;
            merged->ckv.num_kvs += temp->ckv.num_kvs;
            merged->sg_props.num_values += temp->sg_props.num_values;
        }
    }
    va_end(valist);

    if ((merged->gkv.num_kvs > MAX_KVPAIR_PROPS) || (merged->ckv.num_kvs > MAX_KVPAIR_PROPS)
                                             || (merged->sg_props.num_values > MAX_KVPAIR_PROPS)) {
        AGM_LOGE("Num GKVs %d Num CKVs %d Num Props %d more than expected: %d", merged->gkv.num_kvs,
                                merged->ckv.num_kvs, merged->sg_props.num_values, MAX_KVPAIR_PROPS);
        free(merged);
        return NULL;
    }

    merged->gkv.kv = calloc(merged->gkv.num_kvs, sizeof(struct agm_key_value));
    if (!merged->gkv.kv) {
        AGM_LOGE("No memory to merge gkv\n");
        free(merged);
        return NULL;
    }

    merged->ckv.kv = calloc(merged->ckv.num_kvs, sizeof(struct agm_key_value));
    if (!merged->ckv.kv) {
        AGM_LOGE("No memory to merge ckv\n");
        free(merged->gkv.kv);
        free(merged);
        return NULL;
    }

    merged->sg_props.values = calloc(merged->sg_props.num_values,
                                               sizeof(uint32_t));
    if (!merged->sg_props.values) {
        AGM_LOGE("No memory to merge properties\n");
        free(merged->gkv.kv);
        free(merged->ckv.kv);
        free(merged);
        return NULL;
    }

    gkv_offset = merged->gkv.kv;
    ckv_offset = merged->ckv.kv;
    prop_offset = merged->sg_props.values;

    va_start(valist, num);
    for (i = 0; i < num; i++) {
        temp = va_arg(valist, struct agm_meta_data_gsl*);
        if (temp) {
            if (temp->gkv.kv) {
                memcpy(gkv_offset, temp->gkv.kv, temp->gkv.num_kvs *
                                      sizeof(struct agm_key_value));
                gkv_offset += temp->gkv.num_kvs;
            }

            if (temp->ckv.kv) {
                memcpy(ckv_offset, temp->ckv.kv, temp->ckv.num_kvs *
                                      sizeof(struct agm_key_value));
                ckv_offset += temp->ckv.num_kvs;
            }

            if (temp->sg_props.values) {
                merged->sg_props.prop_id = temp->sg_props.prop_id;
                memcpy(prop_offset, temp->sg_props.values,
                       temp->sg_props.num_values * sizeof(uint32_t));
                prop_offset += temp->sg_props.num_values;
            }
        }
    }
    va_end(valist);
    //metadata_print(merged);
    metadata_remove_dup(merged);

    return merged;
}

int metadata_copy(struct agm_meta_data_gsl *dest, uint32_t size,
                                              uint8_t *metadata)
{

    int ret = 0;
    int min_req_len = 0;

    if (!metadata) {
        AGM_LOGI("NULL metadata passed, ignoring\n");
        goto done;
    }
    min_req_len += sizeof(uint32_t);
    if (size < min_req_len) {
        AGM_LOGE("size should be atleast %d size for GKV\n", sizeof(uint32_t));
        ret = -EINVAL;
        goto done;

    }

    dest->gkv.num_kvs = NUM_GKV(metadata);
    if (dest->gkv.num_kvs > MAX_KVPAIR_PROPS) {
        AGM_LOGE("Num GKVs %d more than expected: %d",dest->gkv.num_kvs, MAX_KVPAIR_PROPS);
        ret = -EINVAL;
        goto free_metadata;
    }
    dest->gkv.kv =  calloc(dest->gkv.num_kvs, sizeof(struct agm_key_value));
    if (!dest->gkv.kv) {
        AGM_LOGE("Memory allocation failed to copy GKV\n");
        dest->gkv.num_kvs = 0;
        ret = -ENOMEM;
        goto free_metadata;
    }

    min_req_len += (dest->gkv.num_kvs * sizeof(struct agm_key_value));
    if (size < min_req_len) {
        AGM_LOGE("Invalid GKV passed\n");
        ret = -EINVAL;
        goto free_metadata;
    }
    memcpy(dest->gkv.kv, PTR_TO_GKV(metadata), dest->gkv.num_kvs *
                                    sizeof(struct agm_key_value));

    min_req_len += sizeof(uint32_t);
    if (size < min_req_len) {
        goto done;
    }

    dest->ckv.num_kvs = NUM_CKV(metadata);
    if (dest->ckv.num_kvs > MAX_KVPAIR_PROPS) {
        AGM_LOGE("Num CKVs %d more than expected: %d",dest->ckv.num_kvs, MAX_KVPAIR_PROPS);
        ret = -EINVAL;
        goto free_metadata;
    }
    dest->ckv.kv =  calloc(dest->ckv.num_kvs, sizeof(struct agm_key_value));
    if (!dest->ckv.kv) {
        AGM_LOGE("Memory allocation failed to copy CKV\n");
        ret = -ENOMEM;
        goto free_metadata;
    }
    min_req_len += (dest->ckv.num_kvs * sizeof(struct agm_key_value));
    if (size < min_req_len) {
        AGM_LOGE("Invalid CKV passed\n");
        ret = -EINVAL;
        goto free_metadata;
    }
    memcpy(dest->ckv.kv, PTR_TO_CKV(metadata), dest->ckv.num_kvs *
                                    sizeof(struct agm_key_value));

    min_req_len += sizeof(uint32_t);
    if (size < min_req_len) {
        goto done;
    }
    dest->sg_props.prop_id = PROP_ID(metadata);

    min_req_len += sizeof(uint32_t);
    if (size < min_req_len) {
        AGM_LOGE("Invalid properties passed\n");
        ret = -EINVAL;
        goto free_metadata;
    }
    dest->sg_props.num_values = NUM_PROPS(metadata);
    if (dest->sg_props.num_values > MAX_KVPAIR_PROPS) {
        AGM_LOGE("Num Props %d more than expected: %d",dest->sg_props.num_values, MAX_KVPAIR_PROPS);
        ret = -EINVAL;
        goto free_metadata;
    }
    dest->sg_props.values =  calloc(dest->sg_props.num_values, sizeof(uint32_t));
    if (!dest->sg_props.values) {
        AGM_LOGE("Memory allocation failed to copy properties\n");
        ret = -ENOMEM;
        goto free_metadata;
    }
    min_req_len += (dest->sg_props.num_values * sizeof(uint32_t));
    if (size < min_req_len) {
        AGM_LOGE("Invalid properties passed\n");
        ret = -EINVAL;
        goto free_metadata;
    }
    memcpy(dest->sg_props.values, PTR_TO_PROPS(metadata),
           dest->sg_props.num_values * sizeof(uint32_t));
    goto done;

free_metadata:
    metadata_free(dest);

done:
    return ret;

}

void metadata_free(struct agm_meta_data_gsl *metadata)
{
    if (metadata) {
        if (metadata->ckv.kv)
            free(metadata->ckv.kv);
        metadata->ckv.kv = NULL;

        if (metadata->gkv.kv)
            free(metadata->gkv.kv);
        metadata->gkv.kv = NULL;

        if (metadata->sg_props.values)
            free(metadata->sg_props.values);
        metadata->sg_props.values = NULL;

        memset(metadata, 0, sizeof(struct agm_meta_data_gsl));
    }
}
