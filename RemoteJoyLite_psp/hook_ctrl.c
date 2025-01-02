/*------------------------------------------------------------------------------*/
/* hook_ctrl																	*/
/*------------------------------------------------------------------------------*/
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <pspctrl_kernel.h>
#include <string.h>
#include "debug.h"
#include "hook.h"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define ABS(x)				((x)<0?-x:x)
#define GET_JUMP_TARGET(x)	(0x80000000|(((x)&0x03FFFFFF)<<2))

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static SceCtrlData JoyData;
static SceCtrlLatch JoyLatch;
static u32 LatchCount = 0;
static u32 ButtonData = 0;
static u32 AnalogData = 0x00800080;
static int (*sceCtrlBuffer_Func)( SceCtrlData *, int, int ) = NULL;
static int (*sceCtrlPeekLatch_Func)( SceCtrlLatch * ) = NULL;
static int (*sceCtrlReadLatch_Func)( SceCtrlLatch * ) = NULL;

/*------------------------------------------------------------------------------*/
/* CalcAnalog																	*/
/*------------------------------------------------------------------------------*/
static int CalcAnalog( int old, int new )
{
	int val1 = old - 127;
	int val2 = new - 127;
	if ( ABS(val1) > ABS(val2) ){ return( old ); }
	else						{ return( new ); }
}

/*------------------------------------------------------------------------------*/
/* CalcButtonMask																*/
/*------------------------------------------------------------------------------*/
static u32 CalcButtonMask( void )
{
	int i;
	u32 mask = 0;

	for ( i=0; i<32; i++ ){
		if ( sceCtrlGetButtonMask(1<<i) == 1 ){ mask |= (1<<i); }
	}
	return( mask );
}

/*------------------------------------------------------------------------------*/
/* AddValues																	*/
/*------------------------------------------------------------------------------*/
static void AddValues( SceCtrlData *data, int count, int neg )
{
	int i, k1;
	u32 button = ButtonData;
	int Ly = (AnalogData >> 16) & 0xFF;
	int Lx = (AnalogData >>  0) & 0xFF;

	asm __volatile__ ( "move %0, $k1" : "=r"(k1) );
	if ( k1 ){ button &= ~CalcButtonMask(); }
	for ( i=0; i<count; i++ ){
		if ( neg ){ data[i].Buttons &= ~button; }
		else	  { data[i].Buttons |=  button; }
		data[i].Lx = CalcAnalog( data[i].Lx, Lx );
		data[i].Ly = CalcAnalog( data[i].Ly, Ly );
	}
}

/*------------------------------------------------------------------------------*/
/* MyCtrlBuffer																	*/
/*------------------------------------------------------------------------------*/
static int MyCtrlBuffer( SceCtrlData *data, int count, int type )
{
	int ret = sceCtrlBuffer_Func( data, count, type );
	if ( ret <= 0 ){ return( ret ); }
	int intc = pspSdkDisableInterrupts();
	AddValues( data, ret, type & 1 );
	pspSdkEnableInterrupts( intc );
	return( ret );
}

#if 0
/*------------------------------------------------------------------------------*/
/* HookCtrlBufferFunction														*/
/*------------------------------------------------------------------------------*/
static void HookCtrlBufferFunction( u32 *jump )
{
	u32 target = GET_JUMP_TARGET( *jump );
	u32 inst   = _lw( target+8 );
	u32 func   = (u32)MyCtrlBuffer;

	if ( (inst & ~0x03FFFFFF) != 0x0C000000 ){
		#ifndef RELEASE
		debug_printf("%s: failed hooking buffer function\n", __func__);
		#endif
		return;
	}
	sceCtrlBuffer_Func = (void *)GET_JUMP_TARGET( inst );
	func = (func & 0x0FFFFFFF) >> 2;
	_sw( 0x0C000000 | func, target+8 );
}

/*------------------------------------------------------------------------------*/
/* hookCtrlBuffer																*/
/*------------------------------------------------------------------------------*/
void hookCtrlBuffer( void )
{
	memset( &JoyData,  0, sizeof(JoyData)  );
	memset( &JoyLatch, 0, sizeof(JoyLatch) );
	JoyData.Lx = 0x80;
	JoyData.Ly = 0x80;

	HookCtrlBufferFunction( (u32 *)sceCtrlReadBufferPositive );
	HookCtrlBufferFunction( (u32 *)sceCtrlPeekBufferPositive );
	HookCtrlBufferFunction( (u32 *)sceCtrlReadBufferNegative );
	HookCtrlBufferFunction( (u32 *)sceCtrlPeekBufferNegative );
}
#endif

/*******************************************************************************/
/*------------------------------------------------------------------------------*/
/* MyCtrlPeekLatch																*/
/*------------------------------------------------------------------------------*/
static int MyCtrlPeekLatch( SceCtrlLatch *latch )
{
	int ret  = sceCtrlPeekLatch_Func( latch );
	int intc = pspSdkDisableInterrupts();
	if ( (JoyLatch.uiMake|JoyLatch.uiBreak|JoyLatch.uiPress) != 0 ){
		latch->uiMake    = JoyLatch.uiMake;
		latch->uiBreak   = JoyLatch.uiBreak;
		latch->uiPress   = JoyLatch.uiPress;
		latch->uiRelease = JoyLatch.uiRelease;
	} else if ( JoyLatch.uiRelease == 0 ){
		latch->uiMake    = 0;
		latch->uiBreak   = 0;
		latch->uiPress   =  ButtonData;
		latch->uiRelease = ~ButtonData;
	}
	pspSdkEnableInterrupts( intc );
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* MyCtrlReadLatch																*/
/*------------------------------------------------------------------------------*/
static int MyCtrlReadLatch( SceCtrlLatch *latch )
{
	int ret  = sceCtrlReadLatch_Func( latch );
	int intc = pspSdkDisableInterrupts();
	if ( (JoyLatch.uiMake|JoyLatch.uiBreak|JoyLatch.uiPress) != 0 ){
		latch->uiMake    = JoyLatch.uiMake;
		latch->uiBreak   = JoyLatch.uiBreak;
		latch->uiPress   = JoyLatch.uiPress;
		latch->uiRelease = JoyLatch.uiRelease;
	} else if ( JoyLatch.uiRelease == 0 ){
		if ( LatchCount < 60 ){
			latch->uiMake    = 0;
			latch->uiBreak   = 0;
			latch->uiPress   =  ButtonData;
			latch->uiRelease = ~ButtonData;
			LatchCount++;
		}
	}
	JoyLatch.uiMake    = 0;
	JoyLatch.uiBreak   = 0;
	JoyLatch.uiPress   = 0;
	JoyLatch.uiRelease = 0;
	pspSdkEnableInterrupts( intc );
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* hookCtrlLatch																*/
/*------------------------------------------------------------------------------*/
#if 0
void hookCtrlLatch( void )
{
	SceModule *module = sceKernelFindModuleByName( "sceController_Service" );
	if ( module == NULL ){
		#ifndef RELEASE
		debug_printf("%s: failed fetching sceController_Service module\n", __func__);
		#endif
		return;
	}

	if ( sceCtrlPeekLatch_Func == NULL ){
		sceCtrlPeekLatch_Func = HookNidAddress( module, "sceCtrl", 0xB1D0E5CD );
		void *hook_addr = HookSyscallAddress( sceCtrlPeekLatch_Func );
		#ifndef RELEASE
		if(hook_addr == NULL){
			debug_printf("%s: failed creating hook for sceCtrlPeekLatch\n", __func__);
		}
		#endif
		HookFuncSetting( hook_addr, MyCtrlPeekLatch );
	}

	if ( sceCtrlReadLatch_Func == NULL ){
		sceCtrlReadLatch_Func = HookNidAddress( module, "sceCtrl", 0x0B588501 );
		void *hook_addr = HookSyscallAddress( sceCtrlReadLatch_Func );
		#ifndef RELEASE
		if(hook_addr == NULL){
			debug_printf("%s: failed creating hook for sceCtrlReadLatch\n", __func__);
		}
		#endif
		HookFuncSetting( hook_addr, MyCtrlReadLatch );
	}
}
#endif
/*******************************************************************************/
/*------------------------------------------------------------------------------*/
/* hookCtrlSetData																*/
/*------------------------------------------------------------------------------*/
void hookCtrlSetData( u32 PreData, u32 NowData, u32 Analog )
{
	ButtonData = NowData;
	AnalogData = Analog;
	JoyLatch.uiMake    |= ~PreData &  NowData;
	JoyLatch.uiBreak   |=  PreData & ~NowData;
	JoyLatch.uiPress   |=  NowData;
	JoyLatch.uiRelease |= ~NowData;
	LatchCount = 0;
}

typedef int (*CTRL_BUFFER_FUNC)(SceCtrlData *pad_data, int count);

static CTRL_BUFFER_FUNC sceCtrlPeekBufferPositiveOrig = NULL;
static CTRL_BUFFER_FUNC sceCtrlPeekBufferNegativeOrig = NULL;
static CTRL_BUFFER_FUNC sceCtrlReadBufferPositiveOrig = NULL;
static CTRL_BUFFER_FUNC sceCtrlReadBufferNegativeOrig = NULL;

static int sceCtrlPeekBufferPositivePatched(SceCtrlData *pad_data, int count){
	int ret = sceCtrlPeekBufferPositiveOrig(pad_data, count);
	AddValues(pad_data, ret, 0);
	return ret;
}

static int sceCtrlPeekBufferNegativePatched(SceCtrlData *pad_data, int count){
	int ret = sceCtrlPeekBufferNegativeOrig(pad_data, count);
	AddValues(pad_data, ret, 1);
	return ret;
}

static int sceCtrlReadBufferPositivePatched(SceCtrlData *pad_data, int count){
	int ret = sceCtrlReadBufferPositiveOrig(pad_data, count);
	AddValues(pad_data, ret, 0);
	return ret;
}

static int sceCtrlReadBufferNegativePatched(SceCtrlData *pad_data, int count){
	int ret = sceCtrlReadBufferNegativeOrig(pad_data, count);
	AddValues(pad_data, ret, 1);
	return ret;
}

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);
#define GET_JUMP_TARGET(x) (0x80000000 | (((x) & 0x03FFFFFF) << 2))

#define HIJACK_SYSCALL_STUB(a, f, ptr) { \
	u32 _func_ = (u32)a; \
	_func_ = GET_JUMP_TARGET(_lw(_func_)); \
	u32 _ff = (u32)f; \
	int _interrupts = pspSdkDisableInterrupts(); \
	static u32 trampoline[3]; \
	_sw(_lw(_func_), (u32)trampoline); \
	_sw(_lw(_func_ + 4), (u32)trampoline + 8); \
	MAKE_JUMP((u32)trampoline + 4, _func_ + 8); \
	MAKE_JUMP(_func_, _ff); \
	_sw(0, _func_ + 4); \
	_sw((u32)trampoline, (u32)&ptr); \
	sceKernelDcacheWritebackInvalidateAll(); \
	sceKernelIcacheClearAll(); \
	pspSdkEnableInterrupts(_interrupts); \
}

void hookCtrlBuffer( void ){
	#if 0
	SceModule *module = sceKernelFindModuleByName( "sceController_Service" );
	if ( module == NULL ){
		#ifndef RELEASE
		debug_printf("%s: failed fetching sceController_Service module\n", __func__);
		#endif
		return;
	}

	// huh, why isn't nid hooking working on 6.xx
	#define STR_(s) #s
	#ifndef RELEASE
	#define HOOK_LOG_(name) { \
		if(name##Orig == NULL) { \
			debug_printf("%s: failed fetching " STR_(name) " from nid\n", __func__); \
		} \
		if(hook_addr == NULL){ \
			debug_printf("%s: failed creating hook for " STR_(name) "\n", __func__); \
		} \
	}
	#else
	#define HOOK_LOG_(name)
	#endif

	#define HOOK_(nid, name) { \
		if(name##Orig == NULL) { \
			name##Orig = HookNidAddress(module, "sceCtrl", nid); \
			void *hook_addr = HookSyscallAddress(name##Orig); \
			HOOK_LOG_(name); \
			HookFuncSetting(hook_addr, name##Patched); \
		} \
	}

	HOOK_(0x3A622550, sceCtrlPeekBufferPositive);
	HOOK_(0xC152080A, sceCtrlPeekBufferNegative);
	HOOK_(0x1F803938, sceCtrlReadBufferPositive);
	HOOK_(0x60B81F86, sceCtrlReadBufferNegative);

	#undef STR_
	#undef HOOK_LOG_
	#undef HOOK_

	#else

	HIJACK_SYSCALL_STUB(sceCtrlPeekBufferPositive, sceCtrlPeekBufferPositivePatched, sceCtrlPeekBufferPositiveOrig);
	HIJACK_SYSCALL_STUB(sceCtrlPeekBufferNegative, sceCtrlPeekBufferNegativePatched, sceCtrlPeekBufferNegativeOrig);
	HIJACK_SYSCALL_STUB(sceCtrlReadBufferPositive, sceCtrlReadBufferPositivePatched, sceCtrlReadBufferPositiveOrig);
	HIJACK_SYSCALL_STUB(sceCtrlReadBufferNegative, sceCtrlReadBufferNegativePatched, sceCtrlReadBufferNegativeOrig);

	#endif
}

void hookCtrlLatch( void ){
	HIJACK_SYSCALL_STUB(sceCtrlPeekLatch, MyCtrlPeekLatch, sceCtrlPeekLatch_Func);
	HIJACK_SYSCALL_STUB(sceCtrlReadLatch, MyCtrlReadLatch, sceCtrlReadLatch_Func);
}
