//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <IOHelpers\DriverHandleFactory.h>
#include <IOHelpers\IoOperation.h>
#include <Simulation\NciSimConnector.h>
#include <Simulation\SimSequenceRunner.h>
#include <Simulation\VerifyHelpers.h>
#include <SimulationSequences\InitSequences.h>
#include <SimulationSequences\RfDiscoverySequences.h>
#include <SimulationSequences\TagSequences.h>

#include "TestLogging.h"

using namespace ::winrt::Windows::Devices::SmartCards;

static constexpr DWORD AtrMaxLength = 32;

class SmartcardTagTests
{
public:
    TEST_CLASS(SmartcardTagTests);

    BEGIN_TEST_METHOD(ResetAndGetAtrNci1Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(ResetAndGetAtrNci2Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ResetAndTagDisappearsNci1Test)
        TEST_METHOD_PROPERTY(L"Category", L"Reliability")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(ResetAndTagDisappearsNci2Test)
        TEST_METHOD_PROPERTY(L"Category", L"Reliability")
    END_TEST_METHOD()

private:
    void ResetAndGetAtrTest(bool isNci2);
    void ResetAndTagDisappearsTest(bool isNci2);

    void StartNfcController(NciSimConnector& simConnector, bool isNci2);
    void StopNfcController(NciSimConnector& simConnector, bool isNci2);
    void PresentTag(NciSimConnector& simConnector, bool isNci2, HANDLE nfcScInterface);
    void AfterTagDisconnected(NciSimConnector& simConnector, bool isNci2, HANDLE nfcScInterface);
};

void
SmartcardTagTests::ResetAndGetAtrTest(bool isNci2)
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Start the NFC Controller.
    StartNfcController(simConnector, isNci2);

    // Try to find the smartcard (NFC) interface and open it.
    UniqueHandle nfcScInterface = DriverHandleFactory::OpenSmartcardHandle(simConnector.DeviceId().c_str(), SmartCardReaderKind::Nfc);

    // Present an NFC tag in the reader.
    PresentTag(simConnector, isNci2, nfcScInterface.Get());

    // Issue and smartcard reset and read the ATR.
    DWORD powerType = SCARD_COLD_RESET;
    std::shared_ptr<IoOperation> ioReset = IoOperation::DeviceIoControl(nfcScInterface.Get(), IOCTL_SMARTCARD_POWER, &powerType, sizeof(powerType), AtrMaxLength);

    // Tag will be deactivated and then reactivated.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216::ResetSequence);

    // Get the result of the reset operation.
    VERIFY_IS_TRUE(ioReset->Wait(1'000));
    IoOperationResult ioResetResult = ioReset->Get();
    VERIFY_WIN32_SUCCEEDED(ioResetResult.ErrorCode);

    // Verify the ATR is correct.
    VerifyArraysAreEqual(L"ATR", TagSequences::Ntag216::Atr, std::size(TagSequences::Ntag216::Atr), ioResetResult.Output.data(), ioResetResult.BytesTransferred);

    // Disconnect the tag.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216::PresenceCheckDisconnectedSequence);
    AfterTagDisconnected(simConnector, isNci2, nfcScInterface.Get());

    // Stop the NFC controller.
    StopNfcController(simConnector, isNci2);
}

void
SmartcardTagTests::ResetAndGetAtrNci1Test()
{
    ResetAndGetAtrTest(false);
}

void
SmartcardTagTests::ResetAndGetAtrNci2Test()
{
    ResetAndGetAtrTest(true);
}

void
SmartcardTagTests::ResetAndTagDisappearsTest(bool isNci2)
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Start the NFC Controller.
    StartNfcController(simConnector, isNci2);

    // Try to find the smartcard (NFC) interface and open it.
    UniqueHandle nfcScInterface = DriverHandleFactory::OpenSmartcardHandle(simConnector.DeviceId().c_str(), SmartCardReaderKind::Nfc);

    // Present an NFC tag in the reader.
    PresentTag(simConnector, isNci2, nfcScInterface.Get());

    // Issue a smartcard reset and read the ATR.
    DWORD powerType = SCARD_COLD_RESET;
    std::shared_ptr<IoOperation> ioReset = IoOperation::DeviceIoControl(nfcScInterface.Get(), IOCTL_SMARTCARD_POWER, &powerType, sizeof(powerType), 0);

    // Tag will be deactivated and reactivated. But fail the reactivation.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216::ResetFailedSequence);

    // Get the result of the reset operation.
    // Even though the tag reactivation failed, the NFC CX pretends that the reset was successful. But it will eventually
    // report the tag as absent once the presence check runs again.
    VERIFY_IS_TRUE(ioReset->Wait(1'000));
    IoOperationResult ioResetResult = ioReset->Get();
    VERIFY_WIN32_SUCCEEDED(ioResetResult.ErrorCode);

    // Tag has disconnected due to the reactivation failure.
    AfterTagDisconnected(simConnector, isNci2, nfcScInterface.Get());

    // Stop the NFC controller.
    StopNfcController(simConnector, isNci2);
}

void
SmartcardTagTests::ResetAndTagDisappearsNci1Test()
{
    ResetAndTagDisappearsTest(false);
}

void
SmartcardTagTests::ResetAndTagDisappearsNci2Test()
{
    ResetAndTagDisappearsTest(true);
}

// Starts the NFC Controller.
void
SmartcardTagTests::StartNfcController(NciSimConnector& simConnector, bool isNci2)
{
    // Start NFC Controller.
    LOG_COMMENT(L"# Start NFC Controller.");
    std::shared_ptr<IoOperation> ioStartHost = simConnector.StartHostAsync();

    // Verify NCI is initialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Initialize::NoSEsSequence(isNci2));
    VERIFY_IS_TRUE(ioStartHost->Wait(/*timeout(ms)*/ 1'000));
}

void
SmartcardTagTests::StopNfcController(NciSimConnector& simConnector, bool isNci2)
{
    // Stop NFC Controller.
    LOG_COMMENT(L"# Stop NFC Controller.");
    std::shared_ptr<IoOperation> ioStopHost = simConnector.StopHostAsync();

    // Verify NCI is uninitialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence(isNci2));
    VERIFY_IS_TRUE(ioStopHost->Wait(/*timeout(ms)*/ 1'000));
}

// Enables the NFC radio and presents an NFC tag in the reader.
void
SmartcardTagTests::PresentTag(NciSimConnector& simConnector, bool isNci2, HANDLE nfcScInterface)
{
    // Start an IOCTL_SMARTCARD_IS_PRESENT request, so that the driver considers the smartcard reader handle to be in use.
    // This should result in the radio being initialized.
    std::shared_ptr<IoOperation> ioIsPresent = IoOperation::DeviceIoControl(nfcScInterface, IOCTL_SMARTCARD_IS_PRESENT, nullptr, 0, 0);

    // Verify discovery mode is started.
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Entry);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence(isNci2));

    // Provide a tag in the RF field.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216::ActivatedSequence);
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216::ReadSequence);

    // Ensure smartcard is now present.
    VERIFY_IS_TRUE(ioIsPresent->Wait(1'000));
    IoOperationResult ioIsPresentResult = ioIsPresent->Get();
    VERIFY_WIN32_SUCCEEDED(ioIsPresentResult.ErrorCode);

    // Respond to the tag presence check.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216::PresenceCheckConnectedSequence);
}

void
SmartcardTagTests::AfterTagDisconnected(NciSimConnector& simConnector, bool isNci2, HANDLE nfcScInterface)
{
    // Driver will restart discovery to look for new tags.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence(isNci2));

    // Ensure smartcard reader is reported as empty.
    std::shared_ptr<IoOperation> ioIsAbsent = IoOperation::DeviceIoControl(nfcScInterface, IOCTL_SMARTCARD_IS_ABSENT, nullptr, 0, 0);

    VERIFY_IS_TRUE(ioIsAbsent->Wait(1'000));
    IoOperationResult ioIsAbsentResult = ioIsAbsent->Get();
    VERIFY_WIN32_SUCCEEDED(ioIsAbsentResult.ErrorCode);

    // As nothing is holding a power reference anymore, discovery mode will be stopped after a timeout.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Exit);
}
