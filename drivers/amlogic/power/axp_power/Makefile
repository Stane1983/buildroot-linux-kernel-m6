obj-y += axp-mfd.o

ifneq ($(KBUILD_SRC),)
TOP_KBUILD_SRC := $(KBUILD_SRC)/
endif

MAKE_FOR_LIB := n
ifeq ($(MAKE_FOR_LIB), n)
$(obj)/axp20_algorithm_lib:
	@cp $(TOP_KBUILD_SRC)drivers/amlogic/power/axp_power/axp20_algorithm_lib.z \
        drivers/amlogic/power/axp_power/axp20_algorithm_lib.o 
endif

#Make this built-in, so that it will be loaded before I2C driver
obj-$(CONFIG_AW_AXP18) += axp18-regu.o
obj-$(CONFIG_AW_AXP18) += axp18-sply.o
obj-$(CONFIG_AW_AXP18) += virtual18.o
obj-$(CONFIG_AW_AXP18) += virtual18_dev.o

obj-$(CONFIG_AW_AXP19) += axp19-regu.o
obj-$(CONFIG_AW_AXP19) += axp19-sply.o
obj-$(CONFIG_AW_AXP19) += axp19-gpio.o
obj-$(CONFIG_AW_AXP19) += virtual19.o
obj-$(CONFIG_AW_AXP19) += virtual19_dev.o

obj-$(CONFIG_AW_AXP20) += axp20-regu.o
obj-$(CONFIG_AW_AXP20) += axp20-sply.o
obj-$(CONFIG_AW_AXP20) += axp20-gpio.o
obj-$(CONFIG_AW_AXP20) += virtual20.o
obj-$(CONFIG_AW_AXP20) += virtual20_dev.o

ifeq ($(MAKE_FOR_LIB), y)
    axp20_algorithm_lib-y := axp20_algorithm.o
else 
    # add FORCE to make compile force copy obj file, so no need to delete obj file
    # when .z has updated
    $(obj)/axp20_algorithm_lib.o: $(obj)/axp20_algorithm_lib FORCE
endif

obj-$(CONFIG_AW_AXP20) += axp20_algorithm_lib.o 

