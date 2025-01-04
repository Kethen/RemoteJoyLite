#ifndef _HOOK_USB_H_
#define _HOOK_USB_H_
/*------------------------------------------------------------------------------*/
/* hook_usb																		*/
/*------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern void hookUsbFunc( void );

#ifndef IS_SELF
extern int (*sceUsbStart_Func)( const char *, unsigned int, void * );
extern int (*sceUsbStop_Func)( const char *, unsigned int, void * );
#endif

#endif	// _HOOK_USB_H_
