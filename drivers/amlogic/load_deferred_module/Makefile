#
# load_deferred
#
LOAD_DEFERRED_NAME = deferreddrv

$(LOAD_DEFERRED_NAME)-objs := load_deferred_module.o
#obj-m += $(LOAD_DEFERRED_NAME).o
obj-$(CONFIG_DEFERRED_DRIVERINIT) += $(LOAD_DEFERRED_NAME).o
