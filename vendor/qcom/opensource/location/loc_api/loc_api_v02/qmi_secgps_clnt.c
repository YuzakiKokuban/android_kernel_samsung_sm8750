/******************************************************************************
  ---------------------------------------------------------------------------
  Copyright (c) 2011 QUALCOMM Incorporated. All Rights Reserved.
  QUALCOMM Proprietary and Confidential.
  ---------------------------------------------------------------------------
*******************************************************************************/
#define LOG_TAG "qmi_secgps_clnt"
#include <utils/Log.h>
#include <stdbool.h>

#include "qmi_secgps_clnt.h"

static qmi_client_type clnt, notifier;

secgps_set_supported_agps_mode_resp_msg_v01 agps_mode_resp;
secgps_set_xtra_enable_resp_msg_v01 xtra_enable_resp;
secgps_set_glonass_enable_resp_msg_v01 glonass_enable_resp;
secgps_set_cert_type_resp_msg_v01 cert_type_resp;
secgps_set_spirent_type_resp_msg_v01 spirent_type_resp;
secgps_ftm_resp_msg_v01 ftm_resp;
secgps_inject_ni_message_resp_msg_v01 inject_ni_message_resp;
// ++ NS-FLP
secgps_change_engine_only_mode_resp_msg_v01 change_engine_only_mode_resp;
// -- NS-FLP
secgps_agnss_config_message_resp_msg_v01 agnss_config_message_resp;

/*=============================================================================
  CALLBACK FUNCTION secgps_ind_cb
=============================================================================*/
/*!
@brief
  This callback function is called by the QCCI infrastructure when
  infrastructure receives an indication for this client

@param[in]   user_handle         Opaque handle used by the infrastructure to
                                        identify different services.

@param[in]   msg_id              Message ID of the indication

@param[in]  ind_buf              Buffer holding the encoded indication

@param[in]  ind_buf_len          Length of the encoded indication

@param[in]  ind_cb_data          Cookie value supplied by the client during registration

*/
/*=========================================================================*/
void secgps_ind_cb
(
 qmi_client_type                user_handle,
 unsigned int                   msg_id,
 void                           *ind_buf,
 unsigned int                   ind_buf_len,
 void                           *ind_cb_data
)
{
    ALOGV("SECGPS: Indication: msg_id=0x%x", msg_id);

    int rc;
    secgps_ftm_ind_msg_v01 *pData = NULL;
    secgps_agnss_config_message_ind_msg_v01 *secData = NULL;
    FILE* fp = NULL;

    switch(msg_id)
    {
        case QMI_SECGPS_FTM_IND_V01:
        {
            ALOGD("SECGPS: QMI_SEC_FTM_IND_V01 received");

            if((ind_buf != NULL) && (ind_buf_len != 0))
            {
                pData = (secgps_ftm_ind_msg_v01*)malloc(sizeof(secgps_ftm_ind_msg_v01));

                if(NULL == pData)
                {
                    ALOGD("secgps_ind_cb: memory allocation failed\n");
                    return;
                }

                memset(pData, 0, sizeof(secgps_ftm_ind_msg_v01));

                // decode the indication
                rc = qmi_client_message_decode(
                         user_handle,
                         QMI_IDL_INDICATION,
                         msg_id,
                         ind_buf,
                         ind_buf_len,
                         (void*)pData,
                         sizeof(secgps_ftm_ind_msg_v01));

                if( rc == QMI_NO_ERR )
                {
                    int system = pData->system_type;
                    int resp = pData->resp;
                    int freq = pData->freq;
                    int cno = pData->cno;

                    ALOGD("SECGPS: QMI_SEC_FTM_IND_V01 system_type = %d, resp = %d, freq = %d cno = %d",system, resp, freq, cno);

                    fp = fopen("/data/vendor/gps/sv_cno.info","w");

                    if(fp == NULL)
                    {
                        ALOGD("SECGPS: QMI_SEC_FTM_IND_V01 file open failed");
                        free(pData);
                        return;
                    }
                    fprintf(fp, "%d\n", cno);
                    fclose(fp);
                }
                else
                {
                    ALOGD("SECGPS: qmi_client_message_decode failed rc = %d",rc);
                }
                free(pData);
            }
        }
        break;

        case QMI_SECGPS_AGNSS_CONFIG_MESSAGE_IND_V01:
        {
            ALOGD("SECGPS: QMI_SECGPS_AGNSS_CONFIG_MESSAGE_IND_V01 received");

            if((ind_buf != NULL) && (ind_buf_len != 0))
            {
                secData = (secgps_agnss_config_message_ind_msg_v01*)malloc(sizeof(secgps_agnss_config_message_ind_msg_v01));

                if(NULL == secData)
                {
                    ALOGD("secgps_ind_cb: memory allocation failed\n");
                    return;
                }

                memset(secData, 0, sizeof(secgps_agnss_config_message_ind_msg_v01));

                // decode the indication
                rc = qmi_client_message_decode(
                         user_handle,
                         QMI_IDL_INDICATION,
                         msg_id,
                         ind_buf,
                         ind_buf_len,
                         (void*)secData,
                         sizeof(secgps_agnss_config_message_ind_msg_v01));

                if( rc == QMI_NO_ERR )
                {
                  if(secData->agnssConfigMessage_len > 0 && secData->agnssConfigMessage_len < SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE)
                    handleAgnssConfigIndMsg(secData->agnssConfigMessage,secData->agnssConfigMessage_len);
                }
                else
                {
                    ALOGD("SECGPS: qmi_client_message_decode failed rc = %d",rc);
                }
                free(secData);
            }
        }
        
        default:
        {
        }
    }
}

void secgps_set_agps_mode(int mode){
   int rc;
   qmi_txn_handle txn;
   secgps_set_supported_agps_mode_req_msg_v01 req;

   memset(&agps_mode_resp, 0, sizeof(agps_mode_resp));
   /* Set the value of the basic secgps request */

   req.agps_mode = mode;
   ALOGI("SECGPS: Set AGPS Mode : %d\n", mode);

   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_SET_SUPPORTED_AGPS_MODE_REQ_V01, &req, sizeof(req),
               &agps_mode_resp, sizeof(agps_mode_resp),SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != 0){
      ALOGE("SECGPS: send a qmi message to secgps_server error: %d\n",rc);
   }else{
      ALOGD("SECGPS: send a qmi message to secgps_server OK");
   }
}

void secgps_set_xtra_enable(int enable){
   int rc;
   qmi_txn_handle txn;
   secgps_set_xtra_enable_req_msg_v01 req;

   memset(&xtra_enable_resp, 0, sizeof(xtra_enable_resp));
   /* Set the value of the basic secgps request */
   if(enable == 0){
      req.enable = 0;
   }else{
      req.enable = 1;
   }

   ALOGI("SECGPS: Set XTRA enable : %d\n", enable);

   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_SET_XTRA_ENABLE_REQ_V01, &req, sizeof(req),
               &xtra_enable_resp, sizeof(xtra_enable_resp),SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != 0){
      ALOGE("SECGPS: send a qmi message to secgps_server error: %d\n",rc);
   }else{
      ALOGD("SECGPS: send a qmi message to secgps_server OK");
   }
}

void secgps_set_gnss_rf_config(int gnss_cfg){
   int rc;
   qmi_txn_handle txn;
   secgps_set_glonass_enable_req_msg_v01 req;

   memset(&glonass_enable_resp, 0, sizeof(glonass_enable_resp));
   /* Set the value of the basic secgps request */
   req.enable = (uint8_t) gnss_cfg;

   ALOGI("SECGPS: Set GNSS RF CONFIG : %d\n", gnss_cfg);

   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_SET_GLONASS_ENABLE_REQ_V01, &req, sizeof(req),
               &glonass_enable_resp, sizeof(glonass_enable_resp),SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != 0){
      ALOGE("SECGPS: send a qmi message to secgps_server error: %d\n",rc);
   }else{
      ALOGD("SECGPS: send a qmi message to secgps_server OK");
   }
}

void secgps_set_spirent_type(int spirent_type){
   int rc;
   qmi_txn_handle txn;
   secgps_set_spirent_type_req_msg_v01 req;

   memset(&spirent_type_resp, 0, sizeof(spirent_type_resp));
   /* Set the value of the basic secgps request */
   req.spirent_type = spirent_type;

   ALOGD("SECGPS: Set SPIRENT TYPE %d\n", spirent_type);

   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_SET_SPIRENT_TYPE_REQ_V01, &req, sizeof(req),
               &spirent_type_resp, sizeof(spirent_type_resp),SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != 0){
      ALOGE("SECGPS: send a qmi message to secgps_server error: %d\n",rc);
   }else{
      ALOGD("SECGPS: send a qmi message to secgps_server OK");
   }
}

//In case of models based on WIFI, RIL layer doesn't exit.
//Therefore, something is needed to play a role on behalf of RIL, which passes FTM AT command to GPS engine.
void secgps_operation_ftm_mode(int action, int system_type, int freq)
{
   int rc;
   qmi_txn_handle txn;
   secgps_ftm_req_msg_v01 req;

   memset(&ftm_resp, 0, sizeof(ftm_resp));
   req.action = action;
   req.system_type = system_type;
   req.channel_mode = FTM_SINGLE_CHANNEL_MODE; //multiple channel is not supported by QC.
   req.freq = freq;

   ALOGD("SECGPS:secgps_operation_ftm_mode action = %d, system = %d, ch_mode = %d, frq = %d\n",req.action, req.system_type, req.channel_mode, req.freq);

   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_FTM_REQ_V01, &req, sizeof(req),
               &ftm_resp, sizeof(ftm_resp), SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != QMI_NO_ERR)
   {
      ALOGE("SECGPS: send a qmi message to secgps_server error: %d, resp = %d\n",rc, ftm_resp.resp);
   }
   else
   {
      ALOGD("SECGPS: send a qmi message to secgps_server OK");

      if(ftm_resp.resp == SECGPS_FTM_RESP_OK)
      {
         ALOGD("SECGPS: ftm response Success");
      }
      else
      {
         ALOGD("SECGPS: ftm response Fail");
      }
   }
}

// ++ NS-FLP
void secgps_change_engine_only_mode(uint8_t enabled) {
   int rc;
   qmi_txn_handle txn;
   secgps_change_engine_only_mode_req_msg_v01 req;

   memset(&change_engine_only_mode_resp, 0, sizeof(change_engine_only_mode_resp));

   req.change_engine_only_mode = enabled;
   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_CHANGE_ENGINE_ONLY_MODE_REQ_V01, &req, sizeof(req),
               &change_engine_only_mode_resp, sizeof(change_engine_only_mode_resp), SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != 0){
      ALOGE("SECGPS: send a change_engine_only_mode_resp to secgps_server error: %d\n", rc);
   }else{
      ALOGD("SECGPS: send a change_engine_only_mode_resp to secgps_server OK");
   }
}
// -- NS-FLP

void secgps_inject_ni_message(uint8_t* message, uint32_t length, int8_t sub_id){
   int rc;
   qmi_txn_handle txn;
   secgps_inject_ni_message_req_msg_v01 req;

   memset(&inject_ni_message_resp, 0, sizeof(inject_ni_message_resp));
   /* Set the value of the basic secgps request */
   req.injectedNIMessageType = 0;
   req.injectedNIMessage_len= length;
   if (sub_id >= 0) {
       req.subid_valid = 1;
   } else {
       req.subid_valid = 0;
   }
   req.sub_id = sub_id;

   memcpy(req.injectedNIMessage, message, length);

   ALOGI("SECGPS: Inject NI Message sub_id(%d)",sub_id);

   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_INJECT_NI_MESSAGE_REQ_V01, &req, sizeof(req),
               &inject_ni_message_resp, sizeof(inject_ni_message_resp),SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != 0){
      ALOGE("SECGPS: send a qmi message to secgps_server error: %d\n",rc);
   }else{
      ALOGD("SECGPS: send a qmi message to secgps_server OK");
   }
}

void secgps_agnss_config_message(uint8_t* message, uint32_t length){
   int rc;
   qmi_txn_handle txn;
   secgps_agnss_config_message_req_msg_v01 req;

   memset(&agnss_config_message_resp, 0, sizeof(agnss_config_message_resp));
   /* Set the value of the basic secgps request */
   req.agnssConfigMessageType = 0;
   req.agnssConfigMessage_len= length;

   memcpy(req.agnssConfigMessage, message, length);

   ALOGI("SECGPS: AGNSS Config Message");

   rc = qmi_client_send_msg_sync(clnt, QMI_SECGPS_AGNSS_CONFIG_MESSAGE_REQ_V01, &req, sizeof(req),
               &agnss_config_message_resp, sizeof(agnss_config_message_resp),SECGPS_SYNC_REQUEST_TIMEOUT);

   if (rc != 0){
      ALOGE("SECGPS: send a qmi message to secgps_server error: %d\n",rc);
   }else{
      ALOGD("SECGPS: send a qmi message to secgps_server OK");
   }
}

int qmi_secgps_client_init()
{
   // qmi_txn_handle txn;
   uint32_t num_services, num_entries=0;
   int rc,service_connect;

   qmi_cci_os_signal_type os_params;
   qmi_service_info info[5];
   uint32_t port = 10001;
   uint32_t timeout = 5000; // 5 seconds
   bool notifierInitFlag = false;

   ALOGD("qmi_secgps_client_init()");

   if(clnt != NULL){
      ALOGE("SECGPS: client is not null. close it first");
      qmi_secgps_client_deinit();
   }

   /* Get the service object for the secgps API */
   qmi_idl_service_object_type secgps_service_object = secgps_get_service_object_v01();


   /* Verify that secgps_get_service_object did not return NULL */
   if (secgps_service_object == NULL)
   {
      ALOGE("SECGPS: secgps_get_service_object failed, verify qmi_secgps_api_v01.h and .c match.\n");
      return -1;
   }

   rc = qmi_client_notifier_init(secgps_service_object, &os_params, &notifier);
   notifierInitFlag = (NULL != notifier);
   if(rc != QMI_NO_ERR)
   {
      ALOGE("SECGPS: qmi_client_notifier_init failed, rc=%d", rc);
      rc = -1;
      goto cleanup;
   }

   QMI_CCI_OS_SIGNAL_WAIT(&os_params, timeout);

   if(QMI_CCI_OS_SIGNAL_TIMED_OUT(&os_params))
   {
      ALOGE("SECGPS: timed out waiting for secgps service");
      rc = -1;
      goto cleanup;
   }
   else
   {
      rc = qmi_client_get_service_list( secgps_service_object, NULL, NULL, &num_services);
      ALOGD("SECGPS: qmi_client_get_service_list() returned %d num_services = %d\n", rc, num_services);
      if(rc != QMI_NO_ERR)
      {
         ALOGE("SECGPS: qmi_client_get_service_list() failed even though service is up, rc=%d", rc);
         rc = -1;
         goto cleanup;
      }
   }

   rc = qmi_client_get_service_list( secgps_service_object, info, &num_entries, &num_services);
   ALOGD("SECGPS: qmi_client_get_service_list() returned %d num_entries = %d num_services = %d\n", rc, num_entries, num_services);

   num_entries = num_services;
   /* The server has come up, store the information in info variable */
   rc = qmi_client_get_service_list( secgps_service_object, info, &num_entries, &num_services);
   ALOGD("SECGPS: qmi_client_get_service_list() returned %d num_entries = %d num_services = %d\n", rc, num_entries, num_services);

   if(rc != QMI_NO_ERR)
   {
      ALOGE("SECGPS: qmi_client_get_service_list Error %d", rc);
      rc = -1;
      goto cleanup;
   }

   rc = qmi_client_init(&info[0], secgps_service_object, secgps_ind_cb, NULL, NULL, &clnt);
   if(rc != QMI_NO_ERR)
   {
      ALOGE("SECGPS: qmi_client_init failed");
      rc = -1;
      goto cleanup;
   }
  /* release the notifier handle */

cleanup:
  if(true == notifierInitFlag)
  {
    qmi_client_release(notifier);
  }
   return rc;
}

int qmi_secgps_client_deinit(){
   int rc;
   rc = qmi_client_release(clnt);
   ALOGD("SECGPS: qmi_client_release returned %d\n", rc);
   return 0;
}
