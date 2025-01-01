#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Setting_Key																	*/
/*------------------------------------------------------------------------------*/
#include "Setting.h"
#include "Setting_Key.h"
#include "Setting_Key.dat"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define WSD_TEXT		(WS_CHILD|WS_VISIBLE|SS_CENTER)
#define WSD_GROUPBOX	(WS_CHILD|WS_VISIBLE|WS_GROUP|BS_GROUPBOX)
#define WSD_COMBOBOX	(WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST|WS_TABSTOP)

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static HWND KeyConfTxt[21];
static HWND KeyConfBox[21];
static HWND KeyConfLst[21];
static HWND KeyConfUse;

/*------------------------------------------------------------------------------*/
/* WmDestroyKeyConf																*/
/*------------------------------------------------------------------------------*/
void WmDestroyKeyConf( HWND hWnd )
{
	for ( int i=0; i<21; i++ ){
		DestroyWindow( KeyConfTxt[i] );
		DestroyWindow( KeyConfBox[i] );
		DestroyWindow( KeyConfLst[i] );
	}
	DestroyWindow( KeyConfUse );
	WmDestroySaveButton( hWnd );
}

/*------------------------------------------------------------------------------*/
/* WmCreateKeyConf																*/
/*------------------------------------------------------------------------------*/
void WmCreateKeyConf( HWND hWnd, HINSTANCE hInst )
{
	/* Key Conf */
	for ( int i=0; i<21; i++ ){
		WCHAR *name = ButtonName[ButtonIndex[i]];
		int dik = 0;
		int x = 22 + (i/11)*165;
		int y = 40 + (i%11)*22;

		KeyConfTxt[i] = CreateWindow( L"STATIC",   name, WSD_TEXT,        x, y+5, 60,  21, hWnd, HMENU(i+200), hInst, NULL );
		KeyConfBox[i] = CreateWindow( L"BUTTON",   NULL, WSD_GROUPBOX,    x, y-9, 60,  31, hWnd, HMENU(i+220), hInst, NULL );
		KeyConfLst[i] = CreateWindow( L"COMBOBOX", NULL, WSD_COMBOBOX, x+70, y+1, 80, 300, hWnd, HMENU(i+240), hInst, NULL );

		if ( SettingData.KeyUse == 0 ){ EnableWindow( KeyConfLst[i], FALSE ); }
		SetFontNorm( KeyConfTxt[i] );
		SetFontNorm( KeyConfLst[i] );
		for ( int j=0; j<KEY_NAME_SIZE; j++ ){
			SendMessage( KeyConfLst[i], CB_ADDSTRING, 0, (LPARAM)KeyNameTable[j].name );
			if ( KeyNameTable[j].dik == SettingData.KeyConf[i] ){ dik = j; }
		}
		SendMessage( KeyConfLst[i], CB_SETCURSEL, dik, 0 );
	}
	/* Key Use */
	KeyConfUse = MyCreateWindow( &CWD_KeyConfUse, hWnd, hInst );
	if ( SettingData.KeyUse != 0 ){
		SendMessage( KeyConfUse, BM_SETCHECK, (WPARAM)BST_CHECKED, 0 );
	}

	WmCreateSaveButton( hWnd );
	InvalidateRect( hWnd, NULL, TRUE );
}

/*------------------------------------------------------------------------------*/
/* WmCommandKeyConf																*/
/*------------------------------------------------------------------------------*/
void WmCommandKeyConf( HWND hWnd, WORD code, WORD id, HWND hCtl )
{
	switch ( id ){
	case 240 ... 260: {
		int no = SendMessage( KeyConfLst[id-240], CB_GETCURSEL, 0, 0 );
		SettingData.KeyConf[id-240] = KeyNameTable[no].dik;
		break;
	}
	case 270:
		if ( SendMessage( KeyConfUse, BM_GETCHECK, 0, 0 ) == BST_CHECKED ){
			SettingData.KeyUse = 1;
			for ( int i=0; i<21; i++ ){ EnableWindow( KeyConfLst[i], TRUE ); }
		} else {
			SettingData.KeyUse = 0;
			for ( int i=0; i<21; i++ ){ EnableWindow( KeyConfLst[i], FALSE ); }
		}
		break;
	}
}
