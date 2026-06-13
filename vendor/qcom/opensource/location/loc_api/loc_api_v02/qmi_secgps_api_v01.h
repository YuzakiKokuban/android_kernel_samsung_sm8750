#ifndef SECGPS_SERVICE_01_H
#define SECGPS_SERVICE_01_H
/**
  @file qmi_secgps_api_v01.h
  
  @brief This is the public header file which defines the secgps service Data structures.

  This header file defines the types and structures that were defined in 
  secgps. It contains the constant values defined, enums, structures,
  messages, and service message IDs (in that order) Structures that were 
  defined in the IDL as messages contain mandatory elements, optional 
  elements, a combination of mandatory and optional elements (mandatory 
  always come before optionals in the structure), or nothing (null message)
   
  An optional element in a message is preceded by a uint8_t value that must be
  set to true if the element is going to be included. When decoding a received
  message, the uint8_t values will be set to true or false by the decode
  routine, and should be checked before accessing the values that they
  correspond to. 
   
  Variable sized arrays are defined as static sized arrays with an unsigned
  integer (32 bit) preceding it that must be set to the number of elements
  in the array that are valid. For Example:
   
  uint32_t test_opaque_len;
  uint8_t test_opaque[16];
   
  If only 4 elements are added to test_opaque[] then test_opaque_len must be
  set to 4 before sending the message.  When decoding, the _len value is set 
  by the decode routine and should be checked so that the correct number of 
  elements in the array will be accessed. 

*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  

  
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 6.13 
   It was generated on: Thu Oct 27 2022 (Spin 0)
   From IDL File: qmi_secgps_api_v01.idl */

/** @defgroup secgps_qmi_consts Constant values defined in the IDL */
/** @defgroup secgps_qmi_msg_ids Constant values for QMI message IDs */
/** @defgroup secgps_qmi_enums Enumerated types used in QMI messages */
/** @defgroup secgps_qmi_messages Structures sent as QMI messages */
/** @defgroup secgps_qmi_aggregates Aggregate types used in QMI messages */
/** @defgroup secgps_qmi_accessor Accessor for QMI service object */
/** @defgroup secgps_qmi_version Constant values for versioning information */

#include <stdint.h>
#include "qmi_idl_lib.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup secgps_qmi_version 
    @{ 
  */ 
/** Major Version Number of the IDL used to generate this file */
#define SECGPS_V01_IDL_MAJOR_VERS 0x01
/** Revision Number of the IDL used to generate this file */
#define SECGPS_V01_IDL_MINOR_VERS 0x01
/** Major Version Number of the qmi_idl_compiler used to generate this file */
#define SECGPS_V01_IDL_TOOL_VERS 0x06
/** Maximum Defined Message ID */
#define SECGPS_V01_MAX_MESSAGE_ID 0x000F
/**
    @}
  */


/** @addtogroup secgps_qmi_consts 
    @{ 
  */

/** 
 Maximum size for a data TLV */
#define SECGPS_MAX_DATA_SIZE_V01 8192

/**  Maximum length of a client or service name */
#define SECGPS_MAX_NAME_SIZE_V01 255
#define SECGPS_MAX_SUPL_NI_MSG_SIZE_V01 1024
#define SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE_V01 2048
/**
    @}
  */

/** @addtogroup secgps_qmi_aggregates
    @{
  */
typedef struct {

  uint32_t name_len;  /**< Must be set to # of elements in name */
  char name[SECGPS_MAX_NAME_SIZE_V01];
}secgps_name_type_v01;  /* Type */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  char ping[4];
  /**<   Simple 'secgps' request  */

  /* Optional */
  uint8_t client_name_valid;  /**< Must be set to true if client_name is being passed */
  secgps_name_type_v01 client_name;
  /**<   Optional name to identify clients  */
}secgps_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  char pong[4];
  /**<   Simple 'pong' response  */

  /* Mandatory */
  int8_t resp;

  /* Optional */
  uint8_t service_name_valid;  /**< Must be set to true if service_name is being passed */
  secgps_name_type_v01 service_name;
  /**<   Optional name to identify service  */
}secgps_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  char indication[5];
  /**<   Simple 'hello' indication  */

  /* Optional */
  uint8_t service_name_valid;  /**< Must be set to true if service_name is being passed */
  secgps_name_type_v01 service_name;
  /**<   Optional name to identify service  */
}secgps_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint32_t data_len;  /**< Must be set to # of elements in data */
  uint8_t data[SECGPS_MAX_DATA_SIZE_V01];
  /**<   Variable sized data request  */

  /* Optional */
  uint8_t client_name_valid;  /**< Must be set to true if client_name is being passed */
  secgps_name_type_v01 client_name;
  /**<   Optional name to identify clients  */
}secgps_data_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint32_t data_len;  /**< Must be set to # of elements in data */
  uint8_t data[SECGPS_MAX_DATA_SIZE_V01];
  /**<   Variable sized data response  */

  /* Mandatory */
  int8_t resp;

  /* Optional */
  uint8_t service_name_valid;  /**< Must be set to true if service_name is being passed */
  secgps_name_type_v01 service_name;
  /**<   Optional name to identify service  */
}secgps_data_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Optional */
  uint8_t num_inds_valid;  /**< Must be set to true if num_inds is being passed */
  uint16_t num_inds;
  /**<   Number of indications to send */

  /* Optional */
  uint8_t ind_size_valid;  /**< Must be set to true if ind_size is being passed */
  uint16_t ind_size;
  /**<   Max value 65000 */

  /* Optional */
  uint8_t ind_delay_valid;  /**< Must be set to true if ind_delay is being passed */
  uint16_t ind_delay;

  /* Optional */
  uint8_t num_reqs_valid;  /**< Must be set to true if num_reqs is being passed */
  uint16_t num_reqs;

  /* Optional */
  uint8_t client_name_valid;  /**< Must be set to true if client_name is being passed */
  secgps_name_type_v01 client_name;
  /**<   Optional name to identify clients  */
}secgps_data_ind_reg_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
  /**<   Standard response type.  */

  /* Optional */
  uint8_t service_name_valid;  /**< Must be set to true if service_name is being passed */
  secgps_name_type_v01 service_name;
  /**<   Optional name to identify service  */
}secgps_data_ind_reg_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint32_t data_len;  /**< Must be set to # of elements in data */
  uint8_t data[SECGPS_MAX_DATA_SIZE_V01];
  /**<   Variable sized data indication  */

  /* Optional */
  uint8_t service_name_valid;  /**< Must be set to true if service_name is being passed */
  secgps_name_type_v01 service_name;
  /**<   Optional name to identify service  */
}secgps_data_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Optional */
  uint8_t client_name_valid;  /**< Must be set to true if client_name is being passed */
  secgps_name_type_v01 client_name;
  /**<   Optional name to identify clients  */
}secgps_get_service_name_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  secgps_name_type_v01 service_name;
  /**<   Name to identify service  */

  /* Mandatory */
  int8_t resp;
}secgps_get_service_name_resp_msg_v01;  /* Message */
/**
    @}
  */

typedef struct {
  /* This element is a placeholder to prevent the declaration of 
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}secgps_null_req_msg_v01;

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  secgps_name_type_v01 service_name;
  /**<   Name to identify service  */

  /* Mandatory */
  int8_t resp;
}secgps_null_resp_msg_v01;  /* Message */
/**
    @}
  */

typedef struct {
  /* This element is a placeholder to prevent the declaration of 
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}secgps_null_ind_msg_v01;

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint32_t agps_mode;
}secgps_set_supported_agps_mode_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
}secgps_set_supported_agps_mode_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t enable;
}secgps_set_xtra_enable_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
}secgps_set_xtra_enable_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t enable;
}secgps_set_glonass_enable_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
}secgps_set_glonass_enable_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t cert_type;
}secgps_set_cert_type_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
}secgps_set_cert_type_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t action;

  /* Mandatory */
  uint8_t system_type;

  /* Mandatory */
  uint8_t channel_mode;

  /* Mandatory */
  int8_t freq;
}secgps_ftm_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t resp;
}secgps_ftm_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t system_type;

  /* Mandatory */
  int8_t freq;

  /* Mandatory */
  uint8_t cno;

  /* Mandatory */
  uint8_t resp;
}secgps_ftm_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t change_engine_only_mode;
}secgps_change_engine_only_mode_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t resp;
}secgps_change_engine_only_mode_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t spirent_type;
}secgps_set_spirent_type_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
}secgps_set_spirent_type_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t injectedNIMessageType;

  /* Mandatory */
  uint32_t injectedNIMessage_len;  /**< Must be set to # of elements in injectedNIMessage */
  uint8_t injectedNIMessage[SECGPS_MAX_SUPL_NI_MSG_SIZE_V01];

  /* Mandatory */
  uint8_t subid_valid;

  /* Mandatory */
  int8_t sub_id;
}secgps_inject_ni_message_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
}secgps_inject_ni_message_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint8_t agnssConfigMessageType;

  /* Mandatory */
  uint32_t agnssConfigMessage_len;  /**< Must be set to # of elements in agnssConfigMessage */
  uint8_t agnssConfigMessage[SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE_V01];
}secgps_agnss_config_message_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  int8_t resp;
}secgps_agnss_config_message_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup secgps_qmi_messages
    @{
  */
/**  Message;  */
typedef struct {

  /* Mandatory */
  uint32_t agnssConfigMessage_len;  /**< Must be set to # of elements in agnssConfigMessage */
  uint8_t agnssConfigMessage[SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE_V01];
}secgps_agnss_config_message_ind_msg_v01;  /* Message */
/**
    @}
  */

/* Conditional compilation tags for message removal */ 

/*Service Message Definition*/
/** @addtogroup secgps_qmi_msg_ids
    @{
  */
#define QMI_SECGPS_REQ_V01 0x0001
#define QMI_SECGPS_RESP_V01 0x0001
#define QMI_SECGPS_IND_V01 0x0001
#define QMI_SECGPS_DATA_REQ_V01 0x0002
#define QMI_SECGPS_DATA_RESP_V01 0x0002
#define QMI_SECGPS_DATA_IND_REG_REQ_V01 0x0003
#define QMI_SECGPS_DATA_IND_REG_RESP_V01 0x0003
#define QMI_SECGPS_DATA_IND_V01 0x0004
#define QMI_SECGPS_GET_SERVICE_NAME_REQ_V01 0x0005
#define QMI_SECGPS_GET_SERVICE_NAME_RESP_V01 0x0005
#define QMI_SECGPS_NULL_REQ_V01 0x0006
#define QMI_SECGPS_NULL_RESP_V01 0x0006
#define QMI_SECGPS_NULL_IND_V01 0x0006
#define QMI_SECGPS_SET_SUPPORTED_AGPS_MODE_REQ_V01 0x0007
#define QMI_SECGPS_SET_SUPPORTED_AGPS_MODE_RESP_V01 0x0007
#define QMI_SECGPS_SET_XTRA_ENABLE_REQ_V01 0x0008
#define QMI_SECGPS_SET_XTRA_ENABLE_RESP_V01 0x0008
#define QMI_SECGPS_SET_GLONASS_ENABLE_REQ_V01 0x0009
#define QMI_SECGPS_SET_GLONASS_ENABLE_RESP_V01 0x0009
#define QMI_SECGPS_SET_CERT_TYPE_REQ_V01 0x000A
#define QMI_SECGPS_SET_CERT_TYPE_RESP_V01 0x000A
#define QMI_SECGPS_FTM_REQ_V01 0x000B
#define QMI_SECGPS_FTM_RESP_V01 0x000B
#define QMI_SECGPS_FTM_IND_V01 0x000B
#define QMI_SECGPS_SET_SPIRENT_TYPE_REQ_V01 0x000C
#define QMI_SECGPS_SET_SPIRENT_TYPE_RESP_V01 0x000C
#define QMI_SECGPS_INJECT_NI_MESSAGE_REQ_V01 0x000D
#define QMI_SECGPS_INJECT_NI_MESSAGE_RESP_V01 0x000D
#define QMI_SECGPS_CHANGE_ENGINE_ONLY_MODE_REQ_V01 0x000E
#define QMI_SECGPS_CHANGE_ENGINE_ONLY_MODE_RESP_V01 0x000E
#define QMI_SECGPS_AGNSS_CONFIG_MESSAGE_REQ_V01 0x000F
#define QMI_SECGPS_AGNSS_CONFIG_MESSAGE_RESP_V01 0x000F
#define QMI_SECGPS_AGNSS_CONFIG_MESSAGE_IND_V01 0x000F
/**
    @}
  */

/* Service Object Accessor */
/** @addtogroup wms_qmi_accessor 
    @{
  */
/** This function is used internally by the autogenerated code.  Clients should use the
   macro secgps_get_service_object_v01( ) that takes in no arguments. */
qmi_idl_service_object_type secgps_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
/** This macro should be used to get the service object */ 
#define secgps_get_service_object_v01( ) \
          secgps_get_service_object_internal_v01( \
            SECGPS_V01_IDL_MAJOR_VERS, SECGPS_V01_IDL_MINOR_VERS, \
            SECGPS_V01_IDL_TOOL_VERS )
/** 
    @} 
  */


#ifdef __cplusplus
}
#endif
#endif

