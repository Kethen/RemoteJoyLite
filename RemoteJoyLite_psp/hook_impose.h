#ifndef _HOOK_IMPOSE_H_
#define _HOOK_IMPOSE_H_
/*------------------------------------------------------------------------------*/
/* hook_impose																	*/
/*------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/

#if 0
extern void hookImposeHomeButton( void );
#endif

void sceImposeHomeButton(int open);
int sceImposeGetStatus();

#endif	// _HOOK_IMPOSE_H_
