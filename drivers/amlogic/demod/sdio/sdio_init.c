
#include "sdio_init.h"

#include <linux/kthread.h>

char * sd_error_to_string(int errcode);
#define PREG_AGPIO_EN_N 	PREG_PAD_GPIO4_EN_N//PREG_PAD_GPIO0_EN_N
#define PREG_AGPIO_O		PREG_PAD_GPIO4_O// PREG_PAD_GPIO0_O
#define PREG_BGPIO_EN_N 	PREG_PAD_GPIO0_EN_N
#define PREG_BGPIO_O		 PREG_PAD_GPIO0_O
#define PREG_CGPIO_EN_N 	PREG_PAD_GPIO1_EN_N
#define PREG_CGPIO_O		 PREG_PAD_GPIO1_O




void  cpu_sdio_pwr_prepare(unsigned port)
{
        switch (port) {
        case SDIO_PORT_A://SDIOA,GPIOA_9~GPIOA_14
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_8,0x3f<<0);
			//	CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_5,(0x1f<<10));
			//	CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_3,(0xf<<27));

				CLEAR_CBUS_REG_MASK(PREG_AGPIO_EN_N,(0xf<<0));
				CLEAR_CBUS_REG_MASK(PREG_AGPIO_EN_N,(0x3<<8));
                CLEAR_CBUS_REG_MASK(PREG_AGPIO_O,(0xf<<0));
               	CLEAR_CBUS_REG_MASK(PREG_AGPIO_O,(0x3<<8));
				
                ///@todo pull down these bits
                break;
        case SDIO_PORT_B://SDIOB,GPIOA_0~GPIOA_5
                //diable SDIO_B1
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_2,(0x3f<<10));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_2,0xf<<4);
          
                //CLEAR_CBUS_REG_MASK(PREG_AGPIO_O,(0x3f<<4));
                ///@todo pull down these bits
                break;
        case SDIO_PORT_C://SDIOC GPIOB_2~GPIOB_7
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_2,(0xf<<16)|(1<<12)|(1<<8));
                CLEAR_CBUS_REG_MASK(PREG_BGPIO_EN_N,(0x3f<<21));
                CLEAR_CBUS_REG_MASK(PREG_BGPIO_O,(0x3f<<21));
                ///@todo pull down these bits
                break;
        case SDIO_PORT_B1://SDIOB1 GPIOE_6~GPIOE_11
                // disable SPI and SDIO_B
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1,( (1<<29) | (1<<27) | (1<<25) | (1<<23)|0x3f));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6,0x7fff);
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_7,(0x3f<<24));
                //CLEAR_CBUS_REG_MASK(PREG_EGPIO_EN_N,(0x3f<<6));
                // CLEAR_CBUS_REG_MASK(PREG_EGPIO_O,(0x3f<<6));

                ///@todo pull down these bits
                break;

        }

        /**
            do nothing here
        */
}
int cpu_sdio_init(unsigned port)
{
     //   SET_CBUS_REG_MASK(PREG_CGPIO_EN_N,1<<22);
     //	SET_CBUS_REG_MASK(PREG_PAD_GPIO5_EN_N,1<<29);
        switch (port) {
        case SDIO_PORT_A://SDIOA,GPIOA_9~GPIOA_14
              //  SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_8,0x3f<<0);
              	 SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5,0x3<<13);
			     SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5,0x3<<10);
                break;
        case SDIO_PORT_B://SDIOB,GPIOA_0~GPIOA_5
                //diable SDIO_B1
                //CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_7,(0x3f<<24));
                SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_1,0x3f);
                break;
        case SDIO_PORT_C://SDIOC GPIOB_2~GPIOB_7
                SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_2,(0xf<<16)|(1<<12)|(1<<8));
                break;
        case SDIO_PORT_B1://SDIOB1 GPIOE_6~GPIOE_11
                // disable SPI and SDIO_B
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1,( (1<<29) | (1<<27) | (1<<25) | (1<<23)|0x3f));
                CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_6,0x7fff);
                SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_7,(0x3f<<24));
                break;
        default:
                return -1;
        }
        return 0;
}



#ifdef SD_MMC_HW_CONTROL
irqreturn_t
sdio_interrupt_monitor(int irq, void *dev_id)
{
        unsigned sdio_interrupt_resource = sdio_check_interrupt();
        switch (sdio_interrupt_resource) {
        case SDIO_IF_INT:
                //sdio_if_int_handle();
                break;

        case SDIO_CMD_INT:
                sdio_cmd_int_handle();
                break;

        case SDIO_TIMEOUT_INT:
                sdio_timeout_int_handle();
                break;


        case SDIO_NO_INT:
                break;

        default:
                break;
        }

        return IRQ_HANDLED;
        return 0;

}

#endif//#if SD_MMC_HW_CONTROL
#define PREG_SDIO_IRQ_CFG   CBUS_REG_ADDR(SDIO_IRQ_CONFIG)
void sdio_cfg_swth()
{
        unsigned long sdio_config =	0;
        SDIO_Config_Reg_t *config_reg;
        config_reg = (void *)&sdio_config;
        //sdio_config = READ_CBUS_REG(SDIO_CONFIG);
        sdio_config = ((2 << sdio_write_CRC_ok_status_bit) |
                       (2 << sdio_write_Nwr_bit) |
                       (3 << m_endian_bit) |//  3
                       (1<<bus_width_bit)|
                       (39 << cmd_argument_bits_bit) |
                       (1 << cmd_out_at_posedge_bit) |
                       (0 << cmd_disable_CRC_bit) |            
                       (1 << response_latch_at_negedge_bit) |
                       (2 << cmd_clk_divide_bit));
        config_reg->bus_width = 1;
        config_reg->cmd_clk_divide =FPGA_CLK_DIVIDE;
        WRITE_CBUS_REG(SDIO_CONFIG, sdio_config);
        WRITE_CBUS_REG( SDIO_MULT_CONFIG,(FPGA_SDIO_PORT & 0x3));	//Switch to	SDIO_A/B/C
        printk("sdio port %d \n",FPGA_SDIO_PORT);
        return;
}


void sd_mmc_io_config()
{
        sd_gpio_enable();

        sd_set_cmd_output();
        sd_set_cmd_value(1);
        sd_set_clk_output();
        sd_set_clk_high();
        sd_set_dat0_3_input();
}

static void sdio_pwr_on(unsigned port)
{
        //  CLEAR_CBUS_REG_MASK(PREG_JTAG_GPIO_O,(1<<16));
        // CLEAR_CBUS_REG_MASK(PREG_JTAG_GPIO_EN_N,(1<<20));//test_n
        /// @todo NOT FINISH
}

void Dump( void* data, unsigned int length )
{
        unsigned char* cursor = ( unsigned char* )data;
        unsigned int i = 0;

        for (i = 0;  i < length; i++ ) {
                if ( ( i & 0x0f ) == 0 ) {
                        printk("\n");
                }
                printk( "%02x ", *cursor++ );
        }
        printk("\n" );
}
/*

		case SDIO_GPIOA_0_5:
			MS_BS_OUTPUT_EN_REG = EGPIO_GPIOA_ENABLE;
			MS_BS_OUTPUT_EN_MASK = PREG_IO_9_MASK;
			MS_BS_OUTPUT_REG = EGPIO_GPIOA_OUTPUT;
			MS_BS_OUTPUT_MASK = PREG_IO_9_MASK;

			MS_CLK_OUTPUT_EN_REG = EGPIO_GPIOA_ENABLE;
			MS_CLK_OUTPUT_EN_MASK = PREG_IO_8_MASK;
			MS_CLK_OUTPUT_REG = EGPIO_GPIOA_OUTPUT;
			MS_CLK_OUTPUT_MASK = PREG_IO_8_MASK;

			MS_DAT_OUTPUT_EN_REG = EGPIO_GPIOA_ENABLE;
			MS_DAT0_OUTPUT_EN_MASK = PREG_IO_4_MASK;
			MS_DAT0_3_OUTPUT_EN_MASK = PREG_IO_4_7_MASK;
			MS_DAT_INPUT_REG = EGPIO_GPIOA_INPUT;
			MS_DAT_OUTPUT_REG = EGPIO_GPIOA_OUTPUT;
			MS_DAT0_INPUT_MASK = PREG_IO_4_MASK;
			MS_DAT0_OUTPUT_MASK = PREG_IO_4_MASK;
			MS_DAT0_3_INPUT_MASK = PREG_IO_4_7_MASK;
			MS_DAT0_3_OUTPUT_MASK = PREG_IO_4_7_MASK;
			MS_DAT_INPUT_OFFSET = 4;
			MS_DAT_OUTPUT_OFFSET = 4;
*/
int sdio_init()
{
        int ret = 0;
        unsigned char response[MAX_RESPONSE_BYTES];
        int i=0;
        unsigned char buffer[32];
        for ( i=0; i<32; i++)
                buffer[i] =i;
#if 0
        cpu_sdio_pwr_prepare(FPGA_SDIO_PORT);
        CLEAR_CBUS_REG_MASK(PREG_AGPIO_EN_N,(0x3f<<4));
        SET_CBUS_REG_MASK(PREG_AGPIO_O,(0x3f<<4));

        printk("fpga_sdio_init!!!\n");
        while (1) {
                SET_CBUS_REG_MASK(PREG_AGPIO_O,(0x3f<<4));
                //msleep(1);
                udelay(1);
                CLEAR_CBUS_REG_MASK(PREG_AGPIO_O,(0x3f<<4));
                udelay(1);
        }
#endif
        cpu_sdio_pwr_prepare(FPGA_SDIO_PORT);
        //sdio_pwr_on(FPGA_SDIO_PORT);
        printk("fpga_sdio_init!!!\n");
        cpu_sdio_init(FPGA_SDIO_PORT);
        sdio_cfg_swth();
        printk("sd_buffer_alloc!!!\n");
        sd_buffer_alloc();
#ifdef SDIO_IRQ
        if (request_irq(INT_SDIO, (irq_handler_t) sdio_interrupt_monitor, 0, "sd_mmc", (void *)NULL)) {
                printk("request SDIO irq error!!!\n");
                return -1;
        }
        printk("request SDIO irq ok!!!\n");
#endif//SDIO_IRQ
        printk("sd_send_cmd_hw SD_MMC_SEND_RELATIVE_ADDR!!!\n");
        ret = sd_send_cmd_hw(SD_MMC_SEND_RELATIVE_ADDR, 0xF0F0<<16, RESPONSE_R6, response, 0, 0, 0);   ///* Send out a byte to read RCA*/
        if (ret != SD_MMC_NO_ERROR) {
                printk("sd_send_cmd_hw error!!!\n");
        }

        printk("sd_send_cmd_hw SD_MMC_SELECT_DESELECT_CARD!!!\n");
        ret = sd_send_cmd_hw(SD_MMC_SELECT_DESELECT_CARD, 0xf0f0<<16, RESPONSE_R1B, response, 0, 0, 0);
        if (ret != SD_MMC_NO_ERROR) {
                printk("sd_send_cmd_hw error!!!\n");
        }

        return 0;

}


int sdio_exit()
{
        return 0;
}


