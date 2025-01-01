#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting_PSP																	*/
/*------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Setting.h"
#include "RemoteJoyLite.h"
#include "Setting_PSP.h"
#include "Setting_PSP.dat"

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern "C" { int _wtoi (const WCHAR *); };

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static HWND PSPModeTxt;
static HWND PSPModeLst;
static HWND PSPAdr1Txt;
static HWND PSPAdr1Lst;
static HWND PSPAdr2Txt;
static HWND PSPAdr2Lst;
static HWND PSPPriTxt;
static HWND PSPPriLst;
static HWND PSPFPSTxt;
static HWND PSPFPSLst;
static HWND PSPAsync;
static HWND PSPDispChk;
static HWND PSPTrnsTxt;
static HWND PSPTrnsLst;
static HWND PSPRectTxt;
static HWND PSPRectXLst;
static HWND PSPRectYLst;
static HWND PSPRectWLst;
static HWND PSPRectHLst;
static HWND PSPDbgChk;

#define PRESET_NUM		3

static struct {
	int		x, y, w, h;
} Preset[PRESET_NUM] = {
	{   0,  0, 480, 272 },
	{  80, 16, 320, 240 },
	{ 128,  8, 224, 256 },
};

/*------------------------------------------------------------------------------*/
/* WmDestroyPSPConf																*/
/*------------------------------------------------------------------------------*/
void WmDestroyPSPConf( HWND hWnd )
{
	DestroyWindow( PSPModeTxt );
	DestroyWindow( PSPModeLst );
	DestroyWindow( PSPAdr1Txt );
	DestroyWindow( PSPAdr1Lst );
	DestroyWindow( PSPAdr2Txt );
	DestroyWindow( PSPAdr2Lst );
	DestroyWindow( PSPPriTxt );
	DestroyWindow( PSPPriLst );
	DestroyWindow( PSPFPSTxt );
	DestroyWindow( PSPFPSLst );
	DestroyWindow( PSPAsync );
	DestroyWindow( PSPDispChk );
	DestroyWindow( PSPTrnsTxt );
	DestroyWindow( PSPTrnsLst );
	DestroyWindow( PSPRectTxt );
	DestroyWindow( PSPRectXLst );
	DestroyWindow( PSPRectYLst );
	DestroyWindow( PSPRectWLst );
	DestroyWindow( PSPRectHLst );
	DestroyWindow( PSPDbgChk );
	WmDestroySaveButton( hWnd );
}

/*------------------------------------------------------------------------------*/
/* WmCommandPSPConf																*/
/*------------------------------------------------------------------------------*/
static void SetPSPRectString( void )
{
	WCHAR buff[16];

	wsprintf( buff, L"%d", SettingData.PSPRectX );
	SetWindowText( PSPRectXLst, buff );
	wsprintf( buff, L"%d", SettingData.PSPRectY );
	SetWindowText( PSPRectYLst, buff );
	wsprintf( buff, L"%d", SettingData.PSPRectW );
	SetWindowText( PSPRectWLst, buff );
	wsprintf( buff, L"%d", SettingData.PSPRectH );
	SetWindowText( PSPRectHLst, buff );
}

/*------------------------------------------------------------------------------*/
/* WmCreatePSPConf																*/
/*------------------------------------------------------------------------------*/
void WmCreatePSPConf( HWND hWnd, HINSTANCE hInst )
{
	WCHAR buff[16];

	PSPModeTxt = MyCreateWindow( &CWD_PSPModeTxt, hWnd, hInst );
	PSPModeLst = MyCreateWindow( &CWD_PSPModeLst, hWnd, hInst );
	for ( int i=0; i<5; i++ ){
		SendMessage( PSPModeLst, CB_ADDSTRING, 0, (LPARAM)ModeLst[i] );
	}
	SendMessage( PSPModeLst, CB_SETCURSEL, SettingData.PSPMode, 0 );

	PSPAdr1Txt = MyCreateWindow( &CWD_PSPAdr1Txt, hWnd, hInst );
	PSPAdr1Lst = MyCreateWindow( &CWD_PSPAdr1Lst, hWnd, hInst );
	for ( int i=0; i<128; i++ ){
		wsprintf( buff, L"0x%08X", 0x08400000 + i*0x8000 );
		SendMessage( PSPAdr1Lst, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( PSPAdr1Lst, CB_SETCURSEL, SettingData.PSPAdr1, 0 );

	PSPAdr2Txt = MyCreateWindow( &CWD_PSPAdr2Txt, hWnd, hInst );
	PSPAdr2Lst = MyCreateWindow( &CWD_PSPAdr2Lst, hWnd, hInst );
	SendMessage( PSPAdr2Lst, CB_ADDSTRING, 0, (LPARAM)AdrUnused );
	for ( int i=0; i<128; i++ ){
		wsprintf( buff, L"0x%08X", 0x8A000000 + i*0x40000 );
		SendMessage( PSPAdr2Lst, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( PSPAdr2Lst, CB_SETCURSEL, SettingData.PSPAdr2, 0 );

	PSPPriTxt = MyCreateWindow( &CWD_PSPPriTxt, hWnd, hInst );
	PSPPriLst = MyCreateWindow( &CWD_PSPPriLst, hWnd, hInst );
	for ( int i=0; i<64; i++ ){
		wsprintf( buff, L"%d", i );
		SendMessage( PSPPriLst, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( PSPPriLst, CB_SETCURSEL, SettingData.PSPPri, 0 );

	PSPFPSTxt = MyCreateWindow( &CWD_PSPFPSTxt, hWnd, hInst );
	PSPFPSLst = MyCreateWindow( &CWD_PSPFPSLst, hWnd, hInst );
	SendMessage( PSPFPSLst, CB_ADDSTRING, 0, (LPARAM)L"------" );
	for ( int i=1; i<4; i++ ){
		wsprintf( buff, L"%d FPS", 60/(i+1) );
		SendMessage( PSPFPSLst, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( PSPFPSLst, CB_SETCURSEL, SettingData.PSPFPS, 0 );

	PSPAsync = MyCreateWindow( &CWD_PSPAsync, hWnd, hInst );
	if ( SettingData.PSPAsync != 0 ){
		SendMessage( PSPAsync, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	PSPDispChk = MyCreateWindow( &CWD_PSPDispChk, hWnd, hInst );
	if ( SettingData.PSPDisp != 0 ){
		SendMessage( PSPDispChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	PSPTrnsTxt = MyCreateWindow( &CWD_PSPTrnsTxt, hWnd, hInst );
	PSPTrnsLst = MyCreateWindow( &CWD_PSPTrnsLst, hWnd, hInst );
	for ( int i=0; i<PRESET_NUM; i++ ){
		wsprintf( buff, L"(%d,%d)-(%d,%d)", Preset[i].x, Preset[i].y, Preset[i].w, Preset[i].h );
		SendMessage( PSPTrnsLst, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( PSPTrnsLst, CB_SETCURSEL, 0, 0 );

	PSPRectTxt  = MyCreateWindow( &CWD_PSPRectTxt,  hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.PSPRectX );
	PSPRectXLst = MyCreateWindow( &CWD_PSPRectXLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.PSPRectY );
	PSPRectYLst = MyCreateWindow( &CWD_PSPRectYLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.PSPRectW );
	PSPRectWLst = MyCreateWindow( &CWD_PSPRectWLst, hWnd, hInst );
	wsprintf( NumberBuff, L"%d", SettingData.PSPRectH );
	PSPRectHLst = MyCreateWindow( &CWD_PSPRectHLst, hWnd, hInst );

	PSPDbgChk = MyCreateWindow( &CWD_PSPDbgChk, hWnd, hInst );
	if ( SettingData.PSPDbg != 0 ){
		SendMessage( PSPDbgChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	WmCreateSaveButton( hWnd );
	InvalidateRect( hWnd, NULL, TRUE );
}

/*------------------------------------------------------------------------------*/
/* WmCommandPSPConf																*/
/*------------------------------------------------------------------------------*/
void WmCommandPSPConf( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	WCHAR buff[16];

	switch ( id ){
	case 601: {
		int no0 = SendMessage( PSPModeLst, CB_GETCURSEL, 0, 0 );
		SettingData.PSPMode = no0;
		break;
	}
	case 603: {
		int no1 = SendMessage( PSPAdr1Lst, CB_GETCURSEL, 0, 0 );
		SettingData.PSPAdr1 = no1;
		break;
	}
	case 605: {
		int no2 = SendMessage( PSPAdr2Lst, CB_GETCURSEL, 0, 0 );
		SettingData.PSPAdr2 = no2;
		break;
	}
	case 607: {
		int no3 = SendMessage( PSPPriLst, CB_GETCURSEL, 0, 0 );
		SettingData.PSPPri = no3;
		break;
	}
	case 609: {
		int no4 = SendMessage( PSPFPSLst, CB_GETCURSEL, 0, 0 );
		SettingData.PSPFPS = no4;
		break;
	}
	case 610:
		if ( SendMessage( PSPDispChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.PSPDisp = 1;
		} else {
			SettingData.PSPDisp = 0;
		}
		break;
	case 611:
		if ( SendMessage( PSPAsync, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.PSPAsync = 1;
		} else {
			SettingData.PSPAsync = 0;
		}
		break;
	case 621: {
		int no5 = SendMessage( PSPTrnsLst, CB_GETCURSEL, 0, 0 );
		SettingData.PSPRectX = Preset[no5].x;
		SettingData.PSPRectY = Preset[no5].y;
		SettingData.PSPRectW = Preset[no5].w;
		SettingData.PSPRectH = Preset[no5].h;
		SetPSPRectString();
		RemoteJoyLite_SendPSPCmd();
		break;
	}
	case 623:
		if ( GetWindowText( PSPRectXLst, buff, 16 ) > 0 ){
			SettingData.PSPRectX = _wtoi( buff );
			if ( SettingData.PSPRectX > 480 ){
				SettingData.PSPRectX = 480;
				SetPSPRectString();
			}
			RemoteJoyLite_SendPSPCmd();
		}
		break;
	case 624:
		if ( GetWindowText( PSPRectYLst, buff, 16 ) > 0 ){
			SettingData.PSPRectY = _wtoi( buff );
			if ( SettingData.PSPRectY > 272 ){
				SettingData.PSPRectY = 272;
				SetPSPRectString();
			}
			RemoteJoyLite_SendPSPCmd();
		}
		break;
	case 625:
		if ( GetWindowText( PSPRectWLst, buff, 16 ) > 0 ){
			SettingData.PSPRectW = _wtoi( buff );
			if ( SettingData.PSPRectW > 480 ){
				SettingData.PSPRectW = 480;
				SetPSPRectString();
			}
			RemoteJoyLite_SendPSPCmd();
		}
		break;
	case 626:
		if ( GetWindowText( PSPRectHLst, buff, 16 ) > 0 ){
			SettingData.PSPRectH = _wtoi( buff );
			if ( SettingData.PSPRectH > 272 ){
				SettingData.PSPRectH = 272;
				SetPSPRectString();
			}
			RemoteJoyLite_SendPSPCmd();
		}
		break;
	case 650:
		if ( SendMessage( PSPDbgChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.PSPDbg = 1;
		} else {
			SettingData.PSPDbg = 0;
		}
		break;
	}
}
