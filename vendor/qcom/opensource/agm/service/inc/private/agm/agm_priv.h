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
 */

#ifndef _AGM_PRIV_H_
#define _AGM_PRIV_H_

#include <agm/agm_api.h>

/**
 * Key Vector
 */
struct agm_key_vector_gsl {
    size_t num_kvs;                 /**< number of key value pairs */
    struct agm_key_value *kv;       /**< array of key value pairs */
};
struct sg_prop {
    uint32_t prop_id;
    uint32_t num_values;
    uint32_t *values;
};

/**
  * Metadata Key Vectors
  */
struct agm_meta_data_gsl {
    /**
     * Used to lookup the calibration data
     */
    struct agm_key_vector_gsl gkv;

    /**
    * Used to lookup the calibration data
    */
     struct agm_key_vector_gsl ckv;
     /**
     * Used to lookup the property ids
     */
     struct sg_prop sg_props;
};

struct agm_tag_config_gsl {
    uint32_t tag_id;       /**< tag id */
    struct agm_key_vector_gsl tkv;
};


#endif /* _AGM_PRIV_H_ */
