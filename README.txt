The NFC class extension driver implements all standard NFC Forum Tag (T1T, T2T, T3T, ISO-DEP) and P2P (LLCP and SNEP) protocols, 
and RF Management based on the NCI Core specification. The class extension driver implements all the Windows-defined device
driver interfaces to interact with the NFC controller, Secure Elements, and Remote RF endpoints.

The following are the Windows-defined NFC driver DDI that are implemented by the NFC CX driver:
•Near Field Proximity DDI
•NFC Secure Element Management DDI
•Smart Card DDI for contactless smart card access
•NFC Radio Management DDI
•DTA DDI for NFC Forum certification

The following are the NFC Forum specifications implemented by the NFC CX driver:
•NFC Controller Interface, NCI 1.0 Specification
•NFC Data Exchange Format, NDEF
•NFC Forum Type 1-4 Tag
•Logical Link Control Protocol, LLCP 1.1 Specification
•Simple NDEF Exchange Protocol, SNEP 1.0 Specification
•ISO/IEC 15693

For more information, see https://msdn.microsoft.com/en-us/library/windows/hardware/dn905534(v=vs.85).aspx



