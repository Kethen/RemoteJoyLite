#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting_Etc																	*/
/*------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Setting.h"
#include "WinMain.h"
#include "Setting_Etc.h"
#include "Setting_Etc.dat"

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern "C" { int _wtoi (const WCHAR *); };

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static HWND DispAspeChk;
static HWND DispTopChk;
static HWND FullAdjChk;
static HWND FullRectTxt;
static HWND FullRectXLst;
static HWND FullRectYLst;
static HWND FullRectWLst;
static HWND FullRectHLst;
static HWND RebootBox;
static HWND DispSizeTxt;
static HWND DispSizeLst;
static HWND DispRotTxt;
static HWND DispRotLst;
static HWND DispRectTxt;
static HWND DispRectXLst;
static HWND DispRectYLst;
static HWND DispRectWLst;
static HWND DispRectHLst;
static HWND DispOffChk;
static HWND InputBGChk;

/*------------------------------------------------------------------------------*/
/* WmDestroyEtcConf																*/
/*------------------------------------------------------------------------------*/
void WmDestroyEtcConf( HWND hWnd )
{
	DestroyWindow( DispAspeChk );
	DestroyWindow( DispTopChk );
	DestroyWindow( FullAdjChk );
	DestroyWindow( FullRectTxt );
	DestroyWindow( FullRectXLst );
	DestroyWindow( FullRectYLst );
	DestroyWindow( FullRectWLst );
	DestroyWindow( FullRectHLst );
	DestroyWindow( RebootBox );
	DestroyWindow( DispSizeTxt );
	DestroyWindow( DispSizeLst );
	DestroyWindow( DispRotTxt );
	DestroyWindow( DispRotLst );
	DestroyWindow( DispRectTxt );
	DestroyWindow( DispRectXLst );
	DestroyWindow( DispRectYLst );
	DestroyWindow( DispRectWLst );
	DestroyWindow( DispRectHLst );
	DestroyWindow( DispOffChk );
	DestroyWindow( InputBGChk );
	WmDestroySaveButton( hWnd );
}

/*------------------------------------------------------------------------------*/
/* WmCreateEtcConf																*/
/*------------------------------------------------------------------------------*/
void WmCreateEtcConf( HWND hWnd, HINSTANCE hInst )
{
	DispAspeChk = MyCreateWindow( &CWD_DispAspeChk, hWnd, hInst );
	if ( SettingData.DispAspe != 0 ){
		SendMessage( DispAspeChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	DispTopChk = MyCreateWindow( &CWD_DispTopChk, hWnd, hInst );
	if ( SettingData.DispTop != 0 ){
		SendMessage( DispTopChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	FullAdjChk = MyCreateWindow( &CWD_FullAdjChk, hWnd, hInst );
	if ( SettingData.FullAdjust != 0 ){
		SendMessage( FullAdjChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	FullRectTxt  = MyCreateWindow( &CWD_FullRectTxt,  hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.FullRectX );
	FullRectXLst = MyCreateWindow( &CWD_FullRectXLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.FullRectY );
	FullRectYLst = MyCreateWindow( &CWD_FullRectYLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.FullRectW );
	FullRectWLst = MyCreateWindow( &CWD_FullRectWLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.FullRectH );
	FullRectHLst = MyCreateWindow( &CWD_FullRectHLst, hWnd, hInst );

	if ( SettingData.FullAdjust == 0 ){
		EnableWindow( FullRectXLst, FALSE );
		EnableWindow( FullRectYLst, FALSE );
		EnableWindow( FullRectWLst, FALSE );
		EnableWindow( FullRectHLst, FALSE );
	}

	RebootBox = MyCreateWindow( &CWD_RebootBox, hWnd, hInst );

	DispSizeTxt = MyCreateWindow( &CWD_DispSizeTxt, hWnd, hInst );
	DispSizeLst = MyCreateWindow( &CWD_DispSizeLst, hWnd, hInst );
	for ( int i=0; i<3; i++ ){
		SendMessage( DispSizeLst, CB_ADDSTRING, 0, (LPARAM)DispSize[i] );
	}
	SendMessage( DispSizeLst, CB_SETCURSEL, SettingData.DispSize, 0 );

	DispRotTxt = MyCreateWindow( &CWD_DispRotTxt, hWnd, hInst );
	DispRotLst = MyCreateWindow( &CWD_DispRotLst, hWnd, hInst );
	for ( int i=0; i<4; i++ ){
		SendMessage( DispRotLst, CB_ADDSTRING, 0, (LPARAM)DispRot[i] );
	}
	SendMessage( DispRotLst, CB_SETCURSEL, SettingData.DispRot, 0 );

	DispRectTxt  = MyCreateWindow( &CWD_DispRectTxt,  hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.DispRectX );
	DispRectXLst = MyCreateWindow( &CWD_DispRectXLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.DispRectY );
	DispRectYLst = MyCreateWindow( &CWD_DispRectYLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.DispRectW );
	DispRectWLst = MyCreateWindow( &CWD_DispRectWLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.DispRectH );
	DispRectHLst = MyCreateWindow( &CWD_DispRectHLst, hWnd, hInst );

	DispOffChk = MyCreateWindow( &CWD_DispOffChk, hWnd, hInst );
	if ( SettingData.DispOff != 0 ){
		SendMessage( DispOffChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	InputBGChk = MyCreateWindow( &CWD_InputBGChk, hWnd, hInst );
	if ( SettingData.InputBG != 0 ){
		SendMessage( InputBGChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	WmCreateSaveButton( hWnd );
	InvalidateRect( hWnd, NULL, TRUE );
}

/*------------------------------------------------------------------------------*/
/* WmCommandEtcConf																*/
/*------------------------------------------------------------------------------*/
void WmCommandEtcConf( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	WCHAR buff[16];

	switch ( id ){
	case 300:
		if ( SendMessage( DispAspeChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.DispAspe = 1;
			ChangeAspect();
		} else {
			SettingData.DispAspe = 0;
			ChangeAspect();
		}
		break;
	case 301:
		if ( SendMessage( DispTopChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.DispTop = 1;
			ChangeDispTop();
		} else {
			SettingData.DispTop = 0;
			ChangeDispTop();
		}
		break;

	case 302:
		if ( SendMessage( FullAdjChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.FullAdjust = 1;
			EnableWindow( FullRectXLst, TRUE );
			EnableWindow( FullRectYLst, TRUE );
			EnableWindow( FullRectWLst, TRUE );
			EnableWindow( FullRectHLst, TRUE );
			ChangeAspect();
		} else {
			SettingData.FullAdjust = 0;
			EnableWindow( FullRectXLst, FALSE );
			EnableWindow( FullRectYLst, FALSE );
			EnableWindow( FullRectWLst, FALSE );
			EnableWindow( FullRectHLst, FALSE );
			ChangeAspect();
		}
		break;
	case 304:
		if ( GetWindowText( FullRectXLst, buff, 16 ) > 0 ){
			SettingData.FullRectX = _wtoi( buff );
			ChangeAspect();
		}
		break;
	case 305:
		if ( GetWindowText( FullRectYLst, buff, 16 ) > 0 ){
			SettingData.FullRectY = _wtoi( buff );
			ChangeAspect();
		}
		break;
	case 306:
		if ( GetWindowText( FullRectWLst, buff, 16 ) > 0 ){
			SettingData.FullRectW = _wtoi( buff );
			ChangeAspect();
		}
		break;
	case 307:
		if ( GetWindowText( FullRectHLst, buff, 16 ) > 0 ){
			SettingData.FullRectH = _wtoi( buff );
			ChangeAspect();
		}
		break;
	case 312:
		SettingData.DispSize = SendMessage( DispSizeLst, CB_GETCURSEL, 0, 0 );
		break;
	case 314:
		SettingData.DispRot = SendMessage( DispRotLst, CB_GETCURSEL, 0, 0 );
		break;
	case 316:
		if ( GetWindowText( DispRectXLst, buff, 16 ) > 0 ){
			SettingData.DispRectX = _wtoi( buff );
		}
		break;
	case 317:
		if ( GetWindowText( DispRectYLst, buff, 16 ) > 0 ){
			SettingData.DispRectY = _wtoi( buff );
		}
		break;
	case 318:
		if ( GetWindowText( DispRectWLst, buff, 16 ) > 0 ){
			SettingData.DispRectW = _wtoi( buff );
		}
		break;
	case 319:
		if ( GetWindowText( DispRectHLst, buff, 16 ) > 0 ){
			SettingData.DispRectH = _wtoi( buff );
		}
		break;
	case 320:
		if ( SendMessage( DispOffChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.DispOff = 1;
		} else {
			SettingData.DispOff = 0;
		}
		break;
	case 321:
		if ( SendMessage( InputBGChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.InputBG = 1;
		} else {
			SettingData.InputBG = 0;
		}
		break;
	}
}
