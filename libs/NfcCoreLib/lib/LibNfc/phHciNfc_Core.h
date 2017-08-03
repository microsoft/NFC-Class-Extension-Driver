/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>


#define PHHCINFC_HCP_HEADER_LEN                      0x02U
#define PHHCINFC_HCP_PACKET_HEADER_OFFSET            0x00U

#define PHHCINFC_HCP_MESSAGE_LEN                     0x01U
#define PHHCINFC_HCP_MESSAGE_HEADER_OFFSET           0x01U
#define PHHCINFC_HCP_MESSAGE_PAYLOAD_OFFSET           0x02U

#define PHHCINFC_HCP_CHAINBIT_OFFSET                  0x07U
#define PHHCINFC_HCP_CHAINBIT_LEN                     0x01U

#define HCP_CHAINBIT_UN_CHAINED                       0x01U
#define HCP_CHAINBIT_CHAINED                          0x00U

#define PHHCINFC_HCP_PIPEID_OFFSET                    0x00U
#define PHHCINFC_HCP_PIPEID_LEN                       0x07U

#define PHHCINFC_HCP_MSG_TYPE_OFFSET                  0x06U
#define PHHCINFC_HCP_MSG_TYPE_LEN                     0x02U

#define PHHCINFC_HCP_MSG_INSTRUCTION_OFFSET           0x00U
#define PHHCINFC_HCP_MSG_INSTRUCTION_LEN              0x06U

#define PHHCI_HCP_MSG_TYPE_LEN                        0x02U

#define PHHCI_HCP_MSG_INSTRUCTION_OFFSET              0x00U
#define PHHCI_HCP_MSG_INSTRUCTION_LEN                 0x06U

#define PHHCI_HCP_MAX_PACKET_SIZE                     (255U)
#define PHHCI_HCP_MAX_RECEIVE_PACKET_SIZE             (260U)
#define PHHCI_HCI_MAX_PACKET_PAYLOAD_SIZE             (PHHCI_HCP_MAX_PACKET_SIZE - PHHCINFC_HCP_HEADER_LEN)

#define PHHCI_HCI_PACKET_FRAG_HEADER_LEN              0x01U
#define PHHCI_HCI_FRAG_PACKET_PAYLOAD_TOTAL_LENGTH    (PHHCI_HCP_MAX_PACKET_SIZE - PHHCI_HCI_PACKET_FRAG_HEADER_LEN)

typedef void (*pphHciNfc_SendCb_t) (void *pContext, NFCSTATUS wStatus);
typedef void (*pphHciNfc_UpperLayerSendCb_t) (void *pContext, NFCSTATUS wStatus);

typedef struct phHciNfc_SendParams
{
    uint8_t             bPipeId;       /**<HCI Message Pipe Identifier */
    uint8_t             bIns;          /**<HCI Message Instruction Type.*/
    uint8_t             bMsgType;      /**<HCI Message Type.*/
    uint32_t            dwLen;         /**<HCI Message Payload Length.*/
    uint8_t             *pData;        /**<HCI Message Payload Data.*/
} phHciNfc_SendParams_t;

typedef struct phHciNfc_ReceiveParams
{
    uint8_t             bPipeId;       /**<HCI Message Pipe Identifier */
    uint8_t             bIns;          /**<HCI Message Instruction Type.*/
    uint8_t             bMsgType;      /**<HCI Message Type.*/
    uint16_t            wLen;          /**<HCI Message Payload Length.*/
    uint8_t             *pData;        /**<HCI Message Payload Data.*/
}phHciNfc_ReceiveParams_t, *pphHciNfc_ReceiveParams_t;

typedef struct phHciNfc_HcpMessage
{
    uint8_t     bMsgHeader;                                        /**<Identifies the Type and Kind of Instruction */
    uint8_t     aHcpPktPayload[PHHCI_HCI_MAX_PACKET_PAYLOAD_SIZE]; /**<Variable length Packet Message Payload */
} phHciNfc_HcpMessage_t;

typedef struct phHciNfc_HcpPacket
{
    uint8_t     bHcpPktHeader;              /**<Chaining Information and Pipe Identifier */
    phHciNfc_HcpMessage_t     tHcpMessage;  /**<Host Controller Protocol (HCP) Packet Message */
} phHciNfc_HcpPacket_t;

typedef struct phHciNfc_HciFragmentationInfo
{
    uint32_t                dwTxdHcpPayloadNoOfDataBytes; /**<Total Number of Data Bytes Transmitted */
} phHciNfc_HciFragmentationInfo_t;

typedef struct phHciNfc_sCoreMem
{
    /* Currently, only one packet shall be handled */
    uint8_t  aBuffer[PHHCI_HCP_MAX_RECEIVE_PACKET_SIZE];  /**<Buffer to be used for storing the received packet*/
    uint16_t wLen;                                /**<Indicates length data in the buffer*/
}phHciNfc_sCoreMem_t,*pphHciNfc_sCoreMem_t;       /**< pointer to #phHciNfc_sCoreMem_t structure */

typedef struct phHciNfc_sCoreRecvBuff_List
{
    phHciNfc_sCoreMem_t tMem;                                   /**<Buffer for storing received packet's payload*/
    struct phHciNfc_sCoreRecvBuff_List *pNext;                  /**<Pointer to the next node present in linked list*/
}phHciNfc_sCoreRecvBuff_List_t,*pphHciNfc_sCoreRecvBuff_List_t; /**<pointer to
                                                                 #phHciNfc_sCoreRecvBuff_List_t structure*/

typedef struct phHciNfc_HciCoreReceiveInfo
{
    phHciNfc_sCoreRecvBuff_List_t ListHead;                     /**<Linked list head*/
    uint16_t wNumOfNodes;                                       /**<Total number of nodes in the lsit*/
    uint16_t wPayloadSize;                                      /**<Cummulative size of payloads present all nodes*/
}phHciNfc_HciCoreReceiveInfo_t,*pphHciNfc_HciCoreReceiveInfo_t; /**< pointer to #phNciNfc_sCoreReceiveInfo_t */

typedef struct phHciNfc_HciCoreContext
{
    phHciNfc_HciFragmentationInfo_t  tHciFragInfo;                 /**<HCI Fragmentation Info */
    phHciNfc_SendParams_t            tHciCtxSendMsgParams;        /**<To Store Parameters passed to HCI Send Cmd */
    pphHciNfc_UpperLayerSendCb_t     phHciNfcCoreUpperLayerSendCb; /**<To Store Upper Layer Send Call back */
    void                             *pUpperLayerContext;          /**<To Store Upper Layer Context */
    phHciNfc_ReceiveParams_t         tHciCtxRxMsgParams;
    phHciNfc_HciCoreReceiveInfo_t    tReceiveInfo;              /**< Contains Received packet and its information */
} phHciNfc_HciCoreContext_t;

/* TBD- The Protoype for this function will be defined by Lower Layer,
** hence needs modificaiton after API is avaialble */
NFCSTATUS
phHciNfc_CoreRecvCB( void                           *pContext,
                     void                           *pInfo,
                     NFCSTATUS                      wStatus
                   );

extern NFCSTATUS
phHciNfc_CoreSend(void                              *pHciContext,
                  phHciNfc_SendParams_t             *pSendParam,
                  pphHciNfc_UpperLayerSendCb_t      phHciNfcUpperLayerSendCb,
                  void                              *pContext
                 );

extern NFCSTATUS phHciNfc_CoreInit(void *pContext);
