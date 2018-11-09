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
#include <IOHelpers\DriverHandleFactory.h>
#include <IOHelpers\UniqueHandle.h>
#include <Simulation\VerifyHelpers.h>

using namespace ABI::Windows::Devices::SmartCards;

class InitTests
{
    TEST_CLASS(InitTests);

    BEGIN_TEST_METHOD(InitAndDeinitNci1Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InitAndDeinitNci2Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InitAndDeinitNci1WithSlowIoTest)
        TEST_METHOD_PROPERTY(L"Category", L"Reliability")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DiscoveryInitAndDeinitNci1Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DiscoveryInitAndDeinitNci2Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()

private:
    void InitAndDeinitTest(bool isNci2);
    void DiscoveryInitAndDeinitTest(bool isNci2);
};

void
InitTests::InitAndDeinitTest(bool isNci2)
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    LOG_COMMENT(L"# Pull device into D0.");
    simConnector.AddD0PowerReference();

    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence(isNci2));

    LOG_COMMENT(L"# Allow device to drop out of D0.");
    simConnector.RemoveD0PowerReference();

    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence(isNci2));
}

// Tests NFC controller initialization and deinitialization for NCI 1.1.
void
InitTests::InitAndDeinitNci1Test()
{
    InitAndDeinitTest(false);
}

// Tests NFC controller initialization and deinitialization for NCI 2.0
void
InitTests::InitAndDeinitNci2Test()
{
    InitAndDeinitTest(true);
}

// Ensures driver can properly handle when the NCI write I/O request takes a long time to complete.
void
InitTests::InitAndDeinitNci1WithSlowIoTest()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Pull device into D0 so that NCI is initialized.
    LOG_COMMENT(L"# Pull device into D0.");
    simConnector.AddD0PowerReference();

    // Run through the first half of the initialization sequence, stopping just before the GetConfigCommand step.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1, 6);

    // Manually process the GetConfigCommand step.
    LOG_COMMENT(L"# Manually processing GetConfigCommand step.");
    NciSimCallbackView message = simConnector.ReceiveCallback();
    VerifyNciPacket(InitSequences::InitializeNoSEs::GetConfigCommand.NciPacketData, message);

    // Don't send the NCI write complete message, until after the NCI response timer will have expired.
    Sleep(PHNCINFC_NCI_CMD_RSP_TIMEOUT * 2);
    simConnector.SendNciWriteCompleted();

    // Process the remainder of the initialization sequence.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1 + 7, std::size(InitSequences::InitializeNoSEs::Sequence_Nci1) - 7);

    // Allow device to drop out of D0, so that NCI is deinitialized.
    LOG_COMMENT(L"# Allow device to drop out of D0.");
    simConnector.RemoveD0PowerReference();

    // Run through the deinitialization sequence.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);
}

void
InitTests::DiscoveryInitAndDeinitTest(bool isNci2)
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    LOG_COMMENT(L"# Pull device into D0.");
    simConnector.AddD0PowerReference();

    // Verify NCI is initialized.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence(isNci2));

    // Try to find the smartcard (NFC) interface and open it.
    UniqueHandle nfcScInterface = DriverHandleFactory::OpenSmartcardHandle(simConnector.DeviceId().c_str(), SmartCardReaderKind_Nfc);

    // Start an IOCTL_SMARTCARD_IS_PRESENT request, so that the driver considers the smartcard reader handle to be in use.
    // This should result in the radio being initialized.
    std::shared_ptr<IoOperation> ioIsPresent = IoOperation::DeviceIoControl(nfcScInterface.Get(), IOCTL_SMARTCARD_IS_PRESENT, nullptr, 0, 0);

    // No longer need the extra D0 power reference.
    simConnector.RemoveD0PowerReference();

    // Verify discovery mode is started.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence(isNci2));

    // Cancel the IOCTL_SMARTCARD_IS_PRESENT I/O, so that the smartcard reader handle is no longer considered active.
    ioIsPresent->Cancel();

    // Verify discovery mode is stopped.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);

    // Verify NCI is uninitialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence(isNci2));
}

// Tests entering and exiting RF discovery mode for NCI 1.1.
void
InitTests::DiscoveryInitAndDeinitNci1Test()
{
    DiscoveryInitAndDeinitTest(false);
}

// Tests entering and exiting RF discovery mode for NCI 2.0.
void
InitTests::DiscoveryInitAndDeinitNci2Test()
{
    DiscoveryInitAndDeinitTest(true);
}
