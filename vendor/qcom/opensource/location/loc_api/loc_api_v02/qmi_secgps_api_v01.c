/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        Q M I _ S E C G P S _ A P I _ V 0 1  . C

GENERAL DESCRIPTION
  This is the file which defines the secgps service Data structures.

  

  
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 2.6
   It was generated on: Mon Aug  6 2018
   From IDL File: qmi_secgps_api_v01.idl */

#include "stdint.h"
#include "qmi_idl_lib_internal.h"
#include "qmi_secgps_api_v01.h"


/*Type Definitions*/
static const uint8_t secgps_name_type_data_v01[] = {
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_name_type_v01, name),
  SECGPS_MAX_NAME_SIZE_V01,
  QMI_IDL_OFFSET8(secgps_name_type_v01, name) - QMI_IDL_OFFSET8(secgps_name_type_v01, name_len),

  QMI_IDL_FLAG_END_VALUE
};

/*Message Definitions*/
static const uint8_t secgps_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_req_msg_v01, ping),
  4,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_req_msg_v01, client_name) - QMI_IDL_OFFSET8(secgps_req_msg_v01, client_name_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_req_msg_v01, client_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_resp_msg_v01, pong),
  4,

  0x02,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_resp_msg_v01, resp),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_resp_msg_v01, service_name) - QMI_IDL_OFFSET8(secgps_resp_msg_v01, service_name_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_resp_msg_v01, service_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_ind_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ind_msg_v01, indication),
  5,

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_ind_msg_v01, service_name) - QMI_IDL_OFFSET8(secgps_ind_msg_v01, service_name_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_ind_msg_v01, service_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_data_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_data_req_msg_v01, data),
  ((SECGPS_MAX_DATA_SIZE_V01) & 0xFF), ((SECGPS_MAX_DATA_SIZE_V01) >> 8),
  QMI_IDL_OFFSET8(secgps_data_req_msg_v01, data) - QMI_IDL_OFFSET8(secgps_data_req_msg_v01, data_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(secgps_data_req_msg_v01, client_name) - QMI_IDL_OFFSET16RELATIVE(secgps_data_req_msg_v01, client_name_valid)),
  0x10,
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(secgps_data_req_msg_v01, client_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_data_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_data_resp_msg_v01, data),
  ((SECGPS_MAX_DATA_SIZE_V01) & 0xFF), ((SECGPS_MAX_DATA_SIZE_V01) >> 8),
  QMI_IDL_OFFSET8(secgps_data_resp_msg_v01, data) - QMI_IDL_OFFSET8(secgps_data_resp_msg_v01, data_len),

  0x02,
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(secgps_data_resp_msg_v01, resp),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(secgps_data_resp_msg_v01, service_name) - QMI_IDL_OFFSET16RELATIVE(secgps_data_resp_msg_v01, service_name_valid)),
  0x10,
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(secgps_data_resp_msg_v01, service_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_data_ind_reg_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, num_inds) - QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, num_inds_valid)),
  0x10,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, num_inds),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, ind_size) - QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, ind_size_valid)),
  0x11,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, ind_size),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, ind_delay) - QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, ind_delay_valid)),
  0x12,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, ind_delay),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, num_reqs) - QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, num_reqs_valid)),
  0x13,
  QMI_IDL_GENERIC_2_BYTE,
  QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, num_reqs),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, client_name) - QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, client_name_valid)),
  0x14,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_data_ind_reg_req_msg_v01, client_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_data_ind_reg_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_data_ind_reg_resp_msg_v01, resp),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_data_ind_reg_resp_msg_v01, service_name) - QMI_IDL_OFFSET8(secgps_data_ind_reg_resp_msg_v01, service_name_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_data_ind_reg_resp_msg_v01, service_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_data_ind_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_data_ind_msg_v01, data),
  ((SECGPS_MAX_DATA_SIZE_V01) & 0xFF), ((SECGPS_MAX_DATA_SIZE_V01) >> 8),
  QMI_IDL_OFFSET8(secgps_data_ind_msg_v01, data) - QMI_IDL_OFFSET8(secgps_data_ind_msg_v01, data_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(secgps_data_ind_msg_v01, service_name) - QMI_IDL_OFFSET16RELATIVE(secgps_data_ind_msg_v01, service_name_valid)),
  0x10,
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET16ARRAY(secgps_data_ind_msg_v01, service_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_get_service_name_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(secgps_get_service_name_req_msg_v01, client_name) - QMI_IDL_OFFSET8(secgps_get_service_name_req_msg_v01, client_name_valid)),
  0x10,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_get_service_name_req_msg_v01, client_name),
  QMI_IDL_TYPE88(0, 0)
};

static const uint8_t secgps_get_service_name_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_get_service_name_resp_msg_v01, service_name),
  QMI_IDL_TYPE88(0, 0),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(secgps_get_service_name_resp_msg_v01, resp)
};

/* 
 * secgps_null_req_msg is empty
 * static const uint8_t secgps_null_req_msg_data_v01[] = {
 * };
 */
  
static const uint8_t secgps_null_resp_msg_data_v01[] = {
  0x01,
  QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(secgps_null_resp_msg_v01, service_name),
  QMI_IDL_TYPE88(0, 0),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(secgps_null_resp_msg_v01, resp)
};

/* 
 * secgps_null_ind_msg is empty
 * static const uint8_t secgps_null_ind_msg_data_v01[] = {
 * };
 */
  
static const uint8_t secgps_set_supported_agps_mode_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(secgps_set_supported_agps_mode_req_msg_v01, agps_mode)
};

static const uint8_t secgps_set_supported_agps_mode_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_supported_agps_mode_resp_msg_v01, resp)
};

static const uint8_t secgps_set_xtra_enable_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_xtra_enable_req_msg_v01, enable)
};

static const uint8_t secgps_set_xtra_enable_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_xtra_enable_resp_msg_v01, resp)
};

static const uint8_t secgps_set_glonass_enable_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_glonass_enable_req_msg_v01, enable)
};

static const uint8_t secgps_set_glonass_enable_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_glonass_enable_resp_msg_v01, resp)
};

static const uint8_t secgps_set_cert_type_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_cert_type_req_msg_v01, cert_type)
};

static const uint8_t secgps_set_cert_type_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_cert_type_resp_msg_v01, resp)
};

static const uint8_t secgps_ftm_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_req_msg_v01, action),

  0x02,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_req_msg_v01, system_type),

  0x03,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_req_msg_v01, channel_mode),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x04,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_req_msg_v01, freq)
};

static const uint8_t secgps_ftm_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_resp_msg_v01, resp)
};

static const uint8_t secgps_ftm_ind_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_ind_msg_v01, system_type),

  0x02,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_ind_msg_v01, freq),

  0x03,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_ind_msg_v01, cno),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x04,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_ftm_ind_msg_v01, resp)
};

static const uint8_t secgps_change_engine_only_mode_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_change_engine_only_mode_req_msg_v01, change_engine_only_mode)
};

static const uint8_t secgps_change_engine_only_mode_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_change_engine_only_mode_resp_msg_v01, resp)
};

static const uint8_t secgps_set_spirent_type_req_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_spirent_type_req_msg_v01, spirent_type)
};

static const uint8_t secgps_set_spirent_type_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_set_spirent_type_resp_msg_v01, resp)
};

static const uint8_t secgps_inject_ni_message_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_inject_ni_message_req_msg_v01, injectedNIMessageType),

  0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_inject_ni_message_req_msg_v01, injectedNIMessage),
  ((SECGPS_MAX_SUPL_NI_MSG_SIZE_V01) & 0xFF), ((SECGPS_MAX_SUPL_NI_MSG_SIZE_V01) >> 8),
  QMI_IDL_OFFSET8(secgps_inject_ni_message_req_msg_v01, injectedNIMessage) - QMI_IDL_OFFSET8(secgps_inject_ni_message_req_msg_v01, injectedNIMessage_len),

  0x03,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(secgps_inject_ni_message_req_msg_v01, subid_valid),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x04,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(secgps_inject_ni_message_req_msg_v01, sub_id)
};

static const uint8_t secgps_inject_ni_message_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_inject_ni_message_resp_msg_v01, resp)
};

static const uint8_t secgps_agnss_config_message_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_agnss_config_message_req_msg_v01, agnssConfigMessageType),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_agnss_config_message_req_msg_v01, agnssConfigMessage),
  ((SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE_V01) & 0xFF), ((SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE_V01) >> 8),
  QMI_IDL_OFFSET8(secgps_agnss_config_message_req_msg_v01, agnssConfigMessage) - QMI_IDL_OFFSET8(secgps_agnss_config_message_req_msg_v01, agnssConfigMessage_len)
};

static const uint8_t secgps_agnss_config_message_resp_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_agnss_config_message_resp_msg_v01, resp)
};

static const uint8_t secgps_agnss_config_message_ind_msg_data_v01[] = {
  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(secgps_agnss_config_message_ind_msg_v01, agnssConfigMessage),
  ((SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE_V01) & 0xFF), ((SECGPS_MAX_AGNSS_CONFIG_MSG_SIZE_V01) >> 8),
  QMI_IDL_OFFSET8(secgps_agnss_config_message_ind_msg_v01, agnssConfigMessage) - QMI_IDL_OFFSET8(secgps_agnss_config_message_ind_msg_v01, agnssConfigMessage_len)
};

/* Type Table */
static const qmi_idl_type_table_entry  secgps_type_table_v01[] = {
  {sizeof(secgps_name_type_v01), secgps_name_type_data_v01}
};

/* Message Table */
static const qmi_idl_message_table_entry secgps_message_table_v01[] = {
  {sizeof(secgps_req_msg_v01), secgps_req_msg_data_v01},
  {sizeof(secgps_resp_msg_v01), secgps_resp_msg_data_v01},
  {sizeof(secgps_ind_msg_v01), secgps_ind_msg_data_v01},
  {sizeof(secgps_data_req_msg_v01), secgps_data_req_msg_data_v01},
  {sizeof(secgps_data_resp_msg_v01), secgps_data_resp_msg_data_v01},
  {sizeof(secgps_data_ind_reg_req_msg_v01), secgps_data_ind_reg_req_msg_data_v01},
  {sizeof(secgps_data_ind_reg_resp_msg_v01), secgps_data_ind_reg_resp_msg_data_v01},
  {sizeof(secgps_data_ind_msg_v01), secgps_data_ind_msg_data_v01},
  {sizeof(secgps_get_service_name_req_msg_v01), secgps_get_service_name_req_msg_data_v01},
  {sizeof(secgps_get_service_name_resp_msg_v01), secgps_get_service_name_resp_msg_data_v01},
  {sizeof(secgps_null_req_msg_v01), 0},
  {sizeof(secgps_null_resp_msg_v01), secgps_null_resp_msg_data_v01},
  {sizeof(secgps_null_ind_msg_v01), 0},
  {sizeof(secgps_set_supported_agps_mode_req_msg_v01), secgps_set_supported_agps_mode_req_msg_data_v01},
  {sizeof(secgps_set_supported_agps_mode_resp_msg_v01), secgps_set_supported_agps_mode_resp_msg_data_v01},
  {sizeof(secgps_set_xtra_enable_req_msg_v01), secgps_set_xtra_enable_req_msg_data_v01},
  {sizeof(secgps_set_xtra_enable_resp_msg_v01), secgps_set_xtra_enable_resp_msg_data_v01},
  {sizeof(secgps_set_glonass_enable_req_msg_v01), secgps_set_glonass_enable_req_msg_data_v01},
  {sizeof(secgps_set_glonass_enable_resp_msg_v01), secgps_set_glonass_enable_resp_msg_data_v01},
  {sizeof(secgps_set_cert_type_req_msg_v01), secgps_set_cert_type_req_msg_data_v01},
  {sizeof(secgps_set_cert_type_resp_msg_v01), secgps_set_cert_type_resp_msg_data_v01},
  {sizeof(secgps_ftm_req_msg_v01), secgps_ftm_req_msg_data_v01},
  {sizeof(secgps_ftm_resp_msg_v01), secgps_ftm_resp_msg_data_v01},
  {sizeof(secgps_ftm_ind_msg_v01), secgps_ftm_ind_msg_data_v01},
  {sizeof(secgps_change_engine_only_mode_req_msg_v01), secgps_change_engine_only_mode_req_msg_data_v01},
  {sizeof(secgps_change_engine_only_mode_resp_msg_v01), secgps_change_engine_only_mode_resp_msg_data_v01},
  {sizeof(secgps_set_spirent_type_req_msg_v01), secgps_set_spirent_type_req_msg_data_v01},
  {sizeof(secgps_set_spirent_type_resp_msg_v01), secgps_set_spirent_type_resp_msg_data_v01},
  {sizeof(secgps_inject_ni_message_req_msg_v01), secgps_inject_ni_message_req_msg_data_v01},
  {sizeof(secgps_inject_ni_message_resp_msg_v01), secgps_inject_ni_message_resp_msg_data_v01},
  {sizeof(secgps_agnss_config_message_req_msg_v01), secgps_agnss_config_message_req_msg_data_v01},
  {sizeof(secgps_agnss_config_message_resp_msg_v01), secgps_agnss_config_message_resp_msg_data_v01},
  {sizeof(secgps_agnss_config_message_ind_msg_v01), secgps_agnss_config_message_ind_msg_data_v01}
};

/* Range Table */
/* Predefine the Type Table Object */
static const qmi_idl_type_table_object secgps_qmi_idl_type_table_object_v01;

/*Referenced Tables Array*/
static const qmi_idl_type_table_object *secgps_qmi_idl_type_table_object_referenced_tables_v01[] =
{&secgps_qmi_idl_type_table_object_v01};

/*Type Table Object*/
static const qmi_idl_type_table_object secgps_qmi_idl_type_table_object_v01 = {
  sizeof(secgps_type_table_v01)/sizeof(qmi_idl_type_table_entry ),
  sizeof(secgps_message_table_v01)/sizeof(qmi_idl_message_table_entry),
  1,
  secgps_type_table_v01,
  secgps_message_table_v01,
  secgps_qmi_idl_type_table_object_referenced_tables_v01,
  NULL
};

/*Arrays of service_message_table_entries for commands, responses and indications*/
static const qmi_idl_service_message_table_entry secgps_service_command_messages_v01[] = {
  {QMI_SECGPS_REQ_V01, QMI_IDL_TYPE16(0, 0), 266},
  {QMI_SECGPS_DATA_REQ_V01, QMI_IDL_TYPE16(0, 3), 8456},
  {QMI_SECGPS_DATA_IND_REG_REQ_V01, QMI_IDL_TYPE16(0, 5), 279},
  {QMI_SECGPS_GET_SERVICE_NAME_REQ_V01, QMI_IDL_TYPE16(0, 8), 259},
  {QMI_SECGPS_NULL_REQ_V01, QMI_IDL_TYPE16(0, 10), 0},
  {QMI_SECGPS_SET_SUPPORTED_AGPS_MODE_REQ_V01, QMI_IDL_TYPE16(0, 13), 7},
  {QMI_SECGPS_SET_XTRA_ENABLE_REQ_V01, QMI_IDL_TYPE16(0, 15), 4},
  {QMI_SECGPS_SET_GLONASS_ENABLE_REQ_V01, QMI_IDL_TYPE16(0, 17), 4},
  {QMI_SECGPS_SET_CERT_TYPE_REQ_V01, QMI_IDL_TYPE16(0, 19), 4},
  {QMI_SECGPS_FTM_REQ_V01, QMI_IDL_TYPE16(0, 21), 16},
  {QMI_SECGPS_SET_SPIRENT_TYPE_REQ_V01, QMI_IDL_TYPE16(0, 26), 4},
  {QMI_SECGPS_INJECT_NI_MESSAGE_REQ_V01, QMI_IDL_TYPE16(0, 28), 1041},
  {QMI_SECGPS_CHANGE_ENGINE_ONLY_MODE_REQ_V01, QMI_IDL_TYPE16(0, 24), 4},
  {QMI_SECGPS_AGNSS_CONFIG_MESSAGE_REQ_V01, QMI_IDL_TYPE16(0, 30), 2057}
};

static const qmi_idl_service_message_table_entry secgps_service_response_messages_v01[] = {
  {QMI_SECGPS_RESP_V01, QMI_IDL_TYPE16(0, 1), 270},
  {QMI_SECGPS_DATA_RESP_V01, QMI_IDL_TYPE16(0, 4), 8460},
  {QMI_SECGPS_DATA_IND_REG_RESP_V01, QMI_IDL_TYPE16(0, 6), 263},
  {QMI_SECGPS_GET_SERVICE_NAME_RESP_V01, QMI_IDL_TYPE16(0, 9), 263},
  {QMI_SECGPS_NULL_RESP_V01, QMI_IDL_TYPE16(0, 11), 263},
  {QMI_SECGPS_SET_SUPPORTED_AGPS_MODE_RESP_V01, QMI_IDL_TYPE16(0, 14), 4},
  {QMI_SECGPS_SET_XTRA_ENABLE_RESP_V01, QMI_IDL_TYPE16(0, 16), 4},
  {QMI_SECGPS_SET_GLONASS_ENABLE_RESP_V01, QMI_IDL_TYPE16(0, 18), 4},
  {QMI_SECGPS_SET_CERT_TYPE_RESP_V01, QMI_IDL_TYPE16(0, 20), 4},
  {QMI_SECGPS_FTM_RESP_V01, QMI_IDL_TYPE16(0, 22), 4},
  {QMI_SECGPS_SET_SPIRENT_TYPE_RESP_V01, QMI_IDL_TYPE16(0, 27), 4},
  {QMI_SECGPS_INJECT_NI_MESSAGE_RESP_V01, QMI_IDL_TYPE16(0, 29), 4},
  {QMI_SECGPS_CHANGE_ENGINE_ONLY_MODE_RESP_V01, QMI_IDL_TYPE16(0, 25), 4},
  {QMI_SECGPS_AGNSS_CONFIG_MESSAGE_RESP_V01, QMI_IDL_TYPE16(0, 31), 4}
};

static const qmi_idl_service_message_table_entry secgps_service_indication_messages_v01[] = {
  {QMI_SECGPS_IND_V01, QMI_IDL_TYPE16(0, 2), 267},
  {QMI_SECGPS_DATA_IND_V01, QMI_IDL_TYPE16(0, 7), 8456},
  {QMI_SECGPS_NULL_IND_V01, QMI_IDL_TYPE16(0, 12), 0},
  {QMI_SECGPS_FTM_IND_V01, QMI_IDL_TYPE16(0, 23), 16},
  {QMI_SECGPS_AGNSS_CONFIG_MESSAGE_IND_V01, QMI_IDL_TYPE16(0, 32), 2053}
};

/*Service Object*/
struct qmi_idl_service_object secgps_qmi_idl_service_object_v01 = {
  0x06,
  0x01,
  0xE8,
  8460,
  { sizeof(secgps_service_command_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(secgps_service_response_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(secgps_service_indication_messages_v01)/sizeof(qmi_idl_service_message_table_entry) },
  { secgps_service_command_messages_v01, secgps_service_response_messages_v01, secgps_service_indication_messages_v01},
  &secgps_qmi_idl_type_table_object_v01,
  0x01,
  NULL
};

/* Service Object Accessor */
qmi_idl_service_object_type secgps_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version ){
  if ( SECGPS_V01_IDL_MAJOR_VERS != idl_maj_version || SECGPS_V01_IDL_MINOR_VERS != idl_min_version 
       || SECGPS_V01_IDL_TOOL_VERS != library_version) 
  {
    return NULL;
  } 
  return (qmi_idl_service_object_type)&secgps_qmi_idl_service_object_v01;
}

