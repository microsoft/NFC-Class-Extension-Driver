/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phLibNfc.h>
#include <phOsalNfc.h>
#include <phFriNfc_SnepProtocol.h>

#define MAX_SNEP_SERVER_CNT             0x05
#define MAX_SNEP_SERVER_CONN            0x05
#define MAX_SNEP_CLIENT_CNT             0x0A

/** \ingroup grp_retval
    Minimum MIU requirement for SNEP */
#define MIN_MIU_VAL                     6

/** \ingroup grp_retval
    Minimum inbox size for default server */
#define MIN_INBOX_SIZE                  1024

/** \ingroup grp_retval
    Indicates incoming connection */
#define NFCSTATUS_INCOMING_CONNECTION               (0x0025)

/** \ingroup grp_retval
    Indicates Connection was successful */
#define NFCSTATUS_CONNECTION_SUCCESS                (0x0026)

/** \ingroup grp_retval
    Indicates Connection failed */
#define NFCSTATUS_CONNECTION_FAILED                 (0x0027)

/**
* \ingroup grp_lib_nfc
*
* \brief  Generic Response Callback definition.
*
* Generic callback definition used as callback type in few APIs below.
*
* \note : Status and error codes for this type of callback are documented in respective APIs
* wherever it is used.
*
* \param[in] pContext       LibNfc client context   passed in the corresponding request before.
* \param[in] status         Status of the response  callback.
* \param[in] ConnHandle     Snep Incoming Connection Handle.
*/
typedef void(*pphLibNfc_SnepConn_ntf_t) (void* pContext, NFCSTATUS  Status,  phLibNfc_Handle ConnHandle);


/**
* \ingroup grp_lib_nfc
*
* \brief  Generic Response Callback definition.
*
* Generic callback definition used as callback type in few APIs below.
*
* \note : Status and error codes for this type of callback are documented in respective APIs
* wherever it is used.
*
* \param[in] pContext       LibNfc client context   passed in the corresponding request before.
* \param[in] status         Status of the response  callback.
* \param[in] pDataInbox     Incoming data buffer (NDEF Message).
* \param[in] ConnHandle     Snep Connection Handle.
*/
typedef void(*pphLibNfc_SnepPut_ntf_t) (void* pContext, NFCSTATUS  Status, phLibNfc_Data_t *pDataInbox, phLibNfc_Handle ConnHandle);

/**
* \ingroup grp_lib_nfc
*
* \brief  Generic Response Callback definition.
*
* Generic callback definition used as callback type in few APIs below.
*
* \note : The parameter 'pGetMsgId' is NDEF Message Identifier for Incoming GET request.
*This buffer is deleted by caller as the callback returns. Status and error codes for this type of callback are documented in respective APIs
* wherever it is used.
*
* \param[in] pContext       LibNfc client context   passed in the corresponding request
*                           before.
* \param[in] status         Status of the response  callback.
* \param[in] pGetMsgId      NDEF Message Identifier for Incoming GET request.
* \param[in] ConnHandle     Snep Connection Handle.
*/
typedef void(*pphLibNfc_SnepGet_ntf_t) (void* pContext,NFCSTATUS  Status, phLibNfc_Data_t *pGetMsgId, phLibNfc_Handle ConnHandle);

/**
* \ingroup grp_snep_nfc
*
* \brief  Generic client Callback definition for Put and Get
*
* \note : Status and error codes for this type of callback are documented in respective APIs
* wherever it is used.
*
* \param[in] ConnHandle         Snep Incoming Connection Handle.
* \param[in] pContext           LibNfc client context   passed in the corresponding request before.
* \param[out] status            Status of the response  callback.
* \param[out] pReqResponse      data returned by peer for reuest sent. may be NULL(for PUT request).
*/
typedef void (*pphLibNfc_SnepReqCb_t) (phLibNfc_Handle ConnHandle, void* pContext, NFCSTATUS  Status,
                                       phLibNfc_Data_t *pReqResponse);

/**
*\ingroup grp_lib_nfc
*
*\brief Defines states of snep server.
*/

typedef enum
{
    phLibNfc_SnepServer_Uninitialized = 0x00,   /**< Server Not Registered */
    phLibNfc_SnepServer_Initialized,            /**< Server Registered not connected to client */
    phLibNfc_SnepServer_Connected,              /**< Server Connected with atleast one Peer client */
    phLibNfc_SnepServer_Sending,                /**< Server Sending Response to Peer client */
    phLibNfc_SnepServer_Received_Put,           /**< Server Connection Received PUT Request */
    phLibNfc_SnepServer_Received_Get            /**< Server Connection Received GET Request */
} phLibNfc_SnepServer_status_t;

/**
*\ingroup grp_lib_nfc
*
*\brief Defines snep server type
*/

typedef enum
{
    phLibNfc_SnepServer_Default = 0x00,     /**< Default NFC Forum SNEP server */
    phLibNfc_SnepServer_NonDefault          /**< Proprietary SNEP server */
} phLibNfc_SnepServer_type_t;

/**
*\ingroup grp_lib_nfc
*
*\brief Defines states of snep client.
*/

typedef enum
{
    phLibNfc_SnepClient_Uninitialized = 0x00,   /**< Client Not Registered */
    phLibNfc_SnepClient_Initialized,            /**< Client Registered not connected to Server */
    phLibNfc_SnepClient_Connected,              /**< Client Connected with peer Server */
    phLibNfc_SnepClient_PutResponse,            /**< Client waiting for PUT response */
    phLibNfc_SnepClient_GetResponse             /**< Client waiting for GET response */
} phLibNfc_SnepClient_status_t;

/**
*\ingroup grp_lib_nfc
*
*\brief Defines snep config data
*/
typedef struct phLibNfc_SnepConfig
{
    phLibNfc_SnepServer_type_t SnepServerType;  /**< SNEP Server type to be Initialized */
    phNfc_sData_t *SnepServerName;              /**< SNEP Service name for Non Default servers */
    phLibNfc_Llcp_sSocketOptions_t sOptions;    /**< LLCP Socket Options */
    bool_t bDtaFlag;                            /**< DTA mode flag */
} phLibNfc_SnepConfig_t, *pphLibNfc_SnepConfig_t;

/* Forward declare phLibNfc_SnepServerSession_t so that phLibNfc_SnepServerConnection_t can have a
   reference to it */
typedef struct SnepServerSession phLibNfc_SnepServerSession_t, *pphLibNfc_SnepServerSession_t;

/**
*\ingroup grp_lib_nfc
*
*\brief Single Data link connection on SNEP server
*/
typedef struct SnepServerConnection
{
    phLibNfc_SnepServerSession_t* pServerSession; /**< Back link to the server session that manages this connection */
    phLibNfc_Handle hSnepServerConnHandle;  /**< SNEP Server Data link connection Handle */
    phLibNfc_Handle hRemoteDevHandle;       /**< Remote device Handle for peer device */
    uint8_t         SnepServerVersion;          /**< SNEP protocol version supported by Server */
    uint32_t        iInboxSize;                    /* size of inbox buffer */
    uint32_t        iDataTobeReceived;             /* size of NDEF msg inside SNEP packet to be read */
    phLibNfc_Data_t *pDataInbox;            /**< SNEP Connection Inbox */
    pphLibNfc_SnepPut_ntf_t pPutNtfCb;      /**< Put Notification to upper layer */
    void*                   pContextForPutCb;
    pphLibNfc_SnepGet_ntf_t pGetNtfCb;      /**< Get Notification to upper layer */
    phNfc_sData_t sConnWorkingBuffer;       /**< Working buffer for LLCP connection */
    phNfc_sData_t *pSnepWorkingBuffer;      /**< Working buffer for SNEP connection */
    void*                   pContextForGetCb;
    sendResponseDataContext_t responseDataContext; /* context of data transfer transaction */
    uint32_t            iMiu;  /**< Local MIU for DLC connection */
    uint32_t            iRemoteMiu;  /**< Remote MIU for DLC connection */
    phLibNfc_SnepServer_status_t ServerConnectionState; /**< Connection Status */
    void *pConnectionContext;               /**< Connection Context */
} phLibNfc_SnepServerConnection_t, *pphLibNfc_SnepServerConnection_t;

/**
*\ingroup grp_lib_nfc
*
*\brief Single SNEP Server Info
*/
typedef struct SnepServerSession
{
    phLibNfc_Handle hSnepServerHandle;  /**< SNEP Server Data link connection Handle */
    uint8_t SnepServerSap;              /**< SNEP Server Sap on LLCP */
    uint8_t SnepServerVersion;          /**< SNEP protocol version supported by Server */
    uint8_t SnepServerType;             /**< SNEP server type as initialized */
    phNfc_sData_t sWorkingBuffer;       /**< Working buffer for LLCP socket */
    phLibNfc_SnepServer_status_t Server_state;  /**< SNEP Server status */
    phLibNfc_SnepServerConnection_t *pServerConnection[MAX_SNEP_SERVER_CONN];   /**< SNEP server sessions */
    uint8_t CurrentConnCnt;                     /**< Current connection count */
    pphLibNfc_SnepConn_ntf_t pConnectionCb;     /**< Connection notification callback */
    void *pListenContext;                       /**< Server Listen Context */
    bool_t bDtaFlag;                            /**< DTA mode flag */
} phLibNfc_SnepServerSession_t, *pphLibNfc_SnepServerSession_t;


/**
*\ingroup grp_lib_nfc
*
*\brief SNEP Server context. Maintains all active snep server entries
*/
typedef struct SnepServerContext
{
    phLibNfc_SnepServerSession_t *pServerSession[MAX_SNEP_SERVER_CNT];  /**< SNEP server sessions */
    uint8_t CurrentServerCnt;                          /**< Currently registered SNEP servers */
} phLibNfc_SnepServerContext_t, *pphLibNfc_SnepServerContext_t;

/**
*\ingroup grp_lib_nfc
*
*\brief Single SNEP Client Info
*/
typedef struct SnepClientSession
{
    phLibNfc_Handle                 hSnepClientHandle; /**< SNEP Client Data link connection Handle */
    phLibNfc_Handle                 hRemoteDevHandle;  /**< Remote device Handle for peer device */
    uint32_t                        iMiu;              /**< Local MIU for DLC connection */
    uint32_t                        iRemoteMiu;        /**< Remote MIU for DLC connection */
    uint8_t                         SnepClientVersion; /**< SNEP protocol version supported by Client */
    phNfc_sData_t                   sWorkingBuffer;    /**< Working buffer for LLCP socket */
    phLibNfc_SnepClient_status_t    Client_state;      /**< SNEP Client status */
    pphLibNfc_SnepConn_ntf_t        pConnectionCb;     /**< Connection notification callback */
    void                            *pClientContext;   /**< Client Connect Context */
    pphLibNfc_SnepReqCb_t           pReqCb;            /**< Put callback & associated context*/
    void                            *pReqCbContext;
    putGetDataContext_t             putGetDataContext;
    uint32_t                        acceptableLength;  /* set during init */
    bool_t                          bDtaFlag;          /**< DTA mode flag */
} phLibNfc_SnepClientSession_t, *pphLibNfc_SnepClientSession_t;

/**
*\ingroup grp_lib_nfc
*
*\brief SNEP Client context.Maintains all active snep Client entries
*/
typedef struct SnepClientContext
{
    phLibNfc_SnepClientSession_t *pClientSession[MAX_SNEP_CLIENT_CNT];  /**< SNEP Client sessions */
    uint8_t CurrentClientCnt;                                           /**< Currently registered SNEP Clients */
} phLibNfc_SnepClientContext_t, *pphLibNfc_SnepClientContext_t;

/**
* \ingroup grp_lib_nfc
* \brief <b>Interface to create and configure SNEP server</b>.
*
* This function creates and configures the a SNEP server over LLCP.
*
* \param[in] pConfigInfo   Contains SNEP server configuration
* \param[in] pConnCb       This callback is called once LibNfc
*                          for each incoming connection to server.
* \param[out] pServerHandle   Server Session Handle.*
* \param[in] pContext      Upper layer context to be returned in
*                          the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_INSUFFICIENT_RESOURCES   Could not allocate required memory.
* \retval NFCSTATUS_BUFFER_TOO_SMALL         The working buffer is too small for the MIU and RW
*                                            declared in the options.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_ALREADY_REGISTERED       The selected SAP is already bound to another
*                                            socket.
*/
NFCSTATUS phLibNfc_SnepServer_Init( phLibNfc_SnepConfig_t *pConfigInfo,
                                   pphLibNfc_SnepConn_ntf_t pConnCb,
                                   phLibNfc_Handle *pServerHandle,
                                   void *pContext);

/**
* \ingroup grp_lib_nfc
* \brief <b>Interface to accept incoming client connection to server and share inbox </b>.
*
* This function should be called after pphLibNfc_SnepServerConn_ntf_t callback is received
* to indicate the acceptance of incoming connection and sharing of inbox.
*
* \param[in] pDataInbox   Inbox to hold received and saved data.
* \param[in] pSockOps     Socket options (Latest Link Parameters).
* \param[in] hRemoteDevHandle Remote Device Handle.
* \param[in] ServerHandle Server Session Handle.
* \param[in] ConnHandle   Handle to incoming connection.
* \param[in] pPutNtfCb    Put Notification callback for incoming data.
* \param[in] pGetNtfCb    Get Notification callback for incoming data request.
* \param[in] pContext     Upper layer context to be returned in the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_INSUFFICIENT_RESOURCES   Could not allocate required memory.
* \retval NFCSTATUS_BUFFER_TOO_SMALL         The working buffer is too small for the MIU and RW
*                                            declared in the options.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phLibNfc_SnepServer_Accept( phLibNfc_Data_t *pDataInbox,
                                     phLibNfc_Llcp_sSocketOptions_t *pSockOps,
                                     phLibNfc_Handle hRemoteDevHandle,
                                     phLibNfc_Handle ServerHandle,
                                     phLibNfc_Handle ConnHandle,
                                     pphLibNfc_SnepPut_ntf_t pPutNtfCb,
                                     pphLibNfc_SnepGet_ntf_t pGetNtfCb,
                                     void *pContext );


/**
* \ingroup grp_lib_nfc
* \brief <b>Interface Un-initialize server session</b>.
*
* Un-initializes the server session and closes any open connections.
*
* \param[in] ServerHandle   Server Session Handle received in phLibNfc_SnepServer_Init()
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_FAILED                   Operation failed.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
*/
NFCSTATUS phLibNfc_SnepServer_DeInit( phLibNfc_Handle ServerHandle);

/**
* \ingroup grp_lib_nfc
* \brief <b>Interface to create and configure SNEP server</b>.
*
* This function creates and configures the a SNEP server over LLCP.
*
* \param[in] pConfigInfo   Contains Peer SNEP server information
* \param[in] hRemDevHandle  Remote device handle
* \param[in] pConnClientCb  This callback is called once snep client
*                           is initialized and connected to SNEP Server.
* \param[in] pContext      Upper layer context to be returned in
*                          the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_INSUFFICIENT_RESOURCES   Could not allocate required memory.
* \retval NFCSTATUS_BUFFER_TOO_SMALL         The working buffer is too small for the MIU and RW
*                                            declared in the options.
* \retval NFCSTATUS_PENDING                  Client initialization is in progress.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phLibNfc_SnepClient_Init( phLibNfc_SnepConfig_t *pConfigInfo,
                                   phLibNfc_Handle hRemDevHandle,
                                   pphLibNfc_SnepConn_ntf_t pConnClientCb,
                                   void *pContext);

/**
* \ingroup grp_lib_nfc
* \brief <b>Interface Un-initialize client session</b>.
*
* Un-initializes the Client session and closes any open connections.
*
* \param[in] ClientHandle   Client Session Handle received in pphLibNfc_SnepConn_ntf_t()
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_FAILED                   Operation failed.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
*/
NFCSTATUS phLibNfc_SnepClient_DeInit( phLibNfc_Handle ClientHandle);

/**
 * \ingroup grp_snep_nfc
 *
 *  The phLibNfc_SnepProtocolCliReqPut Sends the data to peer and calls the callback function
 *  to confirm completion. It is the responsibility of the caller to deallocate memory returned
 *  in the callback. Deallocation should be done twice once for phNfc_sData_t->buffer and again
 *  for phNfc_sData_t.
 *
 *  \param[in]  ConnHandle              Connection handle to identify session uniquely.
 *  \param[in]  pPutData                Data to send
 *  \param[in]  fCbPut                  Completion notifier callback function
 *  \param[in]  cbContext               Context to be passed to callback function
 *
 *  \retval NFCSTATUS_SUCCESS                   Operation successful.
 *  \retval NFCSTATUS_PENDING                   PUT operation is in progress.
 *  \retval NFCSTATUS_INVALID_PARAMETER         One or more of the supplied parameters
 *                                              could not be interpreted properly.
 *  \retval NFCSTATUS_INSUFFICIENT_RESOURCES    Could not allocate required memory.
 *  \retval NFCSTATUS_INVALID_STATE             The socket is not in a valid state, or not of
 *                                              a valid type to perform the requsted operation.
 *  \retval NFCSTATUS_NOT_INITIALISED           Indicates stack is not yet initialized.
 *  \retval NFCSTATUS_SHUTDOWN                  Shutdown in progress.
 *  \retval NFCSTATUS_FAILED                    Operation failed.
 */
NFCSTATUS phLibNfc_SnepProtocolCliReqPut(phLibNfc_Handle ConnHandle, phNfc_sData_t *pPutData,
                                         pphLibNfc_SnepReqCb_t fCbPut, void *cbContext);

/**
 * \ingroup grp_lib_nfc
 *
 *  The phLibNfc_SnepProtocolCliReqGet Sends a request to receive data from peer and calls the callback function
 *  to confirm receipt. It is the responsibility of the caller to deallocate memory returned in the callback.
 *  Deallocation should be done twice once for phNfc_sData_t->buffer and again for phNfc_sData_t.
 *
 *  \param[in]  ConnHandle              Connection handle to identify session uniquely.
 *  \param[in]  pGetData                Data sent to server as part of GET request.
 *  \param[in]  fCbGet                  Reception notifier callback function.
 *  \param[in]  acceptable_length       Max length of expected data.
 *  \param[in]  cbContext               Context to be passed to callback function.
 *
 *  \retval NFCSTATUS_SUCCESS                   Operation successful.
 *  \retval NFCSTATUS_INSUFFICIENT_RESOURCES    Could not allocate required memory.
 *  \retval NFCSTATUS_INVALID_PARAMETER         One or more of the supplied parameters
 *                                              could not be properly interpreted.
 *  \retval NFCSTATUS_PENDING                   GET operation is in progress.
 *  \retval NFCSTATUS_INVALID_STATE             The socket is not in a valid state, or not of
 *                                              a valid type to perform the requsted operation.
 *  \retval NFCSTATUS_NOT_INITIALISED           Indicates stack is not yet initialized.
 *  \retval NFCSTATUS_SHUTDOWN                  Shutdown in progress.
 *  \retval NFCSTATUS_FAILED                    Operation failed.
 */
NFCSTATUS phLibNfc_SnepProtocolCliReqGet(phLibNfc_Handle ConnHandle, phNfc_sData_t *pGetData, uint32_t acceptable_length,
                                         pphLibNfc_SnepReqCb_t fCbGet, void *cbContext);

/**
* \ingroup grp_lib_nfc
*
* \brief  Generic Server Response Callback definition.
*
* Generic callback definition used to indicate sending of Server response.
*
* \note : Status and error codes for this type of callback are documented in respective APIs
* wherever it is used.
*
* \param[in] pContext       LibNfc client context   passed in the corresponding request before.
* \param[in] status         Status of the response  callback.
* \param[in] ConnHandle     Snep Incoming Connection Handle.
*/
typedef void(*pphLibNfc_SnepProtocol_SendRspComplete_t) (void* pContext, NFCSTATUS  Status,
                                                         phLibNfc_Handle ConnHandle);

/**
 * \ingroup grp_lib_nfc
 *
 *  \param[in]  ConnHandle              Connection handle to identify session uniquely.
 *  \param[in]  pResponseData           Data sent to client as reponse to a request.
 *  \param[in]  responseStatus          Response status.
 *  \param[in]  fSendCompleteCb         Response sent notifier callback function.
 *  \param[in]  cbContext               Context to be passed to callback function.
 *
 *  \retval NFCSTATUS_SUCCESS                   Operation successful.
 *  \retval NFCSTATUS_INSUFFICIENT_RESOURCES    Could not allocate required memory.
 *  \retval NFCSTATUS_INVALID_PARAMETER         One or more of the supplied parameters
 *                                              could not be properly interpreted.
 *  \retval NFCSTATUS_PENDING                   Server-response operation is in progress.
 *  \retval NFCSTATUS_INVALID_STATE             The socket is not in a valid state, or not of
 *                                              a valid type to perform the requsted operation.
 *  \retval NFCSTATUS_NOT_INITIALISED           Indicates stack is not yet initialized.
 *  \retval NFCSTATUS_SHUTDOWN                  Shutdown in progress.
 *  \retval NFCSTATUS_FAILED                    Operation failed.
 */
NFCSTATUS phLibNfc_SnepProtocolSrvSendResponse(phLibNfc_Handle ConnHandle, phNfc_sData_t *pResponseData, NFCSTATUS responseStatus,
                                                pphLibNfc_SnepProtocol_SendRspComplete_t fSendCompleteCb, void *cbContext);
