/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfcTypes.h"

extern NFCSTATUS phLibNfc_SendDeactDiscCmd(void *pContext,NFCSTATUS status,void *pInfo);
extern NFCSTATUS phLibNfc_ProcessDeactResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
extern NFCSTATUS phLibNfc_ProcDeact2IdleResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
extern NFCSTATUS phLibNfc_ProcessDeactToDiscResp(void *pContext,NFCSTATUS wStatus,void *pInfo);
extern NFCSTATUS phLibNfc_SendDeactSleepCmd(void *pContext,NFCSTATUS status,void *pInfo);
extern NFCSTATUS phLibNfc_ReqInfoComplete(void *pContext,NFCSTATUS status,void *pInfo);

extern NFCSTATUS phLibNfc_ProcessDiscReq(void *pCtx, phNfc_eDiscAndDisconnMode_t RequestedMode, phLibNfc_sADD_Cfg_t *pADDSetup, void *Param3);
extern NFCSTATUS phLibNfc_ProcessReDiscReq(void *pCtx, phNfc_eDiscAndDisconnMode_t RequestedMode, phLibNfc_sADD_Cfg_t *pADDSetup, void *Param3);

extern NFCSTATUS phLibNfc_SendSelectCmd(void *pContext, NFCSTATUS status, void *pInfo);
extern NFCSTATUS phLibNfc_SendSelectCmd1(void *pContext, NFCSTATUS status, void *pInfo);
extern NFCSTATUS phLibNfc_SelectCmdResp(void *pContext, NFCSTATUS status, void *pInfo);

extern NFCSTATUS phLibNfc_P2pActivate(void *pContext, NFCSTATUS status, void *pInfo);
extern NFCSTATUS phLibNfc_P2pActivateResp(void *pContext, NFCSTATUS status, void *pInfo);

extern phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDeactAndDisc[];
extern phLibNfc_Sequence_t gphLibNfc_DeactivateToIdle[];
extern phLibNfc_Sequence_t gphLibNfc_DiscoverSeq[];
extern phLibNfc_Sequence_t gphLibNfc_ReStartDiscoverSeq[];
extern phLibNfc_Sequence_t gphLibNfc_DeactivateToIdleDelay[];
extern phLibNfc_Sequence_t gphLibNfc_ReDiscrySeqWithDeactAndDiscDelay[];
extern phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDisc[];
extern phLibNfc_Sequence_t gphLibNfc_ReDiscSeqWithDiscDelay[];
