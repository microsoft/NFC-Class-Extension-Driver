/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfcTypes.h"

extern NFCSTATUS phLibNfc_Discovered2Recv(void *pContext, void *Param1, void *Param2, void *Param3);
extern NFCSTATUS phLibNfc_Recv2Send(void *pContext, void *Param1, void *Param2, void *Param3);

/** Send 2 receive function is same as discovered to receive function */
#define phLibNfc_Send2Recv      phLibNfc_Discovered2Recv
#define phLibNfc_SEListen2Send  phLibNfc_Recv2Send
#define phLibNfc_SEListen2Recv  phLibNfc_Discovered2Recv
