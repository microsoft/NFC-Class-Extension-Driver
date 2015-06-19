/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassMifare.h

Abstract:

    Storage card Mifare UL implementation
    
Environment:

    User mode

--*/
#include "Pch.h"

#include "StorageClassMifare.tmh"

StorageClassMiFareUL::StorageClassMiFareUL(IStorageCard *pStorageCard)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    m_pStorageCard = pStorageCard;
    m_IsAuthSupported = FALSE;
    m_IsLesserLeLc = FALSE;
    m_CmdBufferSize = 0;
    m_HistoricalByteLength = 0;
    m_UidLength = 0;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void StorageClassMiFareUL::UpdateUniqueID(
                                            _In_reads_bytes_(uidLength) BYTE *Uid,
                                            _In_ BYTE uidLength
                                            )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    m_UidLength = uidLength;
    RtlCopyMemory(m_Uid, Uid, m_UidLength);
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void
StorageClassMiFareUL::GetUniqueID(
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
StorageClassMiFareUL::UpdateHistoricalBytes(
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
StorageClassMiFareUL::GetDataCommand(
                                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                    _In_ DWORD cbSize,
                                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                    _In_ DWORD cbOutBufferSize,
                                    _Out_ DWORD *cbReturnBufferSize 
                                    )
{
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    // Ensure buffer covers LC
    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    retValue = ValidateGetDataCommandParamters(cmdApdu->P1, cmdApdu->P2);
    if (RESULT_SUCCESS != retValue) {
        //Wrong P1 and P2 value
        retValue = RESULT_WRONG_P1P2;
        goto Done;
    }

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    if (cmdApdu->P1 == 0x00) {
        BYTE *uid = NULL;
        BYTE uidLength = 0;

        GetUniqueID(&uid, &uidLength);

        if (cbOutBufferSize < uidLength) {
            retValue = RESULT_INVALID_PARAM;
            goto Done;
        }

        if (0 != cmdApdu->Lc) {
            if (uidLength > cmdApdu->Lc) {
                m_IsLesserLeLc = TRUE;
            }
            else if (uidLength < cmdApdu->Lc) {               
                if (cmdApdu->Lc > cbOutBufferSize) {
                    RtlZeroMemory(pbOutBuffer, cbOutBufferSize);
                    *cbReturnBufferSize = cbOutBufferSize;
                }
                else {
                    RtlZeroMemory(pbOutBuffer, cmdApdu->Lc);
                    *cbReturnBufferSize = cmdApdu->Lc;
                }
                RtlCopyMemory(pbOutBuffer, uid, uidLength);

                retValue = RESULT_LE_GREATER; // 0x62 and 0x82
            }
        }

        if (RESULT_SUCCESS == retValue) {
            RtlCopyMemory(pbOutBuffer, uid, uidLength);
            *cbReturnBufferSize = uidLength;
        }
    }
    else if (cmdApdu->P1 == 0x01) {
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

Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMiFareUL::ValidateGetDataCommandParamters(
                                                      BYTE p1,
                                                      BYTE p2
                                                      )
{
    ApduResult retResult = RESULT_WRONG_P1P2;

    //
    // Get UID
    //
    if((p1 == 0x00) && (p2 ==0x00)) {
        retResult = RESULT_SUCCESS;
    }
    else if((p1 == 0x01) && (p2 ==0x00)) {
        retResult = RESULT_SUCCESS;
    }
    return retResult;
}

ApduResult
StorageClassMiFareUL::ReadBinary(
                                 _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                                 _In_ DWORD cbSize,
                                 _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                 _In_ DWORD cbOutBufferSize,
                                 _Out_ DWORD *pcbReturnBufferSize
                                  )
{
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *pcbReturnBufferSize = 0;

    // ensure buffer covers min header
    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "P1 0x%x, P2 0x%x, Le 0x%x",cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);

    retValue = ValidateReadBinaryParamters(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if (RESULT_SUCCESS != retValue) {
        goto Done;
    }

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    RtlZeroMemory(m_CommandBuffer, sizeof(m_CommandBuffer));
    m_CmdBufferSize = 0;

    retValue = PrepareTransceiveForRead(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if((RESULT_SUCCESS == retValue) || (RESULT_LE_LESSER == retValue )) {
        if (cbOutBufferSize < m_CmdBufferSize) {
            retValue = RESULT_INVALID_PARAM;
            goto Done;
        }

        RtlCopyMemory(pbOutBuffer, m_CommandBuffer, m_CmdBufferSize);
        *pcbReturnBufferSize = m_CmdBufferSize;
    }
    
Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMiFareUL::ValidateReadBinaryParamters(
                                                    BYTE p1,
                                                    BYTE p2,
                                                    BYTE le
                                                    )
{
    // By default any parameter is invalid.
    ApduResult retValue = RESULT_SUCCESS;
    UNREFERENCED_PARAMETER(p2);
    UNREFERENCED_PARAMETER(le);
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // READ 16-bytes
    //
    if(p1 != 0x00) {
        retValue = RESULT_WRONG_P1P2;
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);
    return retValue;
}

ApduResult
StorageClassMiFareUL::PrepareTransceiveForRead(
                                               BYTE p1,
                                               BYTE p2,
                                               BYTE le
                                               )
{
    ApduResult retValue = RESULT_WRONG_P1P2;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    //
    // READ 16 bytes
    //
    TRACE_LINE(LEVEL_INFO, " P2 = 0x%02x, LE = 0x%02x", p2, le);
    if(p1 == 0x00) {
        m_CommandBuffer[0] = phNfc_eMifareRead16;
        m_CommandBuffer[1] = p2;

        m_CmdBufferSize = 2;

        if(MIFARE_UL_READ_BUFFER_SIZE == le || 0x00 == le) {
            retValue = RESULT_SUCCESS;
        }
        else {
            if(MIFARE_UL_READ_BUFFER_SIZE > le) {
                retValue = RESULT_SUCCESS;
                m_IsLesserLeLc = TRUE;
            }
            else if(MIFARE_UL_READ_BUFFER_SIZE < le) {
                retValue = RESULT_LE_GREATER;
            }
        }
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMiFareUL::UpdateBinary(
                                 _In_reads_bytes_(cbSize) BYTE *pbDataBuffer,
                                 _In_ DWORD cbSize,
                                 _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                 _In_ DWORD cbOutBufferSize,
                                 _Out_ DWORD *pcbReturnBufferSize
                                  )
{
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *pcbReturnBufferSize = 0;

    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }
    
    retValue = ValidateUpdateBinaryParameters(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if((RESULT_SUCCESS == retValue) || (RESULT_LE_LESSER == retValue)) {

        RtlZeroMemory(m_CommandBuffer, sizeof(m_CommandBuffer));
        m_CmdBufferSize = 0;

        retValue = PrepareTransceiveForUpdate(cmdApdu);
        if (RESULT_SUCCESS != retValue) {
            goto Done;
        }

        if (cbOutBufferSize < m_CmdBufferSize) {
            retValue = RESULT_INVALID_PARAM;
            goto Done;
        }

        RtlCopyMemory(pbOutBuffer, m_CommandBuffer, m_CmdBufferSize);
        *pcbReturnBufferSize = m_CmdBufferSize;
    }

Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMiFareUL::ValidateUpdateBinaryParameters(
                                                    BYTE p1,
                                                    BYTE p2,
                                                    BYTE lc
                                                    )
{
    ApduResult retValue = RESULT_WRONG_P1P2;
    UNREFERENCED_PARAMETER(lc);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    //
    // WRITE 16 byte
    //
    if(0x00 == p1 && p2 != 0x00) {
        retValue = RESULT_SUCCESS;
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);
    return retValue;
}

ApduResult
StorageClassMiFareUL::PrepareTransceiveForUpdate(
                                                 _In_ PPcscCommandApduInfo pPcscCmdApdu
                                                 )
{
    DWORD size = 0;
    ApduResult retValue = RESULT_INVALID_PARAM;

    //
    // Write 1 Byte with Erase
    //
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);
    TRACE_LINE(LEVEL_INFO, " (LC 0x%02x) (P2 0x%02x)", pPcscCmdApdu->Lc, pPcscCmdApdu->P2);

    if (sizeof(m_CommandBuffer) < (pPcscCmdApdu->Lc + 4)) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    if(pPcscCmdApdu->Lc == MIFARE_UL_PAGE_SIZE) {
        retValue = RESULT_SUCCESS;
    }
    else {
        if (MIFARE_UL_PAGE_SIZE > pPcscCmdApdu->Lc) {
            retValue = RESULT_LE_LESSER;
        }
        if (MIFARE_UL_PAGE_SIZE < pPcscCmdApdu->Lc) {
            retValue = RESULT_LE_GREATER;
        }
    }

    m_CommandBuffer[0] = phNfc_eMifareWrite4;
    m_CommandBuffer[1] = pPcscCmdApdu->P2;
    size = 2;

    RtlCopyMemory(m_CommandBuffer + 2, pPcscCmdApdu->DataIn, pPcscCmdApdu->Lc);
    size += pPcscCmdApdu->Lc;

    m_CmdBufferSize = size;

Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMiFareUL::GetGeneralAuthenticateCommand(
                                                    _In_ void* getLoadKey,
                                                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                                    _In_ DWORD cbSize,
                                                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                                    _In_ DWORD cbOutBufferSize,
                                                    _Out_ DWORD *cbReturnBufferSize 
                                                    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_NOT_SUPPORTED;
    
    UNREFERENCED_PARAMETER(pbDataBuffer);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pbOutBuffer);
    UNREFERENCED_PARAMETER(getLoadKey);
    UNREFERENCED_PARAMETER(cbOutBufferSize);

    *cbReturnBufferSize = 0;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

void
StorageClassMiFareUL::UpdateResponseCodeToResponseBuffer(
                                                          _In_ ApduResult RetError,
                                                          _In_ PcscInsByte Command,
                                                          _In_ DWORD cbSize,
                                                          _Inout_updates_bytes_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[],
                                                          _Inout_updates_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                                          _In_ DWORD cbOutBufferSize,
                                                          _Out_ DWORD *cbReturnBufferSize 
                                                        )
{
    PBYTE pbOutBufferStatus = NULL;
    BYTE Sw1Sw2[DEFAULT_APDU_STATUS_SIZE] = {0};
    NTSTATUS status = STATUS_SUCCESS;
    DWORD tempMaxSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

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
        *cbReturnBufferSize = DEFAULT_APDU_STATUS_SIZE;

        TRACE_LINE(LEVEL_ERROR, "Invalid buffer size, cbOutBufferSize=%d", cbOutBufferSize);
        goto Done;
    }

    Sw1Sw2[0] = Sw1Sw2Return[0];
    Sw1Sw2[1] = Sw1Sw2Return[1];

    if (PcscReadCmd == Command) {
        if (RESULT_SUCCESS == RetError) {
            if(m_IsLesserLeLc) {
                m_IsLesserLeLc = FALSE;
                Sw1Sw2[0] = 0x6C;
                Sw1Sw2[1] = MIFARE_UL_READ_BUFFER_SIZE;
            }
        }
    }
    else if (PcscWriteCmd == Command) {
        RtlZeroMemory(pbOutBuffer, cbSize);
        cbSize = 0;
    }
    else if (PcscGetDataCmd == Command) {
        if (m_IsLesserLeLc) {
            BYTE *uid = NULL;
            BYTE uidLength = 0;

            GetUniqueID(&uid, &uidLength);

            Sw1Sw2[0] = 0x6C;
            Sw1Sw2[1] = uidLength;
            m_IsLesserLeLc = FALSE;
        }
    }

    pbOutBufferStatus = pbOutBuffer + cbSize;
    RtlCopyMemory(pbOutBufferStatus, Sw1Sw2, DEFAULT_APDU_STATUS_SIZE);
    *cbReturnBufferSize = cbSize + DEFAULT_APDU_STATUS_SIZE;

Done:

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

ApduResult
StorageClassMiFareUL::HandleIncDecCommand(
                                            _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                            _In_ DWORD cbSize,
                                            _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                            _In_ DWORD cbOutBufferSize,
                                            _Out_ DWORD *cbReturnBufferSize 
                                            )
{
    ApduResult retResult = RESULT_NOT_SUPPORTED;
    
    UNREFERENCED_PARAMETER(pbDataBuffer);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pbOutBuffer);
    UNREFERENCED_PARAMETER(cbOutBufferSize);

    *cbReturnBufferSize = 0;

    return retResult;
}

ApduResult
StorageClassMiFareUL::StorageCardTransceive(
                                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                        _In_ DWORD cbSize,
                                        _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                        _In_ DWORD cbOutBufferSize,
                                        _Out_ DWORD *cbReturnBufferSize,
                                        _In_ USHORT usTimeout
                                        )
{
    ApduResult retValue = RESULT_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    retValue = MifareTransceive(pbDataBuffer, 
                                cbSize, 
                                pbOutBuffer, 
                                cbOutBufferSize, 
                                cbReturnBufferSize,
                                usTimeout);

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retValue);

    return retValue;
}

ApduResult
StorageClassMiFareUL::MifareTransceive(
                                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                        _In_ DWORD cbSize,
                                        _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                        _In_ DWORD cbOutBufferSize,
                                        _Out_ DWORD *cbReturnBufferSize,
                                        _In_ USHORT usTimeout
                                        )
{
    ApduResult retValue = RESULT_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    status = m_pStorageCard->Transmit((PBYTE)pbDataBuffer,
                                      cbSize,
                                      pbOutBuffer,
                                      cbOutBufferSize,
                                      cbReturnBufferSize,
                                      usTimeout);
    if (!NT_SUCCESS(status)) {
        retValue = RESULT_COMMAND_INCOMPATIBLE; //6981;
        *cbReturnBufferSize = 0;
    }
    else {
        retValue = RESULT_SUCCESS;

        TRACE_LINE(LEVEL_INFO, "Received data buffer size = %d", (DWORD)*cbReturnBufferSize);

        // Check for ACK/NAK if one byte is returned
        if ((DWORD)*cbReturnBufferSize == 1)
        {
            switch (pbOutBuffer[0])
            {
            case MIFARE_NAK_ACK:
                retValue = RESULT_SUCCESS;
                break;
            case MIFARE_NAK_ACCESS_DENIED:
                retValue = RESULT_INVALID_BLOCK_ADDRESS;
                break;
            case MIFARE_NAK_UNDER_OR_OVERFLOW:
                retValue = RESULT_MEMORY_FAILURE;
                break;
            case MIFARE_NAK_CRC_ERROR1:
            case MIFARE_NAK_CRC_ERROR2:
                retValue = RESULT_SECURITY_STATUS_NOT_SATISFIED;
                break;
            case MIFARE_NAK_RFU1:
            case MIFARE_NAK_RFU2:
            default:
                retValue = RESULT_ERROR;
                break;
            }

            // Remove NAK from the result data
            pbOutBuffer[0] = 0;
            *cbReturnBufferSize = 0;
        }
    }

    if (*cbReturnBufferSize > cbOutBufferSize) {
        retValue = RESULT_INVALID_PARAM;
        *cbReturnBufferSize = 0;
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retValue);

    return retValue;
}
