/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>
#include "phNciNfc.h"

extern uint8_t phNciNfc_ValidateRfInterface(phNciNfc_RfInterfaces_t eRfInterface);
extern uint8_t phNciNfc_ValidateRfProtocol(phNciNfc_RfProtocols_t eRfProtocol);
extern uint8_t phNciNfc_ValidateRfTechMode(phNciNfc_RfTechMode_t eRfTechmode);
extern NFCSTATUS phNciNfc_ValidateIntfActvdNtf(uint8_t *pNtf, uint16_t bSize);

extern void phNciNfc_GetRfDevType(uint8_t bRespVal, uint8_t bRespLen,
                                  pphNciNfc_RemoteDevInformation_t pRemDevInf,
                                  phNciNfc_RFDevType_t *pDevType);
