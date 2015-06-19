/*
 *          Modifications Copyright © Microsoft. All rights reserved.
 *
 *              Original code Copyright (c), NXP Semiconductors
 *
 *
 *
 */

#pragma once

#include "phNfcTypes.h"

typedef NFCSTATUS (*phNciNfc_StateEntry_t)(void *pContext);
typedef NFCSTATUS (*phNciNfc_StateTransition_t)(void *pContext);
typedef NFCSTATUS (*phNciNfc_StateExit_t)(void *pContext);
typedef uint8_t (*phNciNfc_Connector_t)(void *pContext);

typedef struct phNciNfc_StateFunction
{
    phNciNfc_StateEntry_t pfEntry; /**< Entry function to be called before entering to the new state*/
    phNciNfc_StateExit_t pfExit;/**< to be performed before exit from the current state*/
}phNciNfc_StateFunction_t, *pphNciNfc_StateFunction_t;

typedef struct phNciNfc_StateContext
{
    uint8_t CurrState; /**< Current state*/
    uint8_t TransitionFlag; /**< Acting on event*/
    uint8_t CurrEvt;/**< Current event in */
    /*Event Queue may be required to handle if more than event occurs and needs to be processed*/
    phNciNfc_StateFunction_t pfStateHandler;/**< Entry and Exit function handlres for state*/
    phNciNfc_Connector_t pfConnectors; /**< Connectors for state*/
}phNciNfc_StateContext_t;
