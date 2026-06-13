/*
 * FST module - shared Control interface definitions
 * Copyright (c) 2014, Qualcomm Atheros, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef FST_CTRL_DEFS_H
#define FST_CTRL_DEFS_H

/* Undefined value */
#define FST_CTRL_PVAL_NONE                     "NONE"

/* FST-ATTACH parameters */
#define FST_ATTACH_CMD_PNAME_LLT      "llt"      /* pval = desired LLT */
#define FST_ATTACH_CMD_PNAME_PRIORITY "priority" /* pval = desired priority */

/* FST-MANAGER parameters */
/* FST Session states */
#define FST_CS_PVAL_STATE_INITIAL              "INITIAL"
#define FST_CS_PVAL_STATE_SETUP_COMPLETION     "SETUP_COMPLETION"
#define FST_CS_PVAL_STATE_TRANSITION_DONE      "TRANSITION_DONE"
#define FST_CS_PVAL_STATE_TRANSITION_CONFIRMED "TRANSITION_CONFIRMED"

/* FST Session reset reasons */
#define FST_CS_PVAL_REASON_TEARDOWN     "REASON_TEARDOWN"
#define FST_CS_PVAL_REASON_SETUP        "REASON_SETUP"
#define FST_CS_PVAL_REASON_SWITCH       "REASON_SWITCH"
#define FST_CS_PVAL_REASON_STT          "REASON_STT"
#define FST_CS_PVAL_REASON_REJECT       "REASON_REJECT"
#define FST_CS_PVAL_REASON_ERROR_PARAMS "REASON_ERROR_PARAMS"
#define FST_CS_PVAL_REASON_RESET        "REASON_RESET"
#define FST_CS_PVAL_REASON_DETACH_IFACE "REASON_DETACH_IFACE"

/* FST Session responses */
#define FST_CS_PVAL_RESPONSE_ACCEPT "ACCEPT"
#define FST_CS_PVAL_RESPONSE_REJECT "REJECT"

/* FST Session action initiator */
#define FST_CS_PVAL_INITIATOR_LOCAL  "LOCAL"
#define FST_CS_PVAL_INITIATOR_REMOTE "REMOTE"

/* FST-CLI subcommands and parameter names */
#define FST_CMD_HELP             "help"
#define FST_CMD_LIST_GROUPS      "list_groups"
#define FST_CMD_LIST_IFACES      "list_ifaces"
#define FST_CMD_IFACE_PEERS      "iface_peers"
#define FST_CMD_GET_PEER_MBIES   "get_peer_mbies"
#define FST_CMD_LIST_SESSIONS    "list_sessions"
#define FST_CMD_SESSION_ADD      "session_add"
#define FST_CMD_SESSION_REMOVE   "session_remove"
#define FST_CMD_SESSION_GET      "session_get"
    #define FST_CSG_PNAME_OLD_PEER_ADDR  "old_peer_addr" /* pval = address string */
    #define FST_CSG_PNAME_NEW_PEER_ADDR  "new_peer_addr" /* pval = address string */
    #define FST_CSG_PNAME_OLD_IFNAME     "old_ifname"    /* pval = ifname */
    #define FST_CSG_PNAME_NEW_IFNAME     "new_ifname"    /* pval = ifname */
    #define FST_CSG_PNAME_LLT            "llt"           /* pval = numeric llt value */
    #define FST_CSG_PNAME_STATE          "state"         /* pval = FST_CS_PVAL_STATE_... */
#define FST_CMD_SESSION_SET      "session_set"
    #define FST_CSS_PNAME_OLD_PEER_ADDR  FST_CSG_PNAME_OLD_PEER_ADDR
    #define FST_CSS_PNAME_NEW_PEER_ADDR  FST_CSG_PNAME_NEW_PEER_ADDR
    #define FST_CSS_PNAME_OLD_IFNAME     FST_CSG_PNAME_OLD_IFNAME
    #define FST_CSS_PNAME_NEW_IFNAME     FST_CSG_PNAME_NEW_IFNAME
    #define FST_CSS_PNAME_LLT            FST_CSG_PNAME_LLT
#define FST_CMD_SESSION_INITIATE "session_initiate"
#define FST_CMD_SESSION_RESPOND  "session_respond"
#define FST_CMD_SESSION_TRANSFER "session_transfer"
#define FST_CMD_SESSION_TEARDOWN "session_teardown"

#ifdef CONFIG_FST_TEST
#define FST_CTR_PVAL_BAD_NEW_BAND        "bad_new_band"

#define FST_CMD_TEST_REQUEST    "test_request"
    #define FST_CTR_IS_SUPPORTED        "is_supported"
    #define FST_CTR_SEND_SETUP_REQUEST  "send_setup_request"
    #define FST_CTR_SEND_SETUP_RESPONSE "send_setup_response"
    #define FST_CTR_SEND_ACK_REQUEST    "send_ack_request"
    #define FST_CTR_SEND_ACK_RESPONSE   "send_ack_response"
    #define FST_CTR_SEND_TEAR_DOWN      "send_tear_down"
    #define FST_CTR_GET_FSTS_ID         "get_fsts_id"
    #define FST_CTR_GET_LOCAL_MBIES     "get_local_mbies"
#endif

/* Events */
#define FST_CTRL_EVENT_IFACE "FST-EVENT-IFACE"
    #define FST_CEI_PNAME_IFNAME       "ifname"
    #define FST_CEI_PNAME_GROUP        "group"
    #define FST_CEI_PNAME_ATTACHED     "attached"
    #define FST_CEI_PNAME_DETACHED     "detached"
#define FST_CTRL_EVENT_PEER "FST-EVENT-PEER"
    #define FST_CEP_PNAME_IFNAME       "ifname"
    #define FST_CEP_PNAME_ADDR         "peer_addr"
    #define FST_CEP_PNAME_CONNECTED    "connected"
    #define FST_CEP_PNAME_DISCONNECTED "disconnected"
#define FST_CTRL_EVENT_SESSION "FST-EVENT-SESSION"
    #define FST_CES_PNAME_SESSION_ID "session_id"
    #define FST_CES_PNAME_EVT_TYPE   "event_type"
        #define FST_PVAL_EVT_TYPE_SESSION_STATE "EVENT_FST_SESSION_STATE"
            /* old_state/new_state: pval = FST_CS_PVAL_STATE_... */
            #define FST_CES_PNAME_OLD_STATE   "old_state"
            #define FST_CES_PNAME_NEW_STATE   "new_state"
            #define FST_CES_PNAME_REASON      "reason" /* pval = FST_CS_PVAL_REASON_... */
            #define FST_CES_PNAME_REJECT_CODE "reject_code" /* pval = u8 code */
            #define FST_CES_PNAME_INITIATOR   "initiator" /* pval = FST_CS_PVAL_INITIATOR_... */
        #define FST_PVAL_EVT_TYPE_ESTABLISHED "EVENT_FST_ESTABLISHED"
        #define FST_PVAL_EVT_TYPE_SETUP "EVENT_FST_SETUP"

#ifdef CONFIG_DBUS

#define WPAS_DBUS_NEW_SERVICE		"fi.w1.wpa_supplicant1"
#define WPAS_DBUS_NEW_PATH		"/fi/w1/wpa_supplicant1"
#define WPAS_DBUS_NEW_INTERFACE		"fi.w1.wpa_supplicant1"

#define WPAS_DBUS_NEW_PATH_INTERFACES	WPAS_DBUS_NEW_PATH "/Interfaces"
#define WPAS_DBUS_NEW_IFACE_INTERFACE	WPAS_DBUS_NEW_INTERFACE ".Interface"
#define WPAS_DBUS_NEW_IFACE_WPS WPAS_DBUS_NEW_IFACE_INTERFACE ".WPS"

#define WPAS_DBUS_NEW_PATH_FST          WPAS_DBUS_NEW_PATH "/FST"
#define WPAS_DBUS_NEW_IFACE_FST         WPAS_DBUS_NEW_INTERFACE ".FST"

#define WPAS_DBUS_NEW_FST_GROUPS_PART   "Groups"
#define WPAS_DBUS_NEW_IFACE_FST_GROUP    WPAS_DBUS_NEW_IFACE_FST ".Group"

#define WPAS_DBUS_NEW_FST_INTERFACES_PART "Interfaces"
#define WPAS_DBUS_NEW_IFACE_FST_INTERFACE  WPAS_DBUS_NEW_IFACE_FST ".Interface"

#define WPAS_DBUS_NEW_FST_SESSIONS_PART "Sessions"
#define WPAS_DBUS_NEW_IFACE_FST_SESSION  WPAS_DBUS_NEW_IFACE_FST ".Session"

/*
 * Global FST properties, methods and signals
 */
#define FST_DBUS_GLOBAL_PROP_GROUPS     "Groups"
#define FST_DBUS_GLOBAL_PROP_SESSIONS   "Sessions"


/* Session state change to INITIAL specific fields */
#define FST_DBUS_SIG_SSTATE_PNAME_INIT_REASON       "init_reason"
#define FST_DBUS_SIG_SSTATE_PNAME_INIT_REJECT_CODE  "reject_code"
#define FST_DBUS_SIG_SSTATE_PNAME_INIT_INITIATOR    "initiator"


#define FST_DBUS_GLOBAL_SIG     "Global"
	#define FST_DBUS_GLOBAL_SIG_PARAM_PATH   "path"
	#define FST_DBUS_GLOBAL_SIG_PARAM_EVENT  "event"
	#define FST_DBUS_GLOBAL_SIG_PARAM_PARAMS "params"

		#define FST_DBUS_GLOBAL_SIG_SALL_PNAME_SESSION_ID "session_id"
	#define FST_DBUS_GLOBAL_SIG_EVT_SESSION_SETUP    "SessionSetup"
	#define FST_DBUS_GLOBAL_SIG_EVT_SESSION_STATE    "SessionState"
		#define FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_OLD_STATE   "old_state"
		#define FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_NEW_STATE   "new_state"
		#define FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_INIT_REASON \
			FST_DBUS_SIG_SSTATE_PNAME_INIT_REASON
		#define FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_INIT_REJECT_CODE \
			FST_DBUS_SIG_SSTATE_PNAME_INIT_REJECT_CODE
		#define FST_DBUS_GLOBAL_SIG_SSTATE_PNAME_INIT_INITIATOR \
			FST_DBUS_SIG_SSTATE_PNAME_INIT_INITIATOR
	#define FST_DBUS_GLOBAL_SIG_EVT_SESSION_ESTAB    "SessionEstablished"
	#define FST_DBUS_GLOBAL_SIG_EVT_IFACE_PEER_STATE "InterfacePeerState"
		#define FST_DBUS_GLOBAL_SIG_PNAME_IFNAME     "path"
		#define FST_DBUS_GLOBAL_SIG_PNAME_CONNECTED  "connected"
		#define FST_DBUS_GLOBAL_SIG_PNAME_PEER_ADDR  "peer_addr"

/*
 * Group FST properties, methods and signals
 */
#define FST_DBUS_GROUP_PROP_SESSIONS    "Sessions"
#define FST_DBUS_GROUP_PROP_IFACES      "Interfaces"

#define FST_DBUS_GROUP_MTHD_ADD_SESSION "AddSession"
	#define FST_DBUS_GROUP_MTHD_ADD_SESSION_ID_PARAM "session_id"


#define FST_DBUS_GROUP_SIG_SESSION_SETUP     "Setup"
	#define FST_DBUS_GROUP_SIG_SSESSION_PARAM_PATH   "path"

/*
 * Session FST properties, methods and signals
 */
#define FST_DBUS_SESSION_SIG_STATE       "State"
	#define FST_DBUS_SESSION_SIG_PARAM_OLD_STATE    "old_state"
	#define FST_DBUS_SESSION_SIG_PARAM_NEW_STATE    "new_state"
	#define FST_DBUS_SESSION_SIG_PARAM_PARAMS       "params"
		#define FST_DBUS_SESSION_SIG_PNAME_REASON \
			FST_DBUS_SIG_SSTATE_PNAME_INIT_REASON
		#define FST_DBUS_SESSION_SIG_PNAME_REJECT_CODE \
			FST_DBUS_SIG_SSTATE_PNAME_INIT_REJECT_CODE
		#define FST_DBUS_SESSION_SIG_PNAME_INITIATOR \
			FST_DBUS_SIG_SSTATE_PNAME_INIT_INITIATOR
#define FST_DBUS_SESSION_SIG_ESTABLISHED "Established"

#define FST_DBUS_SESSION_MTHD_SET       "Set"
	#define FST_DBUS_SESSION_MTHD_SET_PNAME_PNAME       "pname"
	#define FST_DBUS_SESSION_MTHD_SET_PNAME_PVAL        "pvalue"
#define FST_DBUS_SESSION_MTHD_INITIATE  "Initiate"
#define FST_DBUS_SESSION_MTHD_RESPOND   "Respond"
	#define FST_DBUS_SESSION_MTHD_RESPOND_PNAME_STATUS  "respond_status"
#define FST_DBUS_SESSION_MTHD_TRANSFER  "Transfer"
#define FST_DBUS_SESSION_MTHD_TEARDOWN  "TearDown"
#define FST_DBUS_SESSION_MTHD_REMOVE    "Remove"

#define FST_DBUS_SESSION_PROP_GROUP     "Group"
#define FST_DBUS_SESSION_PROP_ID        "ID"
#define FST_DBUS_SESSION_PROP_STATE     "State"
#define FST_DBUS_SESSION_PROP_OLD_IFACE "OldInterface"
#define FST_DBUS_SESSION_PROP_NEW_IFACE "NewInterface"
#define FST_DBUS_SESSION_PROP_OWN_ADDR  "OwnAddress"
#define FST_DBUS_SESSION_PROP_PEER_ADDR "PeerAddress"
#define FST_DBUS_SESSION_PROP_LLT       "LLT"

/*
 * Interface FST properties, methods and signals
 */

#define FST_DBUS_IFACE_PROP_GROUP_ID   "GroupID"
#define FST_DBUS_IFACE_PROP_PRIORITY   "Priority"
#define FST_DBUS_IFACE_PROP_LLT        "LLT"
#define FST_DBUS_IFACE_PROP_PEER_ADDR  "PeerAddress"
#define FST_DBUS_IFACE_PROP_PEER_MBIEs "PeerMBIEs"

#define FST_DBUS_IFACE_SIG_PEER_STATE  "PeerState"
	#define FST_DBUS_IFACE_SIG_PNAME_CONNECTED "connected"
	#define FST_DBUS_IFACE_SIG_PNAME_PEER_ADDR "peer_addr"

#endif

#endif /* FST_CTRL_DEFS_H */
