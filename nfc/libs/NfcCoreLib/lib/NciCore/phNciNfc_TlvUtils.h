/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include "phNfcTypes.h"

/* Size of type and length fields in a tlv */
#define PHNCINFC_TLVUTIL_DEFAULT_LEN                    (2u)
/* Length byte offset */
#define PHNCINFC_TLVUTIL_LEN_BYTEOFFSET                 (1u)
/* Offset to the value field in tlv buffer */
#define PHNCINFC_TLVUTIL_VALUE_BYTEOFFSET               (2u)

/* Nxp Proprietary ID1 Value */
#define PHNCINFC_TLVUTIL_NXP_PROP_ID1                   (0xA0)
/* Nxp Proprietary configuration parameter length offset */
#define PHNCINFC_TLVUTIL_NXP_PROP_LEN_BYTEOFFSET        (2u)
/* Nxp Proprietary ID length */
#define PHNCINFC_TLVUTIL_NXP_PROP_ID2_LEN               (1u)
/* New NXP Proprietary ID2 Offset */
#define PHNCINFC_TLVUTIL_NXP_PROP_ID2_BYTEOFFSET        (1u)
/* Offset to the Proprietary value field in tlv buffer */
#define PHNCINFC_TLVUTIL_NXP_PROP_VALUE_BYTEOFFSET      (3u)

#define PHNCINFC_TLVUTIL_SUCCESS                           (10u)
#define PHNCINFC_TLVUTIL_BUFFER_NOT_EMPTY                  (11u)
#define PHNCINFC_TLVUTIL_BUFFER_EMPTY                      (12u)
#define PHNCINFC_TLVUTIL_INSUFFICIENT_BUFFER               (13u)
#define PHNCINFC_TLVUTIL_TLV_NOT_FOUND                     (14u)
#define PHNCINFC_TLVUTIL_INVALID_PARAMETERS                (15u)
#define PHNCINFC_TLVUTIL_TLV_HEADER_LEN                    (2U)

typedef struct
{
    uint8_t *pBuffer;               /**< Ponter to a buffer which holds TLV formated packet */
    union
    {
        uint32_t dwOffset;          /**<For Parser: The offset to get the next TLV info from pBuffer */
        uint32_t dwpacketSize;      /**<For Generator: The offset location to add next TLV */
    }sizeInfo;                      /**<TLV size information */
    uint32_t dwLength;              /**<Total length of Data packet*/
}phNciNfc_TlvUtilInfo_t,*pphNciNfc_TlvUtilInfo_t; /**<pointer to #phNciNfc_TlvUtilInfo_t */


extern uint8_t phNciNfc_TlvUtilsCreateTlv(
                                       phNciNfc_TlvUtilInfo_t *pttlvInfo, /**< pointer to 'phNciNfc_TlvUtilInfo_t' structure where TLV buffer resides */
                                       uint8_t btag,                /**< 'tag' part of the TLV */
                                       uint8_t blength,             /**< length of the data buffer (value) */
                                       uint8_t *pdataBuffer     /**< buffer containing value of the TLV  */
                                      );

extern uint8_t phNciNfc_TlvUtilsAddNextTlv(
                                        phNciNfc_TlvUtilInfo_t *pttlvInfo, /**< pointer to 'phNciNfc_TlvUtilInfo_t' structure where TLV buffer resides */
                                        uint8_t btag,                /**< 'tag' part of the TLV */
                                        uint8_t blength,             /**< length of the data buffer (value) */
                                        uint8_t *pdataBuffer         /**< buffer containing value of the TLV  */
                                      );

extern uint8_t phNciNfc_TlvUtilsAddTlv(
                            phNciNfc_TlvUtilInfo_t *pttlvInfo, /**< pointer to 'phNciNfc_TlvUtilInfo_t' structure where TLV buffer resides */
                            uint8_t btag,                /**< 'tag' part of the TLV */
                            uint8_t blength,             /**< length of the data buffer (value) */
                            uint8_t *pdataBuffer,        /**< buffer containing value of the TLV  */
                            uint8_t bPropId2
                            );

extern uint8_t phNciNfc_TlvUtilsRemoveTlv(
                           phNciNfc_TlvUtilInfo_t *pttlvInfo,  /**< pointer to 'phNciNfc_TlvUtilInfo_t' structure where TLV buffer resides */
                           uint8_t btag                         /**< 'tag' part of the TLV */
                           );

extern NFCSTATUS phNciNfc_TlvUtilsGetTLVLength(
                          uint8_t *pBuffer,
                          uint16_t wLength,
                          uint16_t *tlvLength
                          );

extern NFCSTATUS phNciNfc_TlvUtilsParseTLV(
                          uint8_t *pBuffer,
                          uint16_t wLength
                         );

extern NFCSTATUS phNciNfc_TlvUtilsGetNxtTlv(
                           pphNciNfc_TlvUtilInfo_t ptTlvInfo,
                           uint8_t *pType,
                           uint8_t *pLen,
                           uint8_t **pValue
                          );

