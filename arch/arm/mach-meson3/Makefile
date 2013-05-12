#
# Makefile for the linux kernel.
#

# Common support
obj-y := clock.o   clk_set.o gpio.o time.o usbclock.o usbsetting.o
obj-m :=
obj-n :=
obj-  :=
#obj-$(CONFIG_ARCH_MESON3)	+= pinmux.o
#obj-$(CONFIG_ARCH_MESON3)	+= gpio.o
obj-$(CONFIG_ARCH_MESON3)	+= power_gate.o
#obj-$(CONFIG_ARCH_MESON3)	+= sd_pad.o
obj-$(CONFIG_MESON_SUSPEND)	+= power_suspend.o
#obj-$(CONFIG_ARCH_MESON3)	+= sram.o sleep.o

obj-$(CONFIG_MACH_M3_REF)	+= $(if $(filter y,$(CONFIG_MESON_PLATFORM_API)),boards/,)board-m3ref.o
obj-$(CONFIG_MACH_M3_REF)	+= $(if $(filter y,$(CONFIG_MESON_PLATFORM_API)),boards/,)board-m3ref-panel.o
obj-$(CONFIG_MACH_M3_REF)	+= $(if $(filter y,$(CONFIG_MESON_PLATFORM_API)),boards/,)board-m3ref-power.o
obj-$(CONFIG_MACH_M3_SKT)	+= $(if $(filter y,$(CONFIG_MESON_PLATFORM_API)),boards/,)board-m3skt.o

plat-api=$(wildcard $(wildcard $(srctree)/$(src)/boards/*.c))
plat-api:=$(plat-api:$(srctree)/$(src)/%.c=%.o) 
obj-$(CONFIG_MESON_PLATFORM_API) += $(filter-out $(obj-y),$(plat-api))

# Power Management
obj-$(CONFIG_SUSPEND)		+= pm.o
obj-$(CONFIG_SUSPEND)		+= sleep.o

ifdef CONFIG_CARDREADER
obj-$(CONFIG_CARDREADER)	+= sd_pad.o
obj-$(CONFIG_CARDREADER)	+= gpio.o
endif
