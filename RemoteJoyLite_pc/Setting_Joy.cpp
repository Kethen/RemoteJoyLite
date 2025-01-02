#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting_Joy																	*/
/*------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Setting.h"
#include "Setting_Joy.h"
#include "Setting_Joy.dat"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define WSD_GROUPBOX	(WS_CHILD|WS_VISIBLE|WS_GROUP|BS_GROUPBOX)
#define WSD_EDITBOX		(WS_CHILD|WS_VISIBLE|ES_READONLY|ES_MULTILINE)
#define WSD_BUTTON		(WS_CHILD|WS_VISIBLE|WS_TABSTOP)
#define TXTP(p)			(&p[lstrlen(p)])

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static HWND JoyConfBtn[22];
static HWND JoyConfBox[22];
static HWND JoyConfTxt[22];
static HWND JoyAnaText;
static HWND JoyAnaRText;
static HWND JoyAnaCBox;
static HWND JoyAnaRCBox;
static HWND JoyMgnText;
static HWND JoyMgnCBox;
static HWND JoyIndexNo;
static HWND JoyStatGrp;
static HWND JoyStatInfo;
static WCHAR JoyStatText[2048];

/*------------------------------------------------------------------------------*/
/* SetJoyConfTxt																*/
/*------------------------------------------------------------------------------*/
static void SetJoyConfTxt( int idx )
{
	WCHAR buff[32];
	int btn1 = SettingData.JoyConf[idx] >> 32;
	int btn2 = SettingData.JoyConf[idx];

	wsprintf( buff, L"%08X\n%08X", btn1, btn2 );
	for ( int i=0; i<32; i++ ){ if ( buff[i] == L'0' ){ buff[i] = L'-'; } }
	SetWindowText( JoyConfTxt[idx], buff );
}

/*------------------------------------------------------------------------------*/
/* WmDestroyJoyConf																*/
/*------------------------------------------------------------------------------*/
void WmDestroyJoyConf( HWND hWnd )
{
	for ( int i=0; i<22; i++ ){
		DestroyWindow( JoyConfBtn[i] );
		DestroyWindow( JoyConfBox[i] );
		DestroyWindow( JoyConfTxt[i] );
	}
	DestroyWindow( JoyAnaText );
	DestroyWindow( JoyAnaRText );
	DestroyWindow( JoyAnaCBox );
	DestroyWindow( JoyAnaRCBox );
	DestroyWindow( JoyMgnText );
	DestroyWindow( JoyMgnCBox );
	DestroyWindow( JoyIndexNo );
	DestroyWindow( JoyStatGrp );
	DestroyWindow( JoyStatInfo );
	JoyStatText[0] = 0;
	WmDestroySaveButton( hWnd );
}

/*------------------------------------------------------------------------------*/
/* WmCreateJoyConf																*/
/*------------------------------------------------------------------------------*/
void WmCreateJoyConf( HWND hWnd, HINSTANCE hInst )
{
	WCHAR buff[16];

	/* Button Conf */
	for ( int i=0; i<22; i++ ){
		WCHAR *name = ButtonName[ButtonIndex[i]];
		int x = 22 + (i/11)*113;
		int y = 40 + (i%11)*22;

		JoyConfBtn[i] = CreateWindow( L"BUTTON", name, WSD_BUTTON,      x,   y, 52, 21, hWnd, (HMENU)(i+100), hInst, NULL );
		JoyConfBox[i] = CreateWindow( L"BUTTON", NULL, WSD_GROUPBOX, x+56, y-9, 46, 31, hWnd, (HMENU)(i+125), hInst, NULL );
		JoyConfTxt[i] = CreateWindow( L"EDIT",   NULL, WSD_EDITBOX,  x+59, y+2, 41, 18, hWnd, (HMENU)(i+150), hInst, NULL );
		SetFontNorm( JoyConfBtn[i] );
		SetFontMini( JoyConfTxt[i] );
		SetJoyConfTxt( i );
	}
	/* Use Joy No */
	JoyIndexNo = MyCreateWindow( &CWD_JoyIndexNo, hWnd, hInst );
	for ( int i=0; i<JOY_MAX; i++ ){
		wsprintf( buff, L"Use Joy%02d", i );
		SendMessage( JoyIndexNo, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( JoyIndexNo, CB_SETCURSEL, SettingData.JoyNo, 0 );
	/* Joy Status */
	JoyStatGrp  = MyCreateWindow( &CWD_JoyStatGrp,  hWnd, hInst );
	JoyStatInfo = MyCreateWindow( &CWD_JoyStatInfo, hWnd, hInst );
	/* Analog Conf */
	JoyAnaText = MyCreateWindow( &CWD_JoyAnaText, hWnd, hInst );
	JoyAnaRText = MyCreateWindow( &CWD_JoyAnaRText, hWnd, hInst );
	JoyAnaCBox = MyCreateWindow( &CWD_JoyAnaCBox, hWnd, hInst );
	JoyAnaRCBox = MyCreateWindow( &CWD_JoyAnaRCBox, hWnd, hInst );
	for ( int i=0; i<4; i++ ){
		SendMessage( JoyAnaCBox, CB_ADDSTRING, 0, (LPARAM)DirectName[i] );
	}
	for ( int i=0; i<4; i++ ){
		SendMessage( JoyAnaRCBox, CB_ADDSTRING, 0, (LPARAM)DirectName[i] );
	}
	SendMessage( JoyAnaCBox, CB_SETCURSEL, SettingData.JoyAnalog, 0 );
	SendMessage( JoyAnaRCBox, CB_SETCURSEL, SettingData.JoyAnalogR, 0 );
	/* Margin Conf */
	JoyMgnText = MyCreateWindow( &CWD_JoyMgnText, hWnd, hInst );
	JoyMgnCBox = MyCreateWindow( &CWD_JoyMgnCBox, hWnd, hInst );
	for ( int i=0; i<101; i++ ){
		wsprintf( buff, L"%3d%%", i-50 );
		SendMessage( JoyMgnCBox, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( JoyMgnCBox, CB_SETCURSEL, SettingData.JoyMargin, 0 );

	WmCreateSaveButton( hWnd );
	InvalidateRect( hWnd, NULL, TRUE );
	JoyStatText[0] = 0;
}

/*------------------------------------------------------------------------------*/
/* WmCommandJoyConf																*/
/*------------------------------------------------------------------------------*/
void WmCommandJoyConf( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	switch ( id ){
	case 100 ... 121:
		SettingData.JoyConf[id - 100] = SettingButton();
		SetJoyConfTxt( id - 100 );
		break;
	case 191:
		SettingData.JoyAnalog = SendMessage( JoyAnaCBox, CB_GETCURSEL, 0, 0 );
		break;
	case 197:
		SettingData.JoyAnalogR = SendMessage( JoyAnaRCBox, CB_GETCURSEL, 0, 0 );
		break;
	case 193:
		SettingData.JoyMargin = SendMessage( JoyMgnCBox, CB_GETCURSEL, 0, 0 );
		break;
	case 194:
		SettingData.JoyNo = SendMessage( JoyIndexNo, CB_GETCURSEL, 0, 0 );
		break;
	}
}

/*------------------------------------------------------------------------------*/
/* UpdateJoyStatus																*/
/*------------------------------------------------------------------------------*/
void UpdateJoyStatus( AkindDI *pAkindDI )
{
	WCHAR TempText[2048] = {0};
	int JoyNo = SettingData.JoyNo;

	int btn1 = pAkindDI->GetJoyButton( JoyNo ) >> 32;
	int btn2 = pAkindDI->GetJoyButton( JoyNo );
	int lX   = pAkindDI->GetJoyAxis( JoyNo, 0 );
	int lY   = pAkindDI->GetJoyAxis( JoyNo, 1 );
	int lZ   = pAkindDI->GetJoyAxis( JoyNo, 2 );
	int lRx  = pAkindDI->GetJoyRot( JoyNo, 0 );
	int lRy  = pAkindDI->GetJoyRot( JoyNo, 1 );
	int lRz  = pAkindDI->GetJoyRot( JoyNo, 2 );

	wsprintf( TXTP(TempText), L"BTN-LO  %04X\r\n", btn1 & 0xFFFF );
	wsprintf( TXTP(TempText), L"BTN-HI  %04X\r\n", btn1 >> 16 );
	wsprintf( TXTP(TempText), L"POV     %04X\r\n", btn2 & 0xFFFF );
	wsprintf( TXTP(TempText), L"ANALOG  %04X\r\n", btn2 >> 16 );
	wsprintf( TXTP(TempText), L"\r\n" );
	wsprintf( TXTP(TempText), L"AXIS-X  %04X\r\n", lX  & 0xFFFF );
	wsprintf( TXTP(TempText), L"AXIS-Y  %04X\r\n", lY  & 0xFFFF );
	wsprintf( TXTP(TempText), L"AXIS-Z  %04X\r\n", lZ  & 0xFFFF );
	wsprintf( TXTP(TempText), L"ROT-X   %04X\r\n", lRx & 0xFFFF );
	wsprintf( TXTP(TempText), L"ROT-Y   %04X\r\n", lRy & 0xFFFF );
	wsprintf( TXTP(TempText), L"ROT-Z   %04X\r\n", lRz & 0xFFFF );

	if ( lstrcmp( JoyStatText, TempText ) != 0 ){
		SetWindowText( JoyStatInfo, TempText );
		lstrcpy( JoyStatText, TempText );
	}
}
