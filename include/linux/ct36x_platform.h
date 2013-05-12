#ifndef PLATFORM_H
#define PLATFORM_H

// Platform data
struct ct36x_platform_data {
	//int 				rst;
	//int 				ss;
	u16		x_max;	
	u16		y_max;

	int 	irq;
  void (*shutdown)(int on);
  void (*init_gpio)(void);
};

extern struct i2c_driver ct36x_ts_driver;


#endif
