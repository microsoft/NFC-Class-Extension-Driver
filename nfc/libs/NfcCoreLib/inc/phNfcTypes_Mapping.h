/*
*          Modifications Copyright (c) Microsoft. All rights reserved.
*
*              Original code Copyright (c), NXP Semiconductors
*/

#pragma once

typedef phNfc_sData_t phHal_sData_t; /**< \copybrief phNfc_sData_t \sa phNfc_sData_t */
typedef phNfc_sSupProtocol_t phHal_sSupProtocol_t; /**< \copybrief phNfc_sSupProtocol_t \sa phNfc_sSupProtocol_t */
typedef phNfc_sDeviceCapabilities_t phHal_sDeviceCapabilities_t; /**< \copybrief phNfc_sDeviceCapabilities_t \sa phNfc_sDeviceCapabilities_t */
typedef phNfc_sHwReference_t phHal_sHwReference_t; /**< \copybrief phNfc_sHwReference_t \sa phNfc_sHwReference_t */
typedef phNfc_sDepFlags_t phHal_sDepFlags_t; /**< \copybrief phNfc_sDepFlags_t \sa phNfc_sDepFlags_t */
typedef phNfc_sDepAdditionalInfo_t phHal_sDepAdditionalInfo_t; /**< \copybrief phNfc_sDepAdditionalInfo_t \sa phNfc_sDepAdditionalInfo_t */

#define phHal_eMifareRaw phNfc_eMifareRaw
#define phHal_eMifareAuthentA phNfc_eMifareAuthentA
#define phHal_eMifareAuthentB phNfc_eMifareAuthentB
#define phHal_eMifareAuthKeyA phNfc_eMifareAuthKeyNumA
#define phHal_eMifareAuthKeyB phNfc_eMifareAuthKeyNumB
#define phHal_eMifareRead16 phNfc_eMifareRead16
#define phHal_eMifareRead phNfc_eMifareRead
#define phHal_eMifareWrite16 phNfc_eMifareWrite16
#define phHal_eMifareWrite4 phNfc_eMifareWrite4
#define phHal_eMifareInc phNfc_eMifareInc
#define phHal_eMifareDec phNfc_eMifareDec
#define phHal_eMifareTransfer phNfc_eMifareTransfer
#define phHal_eMifareRestore phNfc_eMifareRestore
#define phHal_eMifareReadSector phNfc_eMifareReadSector
#define phHal_eMifareWriteSector phNfc_eMifareWriteSector
#define phHal_eMifareReadN phNfc_eMifareReadN
#define phHal_eMifareWriteN phNfc_eMifareWriteN
#define phHal_eMifareSectorSel phNfc_eMifareSectorSel
#define phHal_eMifareAuth phNfc_eMifareAuth
#define phHal_eMifareProxCheck phNfc_eMifareProxCheck
#define phHal_eMifareInvalidCmd phNfc_eMifareInvalidCmd

typedef phNfc_eMifareCmdList_t phHal_eMifareCmdList_t; /**< \copybrief phNfc_eMifareCmdList_t \sa phNfc_eMifareCmdList_t */

#define phHal_eIso14443_4_Raw phNfc_eIso14443_4_Raw

typedef phNfc_eIso14443_4_CmdList_t phHal_eIso14443_4_CmdList_t; /**< \copybrief phNfc_eIso14443_4_CmdList_t \sa phNfc_eIso14443_4_CmdList_t */

#define phHal_eIso15693_Raw phNfc_eIso15693_Raw
#define phHal_eIso15693_Cmd phNfc_eIso15693_Cmd
#define phHal_eIso15693_Invalid phNfc_eIso15693_Invalid

typedef phNfc_eIso15693_CmdList_t phHal_eIso15693_CmdList_t; /**< \copybrief phNfc_eIso15693_CmdList_t \sa phNfc_eIso15693_CmdList_t */
#define phHal_eFelica_Raw phNfc_eFelica_Raw
#define phHal_eFelica_Check phNfc_eFelica_Check
#define phHal_eFelica_Update phNfc_eFelica_Update
#define phHal_eFelica_Invalid phNfc_eFelica_Invalid

typedef phNfc_eFelicaCmdList_t phHal_eFelicaCmdList_t; /**< \copybrief phNfc_eFelicaCmdList_t \sa phNfc_eFelicaCmdList_t */
#define phHal_eJewel_Raw phNfc_eJewel_Raw
#define phHal_eJewel_WriteN phNfc_eJewel_WriteN
#define phHal_eJewel_Invalid phNfc_eJewel_Invalid

typedef phNfc_eJewelCmdList_t phHal_eJewelCmdList_t; /**< \copybrief phNfc_eJewelCmdList_t \sa phNfc_eJewelCmdList_t */
#define phHal_eHeidRead phNfc_eHeidRead

typedef phNfc_eHid_CmdList_t phHal_eHid_CmdList_t; /**< \copybrief phNfc_eHid_CmdList_t \sa phNfc_eHid_CmdList_t */
#define phHal_eEpcGenRead phNfc_eEpcGen_Raw

typedef phNfc_eEpcGenCmdList_t phHal_eEpcGenCmdList_t; /**< \copybrief phNfc_eEpcGenCmdList_t \sa phNfc_eEpcGenCmdList_t */
typedef phNfc_sIso14443AInfo_t phHal_sIso14443AInfo_t; /**< \copybrief phNfc_sIso14443AInfo_t \sa phNfc_sIso14443AInfo_t */
typedef phNfc_sIso14443BInfo_t phHal_sIso14443BInfo_t; /**< \copybrief phNfc_sIso14443BInfo_t \sa phNfc_sIso14443BInfo_t */
typedef phNfc_sIso14443BPrimeInfo_t phHal_sIso14443BPrimeInfo_t; /**< \copybrief phNfc_sIso14443BPrimeInfo_t \sa phNfc_sIso14443BPrimeInfo_t */
typedef phNfc_sJewelInfo_t phHal_sJewelInfo_t; /**< \copybrief phNfc_sJewelInfo_t \sa phNfc_sJewelInfo_t */
typedef phNfc_sFelicaInfo_t phHal_sFelicaInfo_t; /**< \copybrief phNfc_sFelicaInfo_t \sa phNfc_sFelicaInfo_t */
typedef phNfc_sIso15693Info_t phHal_sIso15693Info_t; /**< \copybrief phNfc_sIso15693Info_t \sa phNfc_sIso15693Info_t */
typedef phNfc_sEpcGenInfo_t phHal_sEpcGenInfo_t; /**< \copybrief phNfc_sEpcGenInfo_t \sa phNfc_sEpcGenInfo_t */
typedef phNfc_sKovioInfo_t phHal_eKovio_PICC; /**< \copybrief phNfc_sKovioInfo_t \sa phNfc_sKovioInfo_t */

#define phHal_eDataRate_106 phNfc_eDataRate_106
#define phHal_eDataRate_212 phNfc_eDataRate_212
#define phHal_eDataRate_424 phNfc_eDataRate_424
#define phHal_eDataRate_848 phNfc_eDataRate_848
#define phHal_eDataRate_1696 phNfc_eDataRate_1696
#define phHal_eDataRate_3392 phNfc_eDataRate_3392
#define phHal_eDataRate_6784 phNfc_eDataRate_6784
#define phHal_eDataRate_RFU phNfc_eDataRate_RFU

typedef phNfc_eDataRate_t phHal_eDataRate_t; /**< \copybrief phNfc_eDataRate_t \sa phNfc_eDataRate_t */

typedef phNfc_sNfcIPInfo_t phHal_sNfcIPInfo_t; /**< \copybrief phNfc_sNfcIPInfo_t \sa phNfc_sNfcIPInfo_t */
typedef phNfc_uRemoteDevInfo_t phHal_uRemoteDevInfo_t; /**< \copybrief phNfc_uRemoteDevInfo_t \sa phNfc_uRemoteDevInfo_t */

#define phHal_eUnknown_DevType phNfc_eUnknown_DevType
#define phHal_eISO14443_A_PCD phNfc_eISO14443_A_PCD
#define phHal_eISO14443_B_PCD phNfc_eISO14443_B_PCD
#define phHal_eISO14443_BPrime_PCD phNfc_eISO14443_BPrime_PCD
#define phHal_eFelica_PCD phNfc_eFelica_PCD
#define phHal_eJewel_PCD phNfc_eJewel_PCD
#define phHal_eISO15693_PCD phNfc_eISO15693_PCD
#define phHal_ePCD_DevType phNfc_ePCD_DevType
#define phHal_ePICC_DevType phNfc_ePICC_DevType
#define phHal_eISO14443_A_PICC phNfc_eISO14443_A_PICC
#define phHal_eISO14443_4A_PICC phNfc_eISO14443_4A_PICC
#define phHal_eISO14443_3A_PICC phNfc_eISO14443_3A_PICC
#define phHal_eMifare_PICC phNfc_eMifare_PICC
#define phHal_eISO14443_B_PICC phNfc_eISO14443_B_PICC
#define phHal_eISO14443_4B_PICC phNfc_eISO14443_4B_PICC
#define phHal_eISO14443_BPrime_PICC phNfc_eISO14443_BPrime_PICC
#define phHal_eFelica_PICC phNfc_eFelica_PICC
#define phHal_eJewel_PICC phNfc_eJewel_PICC
#define phHal_eISO15693_PICC phNfc_eISO15693_PICC
#define phHal_eNfcIP1_Target phNfc_eNfcIP1_Target
#define phHal_eNfcIP1_Initiator phNfc_eNfcIP1_Initiator
#define phHal_eNfcIP1_Raw phNfc_eNfcIP1_Raw
#define phHal_eKovio_PICC phNfc_eKovio_PICC
typedef phNfc_eRFDevType_t phHal_eRFDevType_t; /**< \copybrief phNfc_eRFDevType_t \sa phNfc_eRFDevType_t */

typedef phNfc_uCmdList_t phHal_uCmdList_t; /**< \copybrief phNfc_uCmdList_t \sa phNfc_uCmdList_t */

typedef phNfc_sRemoteDevInformation_t phHal_sRemoteDevInformation_t; /**< \copybrief phNfc_sRemoteDevInformation_t \sa phNfc_sRemoteDevInformation_t */
typedef phNfc_sDevInputParam_t phHal_sDevInputParam_t; /**< \copybrief phNfc_sDevInputParam_t \sa phNfc_sDevInputParam_t */
typedef phNfc_sTransceiveInfo_t phHal_sTransceiveInfo_t; /**< \copybrief phNfc_sTransceiveInfo_t \sa phNfc_sTransceiveInfo_t */
typedef phNfc_sIso14443ACfg_t phHal_sIso14443ACfg_t; /**< \copybrief phNfc_sIso14443ACfg_t \sa phNfc_sIso14443ACfg_t */
typedef phNfc_sIso14443BCfg_t phHal_sIso14443BCfg_t; /**< \copybrief phNfc_sIso14443BCfg_t \sa phNfc_sIso14443BCfg_t */
typedef phNfc_sFelicaCfg_t phHal_sFelicaCfg_t; /**< \copybrief phNfc_sFelicaCfg_t \sa phNfc_sFelicaCfg_t */
typedef phNfc_sPollDevInfo_t phHal_sPollDevInfo_t; /**< \copybrief phNfc_sPollDevInfo_t \sa phNfc_sPollDevInfo_t */

#define phHal_eHostController phNfc_eHostController
#define phHal_eTerminalHost phNfc_eTerminalHost
#define phHal_eUICCHost phNfc_eUICCHost
#define phHal_eUnknownHost phNfc_eUnknownHost

typedef phNfc_HostType_t phHal_HostType_t; /**< \copybrief phNfc_HostType_t \sa phNfc_HostType_t */

#define phHal_eDefaultP2PMode phNfc_eDefaultP2PMode
#define phHal_ePassive106 phNfc_ePassive106
#define phHal_ePassive212 phNfc_ePassive212
#define phHal_ePassive424 phNfc_ePassive424
#define phHal_eActive106 phNfc_eActive106
#define phHal_eActive212 phNfc_eActive212
#define phHal_eActive424 phNfc_eActive424

#define phHal_eActive phNfc_eActive
#define phHal_eP2P_ALL phNfc_eP2P_ALL
#define phHal_eInvalidP2PMode phNfc_eInvalidP2PMode

typedef phNfc_eP2PMode_t phHal_eP2PMode_t; /**< \copybrief phNfc_eP2PMode_t \sa phNfc_eP2PMode_t */
typedef phNfc_eNotificationType_t phHal_eNotificationType_t; /**< \copybrief phNfc_eNotificationType_t \sa phNfc_eNotificationType_t */
typedef phNfc_sUiccInfo_t phHal_sUiccInfo_t; /**< \copybrief phNfc_sUiccInfo_t \sa phNfc_sUiccInfo_t */

typedef phNfc_sEmuSupport_t phHal_sEmuSupport_t; /**< \copybrief phNfc_sEmuSupport_t \sa phNfc_sEmuSupport_t */
typedef phNfc_sNfcIPCfg_t phHal_sNfcIPCfg_t; /**< \copybrief phNfc_sNfcIPCfg_t \sa phNfc_sNfcIPCfg_t */
typedef phNfc_eConfigType_t phHal_eConfigType_t; /**< \copybrief phNfc_eConfigType_t \sa phNfc_eConfigType_t */
typedef phNfc_eDiscoveryConfigMode_t phHal_eDiscoveryConfigMode_t; /**< \copybrief phNfc_eDiscoveryConfigMode_t \sa phNfc_eDiscoveryConfigMode_t */
typedef phNfc_eReleaseType_t phHal_eReleaseType_t; /**< \copybrief phNfc_eReleaseType_t \sa phNfc_eReleaseType_t */
typedef phNfc_eEmulationType_t phHal_eEmulationType_t; /**< \copybrief phNfc_eEmulationType_t \sa phNfc_eEmulationType_t */
typedef phNfc_sTargetInfo_t phHal_sTargetInfo_t; /**< \copybrief phNfc_sTargetInfo_t \sa phNfc_sTargetInfo_t */
typedef phNfc_eSmartMX_Mode_t phHal_eSmartMX_Mode_t; /**< \copybrief phNfc_eSmartMX_Mode_t \sa phNfc_eSmartMX_Mode_t */
typedef phNfc_eSWP_Mode_t phHal_eSWP_Mode_t; /**< \copybrief phNfc_eSWP_Mode_t \sa phNfc_eSWP_Mode_t */

typedef phNfc_sSmartMX_Cfg_t phHal_sSmartMX_Cfg_t; /**< \copybrief phNfc_sSmartMX_Cfg_t \sa phNfc_sSmartMX_Cfg_t */
typedef phNfc_sUiccEmuCfg_t phHal_sUiccEmuCfg_t; /**< \copybrief phNfc_sUiccEmuCfg_t \sa phNfc_sUiccEmuCfg_t */
typedef phNfc_sHostEmuCfg_A_t phHal_sHostEmuCfg_A_t; /**< \copybrief phNfc_sHostEmuCfg_A_t \sa phNfc_sHostEmuCfg_A_t */
typedef phNfc_sHostEmuCfg_B_t phHal_sHostEmuCfg_B_t; /**< \copybrief phNfc_sHostEmuCfg_B_t \sa phNfc_sHostEmuCfg_B_t */
typedef phNfc_sHostEmuCfg_F_t phHal_sHostEmuCfg_F_t; /**< \copybrief phNfc_sHostEmuCfg_F_t \sa phNfc_sHostEmuCfg_F_t */

typedef phNfc_sEmulationCfg_t phHal_sEmulationCfg_t; /**< \copybrief phNfc_sEmulationCfg_t \sa phNfc_sEmulationCfg_t */

typedef phNfc_sReaderCfg_t phHal_sReaderCfg_t; /**< \copybrief phNfc_sReaderCfg_t \sa phNfc_sReaderCfg_t */
typedef phNfc_sSEProtectionCfg_t phHal_sSEProtectionCfg_t; /**< \copybrief phNfc_sSEProtectionCfg_t \sa phNfc_sSEProtectionCfg_t */
typedef phNfc_sADD_Cfg_t phHal_sADD_Cfg_t; /**< \copybrief phNfc_sADD_Cfg_t \sa phNfc_sADD_Cfg_t */
typedef phNfc_uConfig_t phHal_uConfig_t; /**< \copybrief phNfc_uConfig_t \sa phNfc_uConfig_t */

/**
 * \addtogroup grp_lib_nfc_mapping
 */

typedef phNfc_sData_t phLibNfc_sData_t; /**< \copybrief phNfc_sData_t \sa phNfc_sData_t */
typedef phNfc_sSupProtocol_t phLibNfc_sSupProtocol_t; /**< \copybrief phNfc_sSupProtocol_t \sa phNfc_sSupProtocol_t */
typedef phNfc_sDeviceCapabilities_t phLibNfc_sDeviceCapabilities_t; /**< \copybrief phNfc_sDeviceCapabilities_t \sa phNfc_sDeviceCapabilities_t */
typedef phNfc_sHwReference_t phLibNfc_sHwReference_t; /**< \copybrief phNfc_sHwReference_t \sa phNfc_sHwReference_t */
typedef phNfc_sDepFlags_t phLibNfc_sDepFlags_t; /**< \copybrief phNfc_sDepFlags_t \sa phNfc_sDepFlags_t */
typedef phNfc_sDepAdditionalInfo_t phLibNfc_sDepAdditionalInfo_t; /**< \copybrief phNfc_sDepAdditionalInfo_t \sa phNfc_sDepAdditionalInfo_t */
#define phLibNfc_eMifareRaw phNfc_eMifareRaw
#define phLibNfc_eMifareAuthentA phNfc_eMifareAuthentA
#define phLibNfc_eMifareAuthentB phNfc_eMifareAuthentB
#define phLibNfc_eMifareRead16 phNfc_eMifareRead16
#define phLibNfc_eMifareRead phNfc_eMifareRead
#define phLibNfc_eMifareWrite16 phNfc_eMifareWrite16
#define phLibNfc_eMifareWrite4 phNfc_eMifareWrite4
#define phLibNfc_eMifareInc phNfc_eMifareInc
#define phLibNfc_eMifareDec phNfc_eMifareDec
#define phLibNfc_eMifareTransfer phNfc_eMifareTransfer
#define phLibNfc_eMifareRestore phNfc_eMifareRestore
#define phLibNfc_eMifareReadSector phNfc_eMifareReadSector
#define phLibNfc_eMifareWriteSector phNfc_eMifareWriteSector
#define phLibNfc_eMifareReadN phNfc_eMifareReadN
#define phLibNfc_eMifareWriteN phNfc_eMifareWriteN
#define phLibNfc_eMifareSectorSel phNfc_eMifareSectorSel
#define phLibNfc_eMifareAuth phNfc_eMifareAuth
#define phLibNfc_eMifareProxCheck phNfc_eMifareProxCheck
#define phLibNfc_eMifareInvalidCmd phNfc_eMifareInvalidCmd
typedef phNfc_eMifareCmdList_t phLibNfc_eMifareCmdList_t; /**< \copybrief phNfc_eMifareCmdList_t \sa phNfc_eMifareCmdList_t */
#define phLibNfc_eIso14443_4_Raw phNfc_eIso14443_4_Raw
typedef phNfc_eIso14443_4_CmdList_t phLibNfc_eIso14443_4_CmdList_t; /**< \copybrief phNfc_eIso14443_4_CmdList_t \sa phNfc_eIso14443_4_CmdList_t */
#define phLibNfc_eIso15693_Raw phNfc_eIso15693_Raw
#define phLibNfc_eIso15693_Cmd phNfc_eIso15693_Cmd
#define phLibNfc_eIso15693_Invalid phNfc_eIso15693_Invalid
typedef phNfc_eIso15693_CmdList_t phLibNfc_eIso15693_CmdList_t; /**< \copybrief phNfc_eIso15693_CmdList_t \sa phNfc_eIso15693_CmdList_t */
#define phLibNfc_eFelica_Raw phNfc_eFelica_Raw
#define phLibNfc_eFelica_Check phNfc_eFelica_Check
#define phLibNfc_eFelica_Update phNfc_eFelica_Update
#define phLibNfc_eFelica_Invalid phNfc_eFelica_Invalid
typedef phNfc_eFelicaCmdList_t phLibNfc_eFelicaCmdList_t; /**< \copybrief phNfc_eFelicaCmdList_t \sa phNfc_eFelicaCmdList_t */
#define phLibNfc_eJewel_Raw phNfc_eJewel_Raw
#define phLibNfc_eJewel_WriteN phNfc_eJewel_WriteN
#define phLibNfc_eJewel_Invalid phNfc_eJewel_Invalid
typedef phNfc_eJewelCmdList_t phLibNfc_eJewelCmdList_t; /**< \copybrief phNfc_eJewelCmdList_t \sa phNfc_eJewelCmdList_t */
#define phLibNfc_eHeidRead phNfc_eHeidRead
typedef phNfc_eHid_CmdList_t phLibNfc_eHid_CmdList_t; /**< \copybrief phNfc_eHid_CmdList_t \sa phNfc_eHid_CmdList_t */
#define phLibNfc_eEpcGenRaw phNfc_eEpcGen_Raw
typedef phNfc_eEpcGenCmdList_t phLibNfc_eEpcGenCmdList_t; /**< \copybrief phNfc_eEpcGen_t \sa phNfc_eEpcGen_t */
typedef phNfc_sIso14443AInfo_t phLibNfc_sIso14443AInfo_t; /**< \copybrief phNfc_sIso14443AInfo_t \sa phNfc_sIso14443AInfo_t */
typedef phNfc_sIso14443BInfo_t phLibNfc_sIso14443BInfo_t; /**< \copybrief phNfc_sIso14443BInfo_t \sa phNfc_sIso14443BInfo_t */
typedef phNfc_sIso14443BPrimeInfo_t phLibNfc_sIso14443BPrimeInfo_t; /**< \copybrief phNfc_sIso14443BPrimeInfo_t \sa phNfc_sIso14443BPrimeInfo_t */
typedef phNfc_sJewelInfo_t phLibNfc_sJewelInfo_t; /**< \copybrief phNfc_sJewelInfo_t \sa phNfc_sJewelInfo_t */
typedef phNfc_sFelicaInfo_t phLibNfc_sFelicaInfo_t; /**< \copybrief phNfc_sFelicaInfo_t \sa phNfc_sFelicaInfo_t */
typedef phNfc_sIso15693Info_t phLibNfc_sIso15693Info_t; /**< \copybrief phNfc_sIso15693Info_t \sa phNfc_sIso15693Info_t */
#define phLibNfc_eDataRate_106 phNfc_eDataRate_106
#define phLibNfc_eDataRate_212 phNfc_eDataRate_212
#define phLibNfc_eDataRate_424 phNfc_eDataRate_424
#define phLibNfc_eDataRate_848 phNfc_eDataRate_848
#define phLibNfc_eDataRate_1696 phNfc_eDataRate_1696
#define phLibNfc_eDataRate_3392 phNfc_eDataRate_3392
#define phLibNfc_eDataRate_6784 phNfc_eDataRate_6784
#define phLibNfc_eDataRate_RFU phNfc_eDataRate_RFU
typedef phNfc_eDataRate_t phLibNfc_eDataRate_t; /**< \copybrief phNfc_eDataRate_t \sa phNfc_eDataRate_t */
typedef phNfc_sNfcIPInfo_t phLibNfc_sNfcIPInfo_t; /**< \copybrief phNfc_sNfcIPInfo_t \sa phNfc_sNfcIPInfo_t */
typedef phNfc_uRemoteDevInfo_t phLibNfc_uRemoteDevInfo_t; /**< \copybrief phNfc_uRemoteDevInfo_t \sa phNfc_uRemoteDevInfo_t */
#define phLibNfc_eUnknown_DevType phNfc_eUnknown_DevType
#define phLibNfc_eISO14443_A_PCD phNfc_eISO14443_A_PCD
#define phLibNfc_eISO14443_B_PCD phNfc_eISO14443_B_PCD
#define phLibNfc_eISO14443_BPrime_PCD phNfc_eISO14443_BPrime_PCD
#define phLibNfc_eFelica_PCD phNfc_eFelica_PCD
#define phLibNfc_eJewel_PCD phNfc_eJewel_PCD
#define phLibNfc_eISO15693_PCD phNfc_eISO15693_PCD
#define phLibNfc_ePCD_DevType phNfc_ePCD_DevType
#define phLibNfc_ePICC_DevType phNfc_ePICC_DevType
#define phLibNfc_eISO14443_A_PICC phNfc_eISO14443_A_PICC
#define phLibNfc_eISO14443_4A_PICC phNfc_eISO14443_4A_PICC
#define phLibNfc_eISO14443_3A_PICC phNfc_eISO14443_3A_PICC
#define phLibNfc_eMifare_PICC phNfc_eMifare_PICC
#define phLibNfc_eISO14443_B_PICC phNfc_eISO14443_B_PICC
#define phLibNfc_eISO14443_4B_PICC phNfc_eISO14443_4B_PICC
#define phLibNfc_eISO14443_BPrime_PICC phNfc_eISO14443_BPrime_PICC
#define phLibNfc_eFelica_PICC phNfc_eFelica_PICC
#define phLibNfc_eJewel_PICC phNfc_eJewel_PICC
#define phLibNfc_eISO15693_PICC phNfc_eISO15693_PICC
#define phLibNfc_eNfcIP1_Target phNfc_eNfcIP1_Target
#define phLibNfc_eNfcIP1_Initiator phNfc_eNfcIP1_Initiator
#define phLibNfc_eKovio_PICC phNfc_eKovio_PICC

typedef phNfc_eRFDevType_t phLibNfc_eRFDevType_t; /**< \copybrief phNfc_eRFDevType_t \sa phNfc_eRFDevType_t */

typedef phNfc_uCmdList_t phLibNfc_uCmdList_t; /**< \copybrief phNfc_uCmdList_t \sa phNfc_uCmdList_t */
typedef phNfc_eRemDevType_t phLibNfc_eRemDevType_t;
typedef phNfc_sRemoteDevInformation_t phLibNfc_sRemoteDevInformation_t; /**< \copybrief phNfc_sRemoteDevInformation_t \sa phNfc_sRemoteDevInformation_t */
typedef phNfc_sDevInputParam_t phLibNfc_sDevInputParam_t; /**< \copybrief phNfc_sDevInputParam_t \sa phNfc_sDevInputParam_t */
typedef phNfc_sTransceiveInfo_t phLibNfc_sTransceiveInfo_t; /**< \copybrief phNfc_sTransceiveInfo_t \sa phNfc_sTransceiveInfo_t */
typedef phNfc_sIso14443ACfg_t phLibNfc_sIso14443ACfg_t; /**< \copybrief phNfc_sIso14443ACfg_t \sa phNfc_sIso14443ACfg_t */
typedef phNfc_sIso14443BCfg_t phLibNfc_sIso14443BCfg_t; /**< \copybrief phNfc_sIso14443BCfg_t \sa phNfc_sIso14443BCfg_t */
typedef phNfc_sFelicaCfg_t phLibNfc_sFelicaCfg_t; /**< \copybrief phNfc_sFelicaCfg_t \sa phNfc_sFelicaCfg_t */
typedef phNfc_sPollDevInfo_t phLibNfc_sPollDevInfo_t; /**< \copybrief phNfc_sPollDevInfo_t \sa phNfc_sPollDevInfo_t */
#define phLibNfc_eHostController phNfc_eHostController
#define phLibNfc_eTerminalHost phNfc_eTerminalHost
#define phLibNfc_eUICCHost phNfc_eUICCHost
#define phLibNfc_eUnknownHost phNfc_eUnknownHost
typedef phNfc_HostType_t phLibNfc_HostType_t; /**< \copybrief phNfc_HostType_t \sa phNfc_HostType_t */
#define phLibNfc_eDefaultP2PMode phNfc_eDefaultP2PMode
#define phLibNfc_ePassive106 phNfc_ePassive106
#define phLibNfc_ePassive212 phNfc_ePassive212
#define phLibNfc_ePassive424 phNfc_ePassive424
#define phLibNfc_eActive phNfc_eActive
#define phLibNfc_eP2P_ALL phNfc_eP2P_ALL
#define phLibNfc_eInvalidP2PMode phNfc_eInvalidP2PMode
typedef phNfc_eP2PMode_t phLibNfc_eP2PMode_t; /**< \copybrief phNfc_eP2PMode_t \sa phNfc_eP2PMode_t */
typedef phNfc_eNotificationType_t phLibNfc_eNotificationType_t; /**< \copybrief phNfc_eNotificationType_t \sa phNfc_eNotificationType_t */
typedef phNfc_sUiccInfo_t phLibNfc_sUiccInfo_t; /**< \copybrief phNfc_sUiccInfo_t \sa phNfc_sUiccInfo_t */
typedef phNfc_sEmuSupport_t phLibNfc_sEmuSupport_t; /**< \copybrief phNfc_sEmuSupport_t \sa phNfc_sEmuSupport_t */
typedef phNfc_sNfcIPCfg_t phLibNfc_sNfcIPCfg_t; /**< \copybrief phNfc_sNfcIPCfg_t \sa phNfc_sNfcIPCfg_t */
typedef phNfc_eConfigType_t phLibNfc_eConfigType_t; /**< \copybrief phNfc_eConfigType_t \sa phNfc_eConfigType_t */
typedef phNfc_eDiscoveryConfigMode_t phLibNfc_eDiscoveryConfigMode_t; /**< \copybrief phNfc_eDiscoveryConfigMode_t \sa phNfc_eDiscoveryConfigMode_t */
typedef phNfc_eReleaseType_t phLibNfc_eReleaseType_t; /**< \copybrief phNfc_eReleaseType_t \sa phNfc_eReleaseType_t */
typedef phNfc_eDiscAndDisconnMode_t phLibNfc_eDiscAndDisconnMode_t; /**< \copybrief phNfc_eDiscAndDisconnMode_t \sa phNfc_eDiscAndDisconnMode_t */
typedef phNfc_eEmulationType_t phLibNfc_eEmulationType_t; /**< \copybrief phNfc_eEmulationType_t \sa phNfc_eEmulationType_t */
typedef phNfc_sTargetInfo_t phLibNfc_sTargetInfo_t; /**< \copybrief phNfc_sTargetInfo_t \sa phNfc_sTargetInfo_t */
typedef phNfc_eSmartMX_Mode_t phLibNfc_eSmartMX_Mode_t; /**< \copybrief phNfc_eSmartMX_Mode_t \sa phNfc_eSmartMX_Mode_t */
typedef phNfc_eSWP_Mode_t phLibNfc_eSWP_Mode_t; /**< \copybrief phNfc_eSWP_Mode_t \sa phNfc_eSWP_Mode_t */
typedef phNfc_sSmartMX_Cfg_t phLibNfc_sSmartMX_Cfg_t; /**< \copybrief phNfc_sSmartMX_Cfg_t \sa phNfc_sSmartMX_Cfg_t */
typedef phNfc_sUiccEmuCfg_t phLibNfc_sUiccEmuCfg_t; /**< \copybrief phNfc_sUiccEmuCfg_t \sa phNfc_sUiccEmuCfg_t */
typedef phNfc_sHostEmuCfg_A_t phLibNfc_sHostEmuCfg_A_t; /**< \copybrief phNfc_sHostEmuCfg_A_t \sa phNfc_sHostEmuCfg_A_t */
typedef phNfc_sHostEmuCfg_B_t phLibNfc_sHostEmuCfg_B_t; /**< \copybrief phNfc_sHostEmuCfg_B_t \sa phNfc_sHostEmuCfg_B_t */
typedef phNfc_sHostEmuCfg_F_t phLibNfc_sHostEmuCfg_F_t; /**< \copybrief phNfc_sHostEmuCfg_F_t \sa phNfc_sHostEmuCfg_F_t */
typedef phNfc_sEmulationCfg_t phLibNfc_sEmulationCfg_t; /**< \copybrief phNfc_sEmulationCfg_t \sa phNfc_sEmulationCfg_t */
typedef phNfc_sReaderCfg_t phLibNfc_sReaderCfg_t; /**< \copybrief phNfc_sReaderCfg_t \sa phNfc_sReaderCfg_t */
typedef phNfc_sSEProtectionCfg_t phLibNfc_sSEProtectionCfg_t; /**< \copybrief phNfc_sSEProtectionCfg_t \sa phNfc_sSEProtectionCfg_t */
typedef phNfc_sADD_Cfg_t phLibNfc_sADD_Cfg_t; /**< \copybrief phNfc_sADD_Cfg_t \sa phNfc_sADD_Cfg_t */
typedef phNfc_uConfig_t phLibNfc_uConfig_t; /**< \copybrief phNfc_uConfig_t \sa phNfc_uConfig_t */
