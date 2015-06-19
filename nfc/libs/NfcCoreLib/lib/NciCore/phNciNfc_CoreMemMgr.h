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
#include <phNfcStatus.h>
#include "phNciNfc_Core.h"

extern pphNciNfc_sCoreRecvBuff_List_t phNciNfc_CoreGetNewNode(pphNciNfc_CoreContext_t pCoreCtx);

extern NFCSTATUS phNciNfc_CoreGetDataLength(
                                            pphNciNfc_CoreContext_t pCoreCtx,
                                            uint16_t *pDataLen,
                                            uint16_t *wNumOfNode
                                           );

extern NFCSTATUS phNciNfc_CoreGetData(pphNciNfc_CoreContext_t pCoreCtx,
                                      uint8_t *pBuff,
                                      uint16_t wLen
                                     );
