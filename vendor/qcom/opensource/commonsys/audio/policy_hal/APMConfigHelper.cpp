/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "APMConfigHelper"

#include "APMConfigHelper.h"
#include <cutils/properties.h>
#include <log/log.h>
#include <stdlib.h>

namespace android {

void APMConfigHelper::dump(String8 *dst) const
{
    // apmconfig struct dump
    dst->appendFormat("\nAudioPolicyManagerCustom Dump: %p\n", this);
    dst->appendFormat("audio_offload_video: %d \n", mConfigs.audio_offload_video);
    dst->appendFormat("audio_offload_disable: %d \n", mConfigs.audio_offload_disable);
    dst->appendFormat("audio_deepbuffer_media: %d \n", mConfigs.audio_deepbuffer_media);
    dst->appendFormat("audio_av_streaming_offload_enable: %d \n", mConfigs.audio_av_streaming_offload_enable);
    dst->appendFormat("audio_offload_track_enable: %d \n", mConfigs.audio_offload_track_enable);
    dst->appendFormat("audio_offload_multiple_enabled: %d \n", mConfigs.audio_offload_multiple_enabled);
    dst->appendFormat("voice_dsd_playback_conc_disabled: %d \n", mConfigs.voice_dsd_playback_conc_disabled);
    dst->appendFormat("audio_sva_conc_enabled: %d \n", mConfigs.audio_sva_conc_enabled);
    dst->appendFormat("audio_va_concurrency_enabled: %d \n", mConfigs.audio_va_concurrency_enabled);
    dst->appendFormat("audio_rec_playback_conc_disabled: %d \n", mConfigs.audio_rec_playback_conc_disabled);
    dst->appendFormat("voice_path_for_pcm_voip: %d \n", mConfigs.voice_path_for_pcm_voip);
    dst->appendFormat("voice_playback_conc_disabled: %d \n", mConfigs.voice_playback_conc_disabled);
    dst->appendFormat("voice_record_conc_disabled: %d \n", mConfigs.voice_record_conc_disabled);
    dst->appendFormat("voice_voip_conc_disabled: %d \n", mConfigs.voice_voip_conc_disabled);
    dst->appendFormat("audio_offload_min_duration_secs: %u \n", mConfigs.audio_offload_min_duration_secs);
    dst->appendFormat("voice_conc_fallbackpath: %s \n", mConfigs.voice_conc_fallbackpath.c_str());
    dst->appendFormat("audio_extn_hdmi_spk_enabled: %d \n", mConfigs.audio_extn_hdmi_spk_enabled);
    dst->appendFormat("audio_extn_formats_enabled: %d \n", mConfigs.audio_extn_formats_enabled);
    dst->appendFormat("audio_extn_afe_proxy_enabled: %d \n", mConfigs.audio_extn_afe_proxy_enabled);
    dst->appendFormat("compress_voip_enabled: %d \n", mConfigs.compress_voip_enabled);
    dst->appendFormat("fm_power_opt: %d \n", mConfigs.fm_power_opt);
    dst->appendFormat("voice_concurrency: %d \n", mConfigs.voice_concurrency);
    dst->appendFormat("record_play_concurrency: %d \n", mConfigs.record_play_concurrency);
    dst->appendFormat("use_xml_audio_policy_conf: %d \n", mConfigs.use_xml_audio_policy_conf);
}

void APMConfigHelper::retrieveConfigs()
{
    // apmconfig struct dump
    ALOGV("apmconfigs retrieved from %s", isRemote? "remote" : "local");
    ALOGV("audio_offload_video: %d", mConfigs.audio_offload_video);
    ALOGV("audio_offload_disable: %d", mConfigs.audio_offload_disable);
    ALOGV("audio_deepbuffer_media: %d", mConfigs.audio_deepbuffer_media);
    ALOGV("audio_av_streaming_offload_enable: %d", mConfigs.audio_av_streaming_offload_enable);
    ALOGV("audio_offload_track_enable: %d", mConfigs.audio_offload_track_enable);
    ALOGV("audio_offload_multiple_enabled: %d", mConfigs.audio_offload_multiple_enabled);
    ALOGV("voice_dsd_playback_conc_disabled: %d", mConfigs.voice_dsd_playback_conc_disabled);
    ALOGV("audio_sva_conc_enabled: %d", mConfigs.audio_sva_conc_enabled);
    ALOGV("audio_va_concurrency_enabled: %d", mConfigs.audio_va_concurrency_enabled);
    ALOGV("audio_rec_playback_conc_disabled: %d", mConfigs.audio_rec_playback_conc_disabled);
    ALOGV("voice_path_for_pcm_voip: %d", mConfigs.voice_path_for_pcm_voip);
    ALOGV("voice_playback_conc_disabled: %d", mConfigs.voice_playback_conc_disabled);
    ALOGV("voice_record_conc_disabled: %d", mConfigs.voice_record_conc_disabled);
    ALOGV("voice_voip_conc_disabled: %d", mConfigs.voice_voip_conc_disabled);
    ALOGV("audio_offload_min_duration_secs: %u", mConfigs.audio_offload_min_duration_secs);
    ALOGV("voice_conc_fallbackpath: %s", mConfigs.voice_conc_fallbackpath.c_str());
    ALOGV("audio_extn_hdmi_spk_enabled: %d", mConfigs.audio_extn_hdmi_spk_enabled);
    ALOGV("audio_extn_formats_enabled: %d", mConfigs.audio_extn_formats_enabled);
    ALOGV("audio_extn_afe_proxy_enabled: %d", mConfigs.audio_extn_afe_proxy_enabled);
    ALOGV("compress_voip_enabled: %d", mConfigs.compress_voip_enabled);
    ALOGV("fm_power_opt: %d", mConfigs.fm_power_opt);
    ALOGV("voice_concurrency: %d", mConfigs.voice_concurrency);
    ALOGV("record_play_concurrency: %d", mConfigs.record_play_concurrency);
    ALOGV("use_xml_audio_policy_conf: %d", mConfigs.use_xml_audio_policy_conf);
    return;
}

APMConfigHelper::APMConfigHelper()
    : isRemote(false)
{
    uint32_t audio_offload_min_duration_secs = 0;
    char voice_conc_fallbackpath[PROPERTY_VALUE_MAX] = "\0";

    audio_offload_min_duration_secs =
        (uint32_t)property_get_int32("audio.offload.min.duration.secs", 0);
    property_get("vendor.voice.conc.fallbackpath", voice_conc_fallbackpath, NULL);

    mConfigs = {
        (bool) property_get_bool("audio.offload.video", true),
        (bool) property_get_bool("audio.offload.disable", false),
        (bool) property_get_bool("audio.deep_buffer.media", true),
        (bool) property_get_bool("vendor.audio.av.streaming.offload.enable", false),
        (bool) property_get_bool("vendor.audio.offload.track.enable", true),
        (bool) property_get_bool("vendor.audio.offload.multiple.enabled", false),
        (bool) property_get_bool("vendor.voice.dsd.playback.conc.disabled", true),
        (bool) property_get_bool("persist.vendor.audio.sva.conc.enabled", false),
        (bool) property_get_bool("persist.vendor.audio.va_concurrency_enabled", false),
        (bool) property_get_bool("vendor.audio.rec.playback.conc.disabled", false),
        (bool) property_get_bool("vendor.voice.path.for.pcm.voip", true),
        (bool) property_get_bool("vendor.voice.playback.conc.disabled", false),
        (bool) property_get_bool("vendor.voice.record.conc.disabled", false),
        (bool) property_get_bool("vendor.voice.voip.conc.disabled", false),
        audio_offload_min_duration_secs,
        voice_conc_fallbackpath,
#ifdef AUDIO_EXTN_HDMI_SPK_ENABLED
        true,
#else
        false,
#endif
#ifdef AUDIO_EXTN_FORMATS_ENABLED
        true,
#else
        false,
#endif
#ifdef AUDIO_EXTN_AFE_PROXY_ENABLED
        true,
#else
        false,
#endif
#ifdef COMPRESS_VOIP_ENABLED
        true,
#else
        false,
#endif
#ifdef FM_POWER_OPT
        true,
#else
        false,
#endif
#ifdef VOICE_CONCURRENCY
        true,
#else
        false,
#endif
#ifdef RECORD_PLAY_CONCURRENCY
        true,
#else
        false,
#endif
#ifdef USE_XML_AUDIO_POLICY_CONF
        true,
#else
        false,
#endif
    };
}

bool APMConfigHelper::isAudioOffloadVideoEnabled()
{
    ALOGV("isAudioOffloadVideoEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_offload_video;
}

bool APMConfigHelper::isAudioOffloadDisabled()
{
    ALOGV("isAudioOffloadDisabled enter");
    retrieveConfigs();
    return mConfigs.audio_offload_disable;
}

bool APMConfigHelper::isAudioDeepbufferMediaEnabled()
{
    ALOGV("isAudioDeepbufferMediaEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_deepbuffer_media;
}

bool APMConfigHelper::isAVStreamingOffloadEnabled()
{
    ALOGV("isAVStreamingOffloadEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_av_streaming_offload_enable;
}

bool APMConfigHelper::isAudioTrackOffloadEnabled()
{
    ALOGV("isAudioTrackOffloadEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_offload_track_enable;
}

bool APMConfigHelper::isAudioMultipleOffloadEnable()
{
    ALOGV("isAudioMultipleOffloadEnable enter");
    retrieveConfigs();
    return mConfigs.audio_offload_multiple_enabled;
}

bool APMConfigHelper::isVoiceDSDConcDisabled()
{
    ALOGV("isVoiceDSDConcDisabled enter");
    retrieveConfigs();
    return mConfigs.voice_dsd_playback_conc_disabled;
}

bool APMConfigHelper::isSVAConcEnabled()
{
    ALOGV("isSVAConcEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_sva_conc_enabled;
}

bool APMConfigHelper::isVAConcEnabled()
{
    ALOGV("isVAConcEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_va_concurrency_enabled;
}

bool APMConfigHelper::isRecPlayConcDisabled()
{
    ALOGV("isRecPlayConcDisabled enter");
    retrieveConfigs();
    return mConfigs.audio_rec_playback_conc_disabled;
}

bool APMConfigHelper::useVoicePathForPCMVOIP()
{
    ALOGV("useVoicePathForPCMVOIP enter");
    retrieveConfigs();
    return mConfigs.voice_path_for_pcm_voip;
}

bool APMConfigHelper::isVoicePlayConcDisabled()
{
    ALOGV("isVoicePlayConcDisabled enter");
    retrieveConfigs();
    return mConfigs.voice_playback_conc_disabled;
}

bool APMConfigHelper::isVoiceRecConcDisabled()
{
    ALOGV("isVoiceRecConcDisabled enter");
    retrieveConfigs();
    return mConfigs.voice_record_conc_disabled;
}

bool APMConfigHelper::isVoiceVOIPConcDisabled()
{
    ALOGV("isVoiceVOIPConcDisabled enter");
    retrieveConfigs();
    return mConfigs.voice_voip_conc_disabled;
}

uint32_t APMConfigHelper::getAudioOffloadMinDuration()
{
    ALOGV("getAudioOffloadMinDuration enter");
    retrieveConfigs();
    return mConfigs.audio_offload_min_duration_secs;
}

string APMConfigHelper::getVoiceConcFallbackPath()
{
    ALOGV("getVoiceConcFallbackPath enter");
    retrieveConfigs();
    return mConfigs.voice_conc_fallbackpath;
}

bool APMConfigHelper::isHDMISpkEnabled()
{
    ALOGV("isHDMISpkEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_extn_hdmi_spk_enabled;
}

bool APMConfigHelper::isExtnFormatsEnabled()
{
    ALOGV("isExtnFormatsEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_extn_formats_enabled;
}

bool APMConfigHelper::isAFEProxyEnabled()
{
    ALOGV("isAFEProxyEnabled enter");
    retrieveConfigs();
    return mConfigs.audio_extn_afe_proxy_enabled;
}

bool APMConfigHelper::isCompressVOIPEnabled()
{
    ALOGV("isCompressVOIPEnabled enter");
    retrieveConfigs();
    return mConfigs.compress_voip_enabled;
}

bool APMConfigHelper::isFMPowerOptEnabled()
{
    ALOGV("isFMPowerOptEnabled enter");
    retrieveConfigs();
    return mConfigs.fm_power_opt;
}

bool APMConfigHelper::isVoiceConcEnabled()
{
    ALOGV("isVoiceConcEnabled enter");
    retrieveConfigs();
    return mConfigs.voice_concurrency;
}

bool APMConfigHelper::isRecPlayConcEnabled()
{
    ALOGV("isRecPlayConcEnabled enter");
    retrieveConfigs();
    return mConfigs.record_play_concurrency;
}

bool APMConfigHelper::useXMLAudioPolicyConf()
{
    ALOGV("useXMLAudioPolicyConf enter");
    retrieveConfigs();
    return mConfigs.use_xml_audio_policy_conf;
}

}
