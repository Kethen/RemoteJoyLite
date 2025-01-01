#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* DirectInput																	*/
/*------------------------------------------------------------------------------*/
#include "DirectInput.h"
#include <dbt.h>
#include <hidclass.h>
#include <stdio.h>

/*------------------------------------------------------------------------------*/
/* GUID_DEVINTERFACE_HID														*/
/*------------------------------------------------------------------------------*/
const GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2, 0xF16F, 0x11CF,
									 { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

/*------------------------------------------------------------------------------*/
/* constructor																	*/
/*------------------------------------------------------------------------------*/
AkindDI::AkindDI()
{
	m_pDInput    = NULL;
	m_pDInputKey = NULL;
	for ( int i=0; i<JOY_MAX; i++ ){
		m_pDInputJoy[i] = NULL;
	}
}

/*------------------------------------------------------------------------------*/
/* destructor																	*/
/*------------------------------------------------------------------------------*/
AkindDI::~AkindDI()
{
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* JoyCallback																	*/
/*------------------------------------------------------------------------------*/
BOOL WINAPI AkindDI::JoyCallback( LPCDIDEVICEINSTANCE lpddi, LPVOID arg )
{
	HRESULT				hRes;
	DIPROPRANGE			diprg;
	IDirectInputDevice	*pDITmp;
	AkindDI				*pObj     = (AkindDI *)arg;
	HWND				hWnd      = pObj->m_hWnd;
	int					JoyNum    = pObj->m_JoyNum;
	IDirectInput		*pDInput  = pObj->m_pDInput;
	IDirectInputDevice2	**ppDIJoy = pObj->m_pDInputJoy;

	if ( JoyNum >= JOY_MAX ){ return( DIENUM_STOP ); }
	for ( int i=0; i<JoyNum; i++ ){
		if ( pObj->m_JoyGUID[i] == lpddi->guidInstance ){
			return( DIENUM_CONTINUE );
		}
	}

	hRes = pDInput->CreateDevice( lpddi->guidInstance, &pDITmp, NULL );
	if ( FAILED( hRes ) ){ return( DIENUM_CONTINUE ); }
	pDITmp->QueryInterface( IID_IDirectInputDevice2, (LPVOID *)&ppDIJoy[JoyNum] );

	hRes = ppDIJoy[JoyNum]->SetDataFormat( &c_dfDIJoystick );
	if ( FAILED( hRes ) ){ pObj->Error( 4, hRes ); return( DIENUM_CONTINUE ); }
	if ( pObj->m_BackGround ){
		hRes = ppDIJoy[JoyNum]->SetCooperativeLevel( hWnd, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
	} else {
		hRes = ppDIJoy[JoyNum]->SetCooperativeLevel( hWnd, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
	}
	if ( FAILED( hRes ) ){ pObj->Error( 5, hRes ); return( DIENUM_CONTINUE ); }

	diprg.diph.dwSize       = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -32768;
	diprg.lMax              = +32767;
	diprg.diph.dwObj        = DIJOFS_X;
	ppDIJoy[JoyNum]->SetProperty( DIPROP_RANGE, &diprg.diph );
	diprg.diph.dwObj        = DIJOFS_Y;
	ppDIJoy[JoyNum]->SetProperty( DIPROP_RANGE, &diprg.diph );
	diprg.diph.dwObj        = DIJOFS_Z;
	ppDIJoy[JoyNum]->SetProperty( DIPROP_RANGE, &diprg.diph );
	diprg.diph.dwObj        = DIJOFS_RX;
	ppDIJoy[JoyNum]->SetProperty( DIPROP_RANGE, &diprg.diph );
	diprg.diph.dwObj        = DIJOFS_RY;
	ppDIJoy[JoyNum]->SetProperty( DIPROP_RANGE, &diprg.diph );
	diprg.diph.dwObj        = DIJOFS_RZ;
	ppDIJoy[JoyNum]->SetProperty( DIPROP_RANGE, &diprg.diph );

	pObj->m_JoyGUID[JoyNum] = lpddi->guidInstance;
	pObj->m_JoyNum += 1;
	return( DIENUM_CONTINUE );
}

/*------------------------------------------------------------------------------*/
/* Error																		*/
/*------------------------------------------------------------------------------*/
void AkindDI::Error( int no, HRESULT hRes )
{
	WCHAR Message[256];

	wsprintf( Message, L"DirectInput Error%d (0x%08X)", no, (int)hRes );
	MessageBox( NULL, Message, L"RemoteJoyLite", MB_OK );
}

/*------------------------------------------------------------------------------*/
/* Acquire																		*/
/*------------------------------------------------------------------------------*/
void AkindDI::Acquire( void )
{
	if ( m_pDInputKey != NULL ){
		m_pDInputKey->Acquire();
	}
	for ( int i=0; i<m_JoyNum; i++ ){
		if ( m_pDInputJoy[i] != NULL ){
			m_pDInputJoy[i]->Acquire();
			m_pDInputJoy[i]->GetDeviceState( sizeof(DIJOYSTATE), &m_JoyState[i] );
			m_JoyReady[i] = FALSE;
		}
	}
}

/*------------------------------------------------------------------------------*/
/* Unacquire																	*/
/*------------------------------------------------------------------------------*/
void AkindDI::Unacquire( void )
{
	if ( m_pDInputKey != NULL ){
		m_pDInputKey->Unacquire();
	}
	for ( int i=0; i<m_JoyNum; i++ ){
		if ( m_pDInputJoy[i] != NULL ){
			m_pDInputJoy[i]->Unacquire();
			ZeroMemory( &m_JoyData[i], sizeof(JOYDATA) );
		}
	}
}

/*------------------------------------------------------------------------------*/
/* JoyAdd																		*/
/*------------------------------------------------------------------------------*/
void AkindDI::JoyAdd( void )
{
	m_pDInput->EnumDevices( DIDEVTYPE_JOYSTICK, JoyCallback, this, DIEDFL_ATTACHEDONLY );
	Acquire();
}

/*------------------------------------------------------------------------------*/
/* JoyDel																		*/
/*------------------------------------------------------------------------------*/
void AkindDI::JoyDel( void )
{
}

/*------------------------------------------------------------------------------*/
/* JoyDataCalc																	*/
/*------------------------------------------------------------------------------*/
void AkindDI::JoyDataCalc( JOYDATA *JoyData, DIJOYSTATE *JoyBuff )
{
	int button1 = 0;
	int button2 = 0;

	for ( int i=0; i<32; i++ ){
		if ( JoyBuff->rgbButtons[i] & 0x80 ){ button1 |= (1<<i); }
	}
	button2 |= (JoyPovDir( JoyBuff, 0 ) << 0);
	button2 |= (JoyPovDir( JoyBuff, 1 ) << 1);
	button2 |= (JoyPovDir( JoyBuff, 2 ) << 2);
	button2 |= (JoyPovDir( JoyBuff, 3 ) << 3);
	button2 |= (JoyAxisDir( JoyBuff, 16384 ) << 16);
	button2 |= (JoyRotDir(  JoyBuff, 16384 ) << 17);
	button2 |= (JoyAzRzDir( JoyBuff, 16384 ) << 18);

	long long button = ((long long)button1 << 32) | button2;

	JoyData->Trigger = ~JoyData->Button & button;
	JoyData->Button  = button;
	JoyData->Axis[0] = JoyBuff->lX;
	JoyData->Axis[1] = JoyBuff->lY;
	JoyData->Axis[2] = JoyBuff->lZ;
	JoyData->Rot[0]  = JoyBuff->lRx;
	JoyData->Rot[1]  = JoyBuff->lRy;
	JoyData->Rot[2]  = JoyBuff->lRz;
}

/*------------------------------------------------------------------------------*/
/* JoyPovDir																	*/
/*------------------------------------------------------------------------------*/
int AkindDI::JoyPovDir( DIJOYSTATE *JoyBuff, int idx )
{
	switch ( JoyBuff->rgdwPOV[idx] ){
	case     0 : return( 0x0001 );
	case  4500 : return( 0x0011 );
	case  9000 : return( 0x0010 );
	case 13500 : return( 0x0110 );
	case 18000 : return( 0x0100 );
	case 22500 : return( 0x1100 );
	case 27000 : return( 0x1000 );
	case 31500 : return( 0x1001 );
	}
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* JoyAxisDir																	*/
/*------------------------------------------------------------------------------*/
int AkindDI::JoyAxisDir( DIJOYSTATE *JoyBuff, int cut )
{
	int ret = 0;
	int x = JoyBuff->lX;
	int y = JoyBuff->lY;

	if ( x >  cut ){ ret |= 0x0010; }
	if ( x < -cut ){ ret |= 0x1000; }
	if ( y >  cut ){ ret |= 0x0100; }
	if ( y < -cut ){ ret |= 0x0001; }
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* JoyRotDir																	*/
/*------------------------------------------------------------------------------*/
int AkindDI::JoyRotDir( DIJOYSTATE *JoyBuff, int cut )
{
	int ret = 0;
	int x = JoyBuff->lRx;
	int y = JoyBuff->lRy;

	if ( x >  cut ){ ret |= 0x0010; }
	if ( x < -cut ){ ret |= 0x1000; }
	if ( y >  cut ){ ret |= 0x0100; }
	if ( y < -cut ){ ret |= 0x0001; }
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* JoyAzRzDir																	*/
/*------------------------------------------------------------------------------*/
int AkindDI::JoyAzRzDir( DIJOYSTATE *JoyBuff, int cut )
{
	int ret = 0;
	int x = JoyBuff->lZ;
	int y = JoyBuff->lRz;

	if ( x >  cut ){ ret |= 0x0010; }
	if ( x < -cut ){ ret |= 0x1000; }
	if ( y >  cut ){ ret |= 0x0100; }
	if ( y < -cut ){ ret |= 0x0001; }
	return( ret );
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* Init																			*/
/*------------------------------------------------------------------------------*/
BOOL AkindDI::Init( HWND hWnd, BOOL BackGround )
{
	HRESULT hRes;
	HINSTANCE hInstance = GetModuleHandle( NULL );

	m_hWnd = hWnd;
	m_JoyNum = 0;
	m_BackGround = BackGround;

	ZeroMemory( m_KeyBuff, sizeof(m_KeyBuff) );
	ZeroMemory( m_JoyGUID, sizeof(m_JoyGUID) );
	ZeroMemory( m_JoyData, sizeof(m_JoyData) );
	for ( int i=0; i<JOY_MAX; i++ ){ m_JoyReady[i] = FALSE; }

	hRes = DirectInputCreate( hInstance, DIRECTINPUT_VERSION, &m_pDInput, NULL );
	if ( FAILED( hRes ) ){ Error( 0, hRes ); return( FALSE ); }
	hRes = m_pDInput->CreateDevice( GUID_SysKeyboard, &m_pDInputKey, NULL );
	if ( FAILED( hRes ) ){ Error( 1, hRes ); return( FALSE ); }
	hRes = m_pDInputKey->SetDataFormat( &c_dfDIKeyboard );
	if ( FAILED( hRes ) ){ Error( 2, hRes ); return( FALSE ); }
	if ( m_BackGround ){
		hRes = m_pDInputKey->SetCooperativeLevel( hWnd, DISCL_BACKGROUND|DISCL_NONEXCLUSIVE );
	} else {
		hRes = m_pDInputKey->SetCooperativeLevel( hWnd, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE );
	}
	if ( FAILED( hRes ) ){ Error( 3, hRes ); return( FALSE ); }

	JoyAdd();

	DEV_BROADCAST_DEVICEINTERFACE filter;
	filter.dbcc_size       = sizeof(filter);
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	filter.dbcc_classguid  = GUID_CLASS_INPUT;
	RegisterDeviceNotification( hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE );
	return( TRUE );
}

/*------------------------------------------------------------------------------*/
/* Exit																			*/
/*------------------------------------------------------------------------------*/
void AkindDI::Exit( void )
{
	if ( m_pDInputKey != NULL ){
		m_pDInputKey->Unacquire();
		m_pDInputKey->Release();
		m_pDInputKey = NULL;
	}
	for ( int i=0; i<m_JoyNum; i++ ){
		if ( m_pDInputJoy[i] != NULL ){
			m_pDInputJoy[i]->Unacquire();
			m_pDInputJoy[i]->Release();
			m_pDInputJoy[i] = NULL;
		}
	}
	if ( m_pDInput != NULL ){
		m_pDInput->Release();
		m_pDInput = NULL;
	}
}

/*------------------------------------------------------------------------------*/
/* Sync																			*/
/*------------------------------------------------------------------------------*/
void AkindDI::Sync( void )
{
	HRESULT hRes;

	if ( m_pDInputKey != NULL ){
		ZeroMemory( m_KeyBuff, sizeof(m_KeyBuff) );
		hRes = m_pDInputKey->GetDeviceState( 256, (void *)m_KeyBuff );
	}
	for ( int i=0; i<m_JoyNum; i++ ){
		if ( m_pDInputJoy[i] != NULL ){
			long long button = m_JoyData[i].Button;
			DIJOYSTATE JoyBuff;

			ZeroMemory( &m_JoyData[i], sizeof(JOYDATA) );
			hRes = m_pDInputJoy[i]->Poll();
			if ( FAILED( hRes ) ){ continue; }
			hRes = m_pDInputJoy[i]->GetDeviceState( sizeof(DIJOYSTATE), &JoyBuff );
			if ( FAILED( hRes ) ){ continue; }
			if ( m_JoyReady[i] == FALSE ){
				if ( memcmp( &m_JoyState[i], &JoyBuff, sizeof(DIJOYSTATE) ) == 0 ){ continue; }
				m_JoyReady[i] = TRUE;
			}
			m_JoyData[i].Button = button;
			JoyDataCalc( &m_JoyData[i], &JoyBuff );
		}
	}
}

/*------------------------------------------------------------------------------*/
/* Message																		*/
/*------------------------------------------------------------------------------*/
void AkindDI::Message( UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ){
	case WM_ACTIVATE:
		if ( m_BackGround == FALSE ){
			if ( WA_INACTIVE == wParam ){ Unacquire(); }
			else						{   Acquire(); }
		}
		break;
	case WM_DEVICECHANGE:
		switch ( wParam ){
		case DBT_DEVICEARRIVAL        : JoyAdd();	break;
		case DBT_DEVICEREMOVECOMPLETE : JoyDel();	break;
		}
		break;
	}
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* CheckKeyData																	*/
/*------------------------------------------------------------------------------*/
BOOL AkindDI::CheckKeyData( int code )
{
	if ( m_KeyBuff[code] & 0x80 ){ return( TRUE ); }
	return( FALSE );
}

/*------------------------------------------------------------------------------*/
/* GetJoyNum																	*/
/*------------------------------------------------------------------------------*/
int AkindDI::GetJoyNum( void )
{
	return( m_JoyNum );
}

/*------------------------------------------------------------------------------*/
/* GetJoyButton																	*/
/*------------------------------------------------------------------------------*/
long long AkindDI::GetJoyButton( int no )
{
	return( m_JoyData[no].Button );
}

/*------------------------------------------------------------------------------*/
/* GetJoyTrigger																*/
/*------------------------------------------------------------------------------*/
long long AkindDI::GetJoyTrigger( int no )
{
	return( m_JoyData[no].Trigger );
}

/*------------------------------------------------------------------------------*/
/* GetJoyAxis																	*/
/*------------------------------------------------------------------------------*/
int AkindDI::GetJoyAxis( int no, int xyz )
{
	return( m_JoyData[no].Axis[xyz] );
}

/*------------------------------------------------------------------------------*/
/* GetJoyRot																	*/
/*------------------------------------------------------------------------------*/
int AkindDI::GetJoyRot( int no, int xyz )
{
	return( m_JoyData[no].Rot[xyz] );
}
