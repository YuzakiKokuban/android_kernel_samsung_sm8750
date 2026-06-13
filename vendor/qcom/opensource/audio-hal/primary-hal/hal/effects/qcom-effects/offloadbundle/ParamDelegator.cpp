/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_Effect_ParamDelegator"

#include "ParamDelegator.h"
#include <android-base/logging.h>

namespace aidl::qti::effects {

#define OFFLOAD_PRESET_START_OFFSET 19
const int kEqualizerOpenSlToOffloadMap[] = {
        OFFLOAD_PRESET_START_OFFSET,     /* Normal Preset */
        OFFLOAD_PRESET_START_OFFSET + 1, /* Classical Preset */
        OFFLOAD_PRESET_START_OFFSET + 2, /* Dance Preset */
        OFFLOAD_PRESET_START_OFFSET + 3, /* Flat Preset */
        OFFLOAD_PRESET_START_OFFSET + 4, /* Folk Preset */
        OFFLOAD_PRESET_START_OFFSET + 5, /* Heavy Metal Preset */
        OFFLOAD_PRESET_START_OFFSET + 6, /* Hip Hop Preset */
        OFFLOAD_PRESET_START_OFFSET + 7, /* Jazz Preset */
        OFFLOAD_PRESET_START_OFFSET + 8, /* Pop Preset */
        OFFLOAD_PRESET_START_OFFSET + 9, /* Rock Preset */
        OFFLOAD_PRESET_START_OFFSET + 10 /* FX Booster */
};

const int kReverbOpenSlToOffloadMap[] = {15, 16, 17, 18, 19, 20};

#define EXIT_IF_SET_PARAM_FAILS(ret)                      \
    if (ret) {                                            \
        LOG(ERROR) << __func__ << "pal_set_param failed"; \
        return ret;                                       \
    }

#define VALUE_OR_RETURN(ptr)                                       \
    ({                                                             \
        auto temp = (ptr);                                         \
        if (temp.get() == nullptr) {                               \
            LOG(ERROR) << __func__ << "could not allocate memory"; \
            return -ENOMEM;                                        \
        }                                                          \
        std::move(temp);                                           \
    })

using CustomDeletor = void (*)(void *);
template <typename T>
std::unique_ptr<T, CustomDeletor> allocate(int size) {
    T *obj = reinterpret_cast<T *>(calloc(1, size));
    return std::unique_ptr<T, CustomDeletor>{obj, free};
}

using EffectCustomPayload = pal_effect_custom_payload_t;

int ParamDelegator::sendKvPayload(pal_stream_handle_t *handle, uint32_t tag,
                                  pal_key_vector_t *kvp) {
    uint32_t payloadSize = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) +
                           sizeof(pal_key_vector_t) + kvp->num_tkvs * sizeof(pal_key_value_pair_t);

    auto payload = VALUE_OR_RETURN(allocate<uint8_t>(payloadSize));
    uint8_t *payloadPtr = payload.get();

    pal_param_payload *palPayload = (pal_param_payload *)payloadPtr;
    palPayload->payload_size = sizeof(effect_pal_payload_t) + sizeof(pal_key_vector_t) +
                               kvp->num_tkvs * sizeof(pal_key_value_pair_t);

    effect_pal_payload_t *effectPalPayload =
            (effect_pal_payload_t *)(payloadPtr + sizeof(pal_param_payload));
    effectPalPayload->isTKV = PARAM_TKV;
    effectPalPayload->tag = tag;
    effectPalPayload->payloadSize =
            sizeof(pal_key_vector_t) + kvp->num_tkvs * sizeof(pal_key_value_pair_t);

    pal_key_vector_t *palKeyVector = (pal_key_vector_t *)(payloadPtr + sizeof(pal_param_payload) +
                                                          sizeof(effect_pal_payload_t));

    palKeyVector->num_tkvs = kvp->num_tkvs;
    memcpy(palKeyVector->kvp, kvp->kvp, (kvp->num_tkvs * sizeof(pal_key_value_pair_t)));

    return pal_stream_set_param(handle, PAL_PARAM_ID_UIEFFECT, palPayload);
}

int ParamDelegator::setCustomPayload(pal_stream_handle_t *handle, uint32_t tag,
                                     pal_effect_custom_payload_t *data, uint32_t customDataSize) {
    uint32_t payloadSize = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) +
                           sizeof(pal_effect_custom_payload_t) + customDataSize;

    auto payload = VALUE_OR_RETURN(allocate<uint8_t>(payloadSize));
    uint8_t *payloadPtr = payload.get();

    pal_param_payload *palPayload = (pal_param_payload *)payloadPtr;
    palPayload->payload_size =
            sizeof(effect_pal_payload_t) + sizeof(pal_effect_custom_payload_t) + customDataSize;

    effect_pal_payload_t *effectPalPayload =
            (effect_pal_payload_t *)(payloadPtr + sizeof(pal_param_payload));
    effectPalPayload->isTKV = PARAM_NONTKV;
    effectPalPayload->tag = tag;
    effectPalPayload->payloadSize = sizeof(pal_effect_custom_payload_t) + customDataSize;

    pal_effect_custom_payload_t *customPayload =
            (pal_effect_custom_payload_t *)(payloadPtr + sizeof(pal_param_payload) +
                                            sizeof(effect_pal_payload_t));

    customPayload->paramId = data->paramId;
    memcpy(customPayload->data, data->data, customDataSize);
    return pal_stream_set_param(handle, PAL_PARAM_ID_UIEFFECT, palPayload);
}

int ParamDelegator::setCustomPayloadGeneric(pal_stream_handle_t *handle, uint32_t tag,
                                            uint32_t paramId, uint32_t data) {
    // All reverb custom params are of same length, so reuse the same size for all params
    uint32_t customDataSize = GENERIC_CUSTOM_PARAM_LEN * sizeof(uint32_t);
    uint32_t payloadAllocSize = sizeof(pal_effect_custom_payload_t) + customDataSize;
    auto customPayload = VALUE_OR_RETURN(allocate<pal_effect_custom_payload_t>(payloadAllocSize));
    customPayload->paramId = paramId;
    customPayload->data[0] = data;
    LOG(VERBOSE) << __func__ << " tag " << std::hex << tag << " paramId " << paramId << std::dec
                 << " data " << data;
    return setCustomPayload(handle, tag, customPayload.get(), customDataSize);
}

int ParamDelegator::updatePalParameters(pal_stream_handle_t *handle,
                                        struct BassBoostParams *bassboost, uint64_t flags) {
    int ret = 0;
    uint32_t streamTag = TAG_STREAM_BASS_BOOST;
    LOG(VERBOSE) << __func__ << " flags " << std::hex << flags;
    if (flags & BASSBOOST_ENABLE_FLAG) {
        uint32_t numberOfKvs = 1;
        auto allocSize = sizeof(pal_key_vector_t) + numberOfKvs * sizeof(pal_key_value_pair_t);
        auto palKeyVector = VALUE_OR_RETURN(allocate<pal_key_vector_t>(allocSize));

        palKeyVector->num_tkvs = numberOfKvs;
        palKeyVector->kvp[0].key = BASS_BOOST_SWITCH;
        palKeyVector->kvp[0].value = bassboost->mEnabled;

        ret = sendKvPayload(handle, streamTag, palKeyVector.get());
        EXIT_IF_SET_PARAM_FAILS(ret);
    }

    if (flags & BASSBOOST_STRENGTH) {
        ret = setCustomPayloadGeneric(handle, streamTag, PARAM_ID_BASS_BOOST_STRENGTH,
                                      bassboost->mStrength);
    }

    return ret;
}

int ParamDelegator::updatePalParameters(pal_stream_handle_t *handle,
                                        struct VirtualizerParams *virtualizer, uint64_t flags) {
    int ret = 0;
    uint32_t streamTag = TAG_STREAM_VIRTUALIZER;
    LOG(VERBOSE) << __func__ << " flags " << std::hex << flags;
    if (flags & VIRTUALIZER_ENABLE_FLAG) {
        uint32_t numberOfKvs = 1;
        auto allocSize = sizeof(pal_key_vector_t) + numberOfKvs * sizeof(pal_key_value_pair_t);
        auto palKeyVector = VALUE_OR_RETURN(allocate<pal_key_vector_t>(allocSize));

        palKeyVector->num_tkvs = numberOfKvs;
        palKeyVector->kvp[0].key = VIRTUALIZER_SWITCH;
        palKeyVector->kvp[0].value = virtualizer->enable;

        ret = sendKvPayload(handle, streamTag, palKeyVector.get());
        EXIT_IF_SET_PARAM_FAILS(ret);
    }

    // only 1 of below flags can be set at a time.
    uint32_t paramId = 0, data = 0;
    if (flags & VIRTUALIZER_STRENGTH) {
        paramId = PARAM_ID_VIRTUALIZER_STRENGTH;
        data = virtualizer->strength;
    } else if (flags & VIRTUALIZER_OUT_TYPE) {
        paramId = PARAM_ID_VIRTUALIZER_OUT_TYPE;
        data = virtualizer->type;
    } else if (flags & VIRTUALIZER_GAIN_ADJUST) {
        paramId = PARAM_ID_VIRTUALIZER_GAIN_ADJUST;
        data = virtualizer->gainAdjust;
    } else { // no valid flag return
        return 0;
    }

    return setCustomPayloadGeneric(handle, streamTag, paramId, data);
}

int ParamDelegator::updatePalParameters(pal_stream_handle_t *handle, struct EqualizerParams *eq,
                                        uint64_t flags) {
    int ret = 0;
    uint32_t streamTag = TAG_STREAM_EQUALIZER;
    if (!handle) {
        LOG(ERROR) << __func__ << " stream is not opened, invalid pal handle";
        return -EINVAL;
    }
    LOG(VERBOSE) << __func__ << " flags " << std::hex << flags;
    if ((eq->config.presetId < -1) || ((flags & EQ_PRESET) && (eq->config.presetId == -1))) {
        LOG(VERBOSE) << __func__ << " no preset to set";
        return 0;
    }

    if (flags & EQ_ENABLE_FLAG) {
        LOG(VERBOSE) << __func__ << " Equalizer enable";
        uint32_t numberOfKvs = 1;
        auto allocSize = sizeof(pal_key_vector_t) + numberOfKvs * sizeof(pal_key_value_pair_t);
        auto palKeyVector = VALUE_OR_RETURN(allocate<pal_key_vector_t>(allocSize));

        palKeyVector->num_tkvs = numberOfKvs;
        palKeyVector->kvp[0].key = EQUALIZER_SWITCH;
        palKeyVector->kvp[0].value = eq->enable;
        ret = sendKvPayload(handle, streamTag, palKeyVector.get());
        LOG(VERBOSE) << __func__ << " Equalizer enable: ret " << ret;
        EXIT_IF_SET_PARAM_FAILS(ret);
    }

    // at a time only 1 of below flags can be set.
    if (flags & EQ_PRESET) {
        LOG(VERBOSE) << __func__ << " Equalizer preset";
        uint32_t customDataSize = EQ_CONFIG_PARAM_LEN * sizeof(uint32_t);
        uint32_t allocSize = sizeof(pal_effect_custom_payload_t) + customDataSize;
        auto customPayload = VALUE_OR_RETURN(allocate<pal_effect_custom_payload_t>(allocSize));
        customPayload->paramId = PARAM_ID_EQ_CONFIG;
        customPayload->data[0] = eq->config.pregain;
        customPayload->data[1] = kEqualizerOpenSlToOffloadMap[eq->config.presetId];
        customPayload->data[2] = 0; // num_of_band must be 0 for preset

        ret = setCustomPayload(handle, streamTag, customPayload.get(), customDataSize);
    } else if (flags & EQ_BANDS_LEVEL) {
        LOG(VERBOSE) << __func__ << " Equalizer band levels";
        uint32_t customDataSize =
                (EQ_CONFIG_PARAM_LEN + (eq->config.numBands * EQ_CONFIG_PER_BAND_PARAM_LEN)) *
                sizeof(uint32_t);

        uint32_t allocSize = sizeof(pal_effect_custom_payload_t) + customDataSize;
        auto customPayload = VALUE_OR_RETURN(allocate<pal_effect_custom_payload_t>(allocSize));

        customPayload->paramId = PARAM_ID_EQ_CONFIG;
        int index = 0;
        customPayload->data[index++] = eq->config.pregain;
        customPayload->data[index++] = CUSTOM_OPENSL_PRESET;
        customPayload->data[index++] = eq->config.numBands;
        for (int i = 0; i < eq->config.numBands; i++) {
            LOG(VERBOSE) << __func__ << " band " << i << " filter " << eq->bandConfig[i].filterType
                         << " frequency " << eq->bandConfig[i].frequencyMhz << " gain "
                         << eq->bandConfig[i].gainMb << " quality factor "
                         << eq->bandConfig[i].qFactor << " index " << eq->bandConfig[i].bandIndex;
            customPayload->data[index++] = eq->bandConfig[i].filterType;
            customPayload->data[index++] = eq->bandConfig[i].frequencyMhz;
            customPayload->data[index++] = eq->bandConfig[i].gainMb;
            customPayload->data[index++] = eq->bandConfig[i].qFactor;
            customPayload->data[index++] = eq->bandConfig[i].bandIndex;
        }
        ret = setCustomPayload(handle, streamTag, customPayload.get(), customDataSize);
        LOG(VERBOSE) << __func__ << " Equalizer band levels ret " << ret;
    }

    return ret;
}

int ParamDelegator::updatePalParameters(pal_stream_handle_t *handle, struct ReverbParams *reverb,
                                        uint64_t flags) {
    int ret = 0;
    uint32_t streamTag = TAG_STREAM_REVERB;
    LOG(VERBOSE) << __func__ << " flags " << std::hex << flags;
    if (flags & REVERB_ENABLE_FLAG) {
        uint32_t numberOfKvs = 1;
        auto memSize = sizeof(pal_key_vector_t) + numberOfKvs * sizeof(pal_key_value_pair_t);
        auto palKeyVector = VALUE_OR_RETURN(allocate<pal_key_vector_t>(memSize));

        palKeyVector->num_tkvs = numberOfKvs;
        palKeyVector->kvp[0].key = REVERB_SWITCH;
        palKeyVector->kvp[0].value = reverb->enable;
        ret = sendKvPayload(handle, streamTag, palKeyVector.get());
        EXIT_IF_SET_PARAM_FAILS(ret);
    }

    // Only 1 of these is possible at a time.
    uint32_t paramId = 0, data = 0;
    if (flags & REVERB_MODE) {
        paramId = PARAM_ID_REVERB_MODE;
        data = reverb->mode;
    } else if (flags & REVERB_PRESET) {
        paramId = PARAM_ID_REVERB_PRESET;
        data = kReverbOpenSlToOffloadMap[reverb->preset - 1];
    } else if (flags & REVERB_WET_MIX) {
        paramId = PARAM_ID_REVERB_WET_MIX;
        data = reverb->wetMix;
    } else if (flags & REVERB_GAIN_ADJUST) {
        paramId = PARAM_ID_REVERB_GAIN_ADJUST;
        data = reverb->gainAdjust;
    } else if (flags & REVERB_ROOM_LEVEL) {
        paramId = PARAM_ID_REVERB_ROOM_LEVEL;
        data = reverb->roomLevel;
    } else if (flags & REVERB_ROOM_HF_LEVEL) {
        paramId = PARAM_ID_REVERB_ROOM_HF_LEVEL;
        data = reverb->roomHfLevel;
    } else if (flags & REVERB_DECAY_TIME) {
        paramId = PARAM_ID_REVERB_DECAY_TIME;
        data = reverb->decayTime;
    } else if (flags & REVERB_DECAY_HF_RATIO) {
        paramId = PARAM_ID_REVERB_DECAY_HF_RATIO;
        data = reverb->decayHfRatio;
    } else if (flags & REVERB_REFLECTIONS_LEVEL) {
        paramId = PARAM_ID_REVERB_REFLECTIONS_LEVEL;
        data = reverb->reflectionsLevel;
    } else if (flags & REVERB_REFLECTIONS_DELAY) {
        paramId = PARAM_ID_REVERB_REFLECTIONS_DELAY;
        data = reverb->reflectionsDelay;
    } else if (flags & REVERB_LEVEL) {
        paramId = PARAM_ID_REVERB_LEVEL;
        data = reverb->level;
    } else if (flags & REVERB_DELAY) {
        paramId = PARAM_ID_REVERB_DELAY;
        data = reverb->delay;
    } else if (flags & REVERB_DIFFUSION) {
        paramId = PARAM_ID_REVERB_DIFFUSION;
        data = reverb->diffusion;
    } else if (flags & REVERB_DENSITY) {
        paramId = PARAM_ID_REVERB_DENSITY;
        data = reverb->density;
    } else { // no flag found
        return 0;
    }

    return setCustomPayloadGeneric(handle, streamTag, paramId, data);
}

} // namespace aidl::qti::effects