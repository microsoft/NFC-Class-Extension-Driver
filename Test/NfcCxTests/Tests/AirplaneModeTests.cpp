//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#include "Precomp.h"

#include <SimulationSequences\InitSequences.h>
#include <IOHelpers\DriverHandleFactory.h>
#include <Simulation\NciSimConnector.h>
#include <IOHelpers\RadioManager.h>
#include <SimulationSequences\RfDiscoverySequences.h>
#include <Simulation\SimSequenceRunner.h>

#include "TestLogging.h"

class AirplaneModeTests
{
    TEST_CLASS(AirplaneModeTests);

    // Tests airplane mode switch and the nfc radio switch, without any SEs or card emulation.
    TEST_METHOD(RadioStateBasicTest);
};

// General guide on interaction between airplane mode and radio switch:
// 1. Toggling the nfc radio switch always changes the state of the radio. (This is not the case for cellular.)
// 2. When airplane mode is enabled, the current state of the radio is remembered.
// 3. When airplane mode is disabled, the previous state of the radio (before airplande mode was enabled) is restored
//    unless it would result in the radio being disabled.
void
AirplaneModeTests::RadioStateBasicTest()
{
    LOG_COMMENT(L"# Open connection to NCI Simulator Driver.");
    NciSimConnector simConnector;

    // Pull the device into D0, so that all the device interfaces are created.
    LOG_COMMENT(L"# Pull device into D0.");
    simConnector.AddD0PowerReference();

    // Verify NCI is initialized.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1);

    // Drop the D0 power reference.
    simConnector.RemoveD0PowerReference();

    // Verify NCI is uninitialized.
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);

    // Open radio manager
    RadioManager radioManager(simConnector.DeviceId().c_str());

    // Reset radio state.
    LOG_COMMENT(L"== Reset radio state ==");
    radioManager.SetAirplaneMode(false);
    radioManager.SetNfcRadioState(true);

    // Verify radio is enabled.
    VERIFY_ARE_EQUAL(true, radioManager.GetNfcRadioState());

    // Open a handle for proximity subscription.
    UniqueHandle nfpSubInterface = DriverHandleFactory::OpenSubscriptionHandle(simConnector.DeviceId().c_str(), L"NDEF");

    // Verify NCI is initialized and discovery mode is started.
    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify enabling airplane mode disables radio.
    LOG_COMMENT(L"== 1 ==");
    radioManager.SetAirplaneMode(true);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);

    // Verify disabling airplane mode re-enables the radio.
    LOG_COMMENT(L"== 2 ==");
    radioManager.SetAirplaneMode(false);
    VERIFY_ARE_EQUAL(true, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify disabling the radio actually disables the radio.
    LOG_COMMENT(L"== 3 ==");
    radioManager.SetNfcRadioState(false);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);

    // Verify enabling airplane mode doesn't enable the radio.
    LOG_COMMENT(L"== 4 ==");
    radioManager.SetAirplaneMode(true);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    // Verify disabling airplane mode doesn't enable the radio, as the radio was manually disabled by the user.
    LOG_COMMENT(L"== 5 ==");
    radioManager.SetAirplaneMode(false);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    // Verify enabling the radio actually re-enables the radio.
    LOG_COMMENT(L"== 6 ==");
    radioManager.SetNfcRadioState(true);
    VERIFY_ARE_EQUAL(true, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify enabling airplane mode disables radio.
    LOG_COMMENT(L"== 7 ==");
    radioManager.SetAirplaneMode(true);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);

    // Verify that the user can enable the radio while airplane mode is enabled.
    LOG_COMMENT(L"== 8 ==");
    radioManager.SetNfcRadioState(true);
    VERIFY_ARE_EQUAL(true, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify disabling the radio actually disables the radio (while airplane mode is enabled).
    LOG_COMMENT(L"== 9 ==");
    radioManager.SetNfcRadioState(false);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);

    // Verify disabling airplane mode re-enables radio (even though the NFC radio was manually enabled and then disabled).
    LOG_COMMENT(L"== 10 ==");
    radioManager.SetAirplaneMode(false);
    VERIFY_ARE_EQUAL(true, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify disabling the radio actually disables the radio.
    LOG_COMMENT(L"== 11 ==");
    radioManager.SetNfcRadioState(false);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);

    // Verify enabling airplane mode doesn't enable the radio.
    LOG_COMMENT(L"== 12 ==");
    radioManager.SetAirplaneMode(true);
    VERIFY_ARE_EQUAL(false, radioManager.GetNfcRadioState());

    // Verify enabling the radio enables the radio (even though airplane mode is enabled).
    LOG_COMMENT(L"== 13 ==");
    radioManager.SetNfcRadioState(true);
    VERIFY_ARE_EQUAL(true, radioManager.GetNfcRadioState());

    SimSequenceRunner::Run(simConnector, InitSequences::InitializeNoSEs::Sequence_Nci1);
    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStart::Sequence_Nci1);

    // Verify disabling airplane mode doesn't disable the radio (even though the radio was disabled when airplane mode was first enabled).
    LOG_COMMENT(L"== 14 ==");
    radioManager.SetAirplaneMode(false);
    VERIFY_ARE_EQUAL(true, radioManager.GetNfcRadioState());

    // Verify closing the proximity handle shuts-down the device.
    nfpSubInterface.Reset();

    SimSequenceRunner::Run(simConnector, RfDiscoverySequences::DiscoveryStop::Sequence);
    SimSequenceRunner::Run(simConnector, InitSequences::Uninitialize::Sequence_Nci1);
}
