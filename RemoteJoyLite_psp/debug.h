#ifndef _DEBUG_H_
#define _DEBUG_H_
/*------------------------------------------------------------------------------*/
/* debug																		*/
/*------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <pspdisplay.h>

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

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern void DebugTrance( int flag );

#endif	// _DEBUG_H_
