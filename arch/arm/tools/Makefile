#
# linux/arch/arm/tools/Makefile
#
# Copyright (C) 2001 Russell King
#

include/generated/mach-types.h: $(src)/gen-mach-types $(src)/mach-types $(wildcard $(srctree)/$(CUSTOMER_DIR_NAME)/boards/mach-types)
	@echo '  Generating $@'
	@mkdir -p $(dir $@)
	$(Q)$(AWK) -f $^ $(wildcard $(srctree)/$(CUSTOMER_DIR_NAME)/boards/mach-types)> $@ || { rm -f $@; /bin/false; }
