/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#include "phNciNfc_CorePch.h"

#include "phNciNfc_TlvUtils.tmh"

static bool_t phNciNfc_TlvUtilsFindTlv( phNciNfc_TlvUtilInfo_t *pttlvInfo, uint8_t btag, \
                                uint32_t *ppos );


uint8_t phNciNfc_TlvUtilsCreateTlv( phNciNfc_TlvUtilInfo_t *pttlvInfo,\
                                uint8_t btag,\
                                uint8_t blength,\
                                uint8_t *pdataBuffer
                              )
{
    uint8_t bRetStat = PHNCINFC_TLVUTIL_INVALID_PARAMETERS;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pttlvInfo) && (NULL != pdataBuffer))
    {
        if(NULL != (pttlvInfo->pBuffer))
        {
            if(0 == (pttlvInfo->sizeInfo.dwpacketSize))
            {
                /* Check whether length of TLV buffer is large enough
                 * to accomodate new TLV
                 */
                if((pttlvInfo->dwLength) >= (uint8_t)(blength + 2))
                {
                    /* Copy the tag, length of 'dataBuffer' and dataBuffer to
                     * the TLV buffer
                     */
                    pttlvInfo->pBuffer[0] = btag;
                    pttlvInfo->pBuffer[1] = blength;
                    phOsalNfc_MemCopy(&pttlvInfo->pBuffer[2], pdataBuffer, blength);

                    /* Updating packet size of the TLV buffer */
                    pttlvInfo->sizeInfo.dwpacketSize += blength + 2;        /**< 2:length of tag and value & length */

                    bRetStat = PHNCINFC_TLVUTIL_SUCCESS;
                }
                else
                {
                    bRetStat = PHNCINFC_TLVUTIL_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                bRetStat = PHNCINFC_TLVUTIL_BUFFER_NOT_EMPTY;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bRetStat;
}

uint8_t phNciNfc_TlvUtilsAddNextTlv( phNciNfc_TlvUtilInfo_t * pttlvInfo,\
                                 uint8_t btag,\
                                 uint8_t blength, \
                                 uint8_t *pdataBuffer
                               )
{
    uint8_t  bRetStat = PHNCINFC_TLVUTIL_INVALID_PARAMETERS;
    uint32_t dwRemainingSize = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != pttlvInfo) && (NULL != pdataBuffer))
    {
        if(NULL != (pttlvInfo->pBuffer))
        {
            /* TLV buffer should not be empty */
            if(0 != pttlvInfo->sizeInfo.dwpacketSize)
            {
                dwRemainingSize = ((pttlvInfo->dwLength) - (pttlvInfo->sizeInfo.dwpacketSize));

                /* Check if there is enough memory in TLV buffer for accomodating the new TLV */
                if(dwRemainingSize >= (uint8_t)(blength + 2))
                {
                    /* Adding new TLV to the TLV buffer */
                    pttlvInfo->pBuffer[pttlvInfo->sizeInfo.dwpacketSize]       = btag;
                    pttlvInfo->pBuffer[(pttlvInfo->sizeInfo.dwpacketSize) + 1] = blength;

                    phOsalNfc_MemCopy(&pttlvInfo->pBuffer[(pttlvInfo->sizeInfo.dwpacketSize) + 2], \
                            pdataBuffer, blength);

                    /* Updating packet size of the TLV buffer */
                    pttlvInfo->sizeInfo.dwpacketSize += blength + 2; /**< 2:length of tag and value & length */

                    bRetStat = PHNCINFC_TLVUTIL_SUCCESS;
                }
                else
                {
                    bRetStat = PHNCINFC_TLVUTIL_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                /* TLV buffe empty */
                bRetStat = PHNCINFC_TLVUTIL_BUFFER_EMPTY;
            }
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bRetStat;
}

uint8_t
phNciNfc_TlvUtilsAddTlv(phNciNfc_TlvUtilInfo_t * pttlvInfo,
                        uint8_t btag,
                        uint8_t blength,
                        uint8_t *pdataBuffer,
                        uint8_t bPropId2)
{
    uint8_t  bRetStat = NFCSTATUS_INVALID_PARAMETER;
    uint8_t bInternalLen;
    uint32_t dwRemSize = 0;
    uint16_t wReqSize = 0;
    uint32_t wBuffOffset = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    bInternalLen = blength;
    if(NULL != pttlvInfo)
    {
        if(NULL != (pttlvInfo->pBuffer))
        {
            dwRemSize = ((pttlvInfo->dwLength) - (pttlvInfo->sizeInfo.dwpacketSize));
            wReqSize = (bInternalLen + PHNCINFC_TLVUTIL_DEFAULT_LEN);
            /* Check if there is enough memory in TLV buffer for accomodating the new TLV */
            if(dwRemSize >= wReqSize)
            {
                bRetStat = NFCSTATUS_SUCCESS;

                /* Adding new TLV to the TLV buffer */
                pttlvInfo->pBuffer[pttlvInfo->sizeInfo.dwpacketSize] = btag;
                /* Incase of NXP proprietary parameter configuration, add ID2 to the payload buffer */
                if(PHNCINFC_TLVUTIL_NXP_PROP_ID1 == btag)
                {
                    /* Nxp proprietary configuration (0xA0) */
                    /* Adding ID2 of NXP propreitary parameter */
                    pttlvInfo->pBuffer[(pttlvInfo->sizeInfo.dwpacketSize)
                                + PHNCINFC_TLVUTIL_NXP_PROP_ID2_BYTEOFFSET] = bPropId2;
                    /* Actual value of the proprietary parameter */
                    bInternalLen -= PHNCINFC_TLVUTIL_NXP_PROP_ID2_LEN;
                    /* Updating length of NXP propreitary parameter */
                    pttlvInfo->pBuffer[(pttlvInfo->sizeInfo.dwpacketSize)
                                + PHNCINFC_TLVUTIL_NXP_PROP_LEN_BYTEOFFSET] = bInternalLen;
                    wBuffOffset = (pttlvInfo->sizeInfo.dwpacketSize) + PHNCINFC_TLVUTIL_NXP_PROP_VALUE_BYTEOFFSET;
                }
                else
                {
                    /* Nci spec configuration */
                    pttlvInfo->pBuffer[(pttlvInfo->sizeInfo.dwpacketSize)
                                        + PHNCINFC_TLVUTIL_LEN_BYTEOFFSET] = bInternalLen;
                    wBuffOffset = (pttlvInfo->sizeInfo.dwpacketSize) + PHNCINFC_TLVUTIL_VALUE_BYTEOFFSET;
                }

                if(0 != bInternalLen)
                {
                    if(NULL != pdataBuffer)
                    {
                        phOsalNfc_MemCopy(&pttlvInfo->pBuffer[wBuffOffset], pdataBuffer, bInternalLen);
                    }
                    else
                    {
                        PH_LOG_NCI_CRIT_STR("pointer to Value field of tlv is invalid");
                        bRetStat = NFCSTATUS_INVALID_PARAMETER;
                    }
                }

                /* Updating packet size of the TLV buffer */
                pttlvInfo->sizeInfo.dwpacketSize += wReqSize;
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Not enough memory in TLV buffer");
                bRetStat = NFCSTATUS_BUFFER_TOO_SMALL;
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Invalid input parameters");
        }
    }
    PH_LOG_NCI_FUNC_EXIT();
    return bRetStat;
}

uint8_t phNciNfc_TlvUtilsRemoveTlv( phNciNfc_TlvUtilInfo_t *pttlvInfo, uint8_t btag )
{
    uint8_t  bRetStat = PHNCINFC_TLVUTIL_INVALID_PARAMETERS;
    uint32_t dwTlvPos;
    uint32_t dwLengthOfTLV;

    PH_LOG_NCI_FUNC_ENTRY();
    if(NULL != pttlvInfo)
    {
        if(NULL != (pttlvInfo->pBuffer))
        {
            /* TLV buffer should not be empty */
            if(0 != (pttlvInfo->sizeInfo.dwpacketSize))
            {
                /* Find the requested TLV in the TLV buffer */
                if(!phNciNfc_TlvUtilsFindTlv(pttlvInfo, btag, &dwTlvPos))
                {
                    /* Requested TLV not found in TLV buffer */
                    bRetStat = PHNCINFC_TLVUTIL_TLV_NOT_FOUND;
                }
                else
                {
                    /* TLV found, its length is (Including length of tag and length fields)  */
                    dwLengthOfTLV = pttlvInfo->pBuffer[dwTlvPos + 1] + 2;

                    /* If requested TLV was not the last TLV of TLV buffer */
                    if((dwTlvPos + dwLengthOfTLV) < pttlvInfo->sizeInfo.dwpacketSize)
                    {
                        /* Remove the requested record from the TLV list */
                        (void)memmove(&pttlvInfo->pBuffer[dwTlvPos],\
                                &pttlvInfo->pBuffer[dwTlvPos + dwLengthOfTLV],\
                                ((pttlvInfo->sizeInfo.dwpacketSize) - (dwTlvPos + dwLengthOfTLV)));
                    }

                    phOsalNfc_SetMemory(&pttlvInfo->pBuffer[(pttlvInfo->sizeInfo.dwpacketSize) - dwLengthOfTLV],\
                            '0', dwLengthOfTLV);

                    /* Update TLV buffer size */
                    pttlvInfo->sizeInfo.dwpacketSize -= dwLengthOfTLV;

                    bRetStat = PHNCINFC_TLVUTIL_SUCCESS;
                }
            }
            else
            {
                bRetStat = PHNCINFC_TLVUTIL_BUFFER_EMPTY;
            }
        }
    }
    PH_LOG_NCI_FUNC_ENTRY();
    return bRetStat;
}

static bool_t phNciNfc_TlvUtilsFindTlv( phNciNfc_TlvUtilInfo_t *pttlvInfo, uint8_t btag, \
                                uint32_t *ppos )
{
    bool_t   bRetStat  = FALSE;
    uint32_t dwCurrPos = 0;

    PH_LOG_NCI_FUNC_ENTRY();
    do
    {
        /* Search for requested Tag in the TLV buffer */
        if((pttlvInfo->pBuffer[dwCurrPos]) == btag)
        {
            /* Update the pos of Target TLV in TLV list */
            *ppos = dwCurrPos;

            bRetStat = TRUE;
            break;
        }
        else
        {
            dwCurrPos = (dwCurrPos + (pttlvInfo->pBuffer[dwCurrPos + 1]) + 2);
        }
    /* Search end of TLV buffer is reached */
    }while(dwCurrPos < (pttlvInfo->sizeInfo.dwpacketSize));
    PH_LOG_NCI_FUNC_EXIT();
    return bRetStat;
}

NFCSTATUS
phNciNfc_TlvUtilsValidate(uint8_t *pBuffer,
                          uint16_t wLength,
                          uint16_t *pwBufferLeftoverLength)
{
    uint16_t wTlvDataLength = 0;
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if (NULL != pBuffer)
    {
        while (wTlvDataLength + 1 < wLength &&
               wTlvDataLength + pBuffer[wTlvDataLength + 1] + PHNCINFC_TLVUTIL_TLV_HEADER_LEN <= wLength)
        {
            wTlvDataLength += pBuffer[wTlvDataLength + 1] + PHNCINFC_TLVUTIL_TLV_HEADER_LEN;
        }

        PH_LOG_NCI_INFO_STR("TLV data length: %u buffer length: %u", wTlvDataLength, wLength);
        if (NULL != pwBufferLeftoverLength)
        {
            *pwBufferLeftoverLength = wLength - wTlvDataLength;
        }

        wStatus = (wTlvDataLength == wLength)
                  ? NFCSTATUS_SUCCESS
                  : NFCSTATUS_FAILED;
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}

NFCSTATUS
phNciNfc_TlvUtilsGetNxtTlv(pphNciNfc_TlvUtilInfo_t ptTlvInfo,
                           uint8_t *pType,
                           uint8_t *pLen,
                           uint8_t **pValue)
{
    NFCSTATUS wStatus = NFCSTATUS_INVALID_PARAMETER;

    PH_LOG_NCI_FUNC_ENTRY();
    if((NULL != ptTlvInfo) && (NULL != pType) && (NULL != pLen))
    {
        wStatus = NFCSTATUS_FAILED;
        if((NULL != ptTlvInfo->pBuffer) && (ptTlvInfo->dwLength >= PHNCINFC_TLVUTIL_TLV_HEADER_LEN))
        {
            /* Validate offset value */
            if((ptTlvInfo->sizeInfo.dwOffset + PHNCINFC_TLVUTIL_TLV_HEADER_LEN) <= ptTlvInfo->dwLength)
            {
                wStatus = NFCSTATUS_SUCCESS;

                *pType = ptTlvInfo->pBuffer[ptTlvInfo->sizeInfo.dwOffset++];
                *pLen = ptTlvInfo->pBuffer[ptTlvInfo->sizeInfo.dwOffset++];
                *pValue = &(ptTlvInfo->pBuffer[ptTlvInfo->sizeInfo.dwOffset]);

                ptTlvInfo->sizeInfo.dwOffset += *pLen;
            }
            else
            {
                PH_LOG_NCI_CRIT_STR("Invalid Tlv buffer offset");
            }
        }
        else
        {
            PH_LOG_NCI_CRIT_STR("Invalid tlv buffer or its length");
        }
    }
    else
    {
        PH_LOG_NCI_CRIT_STR("Invalid input parameter");
    }
    PH_LOG_NCI_FUNC_EXIT();
    return wStatus;
}
