#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
//#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/spi/spi.h>
#include <mach/pinmux.h>
#include "aml_spi.h"

static bool spicc_dbgf = 0;
#define spicc_dbg(fmt, args...)  { if(spicc_dbgf) \
					printk("[spicc]: " fmt, ## args); }

int ss_pinmux[] = {16, 15, 18};
static void spicc_chip_select(struct amlogic_spi *amlogic_spi, bool select)
{
  u8 chip_select = amlogic_spi->tgl_spi->chip_select;
  u32 gpio_cs;
  bool ss_pol = (amlogic_spi->tgl_spi->mode & SPI_CS_HIGH) ? 1 : 0;

  if (amlogic_spi->tgl_spi->controller_data) {
    gpio_cs = *(u32 *)amlogic_spi->tgl_spi->controller_data;
    gpio_out(gpio_cs, ss_pol ? select : !select);
  }
  else if (chip_select < 3){
    amlogic_spi->master_regs->conreg.chip_select = chip_select;
    amlogic_spi->master_regs->conreg.ss_pol = ss_pol;
    amlogic_spi->master_regs->conreg.ss_ctl = ss_pol;
    SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_8, 1<<ss_pinmux[chip_select]);
  }
}

static inline void spicc_reg_dbg(struct amlogic_spi *amlogic_spi)
{
  printk("spiccaddr = 0x%x\n", amlogic_spi->master_regs);
  printk("conreg    = 0x%x\n", amlogic_spi->master_regs->conreg);
  printk("intreg    = 0x%x\n", amlogic_spi->master_regs->intreg);
  printk("dmareg    = 0x%x\n", amlogic_spi->master_regs->dmareg);
  printk("statreg   = 0x%x\n", amlogic_spi->master_regs->statreg);
  printk("periodreg = 0x%x\n", amlogic_spi->master_regs->periodreg);
  printk("testreg   = 0x%x\n", amlogic_spi->master_regs->testreg);
  printk("draddr    = 0x%x\n", amlogic_spi->master_regs->draddr);
  printk("dwaddr    = 0x%x\n", amlogic_spi->master_regs->dwaddr);
}

static void aml_spi_set_mode(struct amlogic_spi *amlogic_spi, unsigned mode) 
{    
  amlogic_spi->cur_mode = mode;
  amlogic_spi->master_regs->conreg.clk_pha = (mode & SPI_CPHA) ? 1:0;
  amlogic_spi->master_regs->conreg.clk_pol = (mode & SPI_CPOL) ? 1:0;
  amlogic_spi->master_regs->conreg.drctl = 0; //data ready, 0-ignore, 1-falling edge, 2-rising edge  
}

static void aml_spi_set_clk(struct amlogic_spi *amlogic_spi, unsigned spi_speed) 
{	
	struct clk *sys_clk = clk_get_sys("clk81", NULL);
	unsigned sys_clk_rate = clk_get_rate(sys_clk);
	unsigned div, mid_speed;
  
  // actually, spi_speed = sys_clk_rate / 2^(conreg.data_rate_div+2)
  mid_speed = (sys_clk_rate * 3) >> 4;
  for(div=0; div<7; div++) {
    if (spi_speed >= mid_speed) break;    
    mid_speed >>= 1;
  }
  amlogic_spi->master_regs->conreg.data_rate_div = div;
  amlogic_spi->cur_speed = spi_speed;
  spicc_dbg("sys_clk_rate=%d, spi_speed=%d, div=%d\n", sys_clk_rate, spi_speed, div);
}

static void aml_spi_set_platform_data(struct amlogic_spi *spi, 
										struct aml_spi_platform *plat)
{
	spi->spi_dev.dev.platform_data = plat;
	
    spi->master_no = plat->master_no;
    spi->io_pad_type = plat->io_pad_type;
    spi->wp_pin_enable = plat->wp_pin_enable;
    spi->hold_pin_enable = plat->hold_pin_enable;
    //spi->irq = plat->irq;
    spi->num_cs = plat->num_cs;
}

/* mode: SPICC_DMA_MODE/SPICC_PIO_MODE
 */
static void spi_hw_init(struct amlogic_spi	*amlogic_spi)
{
  SET_CBUS_REG_MASK(HHI_GCLK_MPEG0, (1<<8)); //SPICC clk gate
  SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_8, (1<<12)|((1<<13))|(1<<14));
  //SET_CBUS_REG_MASK(PAD_PULL_UP_REG5, (1<<1));	//enable internal pullup
  amlogic_spi->master_regs->testreg |= 1<<24; //clock free enable
  amlogic_spi->master_regs->conreg.enable = 0; // disable SPICC
  amlogic_spi->master_regs->conreg.mode = 1; // 0-slave, 1-master
  amlogic_spi->master_regs->conreg.xch = 0;
  amlogic_spi->master_regs->conreg.smc = SPICC_PIO;
  amlogic_spi->master_regs->conreg.bits_per_word = 7;//t->bits_per_word ? t->bits_per_word:spi->bits_per_word;
  aml_spi_set_mode(amlogic_spi, SPI_MODE_0); // default spi mode 0
  aml_spi_set_clk(amlogic_spi, SPI_CLK_2M); // default spi speed 2M
  amlogic_spi->master_regs->conreg.ss_ctl = 1;
}


static int spi_add_dev(struct amlogic_spi *amlogic_spi, struct spi_master	*master)
{
	amlogic_spi->spi_dev.master = master;
	device_initialize(&amlogic_spi->spi_dev.dev);
	amlogic_spi->spi_dev.dev.parent = &amlogic_spi->master->dev;
	amlogic_spi->spi_dev.dev.bus = &spi_bus_type;

	strcpy((char *)amlogic_spi->spi_dev.modalias, SPI_DEV_NAME);
	dev_set_name(&amlogic_spi->spi_dev.dev, "%s:%d", "m1spi", master->bus_num);
	return device_add(&amlogic_spi->spi_dev.dev);
}

//setting clock and pinmux here
static  int amlogic_spi_setup(struct spi_device	*spi)
{
    return 0;
}

static  void amlogic_spi_cleanup(struct spi_device	*spi)
{
	if (spi->modalias)
		kfree(spi->modalias);
}

static void amlogic_spi_handle_one_msg(struct amlogic_spi *amlogic_spi, struct spi_message *m)
{
	struct spi_device *spi = m->spi;
	struct spi_transfer *t;
	struct aml_spi_reg_master __iomem* regs = amlogic_spi->master_regs;

  int len, num, i, retry;
  u8 *txp, *rxp, dat;
  int ret = 0;

  // re-set to prevent others from disable the SPICC clk gate
  SET_CBUS_REG_MASK(HHI_GCLK_MPEG0, (1<<8)); // SPICC clk gate
  if (amlogic_spi->tgl_spi != spi) {
    amlogic_spi->tgl_spi = spi;
    aml_spi_set_clk(amlogic_spi, spi->max_speed_hz);	    
	  aml_spi_set_mode(amlogic_spi, spi->mode);
	}
  spicc_chip_select(amlogic_spi, 1); // select
  regs->conreg.enable = 1; // enable spicc
  
	list_for_each_entry(t, &m->transfers, transfer_list) {
  	if((amlogic_spi->cur_speed != t->speed_hz) && t->speed_hz) {
  	    aml_spi_set_clk(amlogic_spi, t->speed_hz);	    
  	}  
    spicc_dbg("length = %d\n", t->len);
    len = t->len;
    txp = (u8 *)t->tx_buf;
    rxp = (u8 *)t->rx_buf;
    while (len > 0) {
      num = (len > SPICC_FIFO_SIZE) ? SPICC_FIFO_SIZE : len;
      for (i=0; i<num; i++) {
        dat = txp ? (*txp++) : 0;
        regs->txdata = dat;
        spicc_dbg("txdata[%d] = 0x%x\n", i, dat);
      }
      for (i=0; i<num; i++) {
        retry = 100;
        while (!regs->statreg.rx_ready && retry--) {udelay(1);}
        dat = regs->rxdata;
        if (rxp) *rxp++ = dat;
        spicc_dbg("rxdata[%d] = 0x%x\n", i, dat);
        if (!retry) {
          ret = -EBUSY;
          printk("error: spicc timeout\n");
          goto spi_handle_end;
        }
      }
      len -= num;
    }

		m->actual_length += t->len;
		if (t->delay_usecs) {
			udelay(t->delay_usecs);
		}
	}

spi_handle_end:
  regs->conreg.enable = 0; // disable spicc
  spicc_chip_select(amlogic_spi, 0); // unselect
  
  m->status = ret;
  if(m->context) {
    m->complete(m->context);
  }
}

static int amlogic_spi_transfer(struct spi_device *spi,
				struct spi_message *m)
{
	struct amlogic_spi *amlogic_spi = spi_master_get_devdata(spi->master);
	unsigned long flags;

	m->actual_length = 0;
	m->status = -EINPROGRESS;

	spin_lock_irqsave(&amlogic_spi->lock, flags);
	list_add_tail(&m->queue, &amlogic_spi->msg_queue);
	queue_work(amlogic_spi->workqueue, &amlogic_spi->work);
	spin_unlock_irqrestore(&amlogic_spi->lock, flags);

	return 0;
}

static void amlogic_spi_work(struct work_struct *work)
{
	struct amlogic_spi *amlogic_spi = container_of(work, struct amlogic_spi,
						       work);
    unsigned long flags;

	spin_lock_irqsave(&amlogic_spi->lock, flags);
	while (!list_empty(&amlogic_spi->msg_queue)) {
		struct spi_message *m = container_of(amlogic_spi->msg_queue.next,
						   struct spi_message, queue);

		list_del_init(&m->queue);
		spin_unlock_irqrestore(&amlogic_spi->lock, flags);

		amlogic_spi_handle_one_msg(amlogic_spi, m);

		spin_lock_irqsave(&amlogic_spi->lock, flags);
	}
	spin_unlock_irqrestore(&amlogic_spi->lock, flags);
}

static int amlogic_spi_probe(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct amlogic_spi	*amlogic_spi;
	struct resource		*res;
	int			status = 0;

   
	if (pdev->dev.platform_data == NULL) {
		dev_err(&pdev->dev, "platform_data missing!\n");
		return -ENODEV;
	}

	master = spi_alloc_master(&pdev->dev, sizeof *amlogic_spi);
	if (master == NULL) {
		dev_dbg(&pdev->dev, "Unable to allocate SPI Master\n");
		return -ENOMEM;
	}	

  master->bus_num = (pdev->id != -1)? pdev->id:1;

	master->setup = amlogic_spi_setup;
	master->transfer = amlogic_spi_transfer;
	master->cleanup = amlogic_spi_cleanup;

	dev_set_drvdata(&pdev->dev, master);

	amlogic_spi = spi_master_get_devdata(master);
	amlogic_spi->master = master;

	aml_spi_set_platform_data(amlogic_spi, pdev->dev.platform_data);
	//init_completion(&amlogic_spi->done);
	
  master->num_chipselect = amlogic_spi->num_cs;
	res = platform_get_resource(pdev, IORESOURCE_MEM, amlogic_spi->master_no);
	if (res == NULL) {
	    status = -ENOMEM;
	    dev_err(&pdev->dev, "Unable to get SPI MEM resource!\n");
        goto err1;
	}		
    amlogic_spi->master_regs = (struct aml_spi_reg_master __iomem*)(res->start);
#if 0	//remove irq 
	/* Register for SPI Interrupt */
	status = request_irq(amlogic_spi->irq, amlogic_spi_irq_handle,
			  0, "aml_spi", amlogic_spi);	
	if (status != 0)
	{
	    dev_dbg(&pdev->dev, "spi request irq failed and status:%d\n", status);
	    goto err1;
	}
#endif	  
	amlogic_spi->workqueue = create_singlethread_workqueue(
		dev_name(master->dev.parent));
	if (amlogic_spi->workqueue == NULL) {
		status = -EBUSY;
		goto err1;
	}	
	  		  
	spin_lock_init(&amlogic_spi->lock);
	INIT_WORK(&amlogic_spi->work, amlogic_spi_work);
	INIT_LIST_HEAD(&amlogic_spi->msg_queue);
		
	status = spi_register_master(master);
	if (status < 0)
    {
        dev_dbg(&pdev->dev, "spi register failed and status:%d\n", status);
		goto err0;
	}
	
	spi_hw_init(amlogic_spi);
  spicc_reg_dbg(amlogic_spi);
 	status = spi_add_dev(amlogic_spi, master);
	printk("aml spi init ok and status:%d \n", status);
	
	return status;    
err0:
   destroy_workqueue(amlogic_spi->workqueue);     
err1:
	spi_master_put(master);
	return status;
}

static int amlogic_spi_remove(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct amlogic_spi	*amlogic_spi;

	master = dev_get_drvdata(&pdev->dev);
	amlogic_spi = spi_master_get_devdata(master);

	spi_unregister_master(master);

	return 0;
}

static struct platform_driver amlogic_spi_driver = { 
	.probe = amlogic_spi_probe, 
	.remove = amlogic_spi_remove, 
	.driver =
	    {
			.name = "aml_spi", 
			.owner = THIS_MODULE, 
		}, 
};

static int __init amlogic_spi_init(void) 
{	
	return platform_driver_register(&amlogic_spi_driver);
}

static void __exit amlogic_spi_exit(void) 
{	
	platform_driver_unregister(&amlogic_spi_driver);
} 

subsys_initcall(amlogic_spi_init);
module_exit(amlogic_spi_exit);
//module_init(amlogic_spi_init);
//module_exit(amlogic_spi_exit);

MODULE_DESCRIPTION("Amlogic Spi driver");
MODULE_LICENSE("GPL");
