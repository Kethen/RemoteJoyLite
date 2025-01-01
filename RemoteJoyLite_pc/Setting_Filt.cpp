#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting_Filt																	*/
/*------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Setting.h"
#include "RemoteJoyLite.h"
#include "WinMain.h"
#include "Setting_Filt.h"
#include "Setting_Filt.dat"

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static HINSTANCE hInstParent;
static HWND GammaChk;
static HWND ALLGammaTxt;
static HWND ALLGammaLst;
static HWND RGBGammaTxt;
static HWND RGBGammaRLst;
static HWND RGBGammaGLst;
static HWND RGBGammaBLst;

/*------------------------------------------------------------------------------*/
/* WmDestroyFiltConf															*/
/*------------------------------------------------------------------------------*/
void WmDestroyFiltConf( HWND hWnd )
{
	DestroyWindow( GammaChk );

	if ( SettingData.GammaRGB == 0 ){
		DestroyWindow( ALLGammaTxt );
		DestroyWindow( ALLGammaLst );
	} else {
		DestroyWindow( RGBGammaTxt );
		DestroyWindow( RGBGammaRLst );
		DestroyWindow( RGBGammaGLst );
		DestroyWindow( RGBGammaBLst );
	}
	WmDestroySaveButton( hWnd );
}

/*------------------------------------------------------------------------------*/
/* WmCreateGammaALL																*/
/*------------------------------------------------------------------------------*/
static void WmCreateGammaALL( HWND hWnd, HINSTANCE hInst )
{
	WCHAR buff[16];

	ALLGammaTxt = MyCreateWindow( &CWD_ALLGammaTxt, hWnd, hInst );
	ALLGammaLst = MyCreateWindow( &CWD_ALLGammaLst, hWnd, hInst );
	for ( int i=10; i<=500; i++ ){
		wsprintf( buff, L"%d.%02d", i/100, i%100 );
		SendMessage( ALLGammaLst, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( ALLGammaLst, CB_SETCURSEL, SettingData.Gamma[3] - 10, 0 );
}

/*------------------------------------------------------------------------------*/
/* WmCreateGammaRGB																*/
/*------------------------------------------------------------------------------*/
static void WmCreateGammaRGB( HWND hWnd, HINSTANCE hInst )
{
	WCHAR buff[16];

	RGBGammaTxt  = MyCreateWindow( &CWD_RGBGammaTxt,  hWnd, hInst );
	RGBGammaRLst = MyCreateWindow( &CWD_RGBGammaRLst, hWnd, hInst );
	RGBGammaGLst = MyCreateWindow( &CWD_RGBGammaGLst, hWnd, hInst );
	RGBGammaBLst = MyCreateWindow( &CWD_RGBGammaBLst, hWnd, hInst );
	for ( int i=10; i<=500; i++ ){
		wsprintf( buff, L"%d.%02d", i/100, i%100 );
		SendMessage( RGBGammaRLst, CB_ADDSTRING, 0, (LPARAM)buff );
		SendMessage( RGBGammaGLst, CB_ADDSTRING, 0, (LPARAM)buff );
		SendMessage( RGBGammaBLst, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( RGBGammaRLst, CB_SETCURSEL, SettingData.Gamma[0] - 10, 0 );
	SendMessage( RGBGammaGLst, CB_SETCURSEL, SettingData.Gamma[1] - 10, 0 );
	SendMessage( RGBGammaBLst, CB_SETCURSEL, SettingData.Gamma[2] - 10, 0 );
}

/*------------------------------------------------------------------------------*/
/* WmCreateFiltConf																*/
/*------------------------------------------------------------------------------*/
void WmCreateFiltConf( HWND hWnd, HINSTANCE hInst )
{
	hInstParent = hInst;

	GammaChk = MyCreateWindow( &CWD_GammaChk, hWnd, hInst );
	if ( SettingData.GammaRGB == 0 ){
		WmCreateGammaALL( hWnd, hInst );
	} else {
		WmCreateGammaRGB( hWnd, hInst );
		SendMessage( GammaChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	WmCreateSaveButton( hWnd );
	InvalidateRect( hWnd, NULL, TRUE );
}

/*------------------------------------------------------------------------------*/
/* WmCommandFiltConf															*/
/*------------------------------------------------------------------------------*/
void WmCommandFiltConf( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	switch ( id ){
	case 400:
		if ( SendMessage( GammaChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.GammaRGB = 1;
			DestroyWindow( ALLGammaTxt );
			DestroyWindow( ALLGammaLst );
			WmCreateGammaRGB( hWnd, hInstParent );
			InvalidateRect( hWnd, NULL, TRUE );
			RemoteJoyLite_CalcGammaTable();
		} else {
			SettingData.GammaRGB = 0;
			DestroyWindow( RGBGammaTxt );
			DestroyWindow( RGBGammaRLst );
			DestroyWindow( RGBGammaGLst );
			DestroyWindow( RGBGammaBLst );
			WmCreateGammaALL( hWnd, hInstParent );
			InvalidateRect( hWnd, NULL, TRUE );
			RemoteJoyLite_CalcGammaTable();
		}
		break;
	case 411:
		SettingData.Gamma[3] = SendMessage( ALLGammaLst, CB_GETCURSEL, 0, 0 ) + 10;
		RemoteJoyLite_CalcGammaTable();
		break;
	case 421:
		SettingData.Gamma[0] = SendMessage( RGBGammaRLst, CB_GETCURSEL, 0, 0 ) + 10;
		RemoteJoyLite_CalcGammaTable();
		break;
	case 422:
		SettingData.Gamma[1] = SendMessage( RGBGammaGLst, CB_GETCURSEL, 0, 0 ) + 10;
		RemoteJoyLite_CalcGammaTable();
		break;
	case 423:
		SettingData.Gamma[2] = SendMessage( RGBGammaBLst, CB_GETCURSEL, 0, 0 ) + 10;
		RemoteJoyLite_CalcGammaTable();
		break;
	}
}
