/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    NfcCxDTA.h

Abstract:

    DTA Interface declaration
    
Environment:

    User-mode Driver Framework

--*/

#pragma once

#define NFCCX_MAX_REMOTE_DEV_QUEUE_LENGTH (5)
#define NFCCX_MAX_REMOTE_DEV_RECV_QUEUE_LENGTH (5)
#define NFCCX_MAX_SE_GET_NEXT_EVENT_QUEUE_LENGTH (5)
#define NFCCX_MAX_LLCP_SOCKET_LISTEN_QUEUE_LENGTH (5)
#define NFCCX_MAX_LLCP_SOCKET_ERROR_QUEUE_LENGTH (5)
#define NFCCX_MAX_LLCP_LINK_STATUS_QUEUE_LENGTH (5)
#define NFCCX_MAX_SNEP_CONNECTION_QUEUE_LENGTH (5)
#define NFCCX_MAX_SNEP_REQUEST_QUEUE_LENGTH (5)

#define BIT_AT_POSITION(x, p)  ((x >> (p - 1)) & 0x01)

typedef enum _NFCCX_DTA_MESSAGE {
    DTA_SET_MODE,
    DTA_DISCOVER_CONFIG,
    DTA_REMOTE_DEV_CONNECT,
    DTA_REMOTE_DEV_DISCONNECT,
    DTA_REMOTE_DEV_TRANSCEIVE,
    DTA_REMOTE_DEV_SEND,
    DTA_P2P_CONFIG,
    DTA_RF_CONFIG,
    DTA_REMOTE_DEV_NDEF_WRITE,
    DTA_REMOTE_DEV_NDEF_READ,
    DTA_REMOTE_DEV_NDEF_CONVERT_READ_ONLY,
    DTA_REMOTE_DEV_NDEF_CHECK,
    DTA_LLCP_CONFIG,
    DTA_LLCP_ACTIVATE,
    DTA_LLCP_DEACTIVATE,
    DTA_LLCP_DISCOVER_SERVICES,
    DTA_LLCP_LINK_STATUS_CHECK,
    DTA_LLCP_SOCKET_CREATE,
    DTA_LLCP_SOCKET_CLOSE,
    DTA_LLCP_SOCKET_BIND,
    DTA_LLCP_SOCKET_LISTEN,
    DTA_LLCP_SOCKET_ACCEPT,
    DTA_LLCP_SOCKET_CONNECT,
    DTA_LLCP_SOCKET_DISCONNECT,
    DTA_LLCP_SOCKET_RECV,
    DTA_LLCP_SOCKET_SEND,
    DTA_LLCP_SOCKET_GET_ERROR,
    DTA_SNEP_SERVER_INIT,
    DTA_SNEP_SERVER_DEINIT,
    DTA_SNEP_SERVER_GET_NEXT_CONNECTION,
    DTA_SNEP_SERVER_ACCEPT,
    DTA_SNEP_SERVER_GET_NEXT_REQUEST,
    DTA_SNEP_SERVER_SEND_RESPONSE,
    DTA_SNEP_CLIENT_INIT,
    DTA_SNEP_CLIENT_DEINIT,
    DTA_SNEP_CLIENT_PUT,
    DTA_SNEP_CLIENT_GET,
    DTA_SE_ENUMERATE,
    DTA_SE_SET_EMULATION_MODE,
    DTA_SE_SET_ROUTING_TABLE,
    DTA_MESSAGE_MAX
} NFCCX_DTA_MESSAGE, *PNFCCX_DTA_MESSAGE;

typedef
NTSTATUS
NFCCX_DTA_DISPATCH_HANDLER(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );

typedef NFCCX_DTA_DISPATCH_HANDLER *PFN_NFCCX_DTA_DISPATCH_HANDLER;

class CNFCDtaSocketContext
{
public:
    CNFCDtaSocketContext();
    CNFCDtaSocketContext(
        _In_ NFC_LLCP_SOCKET_HANDLE hSocket,
        _In_ PNFCCX_DTA_INTERFACE pDTAInterface
        );
    ~CNFCDtaSocketContext();

    NTSTATUS Initialize(
        _In_ NFC_LLCP_SOCKET_HANDLE hSocket,
        _In_ PNFCCX_DTA_INTERFACE pDTAInterface,
        _In_ DWORD WorkingBufferSize
        );

    VOID Deinitialize();
    NTSTATUS EnableSocketListen();
    NTSTATUS EnableSocketError();
    NTSTATUS EnableSocketReceive();

    NFC_LLCP_SOCKET_HANDLE GetSocket() const
    {
        return m_hSocket;
    }
    VOID SetSocket(const NFC_LLCP_SOCKET_HANDLE hSocket)
    {
        m_hSocket = hSocket;
    }
    PNFCCX_DTA_INTERFACE GetDTAInterface()
    {
        return m_pDTAInterface;
    }
    phNfc_sData_t* GetWorkingBuffer()
    {
        return &m_sWorkingBuffer;
    }
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    BOOL GetListenEnabled() const
    {
        return m_bListenEnabled;
    }
    CNFCPendedRequest* GetSocketError()
    {
        return &m_SocketError;
    }
    CNFCPendedRequest* GetSocketListen()
    {
        return &m_SocketListen;
    }
    NFC_LLCP_SOCKET_TYPE GetSocketType()
    {
        return m_eSocketType;
    }
    void SetSocketType(NFC_LLCP_SOCKET_TYPE eSocketType)
    {
        m_eSocketType = eSocketType;
    }
    phNfc_sData_t* GetReceiveBuffer()
    {
        return &m_sReceiveBuffer;
    }
    CNFCPendedRequest* GetSocketReceive()
    {
        return &m_SocketReceive;
    }
    phNfc_sData_t* GetSendBuffer()
    {
        return &m_sSendBuffer;
    }
    static CNFCDtaSocketContext* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CNFCDtaSocketContext*)CONTAINING_RECORD(pEntry, CNFCDtaSocketContext, m_ListEntry);
    }

private:
    NFC_LLCP_SOCKET_HANDLE      m_hSocket;
    PNFCCX_DTA_INTERFACE        m_pDTAInterface;
    phNfc_sData_t               m_sWorkingBuffer;
    LIST_ENTRY                  m_ListEntry;
    CNFCPendedRequest           m_SocketError;
    CNFCPendedRequest           m_SocketListen;
    BOOLEAN                     m_bListenEnabled;
    NFC_LLCP_SOCKET_TYPE        m_eSocketType;
    phNfc_sData_t               m_sReceiveBuffer;
    CNFCPendedRequest           m_SocketReceive;
    phNfc_sData_t               m_sSendBuffer;
};

class CNFCDtaSnepServerContext
{
public:
    CNFCDtaSnepServerContext();
    CNFCDtaSnepServerContext(
        _In_ NFC_SNEP_SERVER_HANDLE hSnepServer,
        _In_ PNFCCX_DTA_INTERFACE pDTAInterface
        );
    ~CNFCDtaSnepServerContext();

    NTSTATUS Initialize(
        _In_ NFC_SNEP_SERVER_HANDLE hSnepServer,
        _In_ PNFCCX_DTA_INTERFACE pDTAInterface,
        _In_ DWORD InboxBufferSize
        );

    VOID Deinitialize();
    NTSTATUS EnableSnepConnection();
    NTSTATUS EnableSnepRequest();

    NFC_SNEP_SERVER_HANDLE GetSnepServer() const
    {
        return m_hSnepServer;
    }
    VOID SetSnepServer(const NFC_SNEP_SERVER_HANDLE hSnepServer)
    {
        m_hSnepServer = hSnepServer;
    }
    PNFCCX_DTA_INTERFACE GetDTAInterface()
    {
        return m_pDTAInterface;
    }
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    CNFCPendedRequest* GetConnection()
    {
        return &m_Connection;
    }
    NFC_SNEP_SERVER_TYPE GetServerType()
    {
        return m_eServerType;
    }
    void SetServerType(NFC_SNEP_SERVER_TYPE eServerType)
    {
        m_eServerType = eServerType;
    }
    phNfc_sData_t* GetDataInbox()
    {
        return &m_sDataInbox;
    }
    CNFCPendedRequest* GetRequest()
    {
        return &m_Request;
    }
    PNFC_SNEP_SERVER_RESPONSE_INFO GetServerResponseInfo()
    {
        return m_pServerResponseInfo;
    }
    void SetServerResponseInfo(PNFC_SNEP_SERVER_RESPONSE_INFO ServerResponseInfo)
    {
        m_pServerResponseInfo = ServerResponseInfo;
    }
    static CNFCDtaSnepServerContext* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CNFCDtaSnepServerContext*)CONTAINING_RECORD(pEntry, CNFCDtaSnepServerContext, m_ListEntry);
    }

private:
    NFC_SNEP_SERVER_HANDLE      m_hSnepServer;
    PNFCCX_DTA_INTERFACE        m_pDTAInterface;
    LIST_ENTRY                  m_ListEntry;
    CNFCPendedRequest           m_Connection;
    NFC_SNEP_SERVER_TYPE        m_eServerType;
    phLibNfc_Data_t             m_sDataInbox;
    CNFCPendedRequest           m_Request;
    PNFC_SNEP_SERVER_RESPONSE_INFO
                                m_pServerResponseInfo;
};

typedef struct _NFCCX_DTA_REQUEST_CONTEXT {
    PNFCCX_FILE_CONTEXT FileContext;
    WDFREQUEST Request;
    PVOID OutputBuffer;
    size_t OutputBufferLength;
} NFCCX_DTA_REQUEST_CONTEXT, *PNFCCX_DTA_REQUEST_CONTEXT;

typedef struct _NFCCX_DTA_INTERFACE {

    //
    // Back link to the fdo context
    //
    PNFCCX_FDO_CONTEXT FdoContext;

    //
    // Interface Created
    //
    BOOLEAN InterfaceCreated;

    //
    // Device operation Lock
    // Used to serialize all the user calls.
    //
    WDFWAITLOCK DeviceLock;

    //
    // LibNfc references
    //
    phLibNfc_sConfig_t LibConfig;
    PNFCCX_LIBNFC_CONTEXT pLibNfcContext;

    //
    // Buffer used for synchronous calls
    //
    PBYTE Buffer;
    DWORD BufferSize;
    //
    // Send/Receive buffers
    //
    NFC_REMOTE_DEV_HANDLE hRemoteDev;
    phNfc_sTransceiveInfo_t sTransceiveBuffer;
    phNfc_sData_t sReceiveBuffer;
    phNfc_sData_t sSendBuffer;

    //
    // Ndef Message buffer
    //
    NFC_NDEF_INFO sNdefInfo;
    phLibNfc_Data_t sNdefMsg;

    //
    // Llcp Services
    //
    phNfc_sData_t *psServiceNameList;
    WDFWAITLOCK SocketLock;
    _Guarded_by_(SocketLock)
    LIST_ENTRY SocketContext;
    phFriNfc_LlcpMac_eLinkStatus_t eLinkStatus;
    BOOLEAN bLlcpLinkStatusEnabled;

    CNFCPendedRequest RemoteDevGetNext;
    CNFCPendedRequest RemoteDevRecv;
    CNFCPendedRequest SeGetNextEvent;
    CNFCPendedRequest LlcpLinkStatus;
    BOOLEAN bLlcpAutoActivate;

    //
    // Snep Server
    //
    WDFWAITLOCK SnepServerLock;
    _Guarded_by_(SnepServerLock)
    LIST_ENTRY SnepServerContext;

    //
    // Snep Client
    //
    PNFC_SNEP_CLIENT_HANDLE SnepClientHandle;
    PNFC_SNEP_CLIENT_DATA_BUFFER SnepClientDataBuffer;

} NFCCX_DTA_INTERFACE, *PNFCCX_DTA_INTERFACE;


//
// Interface
//
NTSTATUS
NfcCxDTAInterfaceCreate(
_In_ PNFCCX_FDO_CONTEXT FdoContext,
    _Out_ PNFCCX_DTA_INTERFACE * PPDTAInterface
    );

VOID
NfcCxDTAInterfaceDestroy(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    );

NTSTATUS
NfcCxDTAInterfaceStart(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    );

NTSTATUS
NfcCxDTAInterfaceStop(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface
    );

BOOLEAN 
NfcCxDTAInterfaceIsIoctlSupported(
    _In_ PNFCCX_FDO_CONTEXT FdoContext,
    _In_ ULONG IoControlCode
    );

NTSTATUS 
NfcCxDTAInterfaceIoDispatch(
    _In_opt_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST    Request,
    _In_ size_t        OutputBufferLength,
    _In_ size_t        InputBufferLength,
    _In_ ULONG         IoControlCode
    );

NTSTATUS
NfcCxDTAInterfaceValidateRequest(
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength
    );

NTSTATUS
NfcCxDTAInterfaceDispatchRequest(
    _In_ PNFCCX_FILE_CONTEXT FileContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_opt_bytecount_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_opt_bytecap_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    );


VOID
NfcCxDTAPostLibNfcThreadMessage(
    _Inout_ PVOID DTAInterface,
    _In_ DWORD Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    );

VOID
NfcCxDTAInterfaceLibNfcMessageHandler(
    _In_ PVOID Context,
    _In_ DWORD Message,
    _In_ UINT_PTR Param1,
    _In_ UINT_PTR Param2,
    _In_ UINT_PTR Param3,
    _In_ UINT_PTR Param4
    );

//
// NFC DTA DDI
//
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchConfigRfDiscovery;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDeviceGetNext;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDeviceConnect;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDeviceDisconnect;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDeviceTransceive;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDeviceRecv;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDeviceSend;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDeviceCheckPresence;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchConfigP2pParam;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSetRfConfig;

//
// NDEF
//
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDevNdefWrite;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDevNdefRead;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDevNdefConvertReadOnly;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchRemoteDevNdefCheck;

//
// LLCP
//
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpConfig;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpActivate;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpDeactivate;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpDiscoverServices;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpLinkStatusCheck;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpGetNextLinkStatus;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketCreate;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketClose;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketBind;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketListen;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketAccept;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketConnect;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketDisconnect;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpRecv;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpRecvFrom;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketSend;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpSocketSendTo;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchLlcpGetNextError;

//
// SNEP Server
//
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepInitServer;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepDeinitServer;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepServerGetNextConnection;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepServerAccept;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepServerGetNextRequest;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepServerSendResponse;

//
// SNEP Client
//
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepInitClient;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepDeinitClient;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepClientPut;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSnepClientGet;

//
// SE 
//
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSeEnumerate;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSeSetEmulationMode;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSeSetRoutingTable;
NFCCX_DTA_DISPATCH_HANDLER NfcCxDTAInterfaceDispatchSeGetNextEvent;

VOID
NfcCxDTAInterfaceHandleRemoteDev(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ phLibNfc_RemoteDevList_t* RemoteDev
    );

NTSTATUS
NfcCxDTAInterfacePrepareTransceiveBuffer(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ PNFC_REMOTE_DEV_SEND_INFO remoteDevSendInfo,
    _In_ PNFC_REMOTE_DEV_RECV_INFO remoteDevRecvInfo
    );

VOID
NfcCxDTAInterfaceHandleRemoteDevRecv(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_REMOTE_DEV_HANDLE RemoteDevHandle,
    _In_bytecount_(DataLength) PVOID Data,
    _In_ USHORT DataLength
    );

NTSTATUS
NfcCxDTAInterfaceValidateRoutingTable(
    _In_ PNFC_SE_ROUTING_TABLE RoutingTable
    );

NTSTATUS
NfcCxDTAInterfaceConvertRoutingTable(
    _In_ PNFC_SE_ROUTING_TABLE RoutingTable,
    _Out_ phLibNfc_RtngConfig_t* pRtngTable
    );

VOID
NfcCxDTAInterfaceHandleSeEvent(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_SE_EVENT_TYPE EventType,
    _In_ NFC_SE_HANDLE hSecureElement,
    _In_ phLibNfc_uSeEvtInfo_t *pSeEvtInfo
    );

VOID
NfcCxDTAInterfaceHandleLlcpLinkStatus(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ phFriNfc_LlcpMac_eLinkStatus_t LlcpLinkStatus
    );

NTSTATUS
NfcCxDTAInterfaceVerifySocketHandle(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_LLCP_SOCKET_HANDLE LlcpSocketHandle
    );

NTSTATUS
NfcCxDTAInterfaceVerifySnepServerHandle(
    _In_ PNFCCX_DTA_INTERFACE DTAInterface,
    _In_ NFC_SNEP_SERVER_HANDLE SnepServerHandle
    );
