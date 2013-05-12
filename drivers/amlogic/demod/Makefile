
obj-$(CONFIG_AM_DEMOD) += aml_fe.o

aml_fe-objs := demod_func.o isdbt_func.o dvbc_func.o i2c_func.o tuner_func.o

aml_fe-objs += mxl/MxL5007.o
aml_fe-objs += mxl/MxL5007_API.o
aml_fe-objs += mxl/MxL_User_Define.o
aml_fe-objs += nxp/tuner_fj2207.o
aml_fe-objs += nxp/tmbslNT220x.o
aml_fe-objs += nxp/tmbslNT220xInstance.o
aml_fe-objs += nxp/tmddNT220x.o
aml_fe-objs += nxp/tmddNT220xInstance.o
aml_fe-objs += nxp/tmddNT220x_Advanced.o
aml_fe-objs += rda/RDA5880e.o
aml_fe-objs += rda/RDA5880_adp.o
aml_fe-objs += tda/tmbslTDA18273.o
aml_fe-objs += tda/tmbslTDA18273_Instance.o
aml_fe-objs += tda/tmbslTDA18273_RegDef.o
aml_fe-objs += tda/tmsysOM3912.o
aml_fe-objs += tda/tmsysOM3912Instance.o
aml_fe-objs += tda/tmsysOM3912Tuner.o
aml_fe-objs += tda/tmtuner.o
aml_fe-objs += si2176/si2176_func.o


ifeq ($(CONFIG_AM_DEMOD_DVBAPI), y)
  aml_fe-objs += amlfrontend.o
endif

ifeq ($(CONFIG_AM_DEMOD_DEBUG), y)
  aml_fe-objs	+= aml_demod.o
endif

ifeq ($(CONFIG_AM_AMDEMOD_DTMB), y)
  aml_fe-objs	+= dtmb_func.o
endif

ifeq ($(CONFIG_AM_AMDEMOD_ATSC), y)
  aml_fe-objs	+= atsc_func.o
endif

ifeq ($(CONFIG_AM_AMDEMOD_FPGA_VER), y)
#  aml_fe-objs += sdio/sdio_init.o
#  aml_fe-objs += sdio/sdio_cmd.o
#  aml_fe-objs += sdio/sdio_io.o
#  aml_fe-objs += sdio/sd_misc.o
endif

EXTRA_CFLAGS += -Idrivers/media/dvb/dvb-core

