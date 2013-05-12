##########################################
########## Amlogic Drivers ###############
##########################################

##########################################
## The order of directories matter.
## Do not change.
##########################################

###For Display, this is for quick logo show##
obj-$(CONFIG_AM_DISPLAY_MODULE) += display/

obj-$(CONFIG_POST_PROCESS_MANAGER) += ppmgr/

obj-$(CONFIG_V4L_AMLOGIC_VIDEO) += video_dev/
obj-$(CONFIG_V4L_AMLOGIC_VIDEO2) += video_dev/

obj-$(CONFIG_DEINTERLACE) += deinterlace/

##### Basic drivers#####

obj-y += secure/

obj-y += uart/

obj-y += usb/

obj-$(CONFIG_AM_NAND) += nand/

obj-y += ethernet/

#obj-$(CONFIG_SATA_DWC_AHCI) += sata/

obj-$(CONFIG_AM_CANVAS) += canvas/

obj-y += amports/

# ve&cm
obj-y += amvecm/

obj-$(CONFIG_I2C) += i2c/

#obj-y += extern_io/

obj-y += input/

obj-$(CONFIG_AMLOGIC_SPI_NOR) += spi_nor/

#obj-$(CONFIG_CMEM) += cmem/

#obj-$(CONFIG_JPEGLOGO) += jpeglogo/

obj-$(CONFIG_CARDREADER) += cardreader/
obj-$(CONFIG_MMC_AML) +=mmc/

ifneq ($(wildcard $(srctree)/$(src)/ump/Kbuild),)
    # use sources existing within kernel source tree at drivers/amlogic/{ump,mali}
    obj-$(CONFIG_UMP) += ump/
    obj-$(CONFIG_MALI400) += mali/
else
    obj-$(CONFIG_UMP) += ../../../hardware/arm/gpu/ump/
    obj-$(CONFIG_MALI400) += ../../../hardware/arm/gpu/mali/
endif

obj-y += vfm/

obj-$(CONFIG_AML_AUDIO_DSP) += audiodsp/

obj-$(CONFIG_AML_RTC)+= rtc/

obj-$(CONFIG_GPS_POWERCTRL) += gps/

obj-y += power/

obj-$(CONFIG_AM_GPIO) +=gpio/

#obj-$(CONFIG_TVIN) += tvin/
obj-y += tvin_camera/

obj-$(CONFIG_EFUSE)+=efuse/


obj-$(CONFIG_BOOT_MONITOR) += boot_monitor/

obj-$(CONFIG_AML_WDT) += watchdog/
#####Sound system is later init in kernel default..######################
obj-$(CONFIG_AMAUDIO) += amaudio/

obj-y += hdmi/

ifeq ($(CONFIG_AM_NEW_TV_ARCH),y)
obj-$(CONFIG_AM_DVB) += dvb_tv/
else
obj-$(CONFIG_AM_DVB) += dvb/
endif

obj-$(CONFIG_AM_SMARTCARD) += smartcard/

ifneq ($(CONFIG_AM_NEW_TV_ARCH),y)
obj-y += demod/
endif

obj-$(CONFIG_AM_WIFI) += wifi/
obj-y += bluetooth/
#obj-y += camera/
obj-$(CONFIG_AMLOGIC_MODEM) += modem/
obj-$(CONFIG_AM_DEBUG) += debug/

obj-$(CONFIG_VIDEO_AMLOGIC_CAPTURE)	+= camera/

obj-$(CONFIG_AM_SENSORS) += sensors/

obj-$(CONFIG_AMLOGIC_MIPI) += mipi/

obj-$(CONFIG_FREE_SCALE) += freescale/

obj-$(CONFIG_D2D3_PROCESS) += d2d3/

obj-$(CONFIG_AML_REG_DEBUG) += regd/
obj-$(CONFIG_AMLOGIC_SPI_HW_MASTER) += spi/

obj-$(CONFIG_DEFERRED_DRIVERINIT) +=  load_deferred_module/


obj-$(CONFIG_AML_MHL) += mhl/

obj-y += crypto/
