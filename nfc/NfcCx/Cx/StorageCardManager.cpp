/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageCardManager.cpp

Abstract:

    Storage card implementation
    
Environment:

    User mode

--*/
#include "NfcCxPch.h"

#include "StorageCardManager.tmh"

StorageCardManager::StorageCardManager(
                                       _In_ phNfc_eRemDevType_t DeviceType,
                                       _In_ DWORD Sak,
                                       _In_ PNFCCX_SC_INTERFACE pScInterface
                                       )
    : m_cRef(1),
      m_pStorageClass(NULL),
      m_pScInterface(pScInterface),
      m_fTransparentSession(FALSE)
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    switch (DeviceType)
    {
    case phLibNfc_eJewel_PICC:
        m_pStorageClass = new StorageClassJewel(this);
        break;

    case phLibNfc_eMifare_PICC:
        if (Sak == 0x00) {
            m_pStorageClass = new StorageClassMiFareUL(this);
        }
        else {
            m_pStorageClass = new StorageClassMifareStd(this);
        }
        break;

    case phLibNfc_eFelica_PICC:
        m_pStorageClass = new StorageClassFelica(this);
        break;

    case phLibNfc_eISO15693_PICC:
        m_pStorageClass = new StorageClassISO15693(this);
        break;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

StorageCardManager::~StorageCardManager()
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (m_pStorageClass != NULL) {
        delete m_pStorageClass;
        m_pStorageClass = NULL;
    }

    m_pScInterface = NULL;

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

StorageCardManager*
StorageCardManager::Create(
                            _In_ phNfc_eRemDevType_t DeviceType,
                            _In_ DWORD Sak,
                            _In_ PNFCCX_SC_INTERFACE PSCInterface
                            )
{
    StorageCardManager* pManager = NULL;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    pManager = new StorageCardManager(DeviceType, Sak, PSCInterface);

    if (pManager == NULL) {
        TRACE_LINE(LEVEL_ERROR, "Insufficient resources");
        goto Done;
    }

    if (pManager->m_pStorageClass == NULL) {
        delete pManager;
        pManager = NULL;
    }

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return pManager;
}

ApduResult
StorageCardManager::ValidateParameters(
                                       _In_opt_ BYTE *dataBuffer,
                                       _In_opt_ BYTE *outBuffer
                                       )
{
    ApduResult retValue = RESULT_SUCCESS;

    UNREFERENCED_PARAMETER(outBuffer);

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if(NULL == dataBuffer) {
        retValue = RESULT_INVALID_PARAM;
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);
    return retValue;
}
ApduResult
StorageCardManager::GetCommandFromAPDU(
                                        _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                        _In_ DWORD cbSize,
                                        _Out_ PcscInsByte *pCommand
                                        )
{
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;
    
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *pCommand = PcscInvalidCmd;
    
    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    if (0xFF != cmdApdu->Cla) {
        retValue = RESULT_WRONG_CLA;
        goto Done;
    }

    *pCommand = (PcscInsByte)0x00;

    switch (cmdApdu->Ins)
    {
        case PcscGetDataCmd:
        {
            *pCommand = PcscGetDataCmd;
        }
        break;
        case PcscReadCmd:
        {
            *pCommand = PcscReadCmd;
        }
        break;
        case PcscWriteCmd:
        {
            *pCommand = PcscWriteCmd;
        }
        break;
        case PcscMifareGenAuthCmd:
        {
            *pCommand = PcscMifareGenAuthCmd;
        }
        break;
        case PcscEnvelopeCmd:
        {
            if (0x00 == cmdApdu->P1 && 0x00 == cmdApdu->P2) {
                *pCommand = PcscManageSessionCmd;
            }
            else if (0x00 == cmdApdu->P1 && 0x01 == cmdApdu->P2) {
                *pCommand = PcscTransExchangeCmd;
            }
            else if (0x00 == cmdApdu->P1 && 0x02 == cmdApdu->P2) {
                *pCommand = PcscSwitchProtocolCmd;
            }
            else if (0x00 == cmdApdu->P1 && 0x03 == cmdApdu->P2) {
                *pCommand = PcscIncrementDecrementCmd;
            }
        }
        break;
    }

Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

void
StorageCardManager::PrepareResponseCode(
                                         _In_ ApduResult retError,
                                         _Out_writes_bytes_all_(DEFAULT_APDU_STATUS_SIZE) BYTE Sw1Sw2Return[]
                                         )
{
    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    switch (retError)
    {
        case RESULT_SUCCESS:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_SUCCESS");
            Sw1Sw2Return[0] = 0x90;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_INVALID_PARAM:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_INVALID_PARAM");
            Sw1Sw2Return[0] = 0x63;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_WRONG_CLA:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_WRONG_CLA");
            Sw1Sw2Return[0] = 0x68;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_WRONG_P1P2:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_WRONG_P1P2");
            Sw1Sw2Return[0] = 0x6B;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_NOT_SUPPORTED:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_NOT_SUPPORTED");
            Sw1Sw2Return[0] = 0x6A;
            Sw1Sw2Return[1] = 0x81;
        }
        break;
        case RESULT_DONOT_SEND:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_DONOT_SEND");
            Sw1Sw2Return[0] = 0x90;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_COMMAND_INCOMPATIBLE:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_COMMAND_INCOMPATIBLE");
            Sw1Sw2Return[0] = 0x69;
            Sw1Sw2Return[1] = 0x81;
        }
        break;
        case RESULT_LE_LESSER:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_LE_LESSER");
            Sw1Sw2Return[0] = 0x6C;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_LE_GREATER:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_LE_GREATER");
            Sw1Sw2Return[0] = 0x62;
            Sw1Sw2Return[1] = 0x82;
        }
        break;
        case RESULT_WRONG_LENGTH: //0x67, 0x00
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_WRONG_LENGTH");
            Sw1Sw2Return[0] = 0x67;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_AUTH_PARAM_ERR: //0x65, 0x81
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_AUTH_PARAM_ERR");
            Sw1Sw2Return[0] = 0x65;
            Sw1Sw2Return[1] = 0x81;
        }
        break;
        case RESULT_WRONG_KEY_TYPE:   // 0x69, 0x86
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_WRONG_KEY_TYPE");
            Sw1Sw2Return[0] = 0x69;
            Sw1Sw2Return[1] = 0x86;
        }
        break;
        case RESULT_WRONG_KEY_NR:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_WRONG_KEY_NR");
            Sw1Sw2Return[0] = 0x69;
            Sw1Sw2Return[1] = 0x88;
        }
        break;
        case RESULT_WRONG_AUTH_VERSION:  //  0x69, 0x83.
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_WRONG_AUTH_VERSION");
            Sw1Sw2Return[0] = 0x69;
            Sw1Sw2Return[1] = 0x83;
        }
        break;
        case RESULT_SECURITY_STATUS_NOT_SATISFIED:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_SECURITY_STATUS_NOT_SATISFIED");
            Sw1Sw2Return[0] = 0x69;
            Sw1Sw2Return[1] = 0x82;
        }
        break;
        case RESULT_MEMORY_FAILURE:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_MEMORY_FAILURE");
            Sw1Sw2Return[0] = 0x65;
            Sw1Sw2Return[1] = 0x81;
        }
        break;
        case RESULT_INVALID_BLOCK_ADDRESS:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_INVALID_BLOCK_ADDRESS");
            Sw1Sw2Return[0] = 0x6A;
            Sw1Sw2Return[1] = 0x82;
        }
        break;
        case RESULT_COMMAND_NOT_ALLOWED:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_COMMAND_NOT_ALLOWED");
            Sw1Sw2Return[0] = 0x69;
            Sw1Sw2Return[1] = 0x86;
        }
        break;
        case RESULT_INVALID_SERVICE_COUNT:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_INVALID_SERVICE_COUNT");
            Sw1Sw2Return[0] = 0xFF;
            Sw1Sw2Return[1] = 0xA1;
        }
        break;
        case RESULT_WRONG_SERVICE_CODE_ACCESS:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_WRONG_SERVICE_CODE_ACCESS");
            Sw1Sw2Return[0] = 0x01;
            Sw1Sw2Return[1] = 0xA6;
        }
        break;
        case RESULT_INVALID_BLOCK_COUNT:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_INVALID_BLOCK_COUNT");
            Sw1Sw2Return[0] = 0xFF;
            Sw1Sw2Return[1] = 0xA2;
        }
        break;
        case RESULT_INVALID_BLOCK_LIST:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_INVALID_BLOCK_LIST");
            Sw1Sw2Return[0] = 0x01;
            Sw1Sw2Return[1] = 0xA1;
        }
        break;
        case RESULT_INVALID_DATA_SIZE:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_INVALID_DATA_SIZE");
            Sw1Sw2Return[0] = 0x01;
            Sw1Sw2Return[1] = 0xA9;
        }
        break;
        case RESULT_INVALID_VALUE:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_INVALID_VALUE");
            Sw1Sw2Return[0] = 0x6A;
            Sw1Sw2Return[1] = 0x80;
        }
        break;
        case RESULT_UNKNOWN_FAILURE:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_UNKNOWN_FAILURE");
            Sw1Sw2Return[0] = 0x6F;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
        case RESULT_ERROR:
        {
            TRACE_LINE(LEVEL_INFO, "RESULT_ERROR");
            Sw1Sw2Return[0] = 0xFF;
            Sw1Sw2Return[1] = 0xFF;
        }
        break;
        default:
        {
            TRACE_LINE(LEVEL_INFO, "DEFAULT CASE");
            Sw1Sw2Return[0] = 0x00;
            Sw1Sw2Return[1] = 0x00;
        }
        break;
    }
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
}

ApduResult
StorageCardManager::HandleManageSessionCommand(
                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                    _In_ DWORD cbSize,
                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                    _In_ DWORD cbOutBufferSize,
                    _Out_ DWORD *cbReturnBufferSize  
                    )
{
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;
    DWORD index = 0;
    BYTE dataObjectNumber = 1;
    BYTE errorStatus[3], ifdVersion[3];
    const BYTE *tag = NULL, *value = NULL;
    DWORD tagSize = 0, lengthSize = 0, valueSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    while (index < cmdApdu->Lc) {

        retValue = ReadTlvDataObject(cmdApdu->DataIn,
                                     cmdApdu->Lc,
                                     index,
                                     &tag,
                                     &tagSize,
                                     &lengthSize,
                                     &value,
                                     &valueSize);

        if (retValue != RESULT_SUCCESS) {
            TRACE_LINE(LEVEL_ERROR, "ReadTlvDataObject failed");
            break;
        }

        if (tagSize == 1 && tag[0] == VersionCmd) {
            TRACE_LINE(LEVEL_INFO, "Version Command");

            if (valueSize != sizeof(ifdVersion)) {
                retValue = RESULT_INVALID_PARAM;
                TRACE_LINE(LEVEL_ERROR, "Invalid length");
                break;
            }

            ifdVersion[0] = IFD_MAJOR_VER;
            ifdVersion[1] = IFD_MINOR_VER;
            ifdVersion[2] = IFD_BUILD_NUM;

            retValue = AppendTlvDataObjectToResponseBuffer(
                            VersionRsp,
                            sizeof(ifdVersion),
                            ifdVersion,
                            pbOutBuffer,
                            cbOutBufferSize,
                            cbReturnBufferSize);

            if (retValue != RESULT_SUCCESS) {
                TRACE_LINE(LEVEL_ERROR, "AppendTlvDataObjectToResponseBuffer failed");
                break;
            }
        }
        else if (tagSize == 1 && tag[0] == StartTransSessionCmd) {
            TRACE_LINE(LEVEL_INFO, "Start Transparent Session");

            if (valueSize != 0x0) {
                retValue = RESULT_INVALID_PARAM;
                TRACE_LINE(LEVEL_ERROR, "Invalid length");
                break;
            }

            m_fTransparentSession = TRUE;
        }
        else if (tagSize == 1 && tag[0] == EndTransSessionCmd) {
            TRACE_LINE(LEVEL_INFO, "End Transparent Session");

            if (valueSize != 0x0) {
                retValue = RESULT_INVALID_PARAM;
                TRACE_LINE(LEVEL_ERROR, "Invalid length");
                break;
            }

            m_fTransparentSession = FALSE;
        }
        else {
            retValue = RESULT_NOT_SUPPORTED;
            TRACE_LINE(LEVEL_ERROR, "Data object not supported");
            break;
        }

        dataObjectNumber++;
        index += (tagSize + lengthSize + valueSize);
    }

    errorStatus[0] = (retValue == RESULT_SUCCESS) ? 0 : dataObjectNumber;
    PrepareResponseCode(retValue, &errorStatus[1]);

    retValue = AppendTlvDataObjectToResponseBuffer(
        GenericErrorRsp,
        sizeof(errorStatus),
        errorStatus,
        pbOutBuffer,
        cbOutBufferSize,
        cbReturnBufferSize);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retValue;
}

ApduResult
StorageCardManager::HandleTransSessionCommand(
                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                    _In_ DWORD cbSize,
                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                    _In_ DWORD cbOutBufferSize,
                    _Out_ DWORD *cbReturnBufferSize  
                    )
{
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;
    DWORD index = 0;
    BYTE dataObjectNumber = 1;
    BYTE errorStatus[3], responseStatus[2];
    DWORD dwTimeoutMs = 0;
    BYTE outputBuffer[255];
    DWORD responseSize = 0;
    const BYTE *tag = NULL, *value = NULL;
    DWORD tagSize = 0, lengthSize = 0, valueSize = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    while (index < cmdApdu->Lc) {

        retValue = ReadTlvDataObject(cmdApdu->DataIn,
                                     cmdApdu->Lc,
                                     index,
                                     &tag,
                                     &tagSize,
                                     &lengthSize,
                                     &value,
                                     &valueSize);

        if (retValue != RESULT_SUCCESS) {
            TRACE_LINE(LEVEL_ERROR, "ReadTlvDataObject failed");
            break;
        }

        if (tagSize == 1 && tag[0] == TransceiveCmd) {
            TRACE_LINE(LEVEL_INFO, "Transceive Command");

            if (!m_fTransparentSession) {
                retValue = RESULT_COMMAND_NOT_ALLOWED;
                TRACE_LINE(LEVEL_ERROR, "Transparent Session not started");
                break;
            }

            retValue = StorageCardTransceive(
                value,
                valueSize,
                outputBuffer,
                sizeof(outputBuffer),
                &responseSize,
                (USHORT)dwTimeoutMs);

            PrepareResponseCode(retValue, responseStatus);

            retValue = AppendTlvDataObjectToResponseBuffer(
                ResponseStatus,
                sizeof(responseStatus),
                responseStatus,
                pbOutBuffer,
                cbOutBufferSize,
                cbReturnBufferSize);

            if (retValue != RESULT_SUCCESS) {
                TRACE_LINE(LEVEL_ERROR, "AppendTlvDataObjectToResponseBuffer failed");
                break;
            }

            retValue = AppendTlvDataObjectToResponseBuffer(
                IccResponse,
                (BYTE)responseSize,
                outputBuffer,
                pbOutBuffer,
                cbOutBufferSize,
                cbReturnBufferSize);

            if (retValue != RESULT_SUCCESS) {
                TRACE_LINE(LEVEL_ERROR, "AppendTlvDataObjectToResponseBuffer failed");
                break;
            }
        }
        else if (tagSize == 2 && tag[0] == HIBYTE(TimerCmd) && tag[1] == LOBYTE(TimerCmd)) {
            TRACE_LINE(LEVEL_INFO, "Timer Command");

            if (valueSize != sizeof(DWORD)) {
                retValue = RESULT_INVALID_PARAM;
                TRACE_LINE(LEVEL_ERROR, "Invalid length");
                break;
            }

            dwTimeoutMs = *((PDWORD)&cmdApdu->DataIn[index-sizeof(DWORD)]); // in microseconds
            dwTimeoutMs /= 1000;
        }
        else {
            retValue = RESULT_NOT_SUPPORTED;
            TRACE_LINE(LEVEL_ERROR, "Data object not supported");
            break;
        }

        dataObjectNumber++;
        index += (tagSize + lengthSize + valueSize);
    }

    errorStatus[0] = (retValue == RESULT_SUCCESS) ? 0 : dataObjectNumber;
    PrepareResponseCode(retValue, &errorStatus[1]);

    retValue = AppendTlvDataObjectToResponseBuffer(
        GenericErrorRsp,
        sizeof(errorStatus),
        errorStatus,
        pbOutBuffer,
        cbOutBufferSize,
        cbReturnBufferSize);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retValue;
}

NTSTATUS
StorageCardManager::Transmit(
                              _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                              _In_ DWORD cbSize,
                              _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                              _In_ DWORD cbOutBufferSize,
                              _Out_ DWORD *cbReturnBufferSize,
                              _In_ USHORT usTimeout
                              )
{
    return NfcCxSCInterfaceTransmitRawData(m_pScInterface,
                                           (PBYTE)pbDataBuffer,
                                           cbSize,
                                           pbOutBuffer,
                                           cbOutBufferSize,
                                           cbReturnBufferSize,
                                           usTimeout);
}

ApduResult
StorageCardManager::ReadTlvDataObject(
                         _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                         _In_ DWORD cbSize,
                         _In_ DWORD BufferOffset,
                         _Outptr_result_buffer_(*pcbTagSize) const BYTE **ppbTag,
                         _Out_ DWORD *pcbTagSize,
                         _Out_ DWORD *pcbLengthSize,
                         _Outptr_result_buffer_(*pcbValueSize) const BYTE **ppbValue,
                         _Out_ DWORD *pcbValueSize
                         )
{
    ApduResult retValue = RESULT_SUCCESS;

    *ppbTag = *ppbValue = NULL;
    *pcbTagSize = *pcbLengthSize = *pcbValueSize = 0;

    if (BufferOffset >= cbSize) {
        retValue = RESULT_INVALID_PARAM;
        TRACE_LINE(LEVEL_ERROR, "Invalid length");
        goto Done;
    }

    *ppbTag = &pbDataBuffer[BufferOffset];
    *pcbTagSize = 1;

    if ((pbDataBuffer[BufferOffset++] & TAG_MULTI_BYTE_MASK) == TAG_MULTI_BYTE_MASK) {
        while (BufferOffset < cbSize)
        {
            (*pcbTagSize)++;

            if ((TAG_COMPREHENSION_MASK & pbDataBuffer[BufferOffset++]) == 0) {
                break;
            }
        }
    }

    if (BufferOffset >= cbSize) {
        retValue = RESULT_INVALID_PARAM; 
        TRACE_LINE(LEVEL_ERROR, "Invalid length");
        goto Done;
    }

    *pcbLengthSize = 1;

    //
    // Based on ETSI TS 101 220 V10.3.0, Section 7.1.2
    //
    if ((pbDataBuffer[BufferOffset] & TAG_LENGTH_MULTI_BYTE_MASK) == TAG_LENGTH_MULTI_BYTE_MASK) {
        *pcbLengthSize += (pbDataBuffer[BufferOffset] & ~TAG_LENGTH_MULTI_BYTE_MASK);

        if (*pcbLengthSize > sizeof(DWORD)) {
            retValue = RESULT_INVALID_PARAM;
            TRACE_LINE(LEVEL_ERROR, "TLV Length is too large");
            goto Done;
        }
    }

    for (UCHAR index = 0; index < *pcbLengthSize; index++) {
        if (BufferOffset >= cbSize) {
            retValue = RESULT_INVALID_PARAM;
            TRACE_LINE(LEVEL_ERROR, "Invalid length");
            goto Done;
        }

        (*pcbValueSize) <<= 8;
        (*pcbValueSize) |= pbDataBuffer[BufferOffset++];
    }

    if ((BufferOffset + *pcbValueSize) > cbSize) {
        retValue = RESULT_INVALID_PARAM;
        TRACE_LINE(LEVEL_ERROR, "Invalid length");
        goto Done;
    }

    *ppbValue = &pbDataBuffer[BufferOffset];

Done:
    return retValue;
}

ApduResult
StorageCardManager::AppendTlvDataObjectToResponseBuffer(
                            _In_ USHORT DataObjectTag,
                            _In_ BYTE cbDataObjectSize,
                            _In_reads_bytes_(cbDataObjectSize) const BYTE *pbDataObjectValue,
                            _Out_writes_bytes_to_(cbOutBufferSize, *pBufferOffset) BYTE *pbOutBuffer,
                            _In_ DWORD cbOutBufferSize,
                            _Inout_ DWORD *pBufferOffset
                            )
{
    ApduResult retValue = RESULT_SUCCESS;
    BYTE cbDataObjectTag = 1;
    DWORD bufferOffset = *pBufferOffset;

    if ((DataObjectTag & TAG_MULTI_BYTE_MASK) == TAG_MULTI_BYTE_MASK) {
        cbDataObjectTag = 2;
    }

    if ((cbDataObjectTag + sizeof(cbDataObjectSize) + cbDataObjectSize + *pBufferOffset) > cbOutBufferSize) {
        retValue = RESULT_WRONG_LENGTH;
        TRACE_LINE(LEVEL_ERROR, "Invalid length");
        goto Done;
    }

    if (cbDataObjectTag == 1) {
        pbOutBuffer[bufferOffset] = DataObjectTag & 0xFF;
    }
    else {
        *((PUSHORT)&pbOutBuffer[bufferOffset]) = DataObjectTag;
    }

    bufferOffset += cbDataObjectTag;

    pbOutBuffer[bufferOffset++] = cbDataObjectSize;
    RtlCopyMemory(&pbOutBuffer[bufferOffset], pbDataObjectValue, cbDataObjectSize);

    bufferOffset += cbDataObjectSize;
    *pBufferOffset = bufferOffset;

Done:
    return retValue;
}
