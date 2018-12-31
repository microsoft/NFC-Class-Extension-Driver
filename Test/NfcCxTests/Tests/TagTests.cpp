//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <phNciNfc_Common.h>

#include <IOHelpers\DeviceQuery.h>
#include <SimulationSequences\InitSequences.h>
#include <IOHelpers\IoOperation.h>
#include <Simulation\NciSimConnector.h>
#include <IOHelpers\DriverHandleFactory.h>
#include <SimulationSequences\RfDiscoverySequences.h>
#include <Simulation\SimSequenceRunner.h>
#include <Simulation\TagPayloads.h>
#include <SimulationSequences\TagSequences.h>
#include "TestLogging.h"
#include <IOHelpers\UniqueHandle.h>
#include <Simulation\VerifyHelpers.h>

class TagTests
{
public:
    TEST_CLASS(TagTests);

    // Tests reading a NDEF NFC tag using a proximity subscription.
    BEGIN_TEST_METHOD(SimpleNdefSubscriptionNci1Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()
    BEGIN_TEST_METHOD(SimpleNdefSubscriptionNci2Test)
        TEST_METHOD_PROPERTY(L"Category", L"GoldenPath")
    END_TEST_METHOD()
    // Ensures NfcCxRf/NfcCxState will properly defer processing the tag arrivial event while an existing operation
    // is running. Closely mirrors the SimpleNdefSubscriptionTest test.
    BEGIN_TEST_METHOD(NdefSubscriptionWithEarlyTagArrivalTest)
        TEST_METHOD_PROPERTY(L"Category", L"Reliability")
    END_TEST_METHOD()
    // Ensures driver can properly handle when the NCI write I/O request takes a long time to complete.
    // Closely mirrors the SimpleNdefSubscriptionTest test.
    BEGIN_TEST_METHOD(SimpleNdefSubscriptionTestWithSlowIO)
        TEST_METHOD_PROPERTY(L"Category", L"Reliability")
    END_TEST_METHOD()

private:
    void SimpleNdefSubscriptionTest(bool isNci2);

    UniqueHandle OpenSubscription(NciSimConnector& simConnector, bool isNci2);
    void CloseSubscription(NciSimConnector& simConnector, bool isNci2, UniqueHandle&& nfpSubInterface);
    std::shared_ptr<IoOperation> StartGetSubscriptionMessage(HANDLE nfpSubInterface);
    void VerifySubscriptionMessage(const std::shared_ptr<IoOperation>& ioGetMessage);
};

void
TagTests::SimpleNdefSubscriptionTest(bool isNci2)
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Startup.
    UniqueHandle nfpSubInterface = OpenSubscription(simConnector, isNci2);

    // Start get next message request.
    std::shared_ptr<IoOperation> ioGetMessage = StartGetSubscriptionMessage(nfpSubInterface.Get());

    // Verify discovery mode is started.
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Entry);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence(isNci2));

    // Provide a tag for the subscription to read.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216Activated::Sequence);
    SimSequenceRunner::Run(simConnector, TagSequences::NdefSubscriptionNtag216::Sequence);

    // The driver has finished with the tag. So it will restart discovery to look for new tags.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence(isNci2));

    // Verify message.
    VerifySubscriptionMessage(ioGetMessage);

    // Shutdown.
    CloseSubscription(simConnector, isNci2, std::move(nfpSubInterface));
}

void
TagTests::SimpleNdefSubscriptionNci1Test()
{
    SimpleNdefSubscriptionTest(false);
}

void
TagTests::SimpleNdefSubscriptionNci2Test()
{
    SimpleNdefSubscriptionTest(true);
}

void
TagTests::NdefSubscriptionWithEarlyTagArrivalTest()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Startup.
    UniqueHandle nfpSubInterface = OpenSubscription(simConnector, false);

    // Start get next message request.
    std::shared_ptr<IoOperation> ioGetMessage = StartGetSubscriptionMessage(nfpSubInterface.Get());

    // Verify discovery mode is started. But don't complete the SequenceRfDiscStartComplete sequence handler yet.
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Entry);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1, 5);

    NciSimCallbackMessage simCallback = simConnector.ReceiveLibNfcThreadCallback();
    SimSequenceRunner::VerifyStep(RfDiscoverySequences::DiscoveryStart::DiscoverStartComplete, simCallback);

    // Activate an NFC tag in the reader, while the SequenceRfDiscStartComplete sequence handler is still running.
    // This will verify that NfcCx properly defers processing hardware events while another operation is running.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216Activated::Sequence);

    // Complete the SequenceRfDiscStartComplete sequence handler.
    simConnector.SendSequenceCompleted(STATUS_SUCCESS, 0);

    // Verify tag is read.
    SimSequenceRunner::Run(simConnector, TagSequences::NdefSubscriptionNtag216::Sequence);

    // The driver has finished with the tag. So it will restart discovery to look for new tags.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify message.
    VerifySubscriptionMessage(ioGetMessage);

    // Shutdown.
    CloseSubscription(simConnector, false, std::move(nfpSubInterface));
}

void
TagTests::SimpleNdefSubscriptionTestWithSlowIO()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Startup.
    UniqueHandle nfpSubInterface = OpenSubscription(simConnector, false);

    // Start get next message request.
    std::shared_ptr<IoOperation> ioGetMessage = StartGetSubscriptionMessage(nfpSubInterface.Get());

    // Verify discovery mode is started.
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Entry);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Provide a tag for the subscription to read.
    SimSequenceRunner::Run(simConnector, TagSequences::Ntag216Activated::Sequence);

    // Manually process the first read command.
    LOG_COMMENT(L"# Manually processing ReadPage2Command step.");
    NciSimCallbackMessage message = simConnector.ReceiveLibNfcThreadCallback();
    SimSequenceRunner::VerifyStep(TagSequences::NdefSubscriptionNtag216::ReadPage2Command, message);

    // Don't send the NCI write complete message, until after the NCI response timer will have expired.
    LOG_COMMENT(L"Waiting for timeout to trigger.");
    Sleep(PHNCINFC_NCI_TRANSCEIVE_TIMEOUT * 2);
    simConnector.SendNciWriteCompleted();

    // Process the remainder of the tag read sequence.
    SimSequenceRunner::Run(simConnector, TagSequences::NdefSubscriptionNtag216::Sequence + 1, std::size(TagSequences::NdefSubscriptionNtag216::Sequence) - 1);

    // The driver has finished with the tag. So it will restart discovery to look for new tags.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify message.
    VerifySubscriptionMessage(ioGetMessage);

    // Shutdown.
    CloseSubscription(simConnector, false, std::move(nfpSubInterface));
}

UniqueHandle
TagTests::OpenSubscription(NciSimConnector& simConnector, bool isNci2)
{
    // Start NFC Controller.
    LOG_COMMENT(L"# Start NFC Controller.");
    std::shared_ptr<IoOperation> ioStartHost = simConnector.StartHostAsync();

    // Verify NCI is initialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Initialize::NoSEsSequence(isNci2));
    VERIFY_IS_TRUE(ioStartHost->Wait(/*timeout(ms)*/ 1'000));

    // Open handle for NDEF subscription.
    UniqueHandle nfpSubInterface = DriverHandleFactory::OpenSubscriptionHandle(simConnector.DeviceId().c_str(), L"NDEF");

    return std::move(nfpSubInterface);
}

void
TagTests::CloseSubscription(NciSimConnector& simConnector, bool isNci2, UniqueHandle&& nfpSubInterface)
{
    // Close proximity subscription handle.
    // This should drop the last power reference, which should cause discovery to stop and NCI to be uninitialized.
    nfpSubInterface.Reset();

    // Verify discovery mode is stopped.
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Power::D0Exit);

    // Stop NFC Controller.
    LOG_COMMENT(L"# Stop NFC Controller.");
    std::shared_ptr<IoOperation> ioStopHost = simConnector.StopHostAsync();

    // Verify NCI is uninitialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence(isNci2));
    VERIFY_IS_TRUE(ioStopHost->Wait(/*timeout(ms)*/ 1'000));
}

std::shared_ptr<IoOperation>
TagTests::StartGetSubscriptionMessage(HANDLE nfpSubInterface)
{
    // Start an I/O request for the next message,
    constexpr DWORD messageBufferSize = 2048;
    std::shared_ptr<IoOperation> ioGetMessage = IoOperation::DeviceIoControl(nfpSubInterface, IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE, nullptr, 0, messageBufferSize);

    return std::move(ioGetMessage);
}

void
TagTests::VerifySubscriptionMessage(const std::shared_ptr<IoOperation>& ioGetMessage)
{
    // Ensure subscription receives the tag's message.
    VERIFY_IS_TRUE(ioGetMessage->Wait(/*wait (ms)*/ 1'000));

    IoOperationResult ioGetMessageResult = ioGetMessage->Get();
    VERIFY_WIN32_SUCCEEDED(ioGetMessageResult.ErrorCode);

    // Verify message is correct.
    VerifyProximitySubscribeMessage(ioGetMessageResult.Output.data(), ioGetMessageResult.BytesTransferred, TagPayloads::NdefBingUri, std::size(TagPayloads::NdefBingUri));
}
