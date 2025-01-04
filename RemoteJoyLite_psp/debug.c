/*------------------------------------------------------------------------------*/
/* debug																		*/
/*------------------------------------------------------------------------------*/
#include <pspsdk.h>
#include "../remotejoy.h"
#include "usb.h"
#define IS_SELF
#include "debug.h"
#undef IS_SELF

#include <pspiofilemgr.h>

int early_log_fd = -1;

#ifndef RELEASE
/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
struct {
	struct JoyScrHeader	head;
	char				buff[DEBUG_BUFF_SIZE];
} DebugData = {0};

/*------------------------------------------------------------------------------*/
/* DebugTrance																	*/
/*------------------------------------------------------------------------------*/
void DebugTrance( int flag )
{
	if ( flag != 0 && DebugData.buff[0] != 0 ){
		DebugData.head.magic = JOY_MAGIC;
		DebugData.head.mode  = ASYNC_CMD_DEBUG;
		DebugData.head.ref   = 0;
		DebugData.head.size  = DEBUG_BUFF_SIZE;
		UsbAsyncWrite( &DebugData, sizeof(DebugData) );
		DebugData.buff[0] = 0;
	}
}
#endif

void early_log_init(){
	early_log_fd = sceIoOpen(LOG_PATH, PSP_O_TRUNC | PSP_O_CREAT | PSP_O_WRONLY, 00777);
}
