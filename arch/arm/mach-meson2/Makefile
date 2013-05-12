#
# Makefile for the linux kernel.
#

# Common support
obj-y := clock.o clk_set.o time.o gpio.o pinmux.o irq.o
obj-m :=
obj-n :=
obj-  :=

obj-$(CONFIG_ARCH_MESON2)       += power_gate.o
obj-$(CONFIG_MESON_SUSPEND)     += power_suspend.o

obj-$(CONFIG_MACH_M2_REF)       += board-m2-ref.o
obj-$(CONFIG_MACH_M2_SKT)       += board-m2-skt.o

# Power Management
obj-$(CONFIG_SUSPEND)           += pm.o
obj-$(CONFIG_SUSPEND)           += sleep.o

