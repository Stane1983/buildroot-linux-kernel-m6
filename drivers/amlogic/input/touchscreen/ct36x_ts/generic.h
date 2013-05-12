
#ifndef GENERIC_H
#define GENERIC_H

#include "linux/ct36x_platform.h"
//#1include <mach/gpio_data.h>
//#include <mach/gpio.h>
//#include <mach/irqs.h>

//#define CT36X_TS_I2C_SPEED			400000
//#define GPIO_FT_RST  PAD_GPIOC_3
//#define GPIO_FT_IRQ  PAD_GPIOA_16
//#define FT_IRQ     INT_GPIO_0

void ct36x_ts_reg_read(struct i2c_client *client, unsigned short addr, char *buf, int len);
void ct36x_ts_reg_write(struct i2c_client *client, unsigned short addr, char *buf, int len);

void ct36x_platform_get_cfg(struct ct36x_ts_info *ct36x_ts);
int ct36x_platform_set_dev(struct ct36x_ts_info *ct36x_ts);

int ct36x_platform_get_resource(struct ct36x_ts_info *ct36x_ts);
void ct36x_platform_put_resource(struct ct36x_ts_info *ct36x_ts);

void ct36x_platform_hw_reset(struct ct36x_platform_data *pdata);

#endif

