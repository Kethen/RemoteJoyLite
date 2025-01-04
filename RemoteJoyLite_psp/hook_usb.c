/*------------------------------------------------------------------------------*/
/* hook_usb																		*/
/*------------------------------------------------------------------------------*/
#include <pspkernel.h>
#include <pspusb.h>
#include <pspusbbus.h>
#include <string.h>
#include "debug.h"
#include "usb.h"
#define IS_SELF
#include "hook_usb.h"
#undef IS_SELF
#include "hook.h"

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
int (*sceUsbStart_Func)( const char *, int, void * ) = NULL;
int (*sceUsbStop_Func)( const char *, int, void * ) = NULL;

/*------------------------------------------------------------------------------*/
/* MyUsbStart																	*/
/*------------------------------------------------------------------------------*/
static int MyUsbStart( const char *name, int args, void *argp )
{
	#if 0

	// black list all drivers except for the storage driver
	if(strcmp(name, "USBStor_Driver") == 0){
		#ifndef RELEASE
		debug_printf("%s: usb driver %s whitelisted\n", __func__, name);
		#endif
		UsbSuspend();
		return sceUsbStart_Func(name, args, argp);
	}
	#ifndef RELEASE
	debug_printf("%s: usb driver %s not whitelisted\n", __func__, name);
	#endif
	return -1;

	#else

	#ifndef RELEASE
	debug_printf("%s: usb driver %s wants to start but blocked\n", __func__, name);
	#endif
	return -1;

	#endif
}

/*------------------------------------------------------------------------------*/
/* MyUsbStop																	*/
/*------------------------------------------------------------------------------*/
static int MyUsbStop( const char *name, int args, void *argp )
{
	#ifndef RELEASE
	debug_printf("%s: usb driver %s\n", __func__, name);
	#endif

	#if 0
	if(strcmp(name, "USBStor_Driver") == 0){
		UsbResume();
	}

	return sceUsbStop_Func(name, args, argp);

	#else

	UsbResume();
	return -1;

	#endif
}

/*------------------------------------------------------------------------------*/
/* hookUsbFunc																	*/
/*------------------------------------------------------------------------------*/
#if 0
void hookUsbFunc( void )
{
	SceModule *module = sceKernelFindModuleByName( "sceUSB_Driver" );
	if ( module == NULL ){ return; }

	if ( sceUsbStart_Func == NULL ){
		sceUsbStart_Func = HookNidAddress( module, "sceUsb", 0xAE5DE6AF );
		void *hook_addr = HookSyscallAddress( sceUsbStart_Func );
		HookFuncSetting( hook_addr, MyUsbStart );
	}

	if ( sceUsbStop_Func == NULL ){
		sceUsbStop_Func = HookNidAddress( module, "sceUsb", 0xC2464FA0 );
		void *hook_addr = HookSyscallAddress( sceUsbStop_Func );
		HookFuncSetting( hook_addr, MyUsbStop );
	}
}
#endif

void hookUsbFunc(){
	HIJACK_SYSCALL_STUB(sceUsbStart, MyUsbStart, sceUsbStart_Func);
	HIJACK_SYSCALL_STUB(sceUsbStop, MyUsbStop, sceUsbStop_Func);
}
