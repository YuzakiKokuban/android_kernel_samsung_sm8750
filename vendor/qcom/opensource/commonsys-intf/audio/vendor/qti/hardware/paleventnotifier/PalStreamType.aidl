/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

/**
 * Audio stream types
 */
@VintfStability
@Backing(type="int")
enum PalStreamType {
    PAL_STREAM_LOW_LATENCY = 1,
    /**
     * < :low latency, higher power
     */
    PAL_STREAM_DEEP_BUFFER = 2,
    /**
     * < :low power, higher latency
     */
    PAL_STREAM_COMPRESSED = 3,
    /**
     * < :compresssed audio
     */
    PAL_STREAM_VOIP = 4,
    /**
     * < :pcm voip audio
     */
    PAL_STREAM_VOIP_RX = 5,
    /**
     * < :pcm voip audio downlink
     */
    PAL_STREAM_VOIP_TX = 6,
    /**
     * < :pcm voip audio uplink
     */
    PAL_STREAM_VOICE_CALL_MUSIC = 7,
    /**
     * < :incall music
     */
    PAL_STREAM_GENERIC = 8,
    /**
     * < :generic playback audio
     */
    PAL_STREAM_RAW = 9,
    /**
     * < pcm no post processing
     */
    PAL_STREAM_VOICE_RECOGNITION = 10,
    /**
     * < voice recognition
     */
    PAL_STREAM_VOICE_CALL_RECORD = 11,
    /**
     * < incall record
     */
    PAL_STREAM_VOICE_CALL_TX = 12,
    /**
     * < incall record, uplink
     */
    PAL_STREAM_VOICE_CALL_RX_TX = 13,
    /**
     * < incall record, uplink & Downlink
     */
    PAL_STREAM_VOICE_CALL = 14,
    /**
     * < voice call
     */
    PAL_STREAM_LOOPBACK = 15,
    /**
     * < loopback
     */
    PAL_STREAM_TRANSCODE = 16,
    /**
     * < audio transcode
     */
    PAL_STREAM_VOICE_UI = 17,
    /**
     * < voice ui
     */
    PAL_STREAM_PCM_OFFLOAD = 18,
    /**
     * < pcm offload audio
     */
    PAL_STREAM_ULTRA_LOW_LATENCY = 19,
    /**
     * < pcm ULL audio
     */
    PAL_STREAM_PROXY = 20,
    /**
     * < pcm proxy audio
     */
    PAL_STREAM_NON_TUNNEL = 21,
    /**
     * < NT Mode session
     */
    PAL_STREAM_HAPTICS = 22,
    /**
     * < Haptics Stream
     */
    PAL_STREAM_ACD = 23,
    /**
     * < ACD Stream
     */
    PAL_STREAM_CONTEXT_PROXY = 24,
    /**
     * < Context Proxy Stream
     */
    PAL_STREAM_SENSOR_PCM_DATA = 25,
    /**
     * < Sensor PCM Data Stream
     */
    PAL_STREAM_ULTRASOUND = 26,
    /**
     * < Ultrasound Proximity detection
     */
    PAL_STREAM_SPATIAL_AUDIO = 27,
}
