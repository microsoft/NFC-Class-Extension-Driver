/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNciNfc_Core.h"

/*!
 * \ingroup grp_nci_nfc_core
 */
#define PHNCINFC_ENABLE_AUTO_DEREG               (1U)
#define PHNCINFC_DISABLE_AUTO_DEREG              (2U)

/*!
 * \ingroup grp_nci_nfc_core
 *
 * \brief Struct contains buffer where the received payload shall be stored
 */
typedef struct phNciNfc_Buff
{
    _Field_size_bytes_(wLen)
    uint8_t *pBuff;                  /**<pointer to the buffer where received payload shall be stored*/
    uint16_t wLen;                   /**<Buffer length*/
}phNciNfc_Buff_t, *pphNciNfc_Buff_t; /**< pointer to #phNciNfc_Buff_t */

/*!
 * \ingroup grp_nci_nfc_core
 *
 * \brief Struct contains call back function which has to be invoked upon receiving a response
 */
typedef struct phNciNfc_CoreRspCbInfo
{
    phNciNfc_NciCoreMsgType_t MsgType;  /**<pointer to #phNciNfc_NciCoreMsgType_t*/
    phNciNfc_CoreGid_t Gid;             /**<Enum of type #phNciNfc_CoreGid_t*/
    phNciNfc_CoreOid_t Oid;             /**<Enum of type #phNciNfc_CoreGid_t*/
    uint8_t ConnId;                     /**<Logical connection ID*/
    pphNciNfc_CoreIfNtf_t pNotify;    /**<pointer to function of type #pphNciNfc_CoreIfNtf_t*/
}phNciNfc_CoreRspCbInfo_t, *pphNciNfc_CoreRspCbInfo_t; /**< pointer to #phNciNfc_CoreRspCbInfo_t */

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall send command or data packets to NFCC.
 *         It returns to the callback after getting the response within the time out.
 *
 *  \param[in] pCtx - pointer to the Core context structure
 *  \param[in] pTxInfo - pointer to command info structure
 *  \param[in] pRxBuffInfo - pointer to the Response info structure
 *  \param[in] dwTimeOutMs - Timeout value in ms (if set to 0 default timeout = 1 sec)
 *  \param[in] NciCb - pointer to a call back function which shall be
 *                       invoked after completion of write operation
 *  \param[in] pContext - NCI Module level Context
 *
 *  \return Nfc status
 */
extern NFCSTATUS phNciNfc_CoreIfTxRx(pphNciNfc_CoreContext_t pCtx,
                               pphNciNfc_CoreTxInfo_t pTxInfo,
                               pphNciNfc_Buff_t  pRxBuffInfo,
                               uint32_t dwTimeOutMs,
                               pphNciNfc_CoreIfNtf_t NciCb,
                               void *pContext);

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall send command or data packets to NFCC.
 *         It allows the Core to call the callback after send completion.
 *
 *  \param[in] pCtx - pointer to the Core context structure
 *  \param[in] pTxInfo - pointer to command info structure
 *  \param[in] pRxBuffInfo - pointer to the Response info structure
 *  \param[in] NciCb - pointer to a call back function which shall be
 *                       invoked after completion of write operation
 *  \param[in] pContext - NCI Module level Context
 *
 *  \return Nfc status
 */
extern NFCSTATUS phNciNfc_CoreIfTxOnly(pphNciNfc_CoreContext_t pCtx,
                               pphNciNfc_CoreTxInfo_t pTxInfo,
                               pphNciNfc_CoreIfNtf_t NciCb,
                               void *pContext);

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function Register sync NTF function with the NCI Core,
 *  This function shall be envoked when sync events are to be De-Registered from the list.
 *
 *  \param[in] pCtx - pointer to the NCI Core Context structure
 *  \param[in] pInfo - pointer to Header Info stucutre
 *  \param[in] pNotify - Callback function
 *  \param[in] pContext - NCI Module level Context
 *
 *  \return Nfc status
 */
extern NFCSTATUS phNciNfc_CoreIfRegRspNtf(void *pCtx,
                                pphNciNfc_sCoreHeaderInfo_t pInfo,
                                pphNciNfc_CoreIfNtf_t pNotify,
                               void *pContext);

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function De-Registers sync NTF function with the NCI Core,
 *
 *  \param[in] pCtx - pointer to the Core context structure
*  \param[in] pInfo - pointer to header Info stucutre
*  \param[in] pNotify- pointer to the registered call back function
 *
 *  \return Nfc status
 */
extern NFCSTATUS phNciNfc_CoreIfUnRegRspNtf(void *pCtx,
                               pphNciNfc_sCoreHeaderInfo_t pInfo,
                               pphNciNfc_CoreIfNtf_t pNotify
                               );

/**
 *  \ingroup grp_nci_nfc_core
 *
 *  \brief This function shall share Max control packet size learned during Initialisation.
 *
 *  \param[in] pCtx - pointer to the Core context structure
 *  \param[in] PktSize - Max Control Packet siz
 *
 *  \return Nfc status
 */
extern NFCSTATUS phNciNfc_CoreIfSetMaxCtrlPacketSize(void *pCtx,
                                                     uint8_t PktSize);
