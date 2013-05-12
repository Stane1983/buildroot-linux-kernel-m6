
ifneq ($(KBUILD_SRC),)
TOP_KBUILD_SRC := $(KBUILD_SRC)/
endif

MAKE_FOR_LIB := n
ifeq ($(MAKE_FOR_LIB), n)
$(obj)/aml1212_algorithm_lib:
	@cp $(TOP_KBUILD_SRC)drivers/amlogic/power/aml_pmu/aml1212_algorithm_lib.z \
        drivers/amlogic/power/aml_pmu/aml1212_algorithm_lib.o 
endif

ifeq ($(MAKE_FOR_LIB), y)
    aml1212_algorithm_lib-y := aml1212_algorithm.o
else 
# add FORCE to make compile force copy obj file, so no need to delete obj file
# when .z has updated
    $(obj)/aml1212_algorithm_lib.o: $(obj)/aml1212_algorithm_lib FORCE
endif

obj-y += aml_pmu.o
obj-$(CONFIG_AML1212) += aml1212_algorithm_lib.o
obj-$(CONFIG_AML1212) += aml1212-sply.o

