/*
 *  linux/drivers/tty/serial/meson_uart.c
 *
 *  Driver for almogic-type serial ports
 *
 *  Based on serial.c, 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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
#include <plat/platform.h>
#include <mach/hardware.h>
#include <mach/pinmux.h>
#include <linux/clk.h>
#include <linux/uart-aml.h>

#include <asm/io.h>
#include <asm/irq.h>

#include "meson_uart.h"


#include <asm/serial.h>
#define DRIVER_NAME "meson_uart"



#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

///#define PRINT_DEBUG
#ifdef PRINT_DEBUG
unsigned int DEBUG_PORT_ID=0;
#endif
typedef volatile struct {
  volatile unsigned long wdata;
  volatile unsigned long rdata;
  volatile unsigned long mode;
  volatile unsigned long status;
  volatile unsigned long intctl;
} meson_uart_t;
struct meson_uart_port {
	struct uart_port	port;
	unsigned version;
	unsigned inited;

	/* We need to know the current clock divisor
	 * to read the bps rate the chip has currently
	 * loaded.
	 */
	char 		name[16];
	int 		baud;
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
    struct clk *         clksrc;   
	const char *		clksrc_name;
	struct work_struct	tqueue;
	struct work_struct	tqueue_hangup;

/* Sameer: Obsolete structs in linux-2.6 */
/* 	struct termios		normal_termios; */
/* 	struct termios		callout_termios; */
	wait_queue_head_t	open_wait;
	wait_queue_head_t	close_wait;
	meson_uart_t * uart_addr;
	unsigned    fifo_level;
};
static struct clk *  meson_uart_getclksrc(struct meson_uart_port * port)
{
	struct clk * clk;
	if(IS_ERR_OR_NULL(port))
		BUG();
	if(IS_ERR_OR_NULL(port->clksrc_name))
		BUG();
	if(IS_ERR_OR_NULL(port->clksrc))
		clk=clk_get_sys(port->clksrc_name,NULL);
	else
		clk=port->clksrc;
	if(IS_ERR_OR_NULL(clk))
		BUG();
	
	return clk;
}
static struct meson_uart_port * meson_ports[16];
static unsigned uart_nr=0;
#define UART_NR     MIN(uart_nr=uart_nr?uart_nr:mesonplat_cpu_resource_get_num(DRIVER_NAME),ARRAY_SIZE(meson_ports))


/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static inline void meson_uart_sched_event(struct meson_uart_port *info, int event)
{
    info->event |= 1 << event;
    schedule_work(&info->tqueue);
}

static void receive_chars(struct meson_uart_port *info, struct pt_regs *regs,
			  unsigned short rx)
{
	struct tty_struct *tty = info->port.state->port.tty;
	meson_uart_t *uart = info->uart_addr;
	int status;
	int mode;
	char ch;
	unsigned long flag = TTY_NORMAL;  
	unsigned int fifo_level = info->fifo_level;

	fifo_level = (fifo_level-1);

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
		if ((status = readl(&uart->status) & fifo_level))
			rx = readl(&uart->rdata);
	} while (status);

clear_and_exit:
	return;
}

static void BH_receive_chars(struct meson_uart_port *info)
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
		meson_uart_sched_event(info, 0);	

        return;
}

static void transmit_chars(struct meson_uart_port *info)
{
    meson_uart_t *uart = info->uart_addr;
    struct uart_port * up = &info->port;
    unsigned int ch;
    struct circ_buf *xmit = &up->state->xmit;

    unsigned int fifo_level = info->fifo_level;

    fifo_level = (fifo_level-1)<<8;
       
    spin_lock(&up->lock);
    if (up->x_char) {
        writel(up->x_char, &uart->wdata);
        up->x_char = 0;
        goto clear_and_return;
    }
    
    if (uart_circ_empty(xmit) || uart_tx_stopped(up))
        goto clear_and_return;

    while(!uart_circ_empty(xmit))
    {
        if(((readl(&uart->status) & 0xff00) < fifo_level)) {
            ch = xmit->buf[xmit->tail];
            writel(ch, &uart->wdata);
            xmit->tail = (xmit->tail+1) & (SERIAL_XMIT_SIZE - 1);
        }
    }
clear_and_return:
    spin_unlock(&up->lock);
    if (!uart_circ_empty(xmit))
        meson_uart_sched_event(info, 0);	

    /* Clear interrupt (should be auto) */
    return;
}


/*
 * This is the serial driver's generic interrupt routine
 */
static irqreturn_t meson_uart_interrupt(int irq, void *dev, struct pt_regs *regs)
{  
    struct meson_uart_port *info=(struct meson_uart_port *)dev;    
    meson_uart_t *uart = NULL;
    struct tty_struct *tty = NULL;

    if (!info)
        goto out;

    tty = info->port.state->port.tty;
    if (!tty)
    {   
        goto out;
    }
    
    uart = info->uart_addr;
    if (!uart)
        goto out;

    if ((readl(&uart->mode) & UART_RXENB)&& !(readl(&uart->status) & UART_RXEMPTY)){
		
        receive_chars(info, 0, readl(&uart->rdata));
    }
       
out:	
    meson_uart_sched_event(info, 0);
    return IRQ_HANDLED;
    
}

static void meson_uart_workqueue(struct work_struct *work)
{
	
    struct meson_uart_port *info=container_of(work,struct meson_uart_port,tqueue);
    meson_uart_t *uart = NULL;
    struct tty_struct *tty = NULL;
    unsigned int fifo_level = info->fifo_level;

    fifo_level = (fifo_level-1)<<8;
       
    if (!info)
        goto out;

    tty = info->port.state->port.tty;
    if (!tty)
    {   
        goto out;
    }
    
    uart = info->uart_addr;
    if (!uart)
        goto out;

    if (info->rx_cnt>0)
        BH_receive_chars(info);
       
    if ((readl(&uart->mode) & UART_TXENB)&&((readl(&uart->status) & 0xff00) < fifo_level)) {
        transmit_chars(info);
    }
    if (readl(&uart->status) & UART_FRAME_ERR)
        writel(readl(&uart->status) & ~UART_FRAME_ERR,&uart->status);
    if (readl(&uart->status) & UART_OVERFLOW_ERR)
        writel(readl(&uart->status) & ~UART_OVERFLOW_ERR,&uart->status);
out:
    return ;
}

static void meson_uart_stop_tx(struct uart_port *port)
{
	struct meson_uart_port * info = (struct meson_uart_port *)port;
    meson_uart_t *uart = info->uart_addr;
    unsigned long mode;

#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
       
    mode = readl(&uart->mode);
    mode &= ~UART_TXENB;
    writel(mode, &uart->mode);	
}

static void meson_uart_start_tx(struct uart_port *port)
{
	
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    meson_uart_t *uart = info->uart_addr;
    unsigned long mode;
    struct uart_port * up = &info->port;
    unsigned int ch;
    struct circ_buf *xmit = &up->state->xmit;
    unsigned int fifo_level = info->fifo_level;

    fifo_level = (fifo_level-1)<<8; 
    
    mode = readl(&uart->mode);
    mode |= UART_TXENB;
    writel(mode, &uart->mode);	

    while(!uart_circ_empty(xmit))
    {
        if (((readl(&uart->status) & 0xff00) < fifo_level)) {
            ch = xmit->buf[xmit->tail];
            writel(ch, &uart->wdata);
            xmit->tail = (xmit->tail+1) & (SERIAL_XMIT_SIZE - 1);
        }
    }
    
}

static void meson_uart_stop_rx(struct uart_port *port)
{
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    meson_uart_t *uart = info->uart_addr;
    unsigned long mode;

#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    mode = readl(&uart->mode);
    mode &= ~UART_RXENB;
    writel(mode, &uart->mode);	
}

static void meson_uart_enable_ms(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
      return;
}

static unsigned int meson_uart_tx_empty(struct uart_port *port)
{
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    meson_uart_t *uart = info->uart_addr;
    unsigned long mode;

#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    
    mode = readl(&uart->status);
    
      
    return ( mode & UART_TXEMPTY) ? TIOCSER_TEMT : 0;
}

static unsigned int meson_uart_get_mctrl(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
	return TIOCM_CTS;
}

static void meson_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
	return;
}

static void meson_uart_break_ctl(struct uart_port *port, int break_state)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
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

static int meson_uart_get_poll_char(struct uart_port *port)
{
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    meson_uart_t *uart = info->uart_addr;
    int status,rx;
    unsigned int fifo_level = info->fifo_level;

    fifo_level = (fifo_level-1);

    
    if ((status = readl(&uart->status) & fifo_level))
	      rx = readl(&uart->rdata);
    else
        rx = 0;
    

    return rx;
}


static void meson_uart_put_poll_char(struct uart_port *port,
			 unsigned char c)
{
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    meson_uart_t *uart = info->uart_addr;
    unsigned int fifo_level = info->fifo_level;

    fifo_level = (fifo_level-1)<<8;
   
    
    while (!((readl(&uart->status) & 0xff00)< fifo_level)) ;
	writel(c, &uart->wdata);
    

    return;
}

#endif /* CONFIG_CONSOLE_POLL */
static void meson_uart_start_port(struct meson_uart_port *am_port)
{
    struct uart_port *port = &am_port->port;
    int index = am_port->line;
    meson_uart_t *uart = am_port->uart_addr;
    
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

    INIT_WORK(&am_port->tqueue,meson_uart_workqueue);

    setbits_le32(&uart->mode, UART_RXRST);
    clrbits_le32(&uart->mode, UART_RXRST);

    setbits_le32(&uart->mode, UART_RXINT_EN | UART_TXINT_EN);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
     writel( 48 << 8 | 1, &uart->intctl);
#elif defined(CONFIG_ARCH_MESON2) || defined(CONFIG_ARCH_MESON1)
     writel(1 << 7 | 1, &uart->intctl);
#else
#error NO support of this CPU
#endif
    clrbits_le32(&uart->mode, (1 << 19)) ;
       
    sprintf(am_port->name,"UART_ttyS%d:",am_port->line);
    if (request_irq(port->irq, (irq_handler_t) meson_uart_interrupt, IRQF_SHARED,
	     am_port->name, am_port)) {
        printk("request irq error!!!\n");
    }

    printk("%s(irq = %d)\n", am_port->name,
		       port->irq);
}

static int meson_uart_startup(struct uart_port *port)
{
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    meson_uart_t *uart = info->uart_addr;
    unsigned long mode;
 
#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    mode = readl(&uart->mode);
    mode |= UART_RXENB | UART_TXENB;
    writel(mode, &uart->mode);	
	meson_uart_start_port(info);
    return 0;
}

static void meson_uart_shutdown(struct uart_port *port)
{
 // struct meson_uart_port * info = (struct meson_uart_port *)port;
 // meson_uart_t *uart = info->uart_addr;
//	unsigned int mode ;
				/* All off! */
//	mode = readl(&uart->mode);
//	mode &= ~(UART_TXENB | UART_RXENB);
//	writel(mode, &uart->mode);
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    return;
}

static void change_speed(struct meson_uart_port *info, unsigned long newbaud)
{
    meson_uart_t *uart = info->uart_addr;
    struct tty_struct *tty;
    unsigned long tmp;
    struct clk * clksrc;
#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
#if 0
    tty = info->port.state->port.tty;
    if (!tty || !tty->termios)
        return;
#endif        

    if (newbaud == 0)
        return;

    if (newbaud == info->baud)
        return;

    clksrc = meson_uart_getclksrc(info);
    if(IS_ERR_OR_NULL(clksrc))
    {
        printk("Clock source is not available");
        return;
    }

    printk("Changing baud to %d\n", (int)newbaud);

    while (!(readl(&uart->status) & UART_TXEMPTY)) {
    }
    
    tmp = (clk_get_rate(clksrc) / (newbaud * 4)) - 1;
    info->baud = (int)newbaud;

    tmp = (readl(&uart->mode) & ~0xfff) | (tmp & 0xfff);
    writel(tmp, &uart->mode);
}

static void
meson_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
{
    unsigned int baud;
    struct meson_uart_port * info = (struct meson_uart_port *)port;
#ifdef PRINT_DEBUG
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif

    //if (termios->c_cflag == old->c_cflag)
    //    return;

    baud = tty_termios_baud_rate(termios);
    change_speed(info, baud);

    uart_update_timeout(port, termios->c_cflag, baud);

    return;
}

static void
meson_uart_set_ldisc(struct uart_port *port,int new)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
       return;
}

static void
meson_uart_pm(struct uart_port *port, unsigned int state,
	      unsigned int oldstate)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
      return;
}

static void meson_uart_release_port(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
       return;
}

static int meson_uart_request_port(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
     return 0;
}

static void meson_uart_config_port(struct uart_port *port, int flags)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    return;
}

static int
meson_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
    return 0;
}

static const char *
meson_uart_type(struct uart_port *port)
{
#ifdef PRINT_DEBUG
    struct meson_uart_port * info = (struct meson_uart_port *)port;
    if(info->line == DEBUG_PORT_ID)
        printk("%s\n", __FUNCTION__);
#endif
        return NULL;
}



static struct uart_ops meson_uart_pops = {
	.tx_empty	= meson_uart_tx_empty,
	.set_mctrl	= meson_uart_set_mctrl,
	.get_mctrl	= meson_uart_get_mctrl,
	.stop_tx	= meson_uart_stop_tx,
	.start_tx	= meson_uart_start_tx,
	.stop_rx	= meson_uart_stop_rx,
	.enable_ms	= meson_uart_enable_ms,
	.break_ctl	= meson_uart_break_ctl,
	.startup	= meson_uart_startup,
	.shutdown	= meson_uart_shutdown,
	.set_termios	= meson_uart_set_termios,
	.set_ldisc	= meson_uart_set_ldisc,
	.pm		= meson_uart_pm,
	.type		= meson_uart_type,
	.release_port	= meson_uart_release_port,
	.request_port	= meson_uart_request_port,
	.config_port	= meson_uart_config_port,
	.verify_port	= meson_uart_verify_port,
#ifdef CONFIG_CONSOLE_POLL	
	.poll_get_char = meson_uart_get_poll_char,
	.poll_put_char = meson_uart_put_poll_char,
#endif
};
static struct meson_uart_port   meason_uart_early_port={
	.port={.ops=&meson_uart_pops}
};





#ifdef CONFIG_AM_UART_CONSOLE
static int __init meson_uart_console_setup(struct console *co, char *options)
{
	struct meson_uart_port *uart;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow ='n';
	if (co->index < 0 || co->index >=UART_NR)
		return -ENODEV;

	uart = meson_ports[co->index];
	if (!uart)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else{
		///bfin_serial_console_get_options(uart, &baud, &parity, &bits);
		printk(KERN_INFO DRIVER_NAME " option=NULL is not implement");
		return -ENODEV;
	}

	return uart_set_options(&uart->port, co, baud, parity, bits, flow);
}
/*
 * Output a single character, using UART polled mode.
 * This is used for console output.
 */
static void meson_uart_console_put_char(struct uart_port * port,int ch)
{
	meson_uart_t *uart = (typeof(uart))port->mapbase;
	unsigned int fifo_level = port->fifosize;
    fifo_level = (fifo_level-1)<<8;
	while (!((readl(&uart->status) & 0xff00)< fifo_level)) ;
	writel(ch, &uart->wdata);
}
static void meson_uart_console_write(struct console *co, const char *s, u_int count)
{
#if 0
       int index = 0;//am_ports[co->index].line;



       if (index!=default_index)
		meson_uart_console_setup(co, NULL);
	
       
	while (count-- > 0) {
		if (*s == '\n')
			meson_uart_console_put_char(&meson_ports[co->index],'\r');
		meson_uart_console_put_char(&meson_ports[co->index],*s++);
	}    
#endif	
	struct meson_uart_port *uart = meson_ports[co->index];
	unsigned long flags;

	spin_lock_irqsave(&uart->port.lock, flags);
	uart_console_write(&uart->port, s, count, meson_uart_console_put_char);
	spin_unlock_irqrestore(&uart->port.lock, flags);
}

struct uart_driver meson_uart_reg;
struct console meson_uart_console = {
	.name		= "ttyS",
	.write		= meson_uart_console_write,
	.device		= uart_console_device,
	.setup		= meson_uart_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &meson_uart_reg,
};


#define AM_UART_CONSOLE	&meson_uart_console
#else
#define AM_UART_CONSOLE	NULL
#endif

struct uart_driver meson_uart_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "serial",
	.dev_name		= "ttyS",
	.major			= 4,
	.minor			= 64,
	.cons			= AM_UART_CONSOLE,
};

#define meson_uart_suspend NULL
#define meson_uart_resume NULL



static int __devexit meson_uart_remove(struct platform_device *pdev)
{
	/**
	 * @todo modify it later
	 */
	struct meson_uart_port *up;
	int i,ret = 0;
	if(meson_ports[pdev->id]==NULL)
	{
		printk(KERN_INFO DRIVER_NAME " Fatal error");
		return -ENXIO;
	}
	if(is_early_platform_device(pdev)){
		printk(KERN_INFO DRIVER_NAME " remove early platform driver %d",pdev->id);
		meson_ports[pdev->id]=NULL;
		return 0;
	}
	up=dev_get_drvdata(&pdev->dev);
	dev_set_drvdata(&pdev->dev, NULL);
	if (up) {
		uart_remove_one_port(&meson_uart_reg, &up->port);
		kfree(up);
		meson_ports[pdev->id] = NULL;
	}
	return ret;
}

///#define mesonplat_resource_get_num(DRIVER_NAME);
static int __devinit meson_uart_probe(struct platform_device *pdev)
{
	struct meson_uart_port *up;
	struct resource *resource;
	struct uart_port *port;
	int i,ret,irq;
	meson_uart_reg.nr=UART_NR;
	if(pdev->id<0 || pdev->id >=UART_NR)
	{
		printk(KERN_INFO DRIVER_NAME " does not support pdev->id<0 or pdev->id >= %d",UART_NR);
		return -ENXIO;
	}
	if(is_early_platform_device(pdev))
	{
		if(meson_ports[pdev->id]==NULL)
		{
			for(i=0;i<meson_uart_reg.nr;i++)
			{
				if(meson_ports[i]==&meason_uart_early_port)
				{
					printk(KERN_INFO DRIVER_NAME "only one eraly uart dirver allow\n");
					BUG();
				}
			}
		}else{
			printk(KERN_INFO DRIVER_NAME "only one eraly uart dirver allow\n");
			BUG();
		}
	}else if(meson_ports[i]==&meason_uart_early_port){
		printk(KERN_INFO DRIVER_NAME "Register ttyS%d \n",pdev->id);
		ret = uart_add_one_port(&meson_uart_reg, &meason_uart_early_port.port);
		if(ret)
			goto out;	
		meson_uart_start_port(&meason_uart_early_port);
		return 0;		
	}
	if(meson_ports[pdev->id]!=NULL)
	{
		printk(KERN_INFO DRIVER_NAME " %d is reprobe again",pdev->id);
		if(&meason_uart_early_port==meson_ports[pdev->id])
		{
		    printk(KERN_INFO DRIVER_NAME " %d is early device reprobe again",pdev->id);
		}else if(!is_early_platform_device(pdev)){
			meson_uart_remove(pdev);
			meson_ports[pdev->id]=NULL;
		}else{
			printk(KERN_INFO DRIVER_NAME " %d is earlyplat driver",pdev->id);
			meson_ports[pdev->id]=NULL;
		}
///		meson_ports[pdev->id]=NULL;
	}
	if(!is_early_platform_device(pdev))
	{
		printk(KERN_INFO DRIVER_NAME " %d is alloc",pdev->id);
		
		up=(struct meson_uart_port *)kzalloc(sizeof(struct meson_uart_port),GFP_KERNEL);
	}
	else
		up=&meason_uart_early_port;
	printk(KERN_INFO DRIVER_NAME " %d %d ",__LINE__,pdev->id);
	if(unlikely(up==NULL))
		return -ENOMEM;
	printk(KERN_INFO DRIVER_NAME " %d %d ",__LINE__,pdev->id);
	port= &up->port;
	port->flags=UPF_BOOT_AUTOCONF;
	port->ops=&meson_uart_pops;
	port->private_data=up;
	port->line=pdev->id;
	port->iotype	= UPIO_PORT;
    	port->type =1;
	port->x_char = 0;
    	port->dev = &pdev->dev;
    	resource = platform_get_resource(pdev, IORESOURCE_MEM,0);
	printk(KERN_INFO DRIVER_NAME " %d %d ",__LINE__,pdev->id);
	if (unlikely(!resource))
	{
			ret= -ENXIO;
			goto out;
	}
	
	port->mapbase = resource->start;
	up->uart_addr=(meson_uart_t*)resource->start;
	/** get irq number **/
	irq=platform_get_irq(pdev,0);
	printk(KERN_INFO DRIVER_NAME " %d %d ",__LINE__,irq);
	if (unlikely(irq < 0))
	{
			ret= -ENXIO;
			BUG();
			goto out;
	}
	port->irq=irq;
	printk(KERN_INFO DRIVER_NAME " %d %d ",__LINE__,pdev->id);
	resource = platform_get_resource_byname(pdev, 0,"feature");
	if (unlikely(!resource))
	{
			ret= -ENXIO;
			BUG();
			goto out;
	}
	up->clksrc_name=platform_get_resource_byname(pdev, 0,"clksrc");
	if (unlikely(IS_ERR_OR_NULL(up->clksrc_name)))
	{
			ret= -ENXIO;
			BUG();
			goto out;
	}
	port->fifosize	= resource->start;
	up->version=resource->end;
	
	ret=mesonplat_pad_enable((platform_data_t)(pdev->dev.platform_data));
	if(ret){
		BUG();
		goto out;
	}
	port->dev = &pdev->dev;
    	dev_set_drvdata(&pdev->dev, up);
		
	if(!is_early_platform_device(pdev))
	{
		printk(KERN_INFO DRIVER_NAME "Register ttyS%d \n",pdev->id);
		ret = uart_add_one_port(&meson_uart_reg, port);
		if(ret)
			goto out;	
		meson_uart_start_port(up);
	}else{
		printk(KERN_INFO DRIVER_NAME "  %d is early platform driver",pdev->id);
	}
	spin_lock_init(&up->port.lock);
	meson_ports[pdev->id]=up;
	return 0;
out:
	if(!is_early_platform_device(pdev))
	{
		kfree(up);
	}
	meson_ports[pdev->id]=NULL;
	return ret;
}


struct platform_driver meson_uart_platform_driver = {
	.probe		= meson_uart_probe,
	.remove		= __devexit_p(meson_uart_remove),
	.suspend	=  meson_uart_suspend,
	.resume		= meson_uart_resume,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

#ifdef CONFIG_AM_UART_CONSOLE

early_platform_init("earlycon", &meson_uart_platform_driver);

static int __init meson_uart_console_init(void)
{////bootseq 10
	int ret;
	early_platform_driver_probe("earlycon",4,0);
	register_console(&meson_uart_console);
	return 0;
}
console_initcall(meson_uart_console_init);

#endif

static int __init meson_uart_init(void){
	int ret,i;

	ret = uart_register_driver(&meson_uart_reg);
	if (ret)
		return ret;
	ret = platform_driver_register(&meson_uart_platform_driver);
	if (ret)
    {
		uart_unregister_driver(&meson_uart_reg);
        
    }else{
    }

	return ret;
}
static void __exit meson_uart_exit(void)
{
#ifdef CONFIG_AM_UART_CONSOLE
        unregister_console(&meson_uart_console);
#endif
	platform_driver_unregister(&meson_uart_platform_driver);
	uart_unregister_driver(&meson_uart_reg);
}

module_init(meson_uart_init);
module_exit(meson_uart_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("amlogic uart driver");



