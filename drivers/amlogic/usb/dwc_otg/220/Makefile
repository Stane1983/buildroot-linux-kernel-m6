#
# Makefile for DWC_otg Highspeed USB controller driver
#

KBUILD_CFLAGS += -DDEBUG 

#CPPFLAGS	+= -Dlinux -DDWC_HS_ELECT_TST
DWC_OTG_NAME	= dwc_otg

$(DWC_OTG_NAME)-objs	+= dwc_otg_driver.o dwc_otg_attr.o 
$(DWC_OTG_NAME)-objs	+= dwc_otg_cil.o dwc_otg_cil_intr.o 
$(DWC_OTG_NAME)-objs	+= dwc_otg_pcd.o dwc_otg_pcd_intr.o 
$(DWC_OTG_NAME)-objs	+= dwc_otg_hcd.o dwc_otg_hcd_intr.o 
$(DWC_OTG_NAME)-objs	+= dwc_otg_hcd_queue.o

obj-$(CONFIG_USB_DWC_OTG_HCD)	+= $(DWC_OTG_NAME).o
