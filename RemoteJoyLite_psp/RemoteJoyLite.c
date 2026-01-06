/*------------------------------------------------------------------------------*/
/* RemoteJoyLite																*/
/*------------------------------------------------------------------------------*/
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <pspctrl_kernel.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspdisplay_kernel.h>
#include <psputilsforkernel.h>
#include <pspsysmem_kernel.h>
#include <pspimpose_driver.h>
#include <string.h>
#include <stdio.h>
#include "../remotejoy.h"
#include "usb.h"
#include "hook.h"
#include "kmode.h"
#include "debug.h"

SceModule *pops = NULL;

/*------------------------------------------------------------------------------*/
/* module info																	*/
/*------------------------------------------------------------------------------*/
PSP_MODULE_INFO( "RemoteJoyLite", PSP_MODULE_KERNEL, 1, 1 );

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern int sceDisplayEnable( void );
extern int sceDisplayDisable( void );

/*------------------------------------------------------------------------------*/
/* DisplayEnable																*/
/*------------------------------------------------------------------------------*/
static int BrightLevel = 68;

void DisplayEnable( void )
{
	int k1 = pspSdkSetK1( 0 );
	int NowBright = 68;
	sceDisplayGetBrightness( &NowBright, 0 );
	if ( NowBright != 0 ){ BrightLevel = NowBright; }
	sceDisplaySetBrightness( BrightLevel, 0 );
	sceDisplayEnable();
	pspSdkSetK1( k1 );
}

/*------------------------------------------------------------------------------*/
/* DisplayDisable																*/
/*------------------------------------------------------------------------------*/
void DisplayDisable( void )
{
	int k1 = pspSdkSetK1( 0 );
	int NowBright = 68;
	sceDisplayGetBrightness( &NowBright, 0 );
	if ( NowBright != 0 ){ BrightLevel = NowBright; }
	sceDisplaySetBrightness( 0, 0 );
	sceDisplayDisable();
	pspSdkSetK1( k1 );
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite work															*/
/*------------------------------------------------------------------------------*/
#ifndef RELEASE
extern int sceDisplayGetAccumulatedHcount( void );
static int HcountMkFrameTop = 0;
static int HcountMkFrameSub = 0;
int GetHcountMkFrame( void ){ return( HcountMkFrameSub ); }
extern int GetHcountUsbWait();
#endif
static int TranceMode  = 0;
static int TranceFPS   = 0;
int DebugMode   = 0;
static int ScreenReady = 0;
static int LineFlag = 0;
static void *ScreenBuff = NULL;
static int ScreenX = 0;
static int ScreenY = 0;
static int ScreenW = 0;
static int ScreenH = 0;
static SceUID MainThreadID = -1;
static struct JoyScrHeader ScreenProbeCmd = { JOY_MAGIC, -1, 0, 0 };
static SceUID ScreenThreadID = -1;
static SceUID ScreenSemaphore = -1;
static SceUID ScreenEventFlag = -1;

/*------------------------------------------------------------------------------*/
/* SendDispEvent																*/
/*------------------------------------------------------------------------------*/
static int PreVcount = 0;

void SendDispEvent( void )
{
	unsigned int k1;

	if ( ScreenReady == 0 ){ return; }
	int NowVcount = sceDisplayGetVcount();
	int SubVcount = NowVcount - PreVcount;

	if ( (u32)SubVcount <= (u32)TranceFPS ){ return; }
	k1 = psplinkSetK1( 0 );
	if ( sceKernelPollSema( ScreenSemaphore, 1 ) == 0 ){
		sceKernelSetEventFlag( ScreenEventFlag, 1 );
	}
	psplinkSetK1( k1 );
	PreVcount = NowVcount;
}

/*------------------------------------------------------------------------------*/
/* BuildFrame																	*/
/*------------------------------------------------------------------------------*/
static void BuildFrame( void )
{
	void *src;
	int pitch, orgfmt, sync, size = 0;
	struct JoyScrHeader *joy = ScreenBuff;

	sceDisplayGetFrameBufferInternal( 0, &src, &pitch, &orgfmt, &sync );
	if ( src == NULL ){
		sceDisplayGetFrameBufferInternal( 2, &src, &pitch, &orgfmt, &sync );
	}
	if ( src == NULL ){ return; }
	if ( joy == NULL ){ return; }

	int newfmt = (orgfmt << 4);
	int tmode  = TranceMode;
	void *dst  = ScreenBuff + sizeof(*joy);

	int ofs = pitch*ScreenY + ScreenX;
	int end = pitch*ScreenH;

	if ( (tmode == 0) || (tmode == 1) ){
		asm __volatile__ ( "move $t8, %0" ::"r"(pitch-ScreenW):"t8" );
		asm __volatile__ ( "move $t9, %0" ::"r"(ScreenW):"t9" );
		if ( orgfmt != 3 ){
			size = Copy16bpp( src, dst, ofs, end );
		} else {
			if ( tmode == 0 ){
				size = Copy32bpp( src, dst, ofs, end );
			} else {
				size = Cmp888565( src, dst, ofs, end );
				newfmt = 0;
			}
		}
	}

	if ( (tmode == 2) || (tmode == 3) ){
		int flag = 0;
		if ( tmode == 2 ){ LineFlag ^= 1; flag |= 0x200; }
		else			 { LineFlag  = 0; flag |= 0x400; }
		if ( LineFlag & 1 ){
			if ( orgfmt != 3 ){ src = (void *)((u32)src + pitch*2); }
			else			  { src = (void *)((u32)src + pitch*4); }
			flag |= 0x100;
		}
		asm __volatile__ ( "move $t8, %0" ::"r"(pitch*2-ScreenW):"t8" );
		asm __volatile__ ( "move $t9, %0" ::"r"(ScreenW):"t9" );
		if ( orgfmt != 3 ){
			size = Copy16bpp( src, dst, ofs, end );
		} else {
			size = Cmp888565( src, dst, ofs, end );
			newfmt = 0;
		}
		newfmt |= flag;
	}
	if ( tmode == 4 ){
		int pixsize = 0;
		if(orgfmt != 3){
			pixsize = 2;
		}else{
			pixsize = 4;
		}

		#ifndef RELEASE
		// dump in C for debugging

		size = ScreenW * ScreenH * pixsize;
		int src_line_offset = ScreenX * pixsize;
		int dst_line_size = ScreenW * pixsize;
		int src_line_size = pitch * pixsize;
		src = (void *)((u32)src + ScreenY * pitch * pixsize);
		for(int i = 0;i < ScreenH;i++){
			memcpy(dst, (void *)((u32)src + src_line_offset), dst_line_size);
			dst = (void *)((u32)dst + dst_line_size);
			src = (void *)((u32)src + src_line_size);
		}

		#else

		// transfer speed test
		size = 480 * 272 * pixsize;

		#endif
	}

	joy->magic = JOY_MAGIC;
	joy->mode  = orgfmt | newfmt;
	joy->size  = size;
	joy->ref   = sceDisplayGetVcount();
	UsbWriteBulkData( joy, sizeof(*joy) + size );
}

/*------------------------------------------------------------------------------*/
/* ScreenThread																	*/
/*------------------------------------------------------------------------------*/
static int ScreenThread( SceSize args, void *argp )
{
	_sw( 0xFFFFFFFF, 0xBC00000C );

	ScreenSemaphore = sceKernelCreateSema( "ScreenSema", 0, 1, 1, NULL );
	if ( ScreenSemaphore < 0 ){ sceKernelExitDeleteThread( 0 ); }

	BuildFrame();

	while ( 1 ){
		u32 result;
		SceUInt timeout = 100000;

		int ret = sceKernelWaitEventFlag( ScreenEventFlag, 3,
										  PSP_EVENT_WAITOR | PSP_EVENT_WAITCLEAR, &result, &timeout );
		if ( (ret < 0) && (ret != SCE_KERNEL_ERROR_WAIT_TIMEOUT) ){
			sceKernelExitDeleteThread( 0 );
		}
		if ( ret == SCE_KERNEL_ERROR_WAIT_TIMEOUT ){ continue; }
		if ( (result & 1) || (ret == SCE_KERNEL_ERROR_WAIT_TIMEOUT) ){
			_sw( 0xFFFFFFFF, 0xBC00000C );
#ifndef RELEASE
			if(DebugMode){
				HcountMkFrameTop = sceDisplayGetAccumulatedHcount();
			}
#endif
			BuildFrame();
#ifndef RELEASE
			if(DebugMode){
				HcountMkFrameSub = sceDisplayGetAccumulatedHcount() - HcountMkFrameTop;
			}
#endif
			if ( ret != SCE_KERNEL_ERROR_WAIT_TIMEOUT ){
				sceKernelSignalSema( ScreenSemaphore, 1 );
			}
		}
		if ( result & 2 ){ sceKernelSleepThread(); }
	}
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* DoJoyCmd																		*/
/*------------------------------------------------------------------------------*/
static void DoJoyCmd( unsigned int value1, unsigned int value2 )
{
	int Priority = SCREEN_CMD_GET_PRIORITY(value1);
	if ( MainThreadID   >= 0 ){ sceKernelChangeThreadPriority( MainThreadID,   Priority+0 ); }
	if ( ScreenThreadID >= 0 ){ sceKernelChangeThreadPriority( ScreenThreadID, Priority+1 ); }
	if ( value1 & SCREEN_CMD_SCROFF ){ DisplayDisable(); }
	else							 { DisplayEnable();  }
	if ( value1 & SCREEN_CMD_DEBUG  ){ DebugMode = 1;    }
	else							 { DebugMode = 0;    }
	if( value1 & SCREEN_CMD_ASYNC ){
		TranceAsyncOn();
	}else{
		TranceAsyncOff();
	}
	TranceMode = SCREEN_CMD_GET_TRNSMODE(value1);
	TranceFPS  = SCREEN_CMD_GET_TRNSFPS(value1);

	int idx1 = SCREEN_CMD_GET_ADRESS1(value1);
	int idx2 = SCREEN_CMD_GET_ADRESS2(value1);
	if ( (sceKernelGetModel() != 0) && (idx2 != 0xFF) ){
		ScreenBuff = (void *)(0xAA000000 + idx2*0x40000);
	} else {
		ScreenBuff = (void *)(0x48400000 + idx1*0x8000);
	}

	ScreenX = SCREEN_CMD_GET_TRNSX(value2)*8;
	ScreenY = SCREEN_CMD_GET_TRNSY(value2);
	ScreenW = SCREEN_CMD_GET_TRNSW(value2)*32;
	ScreenH = SCREEN_CMD_GET_TRNSH(value2)*2;

	if ( value1 & SCREEN_CMD_ACTIVE ){
		if ( ScreenThreadID < 0 ){
			ScreenEventFlag = sceKernelCreateEventFlag( "ScreenEvent", 0, 0, NULL );
			if ( ScreenEventFlag < 0 ){ return; }
			ScreenThreadID = sceKernelCreateThread( "ScreenThread", ScreenThread, Priority+1, 0x800, PSP_THREAD_ATTR_VFPU, NULL );
			if ( ScreenThreadID >= 0 ){ sceKernelStartThread( ScreenThreadID, 0, NULL ); }
			ScreenReady = 1;
		} else {
			if ( ScreenReady == 0 ){
				sceKernelWakeupThread( ScreenThreadID );
				ScreenReady = 1;
			}
		}
	} else {
		if ( ScreenThreadID >= 0 ){
			if ( ScreenReady == 1 ){
				sceKernelSetEventFlag( ScreenEventFlag, 2 );
				ScreenReady = 0;
			}
		}
	}
}

/*------------------------------------------------------------------------------*/
/* DoJoyDat																		*/
/*------------------------------------------------------------------------------*/
u32 PreButton = 0;

static void DoJoyDat( u32 NowButton, u32 value2 )
{
	int intc = pspSdkDisableInterrupts();
	hookCtrlSetData( PreButton, NowButton, value2 );
	pspSdkEnableInterrupts( intc );

	unsigned int trig = (NowButton & ~PreButton) & NowButton;
	if ( trig & PSP_CTRL_HOME ){
		if(!pops){
			sceImposeHomeButton(sceImposeGetStatus() & 1);
		}
	}
	PreButton = NowButton;
}

/*------------------------------------------------------------------------------*/
/* MainThread																	*/
/*------------------------------------------------------------------------------*/
static int MainThread( SceSize args, void *argp )
{
	if(!usbModuleLoaded()){
		sceKernelDelayThread(100000);
		EARLY_LOG("%s: loading usb module\n", __func__);
		loadUsbModule();

		sceKernelDelayThread(100000);
		EARLY_LOG("%s: hooking usb\n", __func__);
		hookUsbFunc();
	}

	// don't usb yet
	sceKernelDelayThread(1000000 * 3);

	EARLY_LOG("%s: registering usb driver\n", __func__);
	UsbbdRegister();
	EARLY_LOG("%s: starting usb\n", __func__);
	if(UsbStart() != 0){
		EARLY_LOG("%s: failed starting usb\n", __func__);
		return 0;
	}

	// really really clean the usb state
	EARLY_LOG("%s: rebooting usb\n", __func__);
	UsbSuspend();
	UsbResume();

	sceKernelDcacheWritebackInvalidateAll();
	sceKernelIcacheInvalidateAll();

	pops = sceKernelFindModuleByName("scePops_Manager");

	UsbAsyncFlush();
	if ( UsbWait() != 0 ){ return( 0 ); }

	UsbAsyncWrite( &ScreenProbeCmd, sizeof(ScreenProbeCmd) );

	while ( 1 ){
		struct JoyEvent joyevent;
		int len = UsbAsyncRead( (void *)&joyevent, sizeof(joyevent) );

		if ( (len != sizeof(joyevent)) || (joyevent.magic != JOY_MAGIC) ){
			if ( len < 0 ){
				sceKernelDelayThread( 250000 );
			} else {
				UsbAsyncFlush();
			}
			continue;
		}
#ifndef RELEASE
		if(DebugMode){
			int HcountComp = GetHcountMkFrame() - GetHcountUsbWait();
			debug_printf( "%d\n", HcountComp  );
			debug_printf( "%d\n", GetHcountUsbWait() );
			debug_printf("0x%x\n", ScreenBuff);
		}
		DebugTrance( 1 );
#endif
		if ( joyevent.type == TYPE_JOY_DAT ){
			DoJoyDat( joyevent.value1, joyevent.value2 );
		}
		if ( joyevent.type == TYPE_JOY_CMD ){
			DoJoyCmd( joyevent.value1, joyevent.value2 );
		}
		scePowerTick( 0 );
	}
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* module_start																	*/
/*------------------------------------------------------------------------------*/
int module_start( SceSize args, void *argp )
{
	#ifndef RELEASE
	early_log_init();
	EARLY_LOG("%s: begin\n", __func__);
	#endif

	if (usbModuleLoaded()){
		EARLY_LOG("%s: hooking usb\n", __func__);
		hookUsbFunc();
	}

	EARLY_LOG("%s: hooking interrupt\n", __func__);
	hookInterrupt();

	EARLY_LOG("%s: hooking ctrl buffer functions\n", __func__);
	hookCtrlBuffer();

	EARLY_LOG("%s: hooking ctrl latch functions\n", __func__);
	hookCtrlLatch();

	EARLY_LOG("%s: hooking display functions\n", __func__);
	hookDisplay();

	if ( sceKernelDevkitVersion() >= 0x01050001 ){
		u32 *p = (u32 *)sceKernelSetDdrMemoryProtection;
		u32 addr = GET_JUMP_TARGET( *p );
		_sw( 0x03E00008, addr+0 );		// jr	$ra
		_sw( 0x00001021, addr+4 );		// move	$v0, $zr
		sceKernelDcacheWritebackInvalidateRange( (void *)addr, 8 );
		sceKernelIcacheInvalidateRange( (void *)addr, 8 );
	}

	MainThreadID = sceKernelCreateThread( "RemoteJoyLite", MainThread, 16, 0x1000, 0, NULL );
	if ( MainThreadID >= 0 ){ sceKernelStartThread( MainThreadID, args, argp ); }
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* module_stop																	*/
/*------------------------------------------------------------------------------*/
int module_stop( SceSize args, void *argp )
{
	if ( ScreenThreadID  >= 0 ){ sceKernelDeleteThread( ScreenThreadID ); }
	if ( ScreenEventFlag >= 0 ){ sceKernelDeleteEventFlag( ScreenEventFlag ); }
	if ( ScreenSemaphore >= 0 ){ sceKernelDeleteSema( ScreenSemaphore ); }
	if ( MainThreadID    >= 0 ){ sceKernelDeleteThread( MainThreadID ); }

	UsbbdUnregister();
	return( 0 );
}
