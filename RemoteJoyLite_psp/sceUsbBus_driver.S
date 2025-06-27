	.set noreorder

#include "pspimport.s"

IMPORT_START "sceUsbBus_driver",0x00090000
IMPORT_FUNC "sceUsbBus_driver",0xB1644BE7,sceUsbbdRegister
IMPORT_FUNC "sceUsbBus_driver",0xC1E2A540,sceUsbbdUnregister
IMPORT_FUNC "sceUsbBus_driver",0x23E51D8F,sceUsbbdReqSend
IMPORT_FUNC "sceUsbBus_driver",0x913EC15D,sceUsbbdReqRecv
