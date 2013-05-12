
#ifndef AML_MACH_UART
#define AML_MACH_UART

#include <plat/platform_data.h>

/*TO DO PINMUX for uart*/
/*add include pinmux head file*/

#if defined (CONFIG_ARCH_MESON3)
#define UART_AO    0
#define UART_A     1
#define UART_B     2
#define UART_C     3
#define MAX_PORT_NUM 4
#elif defined(CONFIG_ARCH_MESON2) || defined(CONFIG_ARCH_MESON1)
#define UART_A     0
#define UART_B     1
#define MAX_PORT_NUM 2
#else
#include <mach/io.h>
#endif

struct aml_uart_platform{
		plat_data_public_t  public;
		int uart_line[MAX_PORT_NUM];
		void * pinmux_uart[MAX_PORT_NUM];
};

#endif

