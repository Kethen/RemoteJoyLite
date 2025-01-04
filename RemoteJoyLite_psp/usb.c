/*------------------------------------------------------------------------------*/
/* usb																			*/
/*------------------------------------------------------------------------------*/
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspusb.h>
#include <pspusbbus.h>
#include <string.h>
#include "../remotejoy.h"
#include "Descriptor.h"
#include "usb.h"
#include "kmode.h"

/*------------------------------------------------------------------------------*/
/* prototype																	*/
/*------------------------------------------------------------------------------*/
static int UsbMainThread( SceSize size, void *argp );
extern void DisplayEnable( void );
extern void hookCtrlSetData( u32 PreData, u32 NowData, u32 Analog );
extern u32 PreButton;
extern int DebugMode;

/*------------------------------------------------------------------------------*/
/* USB driver define															*/
/*------------------------------------------------------------------------------*/
enum UsbEvents {
	USB_EVENT_ATTACH  = 1,
	USB_EVENT_DETACH  = 2,
	USB_EVENT_ASYNC   = 4,
	USB_EVENT_CONNECT = 8,
	USB_EVENT_ALL     = 0xFFFFFFFF
};

enum UsbTransEvents {
	USB_TRANSEVENT_BULKOUT_DONE = 1,
	USB_TRANSEVENT_BULKIN_DONE  = 2,
};

#define RJLITE_DRIVERNAME "RJLiteDriver"
#define RJLITE_DRIVERPID  (0x1C9)

#define MAX_ASYNC_BUFFER   4096

struct AsyncEndpoint {
	char	buffer[MAX_ASYNC_BUFFER];
	int		read_pos;
	int		write_pos;
	int		size;
};

/*------------------------------------------------------------------------------*/
/* USB driver work																*/
/*------------------------------------------------------------------------------*/
static struct UsbEndpoint UsbEndpoint[4] = { {0,0,0}, {1,0,0}, {2,0,0}, {3,0,0} };
static struct UsbInterface UsbInterface = { 0xFFFFFFFF, 0, 1, };
static struct UsbData UsbData[2];
static struct StringDescriptor StringDescriptor = { 0x8, 0x3, {'<',0,'>',0,0,0} };
static SceUID UsbMainEventFlag = -1;
static SceUID UsbTransEventFlag = -1;
static SceUID UsbAsyncEventFlag = -1;
static SceUID UsbMainSemaphore = -1;
static SceUID UsbMainThreadID = -1;

/*------------------------------------------------------------------------------*/
/* UsbRequest																	*/
/*------------------------------------------------------------------------------*/
static int UsbRequest( int arg1, int arg2, struct DeviceRequest *req )
{
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbUnknown																	*/
/*------------------------------------------------------------------------------*/
static int UsbUnknown( int arg1, int arg2, int arg3 )
{
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbAttach																	*/
/*------------------------------------------------------------------------------*/
static int UsbAttach( int speed, void *arg2, void *arg3 )
{
	sceKernelSetEventFlag( UsbMainEventFlag, USB_EVENT_ATTACH );
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbDetach																	*/
/*------------------------------------------------------------------------------*/
static int UsbDetach( int arg1, int arg2, int arg3 )
{
	DisplayEnable();
	hookCtrlSetData(PreButton, 0, 0x80808080);
	PreButton = 0;
	sceKernelSetEventFlag( UsbMainEventFlag, USB_EVENT_DETACH );
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbStartFunc																	*/
/*------------------------------------------------------------------------------*/
static int UsbStartFunc( int size, void *p )
{
	memset( UsbData, 0, sizeof(UsbData) );
	memcpy( UsbData[0].devdesc, &devdesc_hi, sizeof(devdesc_hi) );
	UsbData[0].config.pconfdesc = &UsbData[0].confdesc;
	UsbData[0].config.pinterfaces = &UsbData[0].interfaces;
	UsbData[0].config.pinterdesc = &UsbData[0].interdesc;
	UsbData[0].config.pendp = &UsbData[0].endp[0];
	memcpy( UsbData[0].confdesc.desc, &confdesc_hi,  sizeof(confdesc_hi) );
	UsbData[0].confdesc.pinterfaces = &UsbData[0].interfaces;
	UsbData[0].interfaces.pinterdesc[0] = &UsbData[0].interdesc;
	UsbData[0].interfaces.intcount = 1;
	memcpy( UsbData[0].interdesc.desc, &interdesc_hi, sizeof(interdesc_hi) );
	UsbData[0].interdesc.pendp = &UsbData[0].endp[0];
	memcpy( UsbData[0].endp[0].desc, &endpdesc_hi[0], sizeof(endpdesc_hi[0]) );
	memcpy( UsbData[0].endp[1].desc, &endpdesc_hi[1], sizeof(endpdesc_hi[1]) );
	memcpy( UsbData[0].endp[2].desc, &endpdesc_hi[2], sizeof(endpdesc_hi[2]) );

	memcpy( UsbData[1].devdesc, &devdesc_full, sizeof(devdesc_full) );
	UsbData[1].config.pconfdesc = &UsbData[1].confdesc;
	UsbData[1].config.pinterfaces = &UsbData[1].interfaces;
	UsbData[1].config.pinterdesc = &UsbData[1].interdesc;
	UsbData[1].config.pendp = &UsbData[1].endp[0];
	memcpy( UsbData[1].confdesc.desc, &confdesc_full,  sizeof(confdesc_full) );
	UsbData[1].confdesc.pinterfaces = &UsbData[1].interfaces;
	UsbData[1].interfaces.pinterdesc[0] = &UsbData[1].interdesc;
	UsbData[1].interfaces.intcount = 1;
	memcpy( UsbData[1].interdesc.desc, &interdesc_full, sizeof(interdesc_full) );
	UsbData[1].interdesc.pendp = &UsbData[1].endp[0];
	memcpy( UsbData[1].endp[0].desc, &endpdesc_full[0], sizeof(endpdesc_full[0]) );
	memcpy( UsbData[1].endp[1].desc, &endpdesc_full[1], sizeof(endpdesc_full[1]) );
	memcpy( UsbData[1].endp[2].desc, &endpdesc_full[2], sizeof(endpdesc_full[2]) );

	UsbMainEventFlag = sceKernelCreateEventFlag( "USBMainEvent", 0x200, 0, NULL );
	if ( UsbMainEventFlag < 0 ){ return( -1 ); }
	UsbTransEventFlag = sceKernelCreateEventFlag( "USBEventTrans", 0x200, 0, NULL );
	if ( UsbTransEventFlag < 0 ){ return( -1 ); }
	UsbAsyncEventFlag = sceKernelCreateEventFlag( "USBEventAsync", 0x200, 0, NULL );
	if ( UsbAsyncEventFlag < 0 ){ return( -1 ); }
	UsbMainSemaphore = sceKernelCreateSema( "USBSemaphore", 0, 1, 1, NULL );
	if ( UsbMainSemaphore < 0 ){ return( -1 ); }
	UsbMainThreadID = sceKernelCreateThread( "USBMainThread", UsbMainThread, 10, 0x10000, 0, NULL );
	if ( UsbMainThreadID < 0 ){ return( -1 ); }
	sceKernelStartThread( UsbMainThreadID, 0, NULL );
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbStopFunc																	*/
/*------------------------------------------------------------------------------*/
static int UsbStopFunc( int size, void *p )
{
	if ( UsbMainThreadID >= 0 ){ sceKernelTerminateDeleteThread( UsbMainThreadID ); }
	UsbMainThreadID = -1;
	if ( UsbMainEventFlag >= 0 ){ sceKernelDeleteEventFlag( UsbMainEventFlag ); }
	UsbMainEventFlag = -1;
	if ( UsbTransEventFlag >= 0 ){ sceKernelDeleteEventFlag( UsbTransEventFlag ); }
	UsbTransEventFlag = -1;
	if ( UsbAsyncEventFlag >= 0 ){ sceKernelDeleteEventFlag( UsbAsyncEventFlag ); }
	UsbAsyncEventFlag = -1;
	if ( UsbMainSemaphore >= 0 ){ sceKernelDeleteSema( UsbMainSemaphore ); }
	UsbMainSemaphore = -1;
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* USB driver structure															*/
/*------------------------------------------------------------------------------*/
struct UsbDriver UsbDriver = {
	RJLITE_DRIVERNAME,
	4,
	UsbEndpoint,
	&UsbInterface,
	&UsbData[0].devdesc[0],
	&UsbData[0].config,
	&UsbData[1].devdesc[0],
	&UsbData[1].config,
	&StringDescriptor,
	UsbRequest,
	UsbUnknown,
	UsbAttach,
	UsbDetach,
	0,
	UsbStartFunc,
	UsbStopFunc,
	NULL
};

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* usb thread work																*/
/*------------------------------------------------------------------------------*/
static int UsbReady = 0;
static struct UsbdDeviceReq UsbBulkinReq;
static struct UsbdDeviceReq UsbBulkoutReq;
static struct UsbdDeviceReq UsbAsyncReq;
static struct AsyncEndpoint UsbAsyncEndp;
static char UsbBuff[64*1024] __attribute__((aligned(64)));
static char UsbAsyncBuff[512] __attribute__((aligned(64)));

/*------------------------------------------------------------------------------*/
/* UsbBulkinReqDone																*/
/*------------------------------------------------------------------------------*/
static int UsbBulkinReqDone( struct UsbdDeviceReq *req, int arg2, int arg3 )
{
	sceKernelSetEventFlag( UsbTransEventFlag, USB_TRANSEVENT_BULKIN_DONE );
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* SetUsbBulkinReq																*/
/*------------------------------------------------------------------------------*/
static int SetUsbBulkinReq( void *data, int size )
{
	sceKernelDcacheWritebackRange( data, size );
	memset( &UsbBulkinReq, 0, sizeof(UsbBulkinReq) );
	UsbBulkinReq.endp = &UsbEndpoint[1];
	UsbBulkinReq.data = data;
	UsbBulkinReq.size = size;
	UsbBulkinReq.func = UsbBulkinReqDone;
	sceKernelClearEventFlag( UsbTransEventFlag, ~USB_TRANSEVENT_BULKIN_DONE );
	return( sceUsbbdReqSend( &UsbBulkinReq ) );
}

/*------------------------------------------------------------------------------*/
/* UsbBulkoutReqDone															*/
/*------------------------------------------------------------------------------*/
static int UsbBulkoutReqDone( struct UsbdDeviceReq *req, int arg2, int arg3 )
{
	sceKernelSetEventFlag( UsbTransEventFlag, USB_TRANSEVENT_BULKOUT_DONE );
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* SetUsbBulkoutReq																*/
/*------------------------------------------------------------------------------*/
static int SetUsbBulkoutReq( void *data, int size )
{
	u32 upper_size = (size + 0x3F) & 0xFFFFFFC0;

	sceKernelDcacheInvalidateRange( data, upper_size );
	memset( &UsbBulkoutReq, 0, sizeof(UsbBulkoutReq) );
	UsbBulkoutReq.endp = &UsbEndpoint[2];
	UsbBulkoutReq.data = data;
	UsbBulkoutReq.size = size;
	UsbBulkoutReq.func = UsbBulkoutReqDone;
	sceKernelClearEventFlag( UsbTransEventFlag, ~USB_TRANSEVENT_BULKOUT_DONE );
	return( sceUsbbdReqRecv( &UsbBulkoutReq ) );
}

/*------------------------------------------------------------------------------*/
/* UsbAsyncReqDone																*/
/*------------------------------------------------------------------------------*/
static int UsbAsyncReqDone( struct UsbdDeviceReq *req, int arg2, int arg3 )
{
	sceKernelSetEventFlag( UsbMainEventFlag, USB_EVENT_ASYNC );
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* SetUsbAyncReq																*/
/*------------------------------------------------------------------------------*/
static int SetUsbAyncReq( void *data, int size )
{
	u32 upper_size = (size + 0x3F) & 0xFFFFFFC0;

	sceKernelDcacheInvalidateRange( data, upper_size );
	memset( &UsbAsyncReq, 0, sizeof(UsbAsyncReq) );
	UsbAsyncReq.endp = &UsbEndpoint[3];
	UsbAsyncReq.data = data;
	UsbAsyncReq.size = size;
	UsbAsyncReq.func = UsbAsyncReqDone;
	sceKernelClearEventFlag( UsbMainEventFlag, ~USB_EVENT_ASYNC );
	return( sceUsbbdReqRecv( &UsbAsyncReq ) );
}

/*------------------------------------------------------------------------------*/
/* UsbReadData																	*/
/*------------------------------------------------------------------------------*/
static int UsbReadData( void *data, int data_size )
{
	u32 result;
	int read_size = 0;

	while ( read_size < data_size ){
		int rest_size = data_size - read_size;
		if ( rest_size > sizeof(UsbBuff) ){ rest_size = sizeof(UsbBuff); }
		SetUsbBulkoutReq( UsbBuff, rest_size );
		int ret = sceKernelWaitEventFlag( UsbTransEventFlag, USB_TRANSEVENT_BULKOUT_DONE,
										  PSP_EVENT_WAITOR | PSP_EVENT_WAITCLEAR, &result, NULL );
		if ( ret == 0 ){
			if ( (UsbBulkoutReq.retcode == 0) && (UsbBulkoutReq.recvsize > 0) ){
				read_size += UsbBulkoutReq.recvsize;
				memcpy( data, UsbBuff, UsbBulkoutReq.recvsize );
				data += UsbBulkoutReq.recvsize;
			} else {
				return( -1 );
			}
		} else {
			return( -1 );
		}
	}
	return( read_size );
}

/*------------------------------------------------------------------------------*/
/* UsbWriteData																	*/
/*------------------------------------------------------------------------------*/
static int UsbWriteData( const void *data, int data_size )
{
	u32 result;
	int write_size = 0;

	if ( (u32)data & 0x3F ){
		while ( write_size < data_size ){
			int rest_size = data_size - write_size;
			if ( rest_size > sizeof(UsbBuff) ){ rest_size = sizeof(UsbBuff); }
			memcpy( UsbBuff, data, rest_size );
			SetUsbBulkinReq( UsbBuff, rest_size );
			int ret = sceKernelWaitEventFlag( UsbTransEventFlag, USB_TRANSEVENT_BULKIN_DONE,
											  PSP_EVENT_WAITOR | PSP_EVENT_WAITCLEAR, &result, NULL );
			if ( ret == 0 ){
				if ( (UsbBulkinReq.retcode == 0) && (UsbBulkinReq.recvsize > 0) ){
					write_size += UsbBulkinReq.recvsize;
					data += UsbBulkinReq.recvsize;
				} else {
					return( -1 );
				}
			} else {
				return( -1 );
			}
		}
	} else {
		while ( write_size < data_size ){
			int rest_size = data_size - write_size;
			if ( rest_size > sizeof(UsbBuff) ){ rest_size = sizeof(UsbBuff); }
			SetUsbBulkinReq( (char *)data, rest_size );
			int ret = sceKernelWaitEventFlag( UsbTransEventFlag, USB_TRANSEVENT_BULKIN_DONE,
											  PSP_EVENT_WAITOR | PSP_EVENT_WAITCLEAR, &result, NULL );
			if ( ret == 0 ){
				if ( (UsbBulkinReq.retcode == 0) && (UsbBulkinReq.recvsize > 0) ){
					write_size += UsbBulkinReq.recvsize;
					data += UsbBulkinReq.recvsize;
				} else {
					return( -1 );
				}
			} else {
				return( -1 );
			}
		}
	}
	return( write_size );
}

/*------------------------------------------------------------------------------*/
/* UsbWriteLargeData															*/
/*------------------------------------------------------------------------------*/
#ifndef RELEASE
extern int sceDisplayGetAccumulatedHcount( void );
static int HcountUsbWaitTop = 0;
static int HcountUsbWaitSub = 0;
int GetHcountUsbWait( void ){ return( HcountUsbWaitSub ); }
#endif

static int UsbWriteLargeData( const void *data, int data_size )
{
	int ret;
	u32 result;

	if ( (u32)data & 0x3F ){ return( -1 ); }
	SetUsbBulkinReq( (char *)data, data_size );
#ifndef RELEASE
	if(DebugMode){
		HcountUsbWaitTop = sceDisplayGetAccumulatedHcount();
	}
#endif
	ret = sceKernelWaitEventFlag( UsbTransEventFlag, USB_TRANSEVENT_BULKIN_DONE,
								  PSP_EVENT_WAITOR | PSP_EVENT_WAITCLEAR, &result, NULL );
#ifndef RELEASE
	if(DebugMode){
		HcountUsbWaitSub = sceDisplayGetAccumulatedHcount() - HcountUsbWaitTop;
	}
#endif
	if ( ret == 0 ){
		if ( (UsbBulkinReq.retcode == 0) && (UsbBulkinReq.recvsize > 0) ){
		} else {
			return( -1 );
		}
	} else {
		return( -1 );
	}
	return( data_size );
}

/*------------------------------------------------------------------------------*/
/* UsbCommandXchg																*/
/*------------------------------------------------------------------------------*/
static int UsbCommandXchg( void *outcmd, int outcmdlen, void *incmd, int incmdlen )
{
	int ret = 0;
	int err = 0;

	err = sceKernelWaitSema( UsbMainSemaphore, 1, NULL );
	if ( err < 0 ){ return( 0 ); }
	do {
		struct HostFsCmd *cmd = (struct HostFsCmd *) outcmd;
		struct HostFsCmd *res = (struct HostFsCmd *) incmd;

		if ( outcmdlen > 0 ){
			err = UsbWriteData( outcmd, outcmdlen );
			if ( err != outcmdlen ){ break; }
		}
		if ( incmdlen > 0 ){
			err = UsbReadData( incmd, incmdlen );
			if ( err != incmdlen ){ break; }
			if ( (res->magic != HOSTFS_MAGIC) && (res->command != cmd->command) ){ break; }
		}
		ret = 1;
	} while( 0 );
	sceKernelSignalSema( UsbMainSemaphore, 1 );
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* send_async																	*/
/*------------------------------------------------------------------------------*/
static int send_async( void *data, int len )
{
	int ret = 0;
	int err = 0;

	err = sceKernelWaitSema( UsbMainSemaphore, 1, NULL );
	if ( err < 0 ){ return( 0 ); }
	do {
		if ( (data) && (len > 0) ){
			err = UsbWriteData( data, len );
			if ( err != len ){ break; }
		}
		ret = 1;
	} while( 0 );
	sceKernelSignalSema( UsbMainSemaphore, 1 );
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* send_hello_cmd																*/
/*------------------------------------------------------------------------------*/
static int send_hello_cmd( void )
{
	struct HostFsHelloCmd cmd;
	struct HostFsHelloResp res;

	memset( &cmd, 0, sizeof(cmd) );
	cmd.cmd.magic   = HOSTFS_MAGIC;
	cmd.cmd.command = HOSTFS_CMD_HELLO(RJL_VERSION);

	return( UsbCommandXchg( &cmd, sizeof(cmd), &res, sizeof(res) ) );
}

/*------------------------------------------------------------------------------*/
/* fill_async																	*/
/*------------------------------------------------------------------------------*/
static void fill_async( void *async_data, int len )
{
	struct AsyncCommand *cmd;
	unsigned char *data;
	int sizeleft;
	int intc;

	if ( len > sizeof(struct AsyncCommand) ){
		len -= sizeof(struct AsyncCommand);
		data = async_data + sizeof(struct AsyncCommand);
		cmd = (struct AsyncCommand *) async_data;

		intc = pspSdkDisableInterrupts();
		if( (cmd->magic == ASYNC_MAGIC) && (cmd->channel == ASYNC_USER) ){
			sizeleft = len < (MAX_ASYNC_BUFFER - UsbAsyncEndp.size) ? len : (MAX_ASYNC_BUFFER - UsbAsyncEndp.size);
			while ( sizeleft > 0 ){
				UsbAsyncEndp.buffer[UsbAsyncEndp.write_pos++] = *data++;
				UsbAsyncEndp.write_pos %= MAX_ASYNC_BUFFER;
				UsbAsyncEndp.size++;
				sizeleft--;
			}
			sceKernelSetEventFlag( UsbAsyncEventFlag, (1 << cmd->channel) );
		}
		pspSdkEnableInterrupts( intc );
	}
}

/*------------------------------------------------------------------------------*/
/* UsbMainThread																*/
/*------------------------------------------------------------------------------*/
static int UsbMainThread( SceSize size, void *argp )
{
	int ret;
	u32 result;

	while( 1 ){
		ret = sceKernelWaitEventFlag( UsbMainEventFlag,
									  USB_EVENT_ATTACH | USB_EVENT_DETACH | USB_EVENT_ASYNC,
									  PSP_EVENT_WAITOR | PSP_EVENT_WAITCLEAR, &result, NULL );
		if ( ret < 0 ){ sceKernelExitDeleteThread( 0 ); }
		if ( result & USB_EVENT_ASYNC ){
			if ( (UsbAsyncReq.retcode == 0) && (UsbAsyncReq.recvsize > 0) ){
				fill_async( UsbAsyncBuff, UsbAsyncReq.recvsize );
				SetUsbAyncReq( UsbAsyncBuff, sizeof(UsbAsyncBuff) );
			}
		}
		if ( result & USB_EVENT_DETACH ){
			UsbReady = 0;
			sceKernelClearEventFlag( UsbMainEventFlag, ~USB_EVENT_CONNECT );
		}
		if ( result & USB_EVENT_ATTACH ){
			uint32_t magic;
			UsbReady = 0;
			sceKernelClearEventFlag( UsbMainEventFlag, ~USB_EVENT_CONNECT );
			if ( UsbReadData( &magic, sizeof(magic) ) == sizeof(magic) ){
				if ( magic == HOSTFS_MAGIC ){
					if ( send_hello_cmd() ){
						SetUsbAyncReq( UsbAsyncBuff, sizeof(UsbAsyncBuff) );
						UsbReady = 1;
						sceKernelSetEventFlag( UsbMainEventFlag, USB_EVENT_CONNECT );
					}
				}
			}
		}
	}
	return( 0 );
}

/********************************************************************************/
/*------------------------------------------------------------------------------*/
/* UsbbdRegister																*/
/*------------------------------------------------------------------------------*/
void UsbbdRegister( void )
{
	sceUsbbdRegister( &UsbDriver );
}

/*------------------------------------------------------------------------------*/
/* UsbbdUnregister																*/
/*------------------------------------------------------------------------------*/
void UsbbdUnregister( void )
{
	sceUsbbdUnregister( &UsbDriver );
}

/*------------------------------------------------------------------------------*/
/* UsbStart																		*/
/*------------------------------------------------------------------------------*/
int UsbStart( void )
{
	int ret;

	ret = sceUsbStart( PSP_USBBUS_DRIVERNAME, 0, 0 );
	if ( ret != 0 ){ return( -1 ); }
	ret = sceUsbStart( RJLITE_DRIVERNAME, 0, 0 );
	if ( ret != 0 ){ return( -1 ); }
	ret = sceUsbActivate( RJLITE_DRIVERPID );
	if ( ret != 0 ){ return( -1 ); }
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbStop																		*/
/*------------------------------------------------------------------------------*/
int UsbStop( void )
{
	int ret;

	ret = sceUsbDeactivate( RJLITE_DRIVERPID );
	if ( ret != 0 ){ return( -1 ); }
	ret = sceUsbStop( RJLITE_DRIVERNAME, 0, 0 );
	if ( ret != 0 ){ return( -1 ); }
	ret = sceUsbStop( PSP_USBBUS_DRIVERNAME, 0, 0 );
	if ( ret != 0 ){ return( -1 ); }
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbResume																	*/
/*------------------------------------------------------------------------------*/
int UsbResume( void )
{
	int ret;

	ret = sceUsbActivate( RJLITE_DRIVERPID );
	if ( ret != 0 ){ return( -1 ); }
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbSuspend																	*/
/*------------------------------------------------------------------------------*/
int UsbSuspend( void )
{
	int ret;

	ret = sceUsbDeactivate( RJLITE_DRIVERPID );
	if ( ret != 0 ){ return( -1 ); }
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbWait																		*/
/*------------------------------------------------------------------------------*/
int UsbWait( void )
{
	int ret;

	while ( UsbMainEventFlag < 0 ){ sceKernelDelayThread( 100000 ); }
	ret = sceKernelWaitEventFlag( UsbMainEventFlag, USB_EVENT_CONNECT,
								  PSP_EVENT_WAITOR, NULL, NULL );
	return( 0 );
}

/*------------------------------------------------------------------------------*/
/* UsbAsyncFlush																*/
/*------------------------------------------------------------------------------*/
void UsbAsyncFlush( void )
{
	int intc;
	intc = pspSdkDisableInterrupts();
	UsbAsyncEndp.size      = 0;
	UsbAsyncEndp.read_pos  = 0;
	UsbAsyncEndp.write_pos = 0;
	sceKernelClearEventFlag( UsbAsyncEventFlag, ~(1 << ASYNC_USER) );
	pspSdkEnableInterrupts( intc );
}

/*------------------------------------------------------------------------------*/
/* UsbAsyncWrite																*/
/*------------------------------------------------------------------------------*/
int UsbAsyncWrite( const void *data, int len )
{
	int ret = -1;
	char buffer[512];
	struct AsyncCommand *cmd;
	int written = 0;
	int k1;

	k1 = psplinkSetK1( 0 );
	do {
		if ( UsbReady == 0 ){ break; }

		cmd = (struct AsyncCommand *) buffer;
		cmd->magic   = ASYNC_MAGIC;
		cmd->channel = ASYNC_USER;

		while ( written < len ){
			int size;
			size = (len-written) > (sizeof(buffer)-sizeof(struct AsyncCommand)) ? (sizeof(buffer)-sizeof(struct AsyncCommand)) : (len-written);
			memcpy( &buffer[sizeof(struct AsyncCommand)], data+written, size );
			if ( send_async( buffer, size + sizeof(struct AsyncCommand)) ){
				written += size;
			} else {
				break;
			}
		}
		ret = written;
	} while( 0 );
	psplinkSetK1( k1 );
	return( ret );
}

/*------------------------------------------------------------------------------*/
/* UsbAsyncRead																	*/
/*------------------------------------------------------------------------------*/
int UsbAsyncRead( unsigned char *data, int len )
{
	int ret;
	int intc;
	int i;
	int k1;
	SceUInt *pTimeout = NULL;

	k1 = psplinkSetK1( 0 );

	ret = sceKernelWaitEventFlag( UsbAsyncEventFlag, 1 << ASYNC_USER,
								  PSP_EVENT_WAITOR | PSP_EVENT_WAITCLEAR, NULL, pTimeout );
	if ( ret < 0 ){ return( -1 ); }

	intc = pspSdkDisableInterrupts();
	len = len < UsbAsyncEndp.size ? len : UsbAsyncEndp.size;
	for ( i=0; i<len; i++ ){
		data[i] = UsbAsyncEndp.buffer[UsbAsyncEndp.read_pos++];
		UsbAsyncEndp.read_pos %= MAX_ASYNC_BUFFER;
		UsbAsyncEndp.size--;
	}
	if ( UsbAsyncEndp.size != 0 ){
		sceKernelSetEventFlag( UsbAsyncEventFlag, 1 << ASYNC_USER );
	}
	pspSdkEnableInterrupts( intc );
	psplinkSetK1( k1 );
	return( len );
}

/*------------------------------------------------------------------------------*/
/* UsbWriteBulkData																*/
/*------------------------------------------------------------------------------*/
void UsbWriteBulkData( const void *data, int len )
{
	int err;
	struct BulkCommand cmd;
	int k1;

	k1 = psplinkSetK1( 0 );
	do {
		if ( UsbReady == 0 ){ break; }
		if ( (len <= 0) || (len > HOSTFS_BULK_MAXWRITE) ){ break; }

		cmd.magic   = BULK_MAGIC;
		cmd.channel = ASYNC_USER;
		cmd.size    = len;

		err = sceKernelWaitSema( UsbMainSemaphore, 1, NULL );
		if ( err < 0 ){ break; }
		do {
			err = UsbWriteData( &cmd, sizeof(cmd) );
			if ( err != sizeof(cmd) ){ break; }
//			err = UsbWriteData( data, len );
			err = UsbWriteLargeData( data, len );
			if ( err != len ){ break; }
		} while( 0 );
		sceKernelSignalSema( UsbMainSemaphore, 1 );
	} while( 0 );
	psplinkSetK1( k1 );
}
