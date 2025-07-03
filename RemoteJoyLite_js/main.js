const canvas = document.getElementById("worker");
const canvasWorker = canvas.transferControlToOffscreen();

const worker = new Worker("usbthread.js");

worker.postMessage({ action: 'set-canvas', canvas: canvasWorker }, [canvasWorker]);

let oldButtons = 0;
let oldAxisData = 0x7F7F7F7F;

function scale1x()
{
    canvas.style.width="480px";
    canvas.style.height="272px";
}

function scale2x()
{
    canvas.style.width="960px";
    canvas.style.height="544px";
}

function scale3x()
{
    canvas.style.width="1440px";
    canvas.style.height="816px";
}

function fullscreen()
{
    canvas.requestFullscreen();
}

function connectPsp() {

    const filters = [{ vendorId: 0x054c, productId: 0x01c9 }];

    navigator.usb
      .requestDevice({ filters })
      .then((usbDevice) => {
        console.log(`Product name: ${usbDevice.productName}`);

            worker.postMessage({
              action: 'connect-psp',
              vendorId: usbDevice.vendorId,
              productId: usbDevice.productId,
            })
      })
      .catch((e) => {
        console.error(`There is no device. ${e}`);
      });


}

window.addEventListener("gamepadconnected", (e) => {
  console.log(
    "Gamepad connected at index %d: %s. %d buttons, %d axes.",
    e.gamepad.index,
    e.gamepad.id,
    e.gamepad.buttons.length,
    e.gamepad.axes.length,
  );
});

function updateStatus() {
  for (const gamepad of navigator.getGamepads()) {
    if (!gamepad) continue;

    let buttons = 0;
    let mapping = [
        14, //0 X
        13, //1 O
        15, //2 SQR
        12, //3 TRI
        8,  //4 L1
        9,  //5 R1
        -1, //6 L2
        -1, //7 R2
        0,  //8 SEL
        3,   //9 ST
        -1, //10 L3
        -1, //11 R3
        4,  //12 UP
        6,  //13 DOWN
        7,  //14 LEFT
        5,  //15 RIGHT
        16, //16 HOME
    ];

    for (const [i, button] of gamepad.buttons.entries()) {
      const val = button.pressed ? 1 : 0;
      if (mapping[i] >= 0)
      {
          buttons |= val << mapping[i];
      }
    }

    let lx = 0;
    let ly = 0;
    let rx = 0;
    let ry = 0;

    for (const [i, axis] of gamepad.axes.entries()) {
        // -1.0 to 1.0
        switch (i) 
        {
            case 0:
                {
                    lx = Math.round(127 * axis + 127);
                    break;
                }
            case 1:
                {
                    ly = Math.round(127 * axis + 127);
                    break;
                }
            case 2:
                {
                    rx = Math.round(127 * axis + 127);
                    break;
                }
            case 3:
                {
                    ry = Math.round(127 * axis + 127);
                    break;
                }
        }
    }
    let axisData = lx | (ly << 16) | (rx << 8) | (ry << 24);

    // compare with old and send
    if ( (buttons != oldButtons) || (axisData != oldAxisData))
    {
        // send
        worker.postMessage({
            action: 'psp-input',
            buttons: buttons,
            axis: axisData,
        })
    }

    oldButtons = buttons;
    oldAxisData = axisData;

  }

  requestAnimationFrame(updateStatus);
}

worker.onmessage = (event) => {
  if (event.data.action == "set-fps")
  {
    document.getElementById("fps").innerHTML="FPS: " + event.data.fps;
  }
  else if (event.data.action == "psp-disconnect")
  {
    console.log("psp disconnect");
    document.getElementById("disconnected").style.display="";
    document.getElementById("connected").style.display="none";
  }
  else if (event.data.action == "psp-connect")
  {
    console.log("psp connect");
    document.getElementById("connected").style.display="";
    document.getElementById("disconnected").style.display="none";
  }
};

onbeforeunload = (event) => {
  // tell worker to close device
  if (worker)
  {
    worker.postMessage({
      action: 'unload'
    })
  }
}

requestAnimationFrame(updateStatus);
