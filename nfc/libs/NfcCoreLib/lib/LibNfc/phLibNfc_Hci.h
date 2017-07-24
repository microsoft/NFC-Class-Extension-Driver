/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

extern void phLibNfc_HciDeInit(void );

extern NFCSTATUS phLibNfc_HciSetSessionIdentity(void* pContext,NFCSTATUS status,void* pInfo);
extern NFCSTATUS phLibNfc_HciSetSessionIdentityProc(void* pContext,NFCSTATUS status,void* pInfo);

extern NFCSTATUS phLibNfc_HciLaunchDevInitSequence(void *pContext);
extern NFCSTATUS phLibNfc_HciLaunchChildDevInitSequence(void *pContext, phLibNfc_SE_Index_t bIndex);

/**
*  \ingroup grp_hci_core
*
*  \brief This function shall process events received on a APDU pipe
*
*  \param[in] pContext - pointer to the HCI context
*  \param[in] wStatus - Receive Status
*  \param[in] pInfo - Received data to be read
*  \return Nfc status
*/
extern void phHciNfc_ProcessEventsOnApduPipe(void *pContext, NFCSTATUS wStatus, void *pInfo);
