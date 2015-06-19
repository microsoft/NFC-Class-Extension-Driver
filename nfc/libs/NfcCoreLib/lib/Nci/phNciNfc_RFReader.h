/*
* =============================================================================
*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
* =============================================================================
*/

#pragma once

#include "phNciNfc_RFReaderA.h"
#include "phNciNfc_RFReaderFelica.h"
#include "phNciNfc_RFReaderIso15693.h"
#include "phNciNfcTypes.h"

#define PH_NCINFC_EXTN_INVALID_PARAM_VAL (0xFFU)    /**< Initial value of Req/Resp Param/Status */
#define DATA_XCHG_PARAMS_LEN             (0x03U)    /**< Length of Data Exchange Params */

/** RdrDataXcng sequence */
extern phNciNfc_SequenceP_t *gpphNciNfc_RdrDataXchgSequence;

extern
NFCSTATUS
phNciNfc_RdrMgmtInit(
                    void    *psContext,
                    pphNciNfc_RemoteDevInformation_t pRemDevInf,
                    uint8_t *pBuff,
                    uint16_t wLen
                );

extern
NFCSTATUS
phNciNfc_RdrMgmtRelease(
                          void     *psContext
                     );

extern
NFCSTATUS
phNciNfc_RdrMgmtXchgData(
                        void     *psContext,
                        void     *pDevHandle,
                        phNciNfc_TransceiveInfo_t *pTranscvIf,
                        pphNciNfc_TransreceiveCallback_t pNotify,
                        void *pContext
                    );

extern NFCSTATUS
phNciNfc_RdrDataXchgSequence(void *pNciCtx,void *pInfo,NFCSTATUS Status);

extern NFCSTATUS
phNciNfc_CompleteDataXchgSequence(void *pContext, NFCSTATUS wStatus);
