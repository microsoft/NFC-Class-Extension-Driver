/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    StorageClassMifareStd.h

Abstract:

    Storage card Mifare Classic implementation

Environment:

    User mode

--*/
#include "Pch.h"

#include "StorageClassMifareStd.tmh"

StorageClassMifareStd::StorageClassMifareStd(IStorageCard *pStorageCard)
    : StorageClassMiFareUL(pStorageCard)
{
    m_IsAuthSupported   = TRUE;
    m_IsLesserLeLc      = FALSE;
}

ApduResult StorageClassMifareStd::GetDataCommand(
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

    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    if (cmdApdu->P1 == 0x00 && cmdApdu->P2 == 0x00) { // Get UID bytes
        BYTE uidLength = 0;
        BYTE *uid = NULL;

        GetUniqueID(&uid, &uidLength);

        if (cbOutBufferSize < uidLength) {
            retValue = RESULT_INVALID_PARAM;
            goto Done;
        }

        if(0 != cmdApdu->Lc) {
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

        if(RESULT_SUCCESS == retValue) {
            RtlCopyMemory(pbOutBuffer, uid, uidLength);
            *cbReturnBufferSize = uidLength;
        }
    }
    else if(cmdApdu->P1 == 0x01 && cmdApdu->P2 == 0x00) { //Get Historical bytes

        if (cbOutBufferSize < m_HistoricalByteLength) {
            retValue = RESULT_INVALID_PARAM;
            goto Done;
        }

        RtlCopyMemory(pbOutBuffer, m_HistoricalBytes, m_HistoricalByteLength);
        *cbReturnBufferSize = m_HistoricalByteLength;
    }
    else {
        //Wrong P1 and P2 value
        retValue = RESULT_WRONG_P1P2;
    }

Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMifareStd::ReadBinary(
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

    // Since there is no Lc and the command structure we have Lc before Le
    // we are using Lc instead of Le

    TRACE_LINE(LEVEL_INFO, "P1 0x%x, P2 0x%x, Le 0x%x",cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);

    retValue = ValidateReadBinaryParamters(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if (RESULT_SUCCESS != retValue) {
        goto Done;
    }

    RtlZeroMemory(m_CommandBuffer, sizeof(m_CommandBuffer));
    m_CmdBufferSize = 0;

    retValue = PrepareTransceiveForRead(cmdApdu->P1, cmdApdu->P2, cmdApdu->Lc);
    if ((RESULT_SUCCESS == retValue) || (RESULT_LE_LESSER == retValue)) {
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
StorageClassMifareStd::ValidateReadBinaryParamters(
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
StorageClassMifareStd::PrepareTransceiveForRead(
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

    if (p1 == 0x00) {
        m_CommandBuffer[0] = phNfc_eMifareRead16;
        m_CommandBuffer[1] = p2;
        m_CmdBufferSize = 2 ;

        if(MIFARE_CLASSIC_BLOCK_SIZE == le || 0x00 == le) {
            retValue = RESULT_SUCCESS;
        }
        else {
            if(MIFARE_CLASSIC_BLOCK_SIZE > le) {
                retValue = RESULT_SUCCESS;
                m_IsLesserLeLc = TRUE;
            }
            else if(MIFARE_CLASSIC_BLOCK_SIZE < le) {
                retValue = RESULT_LE_GREATER;
            }
        }
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMifareStd::UpdateBinary(
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
StorageClassMifareStd::ValidateUpdateBinaryParameters(
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
    if (0x00 == p1 && p2 != 0x00) {
        retValue = RESULT_SUCCESS;
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);
    return retValue;
}

ApduResult
StorageClassMifareStd::PrepareTransceiveForUpdate(
                                                 _In_ PPcscCommandApduInfo pPcscCmdApdu
                                                 )
{
    DWORD size = 0;
    ApduResult retValue = RESULT_INVALID_PARAM;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    TRACE_LINE(LEVEL_INFO, " (LC 0x%02x) (P2 0x%02x)", pPcscCmdApdu->Lc, pPcscCmdApdu->P2);

    if (sizeof(m_CommandBuffer) < (pPcscCmdApdu->Lc + 2)) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    if (pPcscCmdApdu->Lc == MIFARE_CLASSIC_BLOCK_SIZE) {
        retValue = RESULT_SUCCESS;
    }
    else {
        if (MIFARE_CLASSIC_BLOCK_SIZE > pPcscCmdApdu->Lc) {
            retValue = RESULT_LE_LESSER;
        }
        if (MIFARE_CLASSIC_BLOCK_SIZE < pPcscCmdApdu->Lc) {
            retValue = RESULT_LE_GREATER;
        }
    }

    m_CommandBuffer[0] = phNfc_eMifareWrite16;
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
StorageClassMifareStd::ValidateGeneralAuthenticateParameters(
                                                              _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                                              _In_ DWORD cbSize
                                                              )
{
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    if((0x00 != cmdApdu->P1) ||(0x00 != cmdApdu->P2)) {
        retValue = RESULT_WRONG_P1P2; //6B00
    }
    else if(0x05 != cmdApdu->Lc) {
        retValue = RESULT_WRONG_LC; //6300
    }

Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, (DWORD)retValue);

    return retValue;
}

ApduResult
StorageClassMifareStd::PrepareTransceiveForGeneralAuthenticate(
                                                                _In_ void* getLoadKey,
                                                                _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                                                _In_ DWORD cbSize,
                                                                _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                                                _In_ DWORD cbOutBufferSize,
                                                                _Out_ DWORD *cbReturnBufferSize
                                                                )
{
    DWORD size = 0;
    ApduResult retValue = RESULT_SUCCESS;
    PPcscCommandApduInfo cmdApdu = (PPcscCommandApduInfo)pbDataBuffer;
    PPcscAuthentication authData = NULL;
    BYTE keyData[7];
    BYTE uidLength = 0;
    BYTE *uid = NULL;
    BYTE parsedAuthCmd[15] = {0};
    int keyIndex = 0;
    static const DWORD AUTHENTICATION_VERSION = 0x01;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    if ((sizeof(PcscAuthentication) + MIN_APDU_HEADER) < cbSize) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    authData = (PPcscAuthentication)cmdApdu->DataIn;

    if ((phNfc_eMifareAuthentA != authData->AuthKeyType) && (phNfc_eMifareAuthentB != authData->AuthKeyType)) {
        retValue = RESULT_WRONG_KEY_TYPE; //6986
        goto Done;
    }

    if (AUTHENTICATION_VERSION != authData->AuthVersion) {
        retValue = RESULT_WRONG_AUTH_VERSION; //6983
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "Now ExtractKeyIndex \n");
    keyIndex = ((LoadKey*)getLoadKey)->ExtractKeyIndex(authData->AuthKeyNR);

    if ((DWORD)-1 == keyIndex) {
        retValue = RESULT_WRONG_KEY_NR; //6988
    }
    else {

        TRACE_LINE(LEVEL_INFO, "Preparing Auth Data\n");
        RtlZeroMemory(keyData, sizeof(keyData));

        ((LoadKey*)getLoadKey)->ExtractKeys(keyIndex, keyData, KEYSIZE);

        parsedAuthCmd[size++] = authData->AuthKeyType;
        parsedAuthCmd[size++] = (authData->AuthMSB << 4)|authData->AuthLSB;

        GetUniqueID(&uid, &uidLength);

        // 7 byte UID requires a 3 byte offset
        RtlCopyMemory(parsedAuthCmd + size, (uidLength == 7) ? uid + 3 : uid, MIFARE_CLASSIC_AUTH_UIDSIZE);
        size += MIFARE_CLASSIC_AUTH_UIDSIZE;

        RtlCopyMemory(parsedAuthCmd + size, keyData, KEYSIZE);
        size += KEYSIZE;

        if (size <= cbOutBufferSize) {
            RtlCopyMemory(pbOutBuffer, parsedAuthCmd, size);
            *cbReturnBufferSize = size;
        }
    }

Done:
    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retValue);
    return retValue;
}

ApduResult
StorageClassMifareStd::GetGeneralAuthenticateCommand(
                                                    _In_ void* getLoadKey,
                                                    _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                                    _In_ DWORD cbSize,
                                                    _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                                    _In_ DWORD cbOutBufferSize,
                                                    _Out_ DWORD *cbReturnBufferSize
                                                    )
{
    ApduResult retValue = RESULT_SUCCESS;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    if (!m_IsAuthSupported) {
        retValue = RESULT_NOT_SUPPORTED; //6983
    }
    else {
        retValue = ValidateGeneralAuthenticateParameters(pbDataBuffer, cbSize);
        if(RESULT_SUCCESS == retValue) {
            retValue = PrepareTransceiveForGeneralAuthenticate(
                (LoadKey*)getLoadKey,
                pbDataBuffer,
                cbSize,
                pbOutBuffer,
                cbOutBufferSize,
                cbReturnBufferSize);
        }
    }

    TRACE_FUNCTION_EXIT_DWORD(LEVEL_VERBOSE, retValue);
    return retValue;
}

void
StorageClassMifareStd::UpdateResponseCodeToResponseBuffer(
                                                          _In_ ApduResult /*RetError*/,
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
        if(m_IsLesserLeLc) {
            m_IsLesserLeLc = FALSE;
            Sw1Sw2[0] = 0x6C;
            Sw1Sw2[1] = MIFARE_CLASSIC_BLOCK_SIZE;
        }
    }
    else if (PcscWriteCmd == Command) {
        RtlZeroMemory(pbOutBuffer, cbSize);
        cbSize = 0;
    }
    else if (PcscGetDataCmd == Command) {
        if(m_IsLesserLeLc) {
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
StorageClassMifareStd::HandleIncDecCommand(
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

    if (cbSize < MIN_APDU_HEADER) {
        retValue = RESULT_INVALID_PARAM;
        goto Done;
    }

    TRACE_LINE(LEVEL_INFO, "cmdApdu->Lc = %02x\n", cmdApdu->Lc);

    retValue = ParseIncDecDataAndDoTransceive(
        cmdApdu->DataIn,
        cmdApdu->Lc,
        pbOutBuffer,
        cbOutBufferSize,
        cbReturnBufferSize);

Done:
    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);
    return retValue;
}

ApduResult
StorageClassMifareStd::ParseIncDecDataAndDoTransceive(
                                                      _In_reads_bytes_(cbSize) const BYTE *pbDataBuffer,
                                                      _In_ DWORD cbSize,
                                                      _Out_writes_bytes_to_(cbOutBufferSize, *cbReturnBufferSize) BYTE *pbOutBuffer,
                                                      _In_ DWORD cbOutBufferSize,
                                                      _Out_ DWORD *cbReturnBufferSize
                                                      )
{
    static const DWORD c_blockAddressLength = 1;
    static const DWORD c_operandLength = 4;
    ApduResult retValue = RESULT_SUCCESS;
    BYTE outData[6] = {};
    BYTE transferData[2] = {};
    DWORD index = 0;

    TRACE_FUNCTION_ENTRY(LEVEL_VERBOSE);

    *cbReturnBufferSize = 0;

    while (index < cbSize && retValue == RESULT_SUCCESS) {
        BYTE incDecCmd = 0;
        DWORD cmdLength = 0;
        DWORD loopIndex = 0;
        const BYTE* incDecBuffer = nullptr;

        // To break the loop
        retValue = RESULT_COMMAND_INCOMPATIBLE;

        if (pbDataBuffer[index] == DecrementCmd) {
            incDecCmd = phNfc_eMifareDec;
        }
        else if (pbDataBuffer[index] == IncrementCmd) {
            incDecCmd = phNfc_eMifareInc;
        }
        else {
            TRACE_LINE(LEVEL_INFO, "Invalid increment/decrement command: %u", pbDataBuffer[index]);
            retValue = RESULT_COMMAND_INCOMPATIBLE;
            break;
        }

        if (index + 1 >= cbSize) {
            TRACE_LINE(LEVEL_INFO, "Failed to get increment/decrement command length");
            retValue = RESULT_COMMAND_INCOMPATIBLE;
            break;
        }

        cmdLength = pbDataBuffer[++index];
        if (cmdLength == 0 || index + cmdLength >= cbSize) {
            TRACE_LINE(LEVEL_INFO, "Invalid increment/decrement command length: %lu", cmdLength);
            retValue = RESULT_COMMAND_INCOMPATIBLE;
            break;
        }

        incDecBuffer = &pbDataBuffer[++index];
        while (loopIndex < cmdLength) {
            bool restore = false;
            BYTE incDecDest = 0;
            BYTE restoreDest = 0;

            // Destination Block
            if (loopIndex + 1 + c_blockAddressLength >= cmdLength ||
                incDecBuffer[loopIndex] != BlkAddressCmd ||
                incDecBuffer[loopIndex + 1] != c_blockAddressLength) {
                TRACE_LINE(LEVEL_INFO, "Invalid block address");
                retValue = RESULT_INVALID_BLOCK_ADDRESS;
                break;
            }

            incDecDest = incDecBuffer[loopIndex + 2];
            loopIndex += 2 + c_blockAddressLength;

            if (loopIndex + 1 + c_blockAddressLength < cmdLength &&
                incDecBuffer[loopIndex] == BlkAddressCmd) {
                if (incDecBuffer[loopIndex + 1] != c_blockAddressLength) {
                    TRACE_LINE(LEVEL_INFO, "Invalid restore block address");
                    retValue = RESULT_INVALID_BLOCK_ADDRESS;
                    break;
                }

                restore = true;
                restoreDest = incDecBuffer[loopIndex + 2];
                loopIndex += 2 + c_blockAddressLength;
            }

            if (loopIndex + 1 + c_operandLength >= cmdLength ||
                incDecBuffer[loopIndex] != BlkValueCmd ||
                incDecBuffer[loopIndex + 1] != c_operandLength) {
                TRACE_LINE(LEVEL_INFO, "Invalid increment/decrement value");
                retValue = RESULT_COMMAND_NOT_ALLOWED;
                break;
            }

            outData[0] = incDecCmd;
            outData[1] = incDecDest;
            RtlCopyMemory(&outData[2], &incDecBuffer[loopIndex + 2], c_operandLength);
            loopIndex += 2 + c_operandLength;
            TRACE_LINE(LEVEL_INFO, "Increment/decrement (command = 0x%02X) value at block 0x%02X", incDecCmd, incDecDest);
            retValue = MifareTransceive(outData,
                                        sizeof(outData),
                                        pbOutBuffer,
                                        cbOutBufferSize,
                                        cbReturnBufferSize);

            if (retValue != RESULT_SUCCESS) {
                TRACE_LINE(LEVEL_INFO, "Increment/decrement command failed: %d", retValue);
                retValue = RESULT_MEMORY_FAILURE;
                break;
            }

            // Transfer Command - store the value in destination block
            transferData[0] = phNfc_eMifareTransfer;
            transferData[1] = incDecDest;
            TRACE_LINE(LEVEL_INFO, "Transfer incremented/decremented value to block 0x%02X", incDecDest);
            retValue = MifareTransceive(transferData,
                                        sizeof(transferData),
                                        pbOutBuffer,
                                        cbOutBufferSize,
                                        cbReturnBufferSize);

            if (retValue != RESULT_SUCCESS) {
                TRACE_LINE(LEVEL_INFO, "Transfer command for increment/decrement failed: %d", retValue);
                retValue = RESULT_SECURITY_STATUS_NOT_SATISFIED;
                break;
            }

            if (restore) {
                // Restore the block that we just transferred the result of the increment/decrement
                // command to
                ZeroMemory(outData, sizeof(outData));
                outData[0] = phNfc_eMifareRestore;
                outData[1] = incDecDest;
                // Last 4 bytes of outData are zeroed out for the operand
                TRACE_LINE(LEVEL_INFO, "Restore value at block 0x%02X", incDecDest);
                retValue = MifareTransceive(outData,
                                            sizeof(outData),
                                            pbOutBuffer,
                                            cbOutBufferSize,
                                            cbReturnBufferSize);

                if (retValue != RESULT_SUCCESS) {
                    TRACE_LINE(LEVEL_INFO, "Restore command failed: %d", retValue);
                    retValue = RESULT_MEMORY_FAILURE;
                    break;
                }

                // Transfer Command - store the value in destination block
                transferData[0] = phNfc_eMifareTransfer;
                transferData[1] = restoreDest;
                TRACE_LINE(LEVEL_INFO, "Transfer restored value to block 0x%02X", restoreDest);
                retValue = MifareTransceive(transferData,
                                            sizeof(transferData),
                                            pbOutBuffer,
                                            cbOutBufferSize,
                                            cbReturnBufferSize);

                if (retValue != RESULT_SUCCESS) {
                    TRACE_LINE(LEVEL_INFO, "Transfer command for restore failed: %d", retValue);
                    retValue = RESULT_SECURITY_STATUS_NOT_SATISFIED;
                    break;
                }
            }
        }

        index += cmdLength;
    }

    TRACE_FUNCTION_EXIT(LEVEL_VERBOSE);

    return retValue;
}

