#ifndef __MACH_MESON3_GPIO_H_
#define __MACH_MESON3_GPIO_H_
#include <linux/types.h>
#include <mach/pinmux.h>



/**
 * =================================================================================================
 */
/**
 * GPIO operation function
 */
typedef pinmux_item_t gpio_item_t;
typedef gpio_item_t gpio_set_t;
/**
 * @return 0, success , 
 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature . You can not change it
 * 		   NOTAVAILABLE, not available .
 */
#define gpio_status_in    true
#define gpio_status_out   false
int32_t gpio_set_status(uint32_t pin,bool gpio_in);
bool gpio_get_status(uint32_t pin);
int32_t gpio_get_val(uint32_t pin);

/**
 * GPIO out function
 */
int32_t gpio_out(uint32_t pin,bool high);
static inline int32_t gpio_out_high(uint32_t pin)
{
	return gpio_out(pin ,true);
}
static inline int32_t gpio_out_low(uint32_t pin)
{
	return gpio_out(pin ,false);
}

	/**
	 * Multi pin operation
	 * @return 0, success , 
	 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature . You can not change it
	 * 		   NOTAVAILABLE, not available .
	 * 
	 */

gpio_set_t * gpio_out_group_cacl(uint32_t pin,uint32_t bits, ... );
int32_t gpio_out_group_set(gpio_set_t*,uint32_t high_low );
/**
 * GPIO in function .ls
 */
int32_t gpio_in_get(uint32_t pin); ///one bit operation
	/**
	 * Multi pin operation
	 */
	/**
	 * Multi pin operation
	 * @return 0, success , 
	 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature . You can not change it
	 * 		   NOTAVAILABLE, not available .
	 * 
	 */
gpio_set_t * gpio_in_group_cacl(uint32_t pin,uint32_t bits, ... );
typedef int64_t gpio_in_t;
typedef int64_t gpio_in_set_t;
gpio_in_t gpio_in_group(gpio_in_set_t *);

/**
 * GPIO interrupt interface
 */
#define  GPIO_IRQ(irq,type)  ((((irq)&7)<<2)|((type)&3))
#define  GPIO_IRQ_RISING    1
#define  GPIO_IRQ_LOW       2
#define  GPIO_IRQ_FALLING   3
#define  GPIO_IRQ_HIGH      0

#define  GPIO_IRQ_DEFAULT_FILTER -1
typedef struct gpio_irq_s{
    int8_t    filter;
    uint8_t    irq;///
    uint16_t   pad;
}gpio_irq_t;
int32_t gpio_irq_set_lock(int32_t pad, uint32_t irq/*GPIO_IRQ(irq,type)*/,int32_t filter,bool lock);
void gpio_irq_enable(uint32_t irq);
static inline int32_t gpio_irq_set(int32_t pad, uint32_t irq/*GPIO_IRQ(irq,type)*/)
{
    int32_t ret;
    ret=gpio_irq_set_lock(pad,irq,-1,false);
    if(ret==0)
        gpio_irq_enable(irq);
    return ret;
}



#endif
