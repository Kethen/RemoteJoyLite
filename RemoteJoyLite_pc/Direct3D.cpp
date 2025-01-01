#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* Direct3D																		*/
/*------------------------------------------------------------------------------*/
#include "Direct3D.h"
#include <stdio.h>

/*------------------------------------------------------------------------------*/
/* constructor																	*/
/*------------------------------------------------------------------------------*/
AkindD3D::AkindD3D()
{
	m_pD3DObj = NULL;
	m_pD3DDev = NULL;
}

/*------------------------------------------------------------------------------*/
/* destructor																	*/
/*------------------------------------------------------------------------------*/
AkindD3D::~AkindD3D()
{
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* Error																		*/
/*------------------------------------------------------------------------------*/
void Error( int no, HRESULT hRes )
{
	WCHAR Message[256];

	wsprintf( Message, L"Direct3D Error%d (0x%08X)", no, (int)hRes );
	MessageBox( NULL, Message, L"RemoteJoyLite", MB_OK );
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* Init																			*/
/*------------------------------------------------------------------------------*/
BOOL AkindD3D::Init( HWND hWnd )
{
	HRESULT hRes;
	D3DDISPLAYMODE d3ddp;
	D3DPRESENT_PARAMETERS d3dpp;

	m_pD3DObj = Direct3DCreate9( D3D_SDK_VERSION );
	if ( m_pD3DObj == NULL ){ Error( 0, 0 ); return( FALSE ); }
	hRes = m_pD3DObj->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddp );
	if ( FAILED( hRes ) ){ Error( 1, hRes ); return( FALSE ); }

	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed             = TRUE;
	d3dpp.SwapEffect           = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferCount      = 1;
	d3dpp.BackBufferFormat     = d3ddp.Format;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
//	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

	hRes = m_pD3DObj->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
									D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDev );
	if ( FAILED( hRes ) ){
		hRes = m_pD3DObj->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
										D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDev );
		if ( FAILED( hRes ) ){
			hRes = m_pD3DObj->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
											D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDev );
			if ( FAILED( hRes ) ){ Error( 2, hRes ); return( FALSE ); }
		}
	}
	return( TRUE );
}

/*------------------------------------------------------------------------------*/
/* Exit																			*/
/*------------------------------------------------------------------------------*/
void AkindD3D::Exit( void )
{
	if ( m_pD3DObj != NULL ){ m_pD3DObj->Release(); m_pD3DObj = NULL; }
	if ( m_pD3DDev != NULL ){ m_pD3DDev->Release(); m_pD3DDev = NULL; }
}

/*------------------------------------------------------------------------------*/
/* getDevice																	*/
/*------------------------------------------------------------------------------*/
IDirect3DDevice9 *AkindD3D::getDevice( void )
{
	return( m_pD3DDev );
}
