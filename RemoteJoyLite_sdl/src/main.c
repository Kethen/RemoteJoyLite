#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdint.h>

#include "remotejoy.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

#define EP_IN 0x81
#define EP_OUT 0x02
#define EP_OUT_EVENT 0x03

// sdl stuff
static SDL_Window *gWindow     = NULL;
static SDL_Renderer *gRenderer = NULL;
static SDL_Thread *gUsbThread  = NULL;
static SDL_Texture *gTex       = NULL;
static SDL_Gamepad *gGamepad   = NULL;

// usb stuff
static int gUsbHostfsReady = 0;
static int gUsbHostfsExit  = 0;
static libusb_device_handle *gUsbDev;
static int gPSPReady     = 0;
static int gUsbHostError = 0;

// framebuffer
static uint8_t gBuf[SCREEN_WIDTH * SCREEN_HEIGHT * 4];
static uint32_t gBufSize = 0;
static uint8_t gBufMode;

static void usbInit()
{
  libusb_init_context(NULL, NULL, 0);
}

static void usbQuit()
{
  libusb_exit(NULL);
}

static void usbCloseDevice(void)
{
  if (gUsbDev != NULL)
  {
    libusb_release_interface(gUsbDev, 0);
    libusb_reset_device(gUsbDev);
    libusb_close(gUsbDev);
  }
  gUsbDev = NULL;
}

static int usbCheckDevice(void)
{
  if (gUsbDev == NULL)
  {
    return -1;
  }
  int mag = HOSTFS_MAGIC;
  int len = 0;
  int ret = libusb_bulk_transfer(gUsbDev, EP_OUT, (char *)&mag, 4, &len, 1000);
  if (len == 4)
  {
    return 0;
  }
  return -1;
}

static void usbOpenDevice(void)
{
  while (1)
  {
    libusb_device_handle *handle = libusb_open_device_with_vid_pid(NULL, SONY_VID, HOSTFSDRIVER_PID);
    if (handle)
    {
      int ret = libusb_set_configuration(handle, 1);
      if (ret == 0)
      {
        ret = libusb_claim_interface(handle, 0);
        if (ret == 0)
        {
          printf("%s: psp usb interface claimed, ready to go\n", __func__);
          gUsbDev = handle;
          return;
        }
        else
        {
          fprintf(stderr, "%s: failed to claim interface, %d\n", __func__, ret);
        }
      }
      else
      {
        fprintf(stderr, "%s: failed to set config, %d\n", __func__, ret);
      }
      libusb_close(handle);
    }

    SDL_Delay(1000);

    if (gUsbHostfsExit != 0)
    {
      return;
    }
  }
}

static void remotejoyAsync(void *read, int read_len)
{
  if (read_len < (int)sizeof(struct JoyScrHeader))
  {
    return;
  }

  struct JoyScrHeader *cmd = (struct JoyScrHeader *)read;

  if (cmd->mode == ASYNC_CMD_DEBUG)
  {
    // dprintf( 0, 0, "%s", (void *)(cmd+1) );
    printf("psp debug message begin:\n");
    printf("%s", (void *)(cmd + 1));
    printf("psp debug message end:\n");
  }
}

static void remotejoyBulk(void *read, int read_len)
{
  struct JoyScrHeader *cmd = (struct JoyScrHeader *)read;
  //        printf("mode: 0x%08x\n", cmd->mode);
  //        printf("vcount: 0x%08x\n", cmd->ref);

  memcpy(gBuf, (void *)(cmd + 1), cmd->size);
  gBufSize = cmd->size;
  gBufMode = cmd->mode;
}

static void sendEvent(int type, uint32_t value1, uint32_t value2)
{
  int ret;
  struct
  {
    struct AsyncCommand async;
    struct JoyEvent event;
  } data;

  if (gPSPReady == 0)
  {
    return;
  }
  data.async.magic   = ASYNC_MAGIC;
  data.async.channel = ASYNC_USER;
  data.event.magic   = JOY_MAGIC;
  data.event.type    = type;
  data.event.value1  = value1;
  data.event.value2  = value2;
  int len            = 0;
  ret                = libusb_bulk_transfer(gUsbDev, EP_OUT_EVENT, (char *)&data, sizeof(data), &len, 10000);
  if (ret < 0)
  {
    return;
  }
}

void sendPSPCmd(void)
{
  // TODO: un-hardcode and use settings
  uint32_t arg1 = SCREEN_CMD_ACTIVE;
  //        arg1 |= SCREEN_CMD_DEBUG;
  arg1 |= SCREEN_CMD_ASYNC;

  arg1 |= SCREEN_CMD_SET_TRNSFPS(1);
  arg1 |= SCREEN_CMD_SET_TRNSMODE(1);
  arg1 |= SCREEN_CMD_SET_PRIORITY(16);
  arg1 |= SCREEN_CMD_SET_ADRESS1(88);
  arg1 |= SCREEN_CMD_SET_ADRESS2(65 - 1);

  uint32_t arg2 = 0;
  arg2 |= SCREEN_CMD_SET_TRNSX(0);
  arg2 |= SCREEN_CMD_SET_TRNSY(0);
  arg2 |= SCREEN_CMD_SET_TRNSW(480 / 32);
  arg2 |= SCREEN_CMD_SET_TRNSH(272 / 2);

  sendEvent(TYPE_JOY_CMD, arg1, arg2);
}

static void handleHello(void)
{
  int ret;
  struct HostFsHelloResp Resp;

  memset(&Resp, 0, sizeof(Resp));
  Resp.cmd.magic   = HOSTFS_MAGIC;
  Resp.cmd.command = HOSTFS_CMD_HELLO(RJL_VERSION);
  int len          = 0;
  ret              = libusb_bulk_transfer(gUsbDev, EP_OUT, (char *)&Resp, sizeof(Resp), &len, 10000);
  if (ret < 0)
  {
    return;
  }

  gPSPReady = 1;

  sendPSPCmd();
}

static void doDefault(void *read, int read_len) { }

static void doHostfs(void *read, int read_len)
{
  struct HostFsCmd *cmd = (struct HostFsCmd *)read;

  if (read_len < (int)sizeof(struct HostFsCmd))
  {
    return;
  }

  if ((int)cmd->command == HOSTFS_CMD_HELLO(RJL_VERSION))
  {
    gUsbHostError = 0;
    handleHello();
  }
  else
  {
    gUsbHostError = 1;
  }
}

static void doAsync(void *read, int read_len)
{
  struct AsyncCommand *cmd = (struct AsyncCommand *)read;

  if (read_len < (int)sizeof(struct AsyncCommand))
  {
    return;
  }
  if (read_len > (int)sizeof(struct AsyncCommand))
  {
    char *data = (char *)cmd + sizeof(struct AsyncCommand);
    remotejoyAsync(data, read_len - sizeof(struct AsyncCommand));
  }
}

static char gBulkBlock[HOSTFS_BULK_MAXWRITE];

static void doBulk(void *read, int read_len)
{
  struct BulkCommand *cmd = (struct BulkCommand *)read;

  if (read_len < (int)sizeof(struct BulkCommand))
  {
    return;
  }

  int read_size = 0;
  int data_size = cmd->size;

  while (read_size < data_size)
  {
    int rest_size = data_size - read_size;
    if (rest_size > HOSTFS_MAX_BLOCK)
    {
      rest_size = HOSTFS_MAX_BLOCK;
    }
    int len = 0;
    int ret = libusb_bulk_transfer(gUsbDev, EP_IN, &gBulkBlock[read_size], rest_size, &len, 3000);

    if (ret == LIBUSB_ERROR_TIMEOUT)
    {
      continue;
    }
    if (ret < 0)
    {
      break;
    }
    read_size += len;
  }

  remotejoyBulk(gBulkBlock, data_size);
}

static int UsbThread(void *ptr)
{
  usbInit();

  while (1)
  {
    usbOpenDevice();
    if (usbCheckDevice() == 0)
    {
      gUsbHostfsReady = 1;
      while (gUsbHostfsExit == 0)
      {
        uint32_t data[512 / sizeof(uint32_t)];
        int len = 0;
        int ret = libusb_bulk_transfer(gUsbDev, EP_IN, (char *)data, 512, &len, 1000);
        if (ret == LIBUSB_ERROR_TIMEOUT)
        {
          continue;
        }
        if (ret < 0)
        {
          printf("%s: usb read error %d\n", __func__, ret);
          break;
        }
        if (len < 4)
        {
          printf("%s: tiny read\n", __func__);
          continue;
        }

        switch (data[0])
        {
          default:
            doDefault(data, len);
            break;
          case HOSTFS_MAGIC:
            doHostfs(data, len);
            break;
          case ASYNC_MAGIC:
            doAsync(data, len);
            break;
          case BULK_MAGIC:
            doBulk(data, len);
            break;
        }
        SDL_Delay(10);
      }
      gPSPReady       = 0;
      gUsbHostfsReady = 0;
    }
    usbCloseDevice();
    if (gUsbHostfsExit != 0)
    {
      break;
    }
  }

  gUsbHostfsExit = 2;

  usbQuit();
  return 0;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  SDL_SetAppMetadata("RemoteJoyLite", "0.19", "com.psparchive.rjl");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) < 0)
    return -1;

  SDL_CreateWindowAndRenderer("RemoteJoyLite", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE, &gWindow, &gRenderer);
  SDL_SetRenderLogicalPresentation(gRenderer, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  SDL_SetRenderVSync(gRenderer, -1);

  gTex = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
                           SCREEN_HEIGHT);
  // TODO: make configurable/switchable
  SDL_SetTextureScaleMode(gTex, SDL_SCALEMODE_NEAREST);

  gUsbThread = SDL_CreateThread(UsbThread, "UsbThread", (void *)NULL);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
  SDL_RenderClear(gRenderer);

  int pitch              = SCREEN_WIDTH * 2;
  SDL_PixelFormat format = SDL_PIXELFORMAT_BGR565;

  switch (gBufMode >> 4)
  {
    case 0x00:
      break;
    case 0x01:
      format = SDL_PIXELFORMAT_XBGR1555;
      break;
    case 0x02:
      format = SDL_PIXELFORMAT_BGRA4444;
      break;
    case 0x03:
      format = SDL_PIXELFORMAT_BGRA8888;
      pitch  = SCREEN_WIDTH * 4;
      break;
  }

  SDL_Surface *src = SDL_CreateSurfaceFrom(SCREEN_WIDTH, SCREEN_HEIGHT, format, gBuf, pitch);

  SDL_Surface *dst;
  if (SDL_LockTextureToSurface(gTex, NULL, &dst))
  {
    SDL_BlitSurface(src, NULL, dst, NULL);
    SDL_UnlockTexture(gTex);
    SDL_DestroySurface(src);
  }

  SDL_RenderTexture(gRenderer, gTex, NULL, NULL);

  SDL_RenderPresent(gRenderer);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  switch (event->type)
  {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_GAMEPAD_ADDED:
      // TODO: handle multiple gamepads
      const SDL_JoystickID which = event->gdevice.which;
      gGamepad                   = SDL_OpenGamepad(which);
      break;
    case SDL_EVENT_GAMEPAD_REMOVED:
      // TODO
      break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    {
      uint16_t laxis_x  = SDL_GetGamepadAxis(gGamepad, SDL_GAMEPAD_AXIS_LEFTX) / 256;
      uint16_t laxis_y  = SDL_GetGamepadAxis(gGamepad, SDL_GAMEPAD_AXIS_LEFTY) / 256;
      uint16_t raxis_x  = SDL_GetGamepadAxis(gGamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 256;
      uint16_t raxis_y  = SDL_GetGamepadAxis(gGamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 256;
      uint32_t axisData = laxis_x | (laxis_y << 16) | (raxis_x << 8) | (raxis_y << 24);

      uint32_t buttons = 0;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_BACK) << 0;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_START) << 3;

      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) << 4;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) << 5;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) << 6;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) << 7;

      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) << 8;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) << 9;

      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_NORTH) << 12;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_EAST) << 13;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_SOUTH) << 14;
      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_WEST) << 15;

      buttons |= SDL_GetGamepadButton(gGamepad, SDL_GAMEPAD_BUTTON_GUIDE) << 16;

      sendEvent(TYPE_JOY_DAT, buttons, axisData);
    }
    break;
    case SDL_EVENT_KEY_DOWN:
      // TODO: keyboard controls?
      switch (event->key.scancode)
      {
        case SDL_SCANCODE_F12:
        {
          char filename[512];
          time_t now;
          struct tm *date;

          time(&now);
          date       = localtime(&now);
          int year   = date->tm_year + 1900;
          int month  = date->tm_mon + 1;
          int day    = date->tm_mday;
          int hour   = date->tm_hour;
          int minute = date->tm_min;
          int second = date->tm_sec;

          sprintf(filename, "Capture_%04d%02d%02d%02d%02d%02d.bmp", year, month, day, hour, minute, second);

          SDL_Surface *sshot = SDL_RenderReadPixels(gRenderer, NULL);
          SDL_SaveBMP(sshot, filename);
          SDL_DestroySurface(sshot);
        }
        break;
        case SDL_SCANCODE_1:
          SDL_SetWindowSize(gWindow, SCREEN_WIDTH, SCREEN_HEIGHT);
          break;
        case SDL_SCANCODE_2:
          SDL_SetWindowSize(gWindow, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2);
          break;
        case SDL_SCANCODE_3:
          SDL_SetWindowSize(gWindow, SCREEN_WIDTH * 3, SCREEN_HEIGHT * 3);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  gUsbHostfsExit = 2;
  SDL_WaitThread(gUsbThread, NULL);
  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  gWindow   = NULL;
  gRenderer = NULL;
}
