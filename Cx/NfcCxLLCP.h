/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxLLCP.h

Abstract:

    LLCP Interface declarations
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

//
// LLCP Configurable Parameters
//
typedef struct _NFCCX_LLCP_CONFIG_PARAM {
    USHORT      uMIU;
    UCHAR       uLTO;
    UCHAR       uRecvWindowSize;
} NFCCX_LLCP_CONFIG_PARAM;

//
// LLCP Request State
//
#define NFCCX_LLCP_REQUEST_COMPLETE 0x0
#define NFCCX_LLCP_REQUEST_PENDING 0x1

//
// LLCP Context
//
typedef struct _NFCCX_LLCP_INTERFACE {
    PNFCCX_FDO_CONTEXT                  FdoContext;
    NFCCX_LLCP_CONFIG_PARAM             sLlcpConfigParams;
    phLibNfc_Llcp_eLinkStatus_t         eLinkStatus;
    phLibNfc_Llcp_sLinkParameters_t     sLocalLinkInfo;
    phNfc_sData_t                       sServiceName;
    UCHAR                               eRequestState;
    bool                                IsDeactivatePending;
} NFCCX_LLCP_INTERFACE, *PNFCCX_LLCP_INTERFACE;

typedef struct _NFCCX_LLCP_LIBNFC_REQUEST_CONTEXT {
    PNFCCX_LLCP_INTERFACE   LLCPInterface;
    PNFCCX_CX_SEQUENCE      Sequence;
} NFCCX_LLCP_LIBNFC_REQUEST_CONTEXT, *PNFCCX_LLCP_LIBNFC_REQUEST_CONTEXT;

NTSTATUS
NfcCxLLCPInterfaceCreate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Outptr_ PNFCCX_LLCP_INTERFACE * PPLLCPInterface
    );

VOID
NfcCxLLCPInterfaceDestroy(
    _In_ PNFCCX_LLCP_INTERFACE  LLCPInterface
    );

NTSTATUS
NfcCxLLCPInterfaceConfigure(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

#define LLCP_INTERFACE_CONFIG_SEQUENCE \
    { NfcCxLLCPInterfaceConfigure, NULL },

NTSTATUS
NfcCxLLCPInterfaceCheck(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

#define LLCP_INTERFACE_CHECK_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxLLCPInterfaceCheck)

NTSTATUS
NfcCxLLCPInterfaceActivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

#define LLCP_INTERFACE_ACTIVATE_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxLLCPInterfaceActivate)

NTSTATUS
NfcCxLLCPInterfaceDeactivate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

#define LLCP_INTERFACE_DEACTIVATE_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxLLCPInterfaceDeactivate)
