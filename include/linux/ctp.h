#ifndef __CTP_H__
#define __CTP_H__


struct ctp_key {
    int value;
    int x1, y1;
    int x2, y2;
};

/*
* irq_flag: IRQF_TRIGGER_RISING/IRQF_TRIGGER_FALLING/IRQF_TRIGGER_HIGH/IRQF_TRIGGER_LOW
* irq: INT_GPIO_0~7
* init_irq: for m6, gpio_irq_set() should be called.
*/
struct ctp_platform_data {
    int irq_flag;
    int irq;
    int (*init_irq)(void);
    int gpio_interrupt;
    int gpio_power;
    int gpio_reset;
    int gpio_enable;
    void *data;
    int data_len;
    struct ctp_key *key_list;
    int key_num;
    
    int xmin, xmax;
    int ymin, ymax;
    int zmin, zmax;
    int wmin, wmax;
    int swap_xy:1;
    int xpol:1;
    int ypol:1;
};

#endif
