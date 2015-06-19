/*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*/

#pragma once

extern pphHciNfc_sCoreRecvBuff_List_t
phHciNfc_CoreGetNewNode(phHciNfc_HciCoreContext_t *pCoreCtx,
                        uint16_t wLenOfPkt);

extern NFCSTATUS
phHciNfc_HciCoreExtractData(phHciNfc_HciCoreContext_t *pCoreCtx,
                            phHciNfc_ReceiveParams_t *pHciNfcRxdParams);

extern void phHciNfc_CoreDeleteList(phHciNfc_HciCoreContext_t *pCoreCtx);
