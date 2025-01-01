#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting_Capt																	*/
/*------------------------------------------------------------------------------*/
#include "Setting.h"
#include "Bitmap.h"
#include "Setting_Capt.h"
#include "Setting_Capt.dat"

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static HWND JoyAviBox;
static HWND JoyAviChk;
static HWND JoyFpsConf;
static HWND JoyAviConf;
static HWND JoyWavBox;
static HWND JoyWavChk;

/*------------------------------------------------------------------------------*/
/* WmDestroyCaptConf															*/
/*------------------------------------------------------------------------------*/
void WmDestroyCaptConf( HWND hWnd )
{
	DestroyWindow( JoyAviBox );
	DestroyWindow( JoyAviChk );
	DestroyWindow( JoyFpsConf );
	DestroyWindow( JoyAviConf );
	DestroyWindow( JoyWavBox );
	DestroyWindow( JoyWavChk );
	WmDestroySaveButton( hWnd );
}

/*------------------------------------------------------------------------------*/
/* WmCreateCaptConf																*/
/*------------------------------------------------------------------------------*/
void WmCreateCaptConf( HWND hWnd, HINSTANCE hInst )
{
	WCHAR buff[16];

	JoyAviBox  = MyCreateWindow( &CWD_JoyAviBox,  hWnd, hInst );
	JoyAviChk  = MyCreateWindow( &CWD_JoyAviChk,  hWnd, hInst );
	JoyFpsConf = MyCreateWindow( &CWD_JoyFpsConf, hWnd, hInst );
	JoyAviConf = MyCreateWindow( &CWD_JoyAviConf, hWnd, hInst );
	if ( SettingData.AVIOut != 0 ){
		SendMessage( JoyAviChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	} else {
		EnableWindow( JoyFpsConf, FALSE );
		EnableWindow( JoyAviConf, FALSE );
	}
	for ( int i=0; i<6; i++ ){
		wsprintf( buff, L" %d FPS", 60/(i+1) );
		SendMessage( JoyFpsConf, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( JoyFpsConf, CB_SETCURSEL, SettingData.MovieFPS, 0 );

	JoyWavBox = MyCreateWindow( &CWD_JoyWavBox, hWnd, hInst );
	JoyWavChk = MyCreateWindow( &CWD_JoyWavChk, hWnd, hInst );
	if ( SettingData.WAVOut != 0 ){
		SendMessage( JoyWavChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	WmCreateSaveButton( hWnd );
	InvalidateRect( hWnd, NULL, TRUE );
}

/*------------------------------------------------------------------------------*/
/* WmCommandCaptConf															*/
/*------------------------------------------------------------------------------*/
void WmCommandCaptConf( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	switch ( id ){
	case 501:
		if ( SendMessage( JoyAviChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.AVIOut = 1;
			EnableWindow( JoyFpsConf, TRUE );
			EnableWindow( JoyAviConf, TRUE );
		} else {
			SettingData.AVIOut = 0;
			EnableWindow( JoyFpsConf, FALSE );
			EnableWindow( JoyAviConf, FALSE );
		}
		break;
	case 502:
		SettingData.MovieFPS = SendMessage( JoyFpsConf, CB_GETCURSEL, 0, 0 );
		break;
	case 503:
		SettingData.Movie.dwFlags = ICMF_COMPVARS_VALID;
		ICCompressorChoose( hWnd, ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME,
							&PspBitmapBase, NULL, &SettingData.Movie, NULL );
		ICCompressorFree( &SettingData.Movie );
		break;
	case 511:
		if ( SendMessage( JoyWavChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.WAVOut = 1;
		} else {
			SettingData.WAVOut = 0;
		}
		break;
	}
}
