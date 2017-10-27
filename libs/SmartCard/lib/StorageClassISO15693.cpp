/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassISO15693.cpp

Abstract:

    Storage card ISO15693 implementation
    
Environment:

    User mode

--*/
#include "Pch.h"

#include "StorageClassISO15693.tmh"

StorageClassISO15693::StorageClassISO15693(IStorageCard *pStorageCard)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    m_pStorageCard = pStorageCard;
    m_IsLesserLeLc = FALSE;
    m_HistoricalByteLength = 0;
    m_CmdBufferSize = 0;
    m_UidLength = 0;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void
StorageClassISO15693::UpdateUniqueID(
                                    _In_reads_bytes_(uidLength) BYTE *Uid,
                                    _In_ BYTE uidLength
                                    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    m_UidLength = uidLength;
    RtlCopyMemory(m_Uid, Uid, m_UidLength);
    TRACE_LINE(LEVEL_INFO, "m_UidLength=%d", m_UidLength);
    
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void
StorageClassISO15693::GetUniqueID(
                                _Outptr_result_bytebuffer_(*uidLength) BYTE **uid,
                                _Out_ BYTE *uidLength
                               )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    *uidLength = m_UidLength;
    *uid = m_Uid;
    
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void 
StorageClassISO15693::UpdateHistoricalBytes(
                                            _In_reads_bytes_(HistoBytesLength) BYTE *HistoBytes,
                                            _In_ DWORD HistoBytesLength
                                            )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    m_HistoricalByteLength = HistoBytesLength;
    RtlCopyMemory(m_HistoricalBytes, HistoBytes, m_HistoricalByteLength);
    
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

ApduResult
StorageClassISO15693::GetDataCommand(
                                     _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                    _In_ DWORD cbSize,
                                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                    _In_ DWORD cbOutBufferSize,
                                    _Out_ DWORD *cbReturnBufferSize
                                    )
{
    ApduResult retValue    = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    // Ensure buffer covers Lc
    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    if (cmdApdu->P1 == 0x00 && cmdApdu->P2 == 0x00) { // UID
        BYTE *uid = NULL;
        BYTE uidLength = 0;

        GetUniqueID(&uid, &uidLength);

        if (cbOutBufferSize < uidLength) {
            retValue = RESULT_INVALID_PARAM;
            goto Done;
        }

        if (0 != cmdApdu->Lc) {
            if (uidLength > cmdApdu->Lc) {
                retValue = RESULT_SUCCESS;
                m_IsLesserLeLc = TRUE;
            }

            if (uidLength < cmdApdu->Lc) {
                retValue = RESULT_LE_GREATER; // 0x62 and 0x82
                RtlCopyMemory(pbOutBuffer, uid, uidLength);
                RtlCopyMemory(pbOutBuffer + uidLength, 0, (cmdApdu->Lc - uidLength));
                *cbReturnBufferSize = cmdApdu->Lc;
            }
        }

        if (RESULT_SUCCESS == retValue) {
            RtlCopyMemory(pbOutBuffer, uid, uidLength);
            *cbReturnBufferSize = uidLength;
        }
    }
    else if (cmdApdu->P1 == 0x01 && cmdApdu->P2 == 0x00) { // Get Historical bytes
        retValue = RESULT_NOT_SUPPORTED;
        goto Done;
    }
    else { // Wrong P1 and P2 value
        retValue = RESULT_WRONG_P1P2;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retValue;
}

ApduResult 
StorageClassISO15693::HandleIncDecCommand(
                            _In_reads_bytes_(size) const BYTE *pbDataBuffer,
                            _In_ DWORD size,
                            _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                            _In_ DWORD cbOutBufferSize,
                            _Out_ DWORD *pcbReturnBufferSize  
                            )
{
    ApduResult retResult = RESULT_NOT_SUPPORTED;
    
    UNREFERENCED_PARAMETER(pbDataBuffer);
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(pbOutBuffer);
    UNREFERENCED_PARAMETER(cbOutBufferSize);

    *pcbReturnBufferSize = 0;

    return retResult;
}

ApduResult
StorageClassISO15693::UpdateBinary(
                                _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                                _In_ DWORD cbSize,
                                _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                _In_ DWORD cbOutBufferSize,
                                _Out_ DWORD *pcbReturnBufferSize
                                )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_NOT_SUPPORTED;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    *pcbReturnBufferSize = 0;

    if (cbSize < MIN_APDU_HEADER) {
        retResult = RESULT_INVALID_PARAM;
        goto Done;
    }

    retResult = ValidateUpdateBinaryParameters(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if (RESULT_SUCCESS == retResult) {
        RtlZeroMemory(m_CommandBuffer, COMMAND_BUFFER_SIZE);
        m_CmdBufferSize = 0;

        retResult = PrepareTransceiveForUpdate(cmdApdu);
        if (RESULT_SUCCESS != retResult) {
            goto Done;
        }

        if (cbOutBufferSize < m_CmdBufferSize) {
            retResult = RESULT_INVALID_PARAM;
            goto Done;
        }

        RtlCopyMemory(pbOutBuffer, m_CommandBuffer, m_CmdBufferSize);
        *pcbReturnBufferSize = m_CmdBufferSize;
    }
    else {
        RtlZeroMemory(pbOutBuffer, cbOutBufferSize);
        *pcbReturnBufferSize = 0;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassISO15693::ValidateUpdateBinaryParameters(
                                     BYTE  p1,
                                     BYTE  p2,
                                     BYTE  lc
                                     )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_SUCCESS;

    UNREFERENCED_PARAMETER(p1);
    UNREFERENCED_PARAMETER(p2);

    if (lc != ISO15693_BYTES_PER_BLOCK) {
        retResult = RESULT_WRONG_LENGTH;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassISO15693::PrepareTransceiveForUpdate(
                               _In_ PPcscCommandApduInfo pPcscCmdApdu
                               )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_SUCCESS;
    BYTE size = 0;

    if (pPcscCmdApdu->Lc != ISO15693_BYTES_PER_BLOCK) {
        retResult = RESULT_WRONG_LENGTH;
        goto Done;
    }

    if (ISO15693_PROTOEXT(m_Uid) == TRUE) {
        m_CommandBuffer[size++] = ISO15693_FLAG_PROTOEXT; // Request Flags
        m_CommandBuffer[size++] = ISO15693_WRITE_COMMAND;
        m_CommandBuffer[size++] = pPcscCmdApdu->P1; // Block Number
        m_CommandBuffer[size++] = pPcscCmdApdu->P2;
    }
    else if (pPcscCmdApdu->P2 != 0) {
        m_CommandBuffer[size++] = 0; // Request Flags
        m_CommandBuffer[size++] = ISO15693_EXT_WRITE_COMMAND;
        m_CommandBuffer[size++] = pPcscCmdApdu->P1; // Block Number
        m_CommandBuffer[size++] = pPcscCmdApdu->P2;
    }
    else {
        m_CommandBuffer[size++] = 0; // Request Flags
        m_CommandBuffer[size++] = ISO15693_WRITE_COMMAND;
        m_CommandBuffer[size++] = pPcscCmdApdu->P1; // Block Number
    }
    RtlCopyMemory(&m_CommandBuffer[size], pPcscCmdApdu->DataIn, ISO15693_BYTES_PER_BLOCK);
    size += ISO15693_BYTES_PER_BLOCK;

    m_CmdBufferSize = size;
    retResult = RESULT_SUCCESS;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassISO15693::ReadBinary(
                            _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                            _In_ DWORD cbSize,
                            _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                            _In_ DWORD cbOutBufferSize,
                            _Out_ DWORD *pcbReturnBufferSize
                            )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult  retResult = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    *pcbReturnBufferSize = 0;

    if (cbSize < MIN_APDU_HEADER) {
        retResult = RESULT_INVALID_PARAM;
        goto Done;
    }

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    retResult = ValidateReadBinaryParamters(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if (RESULT_SUCCESS == retResult) {
        RtlZeroMemory(m_CommandBuffer, COMMAND_BUFFER_SIZE);
        m_CmdBufferSize = 0;

        retResult = PrepareTransceiveForRead(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
        if (RESULT_SUCCESS != retResult) {
            goto Done;
        }

        if (cbOutBufferSize < m_CmdBufferSize) {
            retResult = RESULT_INVALID_PARAM;
            goto Done;
        }

        RtlCopyMemory(pbOutBuffer, m_CommandBuffer, m_CmdBufferSize);
        *pcbReturnBufferSize = m_CmdBufferSize;
    }
    else {
        TRACE_LINE(LEVEL_ERROR, "Invalid parameter %d", retResult);
        RtlZeroMemory(pbOutBuffer, cbOutBufferSize);
        *pcbReturnBufferSize = 0;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassISO15693::ValidateReadBinaryParamters(
                                BYTE p1,
                                BYTE p2,
                                BYTE le
                                )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_SUCCESS;

    UNREFERENCED_PARAMETER(p1);
    UNREFERENCED_PARAMETER(p2);

    if (le != ISO15693_BYTES_PER_BLOCK) {
        retResult = RESULT_WRONG_LENGTH;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassISO15693::PrepareTransceiveForRead(
                             BYTE p1,
                             BYTE p2,
                             BYTE le
                             )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_SUCCESS;
    BYTE size = 0;

    UNREFERENCED_PARAMETER(p2);

    if (le == ISO15693_BYTES_PER_BLOCK) {
        if (ISO15693_PROTOEXT(m_Uid) == TRUE) {
            m_CommandBuffer[size++] = ISO15693_FLAG_PROTOEXT; // Request Flags
            m_CommandBuffer[size++] = ISO15693_READ_COMMAND;
            m_CommandBuffer[size++] = p1;
            m_CommandBuffer[size++] = p2;
        }
        else if (p2 != 0) {
            m_CommandBuffer[size++] = 0; // Request Flags
            m_CommandBuffer[size++] = ISO15693_EXT_READ_COMMAND;
            m_CommandBuffer[size++] = p1;
            m_CommandBuffer[size++] = p2;
        }
        else {
            m_CommandBuffer[size++] = 0; // Request Flags
            m_CommandBuffer[size++] = ISO15693_READ_COMMAND;
            m_CommandBuffer[size++] = p1;
        }
        m_CmdBufferSize = size;
    }
    else {

        if (le > ISO15693_BYTES_PER_BLOCK) {
            retResult = RESULT_WRONG_LENGTH;
            goto Done;
        }

        retResult = RESULT_SUCCESS;
        m_IsLesserLeLc = TRUE;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult 
StorageClassISO15693::GetGeneralAuthenticateCommand(
                                _In_ void* getLoadKey,
                                _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                _In_ DWORD cbSize,
                                _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                _In_ DWORD cbOutBufferSize,
                                _Out_ DWORD *pcbReturnBufferSize  
                                )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    
    ApduResult retResult = RESULT_NOT_SUPPORTED;    
    
    UNREFERENCED_PARAMETER(pbDataBuffer);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pbOutBuffer);
    UNREFERENCED_PARAMETER(getLoadKey);
    UNREFERENCED_PARAMETER(cbOutBufferSize);

    *pcbReturnBufferSize = 0;
    
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

void
StorageClassISO15693::UpdateResponseCodeToResponseBuffer(
                                    _In_ ApduResult RetError,
                                    _In_ PcscInsByte Command,
                                    _In_ DWORD cbSize,
                                    _Inout_updates_bytes_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[],
                                    _Inout_updates_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                    _In_ DWORD cbOutBufferSize,
                                    _Out_ DWORD *pcbReturnBufferSize  
                                    )
{
    PBYTE pbOutBufferStatus = NULL;
    BYTE Sw1Sw2[DEFAULT_APDU_STATUS_SIZE] = {0};
    NTSTATUS status = STATUS_SUCCESS;
    DWORD tempMaxSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *pcbReturnBufferSize = 0;

    TRACE_LINE(LEVEL_INFO, " (@ first recieved size 0x%02x)", cbSize);

    if (cbOutBufferSize < DEFAULT_APDU_STATUS_SIZE) {
        NT_ASSERTMSG("The cbOutBufferSize should be atleast DEFAULT_APDU_STATUS_SIZE", FALSE);
        goto Done;
    }

    status = RtlULongAdd(cbSize, DEFAULT_APDU_STATUS_SIZE, &tempMaxSize);
    if (!NT_SUCCESS(status) ||
        cbOutBufferSize < tempMaxSize) {
        Sw1Sw2[0] = 0x6C;
        Sw1Sw2[1] = (BYTE)cbSize;
        
        RtlCopyMemory(pbOutBuffer, Sw1Sw2, DEFAULT_APDU_STATUS_SIZE);
        *pcbReturnBufferSize = DEFAULT_APDU_STATUS_SIZE;

        TRACE_LINE(LEVEL_ERROR, "Invalid buffer size, cbOutBufferSize=%d", cbOutBufferSize);
        goto Done;
    }

    Sw1Sw2[0] = Sw1Sw2Return[0];
    Sw1Sw2[1] = Sw1Sw2Return[1];

    if (PcscReadCmd == Command) {
        if (RESULT_SUCCESS == RetError) {
            if (m_IsLesserLeLc) {
                m_IsLesserLeLc = FALSE;
                Sw1Sw2[0] = 0x6C;
                Sw1Sw2[1] = ISO15693_BYTES_PER_BLOCK;
            }

            // Remove response flags from the response and copy just the data blocks
            // We are guarenteed to have no errors since that's validated at the time of transceive

            if (cbSize >= sizeof(BYTE)) {
                cbSize -= sizeof(BYTE);
                RtlMoveMemory(pbOutBuffer, &pbOutBuffer[sizeof(BYTE)], cbSize);
            }
        }
    }
    else if (PcscWriteCmd == Command) {
        RtlZeroMemory(pbOutBuffer, cbSize);
        cbSize = 0;
    }
    else if (PcscGetDataCmd == Command) {
        if (m_IsLesserLeLc) {
            PBYTE uid = NULL;
            BYTE uidLength = 0;

            GetUniqueID(&uid, &uidLength);

            Sw1Sw2[0] = 0x6C;
            Sw1Sw2[1] = uidLength;
            m_IsLesserLeLc = FALSE;
        }
    }

    pbOutBufferStatus = pbOutBuffer + cbSize;
    RtlCopyMemory(pbOutBufferStatus, Sw1Sw2, DEFAULT_APDU_STATUS_SIZE);
    *pcbReturnBufferSize = cbSize + DEFAULT_APDU_STATUS_SIZE;

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

ApduResult
StorageClassISO15693::MapErrorCodeToApduStatus(
                                     _In_ BYTE ErrorCode
                                     )
{
    switch (ErrorCode)
    {
    case ISO15693_COMMAND_NOT_SUPPORTED:
    case ISO15693_OPTION_NOT_SUPPORTED:
        return RESULT_NOT_SUPPORTED;
    case ISO15693_COMMAND_NOT_RECOGNIZED:
        return RESULT_COMMAND_INCOMPATIBLE;
    case ISO15693_INVALID_BLOCK:
        return RESULT_INVALID_BLOCK_COUNT;
    case ISO15693_BLOCK_LOCK_FAILED:
        return RESULT_MEMORY_FAILURE;
    default:
        return RESULT_ERROR;
    }
}

ApduResult
StorageClassISO15693::StorageCardTransceive(
                          _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                          _In_ DWORD cbSize,
                          _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                          _In_ DWORD cbOutBufferSize,
                          _Out_ DWORD *pcbReturnBufferSize,
                          _In_ USHORT usTimeout
                          )
{
    ApduResult retResult = RESULT_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    status = m_pStorageCard->Transmit((PBYTE)pbDataBuffer,
                                      cbSize,
                                      pbOutBuffer,
                                      cbOutBufferSize,
                                      pcbReturnBufferSize,
                                      usTimeout);
    if (!NT_SUCCESS(status)) {
        retResult = RESULT_COMMAND_INCOMPATIBLE; //6981;
        *pcbReturnBufferSize = 0;
    }
    else if (*pcbReturnBufferSize > 0 &&
              pbOutBuffer[0] & ISO15693_RESPONSE_ERROR_FLAG)
    {
        if (*pcbReturnBufferSize > 1) {
            retResult = MapErrorCodeToApduStatus(pbOutBuffer[1]);
        } else {
            retResult = RESULT_ERROR;
        }

        *pcbReturnBufferSize = 0;
    }
    else {
        retResult = RESULT_SUCCESS;
        TRACE_LINE(LEVEL_INFO, "Received data buffer size = %d", (DWORD)*pcbReturnBufferSize);
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retResult);
    return retResult;
}
