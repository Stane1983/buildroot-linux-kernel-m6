#
# Makefile for TVIN.
#
obj-$(CONFIG_TVIN_IT660X) += tvin_it660x.o
tvin_it660x-objs := it660x/it660x_vdin.o it660x/it660x_drv.o it660x/it660x_i2c.o it660x/hdmirx.o it660x/csc.o it660x/edid.o

obj-$(CONFIG_TVIN_VIUIN) += tvin_viuin.o
tvin_viuin-objs := viu_in.o

obj-$(CONFIG_TVIN_BT656IN) += tvin_bt656in.o
tvin_bt656in-objs := bt656_601_in.o

obj-$(CONFIG_TVIN_AFE) += tvin_afe.o
tvin_afe-objs := tvafe_adc.o tvafe_cvd.o tvafe_general.o tvafe.o

obj-$(CONFIG_TVIN_DVIN) += tvin_dvin.o
tvin_dvin-objs := dvin.o

obj-$(CONFIG_TVIN_VDIN) += tvin_vdin.o
tvin_vdin-objs := tvin_format_table.o vdin_vf.o vdin.o tvin_notifier.o vdin_ctl.o

obj-$(CONFIG_GAMMA_PROC) += gamma_proc.o

obj-y += tuners/


