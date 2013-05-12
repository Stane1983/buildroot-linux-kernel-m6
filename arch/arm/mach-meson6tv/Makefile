#
# Makefile for the linux kernel.
#

# Common support
obj-y := cpu.o clock.o gpio.o time.o usbclock.o usbsetting.o power_gate.o mod_gate.o
obj-m :=
obj-n :=
obj-  :=

# Power Management
obj-$(CONFIG_SUSPEND)		+= pm.o

ifeq ($(CONFIG_MESON_PLATFORM_API),y)
plat-api=$(wildcard $(wildcard $(srctree)/$(src)/boards/*.c))
plat-api:=$(plat-api:$(srctree)/$(src)/%.c=%.o)
obj-y +=$(plat-api)
else
obj-$(CONFIG_BOARD_MESON6TV_REF)	+= board-m6tv-ref.o
obj-$(CONFIG_BOARD_MESON6TV_REF)	+= board-m6tv-ref-panel.o
obj-$(CONFIG_BOARD_MESON6TV_REF)	+= board-m6tv-ref-tvin.o
obj-$(CONFIG_BOARD_MESON6TV_SKT)	+= board-m6tv-skt.o
obj-$(CONFIG_BOARD_MESON6TV_SKT)	+= board-m6tv-skt-panel.o
obj-$(CONFIG_BOARD_MESON6TV_SKT)	+= board-m6tv-skt-tvin.o
endif
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o
obj-$(CONFIG_SMP) += headsmp.o platsmp.o


obj-$(CONFIG_CARDREADER)	+= sd_pad.o
#obj-$(CONFIG_CARDREADER)	+= gpio.o

obj-$(CONFIG_MESON_SUSPEND) += power_suspend.o
obj-$(CONFIG_SUSPEND) += sleep.o

obj-$(CONFIG_MESON_CS_DCDC_REGULATOR) += voltage.o
