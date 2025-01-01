#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* DebugFont																	*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <stdarg.h>
#include "DebugFont.h"

/*------------------------------------------------------------------------------*/
/* FontData																		*/
/*------------------------------------------------------------------------------*/
extern DWORD FontData_5x5[];

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define DFONT_MAX	4096
#define PRIM_FVF	(D3DFVF_XYZRHW|D3DFVF_TEX1)

struct PRIM {
	float	x, y, z;
	float	rhw;
	float	u, v;
};

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static int  DFontNum = 0;
static WORD DFontIdx[DFONT_MAX*6];
static PRIM DFontBuf[DFONT_MAX*4];
static IDirect3DTexture9 *pD3DTex = NULL;

/*------------------------------------------------------------------------------*/
/* dprint																		*/
/*------------------------------------------------------------------------------*/
static void dprint( int x, int y, unsigned char str )
{
	int posw = (str - ' ')%16;
	int posh = (str - ' ')/16;
	PRIM *pBuf = &DFontBuf[DFontNum*4];
	int addx = 6;

	if ( DFontNum >= DFONT_MAX ){ return; }
	if ( str == 0x9E ){ addx = 3; }

	pBuf[0].x = pBuf[2].x = x;
	pBuf[1].x = pBuf[3].x = x+addx;
	pBuf[0].y = pBuf[1].y = y;
	pBuf[2].y = pBuf[3].y = y+7;
	pBuf[0].u = pBuf[2].u = ((float)(posw*16 +  1)/256.0f);
	pBuf[1].u = pBuf[3].u = ((float)(posw*16 + 13)/256.0f);
	pBuf[0].v = pBuf[1].v = ((float)(posh*16 +  1)/256.0f);
	pBuf[2].v = pBuf[3].v = ((float)(posh*16 + 15)/256.0f);
	DFontNum += 1;
}

/*------------------------------------------------------------------------------*/
/* dprintf																		*/
/*------------------------------------------------------------------------------*/
void dprintf( int x, int y, const char *arg, ... )
{
	char tmp[1024];
	char *p = tmp;
	int  tmpx = x;

	va_list ap;
	va_start( ap, arg );
	vsprintf( tmp, arg, ap );
	while ( *p != '\0' ){
		unsigned char str = (unsigned char)*p;
		dprint( x, y, str );
		if ( str == 0x9E ){ x += 3; }
		else if ( str == '\n' ){
			dprint( x, y, 0x9F );
			x = tmpx;
			y += 7;
		}
		else			  { x += 6; }
		p++;
	}
	dprint( x, y, 0x9F );
	va_end( ap );
}

/*------------------------------------------------------------------------------*/
/* Error																		*/
/*------------------------------------------------------------------------------*/
static void Error( int no, HRESULT hRes )
{
	WCHAR Message[256];

	wsprintf( Message, L"DebugFont Error%d (0x%08X)", no, (int)hRes );
	MessageBox( NULL, Message, L"RemoteJoyLite", MB_OK );
}

/*------------------------------------------------------------------------------*/
/* DebugFontInit																*/
/*------------------------------------------------------------------------------*/
BOOL DebugFontInit( AkindD3D *pAkindD3D )
{
	HRESULT hRes;
	D3DLOCKED_RECT lockRect;
	IDirect3DDevice9 *pD3DDev = pAkindD3D->getDevice();

	hRes = pD3DDev->CreateTexture( 128, 128, 1, 0, D3DFMT_A8R8G8B8,
								   D3DPOOL_MANAGED, &pD3DTex, NULL );

	if ( FAILED( hRes ) ){ Error( 0, hRes ); return( FALSE ); }

	pD3DTex->LockRect( 0, &lockRect, NULL, D3DLOCK_DISCARD );
	DWORD *src = FontData_5x5;
	DWORD *dst = (DWORD *)lockRect.pBits;
	for ( int y=0; y<64; y++ ){
		memcpy( dst, src, sizeof(DWORD)*128 );
		src += 128;
		dst += lockRect.Pitch/sizeof(DWORD);
	}
	pD3DTex->UnlockRect( 0 );

	for ( int i=0; i<DFONT_MAX; i++ ){
		DFontBuf[i*4+0].z   = 0;
		DFontBuf[i*4+0].rhw = 1.0f;
		DFontBuf[i*4+1].z   = 0;
		DFontBuf[i*4+1].rhw = 1.0f;
		DFontBuf[i*4+2].z   = 0;
		DFontBuf[i*4+2].rhw = 1.0f;
		DFontBuf[i*4+3].z   = 0;
		DFontBuf[i*4+3].rhw = 1.0f;
		DFontIdx[i*6+0] = i*4+0;
		DFontIdx[i*6+1] = i*4+1;
		DFontIdx[i*6+2] = i*4+2;
		DFontIdx[i*6+3] = i*4+3;
		DFontIdx[i*6+4] = i*4+2;
		DFontIdx[i*6+5] = i*4+1;
	}
	return( TRUE );
}

/*------------------------------------------------------------------------------*/
/* DebugFontExit																*/
/*------------------------------------------------------------------------------*/
void DebugFontExit( void )
{
	if ( pD3DTex != NULL ){
		pD3DTex->Release();
	}
}

/*------------------------------------------------------------------------------*/
/* DebugFontDraw																*/
/*------------------------------------------------------------------------------*/
void DebugFontDraw( AkindD3D *pAkindD3D )
{
	IDirect3DDevice9 *pD3DDev = pAkindD3D->getDevice();

	pD3DDev->SetTexture( 0, pD3DTex );
	pD3DDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	pD3DDev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	pD3DDev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	pD3DDev->SetFVF( PRIM_FVF );
	pD3DDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, DFontNum*4, DFontNum*2, DFontIdx,
									  D3DFMT_INDEX16, DFontBuf, sizeof(PRIM) );
	DFontNum = 0;
}
