#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Macro																		*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include "DebugFont.h"
#include "RemoteJoyLite.h"
#include "Macro.h"
#include "Setting.h"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
typedef struct {
	int		time;
	int		button;
	int		analog;
} MACRO_DATA;

#define PSP_SCREEN_W		480
#define PSP_SCREEN_H		272
#define BOTTOM_POS(y)		(SettingData.DispRot&1)?(PSP_SCREEN_W-7*(y)):(PSP_SCREEN_H-7*(y))

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static FILE *RecodeFile;
static int RecodeFlag = 0;
static int RecodeTime = 0;
static int RecodeButton = 0;
static int RecodeAxisXY = 0xFFFFFFFF;
static int MactoDataNum[4];
static MACRO_DATA *MactoData[4] = { NULL, NULL, NULL, NULL };
static int PlayFlag[4] = { 0, 0, 0, 0 };
static int Playtime[4] = { 0, 0, 0, 0 };
static int PlayPos[4]  = { 0, 0, 0, 0 };

/*------------------------------------------------------------------------------*/
/* MacroRecode																	*/
/*------------------------------------------------------------------------------*/
static void MacroRecode( void )
{
	dprintf( 0, BOTTOM_POS(1), "REC MACRO" );

	BOOL flag  = FALSE;
	int Button = RemoteJoyLite_GetButton();
	int AxisXY = RemoteJoyLite_GetAxisXY();

	if ( SettingData.McrRecAna == 0 ){ AxisXY = 0xFFFFFFFF; }
	if ( RecodeButton != Button ){ flag = TRUE; }
	if ( RecodeAxisXY != AxisXY ){ flag = TRUE; }

	RecodeTime++;
	if ( flag == TRUE ){
		fprintf( RecodeFile, "%d\t%08X\t%08X\n", RecodeTime, RecodeButton, RecodeAxisXY );
		RecodeTime = 0;
		RecodeButton = Button;
		RecodeAxisXY = AxisXY;
	}
}

/*------------------------------------------------------------------------------*/
/* MacroStart																	*/
/*------------------------------------------------------------------------------*/
static void MacroStart( int no )
{
	PlayFlag[no] = 1;
	Playtime[no] = 0;
	PlayPos[no] = 0;
}

/*------------------------------------------------------------------------------*/
/* MacroSetData																	*/
/*------------------------------------------------------------------------------*/
static void MacroSetData( long long cncl, int mcr_btn, int mcr_ana )
{
	int Cancel = 0;
	int Button = RemoteJoyLite_GetButton();

	for ( int i=0; i<17; i++ ){
		int mask = (1 << ButtonIndex[i]);
		if ( cncl & SettingData.JoyConf[i] ){ Cancel |= mask; }
	}

	Button &= ~Cancel;
	Button |= mcr_btn;

	RemoteJoyLite_SetButton( Button );
	if ( mcr_ana != (int)0xFFFFFFFF ){
		RemoteJoyLite_SetAxis( mcr_ana, mcr_ana >> 16 );
	}
}

/*------------------------------------------------------------------------------*/
/* MacroPlay																	*/
/*------------------------------------------------------------------------------*/
static void MacroPlay( AkindDI *pMainDI )
{
	long long cncl = 0;
	long long push = pMainDI->GetJoyButton( SettingData.JoyNo );
	long long trig = pMainDI->GetJoyTrigger( SettingData.JoyNo );

	for ( int i=0; i<4; i++ ){
		if ( SettingData.McrType[i] == 0 ){ continue; }
		long long comb_mask = push & SettingData.McrButton[0];
		long long push_mask = push & SettingData.McrButton[i+1];
		long long trig_mask = trig & SettingData.McrButton[i+1];

		switch ( SettingData.McrType[i] ){
		case 2: /* CB-PUSH */
			if ( comb_mask == 0 ){ PlayFlag[i] = 0; break; }
			cncl |= comb_mask;
		case 1: /* SB-PUSH */
			if ( push_mask ){
				if ( PlayFlag[i] == 0 ){ MacroStart( i ); }
				cncl |= push_mask;
			} else {
				PlayFlag[i] = 0;
			}
			break;
		case 4: /* CB-TGL */
		case 6: /* CB-TGL-LOOP */
			if ( comb_mask == 0 ){ break; }
			cncl |= comb_mask;
		case 3: /* SB-TGL */
		case 5: /* SB-TGL-LOOP */
			if ( trig_mask ){
				if ( PlayFlag[i] == 0 ){ MacroStart( i ); }
				else				   { PlayFlag[i] = 0; }
				cncl |= trig_mask;
			}
			break;
		}
	}

	int mcr_btn = 0;
	int mcr_ana = 0xFFFFFFFF;
	int debug_pos = 1;

	for ( int i=0; i<4; i++ ){
		if ( PlayFlag[i] == 0 ){ continue; }
		MACRO_DATA *macro_p = MactoData[i];
		if ( macro_p == NULL ){ continue; }
		macro_p += PlayPos[i];
		int button = macro_p->button;
		int analog = macro_p->analog;

		mcr_btn |= button;
		if ( analog != (int)0xFFFFFFFF ){ mcr_ana = analog; }
		if ( SettingData.McrPlayChk != 0 ){
			char buff[256];
			if ( analog != (int)0xFFFFFFFF ){
				sprintf( buff, "[%d %08X %08X]", i, button, analog );
			} else {
				sprintf( buff, "[%d %08X]", i, button );
			}
			dprintf( 0, BOTTOM_POS(debug_pos++), buff );
		}
		Playtime[i]++;
		if ( Playtime[i] >= macro_p->time ){
			Playtime[i] = 0;
			PlayPos[i]++;
			if ( PlayPos[i] >= MactoDataNum[i] ){
				PlayPos[i] = 0;
				if ( (SettingData.McrType[i] == 3) || (SettingData.McrType[i] == 4) ){
					PlayFlag[i] = 0;
				}
			}
		}
	}

	MacroSetData( cncl, mcr_btn, mcr_ana );
}

/*------------------------------------------------------------------------------*/
/* MacroSync																	*/
/*------------------------------------------------------------------------------*/
void MacroSync( AkindDI *pMainDI )
{
	if ( SettingFlag() != FALSE ){ return; }
	if ( RecodeFlag != 0 ){
		MacroRecode();
	} else {
		if ( (SettingData.McrType[0] != 0) || (SettingData.McrType[1] != 0) ||
			 (SettingData.McrType[2] != 0) || (SettingData.McrType[3] != 0) ){
			MacroPlay( pMainDI );
		}
	}
}

/*------------------------------------------------------------------------------*/
/* MacroRecodeToggle															*/
/*------------------------------------------------------------------------------*/
void MacroRecodeToggle( void )
{
	if ( SettingData.McrRecUse == 0 ){ return; }
	RecodeFlag ^= 1;

	if ( RecodeFlag != 0 ){
		char file_name[256];
		sprintf( file_name, ".\\Macro\\MacroNo%d.txt", SettingData.McrRecNo );
		RecodeFile = fopen( file_name, "w" );
		if ( RecodeFile == NULL ){ RecodeFlag = 0; }
		RecodeTime = 0;
		RecodeButton = 0;
		RecodeAxisXY = 0xFFFFFFFF;
	} else {
		fprintf( RecodeFile, "%d\t%08X\t%08X\n", RecodeTime, RecodeButton, RecodeAxisXY );
		fclose( RecodeFile );
	}
}

/*------------------------------------------------------------------------------*/
/* MacroRecodeCheck																*/
/*------------------------------------------------------------------------------*/
BOOL MacroRecodeCheck( void )
{
	if ( RecodeFlag == 1 ){ return( TRUE ); }
	return( FALSE );
}

/*------------------------------------------------------------------------------*/
/* MallocError																	*/
/*------------------------------------------------------------------------------*/
static void MallocError( void )
{
	MessageBox( NULL, L"MacroLoad Malloc Error", L"RemoteJoyLite", MB_OK );
}

/*------------------------------------------------------------------------------*/
/* MacroLoad																	*/
/*------------------------------------------------------------------------------*/
void MacroLoad( void )
{
	int  line_no;
	char line_buff[256];
	char file_name[256];
	int  macro_no;
	MACRO_DATA *macro_p;

	for ( int i=0; i<4; i++ ){
		if ( MactoData[i] != NULL ){ free( MactoData[i] ); }
		MactoDataNum[i] = 0;
		MactoData[i] = NULL;
		PlayFlag[i] = 0;
		if ( SettingData.McrType[i] == 0 ){ continue; }

		sprintf( file_name, ".\\Macro\\MacroNo%d.txt", SettingData.McrPlayNo[i] );
		FILE *fp = fopen( file_name, "r" );
		if ( fp == NULL ){ continue; }

		line_no = 0;
		while ( fgets( line_buff, 256, fp ) != NULL ){ line_no++; }

		macro_p = (MACRO_DATA *)malloc( sizeof(MACRO_DATA)*line_no );
		if ( macro_p == NULL ){
			MallocError();
		} else {
			macro_no = 0;
			fseek( fp, 0, SEEK_SET );
			while ( fgets( line_buff, 256, fp ) != NULL ){
				int time, button, analog;
				sscanf( line_buff, "%d %X %X", &time, &button, &analog );
				macro_p[macro_no].time   = time;
				macro_p[macro_no].button = button;
				macro_p[macro_no].analog = analog;
				macro_no++;
			}
			MactoDataNum[i] = macro_no;
			MactoData[i] = macro_p;
		}
		fclose( fp );
	}
}
