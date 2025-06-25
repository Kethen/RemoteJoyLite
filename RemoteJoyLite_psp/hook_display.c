/*------------------------------------------------------------------------------*/
/* hook_display																	*/
/*------------------------------------------------------------------------------*/
#include <pspkernel.h>
#include "debug.h"
#include "hook.h"

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern void SendDispEvent( void );

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static int (*sceDisplaySetFrameBuf_Func)( void *, int, int, int ) = NULL;
static int (*sceDisplayWaitVblank_Func)( void ) = NULL;
static int (*sceDisplayWaitVblankCB_Func)( void ) = NULL;
static int (*sceDisplayWaitVblankStart_Func)( void ) = NULL;
static int (*sceDisplayWaitVblankStartCB_Func)( void ) = NULL;

/*------------------------------------------------------------------------------*/
/* MyDisplaySetFrameBuf															*/
/*------------------------------------------------------------------------------*/
static int MyDisplaySetFrameBuf( void *topaddr, int bufferwidth, int pixelformat, int sync )
{
	SendDispEvent();
	int ret = sceDisplaySetFrameBuf_Func( topaddr, bufferwidth, pixelformat, sync );
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* MyDisplayWaitVblank															*/
/*------------------------------------------------------------------------------*/
static int MyDisplayWaitVblank( void )
{
	SendDispEvent();
	int ret = sceDisplayWaitVblank_Func();
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* MyDisplayWaitVblankCB														*/
/*------------------------------------------------------------------------------*/
static int MyDisplayWaitVblankCB( void )
{
	SendDispEvent();
	int ret = sceDisplayWaitVblankCB_Func();
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* MyDisplayWaitVblankStart														*/
/*------------------------------------------------------------------------------*/
static int MyDisplayWaitVblankStart( void )
{
	SendDispEvent();
	int ret = sceDisplayWaitVblankStart_Func();
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* MyDisplayWaitVblankStartCB													*/
/*------------------------------------------------------------------------------*/
static int MyDisplayWaitVblankStartCB( void )
{
	SendDispEvent();
	int ret = sceDisplayWaitVblankStartCB_Func();
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* hookDisplay																	*/
/*------------------------------------------------------------------------------*/
#if 0
void hookDisplay( void )
{
	SceModule *module = sceKernelFindModuleByName( "sceDisplay_Service" );
	if ( module == NULL ){ return; }

	if ( sceDisplaySetFrameBuf_Func == NULL ){
		sceDisplaySetFrameBuf_Func = HookNidAddress( module, "sceDisplay", 0x289D82FE );
		void *hook_addr = HookSyscallAddress( sceDisplaySetFrameBuf_Func );
		HookFuncSetting( hook_addr, MyDisplaySetFrameBuf );
	}

	if ( sceDisplayWaitVblank_Func == NULL ){
		sceDisplayWaitVblank_Func = HookNidAddress( module, "sceDisplay", 0x36CDFADE );
		void *hook_addr = HookSyscallAddress( sceDisplayWaitVblank_Func );
		HookFuncSetting( hook_addr, MyDisplayWaitVblank );
	}

	if ( sceDisplayWaitVblankCB_Func == NULL ){
		sceDisplayWaitVblankCB_Func = HookNidAddress( module, "sceDisplay", 0x8EB9EC49 );
		void *hook_addr = HookSyscallAddress( sceDisplayWaitVblankCB_Func );
		HookFuncSetting( hook_addr, MyDisplayWaitVblankCB );
	}

	if ( sceDisplayWaitVblankStart_Func == NULL ){
		sceDisplayWaitVblankStart_Func = HookNidAddress( module, "sceDisplay", 0x984C27E7 );
		void *hook_addr = HookSyscallAddress( sceDisplayWaitVblankStart_Func );
		HookFuncSetting( hook_addr, MyDisplayWaitVblankStart );
	}

	if ( sceDisplayWaitVblankStartCB_Func == NULL ){
		sceDisplayWaitVblankStartCB_Func = HookNidAddress( module, "sceDisplay", 0x46F186C3 );
		void *hook_addr = HookSyscallAddress( sceDisplayWaitVblankStartCB_Func );
		HookFuncSetting( hook_addr, MyDisplayWaitVblankStartCB );
	}
}
#endif

void hookDisplay(){
	u32 DisplaySetFrameBuf = GET_JUMP_TARGET(_lw((u32)sceDisplaySetFrameBuf));
	u32 DisplayWaitVblank = GET_JUMP_TARGET(_lw((u32)sceDisplayWaitVblank));
	u32 DisplayWaitVblankCB = GET_JUMP_TARGET(_lw((u32)sceDisplayWaitVblankCB));
	u32 DisplayWaitVblankStart = GET_JUMP_TARGET(_lw((u32)sceDisplayWaitVblankStart));
	u32 DisplayWaitVblankStartCB = GET_JUMP_TARGET(_lw((u32)sceDisplayWaitVblankStartCB));

	#define str(s) #s
	#define LOG_IMPORT(n) { \
		EARLY_LOG("%s: %s 0x%x\n", __func__, str(n), n); \
	}
	LOG_IMPORT(DisplaySetFrameBuf);
	LOG_IMPORT(DisplayWaitVblank);
	LOG_IMPORT(DisplayWaitVblankCB);
	LOG_IMPORT(DisplayWaitVblankStart);
	LOG_IMPORT(DisplayWaitVblankStartCB);

	HIJACK_FUNCTION(DisplaySetFrameBuf, MyDisplaySetFrameBuf, sceDisplaySetFrameBuf_Func);
	HIJACK_FUNCTION(DisplayWaitVblank, MyDisplayWaitVblank, sceDisplayWaitVblank_Func);
	HIJACK_FUNCTION(DisplayWaitVblankCB, MyDisplayWaitVblankCB, sceDisplayWaitVblankCB_Func);
	HIJACK_FUNCTION(DisplayWaitVblankStart, MyDisplayWaitVblankStart, sceDisplayWaitVblankStart_Func);
	HIJACK_FUNCTION(DisplayWaitVblankStartCB, MyDisplayWaitVblankStartCB, sceDisplayWaitVblankStartCB_Func);

	/*
	HIJACK_SYSCALL_STUB(sceDisplaySetFrameBuf, MyDisplaySetFrameBuf, sceDisplaySetFrameBuf_Func);
	HIJACK_SYSCALL_STUB(sceDisplayWaitVblank, MyDisplayWaitVblank, sceDisplayWaitVblank_Func);
	HIJACK_SYSCALL_STUB(sceDisplayWaitVblankCB, MyDisplayWaitVblankCB, sceDisplayWaitVblankCB_Func);
	HIJACK_SYSCALL_STUB(sceDisplayWaitVblankStart, MyDisplayWaitVblankStart, sceDisplayWaitVblankStart_Func);
	HIJACK_SYSCALL_STUB(sceDisplayWaitVblankStartCB, MyDisplayWaitVblankStartCB, sceDisplayWaitVblankStartCB_Func);
	*/
}
