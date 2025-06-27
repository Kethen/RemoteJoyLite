    .set noreorder

#include "pspimport.s"

IMPORT_START "sceUsb_driver",0x00090000
IMPORT_FUNC "sceUsb_driver",0xAE5DE6AF,sceUsbStart
IMPORT_FUNC "sceUsb_driver",0xC2464FA0,sceUsbStop
IMPORT_FUNC "sceUsb_driver",0x586DB82C,sceUsbActivate
IMPORT_FUNC "sceUsb_driver",0xC572A9C8,sceUsbDeactivate
