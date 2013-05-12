/*
 *  linux/drivers/tty/serial/am_uart.c
 *
 *  Driver for almogic-type serial ports
 *
 *  Based on serial.c,
 * This program is free software; you can redistribute it and/or modify
 * it under the81 terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifdef CONFIG_MAGIC_SYSRQ
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/nmi.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <mach/hardware.h>
#include <mach/pinmux.h>
#include <linux/clk.h>
#include <linux/uart-aml.h>

#include <asm/io.h>
#include <asm/irq.h>

#include "am_uart.h"


#include <asm/serial.h>

#if defined(CONFIG_ARCH_MESON3)
#define UART_NR     4
static unsigned int uart_irqs[UART_NR] = {INT_UART_AO, INT_UART_0, INT_UART_1, INT_UART_2};
static am_uart_t *uart_addr[UART_NR] = {UART_BASEADDRAO, UART_BASEADDR0, UART_BASEADDR1, UART_BASEADDR2};
static unsigned int uart_FIFO_max_cnt[UART_NR] = {64,128,64,64};
#elif defined(CONFIG_ARCH_MESON2) || defined(CONFIG_ARCH_MESON1)
#define UART_NR     2
static unsigned int uart_irqs[UART_NR] = { INT_UART,INT_UART_1 };
static am_uart_t *uart_addr[UART_NR] = { UART_BASEADDR0,UART_BASEADDR1 };
static unsigned int uart_FIFO_max_cnt[UART_NR] = {64,64};
#else
static unsigned int uart_irqs[] = {UART_INTS};
#define UART_NR     (sizeof(uart_irqs)/sizeof(uart_irqs[0]))
static am_uart_t *uart_addr[] = {UART_ADDRS};
static unsigned int uart_FIFO_max_cnt[] = {UART_FIFO_CNTS};

#endif

#if defined(CONFIG_ARCH_MESON2) || defined(CONFIG_ARCH_MESON1)
static int default_index = 1;
#else
static int default_index = 0;
#endif

static int inited_ports_flag = 0;
#ifndef outl
#define outl(v,addr)	writel(v,(unsigned long)addr)
#endif
#ifndef inl
#define inl(addr)	readl((unsigned long)addr)
#endif
#define in_l(addr)	inl((unsigned long )addr)
#define out_l(v,addr)	outl(v,(unsigned long )addr)
#define clear_mask(addr,mask)		out_l(in_l(addr) &(~(mask)),addr)
#define set_mask(addr,mask)		out_l(in_l(addr) |(mask),addr)

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

//#define PRINT_DEBUG
#ifdef PRINT_DEBUG
unsigned int DEBUG_PORT_ID=1;
#endif
struct am_uart_port {
	struct uart_port	port;

	/* We need to know the current clock divisor
	 * to read the bps rate the chip has currently
	 * loaded.
	 */
	char 		name[16];
	int 			baud;
	int			magic;
	int			baud_base;
	int			irq;
	int			flags; 		/* defined in tty.h */
	int			type; 		/* UART type */
	struct tty_struct 	*tty;
	int			read_status_mask;
	int			ignore_status_mask;
	int			timeout;
	int			custom_divisor;
	int			x_char;	/* xon/xoff character */
	int			close_delay;
	unsigned short		closing_wait;
	unsigned short		closing_wait2;
	unsigned long		event;
	unsigned long		last_active;
	int			line;
	int			count;	    /* # of fd on device */
	int			blocked_open; /* # of blocked opens */
	long			session; /* Session of opening process */
	long			pgrp; /* pgrp of opening process */

       int                  rx_cnt;
       int                  rx_error;

	struct mutex	info_mutex;

	struct work_struct	tqueue;
	struct work_struct	tqueue_hangup;

/* Sameer: Obsolete structs in linux-2.6 */
/* 	struct termios		normal_termios; */
/* 	struct termios		callout_termios; */
	wait_queue_head_t	open_wait;
	wait_queue_head_t	close_wait;

       spinlock_t rd_lock;
       spinlock_t wr_lock;
};

static struct am_uart_port am_ports[UART_NR];
/*
 * Output a single character, using UART polled mode.
 * This is used for console output.
 */
void am_uart_put_char(int index,char ch)
{
	am_uart_t *uart = uart_addr[index];

	while ((readl(&uart->status) & UART_TXFULL)) ;
	writel(ch, &uart->wdata);
}

/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static inline void am_uart_sched_event(struct am_uart_port *info, int event)
{
    info->event |= 1 << event;
    schedule_work(&info->tqueue);
}

static void receive_chars(struct am_uart_port *info, struct pt_regs *regs,
			  unsigned short rx)
{
	struct tty_struct *tty = info->port.state->port.tty;
	am_uart_t *uart = uart_addr[info->line];
	int status;
	int mode;
        char ch;
        unsigned long flag = TTY_NORMAL;

	if (!tty) {
		//printk("Uart : missing tty on line %d\n", info->line);
		goto clear_and_exit;
	}
	status = readl(&uart->status);
	if (status & UART_OVERFLOW_ERR) {
              info->rx_error |= UART_OVERFLOW_ERR;
              mode = readl(&uart->mode) | UART_CLEAR_ERR;
		writel(mode, &uart->mode);
	} else if (status & UART_FRAME_ERR) {
		info->rx_error |= UART_FRAME_ERR;
		mode = readl(&uart->mode) | UART_CLEAR_ERR;
		writel(mode, &uart->mode);
	} else if (status & UART_PARITY_ERR) {
		info->rx_error |= UART_PARITY_ERR;
		mode = readl(&uart->mode) | UART_CLEAR_ERR;
		writel(mode, &uart->mode);
	}
	do {
		ch = (rx & 0x00ff);
              tty_insert_flip_char(tty,ch,flag);

		info->rx_cnt++;
		status = (readl(&uart->status) & UART_RXEMPTY);
		if(!status)
		    rx = readl(&uart->rdata);
	} while (!status);
clear_and_exit:
	return;
}

static void BH_receive_chars(struct am_uart_port *info)
{
	struct tty_struct *tty = info->port.state->port.tty;
	unsigned long flag;
	int status;
	int cnt = info->rx_cnt;

	if (!tty) {
		printk("Uart : missing tty on line %d\n", info->line);
		goto clear_and_exit;
	}
       tty->low_latency = 1;	// Originally added by Sameer, serial I/O slow without this
	flag = TTY_NORMAL;
	status = info->rx_error;
	if (status & UART_OVERFLOW_ERR) {
		printk("Uart  Driver: Overflow Error while receiving a character\n");
		flag = TTY_OVERRUN;
	} else if (status & UART_FRAME_ERR) {
		printk("Uart  Driver: Framing Error while receiving a character\n");
		flag = TTY_FRAME;
	} else if (status & UART_PARITY_ERR) {
		printk("Uart  Driver: Parity Error while receiving a character\n");
		flag = TTY_PARITY;
	}
       info->rx_error = 0;
	if(cnt)
       {
            info->rx_cnt =0;
	     tty_flip_buffer_push(tty);
       }

clear_and_exit:
        if (info->rx_cnt>0)
		am_uart_sched_event(info, 0);

        return;
}


static void transmit_chars(struct am_uart_port *info)
{
    am_uart_t *uart = uart_addr[info->line];
    struct uart_port * up = &info->port;
    unsigned int ch;
    struct circ_buf *xmit = &up->state->xmit;
    int count = 256;

    if (up->x_char) {
        writel(up->x_char, &uart->wdata);
        up->x_char = 0;
        goto clear_and_return;
    }

    if (uart_circ_empty(xmit) || uart_tx_stopped(up))
        goto clear_and_return;

    spin_lock(&info->wr_lock);
    while(!uart_circ_empty(xmit) && count-- > 0)
    {
        if((readl(&uart->status) & UART_TXFULL) ==0) {
            ch = xmit->buf[xmit->tail];
            writel(ch, &uart->wdata);
            xmit->tail = (xmit->tail+1) & (SERIAL_XMIT_SIZE - 1);
        }
	 else
	 {
		break;
	 }
    }
    spin_unlock(&info->wr_lock);
clear_and_return:

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(up);

    return;
}


/*
 * This is the serial driver's generic interrupt routine
 */
static irqreturn_t am_uart_interrupt(int irq, void *dev, struct pt_regs *regs)
{
    struct am_uart_port *info=(struct am_uart_port *)dev;
    am_uart_t *uart = NULL;
    struct tty_struct *tty = NULL;

    if (!info)
        goto out;

    tty = info->port.state->port.tty;
    if (!tty)
    {
        goto out;
    }

    uart = uart_addr[info->line];
    if (!uart)
        goto out;

    if (!(readl(&uart->status) & UART_RXEMPTY)){

        receive_chars(info, 0, readl(&uart->rdata));
    }

    if ((readl(&uart->mode) & UART_TXENB)&&!(readl(&uart->status) & UART_TXFULL)) {
        transmit_chars(info);
    }
out:
    am_uart_sched_event(info, 0);
    return IRQ_HANDLED;

}

static void am_uart_workqueue(struct work_struct *work)
{

    struct am_uart_port *info=container_of(work,struct am_uart_port,tqueue);
    am_uart_t *uart = NULL;
    struct tty_struct *tty = NULL;

    if (!info)
        goto out;

    tty = info->port.state->port.tty;
    if (!tty)
    {
        goto out;
    }

    uart = uart_addr[info->line];
    if (!uart)
        goto out;

    if (info->rx_cnt>0)
        BH_receive_chars(info);


    if (readl(&uart->status) & UART_FRAME_ERR)
        writel(readl(&uart->status) & ~UART_FRAME_ERR,&uart->status);
    if (readl(&uart->status) & UART_OVERFLOW_ERR)
        writel(readl(&uart->status) & ~UART_OVERFLOW_ERR,&uart->status);
out:
    return ;
}

static void am_uart_stop_tx(struct uart_port *port)
{
	struct am_uart_port * info = &am_ports[port->line];
    am_uart_t *uart = uart_addr[info->line];
    unsigned long mode;

#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    mutex_lock(&info->info_mutex);
    mode = readl(&uart->mode);
    mode &= ~UART_TXENB;
    writel(mode, &uart->mode);
    mutex_unlock(&info->info_mutex);
}

static void am_uart_start_tx(struct uart_port *port)
{
    struct am_uart_port * info = &am_ports[port->line];
    am_uart_t *uart = uart_addr[info->line];
//    unsigned long mode;
    struct uart_port * up = &info->port;
    unsigned int ch;
    struct circ_buf *xmit = &up->state->xmit;
    unsigned long flags;
	
#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    spin_lock_irqsave(&info->wr_lock,flags);

    while(!uart_circ_empty(xmit))
    {
        if ((readl(&uart->status) & UART_TXFULL) ==0) {
            ch = xmit->buf[xmit->tail];
            writel(ch, &uart->wdata);
            xmit->tail = (xmit->tail+1) & (SERIAL_XMIT_SIZE - 1);
        }
	  else
        {
		break;
	  }
    }
    spin_unlock_irqrestore(&info->wr_lock, flags);
}

static void am_uart_stop_rx(struct uart_port *port)
{
    struct am_uart_port * info = &am_ports[port->line];
    am_uart_t *uart = uart_addr[info->line];
    unsigned long mode;

#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    mutex_lock(&info->info_mutex);
    mode = readl(&uart->mode);
    mode &= ~UART_RXENB;
    writel(mode, &uart->mode);
    mutex_unlock(&info->info_mutex);
}

static void am_uart_enable_ms(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
      return;
}

static unsigned int am_uart_tx_empty(struct uart_port *port)
{
    struct am_uart_port * info = &am_ports[port->line];
    am_uart_t *uart = uart_addr[info->line];
    unsigned long mode;

#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    mutex_lock(&info->info_mutex);
    mode = readl(&uart->status);
    mutex_unlock(&info->info_mutex);

    return ( mode & UART_TXEMPTY) ? TIOCSER_TEMT : 0;
}

static unsigned int am_uart_get_mctrl(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
	return TIOCM_CTS;
}

static void am_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
	return;
}

static void am_uart_break_ctl(struct uart_port *port, int break_state)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
      return;
}



#ifdef CONFIG_CONSOLE_POLL
/*
 * Console polling routines for writing and reading from the uart while
 * in an interrupt or debug context.
 */

static int am_uart_get_poll_char(struct uart_port *port)
{
    struct am_uart_port * info = &am_ports[port->line];
    am_uart_t *uart = uart_addr[info->line];
    int rx;
    unsigned long flags;	

    if (!(readl(&uart->status) & UART_RXEMPTY))
	 rx = readl(&uart->rdata);
    else
        rx = 0;

    return rx;
}


static void am_uart_put_poll_char(struct uart_port *port,
			 unsigned char c)
{
    struct am_uart_port * info = &am_ports[port->line];
    am_uart_t *uart = uart_addr[info->line];
    unsigned long flags;	

    while (readl(&uart->status) & UART_TXFULL) ;
	  writel(c, &uart->wdata);

    return;
}

#endif /* CONFIG_CONSOLE_POLL */

static int am_uart_startup(struct uart_port *port)
{
    struct am_uart_port * info = &am_ports[port->line];
    am_uart_t *uart = uart_addr[info->line];
    unsigned long mode;

#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    mutex_lock(&info->info_mutex);
    set_mask(&uart->mode, UART_RXRST|UART_TXRST|UART_CLEAR_ERR);
    clear_mask(&uart->mode, UART_RXRST|UART_TXRST|UART_CLEAR_ERR);
    mode = readl(&uart->mode);
    mode |= UART_RXENB | UART_TXENB;
    writel(mode, &uart->mode);
    mutex_unlock(&info->info_mutex);

    return 0;
}

static void am_uart_shutdown(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    return;
}

static void change_speed(struct am_uart_port *info, unsigned long newbaud)
{
    am_uart_t *uart = uart_addr[info->line];
    unsigned long tmp;
	struct clk * clk81;
#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    if (newbaud==0)
        return;

    if (newbaud == info->baud)
        return;

	clk81 = clk_get_sys("clk81", NULL);
	if (IS_ERR_OR_NULL(clk81)) {
		printk(KERN_ERR "clk81 is not available\n");
		return;
	}
    msleep(1);


    while (!(readl(&uart->status) & UART_TXEMPTY))
	msleep(10);

    printk(KERN_INFO "Changing baud from %d to %d\n", info->baud, (int)newbaud);
    tmp = (clk_get_rate(clk81) / (newbaud * 4)) - 1;
    info->baud = (int)newbaud;
#if MESON_CPU_TYPE < MESON_CPU_TYPE_MESON6
	tmp = (readl(&uart->mode) & ~0xfff) | (tmp & 0xfff);
	writel(tmp, &uart->mode);
#else
	  tmp = (readl(&uart->reg5) & ~0x7fffff) | (tmp & 0x7fffff);
	  tmp |= 0x800000;
	  writel(tmp, &uart->reg5); 
#endif
    msleep(1);
}

static void
am_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
{
    unsigned int baud;
    struct am_uart_port * info = &am_ports[port->line];
#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    baud = tty_termios_baud_rate(termios);
    change_speed(info, baud);

    uart_update_timeout(port, termios->c_cflag, baud);

    return;
}

static void
am_uart_set_ldisc(struct uart_port *port,int new)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
       return;
}

static void
am_uart_pm(struct uart_port *port, unsigned int state,
	      unsigned int oldstate)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
      return;
}

static void am_uart_release_port(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
       return;
}

static int am_uart_request_port(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
     return 0;
}

static void am_uart_config_port(struct uart_port *port, int flags)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    return;
}

static int
am_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    return 0;
}

static const char *
am_uart_type(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct am_uart_port * info = &am_ports[port->line];
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
        return NULL;
}

static struct uart_ops am_uart_pops = {
	.tx_empty	= am_uart_tx_empty,
	.set_mctrl	= am_uart_set_mctrl,
	.get_mctrl	= am_uart_get_mctrl,
	.stop_tx	= am_uart_stop_tx,
	.start_tx	= am_uart_start_tx,
	.stop_rx	= am_uart_stop_rx,
	.enable_ms	= am_uart_enable_ms,
	.break_ctl	= am_uart_break_ctl,
	.startup	= am_uart_startup,
	.shutdown	= am_uart_shutdown,
	.set_termios	= am_uart_set_termios,
	.set_ldisc	= am_uart_set_ldisc,
	.pm		= am_uart_pm,
	.type		= am_uart_type,
	.release_port	= am_uart_release_port,
	.request_port	= am_uart_request_port,
	.config_port	= am_uart_config_port,
	.verify_port	= am_uart_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char = am_uart_get_poll_char,
	.poll_put_char = am_uart_put_poll_char,
#endif
};


/*
 * Configure the port from the platform device resource info.
 */
static void am_uart_init_port(struct am_uart_port *am_port,int line)
{
    struct uart_port *port = &am_port->port;
    int index = am_port->line;

    port->iotype	= UPIO_PORT;
    port->flags	= UPF_BOOT_AUTOCONF;
    port->ops	= &am_uart_pops;
    port->fifosize	= 1;
    port->line	= line;
    port->irq	= uart_irqs[index];
    port->type =1;
    port->x_char = 0;
}

static void am_uart_start_port(struct am_uart_port *am_port)
{
    struct uart_port *port = &am_port->port;
    int index = am_port->line;
    am_uart_t *uart = uart_addr[index];
    unsigned int fifo_level = uart_FIFO_max_cnt[index];	

    am_port->magic = SERIAL_MAGIC;

    am_port->custom_divisor = 16;
    am_port->close_delay = 50;
    am_port->closing_wait = 100;
    am_port->event = 0;
    am_port->count = 0;
    am_port->blocked_open = 0;

    am_port->rx_cnt= 0;

    init_waitqueue_head(&am_port->open_wait);
    init_waitqueue_head(&am_port->close_wait);

    mutex_init(&am_port->info_mutex);
    INIT_WORK(&am_port->tqueue,am_uart_workqueue);

    set_mask(&uart->mode, UART_RXRST|UART_TXRST|UART_CLEAR_ERR);
    clear_mask(&uart->mode, UART_RXRST|UART_TXRST|UART_CLEAR_ERR);

    set_mask(&uart->mode, UART_RXINT_EN | UART_TXINT_EN);

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
     writel( (fifo_level/2) << 8 | 1, &uart->intctl);
#else
     writel( (fifo_level/2)<< 7 | 1, &uart->intctl);
#endif
    clear_mask(&uart->mode, (1 << 19)) ;

    sprintf(am_port->name,"UART_ttyS%d:",am_port->line);
    if (request_irq(port->irq, (irq_handler_t) am_uart_interrupt, IRQF_SHARED,
	     am_port->name, am_port)) {
        printk("request irq error!!!\n");
    }

    printk("%s(irq = %d)\n", am_port->name,
		       port->irq);
}

static void am_uart_register_ports(struct platform_device *pdev)
{
       struct am_uart_port *port;
       struct aml_uart_platform *uart_data = pdev->dev.platform_data;
	     int i;

       if(inited_ports_flag==1)
            return;

       for(i=0;i<UART_NR;i++)
       {
           if(uart_data->pinmux_uart[i]==NULL)
               continue;
		   port = &am_ports[i];
           port->line  = uart_data->uart_line[i];
           printk(KERN_INFO " set pinmux %p\n",uart_data->pinmux_uart[i]);
           pinmux_set((pinmux_set_t*)(uart_data->pinmux_uart[i]));

			am_uart_init_port(port,i);
       }

       inited_ports_flag = 1;
       return;
}

#ifdef CONFIG_AM_UART_CONSOLE
static int __init am_uart_console_setup(struct console *co, char *options)
{
// 	struct clk *sysclk;
//	unsigned long baudrate = 0;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
       int index = am_ports[co->index].line;
       am_uart_t *uart;
	if(inited_ports_flag==0)
             index =default_index;
	 uart = uart_addr[index];

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	if (baud < 300 || baud > 115200)
		baud = 115200;

//	am_uart_init_port(port,i);
	default_index=index;
    //~ uart_set_options(&am_ports->port,co,baud,parity,bits,flow)
	return 0;		/* successful initialization */
}

static void am_uart_console_write(struct console *co, const char *s, u_int count)
{
       int index = am_ports[co->index].line;
       spinlock_t  * wr_lock = &am_ports[co->index].wr_lock;
	unsigned long flags;	

        if(inited_ports_flag==0)
            index =default_index;

       if (index!=default_index)
		am_uart_console_setup(co, NULL);

	while (count-- > 0) {
		if (*s == '\n')
			am_uart_put_char(index,'\r');
		am_uart_put_char(index,*s++);
	}
}

struct uart_driver am_uart_reg;

struct console am_uart_console = {
	.name		= "ttyS",
	.write		= am_uart_console_write,
	.device		= uart_console_device,
	.setup		= am_uart_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &am_uart_reg,
};


#define AM_UART_CONSOLE	&am_uart_console
#else
#define AM_UART_CONSOLE	NULL
#endif

struct uart_driver am_uart_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "serial",
	.dev_name		= "ttyS",
	.major			= 4,
	.minor			= 64,
	.nr                        = UART_NR,
	.cons			= AM_UART_CONSOLE,
};


#define am_uart_suspend NULL
#define am_uart_resume NULL

static int __devinit am_uart_probe(struct platform_device *pdev)
{
	struct am_uart_port *up;
	int i,ret;
	struct aml_uart_platform *uart_data = pdev->dev.platform_data;

       am_uart_register_ports(pdev);

       for(i=0;i<UART_NR;i++)
       {
           if(uart_data->pinmux_uart[i]==NULL)
                          continue;
			up = &am_ports[i];
			up->port.dev = &pdev->dev;
			ret = uart_add_one_port(&am_uart_reg, &up->port);
			if(ret)
				return ret;
			am_uart_start_port(up);


       }
	return 0;
}

static int __devexit am_uart_remove(struct platform_device *pdev)
{
	struct am_uart_port *up;
       int i,ret = 0;

       for(i=0;i<UART_NR;i++)
       {
           up = &am_ports[i];
	    ret = uart_remove_one_port(&am_uart_reg, &up->port);
       }

       ///?free irq and bh

	return ret;
}

struct platform_driver am_uart_platform_driver = {
	.probe		= am_uart_probe,
	.remove		= __devexit_p(am_uart_remove),
	.suspend	=  am_uart_suspend,
	.resume		= am_uart_resume,
	.driver		= {
		.name	= "mesonuart",
		.owner	= THIS_MODULE,
	},
};

#ifdef CONFIG_AM_UART_CONSOLE
//~ static __initdata struct early_platform_driver early_sport_uart_driver = {
	//~ .class_str = "early-am-uart",
	//~ .pdrv = &am_uart_platform_driver,
	//~ .requested_id = EARLY_PLATFORM_ID_UNSET,
//~ };

//~ static int __init am_uart_console_init(void)
//~ {
	//~ early_platform_driver_register(&early_sport_uart_driver,"am_uart");
//~
	//~ early_platform_driver_probe("early-am-uart",
		//~ 1, 0);
//~
	//~ register_console(&am_uart_console);
//~
	//~ return 0;
//~ }
static int __init am_uart_console_init(void)
{
//	int ret;


	register_console(&am_uart_console);
	return 0;
}
console_initcall(am_uart_console_init);
#endif

static int __init am_uart_init(void)
{
	int ret;

	ret = uart_register_driver(&am_uart_reg);
	if (ret)
		return ret;
	///ret = platform_driver_probe(&am_uart_platform_driver, am_uart_probe);
	ret = platform_driver_register(&am_uart_platform_driver);
	if (ret)
		uart_unregister_driver(&am_uart_reg);

	return ret;
}
static void __exit am_uart_exit(void)
{
#if 0
#ifdef CONFIG_AM_UART_CONSOLE
        unregister_console(&am_uart_console);
#endif
#endif

       platform_driver_unregister(&am_uart_platform_driver);
	uart_unregister_driver(&am_uart_reg);
}

module_init(am_uart_init);
module_exit(am_uart_exit);


/************************************************************************
 * Interrupt Safe Raw Printing Routine so we dont have to worry about
 * possible side effects of calling printk( ) such as
 * context switching, semaphores, interrupts getting re-enabled etc
 *
 * Syntax: (1) Doesn't understand Format Specifiers
 *         (2) Can print one @string and one @number (in hex)
 *
 *************************************************************************/
static char dec_to_hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F'
};

static void raw_num(unsigned int num, int zero_ok)
{
	int nibble;
	int i;
	int leading_zeroes = 1;

	/* If @num is Zero don't print it */
	if (num != 0 || zero_ok) {

		/* break num into 8 nibbles */
		for (i = 7; i >= 0; i--) {
			nibble = (num >> (i << 2)) & 0xF;
			if (nibble == 0) {
				if (leading_zeroes)
					continue;
			} else {
				if (leading_zeroes)
					leading_zeroes = 0;
			}
			am_uart_put_char(default_index,dec_to_hex[nibble]);
		}
	}

	am_uart_put_char(default_index,' ');
}

static int raw_str(const char *str)
{
	int cr = 0;

	/* Loop until we find a Sentinel in the @str */
	while (*str != '\0') {

		/* If a CR found, make a note so that we
		 * print it after printing the number
		 */
		if (*str == '\n') {
			cr = 1;
			break;
		}
		am_uart_put_char(default_index,*str++);
	}

	return cr;
}

void raw_printk(const char *str, unsigned int num)
{
	int cr;
	unsigned long flags;

	local_irq_save(flags);

	cr = raw_str(str);
	raw_num(num, 0);

	/* Carriage Return and Line Feed */
	if (cr) {
		am_uart_put_char(default_index,'\r');
		am_uart_put_char(default_index,'\n');
	}

	local_irq_restore(flags);
}

EXPORT_SYMBOL(raw_printk);

void raw_printk5(const char *str, uint n1, uint n2, uint n3, uint n4)
{
	int cr;
	unsigned long flags;

	local_irq_save(flags);

	cr = raw_str(str);
	raw_num(n1, 1);
	raw_num(n2, 1);
	raw_num(n3, 1);
	raw_num(n3, 0);

	/* Carriage Return and Line Feed */
	if (cr) {
		am_uart_put_char(default_index,'\r');
		am_uart_put_char(default_index,'\n');
	}
	local_irq_restore(flags);
}

EXPORT_SYMBOL(raw_printk5);

int get_baud(int line)
{
    struct am_uart_port * info = &am_ports[line];

    printk("uart%d %s %d\n", line, __FUNCTION__, info->baud);
    return info->baud ? info->baud : 115200;
}
EXPORT_SYMBOL(get_baud);

void set_baud(int line, unsigned long newbaud)
{
      struct am_uart_port * info = &am_ports[line];

      change_speed(info, newbaud);
      printk("uart%d %s %d\n", line, __FUNCTION__, info->baud);
}
EXPORT_SYMBOL(set_baud);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("amlogic uart driver");



