/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:
    phNfcTraceEnums.h
    
Abstract:
    This module defines the WPP tracing macros for the NFC core library

Environment:
   User mode.

--*/
#pragma once

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Custom Type definitions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// begin_wpp config
// CUSTOM_TYPE(phLibNfc_State, ItemEnum(phLibNfc_State) );
// CUSTOM_TYPE(phLibNfc_Event_t, ItemEnum(phLibNfc_Event) );
// CUSTOM_TYPE(phLibNfc_TransitionFlag, ItemEnum(phLibNfc_TransitionFlag) );
// end_wpp

// begin_wpp config
// CUSTOM_TYPE(phLibNfc_SE_Type_t, ItemEnum(phLibNfc_SE_Type) );
// CUSTOM_TYPE(phLibNfc_eSE_ActivationMode, ItemEnum(phLibNfc_eSE_ActivationMode) );
// CUSTOM_TYPE(phLibNfc_PowerLinkModes_t, ItemEnum(phLibNfc_PowerLinkModes_t) );
// CUSTOM_TYPE(phLibNfc_eSE_EvtType_t, ItemEnum(phLibNfc_eSE_EvtType) );
// end_wpp

// begin_wpp config
// CUSTOM_TYPE(phNfc_eRFDevType_t, ItemEnum(phNfc_eRFDevType) );
// end_wpp

// begin_wpp config
// CUSTOM_TYPE(phNciNfc_StateSend_t, ItemEnum(phNciNfc_StateSends) );
// CUSTOM_TYPE(phNciNfc_EvtSend_t, ItemEnum(phNciNfc_EvtSend) );
// CUSTOM_TYPE(phNciNfc_StateRecv_t, ItemEnum(phNciNfc_StateRecv) );
// CUSTOM_TYPE(phNciNfc_EvtRecv_t, ItemEnum(phNciNfc_EvtRecv) );
// end_wpp

// begin_wpp config
// CUSTOM_TYPE(phNciNfc_CoreGid, ItemEnum(phNciNfc_CoreGid) );
// CUSTOM_TYPE(phNciNfc_CoreNciCoreCmdOid_t, ItemEnum(phNciNfc_CoreNciCoreCmdOid) );
// CUSTOM_TYPE(phNciNfc_CoreNciCoreRspOid_t, ItemEnum(phNciNfc_CoreNciCoreRspOid) );
// CUSTOM_TYPE(phNciNfc_CoreNciCoreNtfOid_t, ItemEnum(phNciNfc_CoreNciCoreNtfOid) );
// CUSTOM_TYPE(phNciNfc_CoreRfMgtCmdOid_t, ItemEnum(phNciNfc_CoreRfMgtCmdOid) );
// CUSTOM_TYPE(phNciNfc_CoreRfMgtRspOid_t, ItemEnum(phNciNfc_CoreRfMgtRspOid) );
// CUSTOM_TYPE(phNciNfc_CoreRfMgtNtfOid_t, ItemEnum(phNciNfc_CoreRfMgtNtfOid) );
// CUSTOM_TYPE(phNciNfc_CoreNfceeMgtCmdOid_t, ItemEnum(phNciNfc_CoreNfceeMgtCmdOid) );
// CUSTOM_TYPE(phNciNfc_CoreNfceeMgtRspOid_t, ItemEnum(phNciNfc_CoreNfceeMgtRspOid) );
// CUSTOM_TYPE(phNciNfc_CoreNfceeMgtNtfOid_t, ItemEnum(phNciNfc_CoreNfceeMgtNtfOid) );
// CUSTOM_TYPE(phNciNfc_CorePropCmdOid_t, ItemEnum(phNciNfc_CorePropCmdOid) );
// CUSTOM_TYPE(phNciNfc_CorePropRspOid_t, ItemEnum(phNciNfc_CorePropRspOid) );
// CUSTOM_TYPE(phNciNfc_CorePropNtfOid_t, ItemEnum(phNciNfc_CorePropNtfOid) );
// CUSTOM_TYPE(phNciNfc_NciCoreMsgType_t, ItemEnum(phNciNfc_NciCoreMsgType) );
// end_wpp

// begin_wpp config
// CUSTOM_TYPE(phNciNfc_ResetType_t, ItemEnum(phNciNfc_ResetType) );
// CUSTOM_TYPE(phNciNfc_DestType_t, ItemEnum(phNciNfc_DestType) );
// CUSTOM_TYPE(phNciNfc_DeActivateType_t, ItemEnum(phNciNfc_DeActivateType) );
// CUSTOM_TYPE(phNciNfc_DeActivateReason_t, ItemEnum(phNciNfc_DeActivateReason) );
// CUSTOM_TYPE(phNciNfc_RFDevType_t, ItemEnum(phNciNfc_RFDevType) );
// CUSTOM_TYPE(phNciNfc_RfProtocols_t, ItemEnum(phNciNfc_RfProtocols) );
// CUSTOM_TYPE(phNciNfc_RfTechMode_t, ItemEnum(phNciNfc_RfTechMode) );
// CUSTOM_TYPE(phNciNfc_RfInterfaces_t, ItemEnum(phNciNfc_RfInterfaces) );
// CUSTOM_TYPE(phNciNfc_RfTechnologies_t, ItemEnum(phNciNfc_RfTechnologies) );
// CUSTOM_TYPE(phNciNfc_RfParamType_t, ItemEnum(phNciNfc_RfParamType) );
// CUSTOM_TYPE(phNciNfc_RfFieldInfo_t, ItemEnum(phNciNfc_RfFieldInfo) );
// CUSTOM_TYPE(phNciNfc_RfNfceeTriggers_t, ItemEnum(phNciNfc_RfNfceeTriggers) );
// CUSTOM_TYPE(phNciNfc_RfNfceeDiscReqType_t, ItemEnum(phNciNfc_RfNfceeDiscReqType) );
// CUSTOM_TYPE(phNciNfc_NfceeIfType_t, ItemEnum(phNciNfc_NfceeIfType) );
// CUSTOM_TYPE(phNciNfc_LstnModeRtngType_t, ItemEnum(phNciNfc_LstnModeRtngType) );
// CUSTOM_TYPE(phNciNfc_NotificationType_t, ItemEnum(phNciNfc_NotificationType) );
// CUSTOM_TYPE(phNciNfc_GenericErrCode_t, ItemEnum(phNciNfc_GenericErrCode) );
// CUSTOM_TYPE(phNciNfc_ResetTrigger_t, ItemEnum(phNciNfc_ResetTrigger) );
// end_wpp

// begin_wpp config
// CUSTOM_TYPE(NFCSTATUS, ItemListShort(NFCSTATUS_SUCCESS,
//                            NFCSTATUS_INVALID_PARAMETER,
//                            NFCSTATUS_BUFFER_TOO_SMALL,
//                            NFCSTATUS_INVALID_DEVICE,
//                            NFCSTATUS_MORE_INFORMATION,
//                            NFCSTATUS_RF_TIMEOUT,
//                            NFCSTATUS_RF_ERROR,
//                            NFCSTATUS_INSUFFICIENT_RESOURCES,
//                            NFCSTATUS_PENDING,
//                            NFCSTATUS_BOARD_COMMUNICATION_ERROR,
//                            NFCSTATUS_INVALID_STATE,
//                            NFCSTATUS_NOT_INITIALISED,
//                            NFCSTATUS_ALREADY_INITIALISED,
//                            NFCSTATUS_FEATURE_NOT_SUPPORTED,
//                            NFCSTATUS_NOT_REGISTERED,
//                            NFCSTATUS_ALREADY_REGISTERED,
//                            NFCSTATUS_MULTIPLE_PROTOCOLS,
//                            NFCSTATUS_MULTIPLE_TAGS,
//                            NFCSTATUS_DESELECTED,
//                            NFCSTATUS_RELEASED,
//                            NFCSTATUS_NOT_ALLOWED,
//                            NFCSTATUS_BUSY,
//                            NFCSTATUS_INVALID_REMOTE_DEVICE,
//                            NFCSTATUS_SMART_TAG_FUNC_NOT_SUPPORTED,
//                            NFCSTATUS_READ_FAILED,
//                            NFCSTATUS_WRITE_FAILED,
//                            NFCSTATUS_NO_NDEF_SUPPORT,
//                            NFCSTATUS_EOF_NDEF_CONTAINER_REACHED,
//                            NFCSTATUS_INVALID_RECEIVE_LENGTH,
//                            NFCSTATUS_INVALID_FORMAT,
//                            NFCSTATUS_INSUFFICIENT_STORAGE,
//                            NFCSTATUS_FORMAT_ERROR,
//                            NFCSTATUS_CREDIT_TIMEOUT,
//                            NFCSTATUS_RESPONSE_TIMEOUT,
//                            NFCSTATUS_ALREADY_CONNECTED,
//                            NFCSTATUS_ANOTHER_DEVICE_CONNECTED,
//                            NFCSTATUS_SINGLE_TAG_ACTIVATED,
//                            NFCSTATUS_SINGLE_TAG_DISCOVERED,
//                            NFCSTATUS_SECURE_ELEMENT_ACTIVATED,
//                            NFCSTATUS_UNKNOWN_ERROR,
//                            NFCSTATUS_FAILED,
//                            NFCSTATUS_CMD_ABORTED,
//                            NFCSTATUS_MULTI_POLL_NOT_SUPPORTED,
//                            NFCSTATUS_NO_DEVICE_FOUND,
//                            NFCSTATUS_NO_TARGET_FOUND,
//                            NFCSTATUS_NO_DEVICE_CONNECTED,
//                            NFCSTATUS_EXTERNAL_RF_DETECTED,
//                            NFCSTATUS_MSG_NOT_ALLOWED_BY_FSM,
//                            NFCSTATUS_ACCESS_DENIED,
//                            NFCSTATUS_NODE_NOT_FOUND,
//                            NFCSTATUS_SMX_BAD_STATE,
//                            NFCSTATUS_ABORT_FAILED,
//                            NFCSTATUS_REG_OPMODE_NOT_SUPPORTED,
//                            NFCSTATUS_SHUTDOWN,
//                            NFCSTATUS_TARGET_LOST,
//                            NFCSTATUS_REJECTED,
//                            NFCSTATUS_TARGET_NOT_CONNECTED,
//                            NFCSTATUS_INVALID_HANDLE,
//                            NFCSTATUS_ABORTED,
//                            NFCSTATUS_COMMAND_NOT_SUPPORTED,
//                            NFCSTATUS_NON_NDEF_COMPLIANT,
//                            NFCSTATUS_NOT_ENOUGH_MEMORY,
//                            NFCSTATUS_INCOMING_CONNECTION,
//                            NFCSTATUS_CONNECTION_SUCCESS,
//                            NFCSTATUS_CONNECTION_FAILED,
//                            NFCSTATUS_RF_TIMEOUT_ERROR) );
// end_wpp

// begin_wpp config
// CUSTOM_TYPE(phFriNfc_LlcpTransport_eSocketType_t, ItemEnum(phFriNfc_LlcpTransport_eSocketType) );
// CUSTOM_TYPE(phFriNfc_LlcpMac_eLinkStatus_t, ItemEnum(phFriNfc_LlcpMac_eLinkStatus) );
// CUSTOM_TYPE(PHFRINFC_LLCP_STATE, ItemListShort(PHFRINFC_LLCP_STATE_RESET_INIT,
//                                            PHFRINFC_LLCP_STATE_CHECKED,
//                                            PHFRINFC_LLCP_STATE_ACTIVATION,
//                                            PHFRINFC_LLCP_STATE_PAX,
//                                            PHFRINFC_LLCP_STATE_OPERATION_RECV,
//                                            PHFRINFC_LLCP_STATE_OPERATION_SEND,
//                                            PHFRINFC_LLCP_STATE_DEACTIVATION) );
// end_wpp

