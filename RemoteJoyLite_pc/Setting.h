#ifndef _SETTING_H_
#define _SETTING_H_
/*------------------------------------------------------------------------------*/
/* Setting																		*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <vfw.h>
#include "DirectInput.h"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define LANG_JA			0
#define LANG_EN			1

#define FNT_NONE		0
#define FNT_MINI		1
#define FNT_NORM		2
#define FNT_BOLD		3

#define CWD_TEXT		L"STATIC",(WS_CHILD|WS_VISIBLE)
#define CWD_GROUP		L"BUTTON",(WS_CHILD|WS_VISIBLE|WS_GROUP|BS_GROUPBOX)
#define CWD_BUTTON		L"BUTTON",(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_MULTILINE)
#define CWD_CHECK		L"BUTTON",(WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|WS_TABSTOP)
#define CWD_COMBO		L"COMBOBOX",(WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST|WS_TABSTOP)
#define CWD_EDIT		L"EDIT",(WS_CHILD|WS_VISIBLE|ES_READONLY|ES_MULTILINE)
#define CWD_NUMBER		L"EDIT",(WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER|ES_CENTER|WS_TABSTOP)
#define CWD_TABCTRL		WC_TABCONTROL,(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_TABSTOP)

typedef struct {
	int			x, y, w, h, id;
	LPCWSTR		lpClassName;
	DWORD		dwStyle;
	int			font;
	LPCWSTR		lpWindowName;
} CW_DATA;

/*------------------------------------------------------------------------------*/
/* SettingData																	*/
/*------------------------------------------------------------------------------*/
typedef struct {
	int				Version;
	int				JoyNo;
	long long		JoyConf[22];
	int				JoyAnalog;
	int				JoyMargin;
	int				KeyUse;
	int				KeyConf[21];
	int				DispAspe;
	int				DispTop;
	int				FullAdjust;
	int				FullRectX, FullRectY, FullRectW, FullRectH;
	int				DispSize;
	int				DispRot;
	int				DispRectX, DispRectY, DispRectW, DispRectH;
	int				DispOff;
	int				InputBG;
	int				GammaRGB;
	int				Gamma[4];
	int				AVIOut;
	unsigned int	MovieFPS;
	COMPVARS		Movie;
	int				WAVOut;
	int				PSPMode;
	int				PSPAdr1;
	int				PSPAdr2;
	int				PSPPri;
	int				PSPAsync;
	int				PSPFPS;
	int				PSPDisp;
	int				PSPRectX, PSPRectY, PSPRectW, PSPRectH;
	int				PSPDbg;
	int				McrRecUse;
	int				McrRecNo;
	int				McrRecAna;
	int				McrPlayChk;
	long long		McrButton[5];
	int				McrType[4];
	int				McrPlayNo[4];

	// post 019 additions
	int				JoyAnalogR;
} SETTING_DATA;

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
extern SETTING_DATA SettingData;
extern WCHAR *ButtonName[29];
extern WCHAR *DirectName[4];
extern int ButtonIndex[22];

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
extern void WmDestroySaveButton( HWND hWnd );
extern void WmCreateSaveButton( HWND hWnd );

extern void SettingLoad( void );
extern BOOL SettingInit( HWND hWnd, HINSTANCE hInst );
extern void SettingExit( void );
extern void SettingProc( UINT msg, WPARAM wParam, LPARAM lParam );
extern BOOL SettingMessage( MSG *msg, int FullScreen );
extern void SettingSync( AkindDI *pMainDI );
extern BOOL SettingFlag( void );
extern long long SettingButton( void );

extern void SetFontMini( HWND hWnd );
extern void SetFontNorm( HWND hWnd );
extern void SetFontBold( HWND hWnd );
extern HWND MyCreateWindow( CW_DATA *cwd, HWND hWnd, HINSTANCE hInst );

#endif	// _SETTING_H_
