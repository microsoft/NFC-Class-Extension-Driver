/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfc_RFReader.h"
#include "phNciNfc_Core.h"
#include "phNciNfc_Common.h"

extern
NFCSTATUS
phNciNfc_LogConnInit(
                     phNciNfc_Context_t     *psNciContext
                );

extern
NFCSTATUS
phNciNfc_LogConnCreate(
                        void  *psContext,
                        phNciNfc_Dest_Info_t    *pDestination,
                        pphNciNfc_IfNotificationCb_t pNotify,
                        void *pContext
              );

extern
NFCSTATUS
phNciNfc_LogConnClose(
                        void     *psContext,
                        uint8_t  bDestId,
                        phNciNfc_DestType_t tDestType,
                        pphNciNfc_IfNotificationCb_t pNotify,
                        void *pContext
                    );

