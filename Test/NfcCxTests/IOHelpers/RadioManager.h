//
// Copyright (c) Microsoft Corporation.  All Rights Reserved
//

#pragma once

#include "UniqueHandle.h"

// Helper class for calling the NFC radio manager driver interface.
class RadioManager
{
public:
    RadioManager(_In_ PCWSTR deviceName);

    // Gets the current state of the NFC radio switch.
    bool GetNfcRadioState();

    // Sets the current state of the NFC radio switch.
    void SetNfcRadioState(bool enableNfcRadio);

    // Sets the current state of the airplane mode switch.
    void SetAirplaneMode(bool enableAirplaneMode);

private:
    void SetRadioState(bool isSystemUpdate, bool enableRadio);

    UniqueHandle _DeviceInterface;
};
