/*
* =============================================================================
*
*          Modifications Copyright © Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*
*
* =============================================================================
*/

#pragma once

#include "phNciNfc_Context.h"

typedef enum phNciNfc_SensFReqRC
{
    phNciNfc_e_NoSysCode,                /**< No SystemCode info requested */
    phNciNfc_e_SysCode,                  /**< SystemCode info requested */
    phNciNfc_e_AdvProt,                  /**< Advanced Protocol features supported */
    phNciNfc_e_InvalidRC                 /**< Invalid request - RFU */
}phNciNfc_SensFReqRC_t;

typedef enum phNciNfc_SensFReqTSN
{
    phNciNfc_e_1TS = 0x00,           /**< Num of TimeSlots = 1 */
    phNciNfc_e_2TS,                  /**< Num of TimeSlots = 2 */
    phNciNfc_e_4TS = 0x03,           /**< Num of TimeSlots = 4 */
    phNciNfc_e_8TS = 0x07,           /**< Num of TimeSlots = 8 */
    phNciNfc_e_16TS = 0x0F,          /**< Num of TimeSlots = 16 */
    phNciNfc_e_InvalidTSN            /**< Invalid request - RFU*/
}phNciNfc_SensFReqTSN_t;

typedef struct phNciNfc_BlkList
{
    uint8_t   bBlkInfo;    /**< Interpreted as "|Len|Ac|Mo|de|Se|rv|Co|de|" ==> |b0|b1|..|b7| */
    uint16_t  wBlkNum;     /**< 1 Byte or 2 Byte BlkNum depending on the "Len"(b0) bit in bBlkInfo */
} phNciNfc_BlkList_t;

extern phNciNfc_SequenceP_t gphNciNfc_T3TSequence[];

extern
NFCSTATUS
phNciNfc_RdrFInit(
                    pphNciNfc_RemoteDevInformation_t  pRemDevInf,
                    uint8_t *pBuff,
                    uint16_t wLen
                    );

extern
NFCSTATUS
phNciNfc_SendFelicaReq(
                              void   *psContext
                         );

extern
NFCSTATUS
phNciNfc_RecvFelicaResp(
                        void                *psContext,
                        NFCSTATUS           wStatus
                       );
