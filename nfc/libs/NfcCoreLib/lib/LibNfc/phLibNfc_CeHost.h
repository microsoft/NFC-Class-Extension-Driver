/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 */

#pragma once

#include <phNfcStatus.h>
#include <phNfcCompId.h>
#include <phNfcTypes.h>
#include <phNciNfc.h>
#include <phNfcHalTypes2.h>

NFCSTATUS phLibNfc_MapRemoteDevCeHost(phNfc_sIso14443AInfo_t     *pNfcAInfo,
                                 pphNciNfc_RemoteDevInformation_t pNciDevInfo);

/*Register for the callback to receive the first data buffer
Once the first data buffer arrives, the HCE activation is intimated to DH*/
NFCSTATUS phLibNfc_RegisterForHceActivation(void);
