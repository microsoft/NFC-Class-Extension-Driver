/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

/**<Hci Init sequence */
extern phLibNfc_Sequence_t gphLibNfc_HciInitSequence[];

/**<Hci Init  for individual NFCEE's sequence */
extern phLibNfc_Sequence_t gphLibNfc_HciChildDevInitSequence[];

extern void phLibNfc_HciDeInit(void );

extern NFCSTATUS phLibNfc_HciSetSessionIdentity(void* pContext,NFCSTATUS status,void* pInfo);
extern NFCSTATUS phLibNfc_HciSetSessionIdentityProc(void* pContext,NFCSTATUS status,void* pInfo);

extern NFCSTATUS phLibNfc_HciLaunchDevInitSequenceNci1x(void *pContext);
extern NFCSTATUS phLibNfc_HciLaunchChildDevInitSequence(void *pContext, phLibNfc_SE_Index_t bIndex);
