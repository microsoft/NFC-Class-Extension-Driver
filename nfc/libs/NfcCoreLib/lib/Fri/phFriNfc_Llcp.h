/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#pragma once

#include <phNfcTypes.h>
#include <phNfcLlcpTypes.h>
#include <phNfcStatus.h>
#include <phFriNfc.h>

#include <phFriNfc_LlcpMac.h>

#define PHFRINFC_LLCP_VERSION_MAJOR   0x01  /**< Major number of local LLCP version.*/
#define PHFRINFC_LLCP_VERSION_MINOR   0x01  /**< Minor number of local LLCP version.*/
#define PHFRINFC_LLCP_VERSION         ((PHFRINFC_LLCP_VERSION_MAJOR << 4) | PHFRINFC_LLCP_VERSION_MINOR) /**< Local LLCP version.*/

#define PHFRINFC_LLCP_PTYPE_SYMM       0x00 /**< Symmetry.*/
#define PHFRINFC_LLCP_PTYPE_PAX        0x01 /**< PArameter Exchange.*/
#define PHFRINFC_LLCP_PTYPE_AGF        0x02 /**< AGgregated Frame.*/
#define PHFRINFC_LLCP_PTYPE_UI         0x03 /**< Unnumbered Information.*/
#define PHFRINFC_LLCP_PTYPE_CONNECT    0x04 /**< Connect.*/
#define PHFRINFC_LLCP_PTYPE_DISC       0x05 /**< Disconnect.*/
#define PHFRINFC_LLCP_PTYPE_CC         0x06 /**< Connection Complete.*/
#define PHFRINFC_LLCP_PTYPE_DM         0x07 /**< Disconnected Mode.*/
#define PHFRINFC_LLCP_PTYPE_FRMR       0x08 /**< FRaMe Reject.*/
#define PHFRINFC_LLCP_PTYPE_SNL        0x09 /**< Service Name Lookup.*/
#define PHFRINFC_LLCP_PTYPE_RESERVED1  0x0A /**< Reserved.*/
#define PHFRINFC_LLCP_PTYPE_RESERVED2  0x0B /**< Reserved.*/
#define PHFRINFC_LLCP_PTYPE_I          0x0C /**< Information.*/
#define PHFRINFC_LLCP_PTYPE_RR         0x0D /**< Receive Ready.*/
#define PHFRINFC_LLCP_PTYPE_RNR        0x0E /**< Receive Not Ready.*/
#define PHFRINFC_LLCP_PTYPE_RESERVED3  0x0F /**< Reserved.*/

#define PHFRINFC_LLCP_SAP_LINK                     0x00 /**< Link SAP.*/
#define PHFRINFC_LLCP_SAP_SDP                      0x01 /**< Service Discovery Protocol SAP.*/
#define PHFRINFC_LLCP_SAP_WKS_FIRST                0x02 /**< Other Well-Known Services defined by the NFC Forum.*/
#define PHFRINFC_LLCP_SAP_SDP_ADVERTISED_FIRST     0x10 /**< First SAP number from SDP-avertised SAP range.*/
#define PHFRINFC_LLCP_SAP_SDP_UNADVERTISED_FIRST   0x20 /**< First SAP number from SDP-unavertised SAP range.*/
#define PHFRINFC_LLCP_SAP_NUMBER                   0x40 /**< Number of possible SAP values (also first invalid value).*/
#define PHFRINFC_LLCP_SAP_DEFAULT                  0xFF /**< Default number when a socket is created or reset */
#define PHFRINFC_LLCP_SDP_ADVERTISED_NB            0x10 /**< Number of SDP advertised SAP slots */

#define PHFRINFC_LLCP_SERVICENAME_SDP              "urn:nfc:sn:sdp" /**< Service Discovery Protocol name.*/

#define PHFRINFC_LLCP_DM_LENGTH                    0x01 /**< Length value for DM opCode */

#define PHFRINFC_LLCP_TLV_MIUX_MASK                 0x07FF   /**< \internal Mask to apply to MIUX TLV Value.*/
#define PHFRINFC_LLCP_TLV_WKS_MASK                  0x0001   /**< \internal Minimal bits to be set in WKS TLV Value.*/
#define PHFRINFC_LLCP_TLV_RW_MASK                   0x0F     /**< \internal Mask to apply to RW TLV Value.*/
#define PHFRINFC_LLCP_TLV_OPT_MASK                  0x03     /**< \internal Mask to apply to OPT TLV Value.*/

#define PHFRINFC_LLCP_TLV_TYPE_VERSION               0x01   /**< \internal VERSION parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_MIUX                  0x02   /**< \internal MIUX parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_WKS                   0x03   /**< \internal WKS parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_LTO                   0x04   /**< \internal LTO parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_RW                    0x05   /**< \internal RW parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_SN                    0x06   /**< \internal SN parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_OPT                   0x07   /**< \internal OPT parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_SDREQ                 0x08   /**< \internal SDREQ parameter Type code.*/
#define PHFRINFC_LLCP_TLV_TYPE_SDRES                 0x09   /**< \internal SDRES parameter Type code.*/

#define PHFRINFC_LLCP_TLV_LENGTH_HEADER              2   /**< \internal Fixed length of Type and Length fields in TLV.*/
#define PHFRINFC_LLCP_TLV_LENGTH_VERSION             1   /**< \internal Fixed length of VERSION parameter Value.*/
#define PHFRINFC_LLCP_TLV_LENGTH_MIUX                2   /**< \internal Fixed length of MIUX parameter Value.*/
#define PHFRINFC_LLCP_TLV_LENGTH_WKS                 2   /**< \internal Fixed length of WKS parameter Value.*/
#define PHFRINFC_LLCP_TLV_LENGTH_LTO                 1   /**< \internal Fixed length of LTO parameter Value.*/
#define PHFRINFC_LLCP_TLV_LENGTH_RW                  1   /**< \internal Fixed length of RW parameter Value.*/
#define PHFRINFC_LLCP_TLV_LENGTH_OPT                 1   /**< \internal Fixed length of OPT parameter Value.*/

#define PHFRINFC_LLCP_PACKET_HEADER_SIZE     2 /**< Size of the general packet header (DSAP+PTYPE+SSAP).*/
#define PHFRINFC_LLCP_PACKET_SEQUENCE_SIZE   1 /**< Size of the sequence field, if present.*/
#define PHFRINFC_LLCP_PACKET_MAX_SIZE        (PHFRINFC_LLCP_PACKET_HEADER_SIZE + \
                                             PHFRINFC_LLCP_PACKET_SEQUENCE_SIZE + \
                                             PHFRINFC_LLCP_MIU_DEFAULT + \
                                             PHFRINFC_LLCP_TLV_MIUX_MASK) /**< Maximum size of a packet */

#define ABS_SUB(a,b) ((a)>(b)?((a)-(b)):((b)-(a)))
#define CHECK_SEND_RW(socket) ( (ABS_SUB((socket)->socket_VS, (socket)->socket_VSA)  % 16) < (socket)->remoteRW )

typedef phFriNfc_LlcpMac_ePeerType_t      phFriNfc_Llcp_eRole_t;
typedef phFriNfc_LlcpMac_eLinkStatus_t    phFriNfc_Llcp_eLinkStatus_t;

typedef void (*phFriNfc_Llcp_Check_CB_t) (
   void                             *pContext,
   NFCSTATUS                        status
);

typedef void (*phFriNfc_Llcp_LinkStatus_CB_t) (
   void                             *pContext,
   phFriNfc_Llcp_eLinkStatus_t      eLinkStatus
);

typedef void (*phFriNfc_Llcp_Send_CB_t) (
   void                             *pContext,
   NFCSTATUS                        status
);

typedef void (*phFriNfc_Llcp_Recv_CB_t) (
   void                             *pContext,
   phNfc_sData_t                    *psData,
   NFCSTATUS                        status
);

typedef struct phFriNfc_Llcp_sPacketHeader
{
   /**< The destination service access point*/
   unsigned   dsap : 6;

   /**< The packet type*/
   unsigned   ptype : 4;

   /**< The source service access point*/
   unsigned   ssap : 6;

} phFriNfc_Llcp_sPacketHeader_t;

typedef struct phFriNfc_Llcp_sPacketSequence
{
   /**< Sequence number for sending*/
   unsigned   ns : 4;

   /**< Sequence number for reception*/
   unsigned   nr : 4;

} phFriNfc_Llcp_sPacketSequence_t;

typedef struct phFriNfc_Llcp_sSendOperation
{
   /**< Sequence number for sending*/
   phFriNfc_Llcp_sPacketHeader_t    *psHeader;

   /**< Sequence number for sending*/
   phFriNfc_Llcp_sPacketSequence_t  *psSequence;

   /**< Sequence number for sending*/
   phNfc_sData_t                    *psInfo;

   /**< Sequence number for sending*/
   phFriNfc_Llcp_Send_CB_t          pfSend_CB;

   /**< Sequence number for sending*/
   void                             *pContext;

} phFriNfc_Llcp_sSendOperation_t;

typedef struct phFriNfc_Llcp_sRecvOperation
{
   /**< Sequence number for sending*/
   uint8_t                    nSap;

   /**< Sequence number for sending*/
   phNfc_sData_t              *psBuffer;

   /**< Sequence number for sending*/
   phFriNfc_Llcp_Recv_CB_t    pfRecv_CB;

   /**< Sequence number for sending*/
   void                       *pContext;

} phFriNfc_Llcp_sRecvOperation_t;

typedef  void (*pphFriNfc_Llcp_DeferFuncPointer_t) (void*);

typedef struct
{
    pphFriNfc_Llcp_DeferFuncPointer_t def_call;
    void* params;
} phFriNfc_Llcp_DeferMsg_t;

typedef struct phFriNfc_Llcp
{
   /**< The current state*/
   uint8_t                          state;

   /**< MAC mapping instance*/
   phFriNfc_LlcpMac_t               MAC;

   /**< Local LLC role*/
   phFriNfc_LlcpMac_eType_t         eRole;

   /**< Activity counter, used to modulate symmetry timeout*/
   uint8_t                          nSymmetryCounter;

   /**< Local link parameters*/
   phFriNfc_Llcp_sLinkParameters_t  sLocalParams;

   /**< Remote link parameters*/
   phFriNfc_Llcp_sLinkParameters_t  sRemoteParams;

   /**< Negociated protocol version (major number on MSB, minor on LSB)*/
   uint8_t                          version;

   /**< Internal reception buffer, its size may vary during time but not exceed nRxBufferSize*/
   phNfc_sData_t                   sRxBuffer;

   /**<Actual base pointer of the reception buffer*/
   uint8_t                         *pRxBuffer;

   /**< Actual size of reception buffer*/
   uint16_t                        nRxBufferLength;

   /**< Internal emission buffer, its size may vary during time but not exceed nTxBufferSize*/
   phNfc_sData_t                   sTxBuffer;

   /**< A message structure for deferred Symm. */
   phFriNfc_Llcp_DeferMsg_t        sDeferredSymm;

   /**< Actual size of emission buffer*/
   uint16_t                        nTxBufferLength;

   /**< Callback function for link status notification*/
   phFriNfc_Llcp_LinkStatus_CB_t    pfLink_CB;

   /**< Callback context for link status notification*/
   void                             *pLinkContext;

   /**< Callback function for compliance checking*/
   phFriNfc_Llcp_Check_CB_t         pfChk_CB;

   /**< Callback context for compliance checking*/
   void                             *pChkContext;

   /**< Symmetry timer*/
   uint32_t                         hSymmTimer;

   /**< Control frames buffer*/
   uint8_t                          pCtrlTxBuffer[10];

   /**< Control frames buffer size*/
   uint8_t                          pCtrlTxBufferLength;

   /**< DISC packet send pending flag*/
   bool_t                           bDiscPendingFlag;

   /**< FRMR packet send pending flag*/
   bool_t                           bFrmrPendingFlag;

   /**< Header of pending FRMR packet*/
   phFriNfc_Llcp_sPacketHeader_t    sFrmrHeader;

   /**< Info field of pending FRMR packet*/
   uint8_t                          pFrmrInfo[4];

   /**< Send callback*/
   phFriNfc_Llcp_Send_CB_t          pfSendCB;

   /**< Send callback*/
   void                             *pSendContext;

   /**< Pending send header*/
   phFriNfc_Llcp_sPacketHeader_t    *psSendHeader;

   /**< Pending send sequence*/
   phFriNfc_Llcp_sPacketSequence_t  *psSendSequence;

   /**< Pending send info*/
   phNfc_sData_t                    *psSendInfo;

   /**< Receive callback*/
   phFriNfc_Llcp_Recv_CB_t          pfRecvCB;

   /**< Receive callback*/
   void                             *pRecvContext;

   /**< DTA mode flag*/
   bool_t                           bDtaFlag;

} phFriNfc_Llcp_t;

NFCSTATUS
phFriNfc_Llcp_EncodeLinkParams(
    _In_ phNfc_sData_t                   *psRawBuffer,
    _In_ phFriNfc_Llcp_sLinkParameters_t *psLinkParams,
    _In_ uint8_t                         nVersion
    );

NFCSTATUS
phFriNfc_Llcp_Reset(
    _Out_                                       phFriNfc_Llcp_t                 *Llcp,
    _In_                                        void                            *LowerDevice,
    _In_                                        phFriNfc_Llcp_sLinkParameters_t *psLinkParams,
    _Pre_writable_byte_size_(nRxBufferLength)   void                            *pRxBuffer,
    _In_                                        uint16_t                        nRxBufferLength,
    _Pre_writable_byte_size_(nTxBufferLength)   void                            *pTxBuffer,
    _In_                                        uint16_t                        nTxBufferLength,
    _In_                                        bool_t                          bDtaFlag,
    _In_                                        phFriNfc_Llcp_LinkStatus_CB_t   pfLink_CB,
    _In_                                        void                            *pContext
    );

NFCSTATUS
phFriNfc_Llcp_ChkLlcp(
    _Inout_ phFriNfc_Llcp_t                 *Llcp,
    _In_    phHal_sRemoteDevInformation_t   *psRemoteDevInfo,
    _In_    phFriNfc_Llcp_Check_CB_t        pfCheck_CB,
    _In_    void                            *pContext
    );

NFCSTATUS
phFriNfc_Llcp_Activate(
    _In_ phFriNfc_Llcp_t *Llcp
    );

NFCSTATUS
phFriNfc_Llcp_Deactivate(
    _In_ phFriNfc_Llcp_t *Llcp
    );

NFCSTATUS
phFriNfc_Llcp_GetLocalInfo(
    _In_    const phFriNfc_Llcp_t           *Llcp,
    _Out_   phFriNfc_Llcp_sLinkParameters_t *pParams
    );

NFCSTATUS
phFriNfc_Llcp_GetRemoteInfo(
    _In_    const phFriNfc_Llcp_t           *Llcp,
    _Out_   phFriNfc_Llcp_sLinkParameters_t *pParams
    );

NFCSTATUS
phFriNfc_Llcp_Send(
    _Inout_ phFriNfc_Llcp_t                 *Llcp,
    _In_    phFriNfc_Llcp_sPacketHeader_t   *psHeader,
    _In_    phFriNfc_Llcp_sPacketSequence_t *psSequence,
    _In_    phNfc_sData_t                   *psInfo,
    _In_    phFriNfc_Llcp_Send_CB_t         pfSend_CB,
    _In_    void                            *pContext
    );

NFCSTATUS
phFriNfc_Llcp_Recv(
    _In_ phFriNfc_Llcp_t         *Llcp,
    _In_ phFriNfc_Llcp_Recv_CB_t pfRecv_CB,
    _In_ void                    *pContext
    );
