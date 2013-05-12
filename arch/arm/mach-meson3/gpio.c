/*
Linux PINMUX.C

*/
#include <linux/module.h>
#include <stdarg.h>
#include <linux/spinlock.h>
#include <mach/am_regs.h>
#include <mach/gpio.h>
#include <mach/gpio_data.h>
#include <plat/regops.h>

#include "gpio_data.c"
#ifndef DEBUG_PINMUX
#define debug(a...)
#else
#define debug(a...) printk(KERN_INFO  a)
#endif

#define set_pin_mux_reg(a,b)   if(b!=NOT_EXIST){  a[(b>>5)&0xf]|=(1<<(b&0x1f))  ;}
static int32_t single_pin_pad(uint32_t  reg_en[P_PIN_MUX_REG_NUM], uint32_t  reg_dis[P_PIN_MUX_REG_NUM],uint32_t pad, uint32_t sig)
{
    
	uint32_t enable,disable;
	int32_t ret=-1;
	foreach_pad_sig_start(pad,sig)
		case_pad_equal(enable,disable);
			set_pin_mux_reg(reg_dis,enable);
			set_pin_mux_reg(reg_en,disable);
		case_end;
		case_sig_equal(enable,disable);
			set_pin_mux_reg(reg_dis,enable);
			set_pin_mux_reg(reg_en,disable);
		case_end;
		case_both_equal(enable,disable);
			set_pin_mux_reg(reg_en,enable);
			set_pin_mux_reg(reg_dis,disable);
			ret=0;
		case_end;
	foreach_pad_sig_end;
	if(ret==-1&&sig!=SIG_GPIOIN&&sig!=SIG_GPIOOUT)
		return -1;
	return 0;
}
#if 0
static uint32_t caculate_pinmux_set_size(uint32_t  reg_en[P_PIN_MUX_REG_NUM], uint32_t  reg_dis[P_PIN_MUX_REG_NUM])
{
	uint32_t ret=0;
	int i;
	for(i=0;i<P_PIN_MUX_REG_NUM;i++)
	{
		if(reg_en[i]||reg_dis[i])
			ret++;
	}
	return ret;
}
#endif
static int32_t caculate_single_pinmux_set(pinmux_item_t pinmux[P_PIN_MUX_REG_NUM+1],uint32_t pad,uint32_t sig)
{
	uint32_t  reg_en[P_PIN_MUX_REG_NUM];
	uint32_t  reg_dis[P_PIN_MUX_REG_NUM];

	int32_t i,j;
	pinmux_item_t end=PINMUX_END_ITEM;

	memset(reg_en,0,sizeof(reg_en));
	memset(reg_dis,0,sizeof(reg_dis));
	if(single_pin_pad(reg_en,reg_dis,pad,sig)<0)
		return -1;
	for(j=0,i=0;i<P_PIN_MUX_REG_NUM;i++)
	{
		if(reg_en[i]==0&&reg_dis[i]==0)
			continue;
		pinmux[j].setmask=reg_en[i];
		pinmux[j].clrmask=reg_dis[i];
		pinmux[j].reg=i;
		j++;
	}
	pinmux[j]=end;
	return 0;
}
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
static uint32_t pimux_locktable[P_PIN_MUX_REG_NUM];
pinmux_set_t* pinmux_cacl_str(char * pad,char * sig ,...)
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return NULL;
}
EXPORT_SYMBOL(pinmux_cacl_str);
pinmux_set_t* pinmux_cacl_int(uint32_t pad,uint32_t sig ,...)
{
#if 0	
	va_list ap;
           int d;
           char c, *s;

           va_start(ap, fmt);
           while (*fmt)
               switch (*fmt++) {
               case 's':              /* string */
                   s = va_arg(ap, char *);
                   printf("string %s\n", s);
                   break;
               case 'd':              /* int */
                   d = va_arg(ap, int);
                   printf("int %d\n", d);
                   break;
               case 'c':              /* char */
                   /* need a cast here since va_arg only
                      takes fully promoted types */
                   c = (char) va_arg(ap, int);
                   printf("char %c\n", c);
                   break;
               }
           va_end(ap);
#endif           

	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	
	/**
	 * @todo NOT implement;
	 */
	 return NULL;
}
EXPORT_SYMBOL(pinmux_cacl_int);
pinmux_set_t* pinmux_cacl(char * str)///formate is "pad=sig pad=sig "
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return NULL;
}
EXPORT_SYMBOL(pinmux_cacl);

char ** pin_get_list(void)
{
	
	 return (char **)&pad_name[0];
}
EXPORT_SYMBOL(pin_get_list);

char ** sig_get_list(void)
{
	 return (char **)&sig_name[0];
}
EXPORT_SYMBOL(sig_get_list);
char * pin_getname(uint32_t pin)
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return NULL;
}
EXPORT_SYMBOL(pin_getname);
char * sig_getname(uint32_t sig)
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return NULL;
}
EXPORT_SYMBOL(sig_getname);
uint32_t pins_num(void)
{
	 return PAD_MAX_PADS;
}
EXPORT_SYMBOL(pins_num);
/**
 * Util Get status function
 */
uint32_t pin_sig(uint32_t pin)
{
	return SIG_MAX_SIGS;
}
EXPORT_SYMBOL(pin_sig);
uint32_t sig_pin(uint32_t sig)
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return 0-1;
}
EXPORT_SYMBOL(sig_pin);
/**
 * pinmux set function
 * @return 0, success , 
 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature . You can not change it
 * 		   NOTAVAILABLE, not available .
 */
int32_t pinmux_set(pinmux_set_t* pinmux )
{
	uint32_t locallock[P_PIN_MUX_REG_NUM];
    uint32_t reg,value,conflict,dest_value;
	ulong flags;
	int i;
    
	if(pinmux==NULL)
		return -4;
    debug( " pinmux addr %p \n",(pinmux->pinmux));
	memset(locallock,0,sizeof(locallock));
	///check lock table
	for(i=0;pinmux->pinmux[i].reg!=0xffffffff;i++)
	{
        reg=pinmux->pinmux[i].reg;
        locallock[reg]=pinmux->pinmux[i].clrmask|pinmux->pinmux[i].setmask;
        dest_value=pinmux->pinmux[i].setmask;
        
        conflict=locallock[reg]&pimux_locktable[reg];
		if(conflict)
        {
            value=readl(p_pin_mux_reg_addr[reg])&conflict;
            dest_value&=conflict;
            if(value!=dest_value)
            {
                printk("set fail , detect locktable conflict");
                return -1;///lock fail some pin is locked by others
            }
        }
	}
	if(pinmux->chip_select!=NULL )
	{
		if(pinmux->chip_select(true)==false){
            debug("error return -3");
			return -3;///@select chip fail;
        }
	}
	
	spin_lock_irqsave(&lock, flags);
	for(i=0;pinmux->pinmux[i].reg!=0xffffffff;i++)
	{
        
        debug( "clrsetbits %08x %08x %08x \n",p_pin_mux_reg_addr[pinmux->pinmux[i].reg],pinmux->pinmux[i].clrmask,pinmux->pinmux[i].setmask);
    	pimux_locktable[pinmux->pinmux[i].reg]|=locallock[pinmux->pinmux[i].reg];
        clrsetbits_le32(p_pin_mux_reg_addr[pinmux->pinmux[i].reg],pinmux->pinmux[i].clrmask,pinmux->pinmux[i].setmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}
EXPORT_SYMBOL(pinmux_set);
int32_t pinmux_clr(pinmux_set_t* pinmux)
{
	ulong flags;
	int i;
    
	if(pinmux==NULL)
		return -4;
    
	if(pinmux->chip_select==NULL)///non share device , we should put the pins in same status always 
		return 0;
	pinmux->chip_select(false);
	debug("pinmux_clr : %p" ,pinmux->pinmux);
    spin_lock_irqsave(&lock, flags);
	for(i=0;pinmux->pinmux[i].reg!=0xffffffff;i++)
	{
		pimux_locktable[pinmux->pinmux[i].reg]&=~(pinmux->pinmux[i].clrmask|pinmux->pinmux[i].setmask);
	}

	
	for(i=0;pinmux->pinmux[i].reg!=0xffffffff;i++)
	{
        debug("clrsetbits %x %x %x",p_pin_mux_reg_addr[pinmux->pinmux[i].reg],pinmux->pinmux[i].setmask|pinmux->pinmux[i].clrmask,pinmux->pinmux[i].clrmask);
		clrsetbits_le32(p_pin_mux_reg_addr[pinmux->pinmux[i].reg],pinmux->pinmux[i].setmask|pinmux->pinmux[i].clrmask,pinmux->pinmux[i].clrmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}
EXPORT_SYMBOL(pinmux_clr);
int32_t pinmux_set_locktable(pinmux_set_t* pinmux )
{	
	ulong flags;
	int i;
	if(pinmux==NULL)
		return -4;
	spin_lock_irqsave(&lock, flags);
	for(i=0;pinmux->pinmux[i].reg!=0xffffffff;i++)
	{
		pimux_locktable[pinmux->pinmux[i].reg]|=pinmux->pinmux[i].clrmask|pinmux->pinmux[i].setmask;
		clrsetbits_le32(p_pin_mux_reg_addr[pinmux->pinmux[i].reg],pinmux->pinmux[i].clrmask,pinmux->pinmux[i].setmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}
EXPORT_SYMBOL(pinmux_set_locktable);

/**
 * @return 0, success , 
 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature . You can not change it
 * 		   NOTAVAILABLE, not available .
 */
static int32_t pad_to_gpio(uint32_t pad)
{
	pinmux_item_t pinmux[P_PIN_MUX_REG_NUM];
	pinmux_set_t dummy;
	memset(&dummy,0,sizeof(dummy));
	if(caculate_single_pinmux_set(pinmux,pad,SIG_GPIOIN)<0)
		return -1;
	dummy.pinmux=&pinmux[0];
	return pinmux_set(&dummy);
}
int32_t gpio_set_status(uint32_t pin,bool gpio_in)
{
	unsigned bit,reg;
	if(pad_to_gpio(pin)<0)
		return -1;
	reg=(pad_gpio_bit[pin]>>5)&0xf;
	bit=(pad_gpio_bit[pin])&0x1f;
	clrsetbits_le32(p_gpio_oen_addr[reg],1<<bit,gpio_in<<bit);
	return 0;
}
EXPORT_SYMBOL(gpio_set_status);

bool gpio_get_status(uint32_t pin)
{
	unsigned bit,reg;

	bool bret;
	reg=(pad_gpio_bit[pin]>>5)&0xf;
	bit=(pad_gpio_bit[pin])&0x1f;

	return ((aml_get_reg32_bits(p_gpio_oen_addr[reg],bit, 1))?(gpio_status_in):(gpio_status_out));
}
EXPORT_SYMBOL(gpio_get_status);

int32_t gpio_get_val(uint32_t pin)
{
	unsigned bit,reg;

	reg=(pad_gpio_bit[pin]>>5)&0xf;
	bit=(pad_gpio_bit[pin])&0x1f;

	return aml_get_reg32_bits(p_gpio_in_addr[reg],bit, 1);
}
EXPORT_SYMBOL(gpio_get_val);


/**
 * GPIO out function
 */
int32_t gpio_out(uint32_t pin,bool high)
{
	unsigned bit,reg;
	if(gpio_set_status(pin,false)==0)
	{
		reg=(pad_gpio_bit[pin]>>5)&0xf;
		bit=(pad_gpio_bit[pin])&0x1f;
		if((p_gpio_out_addr[reg]&3)==2)
		{
			reg=p_gpio_out_addr[reg]&(~3);
			bit+=16;
		}else{
		   reg=p_gpio_out_addr[reg];
		}
		clrsetbits_le32(reg,1<<bit,high<<bit);
		return 0;
	};
	return -1;
}
EXPORT_SYMBOL(gpio_out);
/**
 * GPIO in function .ls
 */
int32_t gpio_in_get(uint32_t pin)
{
	unsigned bit,reg;
	if(gpio_set_status(pin,true)<0)
	{
		printk(" %s , Set gpio to input fail\n",__func__);
		BUG();
	}

	reg=(pad_gpio_bit[pin]>>5)&0xf;
	bit=(pad_gpio_bit[pin])&0x1f;
	
	
	return (readl(p_gpio_in_addr[reg])>>bit)&1;
	
}
EXPORT_SYMBOL(gpio_in_get);
/**
 * Multi pin operation
 * @return 0, success , 
 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature . You can not change it
 * 		   NOTAVAILABLE, not available .
 * 
 */

gpio_set_t * gpio_out_group_cacl(uint32_t pin,uint32_t bits, ... )
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return NULL;
}
EXPORT_SYMBOL(gpio_out_group_cacl);
int32_t gpio_out_group_set(gpio_set_t * set,uint32_t high_low )
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return -1;
}
EXPORT_SYMBOL(gpio_out_group_set);

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
gpio_set_t * gpio_in_group_cacl(uint32_t pin,uint32_t bits, ... )
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return NULL;
}
EXPORT_SYMBOL(gpio_in_group_cacl);

gpio_in_t gpio_in_group(gpio_in_set_t *grp)
{
	printk(" %s , NOT IMPLENMENT\n",__func__);
    BUG();

	/**
	 * @todo NOT implement;
	 */
	 return 0;
}
EXPORT_SYMBOL(gpio_in_group);

//~ typedef struct gpio_irq_s{
    //~ int8_t    filter;
    //~ uint8_t    irq;///
    //~ uint16_t   pad;
//~ }gpio_irq_t;
static gpio_irq_t gpio_irqs[8]={
   
};

int32_t gpio_irq_set_lock(int32_t pad, uint32_t irq/*GPIO_IRQ(irq,type)*/,int32_t filter,bool lock)
{
    if(pad>=PAD_TEST_N)
        return -1;
    gpio_irqs[(irq>>2)].irq=irq&3;
    gpio_irqs[(irq>>2)].pad=pad;
    gpio_irqs[(irq>>2)].filter=filter;
    return 0;
}
EXPORT_SYMBOL(gpio_irq_set_lock);
void gpio_irq_enable(uint32_t irq)
{
    int idx=(irq>>2)&7;
    unsigned reg,start_bit;
    unsigned type[]={0x0, ///high
                    0x1,  ///rising
                    0x10000, ///low
                    0x10001 ///faling
                    };
    debug("write reg %p clr=%x set=%x",P_GPIO_INTR_EDGE_POL,0x10001<<idx,type[gpio_irqs[idx].irq]<<idx);
    /// set trigger type
    clrsetbits_le32(P_GPIO_INTR_EDGE_POL,0x10001<<idx,type[gpio_irqs[idx].irq]<<idx);
    
    ///select pad
    reg=idx<4?P_GPIO_INTR_GPIO_SEL0:P_GPIO_INTR_GPIO_SEL1;
    start_bit=(idx&3)*8;
    clrsetbits_le32(reg,0xff<<start_bit,gpio_irqs[idx].pad<<start_bit);
    debug("write reg %p clr=%x set=%x",reg,0xff<<start_bit,gpio_irqs[idx].pad<<start_bit);
    ///set filter
    start_bit=(idx)*4;
    clrsetbits_le32(P_GPIO_INTR_FILTER_SEL0,0x7<<start_bit,gpio_irqs[idx].filter<<start_bit);
    debug("write reg %p clr=%x set=%x",P_GPIO_INTR_FILTER_SEL0,0x7<<start_bit,gpio_irqs[idx].filter<<start_bit);
    
}
EXPORT_SYMBOL(gpio_irq_enable);


