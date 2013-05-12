/*
Linux PIN_ITEMS.C
*/
//these  data can be shared.
#include <mach/pinmux.h>

//used by pinmux_item.c
//one device may have more than one pinmux relative regs.this is the max number.
#if 0
#define  MAX_PIN_ITEM_NUM     13
enum  device_pinitem_index{
        DEVICE_PIN_ITEM_UART,
        MAX_DEVICE_NUMBER,
 };


#define  uart_pins  { \
			.reg=PINMUX_REG(AO),\
			.clrmask=3<<16,\
			.setmask=3<<11,\
		}
		
		
	
static  pinmux_item_t   __initdata devices_pins[5][MAX_PIN_ITEM_NUM]=
{
    [0]={uart_pins,PINMUX_END_ITEM},
    [1]={{},{}},
    [2]={{},{}},
    //add other devices here. according to the uart item.
};
#endif
