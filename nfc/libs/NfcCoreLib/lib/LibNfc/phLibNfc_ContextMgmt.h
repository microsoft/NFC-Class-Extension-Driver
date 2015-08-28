/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

#include"phLibNfc_Snep.h"

/* ----------------------- SNEP Server -------------------------- */

BOOL phLibNfc_SnepServer_InitServerContext(void);

/* Retreives a Server Context
* If NULL is returned indicates context is not initialized */
phLibNfc_SnepServerContext_t *phLibNfc_SnepServer_GetServerContext(void);

/* Removes a Server Context if server count is '0'
* Should be called in every Server De-init.
* Returns FALSE if still active server sessions are present */
BOOL phLibNfc_SnepServer_RemoveServerContext(void);

/* Retreives a Server Session matching the given Handle */
phLibNfc_SnepServerSession_t *
phLibNfc_SnepServer_GetSessionContext(phLibNfc_Handle ServerHandle);

/* Retreives a Server Connection matching the given Handle */
phLibNfc_SnepServerConnection_t *
phLibNfc_SnepServer_GetConnectionContext(phLibNfc_Handle ConnHandle);

/* Adds a new Server to the Server Context */
phLibNfc_SnepServerSession_t *
phLibNfc_SnepServer_AddSession(void);

/* Adds a new Connection to the Server Session */
phLibNfc_SnepServerConnection_t *
phLibNfc_SnepServer_AddConnection(phLibNfc_Handle ServerHandle);

/* Removes a Server session from the Server Context */
NFCSTATUS
phLibNfc_SnepServer_RemoveSession(phLibNfc_Handle ServerHandle);

/* Removes a Connection from the Server Session Context */
NFCSTATUS
phLibNfc_SnepServer_RemoveOneConnection(phLibNfc_Handle ConnHandle);

/* Removes all Connections from a Server Session Context */
NFCSTATUS
phLibNfc_SnepServer_RemoveAllConnections(phLibNfc_Handle ServerHandle);

/* Retreives a Server Session matching the connection */
phLibNfc_SnepServerSession_t *
phLibNfc_SnepServer_GetSessionByConnection(phLibNfc_Handle ConnHandle);

/* ----------------------- SNEP Client -------------------------- */

/* Initializes the Client Context
* Returns TRUE if succeeded */
BOOL phLibNfc_SnepServer_InitClientContext(void);

/* Retreives a Client Context
* If NULL is returned indicates context allocation failed */
phLibNfc_SnepClientContext_t *
phLibNfc_SnepClient_GetClientContext(void);

/* Removes a Client Context if Client count is '0'
* Should be called in every Client De-init.
* Returns FALSE if still Client sessions are present */
BOOL
phLibNfc_SnepClient_RemoveClientContext(void);

/* Retreives a Client Session matching the given Handle */
phLibNfc_SnepClientSession_t *
phLibNfc_SnepClient_GetClientSession(phLibNfc_Handle ClientHandle);

/* Adds a new Client session to the Client Context */
phLibNfc_SnepClientSession_t *
phLibNfc_SnepClient_AddSession(void);

/* Removes a Client from the Client Context */
NFCSTATUS
phLibNfc_SnepClient_RemoveSession(phLibNfc_Handle ClientHandle);

/* Removes a Incomplete Client from the Client Context */
NFCSTATUS
phLibNfc_SnepClient_RemoveIncompleteSession(phLibNfc_SnepClientSession_t *pClientSession);

/* Removes all Client Sessions from Client Context */
NFCSTATUS
phLibNfc_SnepClient_RemoveAllSessions(void);
