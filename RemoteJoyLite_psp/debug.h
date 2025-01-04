#ifndef _DEBUG_H_
#define _DEBUG_H_
/*------------------------------------------------------------------------------*/
/* debug																		*/
/*------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <pspdisplay.h>
#include <pspiofilemgr.h>

#include "../remotejoy.h"

#define DEBUG_BUFF_SIZE         400

#define debug_printf(...) { \
	char _tmp[DEBUG_BUFF_SIZE]; \
	int _intc = pspSdkDisableInterrupts(); \
	sprintf(_tmp, __VA_ARGS__); \
	if ( strlen(DebugData.buff) + strlen(_tmp) < DEBUG_BUFF_SIZE ){ \
		strcat( DebugData.buff, _tmp ); \
	} \
	pspSdkEnableInterrupts( _intc ); \
}

#ifndef IS_SELF
extern struct {
	struct JoyScrHeader head;
	char                buff[DEBUG_BUFF_SIZE];
} DebugData;

#endif

#define LOG_PATH "ms0:/PSP/rjl_early_log.txt"

#ifndef RELEASE
#define EARLY_LOG(...) { \
	if(early_log_fd < 0){ \
		early_log_fd = sceIoOpen(LOG_PATH, PSP_O_APPEND | PSP_O_CREAT | PSP_O_WRONLY, 00777); \
	} \
	if(early_log_fd >= 0) { \
		char _buf[512] = {0}; \
		char _len = sprintf(_buf, __VA_ARGS__); \
		sceIoWrite(early_log_fd, _buf, _len); \
		sceIoClose(early_log_fd); \
		early_log_fd = -1; \
	} \
}
#else
#define EARLY_LOG(...)
#endif

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern void DebugTrance( int flag );


extern int early_log_fd;
void early_log_init();

#endif	// _DEBUG_H_
