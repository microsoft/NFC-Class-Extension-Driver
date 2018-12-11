//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <IOHelpers\DriverHandleFactory.h>
#include <IOHelpers\IoOperation.h>
#include <Simulation\SimSequenceRunner.h>
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
};

void
SETests::EseClientConnectDisconnectNci1Test()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Start NFC Controller.
    LOG_COMMENT(L"# Start NFC Controller.");
    std::shared_ptr<IoOperation> ioStartHost = simConnector.StartHostAsync();

    // Verify NCI is initialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Initialize::WithEseSequence_Nci1);
    VERIFY_IS_TRUE(ioStartHost->Wait(/*timeout(ms)*/ 1'000));

    // Open eSE interface.
    std::shared_ptr<Async::AsyncTaskBase<UniqueHandle>> openEseTask = DriverHandleFactory::OpenSmartcardHandleAsync(simConnector.DeviceId().c_str(), SmartCardReaderKind::EmbeddedSE);

    // Verify eSE is enabled.
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Entry);
    SimSequenceRunner::Run(simConnector, SEInitializationSequences::WithEse::ClientConnectedSequence_Nci1);

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
        SimSequenceRunner::Run(simConnector, SEInitializationSequences::WithEse::GetAtrSequence_Nci1);
        break;

    case 2:
        // eSE handle opened without running the GetAtr sequence.
        break;
    }

    // Wait for eSE interface to finish opening.
    UniqueHandle eseInterface = std::move(openEseTask->Get());

    // Close eSE interface handle asynchronously.
    std::shared_ptr<Async::AsyncTaskBase<void>> closeEseTask = DriverHandleFactory::CloseHandleAsync(std::move(eseInterface));

    // Verify eSE is disabled.
    SimSequenceRunner::Run(simConnector, SEInitializationSequences::WithEse::ClientDisconnectedSequence_Nci1);
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Exit);

    // Wait for eSE to finish closing.
    closeEseTask->Get();

    // Stop NFC Controller.
    LOG_COMMENT(L"# Stop NFC Controller.");
    std::shared_ptr<IoOperation> ioStopHost = simConnector.StopHostAsync();

    // Verify NCI is uninitialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);
    VERIFY_IS_TRUE(ioStopHost->Wait(/*timeout(ms)*/ 1'000));
}
