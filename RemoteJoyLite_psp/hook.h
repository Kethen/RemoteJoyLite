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
#define GET_JUMP_TARGET(x) (0x80000000 | ((((u32)(x)) & 0x03FFFFFF) << 2))

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


#define MAKE_JUMP_PATCH(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);

#define NOP 0
//by Davee
#define HIJACK_FUNCTION(a, f, p) \
{ \
	int _interrupts = pspSdkDisableInterrupts(); \
	static u32 _pb_[5]; \
	_sw(_lw((u32)(a)), (u32)_pb_); \
	_sw(_lw((u32)(a) + 4), (u32)_pb_ + 4);\
	_sw(NOP, (u32)_pb_ + 8);\
	_sw(NOP, (u32)_pb_ + 16);\
	MAKE_JUMP_PATCH((u32)_pb_ + 12, (u32)(a) + 8); \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), (u32)(a)); \
	_sw(0, (u32)(a) + 4); \
	p = (void *)_pb_; \
	sceKernelDcacheWritebackInvalidateAll(); \
	sceKernelIcacheClearAll(); \
	pspSdkEnableInterrupts(_interrupts); \
}

#define HIJACK_FUNCTION_V1(a, f, ptr) \
{ \
	int _interrupts = pspSdkDisableInterrupts(); \
	u32 func = a; \
	static u32 patch_buffer[3]; \
	_sw(_lw(func), (u32)patch_buffer); \
	_sw(_lw(func + 4), (u32)patch_buffer + 8);\
	MAKE_JUMP_PATCH((u32)patch_buffer + 4, func + 8); \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), func); \
	_sw(0, func + 4); \
	ptr = (void *)patch_buffer; \
	sceKernelDcacheWritebackInvalidateAll(); \
	sceKernelIcacheClearAll(); \
	pspSdkEnableInterrupts(_interrupts); \
}

u32 sctrlHENFindFunction(const char *, const char*, u32);

#endif	// _HOOK_H_
