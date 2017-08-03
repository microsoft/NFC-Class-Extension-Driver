/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include <phNfcTypes.h>

/**
 * \ingroup grp_osal_nfc
 * \brief Invalid timer ID type.This ID used indicate timer creation is failed.
 */
#define PH_OSALNFC_TIMER_ID_INVALID     0

/*!
 * \ingroup grp_osal_nfc
 * \brief Timer callback interface which will be called once registered timer time out expires.
 * \param[in] TimerId  Timer Id for which callback is called.
 */
typedef void (*ppCallBck_t)(uint32_t TimerId, void *pContext);

typedef ppCallBck_t pphOsalNfc_TimerCallbck_t;

/**
 * \ingroup grp_osal_nfc
 * \brief Timer message structure definition.
 * Timer Message Structure contains timer specific informations like timer identifier and timer callback.
 */
typedef struct phOsalNfc_TimerMsg
{
   uint32_t         TimerId;    /**< Timer ID*/
   ppCallBck_t      pCallBck;   /**< pointer to Timer Callback*/
   void*            pContext;   /**< Timer Callback context*/
}phOsalNfc_Timer_Msg_t,*pphOsalNfc_TimerMsg_t;

/**
 * \ingroup grp_osal_nfc
 * \brief Allows to create new timer.
 * This API creates a cyclic timer.Incase valid timer is created returned timer ID will be
 * other than \ref PH_OSALNFC_TIMER_ID_INVALID.Incase returned timer id is \ref PH_OSALNFC_TIMER_ID_INVALID
 * indicates timer creation is failed.
 * When Timer created ,By default timer is not started.Application has to start timer explicitly 
 * needs using \ref phOsalNfc_Timer_Start() API.
 *
 * \retval     Valid timer Timer ID. 
 *             \note <br><br> If timer ID value is PH_OSALNFC_TIMER_ID_INVALID
 *              Indicates timer is not created.
 * \retval     PH_OSALNFC_TIMER_ID_INVALID,Indicates timer creation failed.
 */

uint32_t phOsalNfc_Timer_Create (void);

/**
 * \ingroup grp_osal_nfc
 * \brief Allows to start already created idle timer.
 * This function  starts the requested timer. If the timer is already running, 
 * timer stops and restarts with the new timeout value and new callback function in case any.
 *
 * \note The old timeout and callback reference are not valid any more if timer is restarted 
 * again.<br><br> Notification are periodic and notification stops only when timer is stoped .
 * 
 * \param[in]  TimerId      valid timer ID obtained during timer creation.
 * \param[in]  RegTimeCnt   Requested time out in Milliseconds.
 *                          \note  In windows environment timer resolution should be more than 50 mSec. 
 *                          Incase time out value is below 50 mSec accuracy of timer  behaviour
 *                          is not gauranteed.
 *
 * \param[in]  Application_callback    Application  Callback interface to be called when timer expires.
 */

NFCSTATUS phOsalNfc_Timer_Start(uint32_t     TimerId,
                           uint32_t     RegTimeCnt,
                           ppCallBck_t  Application_callback,
                           void         *pContext);

/**
 * \ingroup grp_osal_nfc
 * \brief Allows to stop the timer which is already started.
 * This API allows to stop running timer.Incase timer is stopped,timer callback will not be 
 * notified any more.However once timer is started again ,it starts notifying 
 * timer callback ,once timeout expires.
 *
 * \param[in] TimerId                Valid timer ID obtained suring timer creation.
 * \param[in] Application_callback   Application Callback interface to be called when timer expires.
 */
NFCSTATUS phOsalNfc_Timer_Stop(uint32_t TimerId);

/**
 * \ingroup grp_osal_nfc
 * \brief Allows to delete the timer which is already created.
 * This  API allows to delete timer which already created.Incase timer is running
 * timer is stopped first and timer is deleted. Incase invalid timer ID ,this function 
 * doesn't return any error code.Application has to explicitly ensure timer ID sent is valid.
 * \param[in]  TimerId	timer identifier to delete the timer.
 */
void phOsalNfc_Timer_Delete(uint32_t TimerId);
