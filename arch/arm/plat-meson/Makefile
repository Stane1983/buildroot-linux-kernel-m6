#
# Makefile for the linux kernel.
#

# Common supporobj-y  += core.o  common.o fiq.o clk_set.o lm.o sleep.o sram.o
obj-y  +=   common.o  lm.o  sram.o debug.o common_platdata.o plat_dev_codec.o\
	plat_dev_fb.o plat_dev_i2c1.o plat_dev_i2c2.o plat_dev_uart.o fiq_bridge.o plat_dev_vout.o plat_dev_lcd.o

obj-n :=
obj-  :=
obj-$(CONFIG_CPU_FREQ) 	+= cpu_freq.o
obj-$(CONFIG_AM_ETHERNET) += plat_dev_eth.o
obj-$(CONFIG_AM_REMOTE) += plat_dev_remote.o
obj-$(CONFIG_MESON_IRQ) +=irq.o fiq.o
obj-$(CONFIG_ARM_GIC) +=gic-irq.o 
obj-$(CONFIG_MESON_ARM_GIC_FIQ) +=gic-fiq.o 
obj-$(CONFIG_AM_FB_EXT) += plat_dev_fb_ext.o
obj-$(CONFIG_AML_HDMI_TX) +=plat_dev_hdmi.o
