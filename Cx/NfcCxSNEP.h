/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxSNEP.h

Abstract:

    SNEP Interface declarations
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

typedef struct _NFCCX_SNEP_INTERFACE {
    PNFCCX_FDO_CONTEXT      FdoContext;
    phLibNfc_SnepConfig_t   sConfigInfo;
    phLibNfc_Handle         pServerHandleDef;
    phLibNfc_Handle         pConnHandleDef;
    phLibNfc_Handle         pClientHandleDef;
    phLibNfc_Data_t         sDataInbox;
    phLibNfc_Data_t         sSendDataBuff;
} NFCCX_SNEP_INTERFACE, *PNFCCX_SNEP_INTERFACE;

typedef struct _NFCCX_SNEP_LIBNFC_REQUEST_CONTEXT {
    PNFCCX_SNEP_INTERFACE   SNEPInterface;
    PNFCCX_CX_SEQUENCE      Sequence;
} NFCCX_SNEP_LIBNFC_REQUEST_CONTEXT, *PNFCCX_SNEP_LIBNFC_REQUEST_CONTEXT;

NTSTATUS
NfcCxSNEPInterfaceCreate(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _Out_ PNFCCX_SNEP_INTERFACE * PPSNEPInterface
    );

VOID
NfcCxSNEPInterfaceDestroy(
    _In_ PNFCCX_SNEP_INTERFACE SNEPInterface
    );

NTSTATUS
NfcCxSNEPInterfaceServerInit(
    _In_ PNFCCX_SNEP_INTERFACE SNEPInterface
    );

NTSTATUS
NfcCxSNEPInterfaceDeinit(
    _In_ PNFCCX_SNEP_INTERFACE SNEPInterface
    );

NTSTATUS
NfcCxSNEPInterfaceClientConnect(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

#define SNEP_INTERFACE_CONNECT_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxSNEPInterfaceClientConnect)

NTSTATUS
NfcCxSNEPInterfaceClientPut(
    _In_ PNFCCX_RF_INTERFACE RFInterface,
    _In_ NTSTATUS Status,
    _In_opt_ VOID* Param1,
    _In_opt_ VOID* Param2
    );

#define SNEP_INTERFACE_CLIENT_PUT_SEQUENCE \
    NFCCX_CX_SEQUENCE_ENTRY(NfcCxSNEPInterfaceClientPut)
