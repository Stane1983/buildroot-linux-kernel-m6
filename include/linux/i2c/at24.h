#ifndef _LINUX_AT24_H
#define _LINUX_AT24_H

#include <linux/types.h>
#include <linux/memory.h>

/*
 * As seen through Linux I2C, differences between the most common types of I2C
 * memory include:
 * - How much memory is available (usually specified in bit)?
 * - What write page size does it support?
 * - Special flags (16 bit addresses, read_only, world readable...)?
 *
 * If you set up a custom eeprom type, please double-check the parameters.
 * Especially page_size needs extra care, as you risk data loss if your value
 * is bigger than what the chip actually supports!
 */

struct at24_platform_data {
	u32		byte_len;		/* size (sum of all addr) */
	u16		page_size;		/* for writes */
	u8		flags;
#define AT24_FLAG_ADDR16	0x80	/* address pointer is 16 bit */
#define AT24_FLAG_READONLY	0x40	/* sysfs-entry will be read-only */
#define AT24_FLAG_IRUGO		0x20	/* sysfs-entry will be world-readable */
#define AT24_FLAG_TAKE8ADDR	0x10	/* take always 8 addresses (24c00) */
#define AT24_FLAG_SLAVE		0x01	/* if have two eeprom in board, use
                                       this flag to indicate the slave one */

	void		(*setup)(struct memory_accessor *, void *context);
	void		*context;
	void		(*write_protect)(int);
};

/* at24 eeprom address mapping */
#define EEPROM_ADDR_SYS_STATUS      4000
#define EEPROM_ADDR_FAST_RESUME     3828

#define AML_SYS_STATUS_NORMAL       0xAA
#define AML_SYS_STATUS_STANDBY      0x55
#define AML_FAST_RESUME_FLAG        0x8a

extern int aml_eeprom_read(u16 off, char *buf, u16 size);
extern int aml_eeprom_write(u16 off, const char *buf, u16 size);

#endif /* _LINUX_AT24_H */
