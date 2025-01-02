TARGET  = RemoteJoyLite
OBJS    = RemoteJoyLite.o usb.o kmode.o exports.o debug.o hook.o hook_test.o hook_impose.o hook_ctrl.o hook_display.o hook_usb.o hook_interrupt.o
LIBS    = -lpspctrl_driver -lpspusb_driver -lpspusbbus_driver -lpsppower_driver -lpspdisplay_driver
#CFLAGS  = -O2 -G0 -Wall -fno-builtin-printf -DRELEASE
CFLAGS  = -O2 -G0 -Wall -fno-builtin-printf
ASFLAGS = $(CFLAGS)
USE_KERNEL_LIBS = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
