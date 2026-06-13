/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.pal;

import vendor.qti.hardware.pal.IPALCallback;
import vendor.qti.hardware.pal.ModifierKV;
import vendor.qti.hardware.pal.PalAudioEffect;
import vendor.qti.hardware.pal.PalBuffer;
import vendor.qti.hardware.pal.PalBufferConfig;
import vendor.qti.hardware.pal.PalDevice;
import vendor.qti.hardware.pal.PalDeviceId;
import vendor.qti.hardware.pal.PalDrainType;
import vendor.qti.hardware.pal.PalMmapBuffer;
import vendor.qti.hardware.pal.PalMmapPosition;
import vendor.qti.hardware.pal.PalParamPayload;
import vendor.qti.hardware.pal.PalSessionTime;
import vendor.qti.hardware.pal.PalStreamAttributes;
import vendor.qti.hardware.pal.PalStreamType;
import vendor.qti.hardware.pal.PalVolumeData;
import vendor.qti.hardware.pal.PalReadReturnData;
import vendor.qti.hardware.pal.PalParamPayloadShmem;

@VintfStability
interface IPAL {
    void ipc_pal_add_remove_effect(in long handle, in PalAudioEffect effect,
        in boolean enable);

    byte[] ipc_pal_gef_rw_param(in int paramId, in byte[] paramPayload,
        in PalDeviceId devId, in PalStreamType streamType, in byte dir);

    boolean ipc_pal_get_mic_mute();

    byte[] ipc_pal_get_param(in int paramId);

    PalSessionTime ipc_pal_get_timestamp(in long handle);

    void ipc_pal_register_global_callback(in IPALCallback cb, in long cookie);

    void ipc_pal_set_mic_mute(in boolean state);

    void ipc_pal_set_param(in int paramId, in byte[] payload);

    void ipc_pal_stream_close(in long handle);

    PalMmapBuffer ipc_pal_stream_create_mmap_buffer(in long handle, in int minSizeFrames);

    void ipc_pal_stream_drain(in long handle, in PalDrainType type);

    void ipc_pal_stream_flush(in long handle);

    int ipc_pal_stream_get_buffer_size(in long handle, in int inBufSize,
        in int outBufSize);

    PalDevice[] ipc_pal_stream_get_device(in long handle);

    PalMmapPosition ipc_pal_stream_get_mmap_position(in long handle);

    boolean ipc_pal_stream_get_mute(in long handle);

    PalParamPayload ipc_pal_stream_get_param(in long handle, in int paramId);

    byte[] ipc_pal_stream_get_tags_with_module_info(in long handle, in int size);

    PalVolumeData ipc_pal_stream_get_volume(in long handle);

    long ipc_pal_stream_open(in PalStreamAttributes attributes, in PalDevice[] devices,
                             in ModifierKV[] modifiers, in IPALCallback cb, in long clientData);

    void ipc_pal_stream_pause(in long handle);

    PalReadReturnData ipc_pal_stream_read(in long handle, in PalBuffer[] buffer);

    void ipc_pal_stream_resume(in long handle);

    PalBufferConfig[] ipc_pal_stream_set_buffer_size(in long handle, in PalBufferConfig rxConfig,
        in PalBufferConfig txConfig);

    void ipc_pal_stream_set_device(in long handle,
        in PalDevice[] devices);

    void ipc_pal_stream_set_mute(in long handle, in boolean state);

    void ipc_pal_stream_set_param(in long handle, in int param_id,
                                  in PalParamPayloadShmem payload);

    void ipc_pal_stream_set_volume(in long handle, in PalVolumeData vol);

    void ipc_pal_stream_start(in long handle);

    void ipc_pal_stream_stop(in long handle);

    void ipc_pal_stream_suspend(in long handle);

    int ipc_pal_stream_write(in long handle, in PalBuffer[] buffer);
}
