/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <errno.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "PalApi.h"
#include "kvh2xml.h"

namespace aidl::qti::effects {

// All existing virtualizer, reverb, bass parameters are of length 1
#define GENERIC_CUSTOM_PARAM_LEN 1

#define EQ_ENABLE_PARAM_LEN 1
#define EQ_CONFIG_PARAM_LEN 3
#define EQ_CONFIG_PER_BAND_PARAM_LEN 5
#define EQ_NUM_BANDS_PARAM_LEN 1
#define EQ_BAND_LEVELS_PARAM_LEN 13
#define EQ_BAND_LEVEL_RANGE_PARAM_LEN 2
#define EQ_BAND_FREQS_PARAM_LEN 13
#define EQ_SINGLE_BAND_FREQ_RANGE_PARAM_LEN 2
#define EQ_SINGLE_BAND_FREQ_PARAM_LEN 1
#define EQ_BAND_INDEX_PARAM_LEN 1
#define EQ_PRESET_ID_PARAM_LEN 1
#define EQ_NUM_PRESETS_PARAM_LEN 1
#define EQ_PRESET_NAME_PARAM_LEN 32

#define EQ_BAND_BOOST 5

#define Q27_UNITY (1 << 27)
#define Q8_UNITY (1 << 8)
#define CUSTOM_OPENSL_PRESET 18

struct VirtualizerParams {
    uint32_t enable = 0;
    uint32_t strength = 0;
    uint32_t type = 0;
    uint32_t gainAdjust = 0;
};

struct ReverbParams {
    uint32_t enable = 0;
    uint32_t mode = 0;
    uint32_t preset = 0;
    uint32_t wetMix = 0;
    int32_t gainAdjust = 0;
    int32_t roomLevel = 0;
    int32_t roomHfLevel = 0;
    uint32_t decayTime = 0;
    uint32_t decayHfRatio = 0;
    int32_t reflectionsLevel = 0;
    uint32_t reflectionsDelay = 0;
    int32_t level = 0;
    uint32_t delay = 0;
    uint32_t diffusion = 0;
    uint32_t density = 0;
    uint32_t bypass = 0;
};

struct BassBoostParams {
    uint32_t mEnabled = 0;
    uint32_t mStrength = 0;
};

#define MAX_EQ_BANDS 12
#define MAX_OSL_EQ_BANDS 5
struct EqualizerConfig {
    int32_t pregain;
    int32_t presetId;
    uint32_t numBands;
};
struct EqualizerBandConfig {
    int32_t bandIndex = 0;
    uint32_t filterType = 0;
    uint32_t frequencyMhz = 0;
    int32_t gainMb = 0;
    uint32_t qFactor = 0;
};

struct EqualizerParams {
    uint32_t enable = 0;
    struct EqualizerConfig config;
    struct EqualizerBandConfig bandConfig[MAX_EQ_BANDS];
};

#define PARAM_ID_MODULE_ENABLE 0x8001026
#define PARAM_ID_EQ_CONFIG 0x800110c
#define PARAM_ID_PBE_PARAMS_CONFIG 0x8001150
#define PARAM_ID_BASS_BOOST_MODE 0x800112c
#define PARAM_ID_BASS_BOOST_STRENGTH 0x800112D
#define PARAM_ID_REVERB_MODE 0x80010fd
#define PARAM_ID_REVERB_PRESET 0x80010fe
#define PARAM_ID_REVERB_WET_MIX 0x80010ff
#define PARAM_ID_REVERB_GAIN_ADJUST 0x8001100
#define PARAM_ID_REVERB_ROOM_LEVEL 0x8001101
#define PARAM_ID_REVERB_ROOM_HF_LEVEL 0x8001102
#define PARAM_ID_REVERB_DECAY_TIME 0x8001103
#define PARAM_ID_REVERB_DECAY_HF_RATIO 0x8001104
#define PARAM_ID_REVERB_REFLECTIONS_LEVEL 0x8001105
#define PARAM_ID_REVERB_REFLECTIONS_DELAY 0x8001106
#define PARAM_ID_REVERB_LEVEL 0x8001107
#define PARAM_ID_REVERB_DELAY 0x8001108
#define PARAM_ID_REVERB_DIFFUSION 0x8001109
#define PARAM_ID_REVERB_DENSITY 0x800110a

#define PARAM_ID_VIRTUALIZER_STRENGTH 0x8001136
#define PARAM_ID_VIRTUALIZER_OUT_TYPE 0x8001137
#define PARAM_ID_VIRTUALIZER_GAIN_ADJUST 0x8001138

#define BASSBOOST_ENABLE_FLAG (1 << 0)
#define BASSBOOST_STRENGTH (BASSBOOST_ENABLE_FLAG << 1)
#define BASSBOOST_MODE (BASSBOOST_STRENGTH << 1)

#define VIRTUALIZER_ENABLE_FLAG (1 << 0)
#define VIRTUALIZER_STRENGTH (VIRTUALIZER_ENABLE_FLAG << 1)
#define VIRTUALIZER_OUT_TYPE (VIRTUALIZER_STRENGTH << 1)
#define VIRTUALIZER_GAIN_ADJUST (VIRTUALIZER_OUT_TYPE << 1)

#define EQ_ENABLE_FLAG (1 << 0)
#define EQ_PRESET (EQ_ENABLE_FLAG << 1)
#define EQ_BANDS_LEVEL (EQ_PRESET << 1)

#define REVERB_ENABLE_FLAG (1 << 0)
#define REVERB_MODE (REVERB_ENABLE_FLAG << 1)
#define REVERB_PRESET (REVERB_MODE << 1)
#define REVERB_WET_MIX (REVERB_PRESET << 1)
#define REVERB_GAIN_ADJUST (REVERB_WET_MIX << 1)
#define REVERB_ROOM_LEVEL (REVERB_GAIN_ADJUST << 1)
#define REVERB_ROOM_HF_LEVEL (REVERB_ROOM_LEVEL << 1)
#define REVERB_DECAY_TIME (REVERB_ROOM_HF_LEVEL << 1)
#define REVERB_DECAY_HF_RATIO (REVERB_DECAY_TIME << 1)
#define REVERB_REFLECTIONS_LEVEL (REVERB_DECAY_HF_RATIO << 1)
#define REVERB_REFLECTIONS_DELAY (REVERB_REFLECTIONS_LEVEL << 1)
#define REVERB_LEVEL (REVERB_REFLECTIONS_DELAY << 1)
#define REVERB_DELAY (REVERB_LEVEL << 1)
#define REVERB_DIFFUSION (REVERB_DELAY << 1)
#define REVERB_DENSITY (REVERB_DIFFUSION << 1)

struct ParamDelegator {
  public:
    /**
    * @brief updatePalParameters sends the pal parameters to update offload parameters
    * for bass boost effect.
    * @param handle valid pal stream handle
    * @param bassboost bassboost configuration
    * @param flags flags related to bassboost
    */
    static int updatePalParameters(pal_stream_handle_t *handle, struct BassBoostParams *bassboost,
                                   uint64_t flags);

    /**
    * @brief updatePalParameters sends the pal parameters to update offload parameters
    * for virtualizer
    * @param handle valid pal stream handle
    * @param virtualizer virtualizer configuration
    * @param flags flags related to virtualizer
    */
    static int updatePalParameters(pal_stream_handle_t *handle,
                                   struct VirtualizerParams *virtualizer, uint64_t flags);

    /**
    * @brief updatePalParameters sends the pal parameters to update offload parameters
    * for Equalizer effect.
    * @param handle valid pal stream handle
    * @param eq Equalizer configuration
    * @param flags flags related to Equalizer
    */
    static int updatePalParameters(pal_stream_handle_t *handle, struct EqualizerParams *eq,
                                   uint64_t flags);

    /**
    * @brief updatePalParameters sends the pal parameters to update offload parameters
    * for reverb effect.
    * @param handle valid pal stream handle
    * @param reverb reverb configuration
    * @param flags flags related to reverb
    */
    static int updatePalParameters(pal_stream_handle_t *handle, struct ReverbParams *reverb,
                                   uint64_t flags);

  private:
    /**
    * @brief sendKvPayload sends the pal key vector to PAL for TKVs config of effects
    * @param handle valid pal stream handle
    * @param tag stream tag
    * @param kvp pal key vector
    */
    static int sendKvPayload(pal_stream_handle_t *handle, uint32_t tag, pal_key_vector_t *kvp);

    /**
    * @brief setCustomPayload sends the pal_effect_custom_payload_t for non TKV config
    * @param handle valid pal stream handle
    * @param data custom payload
    * @param customDataSize size of custom payload
    */
    static int setCustomPayload(pal_stream_handle_t *hanlde, uint32_t tag,
                                pal_effect_custom_payload_t *data, uint32_t customDataSize);

    /**
    * @brief setCustomPayloadGeneric create a custom payload for BB, Virtualizer, Reverb
    * from paramId and data as these all parameters are of size 1.
    * @param handle valid pal stream handle
    * @param tag stream tag
    * @param paramId param id
    * @param data data related to param
    */
    static int setCustomPayloadGeneric(pal_stream_handle_t *handle, uint32_t tag, uint32_t paramId,
                                       uint32_t data);
};
} // namespace aidl::qti::effects