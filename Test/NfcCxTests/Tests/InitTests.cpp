//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phNciNfc_Common.h>

#include <IOHelpers\DeviceQuery.h>
#include <SimulationSequences\InitSequences.h>
#include <IOHelpers\IoOperation.h>
#include <Simulation\NciControlPacket.h>
#include <Simulation\NciSimConnector.h>
#include <SimulationSequences\RfDiscoverySequences.h>
#include <Simulation\SimSequenceRunner.h>
#include "TestLogging.h"
#include <IOHelpers\UniqueHandle.h>
#include <Simulation\VerifyHelpers.h>

using namespace ABI::Windows::Devices::SmartCards;

class InitTests
{
    TEST_CLASS(InitTests);

    BEGIN_TEST_METHOD(InitAndDeinitTest)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InitAndDeinitWithSlowIoTest)
        TEST_METHOD_PROPERTY(L"Category", L"Reliability")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DiscoveryInitAndDeinitTest)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()
};

// Tests NFC controller initialization and deinitialization.
void
InitTests::InitAndDeinitTest()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    LOG_COMMENT(L"# Pull device into D0.");
    simConnector.AddD0PowerReference();

    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence);

    LOG_COMMENT(L"# Allow device to drop out of D0.");
    simConnector.RemoveD0PowerReference();

    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence);
}

// Ensures driver can properly handle when the NCI write I/O request takes a long time to complete.
void
InitTests::InitAndDeinitWithSlowIoTest()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Pull device into D0 so that NCI is initialized.
    LOG_COMMENT(L"# Pull device into D0.");
    simConnector.AddD0PowerReference();

    // Run through the first half of the initialization sequence, stopping just before the GetConfigCommand step.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence, 6);

    // Manually process the GetConfigCommand step.
    LOG_COMMENT(L"# Manually processing GetConfigCommand step.");
    NciSimCallbackView message = simConnector.ReceiveCallback();
    VerifyNciPacket(InitSequences::InitializeNoSEs::GetConfigCommand.NciPacketData, message);

    // Don't send the NCI write complete message, until after the NCI response timer will have expired.
    Sleep(PHNCINFC_NCI_CMD_RSP_TIMEOUT * 2);
    simConnector.SendNciWriteCompleted();

    // Process the remainder of the initialization sequence.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence + 7, std::size(InitSequences::InitializeNoSEs::Sequence) - 7);

    // Allow device to drop out of D0, so that NCI is deinitialized.
    LOG_COMMENT(L"# Allow device to drop out of D0.");
    simConnector.RemoveD0PowerReference();

    // Run through the deinitialization sequence.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence);
}

// Tests entering and exiting RF discovery mode.
void
InitTests::DiscoveryInitAndDeinitTest()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    LOG_COMMENT(L"# Pull device into D0.");
    simConnector.AddD0PowerReference();

    // Verify NCI is initialized.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence);

    // Try to find the smartcard (NFC) interface and open it.
    std::vector<std::wstring> nfcScInterfaces = DeviceQuery::GetSmartcardInterfacesOfType(simConnector.DeviceId().c_str(), SmartCardReaderKind_Nfc);
    VERIFY_ARE_NOT_EQUAL(static_cast<size_t>(0), nfcScInterfaces.size());

    const std::wstring& nfcScInterfaceId = nfcScInterfaces[0];

    UniqueHandle nfcScInterface(CreateFile(
        nfcScInterfaceId.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr));
    VERIFY_WIN32_BOOL_SUCCEEDED(INVALID_HANDLE_VALUE != nfcScInterface.Get());

    // Start an IOCTL_SMARTCARD_IS_PRESENT request, so that the driver considers the smartcard reader handle to be in use.
    // This should result in the radio being initialized.
    std::shared_ptr<IoOperation> ioIsPresent = IoOperation::DeviceIoControl(nfcScInterface.Get(), IOCTL_SMARTCARD_IS_PRESENT, nullptr, 0, 0);

    // No longer need the extra D0 power reference.
    simConnector.RemoveD0PowerReference();

    // Verify discovery mode is started.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence);

    // Cancel the IOCTL_SMARTCARD_IS_PRESENT I/O, so that the smartcard reader handle is no longer considered active.
    ioIsPresent->Cancel();

    // Verify discovery mode is stopped.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);

    // Verify NCI is uninitialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence);
}
