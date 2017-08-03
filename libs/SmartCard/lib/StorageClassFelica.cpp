/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassFelica.cpp

Abstract:

    Storage card Felica implementation
    
Environment:

    User mode

--*/

#include "Pch.h"

#include "StorageClassFelica.tmh"

StorageClassFelica::StorageClassFelica(IStorageCard *pStorageCard)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    m_pStorageCard = pStorageCard;
    m_IsAuthSupported = FALSE;
    m_HistoricalByteLength = 0;
    m_CmdBufferSize = 0;
    m_UidLength = 0;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void StorageClassFelica::UpdateUniqueID(
                                        _In_reads_bytes_(uidLength) BYTE *pUid,
                                        _In_ BYTE uidLength
                                        )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    m_UidLength = uidLength;
    RtlCopyMemory(m_Uid, pUid, m_UidLength);
    TRACE_LINE(LEVEL_INFO, "m_UidLength=%d", m_UidLength);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void
StorageClassFelica::GetUniqueID(
                                _Outptr_result_bytebuffer_(*pUidLength) BYTE **pUid,
                                _Out_ BYTE *pUidLength
                               )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *pUidLength = m_UidLength;
    *pUid = m_Uid;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

void
StorageClassFelica::UpdateHistoricalBytes(
                                        _In_reads_bytes_(HistoBytesLength) BYTE *pHistoBytes,
                                        _In_ DWORD HistoBytesLength
                                        )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    m_HistoricalByteLength = HistoBytesLength;
    RtlCopyMemory(m_HistoricalBytes, pHistoBytes, m_HistoricalByteLength);
    TRACE_LINE(LEVEL_INFO, "m_HistoricalByteLength=%d", m_HistoricalByteLength);

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

ApduResult 
StorageClassFelica::HandleIncDecCommand(
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
StorageClassFelica::GetDataCommand(
                            _In_reads_bytes_(size) const BYTE *pbDataBuffer,
                            _In_ DWORD size,
                            _Out_writes_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                            _In_ DWORD cbOutBufferSize,
                            _Out_ DWORD *pcbReturnBufferSize 
                           )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult retResult = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    *pcbReturnBufferSize = 0;

    // Ensure buffer covers LC
    if (size < MIN_APDU_HEADER) {
        retResult = RESULT_INVALID_PARAM;
        goto Done;
    }

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    if (cmdApdu->P1 == 0x00 && cmdApdu->P2 == 0x00) { // Get UID bytes
        BYTE *uid = NULL;
        BYTE uidLength = 0;

        GetUniqueID(&uid, &uidLength);

        if (cbOutBufferSize < uidLength) {
            retResult = RESULT_INVALID_PARAM;
            goto Done;
        }

        if (0 != cmdApdu->Lc) {
            if (uidLength > cmdApdu->Lc) {
                RtlCopyMemory(pbOutBuffer, uid, uidLength);
                *pcbReturnBufferSize = uidLength;
                retResult = RESULT_LE_LESSER; // 0x6C and XX.
            }
            else if (uidLength < cmdApdu->Lc) {
                RtlCopyMemory(pbOutBuffer, uid, uidLength);
                RtlZeroMemory(pbOutBuffer + uidLength, (cmdApdu->Lc - uidLength));
                *pcbReturnBufferSize = cmdApdu->Lc;
                retResult = RESULT_LE_GREATER; // 0x62 and 0x82
            }
        }

        if (RESULT_SUCCESS == retResult) {
            RtlCopyMemory(pbOutBuffer, uid, uidLength);
            *pcbReturnBufferSize = uidLength;
        }
    }
    else if (cmdApdu->P1 == 0x01 && cmdApdu->P2 == 0x00) { // Get Historical bytes
        retResult = RESULT_NOT_SUPPORTED;
        goto Done;
    }
    else { // Wrong P1 and P2 value
        retResult = RESULT_WRONG_P1P2;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassFelica::ReadBinary(
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

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    retResult = ValidateReadBinaryParamters(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if (RESULT_SUCCESS == retResult) {
        RtlZeroMemory(m_CommandBuffer, COMMAND_BUFFER_SIZE);
        m_CmdBufferSize = 0;

        retResult = PrepareTransceiveForRead(cmdApdu);
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
StorageClassFelica::ValidateReadBinaryParamters(
                                BYTE p1,
                                BYTE p2,
                                BYTE le
                                )
{
    ApduResult retResult = RESULT_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if ((0x00 != p1) || (0x00 != p2)) {
        retResult = RESULT_WRONG_P1P2;
        goto Done;
    }

    if (0x06 > le || 0x12 < le) {
        retResult = RESULT_WRONG_LENGTH;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassFelica::PrepareTransceiveForRead(
                             _In_ PPcscCommandApduInfo pPcscCmdApdu
                             )
{
    ApduResult  retResult    = RESULT_SUCCESS;
    BYTE        size         = 1;
    BYTE        *mID         = NULL;
    BYTE        mIDLength    = 0;
    DWORD       index        = 0;
    DWORD       blockCount   = 0;
    USHORT      serviceCount = pPcscCmdApdu->DataIn[index++];

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Validate number of Services
    if (1 != serviceCount) {
        retResult = RESULT_INVALID_SERVICE_COUNT; //0xFF 0xA1h
        goto Done;
    }

    // Service code List (Little Endian format)
    index += (serviceCount * 2);

    // Number of Blocks
    blockCount = pPcscCmdApdu->DataIn[index++];

    if ((blockCount < 0x01) || (blockCount > 0x04)) {
        retResult = RESULT_INVALID_BLOCK_COUNT;  //0xFF 0xA2h
        goto Done;
    }

    m_CommandBuffer[size++] = FELICA_READ_BLOCK;

    // Manufacturer ID
    GetUniqueID(&mID, &mIDLength);

    RtlCopyMemory(&m_CommandBuffer[size], mID, mIDLength);
    size += mIDLength;

    RtlCopyMemory(&m_CommandBuffer[size], pPcscCmdApdu->DataIn, pPcscCmdApdu->Lc);
    size += pPcscCmdApdu->Lc;

    m_CommandBuffer[0] = size;
    m_CmdBufferSize = size;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassFelica::UpdateBinary(
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
StorageClassFelica::ValidateUpdateBinaryParameters(
                                     BYTE  p1,
                                     BYTE  p2,
                                     BYTE  lc
                                     )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    ApduResult  retResult = RESULT_SUCCESS;

    if ((0x00 != p1) || (0x00 != p2)) {
        retResult = RESULT_WRONG_P1P2;
        goto Done;
    }

    if (0x16 > lc) {
        retResult = RESULT_WRONG_LENGTH;
        goto Done;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult
StorageClassFelica::PrepareTransceiveForUpdate(
                               _In_ PPcscCommandApduInfo pPcscCmdApdu
                               )
{
    ApduResult  retResult           = RESULT_SUCCESS;
    BYTE        size                = 1;
    BYTE        *mID                = NULL;
    BYTE        mIDLength           = 0;
    DWORD       index               = 0;
    USHORT      serviceCount        = pPcscCmdApdu->DataIn[index++];
    USHORT      blockListLength     = 0;
    USHORT      numOfBlocks         = 1;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    // Validate number of Serices
    if (1 != serviceCount) {
        retResult = RESULT_INVALID_SERVICE_COUNT; //0xFF 0xA1h
        goto Done;
    }

    // Service code List (Little Endian format)
    index += (serviceCount * 2);

    numOfBlocks = pPcscCmdApdu->DataIn[index++];
    
    // Number of Blocks
    if (0x01 != numOfBlocks) {
        retResult = RESULT_INVALID_BLOCK_COUNT;  //0xFF 0xA2h
        goto Done;
    }

    // If last bit '1' indicates block list length 2 bytes else 3 bytes
    if ((pPcscCmdApdu->DataIn[index] & 0xFF) == 0x80) {
        blockListLength = 0x02;
    }
    else if ((pPcscCmdApdu->DataIn[index] & 0xFF) == 0x00) {
        blockListLength = 0x03;
    }
    else {
        retResult = RESULT_INVALID_BLOCK_LIST; //0x01 0xA1h
        goto Done;
    }

    index++;

    if (pPcscCmdApdu->Lc != (serviceCount + (serviceCount * 2) +
                             numOfBlocks + (blockListLength * numOfBlocks) + (numOfBlocks * 16))) {
        retResult = RESULT_INVALID_DATA_SIZE; //0x01 0xA9h
        goto Done;
    }

    m_CommandBuffer[size++] = FELICA_WRITE_BLOCK;

    // Manufacturer ID
    GetUniqueID(&mID, &mIDLength);

    RtlCopyMemory(&m_CommandBuffer[size], mID, mIDLength);
    size += mIDLength;

    RtlCopyMemory(&m_CommandBuffer[size], pPcscCmdApdu->DataIn, pPcscCmdApdu->Lc);
    size += pPcscCmdApdu->Lc;

    m_CommandBuffer[0] = size;
    m_CmdBufferSize = size;

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retResult;
}

ApduResult 
StorageClassFelica::GetGeneralAuthenticateCommand(
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
StorageClassFelica::UpdateResponseCodeToResponseBuffer(
                                    _In_ ApduResult /*RetError*/,
                                    _In_ PcscInsByte Command,
                                    _In_ DWORD cbSize,
                                    _Inout_updates_bytes_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[],
                                    _Inout_updates_bytes_to_(cbOutBufferSize, *pcbReturnBufferSize) BYTE *pbOutBuffer,
                                    _In_ DWORD cbOutBufferSize,
                                    _Out_ DWORD *pcbReturnBufferSize  
                                    )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    NTSTATUS status = STATUS_SUCCESS;
    DWORD responseLengthSize = 1;
    DWORD responseCodeSize = 1;
    DWORD idmLength = 8;
    DWORD blockSize = 1;
    DWORD statusOffset = responseLengthSize + responseCodeSize + idmLength;
    DWORD responseDataOffset = statusOffset + DEFAULT_APDU_STATUS_SIZE + blockSize;
    DWORD responseDataSize = 0;
    DWORD tempMaxSize = 0;

    *pcbReturnBufferSize = 0;

    if (cbOutBufferSize < DEFAULT_APDU_STATUS_SIZE) {
        NT_ASSERTMSG("The cbOutBufferSize should be atleast DEFAULT_APDU_STATUS_SIZE", FALSE);
        goto Done;
    }

    if (cbOutBufferSize < cbSize) {
        Sw1Sw2Return[0] = 0x63;
        Sw1Sw2Return[1] = 0x00;

        RtlCopyMemory(pbOutBuffer, Sw1Sw2Return, DEFAULT_APDU_STATUS_SIZE);
        *pcbReturnBufferSize = DEFAULT_APDU_STATUS_SIZE;
        goto Done;
    }

    if (PcscReadCmd == Command) {
        if (cbSize < (statusOffset + DEFAULT_APDU_STATUS_SIZE)) {
            RtlCopyMemory(pbOutBuffer, Sw1Sw2Return, DEFAULT_APDU_STATUS_SIZE);
            *pcbReturnBufferSize = DEFAULT_APDU_STATUS_SIZE;
            goto Done;
        }

        if (pbOutBuffer[statusOffset] == 0x00 && pbOutBuffer[statusOffset+1] == 0x00) {
            Sw1Sw2Return[0] = 0x90;
            Sw1Sw2Return[1] = 0x00;
        }
        else {
            RtlCopyMemory(Sw1Sw2Return, &pbOutBuffer[statusOffset], DEFAULT_APDU_STATUS_SIZE);
        }

        if (cbSize > responseDataOffset) {
            responseDataSize = cbSize - responseDataOffset;
            RtlMoveMemory(pbOutBuffer, &pbOutBuffer[responseDataOffset], responseDataSize);
        }

        *pcbReturnBufferSize = responseDataSize;
        RtlCopyMemory(pbOutBuffer + *pcbReturnBufferSize, Sw1Sw2Return, DEFAULT_APDU_STATUS_SIZE);
        *pcbReturnBufferSize += DEFAULT_APDU_STATUS_SIZE;
    }
    else if (PcscWriteCmd == Command) {
        if (cbSize < (statusOffset + DEFAULT_APDU_STATUS_SIZE)) {
            RtlCopyMemory(pbOutBuffer, Sw1Sw2Return, DEFAULT_APDU_STATUS_SIZE);
            *pcbReturnBufferSize = DEFAULT_APDU_STATUS_SIZE;
            goto Done;
        }

        if (pbOutBuffer[statusOffset] == 0x00 && pbOutBuffer[statusOffset+1] == 0x00) {
            Sw1Sw2Return[0] = 0x90;
            Sw1Sw2Return[1] = 0x00;
        }
        else {
            RtlCopyMemory(Sw1Sw2Return, &pbOutBuffer[statusOffset], DEFAULT_APDU_STATUS_SIZE);
        }

        RtlZeroMemory(pbOutBuffer, cbOutBufferSize);
        RtlCopyMemory(pbOutBuffer, Sw1Sw2Return, DEFAULT_APDU_STATUS_SIZE);
        *pcbReturnBufferSize = DEFAULT_APDU_STATUS_SIZE;
    }
    else {
        status = RtlULongAdd(cbSize, DEFAULT_APDU_STATUS_SIZE, &tempMaxSize);
        if (!NT_SUCCESS(status) ||
            cbOutBufferSize < tempMaxSize) {
            Sw1Sw2Return[0] = 0x6C;
            Sw1Sw2Return[1] = (BYTE)cbSize;
            
            RtlCopyMemory(pbOutBuffer, Sw1Sw2Return, DEFAULT_APDU_STATUS_SIZE);
            *pcbReturnBufferSize = DEFAULT_APDU_STATUS_SIZE;
            goto Done;
        }

        RtlCopyMemory(pbOutBuffer + cbSize, Sw1Sw2Return, DEFAULT_APDU_STATUS_SIZE);
        *pcbReturnBufferSize = cbSize + DEFAULT_APDU_STATUS_SIZE;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

ApduResult
StorageClassFelica::StorageCardTransceive(
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
