#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Bitmap																		*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include "Bitmap.h"

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static char FileName[256];

BITMAPINFOHEADER PspBitmapBase = {
	sizeof(BITMAPINFOHEADER),	// biSize
	BITMAP_W,					// biWidth
	BITMAP_H,					// biHeight
	1,							// biPlanes
	24,							// biBitCount
	BI_RGB,						// biCompression
	BITMAP_SIZE,				// biSizeImage
	0,							// biXPelsPerMeter
	0,							// biYPelsPerMeter
	0,							// biClrUsed
	0							// biClrImportant
};

/*------------------------------------------------------------------------------*/
/* Bitmap_Init																	*/
/*------------------------------------------------------------------------------*/
void Bitmap_Init( PSP_BITMAP *bmp )
{
	BITMAPFILEHEADER	*bmf = (BITMAPFILEHEADER *)(bmp->FHeader);
	BITMAPINFOHEADER	*bmi = (BITMAPINFOHEADER *)(bmp->IHeader);

	ZeroMemory( bmf, sizeof(BITMAPFILEHEADER) );
	ZeroMemory( bmi, sizeof(BITMAPINFOHEADER) );
	bmf->bfType        = *(LPWORD)"BM";
	bmf->bfSize        = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + BITMAP_SIZE;
	bmf->bfOffBits     = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmi->biSize        = sizeof(BITMAPINFOHEADER);
	bmi->biWidth       = BITMAP_W;
	bmi->biHeight      = BITMAP_H;
	bmi->biPlanes      = 1;
	bmi->biBitCount    = 24;
	bmi->biCompression = BI_RGB;
	bmi->biSizeImage   = BITMAP_SIZE;
}

/*------------------------------------------------------------------------------*/
/* Bitmap_Name																	*/
/*------------------------------------------------------------------------------*/
void Bitmap_Name( char *file_name )
{
	for ( int i=0; i<100; i++ ){
		sprintf( FileName, "%s_%x.bmp", file_name, i );
		if ( access( FileName, 0 ) != 0 ){ break; }
	}
}

/*------------------------------------------------------------------------------*/
/* Bitmap_Save																	*/
/*------------------------------------------------------------------------------*/
void Bitmap_Save( PSP_BITMAP *bmp )
{
	HANDLE	hFile;
	DWORD	dwResult;

	hFile = CreateFileA( FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE ){ return; }

	WriteFile( hFile, bmp, sizeof(PSP_BITMAP), &dwResult, NULL );
	CloseHandle( hFile );
}
