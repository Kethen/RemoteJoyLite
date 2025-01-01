#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting																		*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "RemoteJoyLite.h"
#include "Macro.h"
#include "Setting.h"
#include "Setting_Joy.h"
#include "Setting_Key.h"
#include "Setting_Etc.h"
#include "Setting_Filt.h"
#include "Setting_Capt.h"
#include "Setting_PSP.h"
#include "Setting_Macro.h"
#include "Setting.dat"
#include "../remotejoy.h"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define WS_SETTING		(WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_VISIBLE)
#define SETTING_W		360
#define SETTING_H		330
#define	MAX_VAL			32767

/*------------------------------------------------------------------------------*/
/* SettingData																	*/
/*------------------------------------------------------------------------------*/
SETTING_DATA SettingData;
int ButtonIndex[22] = {13,14,15,12,8,9,4,5,6,7,22,0,3,16,23,20,21,24,25,26,27,28};

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static BOOL DispFlag = FALSE;
static AkindDI *pAkindDI;
static HWND hWndParent;
static HWND hWndSetting;
static HINSTANCE hInstParent;

static WCHAR ClsName[] = L"RemoteJoyLiteSettingWindow";

static int KeyDefault[21] = { DIK_X, DIK_Z, DIK_A, DIK_S, DIK_Q, DIK_W,
							  DIK_UP, DIK_RIGHT, DIK_DOWN, DIK_LEFT, DIK_SPACE,
							  DIK_RETURN, DIK_HOME, 0, 0, DIK_PRIOR, DIK_NEXT,
							  0, 0, 0, 0 };

/*------------------------------------------------------------------------------*/
/* SettingSave																	*/
/*------------------------------------------------------------------------------*/
static void SettingSave( void )
{
	FILE *fp = fopen( "RemoteJoyLite.dat", "wb" );

	SettingData.Version = RJL_VERSION;
	if ( fp == NULL ){ return; }
	fwrite( &SettingData, sizeof(SETTING_DATA), 1, fp );
	fclose( fp );
}

/*------------------------------------------------------------------------------*/
/* SettingLoad																	*/
/*------------------------------------------------------------------------------*/
void SettingLoad( void )
{
	FILE *fp = fopen( "RemoteJoyLite.dat", "rb" );

	if ( fp != NULL ){
		fread( &SettingData, sizeof(SETTING_DATA), 1, fp );
		fclose( fp );
		SettingData.Movie.dwFlags = ICMF_COMPVARS_VALID;
		if ( SettingData.Version == RJL_VERSION ){ return; }
		MessageBox( NULL, L"Setting Data Version Error.\r\n   Please Setting Again.",
					L"RemoteJoyLite", MB_OK );
	}
	ZeroMemory( &SettingData, sizeof(SETTING_DATA) );
	SettingData.JoyMargin        = 50;
	SettingData.MovieFPS         = 1;
	SettingData.Movie.cbSize     = sizeof(COMPVARS);
	SettingData.Movie.dwFlags    = ICMF_COMPVARS_VALID;
	SettingData.Movie.fccHandler = mmioFOURCC( 'D','I','B',' ' );
	SettingData.Movie.lQ         = ICQUALITY_DEFAULT;
	memcpy( SettingData.KeyConf, KeyDefault, 21*sizeof(int) );
	SettingData.FullRectX        = 0;
	SettingData.FullRectY        = 0;
	SettingData.FullRectW        = 480;
	SettingData.FullRectH        = 272;
	SettingData.DispRectX        = 100;
	SettingData.DispRectY        = 0;
	SettingData.DispRectW        = 480;
	SettingData.DispRectH        = 272;
	SettingData.GammaRGB		 = 0;
	SettingData.Gamma[0]		 = 100;
	SettingData.Gamma[1]		 = 100;
	SettingData.Gamma[2]		 = 100;
	SettingData.Gamma[3]		 = 100;
	SettingData.PSPMode          = 1;
	SettingData.PSPAdr1          = 88;
	SettingData.PSPAdr2          = 65;
	SettingData.PSPPri           = 16;
	SettingData.PSPRectX         = 0;
	SettingData.PSPRectY         = 0;
	SettingData.PSPRectW         = 480;
	SettingData.PSPRectH         = 272;
}

/*------------------------------------------------------------------------------*/
/* UpdateButton																	*/
/*------------------------------------------------------------------------------*/
static void UpdateButton( AkindDI *pMainDI )
{
	int button = 0;
	int joy_no = SettingData.JoyNo;
	int Analog = SettingData.JoyAnalog;
	int axis_x = 0;
	int axis_y = 0;

	if ( SettingData.KeyUse != 0 ){
		for ( int i=0; i<21; i++ ){
			int dik  = SettingData.KeyConf[i];
			int mask = (1 << ButtonIndex[i]);
			if ( dik == 0 ){ continue; }
			if ( pMainDI->CheckKeyData( dik ) != FALSE ){ button |= mask; }
		}
	}

	long long data = pMainDI->GetJoyButton( joy_no );
	for ( int i=0; i<21; i++ ){
		int mask = (1 << ButtonIndex[i]);
		if ( data & SettingData.JoyConf[i] ){ button |= mask; }
	}
	RemoteJoyLite_SetButton( button );

	if ( Analog != 0 ){
		if ( Analog == 1 ){
			axis_x = pMainDI->GetJoyAxis( joy_no, 0 );
			axis_y = pMainDI->GetJoyAxis( joy_no, 1 );
		}
		if ( Analog == 2 ){
			axis_x = pMainDI->GetJoyRot( joy_no, 0 );
			axis_y = pMainDI->GetJoyRot( joy_no, 1 );
		}
		if ( Analog == 3 ){
			axis_x = pMainDI->GetJoyAxis( joy_no, 2 );
			axis_y = pMainDI->GetJoyRot( joy_no, 2 );
		}
		int mgn = SettingData.JoyMargin;
		if ( mgn != 50 ){
			if ( mgn < 50 ){
				float a = MAX_VAL*(50-mgn)/100;
				float b = (MAX_VAL-a)/MAX_VAL;
				if ( axis_x > 0 ){ axis_x = (int)((float)axis_x*b+a); }
				if ( axis_x < 0 ){ axis_x = (int)((float)axis_x*b-a); }
				if ( axis_y > 0 ){ axis_y = (int)((float)axis_y*b+a); }
				if ( axis_y < 0 ){ axis_y = (int)((float)axis_y*b-a); }
			} else {
				float a = MAX_VAL*(mgn-50)/100;
				float b = MAX_VAL/(MAX_VAL-a);
				if ( abs(axis_x) < a ){ axis_x = 0; }
				if ( axis_x > 0 ){ axis_x = (int)(((float)axis_x-a)*b); }
				if ( axis_x < 0 ){ axis_x = (int)(((float)axis_x+a)*b); }
				if ( abs(axis_y) < a ){ axis_y = 0; }
				if ( axis_y > 0 ){ axis_y = (int)(((float)axis_y-a)*b); }
				if ( axis_y < 0 ){ axis_y = (int)(((float)axis_y+a)*b); }
			}
		}
	}

	if ( button & 0x01000000 ){ axis_y -= MAX_VAL; }
	if ( button & 0x02000000 ){ axis_x += MAX_VAL; }
	if ( button & 0x04000000 ){ axis_y += MAX_VAL; }
	if ( button & 0x08000000 ){ axis_x -= MAX_VAL; }
	if ( button & 0x0F000000 ){
		if ( axis_x >  MAX_VAL ){ axis_x =  MAX_VAL; }
		if ( axis_x < -MAX_VAL ){ axis_x = -MAX_VAL; }
		if ( axis_y >  MAX_VAL ){ axis_y =  MAX_VAL; }
		if ( axis_y < -MAX_VAL ){ axis_y = -MAX_VAL; }
	}
	axis_x = (axis_x + 32768)/256;
	axis_y = (axis_y + 32768)/256;
	RemoteJoyLite_SetAxis( axis_x, axis_y );
}

/*------------------------------------------------------------------------------*/
/* CheckFullScreenButton														*/
/*------------------------------------------------------------------------------*/
static void CheckFullScreenButton( AkindDI *pMainDI )
{
	long long data = pMainDI->GetJoyTrigger( SettingData.JoyNo );

	if ( data & SettingData.JoyConf[21] ){
		SendMessage( hWndParent, WM_LBUTTONDBLCLK, 0, 0 );
	}
}

/*------------------------------------------------------------------------------*/
/* MakeFont																		*/
/*------------------------------------------------------------------------------*/
static HFONT hFontMini;
static HFONT hFontNorm;
static HFONT hFontBold;

static void MakeFont( void )
{
	hFontMini = CreateFont( 9, 0, 0, 0,
							FW_NORMAL, FALSE, FALSE, FALSE,
							SHIFTJIS_CHARSET,
							OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,
							FIXED_PITCH | FF_ROMAN,
							L"ＭＳ ゴシック" );

	hFontNorm = CreateFont( 12, 0, 0, 0,
							FW_NORMAL, FALSE, FALSE, FALSE,
							SHIFTJIS_CHARSET,
							OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,
							FIXED_PITCH | FF_ROMAN,
							L"ＭＳ ゴシック" );

	hFontBold = CreateFont( 12, 0, 0, 0,
							FW_BOLD, FALSE, FALSE, FALSE,
							SHIFTJIS_CHARSET,
							OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,
							FIXED_PITCH | FF_ROMAN,
							L"ＭＳ Ｐゴシック" );
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static int TabPos = -1;
static HWND TabCtrl;
static HWND JoyCnfSave;

/*------------------------------------------------------------------------------*/
/* WmDestroySaveButton															*/
/*------------------------------------------------------------------------------*/
void WmDestroySaveButton( HWND hWnd )
{
	DestroyWindow( JoyCnfSave );
}

/*------------------------------------------------------------------------------*/
/* WmCreateSaveButton															*/
/*------------------------------------------------------------------------------*/
void WmCreateSaveButton( HWND hWnd )
{
	JoyCnfSave = MyCreateWindow( &CWD_JoyCnfSave, hWnd, hInstParent );
}

/*------------------------------------------------------------------------------*/
/* TabChange																	*/
/*------------------------------------------------------------------------------*/
static void TabChange( HWND hWnd )
{
	switch ( TabPos ){
	case 0 : WmDestroyJoyConf  ( hWnd );	break;
	case 1 : WmDestroyKeyConf  ( hWnd );	break;
	case 2 : WmDestroyEtcConf  ( hWnd );	break;
	case 3 : WmDestroyFiltConf ( hWnd );	break;
	case 4 : WmDestroyCaptConf ( hWnd );	break;
	case 5 : WmDestroyPSPConf  ( hWnd );	break;
	case 6 : WmDestroyMacroConf( hWnd );	break;
	}
	TabPos = TabCtrl_GetCurSel( TabCtrl );
	switch ( TabPos ){
	case 0 : WmCreateJoyConf  ( hWnd, hInstParent );	break;
	case 1 : WmCreateKeyConf  ( hWnd, hInstParent );	break;
	case 2 : WmCreateEtcConf  ( hWnd, hInstParent );	break;
	case 3 : WmCreateFiltConf ( hWnd, hInstParent );	break;
	case 4 : WmCreateCaptConf ( hWnd, hInstParent );	break;
	case 5 : WmCreatePSPConf  ( hWnd, hInstParent );	break;
	case 6 : WmCreateMacroConf( hWnd, hInstParent );	break;
	}
}

/*------------------------------------------------------------------------------*/
/* WmCreateSettingTab															*/
/*------------------------------------------------------------------------------*/
static CW_DATA CWD_TabCtrl = { 5, 5, 350, 292, 0, CWD_TABCTRL,  FNT_NORM, NULL };

static void WmCreateSettingTab( HWND hWnd )
{
	TCITEM	tc_item;

	TabCtrl = MyCreateWindow( &CWD_TabCtrl, hWnd, hInstParent );

	tc_item.mask = TCIF_TEXT;
	tc_item.pszText = L"Joy";
	TabCtrl_InsertItem( TabCtrl, 0, &tc_item );
	tc_item.pszText = L"Key";
	TabCtrl_InsertItem( TabCtrl, 1, &tc_item );
	tc_item.pszText = L"Etc";
	TabCtrl_InsertItem( TabCtrl, 2, &tc_item );
	tc_item.pszText = L"Filter";
	TabCtrl_InsertItem( TabCtrl, 3, &tc_item );
	tc_item.pszText = L"Capture";
	TabCtrl_InsertItem( TabCtrl, 4, &tc_item );
	tc_item.pszText = L"PSP";
	TabCtrl_InsertItem( TabCtrl, 5, &tc_item );
	tc_item.pszText = L"Macro";
	TabCtrl_InsertItem( TabCtrl, 6, &tc_item );

	if ( TabPos != -1 ){ SendMessage( TabCtrl, TCM_SETCURSEL, TabPos, 0 ); }
	TabPos = -1;
	TabChange( hWnd );
}

/*------------------------------------------------------------------------------*/
/* WmCommandProc																*/
/*------------------------------------------------------------------------------*/
static void WmCommandProc( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	WmCommandJoyConf  ( hWnd, code, id, hCtl );
	WmCommandKeyConf  ( hWnd, code, id, hCtl );
	WmCommandEtcConf  ( hWnd, code, id, hCtl );
	WmCommandFiltConf ( hWnd, code, id, hCtl );
	WmCommandCaptConf ( hWnd, code, id, hCtl );
	WmCommandPSPConf  ( hWnd, code, id, hCtl );
	WmCommandMacroConf( hWnd, code, id, hCtl );
	if ( id == 999 ){ SettingSave(); }
}

/*------------------------------------------------------------------------------*/
/* SettingWindowCreate															*/
/*------------------------------------------------------------------------------*/
static void SettingWindowCreate( int FullScreen )
{
	RECT ParentRect;
	GetWindowRect( hWndParent, &ParentRect );

	hWndSetting = CreateWindow( ClsName,
								L"RemoteJoyLiteSetting",
								WS_SETTING,
								ParentRect.left+50,
								ParentRect.top+50,
								SETTING_W,
								SETTING_H,
								NULL,
								NULL,
								hInstParent,
								NULL );

	RECT rect = { 0, 0, SETTING_W, SETTING_H };
	AdjustWindowRect( &rect, WS_SETTING, FALSE );
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	SetWindowPos( hWndSetting, HWND_TOPMOST, 0, 0, w, h, SWP_NOMOVE );
	pAkindDI->Init( hWndSetting, FALSE );
}

/*------------------------------------------------------------------------------*/
/* WndProc																		*/
/*------------------------------------------------------------------------------*/
static LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	pAkindDI->Message( msg, wParam, lParam );

	switch ( msg ){
	case WM_CREATE:
		WmCreateSettingTab( hWnd );
		break;
	case WM_NOTIFY:
		if ( ((NMHDR *)lParam)->code == TCN_SELCHANGE ){ TabChange( hWnd ); }
		break;
	case WM_COMMAND:
		WmCommandProc( hWnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam );
		break;
	case WM_CLOSE:
		EnableWindow( hWndParent, TRUE );
		BringWindowToTop( hWndParent );
		MacroLoad();
		RemoteJoyLite_SendPSPCmd();
		DispFlag = FALSE;
		break;
	case WM_DESTROY:
		PostQuitMessage( 0 );
		pAkindDI->Exit();
		break;
	}
	return( DefWindowProc( hWnd, msg, wParam, lParam ) );
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* SettingInit																	*/
/*------------------------------------------------------------------------------*/
BOOL SettingInit( HWND hWnd, HINSTANCE hInst )
{
	pAkindDI    = new AkindDI();
	hWndParent  = hWnd;
	hInstParent = hInst;
	MakeFont();

	WNDCLASS WndCls;

	WndCls.style         = CS_HREDRAW | CS_VREDRAW;
	WndCls.lpfnWndProc   = WndProc;
	WndCls.cbClsExtra    = 0;
	WndCls.cbWndExtra    = 0;
	WndCls.hInstance     = hInstParent;
	WndCls.hIcon         = LoadIcon( hInstParent, L"REMOTEJOYLITE_ICON" );
	WndCls.hCursor       = LoadCursor( NULL, IDC_ARROW );
	WndCls.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	WndCls.lpszMenuName  = NULL;
	WndCls.lpszClassName = ClsName;
	if ( RegisterClass(&WndCls) == FALSE ){ return( FALSE ); }
	return( TRUE );
}

/*------------------------------------------------------------------------------*/
/* SettingExit																	*/
/*------------------------------------------------------------------------------*/
void SettingExit( void )
{
}

/*------------------------------------------------------------------------------*/
/* SettingProc																	*/
/*------------------------------------------------------------------------------*/
void SettingProc( UINT msg, WPARAM wParam, LPARAM lParam )
{
	if ( DispFlag == FALSE ){ return; }
	SendMessage( hWndSetting, msg, wParam, lParam );
}

/*------------------------------------------------------------------------------*/
/* SettingMessage																*/
/*------------------------------------------------------------------------------*/
BOOL SettingMessage( MSG *msg, int FullScreen )
{
	if ( MacroRecodeCheck() != FALSE ){ return( FALSE ); }
	if ( RemoteJoyLite_CheckMovie() != FALSE ){ return( FALSE ); }
	if ( ((msg->message == WM_KEYDOWN) && (msg->wParam == VK_ESCAPE)) || (msg->message == WM_RBUTTONDOWN) ){
		if ( DispFlag != FALSE ){
			SendMessage( hWndSetting, WM_CLOSE, 0, 0 );
		} else {
			SettingWindowCreate( FullScreen );
			EnableWindow( hWndParent, FALSE );
			DispFlag = TRUE;
		}
		return( TRUE );
	}
	if ( DispFlag == FALSE ){ return( FALSE ); }
	return( IsDialogMessage( hWndSetting, msg ) );
}

/*------------------------------------------------------------------------------*/
/* SettingSync																	*/
/*------------------------------------------------------------------------------*/
void SettingSync( AkindDI *pMainDI )
{
	if ( DispFlag == FALSE ){
		UpdateButton( pMainDI );
		CheckFullScreenButton( pMainDI );
	} else {
		pAkindDI->Sync();
		if ( TabPos == 0 ){ UpdateJoyStatus( pAkindDI ); }
	}
}

/*------------------------------------------------------------------------------*/
/* SettingFlag																	*/
/*------------------------------------------------------------------------------*/
BOOL SettingFlag( void )
{
	return( DispFlag );
}

/*------------------------------------------------------------------------------*/
/* SettingButton																*/
/*------------------------------------------------------------------------------*/
long long SettingButton( void )
{
	return ( pAkindDI->GetJoyButton( SettingData.JoyNo ) );
}

/*------------------------------------------------------------------------------*/
/* SetFont																		*/
/*------------------------------------------------------------------------------*/
void SetFontMini( HWND hWnd )
{
	SendMessage( hWnd, WM_SETFONT, (WPARAM)hFontMini, 0 );
}

void SetFontNorm( HWND hWnd )
{
	SendMessage( hWnd, WM_SETFONT, (WPARAM)hFontNorm, 0 );
}

void SetFontBold( HWND hWnd )
{
	SendMessage( hWnd, WM_SETFONT, (WPARAM)hFontBold, 0 );
}

/*------------------------------------------------------------------------------*/
/* MyCreateWindow																*/
/*------------------------------------------------------------------------------*/
HWND MyCreateWindow( CW_DATA *cwd, HWND hWnd, HINSTANCE hInst )
{
	HWND ret;

	ret = CreateWindowEx( 0, cwd->lpClassName, cwd->lpWindowName, cwd->dwStyle,
						  cwd->x, cwd->y, cwd->w, cwd->h, hWnd, (HMENU)cwd->id, hInst, NULL );

	switch ( cwd->font ){
	case FNT_MINI:SetFontMini( ret );	break;
	case FNT_NORM:SetFontNorm( ret );	break;
	case FNT_BOLD:SetFontBold( ret );	break;
	}
	return( ret );
}
