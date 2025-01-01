#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting_Macro																*/
/*------------------------------------------------------------------------------*/
#include "Setting.h"
#include "Setting_Macro.h"
#include "Setting_Macro.dat"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define WSD_COMBOBOX	(WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST|WS_TABSTOP)
#define WSD_GROUPBOX	(WS_CHILD|WS_VISIBLE|WS_GROUP|BS_GROUPBOX)
#define WSD_EDITBOX		(WS_CHILD|WS_VISIBLE|ES_READONLY|ES_MULTILINE)
#define WSD_BUTTON		(WS_CHILD|WS_VISIBLE|WS_TABSTOP)

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static HWND McrRecTxt;
static HWND McrRecUse;
static HWND McrRecNo;
static HWND McrRecAna;
static HWND McrPlyTxt;
static HWND McrPlyChk;
static HWND McrPlaySet[5];
static HWND McrPlayGrp[5];
static HWND McrPlayEdt[5];
static HWND McrPlayLst[4];
static HWND McrPlayMno[4];

/*------------------------------------------------------------------------------*/
/* WmDestroyMacroConf															*/
/*------------------------------------------------------------------------------*/
void WmDestroyMacroConf( HWND hWnd )
{
	DestroyWindow( McrRecTxt );
	DestroyWindow( McrRecUse );
	DestroyWindow( McrRecNo );
	DestroyWindow( McrRecAna );
	DestroyWindow( McrPlyTxt );
	DestroyWindow( McrPlyChk );
	for ( int i=0; i<5; i++ ){
		DestroyWindow( McrPlaySet[i] );
		DestroyWindow( McrPlayGrp[i] );
		DestroyWindow( McrPlayEdt[i] );
	}
	for ( int i=0; i<4; i++ ){
		DestroyWindow( McrPlayLst[i] );
		DestroyWindow( McrPlayMno[i] );
	}
	WmDestroySaveButton( hWnd );
}

/*------------------------------------------------------------------------------*/
/* SetMacroButtonTxt															*/
/*------------------------------------------------------------------------------*/
static void SetMacroButtonTxt( void )
{
	WCHAR buff[32];

	for ( int j=0; j<5; j++ ){
		int btn1 = SettingData.McrButton[j] >> 32;
		int btn2 = SettingData.McrButton[j];

		wsprintf( buff, L"%08X\n%08X", btn1, btn2 );
		for ( int i=0; i<32; i++ ){ if ( buff[i] == L'0' ){ buff[i] = L'-'; } }
		SetWindowText( McrPlayEdt[j], buff );
	}
}

/*------------------------------------------------------------------------------*/
/* WmEnableMacroWindow															*/
/*------------------------------------------------------------------------------*/
void WmEnableMacroWindow( void )
{
	if ( SettingData.McrRecUse != 0 ){
		EnableWindow( McrRecNo, TRUE );
		EnableWindow( McrRecAna, TRUE );
	} else {
		EnableWindow( McrRecNo, FALSE );
		EnableWindow( McrRecAna, FALSE );
	}
}

/*------------------------------------------------------------------------------*/
/* WmCreateMacroConf															*/
/*------------------------------------------------------------------------------*/
void WmCreateMacroConf( HWND hWnd, HINSTANCE hInst )
{
	WCHAR buff[16];

	McrRecTxt = MyCreateWindow( &CWD_McrRecTxt, hWnd, hInst );

	McrRecUse = MyCreateWindow( &CWD_McrRecUse, hWnd, hInst );
	if ( SettingData.McrRecUse != 0 ){
		SendMessage( McrRecUse, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	McrRecNo = MyCreateWindow( &CWD_McrRecNo, hWnd, hInst );
	for ( int i=0; i<10; i++ ){
		wsprintf( buff, L"MacroNo%d", i );
		SendMessage( McrRecNo, CB_ADDSTRING, 0, (LPARAM)buff );
	}
	SendMessage( McrRecNo, CB_SETCURSEL, SettingData.McrRecNo, 0 );

	McrRecAna = MyCreateWindow( &CWD_McrRecAna, hWnd, hInst );
	if ( SettingData.McrRecAna != 0 ){
		SendMessage( McrRecAna, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	McrPlyTxt = MyCreateWindow( &CWD_McrPlyTxt, hWnd, hInst );
	McrPlyChk = MyCreateWindow( &CWD_McrPlyChk, hWnd, hInst );
	if ( SettingData.McrPlayChk != 0 ){
		SendMessage( McrPlyChk, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	for ( int i=0; i<5; i++ ){
		int y = 130 + i*25;

		wsprintf( buff, L"Macro%d", i );
		if ( i == 0 ){ wsprintf( buff, L"MacroKey" ); }
		McrPlaySet[i] = CreateWindow( L"BUTTON",   buff, WSD_BUTTON,   30,   y, 60, 21, hWnd, (HMENU)(i+750), hInst, NULL );
		McrPlayGrp[i] = CreateWindow( L"BUTTON",   NULL, WSD_GROUPBOX, 94, y-9, 46, 31, hWnd, (HMENU)(i+760), hInst, NULL );
		McrPlayEdt[i] = CreateWindow( L"EDIT",     NULL, WSD_EDITBOX,  97, y+2, 41, 18, hWnd, (HMENU)(i+770), hInst, NULL );
		SetFontNorm( McrPlaySet[i] );
		SetFontMini( McrPlayEdt[i] );
	}

	for ( int i=0; i<4; i++ ){
		int y = 155 + i*25;

		wsprintf( buff, L"Macro%d", i );
		McrPlayLst[i] = CreateWindow( L"COMBOBOX", NULL, WSD_COMBOBOX, 145, y, 105, 300, hWnd, (HMENU)(i+780), hInst, NULL );
		McrPlayMno[i] = CreateWindow( L"COMBOBOX", NULL, WSD_COMBOBOX, 255, y,  75, 300, hWnd, (HMENU)(i+790), hInst, NULL );
		SetFontNorm( McrPlayLst[i] );
		SetFontNorm( McrPlayMno[i] );

		for ( int j=0; j<7; j++ ){
			SendMessage( McrPlayLst[i], CB_ADDSTRING, 0, (LPARAM)PlayModeLst[j] );
		}
		SendMessage( McrPlayLst[i], CB_SETCURSEL, SettingData.McrType[i], 0 );
		for ( int j=0; j<10; j++ ){
			wsprintf( buff, L"MacroNo%d", j );
			SendMessage( McrPlayMno[i], CB_ADDSTRING, 0, (LPARAM)buff );
		}
		SendMessage( McrPlayMno[i], CB_SETCURSEL, SettingData.McrPlayNo[i], 0 );
	}

	SetMacroButtonTxt();
	WmEnableMacroWindow();
	WmCreateSaveButton( hWnd );
	InvalidateRect( hWnd, NULL, TRUE );
}

/*------------------------------------------------------------------------------*/
/* WmCommandMacroConf															*/
/*------------------------------------------------------------------------------*/
void WmCommandMacroConf( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	switch ( id ){
	case 701:
		if ( SendMessage( McrRecUse, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.McrRecUse = 1;
			WmEnableMacroWindow();
		} else {
			SettingData.McrRecUse = 0;
			WmEnableMacroWindow();
		}
		break;
	case 702: {
		int no1 = SendMessage( McrRecNo, CB_GETCURSEL, 0, 0 );
		SettingData.McrRecNo = no1;
		break;
	}
	case 703:
		if ( SendMessage( McrRecAna, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.McrRecAna = 1;
		} else {
			SettingData.McrRecAna = 0;
		}
		break;
	case 711:
		if ( SendMessage( McrPlyChk, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.McrPlayChk = 1;
		} else {
			SettingData.McrPlayChk = 0;
		}
		break;
	case 750 ... 754:
		SettingData.McrButton[id - 750] = SettingButton();
		SetMacroButtonTxt();
		break;
	case 780 ... 783: {
		int no2 = SendMessage( McrPlayLst[id-780], CB_GETCURSEL, 0, 0 );
		SettingData.McrType[id-780] = no2;
		break;
	}
	case 790 ... 793: {
		int no3 = SendMessage( McrPlayMno[id-790], CB_GETCURSEL, 0, 0 );
		SettingData.McrPlayNo[id-790] = no3;
		break;
	}
	}
}
