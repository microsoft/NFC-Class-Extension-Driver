/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassJewel.cpp

Abstract:

    Storage card Jewel implementation
    
Environment:

    User mode

--*/
#include "Pch.h"

#include "StorageClassJewel.tmh"

StorageClassJewel::StorageClassJewel(IStorageCard *pStorageCard)
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
StorageClassJewel::UpdateUniqueID(
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
StorageClassJewel::GetUniqueID(
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
StorageClassJewel::UpdateHistoricalBytes(
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
StorageClassJewel::GetDataCommand(
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
        if (cbOutBufferSize < m_HistoricalByteLength) {
            retValue = RESULT_INVALID_PARAM;
            goto Done;
        }

        if (0 != cmdApdu->Lc) {
            if (m_HistoricalByteLength > cmdApdu->Lc) {
                retValue = RESULT_LE_LESSER; // 0x6C and XX. 
            }
            if (m_HistoricalByteLength < cmdApdu->Lc) {
                if (cmdApdu->Lc > cbOutBufferSize) {
                    RtlZeroMemory(pbOutBuffer, cbOutBufferSize);
                    *cbReturnBufferSize = cbOutBufferSize;
                }
                else {
                    RtlZeroMemory(pbOutBuffer, cmdApdu->Lc);
                    *cbReturnBufferSize = cmdApdu->Lc;
                }

                RtlCopyMemory(pbOutBuffer, m_HistoricalBytes, m_HistoricalByteLength);
                retValue = RESULT_LE_GREATER; // 0x62 and 0x82
            }
        }

        if (RESULT_LE_GREATER != retValue) {
            RtlCopyMemory(pbOutBuffer, m_HistoricalBytes, m_HistoricalByteLength);
            *cbReturnBufferSize = m_HistoricalByteLength;
        }
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
StorageClassJewel::HandleIncDecCommand(
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
StorageClassJewel::UpdateBinary(
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
StorageClassJewel::ValidateUpdateBinaryParameters(
                                     BYTE  p1,
                                     BYTE  p2,
                                     BYTE  lc
                                     )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_WRONG_P1P2;

    if (lc == 0x1) {
        //
        // WRITE-E
        //
        if ((p1 <= 0x0f) && (p2 <= 0x07)) {
            retResult = RESULT_SUCCESS;
        }
    } else if (lc == 0x8) {
        //
        // WRITE-E8
        //
        if ((p1 <= 0xff) && (p2 == 0x00)) {
            retResult = RESULT_SUCCESS;
        }
    } else {
        retResult = RESULT_ERROR;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassJewel::PrepareTransceiveForUpdate(
                               _In_ PPcscCommandApduInfo pPcscCmdApdu
                               )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_NOT_SUPPORTED;
    DWORD size = 0;
    PBYTE uid = NULL;
    BYTE uidLength = 0;

    GetUniqueID(&uid, &uidLength);

    if (pPcscCmdApdu->Lc == 1) {
        //
        // WRITE-E
        //
        m_CommandBuffer[size++] = TOPAZ_WRITEE_COMMAND;
        m_CommandBuffer[size++] = (pPcscCmdApdu->P1 << 3) | pPcscCmdApdu->P2;
        m_CommandBuffer[size++] = pPcscCmdApdu->DataIn[0];

        RtlCopyMemory(&m_CommandBuffer[size], uid, uidLength);
        size += uidLength;

        m_CmdBufferSize = size;
        retResult = RESULT_SUCCESS;

    } else if (pPcscCmdApdu->Lc == 8) {
        //
        // WRITE-E8
        //
        m_CommandBuffer[size++] = TOPAZ_WRITEE8_COMMAND;
        m_CommandBuffer[size++] = pPcscCmdApdu->P1;

        RtlCopyMemory(&m_CommandBuffer[size], pPcscCmdApdu->DataIn, 8);
        size += 8;

        RtlCopyMemory(&m_CommandBuffer[size], uid, uidLength);
        size += uidLength;

        m_CmdBufferSize = size;
        retResult = RESULT_SUCCESS;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassJewel::ReadBinary(
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
StorageClassJewel::ValidateReadBinaryParamters(
                                BYTE p1,
                                BYTE p2,
                                BYTE le
                                )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_WRONG_P1P2;

    if (le == 0x00) {
        //
        // READ-ALL
        //
        if ((p1 == 0x00) && (p2 == 0x00)) {
            retResult = RESULT_SUCCESS;
        }
    } else if (le == 0x06) {
        //
        // READ-RID
        //
        if ((p1 == 0x00) && (p2 == 0x00)) {
            retResult = RESULT_SUCCESS;
        }
    } else if (le == 0x01) {
        //
        // READ
        //
        if ((p1 <= 0x0f) && (p2 <= 0x07)) {
            retResult = RESULT_SUCCESS;
        }
    } else if (le == 0x08) {
        //
        // READ-8
        //
        if ((p1 <= 0xff) && (p2 == 0x00)) {
            retResult = RESULT_SUCCESS;
        }
    } else if (le == 0x80) {
        //
        // READ-SEG
        //
        if ((p1 == 0x00) && (p2 <= 0x03)) {
            retResult = RESULT_SUCCESS;
        }
    } else {
        retResult = RESULT_ERROR;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassJewel::PrepareTransceiveForRead(
                             BYTE p1,
                             BYTE p2,
                             BYTE le
                             )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_NOT_SUPPORTED;
    DWORD size = 0;
    PBYTE uid = NULL;
    BYTE uidLength = 0;

    GetUniqueID(&uid, &uidLength);

    if (le == 0x00) {
        //
        // READ-ALL
        //
        m_CommandBuffer[size++] = TOPAZ_READALL_COMMAND;
        m_CommandBuffer[size++] = 0x00;
        m_CommandBuffer[size++] = 0x00;

        RtlCopyMemory(&m_CommandBuffer[size], uid, uidLength);
        size += uidLength;

        m_CmdBufferSize = size;
        retResult = RESULT_SUCCESS;

    } else if (le == 0x06) {
        //
        // READ-ID
        //
        m_CommandBuffer[size++] = TOPAZ_READID_COMMAND;
        
        RtlZeroMemory(&m_CommandBuffer[size], 6);
        size += 6;

        m_CmdBufferSize = size;
        retResult = RESULT_SUCCESS;

    } else if (le == 0x01) {
        //
        // READ
        //
        m_CommandBuffer[size++] = TOPAZ_READ_COMMAND;
        m_CommandBuffer[size++] = (p1 << 3) | p2;
        m_CommandBuffer[size++] = 0x00;

        RtlCopyMemory(&m_CommandBuffer[size], uid, uidLength);
        size += uidLength;

        m_CmdBufferSize = size;
        retResult = RESULT_SUCCESS;

    } else if (le == 0x08) {
        //
        // READ-8
        //
        m_CommandBuffer[size++] = TOPAZ_READ8_COMMAND;
        m_CommandBuffer[size++] = p1;

        RtlZeroMemory(&m_CommandBuffer[size], 8);
        size += 8;

        RtlCopyMemory(&m_CommandBuffer[size], uid, uidLength);
        size += uidLength;

        m_CmdBufferSize = size;
        retResult = RESULT_SUCCESS;

    } else if (le == 0x80) {
        //
        // READ-SEG
        //
        m_CommandBuffer[size++] = TOPAZ_RSEG_COMMAND;
        m_CommandBuffer[size++] = (p2 << 4);

        RtlZeroMemory(&m_CommandBuffer[size], 8);
        size += 8;

        RtlCopyMemory(&m_CommandBuffer[size], uid, uidLength);
        size += uidLength;

        m_CmdBufferSize = size;
        retResult = RESULT_SUCCESS;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult 
StorageClassJewel::GetGeneralAuthenticateCommand(
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
StorageClassJewel::UpdateResponseCodeToResponseBuffer(
                                    _In_ ApduResult /*RetError*/,
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

    if (PcscWriteCmd == Command) {
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
StorageClassJewel::StorageCardTransceive(
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
    else {
        retResult = RESULT_SUCCESS;
        TRACE_LINE(LEVEL_INFO, "Received data buffer size = %d", (DWORD)*pcbReturnBufferSize);
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retResult);
    return retResult;
}
