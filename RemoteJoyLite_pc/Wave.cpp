#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Movie																		*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <vfw.h>
#include "Direct3D.h"
#include "DirectInput.h"
#include "DebugFont.h"
#include "Setting.h"

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define WAVEBUFFNUM				8

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static int WaveOutNextNo = 0;
static int WaveOutLastNo = 0;
static WAVEHDR WaveBuff[WAVEBUFFNUM];
static WAVEFORMATEX WaveFormat;

static HMMIO hWaveRec;
static MMCKINFO WaveRiff;
static MMCKINFO WaveData;
static MMCKINFO WaveFmt;
static int WaveOutFlag = 0;

/*------------------------------------------------------------------------------*/
/* WaveWorkInit																	*/
/*------------------------------------------------------------------------------*/
static void WaveWorkInit( void )
{
	WaveOutNextNo = 0;
	WaveOutLastNo = 0;
	WaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
	WaveFormat.nChannels       = 2;
	WaveFormat.wBitsPerSample  = 16;
	WaveFormat.nBlockAlign     = 2*16/8;
	WaveFormat.nSamplesPerSec  = 44100;
	WaveFormat.cbSize          = 0;
	WaveFormat.nAvgBytesPerSec = 44100*(2*16/8);
	for ( int i=0; i<WAVEBUFFNUM; i++ ){
		WaveBuff[i].lpData         = (CHAR *)malloc(44100*(2*16/8)/30);
		WaveBuff[i].dwBufferLength = 44100*(2*16/8)/30;
		WaveBuff[i].dwUser         = 0;
		WaveBuff[i].dwFlags        = WHDR_BEGINLOOP | WHDR_ENDLOOP;
		WaveBuff[i].dwLoops        = 0;
		WaveBuff[i].lpNext         = 0;
		WaveBuff[i].reserved       = 0;
	}
}

/********************************************************************************/
/* WaveIn																		*/
/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define WAVEINMAX		32

enum {
	WAVEINSTATUS_INIT,
	WAVEINSTATUS_OPEN,
	WAVEINSTATUS_CLOSE,
};

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static int WaveInNum = 0;
static int WaveInUse = WAVE_MAPPER;
static int WaveInStep = 0;
static int WaveInErrorStep = 0;
static MMRESULT WaveInErrorRes = 0;
static int WaveInStatus = WAVEINSTATUS_INIT;
static HWAVEIN HdWaveIn = NULL;
static WAVEINCAPS WaveInCaps[WAVEINMAX];

/*------------------------------------------------------------------------------*/
/* WaveInInit																	*/
/*------------------------------------------------------------------------------*/
static void WaveInInit( void )
{
	MMRESULT	mmRes;

	WaveInNum = waveInGetNumDevs();
	if ( WaveInNum >= WAVEINMAX ){ WaveInNum = WAVEINMAX-1; }
	for ( int i=0; i<WaveInNum; i++ ){
		mmRes = waveInGetDevCaps( i, &WaveInCaps[i], sizeof(WAVEINCAPS) );
		if ( mmRes != MMSYSERR_NOERROR ){ WaveInCaps[i].wChannels = 0; }
	}
	mmRes = waveInGetDevCaps( WAVE_MAPPER, &WaveInCaps[WAVEINMAX-1], sizeof(WAVEINCAPS) );
	if ( mmRes != MMSYSERR_NOERROR ){ WaveInCaps[WAVEINMAX-1].wChannels = 0; }
}

/*------------------------------------------------------------------------------*/
/* WaveInProc																	*/
/*------------------------------------------------------------------------------*/
static VOID CALLBACK WaveInProc( HWAVEIN hwi, UINT msg, DWORD inst, DWORD dwP1, DWORD dwP2 )
{
	switch ( msg ){
	case WIM_DATA  : ((WAVEHDR *)dwP1)->dwUser |= 0x0001;	break;
	case WIM_OPEN  : WaveInStatus = WAVEINSTATUS_OPEN;		break;
	case WIM_CLOSE : WaveInStatus = WAVEINSTATUS_CLOSE;		break;
	}
}

/*------------------------------------------------------------------------------*/
/* WaveInErrorCheck																*/
/*------------------------------------------------------------------------------*/
static BOOL WaveInErrorCheck( MMRESULT mmRes )
{
	if ( mmRes != MMSYSERR_NOERROR ){
		WaveInErrorStep = WaveInStep;
		WaveInErrorRes  = mmRes;
		WaveInStep = -1;
		return( TRUE );
	}
	return( FALSE );
}

/*------------------------------------------------------------------------------*/
/* WaveInSync																	*/
/*------------------------------------------------------------------------------*/
static void WaveInSync( void )
{
	MMRESULT	mmRes;

	switch ( WaveInStep ){
	case -1:
		dprintf( 0, 0, "WaveIn Error%d (0x%08X)", WaveInErrorStep, (int)WaveInErrorRes );
		break;
	case 0:
		if ( SettingData.WAVOut != 0 ){ WaveInStep = 1; }
		break;
	case 1:
		mmRes = waveInOpen( &HdWaveIn, WaveInUse, &WaveFormat, (DWORD_PTR)WaveInProc, 0, CALLBACK_FUNCTION );
		if ( WaveInErrorCheck( mmRes ) != FALSE ){ return; }
		WaveInStep = 2;
		break;
	case 2:
		if ( WaveInStatus == WAVEINSTATUS_OPEN ){ WaveInStep = 3; }
		break;
	case 3:
		WaveOutNextNo = 0;
		WaveOutLastNo = 0;
		for ( int i=0; i<WAVEBUFFNUM; i++ ){
			WaveBuff[i].dwUser = 0;
			mmRes = waveInPrepareHeader( HdWaveIn, &WaveBuff[i],sizeof(WAVEHDR) );
			if ( WaveInErrorCheck( mmRes ) != FALSE ){ return; }
			mmRes = waveInAddBuffer( HdWaveIn, &WaveBuff[i],sizeof(WAVEHDR) );
			if ( WaveInErrorCheck( mmRes ) != FALSE ){ return; }
		}
		mmRes = waveInStart( HdWaveIn );
		if ( WaveInErrorCheck( mmRes ) != FALSE ){ return; }
		WaveInStep = 4;
		break;
	case 4:
		if ( SettingData.WAVOut == 0 ){ WaveInStep = 5; }
		break;
	case 5:
		waveInStop( HdWaveIn );
		waveInReset( HdWaveIn );
		for ( int i=0; i<WAVEBUFFNUM; i++ ){
			waveInUnprepareHeader( HdWaveIn, &WaveBuff[i], sizeof(WAVEHDR) );
		}
		waveInClose( HdWaveIn );
		WaveInStep = 0;
		break;
	}
}

/********************************************************************************/
/* WaveOut																		*/
/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define WAVEOUTMAX		32

enum {
	WAVEOUTSTATUS_INIT,
	WAVEOUTSTATUS_OPEN,
	WAVEOUTSTATUS_CLOSE,
};

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static int WaveOutNum = 0;
static int WaveOutUse = WAVE_MAPPER;
static int WaveOutStep = 0;
static int WaveOutErrorStep = 0;
static MMRESULT WaveOutErrorRes = 0;
static int WaveOutStatus = WAVEOUTSTATUS_INIT;
static HWAVEOUT HdWaveOut = NULL;
static WAVEOUTCAPS WaveOutCaps[WAVEOUTMAX];

/*------------------------------------------------------------------------------*/
/* WaveOutInit																	*/
/*------------------------------------------------------------------------------*/
static void WaveOutInit( void )
{
	MMRESULT	mmRes;

	WaveOutNum = waveOutGetNumDevs();
	if ( WaveOutNum >= WAVEOUTMAX ){ WaveOutNum = WAVEOUTMAX-1; }
	for ( int i=0; i<WaveOutNum; i++ ){
		mmRes = waveOutGetDevCaps( i, &WaveOutCaps[i], sizeof(WAVEOUTCAPS) );
		if ( mmRes != MMSYSERR_NOERROR ){ WaveOutCaps[i].wChannels = 0; }
	}
	mmRes = waveOutGetDevCaps( WAVE_MAPPER, &WaveOutCaps[WAVEOUTMAX-1], sizeof(WAVEOUTCAPS) );
	if ( mmRes != MMSYSERR_NOERROR ){ WaveOutCaps[WAVEOUTMAX-1].wChannels = 0; }
}

/*------------------------------------------------------------------------------*/
/* WaveOutProc																	*/
/*------------------------------------------------------------------------------*/
static VOID CALLBACK WaveOutProc( HWAVEOUT hwo, UINT msg, DWORD inst, DWORD dwP1, DWORD dwP2 )
{
	switch ( msg ){
	case WOM_DONE  : ((WAVEHDR *)dwP1)->dwUser |= 0x0010;	break;
	case WOM_OPEN  : WaveOutStatus = WAVEOUTSTATUS_OPEN;	break;
	case WOM_CLOSE : WaveOutStatus = WAVEOUTSTATUS_CLOSE;	break;
	}
}

/*------------------------------------------------------------------------------*/
/* WaveOutErrorCheck															*/
/*------------------------------------------------------------------------------*/
static BOOL WaveOutErrorCheck( MMRESULT mmRes )
{
	if ( mmRes != MMSYSERR_NOERROR ){
		WaveOutErrorStep = WaveOutStep;
		WaveOutErrorRes  = mmRes;
		WaveOutStep = -1;
		return( TRUE );
	}
	return( FALSE );
}

/*------------------------------------------------------------------------------*/
/* WaveOutSync																	*/
/*------------------------------------------------------------------------------*/
static void WaveOutSync( void )
{
	MMRESULT	mmRes;

	switch ( WaveOutStep ){
	case -1:
		dprintf( 0, 7, "WaveOut Error%d (0x%08X)", WaveOutErrorStep, (int)WaveOutErrorRes );
		break;
	case 0:
		if ( SettingData.WAVOut != 0 ){ WaveOutStep = 1; }
		break;
	case 1:
		mmRes = waveOutOpen( &HdWaveOut, WaveOutUse, &WaveFormat, (DWORD_PTR)WaveOutProc, 0, CALLBACK_FUNCTION );
		if ( WaveOutErrorCheck( mmRes ) != FALSE ){ return; }
		WaveOutStep = 2;
		break;
	case 2:
		if ( WaveInStep != 4 ){ break; }
		if ( WaveOutStatus == WAVEOUTSTATUS_OPEN ){ WaveOutStep = 3; }
		break;
	case 3:
		if ( SettingData.WAVOut == 0 ){ WaveOutStep = 4; break; }
		while ( WaveBuff[WaveOutNextNo].dwUser & 0x0001 ){
			mmRes = waveInUnprepareHeader( HdWaveIn, &WaveBuff[WaveOutNextNo], sizeof(WAVEHDR) );
			if ( WaveOutFlag != 0 ){
				mmioWrite( hWaveRec, (char *)WaveBuff[WaveOutNextNo].lpData,
						   WaveBuff[WaveOutNextNo].dwBufferLength );
			}
			int diff = (WaveOutNextNo+WAVEBUFFNUM - WaveOutLastNo)%WAVEBUFFNUM;
			if ( diff <= WAVEBUFFNUM/2 ){
				mmRes = waveOutPrepareHeader( HdWaveOut, &WaveBuff[WaveOutNextNo], sizeof(WAVEHDR) );
				mmRes = waveOutWrite( HdWaveOut, &WaveBuff[WaveOutNextNo], sizeof(WAVEHDR) );
				WaveBuff[WaveOutNextNo].dwUser |= 0x0020;
			} else {
				WaveBuff[WaveOutNextNo].dwUser |= 0x0010;
			}
			WaveBuff[WaveOutNextNo].dwUser &= ~0x0001;
			WaveOutNextNo = (WaveOutNextNo + 1 ) % WAVEBUFFNUM;
		}
		while ( WaveBuff[WaveOutLastNo].dwUser & 0x0010 ){
			if ( WaveBuff[WaveOutNextNo].dwUser & 0x0020 ){
				mmRes = waveOutUnprepareHeader( HdWaveOut, &WaveBuff[WaveOutLastNo], sizeof(WAVEHDR) );
			}
			mmRes = waveInPrepareHeader( HdWaveIn, &WaveBuff[WaveOutLastNo], sizeof(WAVEHDR) );
			mmRes = waveInAddBuffer( HdWaveIn, &WaveBuff[WaveOutLastNo], sizeof(WAVEHDR) );
			WaveBuff[WaveOutLastNo].dwUser &= ~0x0030;
			WaveOutLastNo = (WaveOutLastNo + 1 ) % WAVEBUFFNUM;
		}
		break;
	case 4:
		waveOutReset( HdWaveOut );
		for ( int i=0; i<WAVEBUFFNUM; i++ ){
			waveOutUnprepareHeader( HdWaveOut, &WaveBuff[i], sizeof(WAVEHDR) );
		}
		waveOutClose( HdWaveOut );
		WaveOutStep = 0;
		break;
	}
}

/********************************************************************************/
/* Wave																			*/
/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* WaveInit																		*/
/*------------------------------------------------------------------------------*/
void WaveInit( void )
{
	WaveWorkInit();
	WaveInInit();
	WaveOutInit();
}

/*------------------------------------------------------------------------------*/
/* WaveSync																		*/
/*------------------------------------------------------------------------------*/
void WaveSync( void )
{
	WaveInSync();
	WaveOutSync();
}

/*------------------------------------------------------------------------------*/
/* WaveExit																		*/
/*------------------------------------------------------------------------------*/
void WaveExit( void )
{
	WaveOutStep = -1;
	WaveInStep = -1;
}

/*------------------------------------------------------------------------------*/
/* WaveOpen																		*/
/*------------------------------------------------------------------------------*/
void WaveOpen( char *file_name )
{
	char		FileName[256];
	sprintf( FileName, "%s.wav", file_name );

	hWaveRec = mmioOpenA( FileName, NULL, MMIO_CREATE|MMIO_WRITE );
	WaveRiff.fccType = mmioStringToFOURCC( TEXT("WAVE"), 0 );
	mmioCreateChunk( hWaveRec, &WaveRiff, MMIO_CREATERIFF );
	WaveFmt.ckid = mmioStringToFOURCC( TEXT("fmt "), 0 );
	mmioCreateChunk( hWaveRec, &WaveFmt, 0 );
	mmioWrite( hWaveRec, (char *)&WaveFormat, sizeof(WAVEFORMATEX) );
	mmioAscend( hWaveRec, &WaveFmt, 0 );
	WaveData.ckid = mmioStringToFOURCC( TEXT("data"), 0 );
	mmioCreateChunk( hWaveRec, &WaveData, 0 );
	WaveOutFlag = 1;
}

/*------------------------------------------------------------------------------*/
/* WaveClose																	*/
/*------------------------------------------------------------------------------*/
void WaveClose( void )
{
	mmioAscend( hWaveRec, &WaveData, 0 );
	mmioAscend( hWaveRec, &WaveRiff, 0 );
	mmioClose( hWaveRec, 0 );
	WaveOutFlag = 0;
}
