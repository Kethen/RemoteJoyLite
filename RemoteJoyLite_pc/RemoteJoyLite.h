#ifndef _REMOTEJOYLITE_H_
#define _REMOTEJOYLITE_H_
/*------------------------------------------------------------------------------*/
/* RemoteJoyLite																*/
/*------------------------------------------------------------------------------*/
#include "Direct3D.h"

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern void RemoteJoyLite_CalcGammaTable( void );
extern BOOL RemoteJoyLiteInit( AkindD3D *pAkindD3D );
extern void RemoteJoyLiteExit( void );
extern void RemoteJoyLiteDraw( AkindD3D *pAkindD3D );
extern void RemoteJoyLiteSync( void );
extern void RemoteJoyLite_ToggleDebug( void );
extern void RemoteJoyLite_ToggleDisp( void );
extern void RemoteJoyLite_SendPSPCmd( void );
extern void RemoteJoyLite_SetButton( int button );
extern void RemoteJoyLite_SetAxis( int x, int y );
extern void RemoteJoyLite_SetAxisR( int x, int y );
extern int RemoteJoyLite_GetButton( void );
extern int RemoteJoyLite_GetAxisXY( void );
extern void RemoteJoyLite_SaveBitmap( void );
extern void RemoteJoyLite_SaveMovie( void );
extern BOOL RemoteJoyLite_CheckMovie( void );

#endif	// _REMOTEJOYLITE_H_
