/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfcTypes.h"

extern
NFCSTATUS
phNciNfc_NfcIPollInit(
                     void    *psContext,
                     pphNciNfc_RemoteDevInformation_t pRemDevInf,
                     uint8_t *pBuff,
                     uint16_t wLen
                    );

extern
NFCSTATUS
phNciNfc_NfcILstnInit(
                     void    *psContext,
                     pphNciNfc_RemoteDevInformation_t pRemDevInf,
                     uint8_t *pBuff,
                     uint16_t wLen
                    );

extern
NFCSTATUS
phNciNfc_P2pMgmtDataXchg(
                         void     *psContext,
                         void     *pDevHandle,
                         phNciNfc_TransceiveInfo_t *pTranscvIf,
                         pphNciNfc_TransreceiveCallback_t pNotify,
                         void *pContext
                        );

extern
NFCSTATUS
phNciNfc_P2pMgmtDataXchgSeq(
                            void *pNciCtx,
                            void *pInfo,NFCSTATUS Status
                           );

extern
NFCSTATUS
phNciNfc_P2pMgmtCompleteDataXchgSeq(
                                    void *pContext,
                                    NFCSTATUS wStatus
                                   );
