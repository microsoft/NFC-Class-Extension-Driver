/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcStatus.h>
#include <phNfcCompId.h>
#include <phNfcTypes.h>
#include <phFriNfc_NdefMap.h>
#include <phFriNfc_OvrHal.h>
#include <phFriNfc_SmtCrdFmt.h>
#include <phFriNfc_NdefReg.h>
#include <phLibNfc.h>
#include <phLibNfc_ndef_raw.h>

#include <phNfcHalTypes2.h>
#include <phFriNfc_LlcpTransport.h>
#include <phLibNfc_State.h>


typedef struct phLibNfc_LlcpInfo
{
   /* Local parameters for LLC, given upon config
    * and used upon detection.
    */
   phLibNfc_Llcp_sLinkParameters_t sLocalParams;

   /* LLCP compliance flag */
   bool_t bIsLlcp;

   /* Monitor structure for LLCP Transport */
   phFriNfc_LlcpTransport_t sLlcpTransportContext;

   /* Monitor structure for LLCP LLC */
   phFriNfc_Llcp_t sLlcpContext;

   /* LLC Rx buffer */
   uint8_t pRxBuffer[PHFRINFC_LLCP_PACKET_MAX_SIZE];

   /* LLC Tx buffer */
   uint8_t pTxBuffer[PHFRINFC_LLCP_PACKET_MAX_SIZE];

} phLibNfc_LlcpInfo_t;

