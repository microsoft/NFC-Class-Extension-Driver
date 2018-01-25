/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phHciNfc_Interface.h"

#define PHHCINFC_LINK_MGMT_GATEID           0x04U

#define PHHCINFC_PIPE_GENERICCMD_BYTE_OFFSET    0x01U

#define PHHCINFC_IDENTITY_LENGTH            0x01U
#define PHHCINFC_PIPE_SESSIONID_LEN         0x08U
#define PHHCINFC_ANY_GET_PARAMETER_LEN      0x01U
#define PHHCINFC_CREATE_PIPE_REQ_LEN        0x05U

#define PHHCINFC_MAX_PIPES                  0x03

/*
    Note on Session ID Management:
    The Session id is implemented as per IR SC32582.
    The 8 bytes of Session id is used to track the pipes created.
    Session Id  FF FF FF FF  FF FF FF FF.
    Byte nos-   0  1   2  3  4   5  6  7
    Byte 7 - Informs about pipe presence bit wise. 1 – no pipe 0 – pipe present
    ( values expected FF for no pipe created. 7F -1 pipe created. 3F -2 pipes created)
    Byte 6 - Pipe ID for eSE (APDU gate)
    Byte 5 - Pipe ID for eSE (connectivity gate)
    Byte 3 - Not used
    Byte 3 - Pipe ID for UICC.
*/
#define PHHCI_PIPE_PRESENCE_INDEX                    7

/* Macros used to Store Pipe Info of eSE for APDU gate in Session ID*/
#define PHHCI_ESE_APDU_PIPE_STORAGE_INDEX                6
#define PHHCI_ESE_APDU_PIPE_CREATED_BIT_INDEX            7
#define PHHCI_ESE_APDU_PIPE_LIST_INDEX                   0

/* Macros used to Store Pipe Info of eSE for APDU gate in Session ID*/
#define PHHCI_ESE_CONNECTIVITY_PIPE_STORAGE_INDEX        5
#define PHHCI_ESE_CONNECTIVITY_PIPE_CREATED_BIT_INDEX    6
#define PHHCI_ESE_CONN_PIPE_LIST_INDEX                   1

/* Macros used to Store Pipe Info of UICC for connectivity gate in Session ID*/
#define PHHCI_UICC_CONNECTIVITY_PIPE_STORAGE_INDEX       3
#define PHHCI_UICC_CONNECTIVITY_PIPE_CREATED_BIT_INDEX   3
#define PHHCI_UICC_CONN_PIPE_LIST_INDEX                  0

typedef struct phHciNfc_AdmNotfPipeCrCmdParams
{
    uint8_t     bSourceHID;
    uint8_t     bSourceGID;
    uint8_t     bDestHID;
    uint8_t     bDestGID;
    uint8_t     bPipeID;
} phHciNfc_AdmNotfPipeCrCmdParams_t;

typedef enum phNciNfc_eHciMsgType
{
    phHciNfc_e_HciMsgTypeCmd = 0,    /**<Hci command message type*/
    phHciNfc_e_HciMsgTypeEvent,       /**<Hci event message type*/
    phHciNfc_e_HciMsgTypeRsp        /**<Hci response message type*/
}phNciNfc_eHciMsgType_t;

typedef struct phHciNfc_HciRegData
{
    phNciNfc_eHciMsgType_t  eMsgType;
    uint8_t                 bPipeId;       /**< Group ID */
}phHciNfc_HciRegData_t, *pphHciNfc_HciRegData_t; /**< pointer to #phHciNfc_HciRegData_t */

typedef struct phHciNfc_AdmPipeCreateCmdParams
{
    uint8_t     bSourceGID;
    uint8_t     bDestHID;
    uint8_t     bDestGID;
} phHciNfc_AdmPipeCreateCmdParams_t;

extern NFCSTATUS
phHciNfc_Transceive(
                        void                *pHciContext,
                        uint8_t             bpipeId,
                        uint8_t             bEvent,
                        uint32_t            dwDataLen,
                        uint8_t             *pdata,
                        pphHciNfc_RspCb_t    pRspCb,
                        void                *pContext
                     );

extern
void phHciNfc_ReceiveHandler(void *pContext,phHciNfc_ReceiveParams_t *pReceivedParams, NFCSTATUS wStatus);

extern NFCSTATUS
phHciNfc_RegisterCmdRspEvt(
                        void                    *pHciContext,
                        pphHciNfc_HciRegData_t  pHciRegData,
                        pphHciNfc_RspCb_t       pRegisterCb,
                        void                    *pContext);

extern NFCSTATUS
phHciNfc_AddRegistration(
                        phHciNfc_RegInfo_t *pList,
                        uint8_t             bNumOfEntries,
                        uint8_t             bPipeId,
                        pphHciNfc_RspCb_t   pRspCb,
                        void                *pContext);

extern NFCSTATUS
phHciNfc_VerifyIfRegistered(
                        phHciNfc_RegInfo_t *pList,
                        uint8_t             bNumOfEntries,
                        uint8_t bPipeId,
                        pphHciNfc_RspCb_t   pRspCb);

extern
NFCSTATUS
phHciNfc_UnRegisterCmdRspEvt(
                        void                    *pHciContext,
                        pphHciNfc_HciRegData_t  pHciRegData,
                        pphHciNfc_RspCb_t       pRegisterCb);

extern
NFCSTATUS
phHciNfc_RemoveRegistration(
                        phHciNfc_RegInfo_t *pList,
                        uint8_t             bNumOfEntries,
                        uint8_t             bPipeId,
                        pphHciNfc_RspCb_t   pRspCb);

extern
NFCSTATUS
phHciNfc_OpenPipe(
                        void                    *pHciContext,
                        uint8_t                 bPipeId,
                        pphHciNfc_RspCb_t       pRspCb,
                        void                    *pContext
                     );

extern
NFCSTATUS
phHciNfc_AnyGetParameter(
                        void                *pHciContext,
                        uint8_t             bGateId,
                        uint8_t             bIdentifier,
                        uint8_t             bPipeId,
                        pphHciNfc_RspCb_t   pRspCb,
                        void                *pContext
                     );

extern
NFCSTATUS
phHciNfc_AnySetParameter(
                        void                *pHciContext,
                        uint8_t             bIdentifier,
                        uint8_t             bPipeId,
                        uint8_t             bSetDataLen,
                        const uint8_t       *pSetData,
                        pphHciNfc_RspCb_t   pRspCb,
                        void                *pContext
                     );

extern
void
phHciNfc_ReceiveAdminNotifyCmd( void *pContext,NFCSTATUS wStatus, void *pInfo );

extern
void
phHciNfc_ReceiveAdminNotifyEvt( void *pContext,NFCSTATUS wStatus, void *pInfo );

extern
void
phHciNfc_ReceiveOpenPipeNotifyCmd( void *pContext,NFCSTATUS wStatus, void *pInfo );

extern
void
phHciNfc_ProcessEventsOnPipe( void *pContext,NFCSTATUS wStatus, void *pInfo );

extern
NFCSTATUS
phHciNfc_GetPipeId(void *pContext, uint8_t *bPipeId);

extern void phHciNfc_CmdSendCb(void *pContext, NFCSTATUS wStatus);



/**
*  \ingroup grp_hci_nfc_pipe
*
*  \brief This function shall be invoked to Create a HCI pipe
*
*  \param[in] pHciContext   - pointer to the HCI context
*  \param[in] bDestHostId   - Destination Host ID
*  \param[in] bPipeId       - Pipe Id
*  \param[in] pRspCb        - Call Back function from upper layer
*  \param[in] pContext      - Context of upper Layer
*
*  \return Nfc status
*/
extern
NFCSTATUS
phHciNfc_CreatePipe(
    void *pHciContext,
    phHciNfc_AdmPipeCreateCmdParams_t Pipedata,
    pphHciNfc_RspCb_t pRspCb,
    void *pContext
    );
