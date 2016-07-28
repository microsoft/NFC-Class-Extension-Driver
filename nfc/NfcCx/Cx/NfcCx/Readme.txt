NFC Class Extension (NfcCx)

Prerequisites
- Windows PC
- Visual Studio 2015 (includes Windows 10 SDK)
- Windows 10 WDK


Building the NfcCx

In order to build the NfcCx solution, you will need to get WdfCxProxy.lib from Microsoft and place it for the appropriate platform in the following directory:

	$(SolutionDir)libs\$(Platform)

where $(Platform) can be "Win32", "x64", or "ARM".

Once you have built the solution, you will need to deploy NfcCx.dll to the device.


Deploying to desktop

Acquire permission to modify NfcCx.dll - one time setup
1. Open command prompt as administrator
2. Run: cd %SystemRoot%\System32\drivers\UMDF
3. Run: takeown /f NfcCx.dll
4. Run the following command, replacing <username> with the username obtained from the output of the previous command: icacls NfcCx.dll /grant <username>:M
You should now be able to copy+paste the NfcCx.dll you built into %SystemRoot%\System32\drivers\UMDF.

When copying NfcCx.dll to %SystemRoot%\System32\drivers\UMDF, you will need to make sure that no driver is using that dll. You can use Device Manager to disable any driver that is using NfcCx.dll. You can open Device Manager by pressing the Windows key + R and then entering "devmgmt.msc" into the Windows Run window. To disable a driver, right-click it, then click "Disable". Then you can copy+paste NfcCx.dll into %SystemRoot%\System32\drivers\UMDF. To re-enable a driver, right-click it in Device Manager, then click "Enable".


Deploying to phone

You will need a phone that is engineering- or retail-unlocked. To deploy NfcCx.dll to the phone, you will need to plug it into a PC via USB.

Acquire permission to modify NfcCx.dll - one time setup
1. Put the phone in mass storage mode. To do this, you must first put the phone into flashing mode by holding the volume up button while the phone is booting. Then run the following command: ffutool.exe -massStorage
	- FFUTool.exe is part of the WDK. After installing the WDK, you can find it here: %ProgramFiles(x86)%\Windows Kits\10\Tools\bin\i386\
2. You should see the phone enumerate as a storage drive in File Explorer. It will have a drive letter associated with it. Open up command prompt as administrator and navigate to this drive with the following command: cd /d <drive_letter>
	- For example: cd /d G:
3. Run: cd Windows\System32\Drivers\UMDF
4. Run: takeown /f NfcCx.dll
5. Run: icacls NfcCx.dll /grant Administrators:M
You should now be able to put a *signed* copy of NfcCx.dll into C:\Windows\System32\Drivers\UMDF on the phone while the phone is in mass storage mode.

Signing NfcCx.dll
1. Set up the signing environment by following the instructions here: https://msdn.microsoft.com/en-us/library/windows/hardware/dn756804%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396 
2. Sign NfcCx.dll by following the instructions here: https://msdn.microsoft.com/en-us/library/windows/hardware/dn789217%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396 
