/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 *
 */

#pragma once

#include <phNfcTypes.h>
#include "phNciNfc_Core.h"

extern NFCSTATUS
phNciNfc_CoreRecvMgrInit(
                         pphNciNfc_CoreContext_t pCoreCtx
                        );

extern NFCSTATUS
phNciNfc_CoreRecvMgrRelease(
                            pphNciNfc_CoreContext_t pCoreCtx
                           );

extern NFCSTATUS
phNciNfc_CoreRecvMgrRegisterCb(
                               void *pCtx,
                               pphNciNfc_CoreRegInfo_t pRegInfo,
                               phNciNfc_NciCoreMsgType_t eMsgType
                              );

extern NFCSTATUS
phNciNfc_CoreRecvMgrDeRegisterCb(void *pCtx,
                                 pphNciNfc_CoreRegInfo_t pRegInfo,
                                 phNciNfc_NciCoreMsgType_t eMsgType);

extern void
phNciNfc_CoreRecvMgrDeRegisterAll(void *pCtx);

extern void
phNciNfc_CoreRecvMgrDeRegDataCb(void *pCtx, uint8_t bConnId);

extern NFCSTATUS
phNciNfc_CoreRecvManager(
                         void *pCtx,
                         NFCSTATUS wStatus,
                         pphNciNfc_sCoreHeaderInfo_t pHdrInfo,
                         phNciNfc_NciCoreMsgType_t eMsgType
                        );
