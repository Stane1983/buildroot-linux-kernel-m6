#
# Makefile for TVIN.
#

obj-$(CONFIG_TVIN) += tvin_common.o
tvin_common-objs := tvin_format_table.o tvin_frontend.o tvin_global.o

obj-$(CONFIG_TVIN_VDIN)	  += vdin/
obj-$(CONFIG_TVIN_AFE)	  += tvafe/
obj-$(CONFIG_TVIN_HDMI)	  += hdmirx/
obJ-$(CONFIG_TVIN_BT656)  += bt656/
obj-$(CONFIG_VIUIN)	  += viu/
obj-$(CONFIG_TVIN_IT660X) += it660x/
obj-y                     += vdin_v4l2/
