/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>

/**< Min length of input buffer */
#define PHLIBNFC_INPUT_BUFF_MIN_LEN         (4U)

typedef struct
{
  void                         *pCliCntx;      /**<Upper layer context*/
  pphLibNfc_IoctlCallback_t    CliRspCb;       /**<Upper layer call back funciton pointer*/
  phNfc_sHwReference_t         *psHwReference; /**<Hardware reference*/
  phNfc_sData_t*               pOutParam;      /**<pointer to #phNfc_sData_t to store output data*/
  uint16_t                     IoctlCode;    /**<Ioctl code*/
}phLibNfc_Ioctl_Cntx_t;
