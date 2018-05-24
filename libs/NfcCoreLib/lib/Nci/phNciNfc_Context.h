/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNfcTypes.h"
#include "phNciNfc.h"
#include "phNciNfc_Core.h"
#include "phNciNfc_State.h"
#include "phNciNfc_Sequence.h"
#include "phNciNfc_Common.h"
#include "phNciNfc_CoreIf.h"
#include "phNciNfc_NfceeMgmt.h"
#include "phNciNfc_Discovery.h"

typedef struct phNciNfc_Context
{
    phNciNfc_Config_t Config;
    phNciNfc_sInitRspParams_t InitRspParams;    /**<Structure object to #phNciNfc_sInitRspParams_t*/
    phNciNfc_ResetInfo_t ResetInfo;             /**<Structure object to #phNciNfc_ResetInfo_t*/
    phNciNfc_InitInfo_t tInitInfo;              /**<Structure object to #phNciNfc_InitInfo_t*/
    phNciNfc_DiscContext_t NciDiscContext;      /**<Structure object to #phNciNfc_DiscContext_t*/
    phNciNfc_RfDeActvInfo_t  tDeActvInfo;       /**<Structure object to #phNciNfc_RfDeActvInfo_t*/
    phNciNfc_RegListenerInfo_t tRegListInfo;    /**<Structure object to #phNciNfc_RegListenerInfo_t*/
    phNciNfc_RegForSyncNtf_t tRegSyncInfo;      /**<Structure object to #phNciNfc_RegForSyncNtf_t*/
    phNciNfc_RfConfigCtx_t tRfConfContext;        /**<Structure object to #phNciNfc_RfConfigCtx_t*/
    phNciNfc_SetConfigOptInfo_t tSetConfOptInfo;  /**< Structure holding upper layer set config info and get config param info*/
    phNciNfc_SendPayload_t tSendPayload;        /**<Structure object to #phNciNfc_SendPayload_t*/
    phNciNfc_RtngConfInfo_t tRtngConfInfo;      /**<Structure object to #phNciNfc_RtngConfInfo_t*/
    phNciNfc_CoreContext_t NciCoreContext;      /**<Structure object to #phNciNfc_CoreContext_t*/
    pphNciNfc_IfNotificationCb_t IfNtf;         /**<Pointer to upper layer call back function*/
    void *IfNtfCtx;                             /**<Pointer to upper layer context*/
    phNciNfc_SequenceP_t *pSeqHandler;          /**<Pointer to #phNciNfc_SequenceP_t*/
    phNciNfc_Buff_t RspBuffInfo;                /**<Buffer to store payload field of the received response*/
    uint8_t SeqNext;                            /**<Next sequence*/
    uint8_t SeqMax;                             /**<Maximum number of sequences*/
    phNciNfc_LogConnContext_t tLogConnCtxt;     /**<Logical connection context*/
    phNciNfc_NfceeContext_t tNfceeContext;      /**<Nfcee module context specific information*/
    phNciNfc_ActiveDeviceInfo_t tActvDevIf;     /**< Reader Mgmt info */
    phNciNfc_TranscvContext_t tTranscvCtxt;     /**<Remote device transaction context */
    phNciNfc_LstnModeRecvInfo_t tLstnModeRecvInfo;  /**<Holds information Data if received before
                                                        application invokes "RemoteDev_Receive" API  in listen mode*/
    void *pUpperLayerInfo;                      /**< Used to store data pointer of Upper layer */
    uint32_t dwNtfTimerId;                      /**< Timer for to handle NTF*/
    phNciNfc_SeEventList_t  tSeEventList;       /**< Structure holding Se event registrations*/
}phNciNfc_Context_t, *pphNciNfc_Context_t;      /**< pointer to #phNciNfc_Context_t structure */

extern pphNciNfc_Context_t phNciNfc_GetContext();

extern void phNciNfc_NciCtxInitialize(pphNciNfc_Context_t pNciCtx);

extern void phNciNfc_SetUpperLayerCallback(pphNciNfc_Context_t nciContext, pphNciNfc_IfNotificationCb_t callbackFunction, void* callbackContext);
