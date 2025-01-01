#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Movie																		*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include <vfw.h>
#include "Bitmap.h"
#include "Movie.h"
#include "Direct3D.h"
#include "DirectInput.h"
#include "Setting.h"

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static unsigned int TopCount;
static unsigned int PreCount;
static PAVIFILE				pAviF;
static PAVISTREAM			pAviS;
static AVISTREAMINFO		AviSInfo;
static AVICOMPRESSOPTIONS	AviOption;

/*------------------------------------------------------------------------------*/
/* Error																		*/
/*------------------------------------------------------------------------------*/
static void Error( int no, HRESULT hRes )
{
	WCHAR Message[256];

	wsprintf( Message, L"Movie Error%d (0x%08X)", no, (int)hRes );
	MessageBox( NULL, Message, L"RemoteJoyLite", MB_OK );
}

/*------------------------------------------------------------------------------*/
/* Movie_Init																	*/
/*------------------------------------------------------------------------------*/
int Movie_Init( char *file_name, PSP_BITMAP *bmp, unsigned int vcount )
{
	HRESULT		hRes;
	PAVISTREAM	pAviSTmp;
	char		FileName[256];
	DWORD		dwFlagsTmp = 0;

	AVIFileInit();
	sprintf( FileName, "%s.avi", file_name );
	hRes = AVIFileOpenA( &pAviF, FileName, OF_CREATE|OF_WRITE, NULL );
	if ( hRes != AVIERR_OK ){ Error( 0, hRes ); return( -1 ); }

	ZeroMemory( &AviSInfo, sizeof(AVISTREAMINFO) );
	AviSInfo.fccType               = streamtypeVIDEO;
	AviSInfo.fccHandler            = mmioFOURCC( 'D', 'I', 'B', ' ' );
	AviSInfo.dwScale               = 1;
	AviSInfo.dwRate                = 60/(SettingData.MovieFPS + 1);
	AviSInfo.dwSuggestedBufferSize = BITMAP_SIZE;
	SetRect( &AviSInfo.rcFrame, 0, 0, BITMAP_W, BITMAP_H );
	hRes = AVIFileCreateStream( pAviF, &pAviS, &AviSInfo );
	if ( hRes != AVIERR_OK ){ Error( 1, hRes ); return( -1 ); }

	if ( SettingData.Movie.lDataRate > 0 ){ dwFlagsTmp |= AVICOMPRESSF_DATARATE;  }
	if ( SettingData.Movie.lKey      > 0 ){ dwFlagsTmp |= AVICOMPRESSF_KEYFRAMES; }
	AviSInfo.fccHandler         = SettingData.Movie.fccHandler;
	AviOption.fccType           = streamtypeVIDEO;
	AviOption.fccHandler        = SettingData.Movie.fccHandler;
	AviOption.dwKeyFrameEvery   = SettingData.Movie.lKey;
	AviOption.dwQuality         = SettingData.Movie.lQ;
	AviOption.dwBytesPerSecond  = SettingData.Movie.lDataRate;
	AviOption.dwFlags           = dwFlagsTmp;
	AviOption.lpFormat          = NULL;
	AviOption.cbFormat          = 0;
	AviOption.lpParms           = SettingData.Movie.lpState;
	AviOption.cbParms           = SettingData.Movie.cbState;
	AviOption.dwInterleaveEvery = 0;
	hRes = AVIMakeCompressedStream( &pAviSTmp, pAviS, &AviOption, NULL );
	if ( hRes != AVIERR_OK ){ Error( 2, hRes ); return( -1 ); }

	AVIStreamClose( pAviS );
	pAviS = pAviSTmp;

	hRes = AVIStreamSetFormat( pAviS, 0, (LPBYTE)(bmp->IHeader), sizeof(BITMAPINFOHEADER) );
	if ( hRes != AVIERR_OK ){ Error( 3, hRes ); return( -1 ); }
	TopCount = vcount;
	PreCount = 0;
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* Movie_Save																	*/
/*------------------------------------------------------------------------------*/
void Movie_Save( PSP_BITMAP *bmp, unsigned int vcount )
{
	HRESULT		hRes;
	unsigned int		FrameCount;
	unsigned int		DiffCount  = vcount - PreCount;

	if ( DiffCount < (SettingData.MovieFPS + 1) ){ return; }
	PreCount   = vcount;
	FrameCount = (vcount - TopCount)/(SettingData.MovieFPS + 1);

	hRes = AVIStreamWrite( pAviS, FrameCount, 1, (LPBYTE)(bmp->IHeader) + sizeof(BITMAPINFOHEADER),
						   BITMAP_SIZE, AVIIF_KEYFRAME, NULL, NULL );
}

/*------------------------------------------------------------------------------*/
/* Movie_End																	*/
/*------------------------------------------------------------------------------*/
void Movie_End( void )
{
	AVIStreamClose( pAviS );
	AVIFileClose( pAviF );
	AVIFileExit();
}
