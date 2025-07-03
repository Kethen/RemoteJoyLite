
let frames = 0;
let tm = Date.now();
let usbd = null;
let canvas = null;
let ctxWorker = null;
let gPSPReady = 0;
let gUsbhostfsError = 0;
let gUsbhostfsExit = 0;
let gUsbhostfsReady = 0;

let dataBuffer = new Uint8ClampedArray(480*272*4 + 16); // cmd + max buffer

const HOSTFS_MAGIC = 0x782F0812;
const ASYNC_MAGIC = 0x782F0813;
const BULK_MAGIC = 0x782F0814;
const JOY_MAGIC = 0x909ACCEF;

const TYPE_JOY_CMD = 1;
const TYPE_JOY_DAT = 2;

const ASYNC_CMD_DEBUG = 1;

const ASYNC_USER = 4;

const RJL_VERSION = 190;
const HOSTFS_CMD_HELLO = ((0x8FFC<<16)|RJL_VERSION);

const SCREEN_CMD_ACTIVE = 1;
const SCREEN_CMD_SCROFF = 2;
const SCREEN_CMD_DEBUG  = 4;
const SCREEN_CMD_ASYNC  = 8;


function SCREEN_CMD_SET_TRNSFPS(x)  { return    ((x)<<4); }
function SCREEN_CMD_GET_TRNSFPS(x)  { return    (((x)>>4)&0x03); }
function SCREEN_CMD_SET_TRNSMODE(x) { return    ((x)<<6); }
function SCREEN_CMD_GET_TRNSMODE(x) { return    (((x)>>6)&0x0F); }
function SCREEN_CMD_SET_PRIORITY(x) { return    ((x)<<10); }
function SCREEN_CMD_GET_PRIORITY(x) { return    (((x)>>10)&0x3F); }
function SCREEN_CMD_SET_ADRESS1(x)  { return    ((x)<<16); }
function SCREEN_CMD_GET_ADRESS1(x)  { return    (((x)>>16)&0xFF); }
function SCREEN_CMD_SET_ADRESS2(x)  { return    ((x)<<24); }
function SCREEN_CMD_GET_ADRESS2(x)  { return    (((x)>>24)&0xFF); }

function SCREEN_CMD_SET_TRNSX(x)    { return    ((x)<<0); }
function SCREEN_CMD_GET_TRNSX(x)    { return    (((x)>>0)&0x7F); }
function SCREEN_CMD_SET_TRNSY(x)    { return    ((x)<<7); }
function SCREEN_CMD_GET_TRNSY(x)    { return    (((x)>>7)&0x1FF); }
function SCREEN_CMD_SET_TRNSW(x)    { return    ((x)<<16); }
function SCREEN_CMD_GET_TRNSW(x)    { return    (((x)>>16)&0x7F); }
function SCREEN_CMD_SET_TRNSH(x)    { return    ((x)<<23); }
function SCREEN_CMD_GET_TRNSH(x)    { return    (((x)>>23)&0x1FF); }



async function sendEvent(type, arg1, arg2)
{
    if (!gPSPReady) return;
    let data = new Uint32Array(2 + 4); // async command + joy command
    data[0] = ASYNC_MAGIC;
    data[1] = ASYNC_USER;

    data[2] = JOY_MAGIC;
    data[3] = type;
    data[4] = arg1;
    data[5] = arg2;

    let d8 = new Uint8Array(data.buffer);

    try {
      await usbd.transferOut(3, d8);
    } catch(e) {
      usbd.close();
      usbd = null;
      gUsbhostfsError = 1;
      gPSPREady = 0;
    }
}

async function sendInput(buttons, axis)
{
    sendEvent(TYPE_JOY_DAT, buttons, axis);
}

async function sendPSPCommand()
{
  let arg1 = 0;
  let arg2 = 0;

  // TODO

  arg1 |= SCREEN_CMD_ACTIVE;
  //        arg1 |= SCREEN_CMD_DEBUG;
  arg1 |= SCREEN_CMD_ASYNC;

  arg1 |= SCREEN_CMD_SET_TRNSFPS(0);
  arg1 |= SCREEN_CMD_SET_TRNSMODE(1);
  arg1 |= SCREEN_CMD_SET_PRIORITY(16);
  arg1 |= SCREEN_CMD_SET_ADRESS1(88);
  arg1 |= SCREEN_CMD_SET_ADRESS2(65 - 1);

  arg2 |= SCREEN_CMD_SET_TRNSX(0);
  arg2 |= SCREEN_CMD_SET_TRNSY(0);
  arg2 |= SCREEN_CMD_SET_TRNSW(480 / 32);
  arg2 |= SCREEN_CMD_SET_TRNSH(272 / 2);

  await sendEvent(TYPE_JOY_CMD, arg1, arg2);
}

async function handleHello()
{
    const resp = new Uint32Array(3);
    resp[0] = HOSTFS_MAGIC;
    resp[1] = HOSTFS_CMD_HELLO;
    resp[2] = 0;
    try {
      await usbd.transferOut(2, resp);
    } catch(e) {
      usbd.close();
      usbd = null;
      gUsbhostfsError = 1;
      gPSPREady = 0;
    }

    gPSPReady = 1;
    await sendPSPCommand();
}

async function doHostfs(data)
{

      if(data.getInt32(4, true) == HOSTFS_CMD_HELLO)
      {
        gUsbhostfsError = 0;
        console.log("handle hello\n");
        await handleHello();
      }
      else
      {
        gUsbhostError = 1;
      }
}

async function blitARGB0565(fb)
{
  const imageData = ctxWorker.createImageData(480, 272);

  for (let i = 0; i < imageData.data.length; i += 4)
  {
    let src = fb.getUint16(i / 2, true);

    let r = (src & 0x001F) << 19;
    let g = (src & 0x07E0) <<  5;
    let b = (src & 0xF800) >>  8;

    let c = r|g|b|0xFF000000;
    let m = (c & 0x00E000E0) >> 5;
    let n = (c & 0x0000C000) >> 6;
    let o = c|m|n;

    imageData.data[i + 0] = (o >> 16) & 0xFF; // R
    imageData.data[i + 1] = (o >> 8) & 0xFF;  // G
    imageData.data[i + 2] = o & 0xFF;         // B
    imageData.data[i + 3] = 255;              // A
  }
  ctxWorker.putImageData(imageData, 0,0);
}

async function blitARGB1555(fb)
{
  const imageData = ctxWorker.createImageData(480, 272);

  for (let i = 0; i < imageData.data.length; i += 4)
  {
    let src = fb.getUint16(i / 2, true);

    let r = (src & 0x001F) << 19;
    let g = (src & 0x03E0) <<  6;
    let b = (src & 0x7C00) >>  7;

    let c = r|g|b|0xFF000000;
    let m = (c & 0x00E0E0E0) >> 5;
    let o = c|m;

    imageData.data[i + 0] = (o >> 16) & 0xFF; // R
    imageData.data[i + 1] = (o >> 8) & 0xFF;  // G
    imageData.data[i + 2] = o & 0xFF;         // B
    imageData.data[i + 3] = 255;              // A
  }
  ctxWorker.putImageData(imageData, 0,0);
}

async function blitARGB4444(fb)
{
  const imageData = ctxWorker.createImageData(480, 272);

  for (let i = 0; i < imageData.data.length; i += 4)
  {
    let src = fb.getUint16(i / 2, true);

    let r = (src & 0x000F) << 20;
    let g = (src & 0x00F0) <<  8;
    let b = (src & 0x0F00) >>  4;

    let c = r|g|b|0xFF000000;
    let m = (c & 0x00F0F0F0) >> 5;
    let o = c|m;

    imageData.data[i + 0] = (o >> 16) & 0xFF; // R
    imageData.data[i + 1] = (o >> 8) & 0xFF;  // G
    imageData.data[i + 2] = o & 0xFF;         // B
    imageData.data[i + 3] = 255;              // A
  }
  ctxWorker.putImageData(imageData, 0,0);
}

async function blitARGB8888(fb)
{
  const imageData = ctxWorker.createImageData(480, 272);

  for (let i = 0; i < imageData.data.length; i += 4)
  {
    let src = fb.getUint32(i, true);

    let r = (src & 0x0000FF) << 16;
    let g = (src & 0x00FF00) <<  0;
    let b = (src & 0xFF0000) >>  16;

    imageData.data[i + 0] = r; // R
    imageData.data[i + 1] = g;  // G
    imageData.data[i + 2] = b;         // B
    imageData.data[i + 3] = 255;              // A
  }
  ctxWorker.putImageData(imageData, 0,0);
}


async function doBulk(data)
{

  let data_size = data.getInt32(8, true);

  try {
      let read = await usbd.transferIn(1, data_size);

    for(i=0; i < read.data.byteLength; i++)
    {
      dataBuffer[i] = read.data.getUint8(i);
    }

    // buffer contains cmd + buffer itself

    let cmd = new DataView(dataBuffer.buffer, 0, 16);

    let mode = cmd.getUint32(4, true) >> 4 & 0x0F;
    let fbSize = cmd.getUint32(8, true)

    let fb = new DataView(dataBuffer.buffer,16, 16+fbSize);

    switch(mode)
    {
      case 0:
      {
        blitARGB0565(fb);
        break;
      }
      case 1:
      {
        blitARGB1555(fb);
        break;
      }
      case 2:
      {
        blitARGB4444(fb);
        break;
      }
      case 3:
      {
        blitARGB8888(fb);
        break;
      }
    }

    frames++;
    if (Date.now() - tm > 1000)
    {
      postMessage({action: "set-fps", fps: frames});
      tm = Date.now();
      frames = 0;
    }
  } catch(e)
  {
    console.log(e);
    postMessage({action: "psp-disconnect"});
    usbd.close();
    usbd = null;
    gUsbhostfsError = 1;
    gPspReady = 0;
    return;
  }

}

async function doAsync(data)
{
  console.log(data);
}

async function usbThread() {
  if (usbd)
  {
    // do stuff
    try {
      let data = await usbd.transferIn(1, 512);

      switch (data.data.getInt32(0, true))
      {
        case HOSTFS_MAGIC: // HOSTFS
        {
          await doHostfs(data.data);
          break;
        }
        case ASYNC_MAGIC: // ASYNC
        {
          await doAsync(data.data);
          break;
        }
        case BULK_MAGIC: // BULK
        {
          await doBulk(data.data);
          break;
        }
      }
    } catch (e) {
      console.log(e);
      postMessage({action: "psp-disconnect"});
      usbd.close();
      usbd = null;
      gUsbhostfsError = 1;
      gPSPREady = 0;
    }
  }

  if (!gUsbhostfsError)
  {
      requestAnimationFrame(usbThread);
  }
}

async function connectPsp() {

    const filter = { vendorId: 0x054c, productId: 0x01c9 };

    let devices = await navigator.usb.getDevices();
    for (const device of devices) {
        if (device.vendorId === filter.vendorId && device.productId === filter.productId)
        {
          console.log("match");
          usbDevice = device;
          usbDevice.open().then( () => {
            console.log("open");
            usbDevice.reset();

            usbDevice.selectConfiguration(1).then( () => {
                console.log("config");
                usbDevice.claimInterface(0).then( () => {
                    console.log("claim");
                    usbd = usbDevice;
                    // send hello packet
                    let bytes = new Uint8Array(4);//.fromHex("782F0812");
                    bytes[3] = 0x78;
                    bytes[2] = 0x2F;
                    bytes[1] = 0x08;
                    bytes[0] = 0x12;
                    usbd.transferOut(2, bytes).then( () => {
                        gUsbhostfsReady = 1;
                        console.log(self.postMessage({action: "psp-connect"}));
                        requestAnimationFrame(usbThread);
                    }).catch((e) => {
                         gUsbhostfsReady = 0;
                         gPspReady = 0;
                         console.log(e);
                         usbDevice.close();
                         postMessage({action: "psp-disconnect"});
                         usbd = null;
                    });
                }).catch((e) => {
                     console.log(e);
                     postMessage({action: "psp-disconnect"});
                     usbDevice.close();
                     usbd = null;
                     gPspReady = 0;
                });
            }).catch((e) => {
                 console.log(e);
                 postMessage({action: "psp-disconnect"});
                 usbDevice.close();
                 usbd = null;
                 gPspReady = 0;
            });
          }).catch((e) => {
               console.log(e);
               postMessage({action: "psp-disconnect"});
               usbd = null;
               gPspReady = 0;
          });
          break;
        }
    }

}


onmessage = async function(event) {
  if (event.data.action == "connect-psp")
  {
    await connectPsp();
  }
  else if (event.data.action == "set-canvas")
  {
    canvas = event.data.canvas;
    ctxWorker = canvas.getContext("2d");
    ctxWorker.imageSmoothingEnabled = false;
  }
  else if (event.data.action == "psp-input")
  {
    sendInput(event.data.buttons, event.data.axis);
  }
  else if (event.data.action == "unload")
  {
    if (usbd)
    {
      console.log("closing usb");
      usbd.close();
      usbd = null;
      gPspReady = 0;
    }
  }
}


