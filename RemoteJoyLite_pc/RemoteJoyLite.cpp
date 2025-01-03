#include <stdint.h>
#include <windows.h>
/*------------------------------------------------------------------------------*/
/* RemoteJoyLite																*/
/*------------------------------------------------------------------------------*/
#include <windows.h>
#include <d3d9.h>
#include <dinput.h>
#include <stdio.h>
#include <usb.h>
#include <errno.h>
#include <time.h>
#include <vfw.h>
#include <math.h>
#include "Direct3D.h"
#include "DirectInput.h"
#include "DebugFont.h"
#include "RemoteJoyLite.h"
#include "Bitmap.h"
#include "Movie.h"
#include "Wave.h"
#include "Setting.h"
#include "../remotejoy.h"
#include "lusb0_usb.h"

#include <time.h>

/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define PSP_SCREEN_W		480
#define PSP_SCREEN_H		272
#define LEFT_POS(x)			(work.disp_rot&1)?(PSP_SCREEN_H-6*(x)-1):(PSP_SCREEN_W-6*(x)-1)
#define FRAME_COUNTER_SIZE	32

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static struct {
	int			rect_x;
	int			rect_y;
	int			rect_w;
	int			rect_h;
	char		buff[PSP_SCREEN_W*PSP_SCREEN_H*4];
	HANDLE		buff_sema;
	int			buff_mode;
	int			gamma_flag;
	int			buff_vcount;
	int			prev_vcount;
	int			disp_flag;
	int			disp_rot;
	int			disp_debug;
	int			fps_idx;
	LONGLONG	fps_cnt[FRAME_COUNTER_SIZE];
	int			button;
	int			axis_x;
	int			axis_y;
	int         axis_rx;
	int         axis_ry;
	int			save_flag;
	int			save_bmp;
	int			save_avi;
	int			save_wav;
	PSP_BITMAP	psp_bmp;
} work;

/*------------------------------------------------------------------------------*/
/* UpdateFps																	*/
/*------------------------------------------------------------------------------*/
static void UpdateFps( int diff_vcount )
{
	int now_index = work.fps_idx;
	int lst_index = (now_index + 1) % FRAME_COUNTER_SIZE;

	if ( diff_vcount != 0 ){
		now_index = lst_index;
		lst_index = (now_index + 1) % FRAME_COUNTER_SIZE;

		QueryPerformanceCounter( (PLARGE_INTEGER)&work.fps_cnt[now_index] );
		work.fps_idx = now_index;
	}

	double fps = 0.0f;
	if ( work.fps_cnt[lst_index] != 0 ){
		LONGLONG frequency;
		LONGLONG diff_count = work.fps_cnt[now_index] - work.fps_cnt[lst_index];

		QueryPerformanceFrequency( (PLARGE_INTEGER)&frequency );
		fps = ((double)(FRAME_COUNTER_SIZE-1))/((double)diff_count/(double)frequency);
	}
	if ( work.disp_flag == 0 ){
		dprintf( LEFT_POS(8), 0, "DISP OFF" );
	} else {
		if ( work.disp_debug != 0 ){
			dprintf( LEFT_POS(8), 0, "FPS %4.1f", fps );
		}
	}
}

/********************************************************************************/
/* Usb																			*/
/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* usb device																	*/
/*------------------------------------------------------------------------------*/
static usb_dev_handle *UsbDev = NULL;
static int UsbhostfsReady = 0;
static int UsbhostfsExit  = -1;
static int UsbhostError = 0;
static int PSPReady = 0;

/*------------------------------------------------------------------------------*/
/* async_remotejoy																*/
/*------------------------------------------------------------------------------*/
static void async_remotejoy( void *read, int read_len )
{
	if ( read_len < (int)sizeof(JoyScrHeader) ){ return; }

	JoyScrHeader *cmd = (JoyScrHeader *)read;

	if ( cmd->mode == ASYNC_CMD_DEBUG ){
		dprintf( 0, 0, "%s", (void *)(cmd+1) );
		printf("psp debug message begin:\n");
		printf("%s", (void *)(cmd + 1));
		printf("psp debug message end:\n");
	}
}

/*------------------------------------------------------------------------------*/
/* bulk_remotejoy																*/
/*------------------------------------------------------------------------------*/
static void bulk_remotejoy( void *read, int read_len )
{
	JoyScrHeader *cmd = (JoyScrHeader *)read;

	work.buff_mode   = cmd->mode;
	work.buff_vcount = cmd->ref;
	WaitForSingleObject( work.buff_sema, INFINITE );
	memcpy( work.buff, (void *)(cmd+1), cmd->size );
	ReleaseSemaphore( work.buff_sema, 1, NULL );
}

/*------------------------------------------------------------------------------*/
/* send_event																	*/
/*------------------------------------------------------------------------------*/
static void send_event( int type, int value1, int value2 )
{
	int ret;
	struct {
		AsyncCommand	async;
		JoyEvent		event;
	} data;

	if ( PSPReady == 0 ){ return; }
	data.async.magic   = ASYNC_MAGIC;
	data.async.channel = ASYNC_USER;
	data.event.magic   = JOY_MAGIC;
	data.event.type    = type;
	data.event.value1  = value1;
	data.event.value2  = value2;
	ret = usb_bulk_write( UsbDev, 3, (char *)&data, sizeof(data), 10000 );
	if ( ret < 0 ){ return; }
}

/*------------------------------------------------------------------------------*/
/* handle_hello																	*/
/*------------------------------------------------------------------------------*/
static void handle_hello( void )
{
	int ret;
	HostFsHelloResp Resp;

	memset( &Resp, 0, sizeof(Resp) );
	Resp.cmd.magic   = HOSTFS_MAGIC;
	Resp.cmd.command = HOSTFS_CMD_HELLO(RJL_VERSION);
	ret = usb_bulk_write( UsbDev, 2, (char *)&Resp, sizeof(Resp), 10000 );
	if ( ret < 0 ){ return; }
	if ( SettingData.DispOff == 0 ){ work.disp_flag = 1; }
	PSPReady = 1;
	RemoteJoyLite_SendPSPCmd();
}

/*------------------------------------------------------------------------------*/
/* do_default																	*/
/*------------------------------------------------------------------------------*/
static void do_default( void *read, int read_len ){}

/*------------------------------------------------------------------------------*/
/* do_hostfs																	*/
/*------------------------------------------------------------------------------*/
static void do_hostfs( void *read, int read_len )
{
	HostFsCmd *cmd = (HostFsCmd *)read;

	if ( read_len < (int)sizeof(HostFsCmd) ){ return; }

	if ( (int)cmd->command == HOSTFS_CMD_HELLO(RJL_VERSION) ){
		UsbhostError = 0;
		handle_hello();
	} else {
		UsbhostError = 1;
	}
}

/*------------------------------------------------------------------------------*/
/* do_async																		*/
/*------------------------------------------------------------------------------*/
static void do_async( void *read, int read_len )
{
	AsyncCommand *cmd = (AsyncCommand *)read;

	if ( read_len < (int)sizeof(AsyncCommand) ){ return; }
	if ( read_len > (int)sizeof(AsyncCommand) ){
		char *data = (char *)cmd + sizeof(AsyncCommand);
		async_remotejoy( data, read_len - sizeof(AsyncCommand) );
	}
}

/*------------------------------------------------------------------------------*/
/* do_bulk																		*/
/*------------------------------------------------------------------------------*/
static char BulkBlock[HOSTFS_BULK_MAXWRITE];

static void do_bulk( void *read, int read_len )
{
	BulkCommand *cmd = (BulkCommand *)read;

	if ( read_len < (int)sizeof(BulkCommand) ){ return; }

	int read_size = 0;
	int data_size = cmd->size;

	while ( read_size < data_size ){
		int rest_size = data_size - read_size;
		if ( rest_size > HOSTFS_MAX_BLOCK ){ rest_size = HOSTFS_MAX_BLOCK; }
		int ret = usb_bulk_read( UsbDev, 0x81, &BulkBlock[read_size], rest_size, 3000 );
		if (ret == -110 || ret == -ETIMEDOUT) {continue;}
		if ( ret < 0 ){; break;}
		read_size += ret;
	}
	bulk_remotejoy( BulkBlock, data_size );
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* UsbOpenDevice																*/
/*------------------------------------------------------------------------------*/
static void UsbOpenDevice( void )
{
	struct usb_bus    *bus;
	struct usb_device *dev;
	while ( 1 ){
		usb_find_busses();
		usb_find_devices();
		for ( bus=usb_get_busses(); bus!=NULL; bus=bus->next ){
			for ( dev=bus->devices; dev!=NULL; dev=dev->next ){
				if ( dev->descriptor.idVendor != SONY_VID ){ continue; }
				if ( dev->descriptor.idProduct != HOSTFSDRIVER_PID ){ continue; }
				UsbDev = usb_open( dev );
				if ( UsbDev == NULL ){ continue; }
				if ( usb_set_configuration( UsbDev, 1 ) == 0 ){
					if ( usb_claim_interface( UsbDev, 0 ) == 0 ){ return; }else{ printf("%s: failed to claim interface\n", __func__); }
				}else{
					printf("%s: failed to set config\n", __func__);
				}
				usb_close( UsbDev );
				UsbDev = NULL;
			}
		}
		Sleep( 1000 );
		if ( UsbhostfsExit != 0 ){ return; }
	}
}

/*------------------------------------------------------------------------------*/
/* UsbCloseDevice																*/
/*------------------------------------------------------------------------------*/
static void UsbCloseDevice( void )
{
	if ( UsbDev != NULL ){
		usb_release_interface( UsbDev, 0 );
		usb_reset( UsbDev );
		usb_close( UsbDev );
	}
	UsbDev = NULL;
}

/*------------------------------------------------------------------------------*/
/* UsbCheckDevice																*/
/*------------------------------------------------------------------------------*/
static int UsbCheckDevice( void )
{
	if ( UsbDev == NULL ){ return( -1 ); }
	int mag = HOSTFS_MAGIC;
	int len = usb_bulk_write( UsbDev, 2, (char *)&mag, 4, 1000 );
	if ( len == 4 ){ return( 0 ); }
	return( -1 );
}

/*------------------------------------------------------------------------------*/
/* UsbDeviceMain																*/
/*------------------------------------------------------------------------------*/
static DWORD WINAPI UsbDeviceMain( LPVOID lpv )
{
	usb_init();
	while( 1 ){
		UsbOpenDevice();
		if ( UsbCheckDevice() == 0 ){
			UsbhostfsReady = 1;
			while ( UsbhostfsExit == 0 ){
				int data[512/sizeof(int)];
				int len = usb_bulk_read( UsbDev, 0x81, (char *)data, 512, 1000 );
				// XXX hoo boy, error codes don't match, just put the linux one here for now
				if ( len == -ETIMEDOUT || len == -110 ){ continue; }
				if ( len < 0 ){ printf("%s: usb read error %d\n", __func__, len); break; }
				if ( len < 4 ){ printf("%s: tiny read\n", __func__); continue; }
				switch ( data[0] ){
				default           : do_default( data, len );	break;
				case HOSTFS_MAGIC : do_hostfs ( data, len );	break;
				case ASYNC_MAGIC  : do_async  ( data, len );	break;
				case BULK_MAGIC   : do_bulk   ( data, len );	break;
				}
				Sleep( 0 );
			}
			PSPReady = 0;
			UsbhostfsReady = 0;
		}
		UsbCloseDevice();
		if ( UsbhostfsExit != 0 ){ break; }
	}
	UsbhostfsExit = 2;
	return( 0 );
}

/********************************************************************************/
/* RemoteJoy Main																*/
/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* define																		*/
/*------------------------------------------------------------------------------*/
#define PRIM_FVF	(D3DFVF_XYZRHW|D3DFVF_TEX1)

struct PRIM {
	float x, y, z;
	float rhw;
	float u, v;
};

/*------------------------------------------------------------------------------*/
/* work																			*/
/*------------------------------------------------------------------------------*/
static IDirect3DTexture9 *pD3DTex = NULL;

static WORD	DispIdx[6] = { 0, 1, 2, 3, 2, 1 };
static PRIM DispBuf[4][4] = {
	{ {   0.0f,   0.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  { 480.0f,   0.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  {   0.0f, 272.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f },
	  { 480.0f, 272.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f }},
	{ { 272.0f,   0.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  { 272.0f, 480.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  {   0.0f,   0.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f },
	  {   0.0f, 480.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f }},
	{ { 480.0f, 272.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  {   0.0f, 272.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  { 480.0f,   0.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f },
	  {   0.0f,   0.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f }},
	{ {   0.0f, 480.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  {   0.0f,   0.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f,          0.0f + 0.5f/512.0f },
	  { 272.0f, 480.0f, 0.0f, 1.0f,          0.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f },
	  { 272.0f,   0.0f, 0.0f, 1.0f, 480.0f/512.0f + 0.5f/512.0f, 272.0f/512.0f + 0.5f/512.0f }},
};

/*------------------------------------------------------------------------------*/
/* Gamma																		*/
/*------------------------------------------------------------------------------*/
static unsigned char GammaTable[3][256];

void RemoteJoyLite_CalcGammaTable( void )
{
	float gamma[3];

	for ( int rgb=0; rgb<3; rgb++ ){
		if ( SettingData.GammaRGB == 0 ){
			gamma[rgb] = 100.0f/(float)SettingData.Gamma[3];
		} else {
			gamma[rgb] = 100.0f/(float)SettingData.Gamma[rgb];
		}
	}

	for ( int rgb=0; rgb<3; rgb++ ){
		unsigned char *tbl = GammaTable[rgb];
		for ( int i=0; i<256; i++ ){
			float raw = (float)i/255.0f;
			float cor = powf( raw, gamma[rgb] );
			tbl[i] = (unsigned char)(0.5f + cor*255.0f);
		}
	}
	work.gamma_flag = 0;
	if ( SettingData.GammaRGB == 0 ){
		if ( SettingData.Gamma[3] != 100 ){ work.gamma_flag = 1; }
	} else {
		if ( SettingData.Gamma[0] != 100 ){ work.gamma_flag = 1; }
		if ( SettingData.Gamma[1] != 100 ){ work.gamma_flag = 1; }
		if ( SettingData.Gamma[2] != 100 ){ work.gamma_flag = 1; }
	}
}

/*------------------------------------------------------------------------------*/
/* TranceGamma																	*/
/*------------------------------------------------------------------------------*/
static DWORD TranceGamma( DWORD argb )
{
	DWORD r = (argb >> 16) & 0xFF;
	DWORD g = (argb >>  8) & 0xFF;
	DWORD b = (argb >>  0) & 0xFF;
	r = GammaTable[0][r];
	g = GammaTable[1][g];
	b = GammaTable[2][b];
	return( (r<<16)|(g<<8)|b|0xFF000000 );
}

/*------------------------------------------------------------------------------*/
/* Trancetexture_ARGB0565														*/
/*------------------------------------------------------------------------------*/
static void Trancetexture_ARGB0565( D3DLOCKED_RECT *plockRect )
{
	WORD  *src = (WORD *)work.buff;
	DWORD *dst = (DWORD *)plockRect->pBits;
	DWORD texw = plockRect->Pitch/sizeof(DWORD);
	DWORD addp = texw - work.rect_w;
	dst += texw*work.rect_y + work.rect_x;

	if ( work.buff_mode & 0x100 ){ dst += texw; }
	for ( int y=0; y<work.rect_h; y++ ){
		for ( int i=0; i<work.rect_w; i++ ){
			DWORD r = ((DWORD)*src & 0x001F) << 19;
			DWORD g = ((DWORD)*src & 0x07E0) <<  5;
			DWORD b = ((DWORD)*src & 0xF800) >>  8;
			DWORD c = r|g|b|0xFF000000;
			DWORD m = (c & 0x00E000E0) >> 5;
			DWORD n = (c & 0x0000C000) >> 6;
			DWORD o = c|m|n;
			if ( work.gamma_flag != 0 ){ o = TranceGamma( o ); }
			*dst++ = o;
			src++;
		}
		dst += addp;
		if ( work.buff_mode & 0x400 ){ if ( !(y&1) ){ src -= work.rect_w; } }
		if ( work.buff_mode & 0x200 ){ dst += texw; y++; }
	}
}

/*------------------------------------------------------------------------------*/
/* Trancetexture_ARGB1555														*/
/*------------------------------------------------------------------------------*/
static void Trancetexture_ARGB1555( D3DLOCKED_RECT *plockRect )
{
	WORD  *src = (WORD *)work.buff;
	DWORD *dst = (DWORD *)plockRect->pBits;
	DWORD texw = plockRect->Pitch/sizeof(DWORD);
	DWORD addp = texw - work.rect_w;
	dst += texw*work.rect_y + work.rect_x;

	if ( work.buff_mode & 0x100 ){ dst += texw; }
	for ( int y=0; y<work.rect_h; y++ ){
		for ( int i=0; i<work.rect_w; i++ ){
			DWORD r = ((DWORD)*src & 0x001F) << 19;
			DWORD g = ((DWORD)*src & 0x03E0) <<  6;
			DWORD b = ((DWORD)*src & 0x7C00) >>  7;
			DWORD c = r|g|b|0xFF000000;
			DWORD m = (c & 0x00E0E0E0) >> 5;
			DWORD o = c|m;
			if ( work.gamma_flag != 0 ){ o = TranceGamma( o ); }
			*dst++ = o;
			src++;
		}
		dst += addp;
		if ( work.buff_mode & 0x400 ){ if ( !(y&1) ){ src -= work.rect_w; } }
		if ( work.buff_mode & 0x200 ){ dst += texw; y++; }
	}
}

/*------------------------------------------------------------------------------*/
/* Trancetexture_ARGB4444														*/
/*------------------------------------------------------------------------------*/
static void Trancetexture_ARGB4444( D3DLOCKED_RECT *plockRect )
{
	WORD  *src = (WORD *)work.buff;
	DWORD *dst = (DWORD *)plockRect->pBits;
	DWORD texw = plockRect->Pitch/sizeof(DWORD);
	DWORD addp = texw - work.rect_w;
	dst += texw*work.rect_y + work.rect_x;

	if ( work.buff_mode & 0x100 ){ dst += texw; }
	for ( int y=0; y<work.rect_h; y++ ){
		for ( int i=0; i<work.rect_w; i++ ){
			DWORD r = ((DWORD)*src & 0x000F) << 20;
			DWORD g = ((DWORD)*src & 0x00F0) <<  8;
			DWORD b = ((DWORD)*src & 0x0F00) >>  4;
			DWORD c = r|g|b|0xFF000000;
			DWORD m = (c & 0x00F0F0F0) >> 4;
			DWORD o = c|m;
			if ( work.gamma_flag != 0 ){ o = TranceGamma( o ); }
			*dst++ = o;
			src++;
		}
		dst += addp;
		if ( work.buff_mode & 0x400 ){ if ( !(y&1) ){ src -= work.rect_w; } }
		if ( work.buff_mode & 0x200 ){ dst += texw; y++; }
	}
}

/*------------------------------------------------------------------------------*/
/* Trancetexture_ARGB8888														*/
/*------------------------------------------------------------------------------*/
static void Trancetexture_ARGB8888( D3DLOCKED_RECT *plockRect )
{
	DWORD *src = (DWORD *)work.buff;
	DWORD *dst = (DWORD *)plockRect->pBits;
	DWORD texw = plockRect->Pitch/sizeof(DWORD);
	DWORD addp = texw - work.rect_w;
	dst += texw*work.rect_y + work.rect_x;

	for ( int y=0; y<work.rect_h; y++ ){
		for ( int i=0; i<work.rect_w; i++ ){
			DWORD r = (*src & 0x0000FF) << 16;
			DWORD g = (*src & 0x00FF00) <<  0;
			DWORD b = (*src & 0xFF0000) >> 16;
			DWORD o = r|g|b|0xFF000000;
			if ( work.gamma_flag != 0 ){ o = TranceGamma( o ); }
			*dst++ = o;
			src++;
		}
		dst += addp;
	}
}

#define bzero(dst, size) memset(dst, 0, size)

/*------------------------------------------------------------------------------*/
/* Trancetexture_UNKNOWN														*/
/*------------------------------------------------------------------------------*/
static void Trancetexture_UNKNOWN( D3DLOCKED_RECT *plockRect )
{
	DWORD *dst = (DWORD *)plockRect->pBits;
	DWORD texw = plockRect->Pitch/sizeof(DWORD);

	for ( int y=0; y<272; y++ ){
		bzero( dst, sizeof(DWORD)*480 );
		dst += texw;
	}
}

/*------------------------------------------------------------------------------*/
/* TranceSaveBmpBuff															*/
/*------------------------------------------------------------------------------*/
static void TranceSaveBmpBuff( D3DLOCKED_RECT *plockRect )
{
	DWORD *src = (DWORD *)plockRect->pBits;
	DWORD texw = plockRect->Pitch/sizeof(DWORD);
	DWORD addp = texw - 480;

	for ( int y=0; y<272; y++ ){
		char *dst = &work.psp_bmp.Buff[PSP_SCREEN_W*3*(271-y)];
		for ( int i=0; i<480; i++ ){
			*dst++ = *src >>  0;
			*dst++ = *src >>  8;
			*dst++ = *src >> 16;
			src++;
		}
		src += addp;
	}
}

/*------------------------------------------------------------------------------*/
/* Trancetexture																*/
/*------------------------------------------------------------------------------*/
static void Trancetexture( void )
{
	D3DLOCKED_RECT lockRect;

	work.save_flag = 0;
	if ( work.save_bmp != 0 ){ work.save_flag = 1; }
	if ( work.save_avi != 0 ){ work.save_flag = 1; }

	WaitForSingleObject( work.buff_sema, INFINITE );
	pD3DTex->LockRect( 0, &lockRect, NULL, D3DLOCK_DISCARD );
	switch ( (work.buff_mode >> 4) & 0x0F ){
	case 0x00 : Trancetexture_ARGB0565( &lockRect );	break;
	case 0x01 : Trancetexture_ARGB1555( &lockRect );	break;
	case 0x02 : Trancetexture_ARGB4444( &lockRect );	break;
	case 0x03 : Trancetexture_ARGB8888( &lockRect );	break;
	default   : Trancetexture_UNKNOWN(  &lockRect );	break;
	}
	if ( work.save_flag != 0 ){ TranceSaveBmpBuff( &lockRect ); }

	pD3DTex->UnlockRect( 0 );
	ReleaseSemaphore( work.buff_sema, 1, NULL );
	if ( work.save_bmp != 0 ){
		Bitmap_Save( &work.psp_bmp );
		work.save_bmp = 0;
	}
	if ( work.save_avi != 0 ){
		u_int vcount = (u_int)work.buff_vcount;
		Movie_Save( &work.psp_bmp, vcount );
	}
}

/*------------------------------------------------------------------------------*/
/* Error																		*/
/*------------------------------------------------------------------------------*/
static void Error( int no, HRESULT hRes )
{
	WCHAR Message[256];

	wsprintf( Message, L"RemoteJoyLite Error%d (0x%08X)", no, (int)hRes );
	MessageBox( NULL, Message, L"RemoteJoyLite", MB_OK );
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLiteInit															*/
/*------------------------------------------------------------------------------*/
BOOL RemoteJoyLiteInit( AkindD3D *pAkindD3D )
{
	DWORD   thid;
	HRESULT hRes;

	ZeroMemory( &work, sizeof(work) );
	work.buff_sema = CreateSemaphore( NULL, 1, 1, NULL );
	work.buff_mode = -1;
	work.disp_rot = SettingData.DispRot;
	work.axis_x = 32768/256;
	work.axis_y = 32768/256;
	work.axis_rx = 32768/256;
	work.axis_ry = 32768/256;
	Bitmap_Init( &work.psp_bmp );
	RemoteJoyLite_CalcGammaTable();

	IDirect3DDevice9 *pD3DDev = pAkindD3D->getDevice();
	hRes = pD3DDev->CreateTexture( 512, 512, 1, 0, D3DFMT_A8R8G8B8,
								   D3DPOOL_MANAGED, &pD3DTex, NULL );

	if ( FAILED( hRes ) ){ Error( 0, hRes ); return( FALSE ); }
	UsbhostfsExit = 0;
	CreateThread( NULL, 0, UsbDeviceMain, 0, 0, &thid );
	return( TRUE );
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLiteExit															*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLiteExit( void )
{
	if ( work.save_avi == 1 ){ Movie_End(); }
	if ( UsbhostfsExit == -1 ){ return; }
	UsbhostfsExit = 1;
	printf( "Waiting for usbhostfs exit...\n" );
	while ( UsbhostfsExit != 2 ){ Sleep( 0 ); }
	if ( pD3DTex != NULL ){
		pD3DTex->Release();
	}
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLiteDraw															*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLiteDraw( AkindD3D *pAkindD3D )
{
	int diff_vcount;

	diff_vcount = work.buff_vcount - work.prev_vcount;
	if ( diff_vcount == 0 ){ Sleep( 1 ); }
	diff_vcount = work.buff_vcount - work.prev_vcount;
	work.prev_vcount = work.buff_vcount;

	if ( UsbhostfsReady != 0 ){
		UpdateFps( diff_vcount );
	} else {
		work.buff_mode = -1;
		dprintf( LEFT_POS(7), 0, "WAITING" );
	}
	if ( UsbhostError != 0 ){
		dprintf( 240-6*9, 136-4, "PRX Version Error!" );
	}

	if ( work.disp_flag != 0 ){ Trancetexture(); }
	IDirect3DDevice9 *pD3DDev = pAkindD3D->getDevice();
	pD3DDev->SetTexture( 0, pD3DTex );
	pD3DDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	pD3DDev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	pD3DDev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	pD3DDev->SetFVF( PRIM_FVF );
	pD3DDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, 4, 2, DispIdx,
									 D3DFMT_INDEX16, DispBuf[work.disp_rot], sizeof(PRIM) );
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLiteSync															*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLiteSync( void )
{
	static struct timespec last_input = {0};
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	uint64_t time_since_last_input = now.tv_nsec - last_input.tv_nsec + (now.tv_sec - last_input.tv_sec) * 1000000000;
	if(time_since_last_input > (1000000000/75)){
		int	AxisData = work.axis_x | (work.axis_y << 16) | (work.axis_rx << 8) | (work.axis_ry << 24);
		send_event( TYPE_JOY_DAT, work.button, AxisData );
		clock_gettime(CLOCK_MONOTONIC, &last_input);
	}

	if ( work.disp_debug != 0 ){
		switch ( work.buff_mode & 0x0F ){
		case 0x00 : dprintf( LEFT_POS(8), 7, "ARGB0565" );	break;
		case 0x01 : dprintf( LEFT_POS(8), 7, "ARGB1555" );	break;
		case 0x02 : dprintf( LEFT_POS(8), 7, "ARGB4444" );	break;
		case 0x03 : dprintf( LEFT_POS(8), 7, "ARGB8888" );	break;
		default   : dprintf( LEFT_POS(7), 7, "UNKNOWN"  );	break;
		}
	}
	if ( (work.save_avi != 0) || (work.save_wav != 0) ){
		dprintf( 0, 0, "REC" );
	}
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_ToggleDebug													*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLite_ToggleDebug( void )
{
	work.disp_debug ^= 1;
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_ToggleDisp														*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLite_ToggleDisp( void )
{
	work.disp_flag ^= 1;
	RemoteJoyLite_SendPSPCmd();
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_SendPSPCmd														*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLite_SendPSPCmd( void )
{
	int	arg1 = 0;
	if ( work.disp_flag       != 0 ){ arg1 |= SCREEN_CMD_ACTIVE; }
	if ( SettingData.PSPDisp  != 0 ){ arg1 |= SCREEN_CMD_SCROFF; }
	if ( SettingData.PSPDbg   != 0 ){ arg1 |= SCREEN_CMD_DEBUG;  }
	if ( SettingData.PSPAsync != 0 ){ arg1 |= SCREEN_CMD_ASYNC;  }
	arg1 |= SCREEN_CMD_SET_TRNSFPS(SettingData.PSPFPS);
	arg1 |= SCREEN_CMD_SET_TRNSMODE(SettingData.PSPMode);
	arg1 |= SCREEN_CMD_SET_PRIORITY(SettingData.PSPPri);
	arg1 |= SCREEN_CMD_SET_ADRESS1(SettingData.PSPAdr1);
	arg1 |= SCREEN_CMD_SET_ADRESS2(SettingData.PSPAdr2 - 1);

	work.rect_x = SettingData.PSPRectX & 0xFFF8;
	work.rect_y = SettingData.PSPRectY;
	work.rect_w = ((SettingData.PSPRectW+(SettingData.PSPRectX&7))+31) & 0xFFE0;
	work.rect_h = (SettingData.PSPRectH+1) & 0xFFFE;

	int	arg2 = 0;
	arg2 |= SCREEN_CMD_SET_TRNSX(work.rect_x/8);
	arg2 |= SCREEN_CMD_SET_TRNSY(work.rect_y);
	arg2 |= SCREEN_CMD_SET_TRNSW(work.rect_w/32);
	arg2 |= SCREEN_CMD_SET_TRNSH(work.rect_h/2);

	send_event( TYPE_JOY_CMD, arg1, arg2 );

	D3DLOCKED_RECT lockRect;
	pD3DTex->LockRect( 0, &lockRect, NULL, D3DLOCK_DISCARD );
	Trancetexture_UNKNOWN( &lockRect );
	pD3DTex->UnlockRect( 0 );
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_SetButton														*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLite_SetButton( int button )
{
	work.button = button;
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_SetAxis														*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLite_SetAxis( int x, int y )
{
	work.axis_x = x & 0xFF;
	work.axis_y = y & 0xFF;
}

void RemoteJoyLite_SetAxisR( int x, int y )
{
	work.axis_rx = x & 0XFF;
	work.axis_ry = y & 0xFF;
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_GetButton														*/
/*------------------------------------------------------------------------------*/
int RemoteJoyLite_GetButton( void )
{
	return( work.button );
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_GetAxisXY														*/
/*------------------------------------------------------------------------------*/
int RemoteJoyLite_GetAxisXY( void )
{
	return( work.axis_x | (work.axis_y << 16) );
}

/*------------------------------------------------------------------------------*/
/* SetSaveName																	*/
/*------------------------------------------------------------------------------*/
static void SetSaveName( char *file_name )
{
	time_t		now;
	struct tm	*date;

	time( &now );
	date = localtime( &now );
	int year   = date->tm_year + 1900;
	int month  = date->tm_mon + 1;
	int day    = date->tm_mday;
	int hour   = date->tm_hour;
	int minute = date->tm_min;
	int second = date->tm_sec;

	sprintf( file_name, ".\\Capture\\%04d%02d%02d%02d%02d%02d",
								 year, month, day, hour, minute, second );
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_SaveBitmap														*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLite_SaveBitmap( void )
{
	char	file_name[256];

	if ( work.save_bmp == 0 ){
		SetSaveName( file_name );
		Bitmap_Name( file_name );
		work.save_bmp = 1;
	}
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_SaveMovie														*/
/*------------------------------------------------------------------------------*/
void RemoteJoyLite_SaveMovie( void )
{
	char	file_name[256];

	SetSaveName( file_name );

	if ( SettingData.AVIOut != 0 ){
		work.save_avi ^= 1;

		if ( work.save_avi == 1 ){
			u_int vcount = (u_int)work.buff_vcount;
			if ( Movie_Init( file_name, &work.psp_bmp, vcount ) != 0 ){ work.save_avi = 0; }
		} else {
			Movie_End();
		}
	}
	if ( SettingData.WAVOut != 0 ){
		work.save_wav ^= 1;

		if ( work.save_wav == 1 ){
			WaveOpen( file_name );
		} else {
			WaveClose();
		}
	}
}

/*------------------------------------------------------------------------------*/
/* RemoteJoyLite_CheckMovie														*/
/*------------------------------------------------------------------------------*/
BOOL RemoteJoyLite_CheckMovie( void )
{
	if ( work.save_avi == 1 ){ return( TRUE ); }
	if ( work.save_wav == 1 ){ return( TRUE ); }
	return( FALSE );
}
