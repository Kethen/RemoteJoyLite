/*------------------------------------------------------------------------------*/
/* hook_impose																	*/
/*------------------------------------------------------------------------------*/
#include <pspkernel.h>
#include "debug.h"
#include "hook.h"

#if 0
/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static int (*sceImpose_Flag_Func)( void ) = NULL;
static void (*sceImposeHomeButton_Func)( int ) = NULL;

/*------------------------------------------------------------------------------*/
/* hookImposeHomeButton															*/
/*------------------------------------------------------------------------------*/
void hookImposeHomeButton( void )
{
	SceModule *module = sceKernelFindModuleByName( "sceImpose_Driver" );
	if ( module == NULL ){ return; }

	if ( sceImposeHomeButton_Func == NULL ){
		sceImposeHomeButton_Func = HookNidAddress( module, "sceImpose", 0x381BD9E7 );
	}
	if ( sceImposeHomeButton_Func == NULL ){ return; }

	if ( sceImpose_Flag_Func == NULL ){
		if ( sceKernelDevkitVersion() < 0x05000000 ){
			sceImpose_Flag_Func = HookNidAddress( module, "sceImpose_driver", 0x2EC0246D );
		} else {
			sceImpose_Flag_Func = HookNidAddress( module, "sceImpose_driver", 0x04BC5517 );
		}
	}
	if ( sceImpose_Flag_Func == NULL ){ return; }

	SceModule *popsman = sceKernelFindModuleByName( "scePops_Manager" );
	if ( popsman != NULL ){ return; }

 	if ( sceImpose_Flag_Func() & 1 ){
		sceImposeHomeButton_Func( 1 );
	} else {
		sceImposeHomeButton_Func( 0 );
	}
}

#endif
