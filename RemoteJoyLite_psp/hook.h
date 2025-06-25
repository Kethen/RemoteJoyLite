#ifndef _HOOK_H_
#define _HOOK_H_
/*------------------------------------------------------------------------------*/
/* hook																			*/
/*------------------------------------------------------------------------------*/
#include "hook_test.h"
#include "hook_impose.h"
#include "hook_ctrl.h"
#include "hook_display.h"
#include "hook_usb.h"
#include "hook_interrupt.h"

#include <pspsdk.h>
/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern void *HookNidAddress( SceModule *mod, char *libname, u32 nid );
extern void *HookSyscallAddress( void *addr );
extern void HookFuncSetting( void *addr, void *entry );

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

//by Davee
#define HIJACK_FUNCTION(a, f, ptr) \
{ \
	u32 func = a; \
	static u32 patch_buffer[3]; \
	_sw(_lw(func), (u32)patch_buffer); \
	_sw(_lw(func + 4), (u32)patch_buffer + 8);\
	MAKE_JUMP((u32)patch_buffer + 4, func + 8); \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), func); \
	_sw(0, func + 4); \
	ptr = (void *)patch_buffer; \
}

u32 sctrlHENFindFunction(const char *, const char*, u32);

#endif	// _HOOK_H_
