/**
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.AgmBuff;
import vendor.qti.hardware.agm.AgmBufferConfig;
import vendor.qti.hardware.agm.AgmCalConfig;
import vendor.qti.hardware.agm.AgmDumpInfo;
import vendor.qti.hardware.agm.AgmEventRegistrationConfig;
import vendor.qti.hardware.agm.AgmGaplessSilenceType;
import vendor.qti.hardware.agm.AgmGroupMediaConfig;
import vendor.qti.hardware.agm.AgmMediaConfig;
import vendor.qti.hardware.agm.AgmSessionConfig;
import vendor.qti.hardware.agm.AgmSessionMode;
import vendor.qti.hardware.agm.AgmTagConfig;
import vendor.qti.hardware.agm.AifInfo;
import vendor.qti.hardware.agm.Direction;
import vendor.qti.hardware.agm.IAGMCallback;
import vendor.qti.hardware.agm.MmapBufInfo;

@VintfStability
interface IAGM {

    /**
    * Initialize agm.
    * Clients can't directly access this method as init is done in context of server
    * API is provided to keep in sync with native agm_api.h
    */
    void ipc_agm_init();

    /**
    * De-Initialize agm.
    * Clients can't directly access this method as deinit is done in context of server
    * API is provided to keep in sync with native agm_api.h
    */
    void ipc_agm_deinit();

    /**
    * Start the session.
    * @param sessionId  valid audio session id
    * @param sessionMode mode in which this agm session is to be opened
    * @return handle of opened session
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    long ipc_agm_session_open(in int sessionId, in AgmSessionMode sessionMode);

    /**
    * Start the session.
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_start(in long handle);

    /**
    * Stop the session, session must be in started/paused state before stopping.
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_stop(in long handle);

    /**
    * suspend the session. session must be in started state before suspending
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_suspend(in long handle);

    /**
    * Pause the session, session must be in started state before pausing.
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_pause(in long handle);

    /**
    * Resume the session. session must be in paused state before resuming.
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_resume(in long handle);

    /**
    * prepare the session.
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_prepare(in long handle);

    /**
    * send eos of the session
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_eos(in long handle);

    /**
    * flush the session, session must be in pause state before flushing.
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_flush(in long handle);

    /**
    * Close the session.
    * @param handle valid session handle obtained from ipc_agm_session_open
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_close(in long handle);

    /**
    * Get shared memory MmapBufInfo of a given session
    * @param sessionId valid audio session id
    * @param flag determine data buf/pos buf
    * @return MmapBufInfo shared memory in form of MmapBufInfo
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    MmapBufInfo ipc_agm_session_get_buf_info(in int sessionId, in int flag);

    /**
    * Get Get parameters of the modules of a given session
    * @param sessionId valid audio session id
    * @param buffer payload for get parameter
    * @return byte [] with result of get parameters
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    byte[] ipc_agm_session_get_params(in int sessionId , in byte[] buffer);

    /**
    * Read data buffers from opened agm session
    * @param handle session handle returned from agm_session_open
    * @param count number of bytes requested to fill into
    * @return byte [] buffer read from agm, use its size to get numbers of bytes read,
    * could be different requested count
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    byte[] ipc_agm_session_read(in long handle, in int count);

    parcelable AgmReadWithMetadataReturn {
        AgmBuff buffer;
        int capturesSize;
    }

    /**
    * Read data buffers with metadata to session
    * @param handle session handle returned from agm_session_open
    * @param buffer AgmBuffer containing metadata info
    * @param capturedSize Actual number of bytes that were captured
    * @return AgmReadWithMetadataReturn containing buffer read and captureSize
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    AgmReadWithMetadataReturn ipc_agm_session_read_with_metadata(in long handle,
                                                         in AgmBuff buffer, in int capturedSize);

    /**
    * Register/deregister a callback to listen to various events 
    * @param callback IAGMCallback instance to be registered, on event from lower layers,
    * IAGMCallback's respective methods will be notified
    * @param sessionId valid audio session id
    * @param eventType event type to be monitored
    * @param register number of bytes requested to fill into
    * @param clientData client data
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_register_callback(in IAGMCallback callback, in int sessionId,
                                     in int eventType, in boolean register, in long clientData);

    /**
    * Register for events from Modules. Not needed for data path events.
    * @param sessionId valid audio session id
    * @param eventConfig event specific configuration
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_register_for_events(in int sessionId,
                                         in AgmEventRegistrationConfig eventConfig);

    /**
    * Set Session config
    * @param handle session handle returned from agm_session_open
    * @param sessionConfig valid stream configuration of the session
    * @param mediaConfig valid media configuration of the session
    * @param bufferConfig buffer configuration for the session, null if hostless
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_set_config(in long handle, in AgmSessionConfig sessionConfig,
        in AgmMediaConfig mediaConfig, in AgmBufferConfig bufferConfig);

    /**
    * Set echo reference on capture session
    * @param sessionId valid audio session id
    * @param aifId aifId on RX path.
    * @param state flag to indicate to enable(true) or disable(false) echo reference
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_set_ec_ref(in int sessionId, in int aifId, in boolean state);

    /**
    * Set loopback between capture and playback sessions
    * @param captureSessionId a non zero capture session id
    * @param playbackSessionId playback session id
    * @param state flag to indicate to enable(true) or disable(false) loopback
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_set_loopback(in int captureSessionId, in int playbackSessionId,
        in boolean state);

    /**
    * Set metadata for the session
    * @param sessionId valid audio session id
    * @param metadata valid metadata for the session.
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
   void ipc_agm_session_set_metadata(in int sessionId, in byte[] metadata);

    /**
    * set config for non tunnel mode (rx and tx path)
    * @param handle session handle returned from agm_session_open
    * @param sessionConfig valid stream configuration of the session
    * @param inMediaConfig valid media configuration of the input data
    * @param outMediaConfig valid media configuration of the output data
    * @param inBufferConfig valid buffer configuration of the input data
    * @param outBufferConfig valid buffer configuration of the output data
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_set_non_tunnel_mode_config(in long handle,
        in AgmSessionConfig sessionConfig, in AgmMediaConfig inMediaConfig,
        in AgmMediaConfig outMediaConfig, in AgmBufferConfig inBufferConfig,
        in AgmBufferConfig outBufferConfig);

    /**
    * Set parameters for modules in stream
    * @param sessionId valid audio session id
    * @param payload payload for set parameters
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_set_params(in int sessionId, in byte[] payload);

    /**
    * Write data buffers to the session
    * @param handle session handle returned from agm_session_open
    * @param buff buffer containing the data to be written
    * @return number of bytes written
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    int ipc_agm_session_write(in long handle, in byte[] buff);

    /**
    * Write buffers containing codec params to session on datapath
    * @param sessionId valid audio session id
    * @param buff AgmBuff where data will be copied from
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_write_datapath_params(in int sessionId, in AgmBuff buff);

    /**
    * Write data buffers with metadata to session
    * @param handle session handle returned from agm_session_open
    * @param buff AgmBuff where data will be copied fromo
    * @return number of bytes written
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    int ipc_agm_session_write_with_metadata(in long handle, in AgmBuff buff);

    /**
    * Set media configuration for a group audio interface.
    * @param groupId valid group id
    * @param config valid media configuration for the audio interface
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_aif_group_set_media_config(in int groupId, in AgmGroupMediaConfig config);

    /**
    * Set media configuration for an audio interface.
    * @param aifId Valid audio interface id
    * @param config valid media configuration for the audio interface
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_aif_set_media_config(in int aifId, in AgmMediaConfig config);

    /**
    * Set metadata for an audio interface.
    * @param aifId Valid audio interface id
    * @param metadata valid metadata for the audio interface
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_aif_set_metadata(in int aifId, in byte[] metadata);

    /**
    * Set parameters for modules in audio interface
    * @param aifId Valid audio interface id
    * @param payload parameter payload
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_aif_set_params(in int aifId, in byte[] payload);

    /**
    * connect/disconnect the audio interface.
    * @param sessionId valid audio session id
    * @param aifId Valid audio interface id
    * @param state connect or disconnect AIF to Session
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_aif_connect(in int sessionId, in int aifId, in boolean state);

    /**
    * Read the module tag info for audio interface.
    * @param sessionId valid audio session id
    * @param aifId Valid audio interface id
    * @param size if the value of size is zero, AGM will update required module
    * info list of a given graph. if size equal or greater than the required size,
    * AGM will copy the module info.
    * @return byte [] payload containing tag module info list in the graph
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    byte[] ipc_agm_session_aif_get_tag_module_info(in int sessionId, in int aifId, in int size);

    /**
    * Set calibration for modules in b/w stream and audio interface
    * @param sessionId valid audio session id
    * @param aifId Valid audio interface id
    * @param calibration key vector
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_aif_set_cal(in int sessionId, in int aifId,
        in AgmCalConfig calConfig);

    /**
    * Set metadata for the session, audio interface pair.
    * @param sessionId valid audio session id
    * @param aifId Valid audio interface id
    * @param metadata valid metadata for the session and audio interface
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_aif_set_metadata(in int sessionId, in int aifId, in byte[] metadata);

    /**
    * Set parameters for modules in b/w stream and audio interface.
    * @param sessionId valid audio session id
    * @param aifId Valid audio interface id
    * @param payload payload containing parameters
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_session_aif_set_params(in int sessionId, in int aifId, in byte[] payload);

    /**
    * Get list of AIF info objects
    * @param numAifInfo number of aif info items in the list.
    * if numAifInfo value is listed as zero, AGM will update numAifInfo with
    * the number of aif info items in AGM.
    * if numAifInfo is greater than zero,
    * AGM will copy client specified numAifInfo of items into out result.
    * @return AifInfo[] vector of AifInfo objects
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    AifInfo[] ipc_agm_get_aif_info_list(in int numAifInfo);

    /**
    * Get timestamp of last read buffer.
    * @param sessionId valid audio session id
    * @return valid timestamp
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    long ipc_agm_get_buffer_timestamp(in int sessiondId);

    /**
    * Get list of group AIF objects.
    * @param numberOfGroups valid audio session id
    * number of group aif items in the list.
    * if numberOfGroups value is listed as zero, AGM will update numberOfGroups with
    * the number of group aif items in AGM.
    * if numberOfGroups is greater than zero,
    * AGM will copy client specified numberOfGroups of items into aif_list.
    * @return AifInfo[] vector of AifInfo objects
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    AifInfo[] ipc_agm_get_group_aif_info_list(in int numberOfGroups);

    /**
    * Get count of Buffer processed by h/w
    * @param handle session handle returned from agm_session_open
    * @param direction indicates whether to return the write or read buffer count
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_get_hw_processed_buff_cnt(in long handle, in Direction direction);

    /**
    * Get parameters for modules at acdb without session
    * @param payload payload with tag and calibration 
    * @return byte [] return the parameters for given payload
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    byte[] ipc_agm_get_params_from_acdb_tunnel(in byte[] payload);

    /**
    * get timestamp of the session.
    * @param handle session handle returned from agm_session_open
    * @return valid timestamp if the operation is successful
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    long ipc_agm_get_session_time(in long handle);

    /**
    * flush the session. session must be in pause state before flushing.
    * @param sessiondId valid session id.
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_sessionid_flush(in int sessiondId);

    /**
    * set gapless metadata of the session.
    * @param handle session handle returned from agm_session_open
    * @param type Silence Type (Initial or Trailing)
    * @param silence Initial/Trailing silence samples to be removed
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_set_gapless_session_metadata(in long handle, in AgmGaplessSilenceType type, in int silence);

    /**
    * Set parameters for modules at acdb without session
    * @param payload payload with tag and calibration data
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_set_params_to_acdb_tunnel(in byte[] payload);

    /**
    * Set parameters for modules in b/w stream and audio interface
    * @param sessionId valid audio session id
    * @param aifId Valid audio interface id
    * @param tagConfig tag config structure with tag id and tag key vector
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_set_params_with_tag(in int sessiondId, in int aifId,
        in AgmTagConfig tagConfig);

    /**
    * Set parameters for modules in b/w stream and audio interface
    * @param sessionId valid audio session id
    * @param aifId Valid audio interface id
    * @param payload payload with tag and calibration data
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_set_params_with_tag_to_acdb(in int sessiondId, in int aifId, in byte[] payload);

    /**
    * Dump AGM information based on client
    * @param dumpInfo dump info
    * @throws ServiceSpecificException with one of the values defined in Status.aidl
    * These exceptions are used to preserve the linux error codes over AIDL.
    * check converstion details at: aidlconverter/inc/agm/BinderStatus.h
    */
    void ipc_agm_dump(in AgmDumpInfo dumpInfo);
}
