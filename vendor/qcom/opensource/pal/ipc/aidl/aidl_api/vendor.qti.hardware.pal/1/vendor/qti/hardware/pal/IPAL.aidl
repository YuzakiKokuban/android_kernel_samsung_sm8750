/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.qti.hardware.pal;
@VintfStability
interface IPAL {
  void ipc_pal_add_remove_effect(in long handle, in vendor.qti.hardware.pal.PalAudioEffect effect, in boolean enable);
  byte[] ipc_pal_gef_rw_param(in int paramId, in byte[] paramPayload, in vendor.qti.hardware.pal.PalDeviceId devId, in vendor.qti.hardware.pal.PalStreamType streamType, in byte dir);
  boolean ipc_pal_get_mic_mute();
  byte[] ipc_pal_get_param(in int paramId);
  vendor.qti.hardware.pal.PalSessionTime ipc_pal_get_timestamp(in long handle);
  void ipc_pal_register_global_callback(in vendor.qti.hardware.pal.IPALCallback cb, in long cookie);
  void ipc_pal_set_mic_mute(in boolean state);
  void ipc_pal_set_param(in int paramId, in byte[] payload);
  void ipc_pal_stream_close(in long handle);
  vendor.qti.hardware.pal.PalMmapBuffer ipc_pal_stream_create_mmap_buffer(in long handle, in int minSizeFrames);
  void ipc_pal_stream_drain(in long handle, in vendor.qti.hardware.pal.PalDrainType type);
  void ipc_pal_stream_flush(in long handle);
  int ipc_pal_stream_get_buffer_size(in long handle, in int inBufSize, in int outBufSize);
  vendor.qti.hardware.pal.PalDevice[] ipc_pal_stream_get_device(in long handle);
  vendor.qti.hardware.pal.PalMmapPosition ipc_pal_stream_get_mmap_position(in long handle);
  boolean ipc_pal_stream_get_mute(in long handle);
  vendor.qti.hardware.pal.PalParamPayload ipc_pal_stream_get_param(in long handle, in int paramId);
  byte[] ipc_pal_stream_get_tags_with_module_info(in long handle, in int size);
  vendor.qti.hardware.pal.PalVolumeData ipc_pal_stream_get_volume(in long handle);
  long ipc_pal_stream_open(in vendor.qti.hardware.pal.PalStreamAttributes attributes, in vendor.qti.hardware.pal.PalDevice[] devices, in vendor.qti.hardware.pal.ModifierKV[] modifiers, in vendor.qti.hardware.pal.IPALCallback cb, in long clientData);
  void ipc_pal_stream_pause(in long handle);
  vendor.qti.hardware.pal.PalReadReturnData ipc_pal_stream_read(in long handle, in vendor.qti.hardware.pal.PalBuffer[] buffer);
  void ipc_pal_stream_resume(in long handle);
  vendor.qti.hardware.pal.PalBufferConfig[] ipc_pal_stream_set_buffer_size(in long handle, in vendor.qti.hardware.pal.PalBufferConfig rxConfig, in vendor.qti.hardware.pal.PalBufferConfig txConfig);
  void ipc_pal_stream_set_device(in long handle, in vendor.qti.hardware.pal.PalDevice[] devices);
  void ipc_pal_stream_set_mute(in long handle, in boolean state);
  void ipc_pal_stream_set_param(in long handle, in int param_id, in vendor.qti.hardware.pal.PalParamPayloadShmem payload);
  void ipc_pal_stream_set_volume(in long handle, in vendor.qti.hardware.pal.PalVolumeData vol);
  void ipc_pal_stream_start(in long handle);
  void ipc_pal_stream_stop(in long handle);
  void ipc_pal_stream_suspend(in long handle);
  int ipc_pal_stream_write(in long handle, in vendor.qti.hardware.pal.PalBuffer[] buffer);
}
