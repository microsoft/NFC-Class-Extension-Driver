//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <IOHelpers\DriverHandleFactory.h>
#include <IOHelpers\IoOperation.h>
#include <IOHelpers\SmartcardIo.h>
#include <Simulation\ApduOverNciHciGenerator.h>
#include <Simulation\ApduSamples.h>
#include <Simulation\SimSequenceRunner.h>
#include <Simulation\VerifyHelpers.h>
#include <SimulationSequences\InitSequences.h>
#include <SimulationSequences\SEInitializationSequences.h>

#include "TestLogging.h"

using namespace ::winrt::Windows::Devices::SmartCards;

class SETests
{
    TEST_CLASS(SETests);

    BEGIN_TEST_METHOD(EseClientConnectDisconnectNci1Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EseClientConnectDisconnectNci2Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EseApduNci1Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EseApduNci2Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

private:
    void EseClientConnectDisconnectTest(bool isNci2);
    void EseApduTest(bool isNci2);

    UniqueHandle ConnectToEse(NciSimConnector& simConnector, bool isNci2);
    void DisconnectFromEse(NciSimConnector& simConnector, bool isNci2, UniqueHandle&& eseInterface);
    void TestApdu(
        NciSimConnector& simConnector,
        _In_ HANDLE eseInterface,
        _In_reads_bytes_(commandLength) const void* command,
        _In_ DWORD commandLength,
        _In_reads_bytes_(responseLength) const void* response,
        _In_ DWORD responseLength);
};

void
SETests::EseClientConnectDisconnectTest(bool isNci2)
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    UniqueHandle eseInterface = ConnectToEse(simConnector, isNci2);
    DisconnectFromEse(simConnector, isNci2, std::move(eseInterface));
}

void
SETests::EseClientConnectDisconnectNci1Test()
{
    EseClientConnectDisconnectTest(false);
}

void
SETests::EseClientConnectDisconnectNci2Test()
{
    EseClientConnectDisconnectTest(true);
}

void
SETests::EseApduTest(bool isNci2)
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Startup.
    UniqueHandle eseInterface = ConnectToEse(simConnector, isNci2);

    // Ensure eSE reports as present.
    std::shared_ptr<IoOperation> ioIsPresent = IoOperation::DeviceIoControl(eseInterface.Get(), IOCTL_SMARTCARD_IS_PRESENT, nullptr, 0, 0);
    VERIFY_IS_TRUE(ioIsPresent->Wait(/*timeout(ms)*/ 1'000));
    IoOperationResult isPresentResult = ioIsPresent->Get();
    VERIFY_WIN32_SUCCEEDED(isPresentResult.ErrorCode);

    // Get the smartcard state of the eSE.
    std::shared_ptr<IoOperation> getState = IoOperation::DeviceIoControl(eseInterface.Get(), IOCTL_SMARTCARD_GET_STATE, nullptr, 0, sizeof(DWORD));
    IoOperationResult getStateResult = getState->Get();
    VERIFY_WIN32_SUCCEEDED(getStateResult.ErrorCode);

    // eSE doesn't support smartcard protocol negotiation. So its state skips directly to 'SCARD_SPECIFIC'.
    VERIFY_ARE_EQUAL(DWORD(SCARD_SPECIFIC), *reinterpret_cast<DWORD*>(getStateResult.Output.data()));

    // Test basic APDU.
    LOG_COMMENT(L"# Basic APDU");
    TestApdu(simConnector, eseInterface.Get(), ApduSamples::GetDataCommand, ARRAYSIZE(ApduSamples::GetDataCommand), ApduSamples::GetDataResponse, ARRAYSIZE(ApduSamples::GetDataResponse));

    // Test extended APDU.
    LOG_COMMENT(L"# Extended APDU");

    std::vector<uint8_t> apduCommand = ApduSamples::GenerateExtendedApduCommand(/*payloadLength*/ 1'000);
    std::vector<uint8_t> apduResponse = ApduSamples::GenerateExtendedApduResponse(/*payloadLength*/ 1'000);

    TestApdu(simConnector, eseInterface.Get(), apduCommand.data(), DWORD(apduCommand.size()), apduResponse.data(), DWORD(apduResponse.size()));

    // Shutdown.
    DisconnectFromEse(simConnector, isNci2, std::move(eseInterface));
}

void
SETests::EseApduNci1Test()
{
    EseApduTest(false);
}

void
SETests::EseApduNci2Test()
{
    EseApduTest(true);
}

UniqueHandle
SETests::ConnectToEse(NciSimConnector& simConnector, bool isNci2)
{
    // Start NFC Controller.
    LOG_COMMENT(L"# Start NFC Controller.");
    std::shared_ptr<IoOperation> ioStartHost = simConnector.StartHostAsync();

    // Verify NCI is initialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Initialize::WithEseSequence(isNci2));
    VERIFY_IS_TRUE(ioStartHost->Wait(/*timeout(ms)*/ 1'000));

    // Open eSE interface.
    std::shared_ptr<Async::AsyncTaskBase<UniqueHandle>> openEseTask = DriverHandleFactory::OpenSmartcardHandleAsync(simConnector.DeviceId().c_str(), SmartCardReaderKind::EmbeddedSE);

    // Verify eSE is enabled.
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Entry);
    SimSequenceRunner::Run(simConnector, SEInitializationSequences::WithEse::ClientConnectedSequence(isNci2));

    // When the first client connects to the eSE interface, the driver retrieves the eSE's ATR.
    // Check if this is the first connection by waiting for either a new NCI packet or handle creation to complete.
    std::shared_ptr<Async::AsyncTaskBase<void>> waitForLibNfcCallback = simConnector.WhenLibNfcThreadCallbackAvailableAsync();

    size_t waitResult = Async::WaitForAnyWithTimeout(/*timeout(ms)*/ 5'000, waitForLibNfcCallback, openEseTask);
    switch (waitResult)
    {
    case 0:
        VERIFY_FAIL(L"Open eSE handle timed out");
        break;

    case 1:
        // Process the GetAtr sequence.
        SimSequenceRunner::Run(simConnector, SEInitializationSequences::WithEse::GetAtrSequence);
        break;

    case 2:
        // eSE handle opened without running the GetAtr sequence.
        break;
    }

    // Wait for eSE interface to finish opening.
    return openEseTask->Get();
}

void
SETests::DisconnectFromEse(NciSimConnector& simConnector, bool isNci2, UniqueHandle&& eseInterface)
{
    // Close eSE interface handle asynchronously.
    std::shared_ptr<Async::AsyncTaskBase<void>> closeEseTask = DriverHandleFactory::CloseHandleAsync(std::move(eseInterface));

    // Verify eSE is disabled.
    SimSequenceRunner::Run(simConnector, SEInitializationSequences::WithEse::ClientDisconnectedSequence(isNci2));
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Exit);

    // Wait for eSE to finish closing.
    closeEseTask->Get();

    // Stop NFC Controller.
    LOG_COMMENT(L"# Stop NFC Controller.");
    std::shared_ptr<IoOperation> ioStopHost = simConnector.StopHostAsync();

    // Verify NCI is uninitialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence(isNci2));
    VERIFY_IS_TRUE(ioStopHost->Wait(/*timeout(ms)*/ 1'000));
}

void
SETests::TestApdu(
    NciSimConnector& simConnector,
    _In_ HANDLE eseInterface,
    _In_reads_bytes_(commandLength) const void* command,
    _In_ DWORD commandLength,
    _In_reads_bytes_(responseLength) const void* response,
    _In_ DWORD responseLength)
{
    // Send an APDU I/O request.
    std::shared_ptr<Async::AsyncTaskBase<IoApduResult>> ioApdu = SmartcardIo::SendApdu(eseInterface, command, commandLength, responseLength);

    // Process APDU NCI packets.
    SimSequenceRunner::Run(simConnector, ApduOverNciHciGenerator::CreateCommandSequence(
        L"eSE command APDU",
        SEInitializationSequences::HciNetworkConnectionId,
        SEInitializationSequences::EseApduPipeId,
        command,
        commandLength));

    SimSequenceRunner::Run(simConnector, ApduOverNciHciGenerator::CreateResponseSequence(
        L"eSE response APDU (NCI)",
        SEInitializationSequences::HciNetworkConnectionId,
        SEInitializationSequences::EseApduPipeId,
        response,
        responseLength));

    // Wait for the APDU I/O request to complete.
    VERIFY_IS_TRUE(ioApdu->Wait(/*timeout(ms)*/ 1'000));

    // Verify APDU I/O request's results.
    IoApduResult ioApduResult = ioApdu->Get();
    VERIFY_WIN32_SUCCEEDED(ioApduResult.ErrorCode);
    VerifyArraysAreEqual(L"eSE response APDU (IOCTL)", response, responseLength, ioApduResult.ApduResponse.data(), ioApduResult.ApduResponse.size());
}
