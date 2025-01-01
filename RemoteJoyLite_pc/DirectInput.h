#ifndef _DIRECTINPUT_H_
#define _DIRECTINPUT_H_
/*------------------------------------------------------------------------------*/
/* DirectInput																	*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define JOY_MAX			16

/*------------------------------------------------------------------------------*/
/* class																		*/
/*------------------------------------------------------------------------------*/
class AkindDI {
  private:
	struct JOYDATA {
		long long 		Button;
		long long 		Trigger;
		int				Axis[3];
		int				Rot[3];
	};
  private:
	IDirectInput		*m_pDInput;
	IDirectInputDevice	*m_pDInputKey;
	IDirectInputDevice2	*m_pDInputJoy[JOY_MAX];
	HWND				m_hWnd;
	int					m_JoyNum;
	BOOL				m_BackGround;
	char				m_KeyBuff[256];
	GUID				m_JoyGUID[JOY_MAX];
	JOYDATA				m_JoyData[JOY_MAX];
	DIJOYSTATE			m_JoyState[JOY_MAX];
	BOOL				m_JoyReady[JOY_MAX];
  public:
	AkindDI();
	~AkindDI();
  private:
	AkindDI( const AkindDI& ){}
	AkindDI &operator=( const AkindDI& ){ return *this; }
  private:
	static BOOL WINAPI JoyCallback( LPCDIDEVICEINSTANCE lpddi, LPVOID arg );
	void Error( int no, HRESULT hRes );
	void Acquire( void );
	void Unacquire( void );
	void JoyAdd( void );
	void JoyDel( void );
	void JoyDataCalc( JOYDATA *JoyData, DIJOYSTATE *JoyBuff );
	int JoyPovDir( DIJOYSTATE *JoyBuff, int idx );
	int JoyAxisDir( DIJOYSTATE *JoyBuff, int cut );
	int JoyRotDir( DIJOYSTATE *JoyBuff, int cut );
	int JoyAzRzDir( DIJOYSTATE *JoyBuff, int cut );
  public:
	BOOL Init( HWND hWnd, BOOL BackGround );
	void Exit( void );
	void Sync( void );
	void Message( UINT msg, WPARAM wParam, LPARAM lParam );
  public:
	BOOL CheckKeyData( int code );
	int GetJoyNum( void );
	long long GetJoyButton( int no );
	long long GetJoyTrigger( int no );
	int GetJoyAxis( int no, int xyz );
	int GetJoyRot( int no, int xyz );
};

#endif	// _DIRECTINPUT_H_
