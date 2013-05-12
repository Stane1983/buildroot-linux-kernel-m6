#
# Makefile for the linux kernel.
#

# Common support
obj-y := clock.o clk_set.o pinmux.o timer.o gpio.o
obj-m :=
obj-n :=
obj-  :=
obj-$(CONFIG_ARCH_MESON1) += power_gate.o
obj-$(CONFIG_AML_SUSPEND) += power_suspend.o

# Board 
obj-$(CONFIG_MACH_M1_8626M_REFC02)   += board-8626m-refc02.o
obj-$(CONFIG_MACH_M1_8726M_REFC03)   += board-8726m-refc03.o

# Power Management
#obj-$(CONFIG_SUSPEND) += pm.o
#obj-$(CONFIG_SUSPEND) += sleep.o
