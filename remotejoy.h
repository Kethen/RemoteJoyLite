#ifndef _REMOTE_JOY_H_
#define _REMOTE_JOY_H_

#define HOSTFSDRIVER_PID		(0x1C9)
#define SONY_VID				(0x54C)

#define HOSTFS_MAX_BLOCK		(64*1024)
#define HOSTFS_BULK_MAXWRITE	(1024*1024)

#define HOSTFS_MAGIC			0x782F0812
#define ASYNC_MAGIC				0x782F0813
#define BULK_MAGIC				0x782F0814
#define JOY_MAGIC				0x909ACCEF

#define TYPE_JOY_CMD			1
#define TYPE_JOY_DAT			2

#define ASYNC_CMD_DEBUG			1

#define RJL_VERSION				190
#define HOSTFS_CMD_HELLO(ver)	((0x8FFC<<16)|ver)

/* Screen commands */
#define SCREEN_CMD_ACTIVE		(1<<0)
#define SCREEN_CMD_SCROFF		(1<<1)
#define SCREEN_CMD_DEBUG		(1<<2)
#define SCREEN_CMD_ASYNC		(1<<3)

/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/* |    ADRESS2    |    ADRESS1    |  PRIORITY |  MODE |FPS|A|D|S|A| */
/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
#define SCREEN_CMD_SET_TRNSFPS(x)	((x)<<4)
#define SCREEN_CMD_GET_TRNSFPS(x)	(((x)>>4)&0x03)
#define SCREEN_CMD_SET_TRNSMODE(x)	((x)<<6)
#define SCREEN_CMD_GET_TRNSMODE(x)	(((x)>>6)&0x0F)
#define SCREEN_CMD_SET_PRIORITY(x)	((x)<<10)
#define SCREEN_CMD_GET_PRIORITY(x)	(((x)>>10)&0x3F)
#define SCREEN_CMD_SET_ADRESS1(x)	((x)<<16)
#define SCREEN_CMD_GET_ADRESS1(x)	(((x)>>16)&0xFF)
#define SCREEN_CMD_SET_ADRESS2(x)	((x)<<24)
#define SCREEN_CMD_GET_ADRESS2(x)	(((x)>>24)&0xFF)

/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/* |      TRNSH      |    TRNSW    |      TRNSY      |    TRNSX    | */
/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
#define SCREEN_CMD_SET_TRNSX(x)		((x)<<0)
#define SCREEN_CMD_GET_TRNSX(x)		(((x)>>0)&0x7F)
#define SCREEN_CMD_SET_TRNSY(x)		((x)<<7)
#define SCREEN_CMD_GET_TRNSY(x)		(((x)>>7)&0x1FF)
#define SCREEN_CMD_SET_TRNSW(x)		((x)<<16)
#define SCREEN_CMD_GET_TRNSW(x)		(((x)>>16)&0x7F)
#define SCREEN_CMD_SET_TRNSH(x)		((x)<<23)
#define SCREEN_CMD_GET_TRNSH(x)		(((x)>>23)&0x1FF)

struct JoyEvent
{
	uint32_t magic;
	int32_t type;
	uint32_t value1;
	uint32_t value2;
} __attribute__((packed));

struct JoyScrHeader
{
	uint32_t magic;
	int32_t mode; /* 0-3 */
	int32_t size;
	int32_t ref;
} __attribute__((packed));

enum USB_ASYNC_CHANNELS
{
	ASYNC_SHELL    = 0,
	ASYNC_GDB      = 1,
	ASYNC_STDOUT   = 2,
	ASYNC_STDERR   = 3,
	ASYNC_USER     = 4,
};

struct HostFsCmd
{
	uint32_t magic;
	uint32_t command;
	uint32_t extralen;
} __attribute__((packed));

struct HostFsHelloCmd
{
	struct HostFsCmd cmd;
} __attribute__((packed));

struct HostFsHelloResp
{
	struct HostFsCmd cmd;
} __attribute__((packed));

struct AsyncCommand
{
	uint32_t magic;
	uint32_t channel;
} __attribute__((packed));

struct BulkCommand
{
	uint32_t magic;
	uint32_t channel;
	uint32_t size;
} __attribute__((packed));

#endif	// _REMOTE_JOY_H_
