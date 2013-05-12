
# Comment/uncomment the following line to enable/disable debugging
#DEBUG = y
INSTALLDIR = /home/taofeng/target/home/taofeng/11n
KVERSION = 2.6.34
ARCH		?= arm
CROSS_COMPILE	?=  arm-none-linux-gnueabi-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DHALM1_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

ifeq ($(USE_DMA),y)
EXTRA_CFLAGS += -DUSE_WIFI_DMA
endif
EXTRA_CFLAGS += -DFPGA
EXTRA_CFLAGS += -DSDIOSRAM
EXTRA_CFLAGS += $(DEBFLAGS) -I$(LDDINC)

TARGET = sdio

ifneq ($(KERNELRELEASE),)

sdio-objs :=  sdio_cmd.o  sdio_init.o  sdio_io.o  sd_misc.o \

obj-m	:= sdio.o

else

KERNELDIR ?= /mnt/hgfs/work/meson/v2.6.34
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD) modules
	
endif

install:
	sudo cp sdio.ko $(INSTALLDIR)
	#install -d $(INSTALLDIR)
	#install -c $(TARGET).o $(INSTALLDIR)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions


depend .depend dep:
	$(CC) $(EXTRA_CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
