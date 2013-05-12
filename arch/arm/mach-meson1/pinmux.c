/*
Linux PINMUX.C

*/
#include <linux/module.h>
#include <linux/spinlock.h>
#include <mach/am_regs.h>
#include <mach/pinmux.h>
/**
 * UTIL interface  
 * these function can be implement in a tools
 */
 /**
  * @return NULL is fail
  * 		errno NOTAVAILABLE , 
  * 			  SOMEPIN IS LOCKED
  */
static DEFINE_SPINLOCK(lock);
#define PINMUX_REG_NUM sizeof(pimux_reg_addr)/sizeof(pimux_reg_addr[0])
static const uint32_t pimux_reg_addr[] = {
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_0),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_1),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_2),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_3),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_4),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_5),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_6),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_7),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_8),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_9),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_10),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_11),
	CBUS_REG_ADDR(PERIPHS_PIN_MUX_12),
	//AOBUS_REG_ADDR(AO_RTI_PIN_MUX_REG)
};

static uint32_t pimux_locktable[PINMUX_REG_NUM];
pinmux_set_t *pinmux_cacl_str(char *pad, char *sig, ...)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return NULL;
}

EXPORT_SYMBOL(pinmux_cacl_str);
pinmux_set_t *pinmux_cacl_int(uint32_t pad, uint32_t sig, ...)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return NULL;
}

EXPORT_SYMBOL(pinmux_cacl_int);
pinmux_set_t *pinmux_cacl(char *str)	///formate is "pad=sig pad=sig "
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return NULL;
}

EXPORT_SYMBOL(pinmux_cacl);

char **pin_get_list(void)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return NULL;
}

EXPORT_SYMBOL(pin_get_list);

char **sig_get_list(void)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return NULL;
}

EXPORT_SYMBOL(sig_get_list);
char *pin_getname(uint32_t pin)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return NULL;
}

EXPORT_SYMBOL(pin_getname);
char *sig_getname(uint32_t sig)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return NULL;
}

EXPORT_SYMBOL(sig_getname);
uint32_t pins_num(void)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return 0 - 1;
}

EXPORT_SYMBOL(pins_num);
/**
 * Util Get status function
 */
uint32_t pin_sig(uint32_t pin)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return 0 - 1;
}

EXPORT_SYMBOL(pin_sig);
uint32_t sig_pin(uint32_t sig)
{
	printk(" %s , NOT IMPLENMENT\n", __func__);
	BUG();

	/**
	 * @todo NOT implement;
	 */
	return 0 - 1;
}

EXPORT_SYMBOL(sig_pin);
/**
 * pinmux set function
 * @return 0, success , 
 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature . You can not change it
 * 		   NOTAVAILABLE, not available .
 */
int32_t pinmux_set(pinmux_set_t * pinmux)
{
	uint32_t locallock[PINMUX_REG_NUM];
	ulong flags;
	int i;
	if (pinmux == NULL)
		return -4;
	memset(locallock, 0, sizeof(locallock));
	///check lock table
	for (i = 0; pinmux->pinmux[i].reg != 0xffffffff; i++) {
		locallock[pinmux->pinmux[i].reg] =
		    pinmux->pinmux[i].clrmask | pinmux->pinmux[i].setmask;
		if (locallock[pinmux->pinmux[i].reg] &
		    pimux_locktable[pinmux->pinmux[i].reg])
			return -1;	///lock fail some pin is locked by others
	}
	if (pinmux->chip_select != NULL) {
		if (pinmux->chip_select(true) == false)
			return -3;	///@select chip fail;
	}

	spin_lock_irqsave(&lock, flags);
	for (i = 0; pinmux->pinmux[i].reg != 0xffffffff; i++) {
		pimux_locktable[pinmux->pinmux[i].reg] |=
		    locallock[pinmux->pinmux[i].reg];
		clrsetbits_le32(pimux_reg_addr[pinmux->pinmux[i].reg],
				pinmux->pinmux[i].clrmask,
				pinmux->pinmux[i].setmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

EXPORT_SYMBOL(pinmux_set);
int32_t pinmux_clr(pinmux_set_t * pinmux)
{
	ulong flags;
	int i;
	if (pinmux == NULL)
		return -4;
	if (pinmux->chip_select == NULL)	///non share device , we should put the pins in same status always 
		return 0;
	pinmux->chip_select(false);
	spin_lock_irqsave(&lock, flags);
	for (i = 0; pinmux->pinmux[i].reg != 0xffffffff; i++) {
		pimux_locktable[pinmux->pinmux[i].reg] &=
		    ~(pinmux->pinmux[i].clrmask | pinmux->pinmux[i].setmask);
	}

	for (i = 0; pinmux->pinmux[i].reg != 0xffffffff; i++) {
		clrbits_le32(pimux_reg_addr[pinmux->pinmux[i].reg],
			     pinmux->pinmux[i].setmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

EXPORT_SYMBOL(pinmux_clr);
int32_t pinmux_set_locktable(pinmux_set_t * pinmux)
{
	ulong flags;
	int i;
	if (pinmux == NULL)
		return -4;
	spin_lock_irqsave(&lock, flags);
	for (i = 0; pinmux->pinmux[i].reg != 0xffffffff; i++) {
		pimux_locktable[pinmux->pinmux[i].reg] |=
		    pinmux->pinmux[i].clrmask | pinmux->pinmux[i].setmask;
		clrsetbits_le32(pimux_reg_addr[pinmux->pinmux[i].reg],
				pinmux->pinmux[i].clrmask,
				pinmux->pinmux[i].setmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}

EXPORT_SYMBOL(pinmux_set_locktable);
