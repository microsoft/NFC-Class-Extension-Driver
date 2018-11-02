# NFC class extension (CX) test guide

The `NfcCxTests` test suite verifies the NFC CX behavior without requiring any NFC hardware.

## How to run

1. Build the Visual Studio solution for `Debug` and `x64`.

2. On the test system, [enable test signing](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option).

3. Copy the test client driver to the test system:

    - `x64\Debug\NfcCxTestDeviceDriver\`

4. On the test system, install the test client driver:

    - In explorer, right-click `NfcCxTestDeviceDriver.inf`.
    - Click `Install`.

5. Locate the TAEF directory in the WDK (or eWDK) and copy it to the test system.

    - `C:\Program Files (x86)\Windows Kits\10\Testing\Runtimes\TAEF\x64\`

6. On the test system, install the TE.Service. In an administrator command prompt, run:

    - `cd '<TAEF-Directory>'`
    - `.\Wex.Services.exe /install:Te.Service`
    - `sc.exe start Te.Service`

7. Copy the `NfcCxTests.dll` to the test system:

    - `x64\Debug\NfcCxTests.dll`

8. Copy the VC++ redistribution DLLs to the same directory as `NfcCxTests.dll`.

    - `C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Redist\MSVC\14.15.26706\onecore\debug_nonredist\x64\Microsoft.VC141.DebugCRT\`

9. Copy the UCRT debug DLL to the same directory as `NfcCxTests.dll`.

    - `C:\Program Files (x86)\Windows Kits\10\bin\x64\ucrt\ucrtbased.dll`

10. On the test system, run:

    - `<TAEF-Directory>\te.exe NfcCxTests.dll`
