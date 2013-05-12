//#include "register.h" 
//#include "c_arc_pointer_reg.h" 
//#include "a9_func.h" 
//#include "clk_util.h" 
//#include "c_stimulus.h" 
//#include "a9_l2_func.h" 

#include "demod_func.h"
#include "aml_demod.h"
#include <linux/string.h>
#include <linux/kernel.h>
#include "../si2176/si2176_func.h"

void stimulus_finish_pass()
{


}

extern struct si2176_device_s *si2176_devp;
extern void si2176_set_frequency(unsigned int freq);

int demod_set_tuner(struct aml_demod_sta *demod_sta, 
		  struct aml_demod_i2c *demod_i2c, 
		  struct aml_tuner_sys *tuner_sys)
{
	unsigned int freq;
	int err_code;
	freq=tuner_sys->ch_freq;
	printk("freq is %d\n",freq);
	err_code = si2176_init(&si2176_devp->tuner_client,&si2176_devp->si_cmd_reply,&si2176_devp->si_common_reply);
        err_code = si2176_configure(&si2176_devp->tuner_client,&si2176_devp->si_prop,&si2176_devp->si_cmd_reply,&si2176_devp->si_common_reply);
        if(err_code)
        {
                printk("[si2176..]%s init si2176 error.\n",__func__);
                return err_code;
        }
		si2176_devp->si_prop.dtv_lif_out.amp = tuner_sys->amp;
		si2176_devp->si_prop.dtv_lif_freq.offset = tuner_sys->if_freq;
		printk("amp is %d,offset is %d,amp+offset is %x\n",si2176_devp->si_prop.dtv_lif_out.amp,si2176_devp->si_prop.dtv_lif_out.offset,((si2176_devp->si_prop.dtv_lif_out.offset)+((si2176_devp->si_prop.dtv_lif_out.amp)<<8)));
		if(si2176_set_property(&si2176_devp->tuner_client,0,0x707,(si2176_devp->si_prop.dtv_lif_out.offset)+((si2176_devp->si_prop.dtv_lif_out.amp)<<8),&si2176_devp->si_cmd_reply))
					printk("[si2176..]%s set dtv lif out amp error.\n",__func__);
		printk("if is %d\n",si2176_devp->si_prop.dtv_lif_freq.offset);
		if(si2176_set_property(&si2176_devp->tuner_client,0,0x706,si2176_devp->si_prop.dtv_lif_freq.offset,&si2176_devp->si_cmd_reply))
					printk("[si2176..]%s set dtv lif out if error.\n",__func__);
		
		si2176_set_frequency(freq);
		demod_sta->ch_if=tuner_sys->if_freq;
}


void demod_set_adc_core_clk(int clk_adc, int clk_dem)//dvbt dvbc 28571, 66666
{
	  struct aml_demod_reg  reg;
//    volatile unsigned long *demod_dig_clk = 0xf11041d0;
//    volatile unsigned long *demod_adc_clk = 0xf11042a8;
    int unit, error;
    demod_dig_clk_t dig_clk_cfg;
    demod_adc_clk_t adc_clk_cfg;
    int pll_m, pll_n, pll_od, div_dem, div_adc;
    int freq_osc, freq_vco, freq_out, freq_dem, freq_adc;
    int freq_dem_act, freq_adc_act, err_tmp, best_err;
    int tmp;
    
    unit = 10000; // 10000 as 1 MHz, 0.1 kHz resolution.
    freq_osc = 24*unit;
    adc_clk_cfg.d32 = 0;
    dig_clk_cfg.d32 = 0;

    if (clk_adc > 0) {
        adc_clk_cfg.b.reset = 0;
        adc_clk_cfg.b.pll_pd = 0;
        if (clk_adc < 1000) 
            freq_adc = clk_adc*unit;
        else
            freq_adc = clk_adc*unit/1000;
    }
    else {
        adc_clk_cfg.b.pll_pd = 1;
    }

    if (clk_dem > 0) {
        dig_clk_cfg.b.demod_clk_en = 1;
        dig_clk_cfg.b.demod_clk_sel = 3;        
        if (clk_dem < 1000) 
            freq_dem = clk_dem*unit;
        else
            freq_dem = clk_dem*unit/1000;
    }
    else {
        dig_clk_cfg.b.demod_clk_en = 0;
    }

    error = 1;
    best_err = 100*unit;
    for (pll_m=1; pll_m<512; pll_m++) {
        for (pll_n=1; pll_n<=5; pll_n++) { 
            freq_vco = freq_osc * pll_m / pll_n;
            if (freq_vco<750*unit || freq_vco>1500*unit) continue;

            for (pll_od=0; pll_od<3; pll_od++) {
                freq_out = freq_vco / (1<<pll_od);
                if (freq_out > 800*unit) continue;

                div_dem = freq_out / freq_dem;
                if (div_dem==0 || div_dem>127) continue;
                
                freq_dem_act = freq_out / div_dem;
                err_tmp = freq_dem_act - freq_dem;
                        
                div_adc = freq_out / freq_adc / 2;
                div_adc *= 2;
                if (div_adc==0 || div_adc>31) continue;

                freq_adc_act = freq_out / div_adc;
                if (freq_adc_act-freq_adc > unit/5) continue;
                
                if (err_tmp >= best_err) continue;
                
                adc_clk_cfg.b.pll_m = pll_m;
                adc_clk_cfg.b.pll_n = pll_n;
                adc_clk_cfg.b.pll_od = pll_od;
                adc_clk_cfg.b.pll_xd = div_adc-1;
                dig_clk_cfg.b.demod_clk_div = div_dem - 1;
                
                error = 0;
                best_err = err_tmp;
            } 
        }
    }
    
    pll_m = adc_clk_cfg.b.pll_m;
    pll_n = adc_clk_cfg.b.pll_n;
    pll_od = adc_clk_cfg.b.pll_od;
    div_adc = adc_clk_cfg.b.pll_xd + 1;
    div_dem = dig_clk_cfg.b.demod_clk_div + 1;
    
    if (error) {
        printk(" ERROR DIG %7d kHz  ADC %7d kHz\n",freq_dem/(unit/1000), freq_adc/(unit/1000));
    }
    else {
      /*  printf("dig_clk_cfg %x \n",dig_clk_cfg.d32);
        printf("adc_clk_cfg %x \n",adc_clk_cfg.d32);
        printf("Whether write configure to IC?");
        scanf("%d", &tmp);
        if (tmp == 1)
        {
        reg.addr = 0xf11041d0; reg.val = dig_clk_cfg.d32;ioctl(fd, AML_DEMOD_SET_REG, &reg);
        reg.addr = 0xf11042a8; reg.val = adc_clk_cfg.d32;ioctl(fd, AML_DEMOD_SET_REG, &reg);
        }*/
		__raw_writel(dig_clk_cfg.d32,CBUS_REG_ADDR(0x1074));
		__raw_writel(adc_clk_cfg.d32,CBUS_REG_ADDR(0x10aa));

        printk("dig_clk_cfg.d32 is %x,adc_clk_cfg.d32 is %x\n",dig_clk_cfg.d32,adc_clk_cfg.d32);
        freq_vco = freq_osc * pll_m / pll_n;    
        freq_out = freq_vco / (1<<pll_od);
        freq_dem_act = freq_out / div_dem;
        freq_adc_act = freq_out / div_adc;
        
        printk(" ADC PLL  M %3d   N %3d\n", pll_m, pll_n);
        printk(" ADC PLL OD %3d  XD %3d\n", pll_od, div_adc);
        printk(" DIG SRC SEL %2d  DIV %2d\n", 3, div_dem);
        printk(" DIG %7d kHz %7d kHz\n", freq_dem/(unit/1000), freq_dem_act/(unit/1000));
        printk(" ADC %7d kHz %7d kHz\n", freq_adc/(unit/1000), freq_adc_act/(unit/1000));
    }
}


void clocks_set_sys_defaults(unsigned char dvb_mode)
{
	__raw_writel(0x003b0232,CBUS_REG_ADDR(0x10aa));
	__raw_writel(0x814d3928,CBUS_REG_ADDR(0x10ab));
	__raw_writel(0x6b425012,CBUS_REG_ADDR(0x10ac));
	__raw_writel(0x101,CBUS_REG_ADDR(0x10ad));
	__raw_writel(0x70b,CBUS_REG_ADDR(0x1073));
	__raw_writel(0x713,CBUS_REG_ADDR(0x1074));
	__raw_writel(0x201,(DEMOD_BASE+0xc08));
	__raw_writel(0x00001007,(DEMOD_BASE+0xc00));
	__raw_writel(0x2e805400,(DEMOD_BASE+0xc04));
	printk("0xc8020c08 is %x\n",__raw_readl((volatile unsigned long *)(DEMOD_BASE+0xc08)));
	printk("0xc8020c04 is %x\n",__raw_readl((volatile unsigned long *)(DEMOD_BASE+0xc04)));	
	if(dvb_mode==0)    //// 0 -DVBC, 1-DVBT, ISDBT, 2-ATSC
	{
		__raw_writel(0x00001027,(DEMOD_BASE+0xc00));
	}else if(dvb_mode==1){
		__raw_writel(0x00001017,(DEMOD_BASE+0xc00));
	}else if(dvb_mode==2){
		__raw_writel(0x00001047,(DEMOD_BASE+0xc00));
	}
		printk("0xc8020c00 is %x,dvb_mode is %d\n",__raw_readl((volatile unsigned long *)(DEMOD_BASE+0xc00)),dvb_mode);		
	
int i;
int addr;

for(i=0;i<0x200;i+=4){
	addr=DEMOD_BASE+i;
//	printk("[0x%x]rd  is 0x%x\n",addr,*(volatile unsigned long *)(addr));
}







}

void atsc_write_reg(int reg_addr, int reg_data)
{
    apb_write_reg(ATSC_BASE, (reg_addr&0xffff)<<8 | (reg_data&0xff));
}

unsigned long atsc_read_reg(int reg_addr)
{
    unsigned long tmp;

    apb_write_reg(ATSC_BASE+4, (reg_addr&0xffff)<<8);
    tmp = apb_read_reg(ATSC_BASE);

    return tmp&0xff;
}


void atsc_initial(struct aml_demod_sta *demod_sta)
{
    int i,fc,fs,cr,ck;
    
    atsc_cfg_t list[21] = {
    {0x0733, 0x00, 0},
    {0x0734, 0xff, 0},
    {0x0716, 0x02, 0},		//F06[7] invert spectrum  0x02 0x06
    {0x05e7, 0x00, 0},
    {0x05e8, 0x00, 0},
    {0x0f06, 0x80, 0},
    {0x0f09, 0x04, 0},
    {0x070c, 0x18, 0},
    {0x070d, 0x9d, 0},
    {0x070e, 0x89, 0},
    {0x070f, 0x6a, 0},
    {0x0710, 0x75, 0},
    {0x0711, 0x6f, 0},
    {0x072a, 0x02, 0},
    {0x072c, 0x02, 0},
    {0x090d, 0x03, 0},
    {0x090e, 0x02, 0},
    {0x090f, 0x00, 0},
    {0x0900, 0x01, 0},
    {0x0900, 0x00, 0},
    {0x0f00, 0x01, 0},
    {0x0000, 0x00, 1}};
    	    
    for (i=0; list[i].adr != 0; i++) {
        if (list[i].rw) 
            atsc_read_reg(list[i].adr);
        else
            atsc_write_reg(list[i].adr, list[i].dat);
    }
	
	fs = demod_sta->sts;//KHZ 26000
	fc = demod_sta->ch_if;//KHZ 5000

	cr = (fc*(1<<17)/fs)*(1<<6) ;
	ck = fs*15589/10-(1<<25);

	
	atsc_write_reg(0x070e,cr&0xff);
	atsc_write_reg(0x070d,(cr>>8)&0xff);
	atsc_write_reg(0x070c,(cr>>16)&0xff);

	atsc_write_reg(0x0711,ck&0xff);
	atsc_write_reg(0x0710,(ck>>8)&0xff);
	atsc_write_reg(0x070f,(ck>>16)&0xff);


	printk("fs is %d(SR),fc is %d(IF),cr is %x,ck is %x\n",fs,fc,cr,ck);
}



int atsc_set_ch(struct aml_demod_sta *demod_sta, 
			struct aml_demod_i2c *demod_i2c, 
			struct aml_demod_atsc *demod_atsc)
{
	int ret = 0;
	u8 demod_mode;
	u8 bw, sr, ifreq, agc_mode;
	u32 ch_freq;
	bw		 = demod_atsc->bw;
	sr		 = demod_atsc->sr;
	ifreq	 = demod_atsc->ifreq;
	agc_mode = demod_atsc->agc_mode;
	ch_freq  = demod_atsc->ch_freq;
	demod_mode = demod_atsc->dat0;
	demod_i2c->tuner = 7;
	// Set registers
	//////////////////////////////////////
	// bw == 0 : 8M 
	//		 1 : 7M
	//		 2 : 6M
	//		 3 : 5M
	// sr == 0 : 20.7M
	//		 1 : 20.8333M
	//		 2 : 28.5714M
	//		 3 : 45M
	// agc_mode == 0: single AGC
	//			   1: dual AGC
	//////////////////////////////////////
	if (bw > 3) {
		printk("Error: Invalid Bandwidth option %d\n", bw);
	bw = 0;
	ret = -1;
	}

	if (sr > 3) {
		printk("Error: Invalid Sampling Freq option %d\n", sr);
	sr = 2;
//	ret = -1;
	}

	if (ifreq > 1) {
		printk("Error: Invalid IFreq option %d\n", ifreq);
	ifreq = 0;
	ret = -1;
	}

	if (agc_mode > 3) {
		printk("Error: Invalid AGC mode option %d\n", agc_mode);
	agc_mode = 0;
	ret = -1;
	}
	
	if (ch_freq<1000 || ch_freq>900000000) {
	printk("Error: Invalid Channel Freq option %d\n", ch_freq);
	ch_freq = 474000;
	ret = -1;
	}

	if (demod_mode<0||demod_mode>4) {
		printk("Error: Invalid demod mode option %d\n", demod_mode);
		printk("Note: 0 is QAM, 1 is DVBT , 2 is ISDBT, 3 is DTMB, 4 is ATSC\n");
		demod_mode = 1;
		ret = -1;
	}

	//demod_sta->dvb_mode  = 1;
	demod_sta->ch_mode	 = 0; // TODO
	demod_sta->agc_mode  = agc_mode;
	demod_sta->ch_freq	 = ch_freq;
	demod_sta->dvb_mode  = demod_mode;
	if (demod_i2c->tuner == 1) 
	demod_sta->ch_if = 36130; 
	else if (demod_i2c->tuner == 2) 
	demod_sta->ch_if = 4570; 
	else if (demod_i2c->tuner == 3)
	demod_sta->ch_if = 4000;// It is nouse.(alan)
	else if (demod_i2c->tuner == 7)
	demod_sta->ch_if = 5000;//silab 5000kHz IF

		
	demod_sta->ch_bw	 = (8-bw)*1000; 
	demod_sta->sts=26000;
	// Set Tuner
//	tuner_set_ch(demod_sta, demod_i2c);
  	atsc_initial(demod_sta);
	printk("ATSC mode\n");
	

	return ret;
}



/*int dvbc_set_ch(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c, 
		struct aml_demod_dvbc *demod_dvbc)
{
    int ret = 0;
    u16 symb_rate;
    u8  mode;
    u32 ch_freq;

   printk("f=%d, s=%d, q=%d\n", demod_dvbc->ch_freq, demod_dvbc->symb_rate, demod_dvbc->mode);
   
    mode      = demod_dvbc->mode;
    symb_rate = demod_dvbc->symb_rate;
    ch_freq   = demod_dvbc->ch_freq;
	demod_i2c->tuner=7;
    if (mode > 4) {
	printk("Error: Invalid QAM mode option %d\n", mode);
	mode = 2;
       	ret = -1;
    }

    if (symb_rate<1000 || symb_rate>7000) {
	printk("Error: Invalid Symbol Rate option %d\n", symb_rate);
	symb_rate = 6875;
	ret = -1;
    }

    if (ch_freq<1000 || ch_freq>900000) {
	printk("Error: Invalid Channel Freq option %d\n", ch_freq);
	ch_freq = 474000;
	ret = -1;
    }

    // if (ret != 0) return ret;

    // Set DVB-C
  //  (*DEMOD_REG0) &= ~1;

    demod_sta->dvb_mode  = 0;    // 0:dvb-c, 1:dvb-t
    demod_sta->ch_mode   = mode; // 0:16, 1:32, 2:64, 3:128, 4:256
    demod_sta->agc_mode  = 1;    // 0:NULL, 1:IF, 2:RF, 3:both
    demod_sta->ch_freq   = ch_freq;
    demod_sta->tuner     = demod_i2c->tuner;

    if(demod_i2c->tuner == 1)
        demod_sta->ch_if     = 36130; // TODO  DCT tuner
    else if (demod_i2c->tuner == 2)
        demod_sta->ch_if     = 4570; // TODO  Maxlinear tuner
    else if (demod_i2c->tuner == 7)
        demod_sta->ch_if     = 5000; // TODO  Si2176 tuner
            
    demod_sta->ch_bw     = 8000;  // TODO
    demod_sta->symb_rate = symb_rate;  
    
    // Set Tuner
    tuner_set_ch(demod_sta, demod_i2c);
    
 //   mdelay((demod_sta->ch_freq % 10) * 1000);
	qam_initial(mode);
  //  dvbc_reg_initial(demod_sta);
    
    return ret;
}  */



int dvbt_set_ch(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c, 
		struct aml_demod_dvbt *demod_dvbt)
{
    int ret = 0;
    u8_t demod_mode = 1;
    u8_t bw, sr, ifreq, agc_mode;
    u32_t ch_freq;

    bw       = demod_dvbt->bw;
    sr       = demod_dvbt->sr;
    ifreq    = demod_dvbt->ifreq;
    agc_mode = demod_dvbt->agc_mode;
    ch_freq  = demod_dvbt->ch_freq;
    demod_mode = demod_dvbt->dat0; 
    if (ch_freq<1000 || ch_freq>900000000) {
	//printk("Error: Invalid Channel Freq option %d\n", ch_freq);
	ch_freq = 474000;
	ret = -1;
    }

    if (demod_mode<0||demod_mode>4) {
    	//printk("Error: Invalid demod mode option %d\n", demod_mode);
    	//printk("Note: 0 is QAM, 1 is DVBT , 2 is ISDBT, 3 is DTMB, 4 is ATSC\n");
    	demod_mode = 1;
    	ret = -1;
    }

    //demod_sta->dvb_mode  = 1;
    demod_sta->ch_mode   = 0; // TODO
    demod_sta->agc_mode  = agc_mode;
    demod_sta->ch_freq   = ch_freq;
    demod_sta->dvb_mode  = demod_mode;
 /*   if (demod_i2c->tuner == 1) 
	demod_sta->ch_if = 36130; 
    else if (demod_i2c->tuner == 2) 
	demod_sta->ch_if = 4570; 
    else if (demod_i2c->tuner == 3)
	demod_sta->ch_if = 4000;// It is nouse.(alan)
    else if (demod_i2c->tuner == 7)
	demod_sta->ch_if = 5000;//silab 5000kHz IF*/

        
    demod_sta->ch_bw     = (8-bw)*1000; 
    demod_sta->symb_rate = 0; // TODO

    //bw=0;sr=3;ifreq=4;demod_mode=2;
    //for si2176 IF:5M   sr 28.57  
    sr=3;ifreq=4;
    ofdm_initial(
  		  bw            ,// 00:8M 01:7M 10:6M 11:5M
     		sr            ,// 00:45M 01:20.8333M 10:20.7M 11:28.57 
     		ifreq         ,// 000:36.13M 001:-5.5M 010:4.57M 011:4M 100:5M
     		demod_mode-1    ,// 00:DVBT,01:ISDBT
     		1        // 0: Unsigned, 1:TC
    	) ;
    printk("DVBT/ISDBT mode\n");


    return ret;
}

static void demod_get_agc(struct aml_demod_sys *demod_sys)
{
    //u8_t agc_sel = 0;
    //u32_t tmp;
    //
    //tmp = (*P_PERIPHS_PIN_MUX_6);
    //if ((tmp>>30&3) == 3) agc_sel = 1;
    //
    //tmp = (*P_PERIPHS_PIN_MUX_2);
    //if ((tmp>>28&3) == 3) agc_sel = 2;
    //
    //demod_sys->agc_sel = agc_sel;
}

static void demod_get_adc(struct aml_demod_sys *demod_sys)
{
    //u8_t adc_en;
    //u32_t tmp;
    //
    //tmp = (*DEMOD_REG1);
    //adc_en = tmp>>31&1;
    //
    //demod_sys->adc_en = adc_en;
}

static void printk_sys(struct aml_demod_sys *demod_sys)
{
    //printk("sys.clk_en     %d\n", demod_sys->clk_en);
    //printk("sys.clk_src    %d\n", demod_sys->clk_src);       
    //printk("sys.clk_div    %d\n", demod_sys->clk_div);       
    //printk("sys.pll_n      %d\n", demod_sys->pll_n);     
    //printk("sys.pll_m      %d\n", demod_sys->pll_m);     
    //printk("sys.pll_od     %d\n", demod_sys->pll_od);
    //printk("sys.pll_sys_xd %d\n", demod_sys->pll_sys_xd);
    //printk("sys.pll_adc_xd %d\n", demod_sys->pll_adc_xd);
    //printk("sys.agc_sel    %d\n", demod_sys->agc_sel);
    //printk("sys.adc_en     %d\n", demod_sys->adc_en);
}

static void demod_get_clk(struct aml_demod_sys *demod_sys)
{
    u8_t clk_en, clk_src, clk_div, pll_n, pll_od, pll_sys_xd, pll_adc_xd;
    u16_t pll_m;
    u32_t tmp;
    
    tmp = (*P_HHI_DEMOD_CLK_CNTL);
    clk_src = tmp>>9&3;
    clk_en  = tmp>>8&1;
    clk_div = tmp&0x7f;

    tmp = (*P_HHI_DEMOD_PLL_CNTL);
    pll_n = tmp>>9&0x1f;
    pll_m = tmp&0x1ff;
    pll_od = tmp>>16&0x7f;

    tmp = (*P_HHI_DEMOD_PLL_CNTL2);
    pll_adc_xd = tmp>>21&0x1f;

    tmp = (*P_HHI_DEMOD_PLL_CNTL3);
    pll_sys_xd = tmp>>16&0x1f;

    demod_sys->clk_en     = clk_en;
    demod_sys->clk_src    = clk_src;
    demod_sys->clk_div    = clk_div;
    demod_sys->pll_n      = pll_n;
    demod_sys->pll_m      = pll_m;
    demod_sys->pll_od     = pll_od;
    demod_sys->pll_sys_xd = pll_sys_xd;
    demod_sys->pll_adc_xd = pll_adc_xd;
}

int demod_get_sys(struct aml_demod_i2c *demod_i2c, 
		  struct aml_demod_sys *demod_sys)
{
    *(struct aml_demod_i2c *)demod_sys->i2c = *demod_i2c;
    demod_get_clk(demod_sys);
    demod_get_adc(demod_sys);
    demod_get_agc(demod_sys);
    
    printk_sys(demod_sys);

    return 0;
}

int demod_set_sys(struct aml_demod_sta *demod_sta, 
		  struct aml_demod_i2c *demod_i2c, 
		  struct aml_demod_sys *demod_sys)
{
    int i;
    unsigned tmp;
    unsigned char dvb_mode;
    demod_cfg0_t cfg0;
    demod_cfg1_t cfg1;
    demod_cfg2_t cfg2;
    demod_cfg_regs_t *demod_cfg_regs;
    int clk_adc,clk_dem;
    dvb_mode=demod_sta->dvb_mode;
    int adc_clk_cfg = 0x001a021e;
    int dig_clk_cfg = 0x0000070b;
  /*  if((dvb_mode==0) ||(dvb_mode==1)){   		//// 0 -DVBC, 1-DVBT, ISDBT, 2-ATSC
	    clk_adc=35000;//28571;
	    clk_dem=70000;//66666;
    }else if (dvb_mode==2)
    {
	 clk_adc=26000;
   	 clk_dem=3*clk_adc;
	}*/
    clk_adc=demod_sys->adc_clk;
	clk_dem=demod_sys->demod_clk;
	printk("demod_set_sys,clk_adc is %d,clk_demod is %d\n",clk_adc,clk_dem);
    clocks_set_sys_defaults(dvb_mode);
    demod_set_adc_core_clk(clk_adc, clk_dem);
	demod_sta->adc_freq=clk_adc;
	demod_sta->clk_freq=clk_dem;
	

/*    demod_set_adc_core_clk_quick(adc_clk_cfg, dig_clk_cfg);
printk("demod_set_sys 1\n");
    Wr(RESET0_REGISTER, (1<<8));
printk("demod_set_sys 2\n");
    cfg2.b.en_adc = 1;
    demod_cfg_regs->cfg2 = cfg2.d32;

    cfg0.b.mode = (1<<1);
    cfg0.b.ts_sel = (1<<1);
    cfg0.b.adc_ext = 0;
    cfg0.b.adc_regout = 0;
    cfg0.b.adc_regadj = 1;
    demod_cfg_regs->cfg0 = cfg0.d32;

    // test out 
    (*(volatile unsigned long *)(P_PERIPHS_PIN_MUX_8)) |= (1<<11); 
    // agc out
    (*(volatile unsigned long *)(P_PERIPHS_PIN_MUX_9)) |= (3<<0);
printk("demod_set_sys 3\n");
    delay_us(15);
    // external adc
    cfg0.b.adc_ext = 1;
    demod_cfg_regs->cfg0 = cfg0.d32;

    // disable test out
    (*(volatile unsigned long *)(P_PERIPHS_PIN_MUX_8)) &= ~(1<<11); 
    // enable adc in
    (*(volatile unsigned long *)(P_PERIPHS_PIN_MUX_8)) |= (1<<10);
printk("demod_set_sys  4\n");
    delay_us(15);
    //stimulus_finish_pass();*/
    return 0;
}

void demod_set_reg(struct aml_demod_reg *demod_reg)
{
	 if(demod_reg->mode==0){
		demod_reg->addr=demod_reg->addr+QAM_BASE;
	}else if((demod_reg->mode==1)||((demod_reg->mode==2))){
		demod_reg->addr=demod_reg->addr*4+DVBT_BASE;
	}else if(demod_reg->mode==3){
	//	demod_reg->addr=ATSC_BASE;
	}
	else if(demod_reg->mode==4){
		demod_reg->addr=demod_reg->addr*4+DEMOD_CFG_BASE;
	}else if(demod_reg->mode==5){
	//	printk("DEMOD_BASE    	0xf1100000+addr*4 \n ISDBT_BASE 	  	0xf1100000+addr*4\n QAM_BASE	  	0xf1100400+addr\n DEMOD_CFG_BASE	0xf1100c00+addr\n");
	}
	if(demod_reg->mode==3)
		atsc_write_reg(demod_reg->addr, demod_reg->val);
	//	apb_write_reg(demod_reg->addr, (demod_reg->val&0xffff)<<8 | (demod_reg->val&0xff));
	else
   		apb_write_reg(demod_reg->addr, demod_reg->val);
}


void demod_get_reg(struct aml_demod_reg *demod_reg)
{
    if(demod_reg->mode==0){
		demod_reg->addr=demod_reg->addr+QAM_BASE;
	}else if((demod_reg->mode==1)||(demod_reg->mode==2)){
		demod_reg->addr=demod_reg->addr*4+DVBT_BASE;
	}else if(demod_reg->mode==3){
	//	demod_reg->addr=demod_reg->addr+ATSC_BASE;
	}else if(demod_reg->mode==4){
		demod_reg->addr=demod_reg->addr*4+DEMOD_CFG_BASE;
	}else if(demod_reg->mode==5){
	//	printk("DEMOD_BASE    	0xf1100000+addr*4 \n ISDBT_BASE 	  	0xf1100000+addr*4\n QAM_BASE	  	0xf1100400+addr\n DEMOD_CFG_BASE	0xf1100c00+addr\n");
	}

	if(demod_reg->mode==3){
		demod_reg->val=atsc_read_reg(demod_reg->addr);
	//	apb_write_reg(ATSC_BASE+4, (demod_reg->addr&0xffff)<<8);
	//	demod_reg->val = apb_read_reg(ATSC_BASE)&0xff;
	}
	else
   		demod_reg->val = apb_read_reg(demod_reg->addr);
}

void delay_us(int us) 
{
    Wr( ISA_TIMERE, 0); while( Rd(ISA_TIMERE) < us ) {}
}

void demod_reset()
{
    Wr(RESET0_REGISTER, (1<<8));
}

void demod_set_irq_mask()
{
 (*(volatile unsigned long *)(P_SYS_CPU_0_IRQ_IN4_INTR_MASK)) |= (1 << 8);    
}

void demod_clr_irq_stat()
{
   (*(volatile unsigned long *)(P_SYS_CPU_0_IRQ_IN4_INTR_STAT)) |= (1 << 8);    
}

void demod_set_adc_core_clk_quick(int clk_adc_cfg, int clk_dem_cfg)
{
    volatile unsigned long *demod_dig_clk = P_HHI_DEMOD_CLK_CNTL;
    volatile unsigned long *demod_adc_clk = P_HHI_ADC_PLL_CNTL;
    int unit;
    demod_dig_clk_t dig_clk_cfg;
    demod_adc_clk_t adc_clk_cfg;
    int pll_m, pll_n, pll_od, div_dem, div_adc;
    int freq_osc, freq_vco, freq_out, freq_dem_act, freq_adc_act;

    unit = 10000; // 10000 as 1 MHz, 0.1 kHz resolution.
    freq_osc = 25*unit;

    adc_clk_cfg.d32 = clk_adc_cfg;
    dig_clk_cfg.d32 = clk_dem_cfg;

    pll_m = adc_clk_cfg.b.pll_m;
    pll_n = adc_clk_cfg.b.pll_n;
    pll_od = adc_clk_cfg.b.pll_od;
    div_adc = adc_clk_cfg.b.pll_xd;
    div_dem = dig_clk_cfg.b.demod_clk_div + 1;
    
    *demod_dig_clk = dig_clk_cfg.d32;
    *demod_adc_clk = adc_clk_cfg.d32;
        
    freq_vco = freq_osc * pll_m / pll_n;    
    freq_out = freq_vco / (1<<pll_od);
    freq_dem_act = freq_out / div_dem;
    freq_adc_act = freq_out / div_adc;
        
    //printk(" ADC PLL  M %3d   N %3d\n", pll_m, pll_n);
    //printk(" ADC PLL OD %3d  XD %3d\n", pll_od, div_adc);
    //printk(" DIG SRC SEL %2d  DIV %2d\n", 3, div_dem);
    //printk(" DIG %7d kHz ADC %7d kHz\n", 
               //       freq_dem_act/(unit/1000), freq_adc_act/(unit/1000));
}

/*void demod_set_adc_core_clk(int clk_adc, int clk_dem)
{
    volatile unsigned long *demod_dig_clk = P_HHI_DEMOD_CLK_CNTL;
    volatile unsigned long *demod_adc_clk = P_HHI_ADC_PLL_CNTL;
    int unit, error;
    demod_dig_clk_t dig_clk_cfg;
    demod_adc_clk_t adc_clk_cfg;
    int pll_m, pll_n, pll_od, div_dem, div_adc;
    int freq_osc, freq_vco, freq_out, freq_dem, freq_adc;
    int freq_dem_act, freq_adc_act, err_tmp, best_err;
    
    unit = 10000; // 10000 as 1 MHz, 0.1 kHz resolution.
    freq_osc = 25*unit;
    adc_clk_cfg.d32 = 0;
    dig_clk_cfg.d32 = 0;

    if (clk_adc > 0) {
        adc_clk_cfg.b.reset = 0;
        adc_clk_cfg.b.pll_pd = 0;
        if (clk_adc < 1000) 
            freq_adc = clk_adc*unit;
        else
            freq_adc = clk_adc*unit/1000;
    }
    else {
        adc_clk_cfg.b.pll_pd = 1;
    }

    if (clk_dem > 0) {
        dig_clk_cfg.b.demod_clk_en = 1;
        dig_clk_cfg.b.demod_clk_sel = 3;        
        if (clk_dem < 1000) 
            freq_dem = clk_dem*unit;
        else
            freq_dem = clk_dem*unit/1000;
    }
    else {
        dig_clk_cfg.b.demod_clk_en = 0;
    }

    error = 1;
    best_err = 100*unit;
    for (pll_m=1; pll_m<512; pll_m++) {
        for (pll_n=1; pll_n<=5; pll_n++) { 
            freq_vco = freq_osc * pll_m / pll_n;
            if (freq_vco<750*unit || freq_vco>1500*unit) continue;

            for (pll_od=0; pll_od<3; pll_od++) {
                freq_out = freq_vco / (1<<pll_od);
                if (freq_out > 800*unit) continue;

                div_dem = freq_out / freq_dem;
                if (div_dem==0 || div_dem>127) continue;
                
                freq_dem_act = freq_out / div_dem;
                err_tmp = freq_dem_act - freq_dem;
                        
                div_adc = freq_out / freq_adc / 2;
                div_adc *= 2;
                if (div_adc==0 || div_adc>31) continue;

                freq_adc_act = freq_out / div_adc;
                if (freq_adc_act-freq_adc > unit/5) continue;
                
                if (err_tmp >= best_err) continue;
                
                adc_clk_cfg.b.pll_m = pll_m;
                adc_clk_cfg.b.pll_n = pll_n;
                adc_clk_cfg.b.pll_od = pll_od;
                adc_clk_cfg.b.pll_xd = div_adc;
                dig_clk_cfg.b.demod_clk_div = div_dem - 1;
                
                error = 0;
                best_err = err_tmp;
            } 
        }
    }
    
    pll_m = adc_clk_cfg.b.pll_m;
    pll_n = adc_clk_cfg.b.pll_n;
    pll_od = adc_clk_cfg.b.pll_od;
    div_adc = adc_clk_cfg.b.pll_xd;
    div_dem = dig_clk_cfg.b.demod_clk_div + 1;
    
    if (error) {
        //printk(" ERROR DIG %7d kHz  ADC %7d kHz\n", 
                    //      freq_dem/(unit/1000), freq_adc/(unit/1000));
    }
    else {
        *demod_dig_clk = dig_clk_cfg.d32;
        *demod_adc_clk = adc_clk_cfg.d32;
        
        freq_vco = freq_osc * pll_m / pll_n;    
        freq_out = freq_vco / (1<<pll_od);
        freq_dem_act = freq_out / div_dem;
        freq_adc_act = freq_out / div_adc;
        
        //printk(" ADC PLL  M %3d   N %3d\n", pll_m, pll_n);
        //printk(" ADC PLL OD %3d  XD %3d\n", pll_od, div_adc);
        //printk(" DIG SRC SEL %2d  DIV %2d\n", 3, div_dem);
        //printk(" DIG %7d kHz %7d kHz\n", 
//                          freq_dem/(unit/1000), freq_dem_act/(unit/1000));
        //printk(" ADC %7d kHz %7d kHz\n", 
                  //        freq_adc/(unit/1000), freq_adc_act/(unit/1000));
    }
}*/

void apb_write_reg(int addr, int data)
{
    *(volatile unsigned long*)addr = data;
//	printk("[write]addr is %x,data is %x\n",addr,data);
}

unsigned long apb_read_reg(int addr)
{
    unsigned long tmp;

    tmp = *(volatile unsigned long*)addr;
//	printk("[read]addr is %x,data is %x\n",addr,tmp);
    return tmp;
}

void apb_write_regb(int addr, int index, int data)
{
    unsigned long tmp;

    tmp = *(volatile unsigned *)addr;
    tmp &= ~(1<<index);
    tmp |= (data<<index);
    *(volatile unsigned *)addr = tmp;
}

void enable_qam_int(int idx) 
{
    unsigned long mask;

    mask = apb_read_reg(QAM_BASE+0xd0);
    mask |= (1<<idx);
    apb_write_reg(QAM_BASE+0xd0, mask);
}

void disable_qam_int(int idx) 
{
    unsigned long mask;

    mask = apb_read_reg(QAM_BASE+0xd0);
    mask &= ~(1<<idx);
    apb_write_reg(QAM_BASE+0xd0, mask);
}

char *qam_int_name[] = {"      ADC", 
			"   Symbol", 
			"       RS", 
			" In_Sync0", 
			" In_Sync1", 
			" In_Sync2", 
			" In_Sync3", 
			" In_Sync4", 
			"Out_Sync0", 
			"Out_Sync1", 
			"Out_Sync2", 
			"Out_Sync3", 
			"Out_Sync4", 
			"In_SyncCo", 
			"OutSyncCo", 
			"  In_Dagc", 
			" Out_Dagc", 
			"  Eq_Mode",
			"RS_Uncorr"};

unsigned long read_qam_int()
{
    unsigned long stat, mask, tmp;
    int idx;
    char buf[80];
    static int rs_total;

    stat = apb_read_reg(QAM_BASE+0xd4);
    mask = apb_read_reg(QAM_BASE+0xd0);
    stat = stat & mask;

    for (idx=0; idx<20; idx++) {
	if (stat>>idx&1) { 
            printk("QAM Irq %2d %s %08x\n", idx, qam_int_name[idx], stat);

	    if (idx == 2) {
		rs_total += 100;
		if (rs_total == 900)
		    stimulus_finish_pass();
	    }

	    if (idx == 7) { // in_sync4
		tmp = apb_read_reg(QAM_BASE+0x10);
		tmp = (tmp&0xffff0000) + 100;
		apb_write_reg(QAM_BASE+0x10, tmp);
		rs_total = 0;
		enable_qam_int(2);
		enable_qam_int(18);
	    }

	    if (idx == 18) {
		rs_total = 0;
	    }
	}
    }
    return stat;
}
#define OFDM_INT_STS         (volatile unsigned long *)(DVBT_BASE+4*0x0d)
#define OFDM_INT_EN          (volatile unsigned long *)(DVBT_BASE+4*0x0e)

void enable_ofdm_int(int ofdm_irq) 
{
    // clear ofdm/xxx status
    (*OFDM_INT_STS) &= ~(1<<ofdm_irq); 
    // enable ofdm/xxx irq
    (*OFDM_INT_EN) |= (1<<ofdm_irq);
}

void disable_ofdm_int(int ofdm_irq) 
{
    unsigned long tmp;

    // disable ofdm/xxx irq
    tmp = (*OFDM_INT_EN);
    tmp &= ~(1<<ofdm_irq);
    (*OFDM_INT_EN) = tmp;
    // clear ofdm/xxx status
    (*OFDM_INT_STS) &= ~(1<<ofdm_irq); 
}

char *ofdm_int_name[] = {"PFS_FCFO", 
			 "PFS_ICFO", 
			 " CS_FCFO", 
			 " PFS_SFO", 
			 " PFS_TPS", 
			 "      SP", 
			 "     CCI", 
			 "  Symbol", 
			 " In_Sync", 
			 "Out_Sync", 
			 "FSM Stat"};

unsigned long read_ofdm_int()
{
    unsigned long stat, mask, tmp;
    int idx;
    char buf[80];

    // read ofdm/xxx status
    tmp = (*OFDM_INT_STS);
    mask = (*OFDM_INT_EN);
    stat = tmp & mask;

    for (idx=0; idx<11; idx++) {
	if (stat>>idx&1) { 
	    strcpy(buf, "OFDM ");
	    strcat(buf, ofdm_int_name[idx]);
	    strcat(buf, " INT %d    STATUS   %x");
	    //printk(buf, idx, stat);    
	}
    }
    // clear ofdm/xxx status
    (*OFDM_INT_STS) = 0;

    return stat;
}

#define PHS_LOOP_OPEN

void qam_initial(int qam_id)  // 0:16QAM 1:32QAM 2:64QAM 3:128QAM 4:256QAM
{
    dvbc_cfg_regs_t *dvbc_cfg_regs = QAM_BASE;
    dvbc_cfg_04_t dvbc_cfg_04;
    dvbc_cfg_08_t dvbc_cfg_08;

    dvbc_cfg_04.d32 = 0;
    dvbc_cfg_04.b.dc_enable = 1;
    dvbc_cfg_04.b.dc_alpha = 4;
    dvbc_cfg_regs->dvbc_cfg_04 = dvbc_cfg_04.d32;

    dvbc_cfg_08.b.qam_mode_cfg = qam_id;
    dvbc_cfg_regs->dvbc_cfg_08 = dvbc_cfg_08.d32;

    if (qam_id==0) { // 16QAM
#ifdef PHS_LOOP_OPEN  
	apb_write_reg(QAM_BASE+0x54, 0x23360224);  // EQ_FIR_CTL,
#else
	apb_write_reg(QAM_BASE+0x54, 0x23440222);  // EQ_FIR_CTL
#endif    

	apb_write_reg(QAM_BASE+0x68, 0x00c000c0);  // EQ_CRTH_SNR

#ifdef PHS_LOOP_OPEN  
	apb_write_reg(QAM_BASE+0x74, 0x50001a0);  // EQ_TH_LMS  40db  13db
#else
	apb_write_reg(QAM_BASE+0x74, 0x02040200);  // EQ_TH_LMS  16.5db  16db
#endif      

	apb_write_reg(QAM_BASE+0x7c, 0x003001e9);  // EQ_TH_MMA
	apb_write_reg(QAM_BASE+0x80, 0x000be1ff);  // EQ_TH_SMMA0
	apb_write_reg(QAM_BASE+0x84, 0x00000000);  // EQ_TH_SMMA1
	apb_write_reg(QAM_BASE+0x88, 0x00000000);  // EQ_TH_SMMA2
	apb_write_reg(QAM_BASE+0x8c, 0x00000000);  // EQ_TH_SMMA3
	apb_write_reg(QAM_BASE+0x94, 0x7f800d2b);   // AGC_CTRL
    }
    else if (qam_id==1) { // 32QAM
#ifdef PHS_LOOP_OPEN  
	apb_write_reg(QAM_BASE+0x54, 0x24560506);  // EQ_FIR_CTL, 
#else
	apb_write_reg(QAM_BASE+0x54, 0x24540502);  // EQ_FIR_CTL, 
#endif   

	apb_write_reg(QAM_BASE+0x68, 0x00c000c0);  // EQ_CRTH_SNR

#ifdef PHS_LOOP_OPEN  
	apb_write_reg(QAM_BASE+0x74, 0x50001f0);  // EQ_TH_LMS  40db  17.5db
#else
	apb_write_reg(QAM_BASE+0x74, 0x02440240);  // EQ_TH_LMS  18.5db  18db
#endif    

	apb_write_reg(QAM_BASE+0x7c, 0x00500102);  // EQ_TH_MMA  0x000001cc
	apb_write_reg(QAM_BASE+0x80, 0x00077140);  // EQ_TH_SMMA0
	apb_write_reg(QAM_BASE+0x84, 0x001fb000);  // EQ_TH_SMMA1
	apb_write_reg(QAM_BASE+0x88, 0x00000000);  // EQ_TH_SMMA2
	apb_write_reg(QAM_BASE+0x8c, 0x00000000);  // EQ_TH_SMMA3 	
	apb_write_reg(QAM_BASE+0x94, 0x7f800d2b);   // AGC_CTRL
    }
    else if (qam_id==2) { // 64QAM

#ifdef PHS_LOOP_OPEN  
	apb_write_reg(QAM_BASE+0x54, 0x2256033a);  // EQ_FIR_CTL, 
#else
	apb_write_reg(QAM_BASE+0x54, 0x22530333);  // EQ_FIR_CTL, 
#endif

	apb_write_reg(QAM_BASE+0x68, 0x00c000c0);  // EQ_CRTH_SNR

#ifdef PHS_LOOP_OPEN  
	apb_write_reg(QAM_BASE+0x74, 0x5000230);  // EQ_TH_LMS  40db  17.5db
#else
	apb_write_reg(QAM_BASE+0x74, 0x02c402c0);  // EQ_TH_LMS  22db  22.5db
#endif

	apb_write_reg(QAM_BASE+0x7c, 0x007001bd);  // EQ_TH_MMA
	apb_write_reg(QAM_BASE+0x80, 0x000580ed);  // EQ_TH_SMMA0
	apb_write_reg(QAM_BASE+0x84, 0x001771fb);  // EQ_TH_SMMA1
	apb_write_reg(QAM_BASE+0x88, 0x00000000);  // EQ_TH_SMMA2
	apb_write_reg(QAM_BASE+0x8c, 0x00000000);  // EQ_TH_SMMA3
	apb_write_reg(QAM_BASE+0x94, 0x7f800d2c);   // AGC_CTRL
    }
    else if (qam_id==3) { // 128QAM
	apb_write_reg(QAM_BASE+0x68, 0x00c000d0);  // EQ_CRTH_SNR
	apb_write_reg(QAM_BASE+0x74, 0x03240320);  // EQ_TH_LMS  25.5db  25db

	apb_write_reg(QAM_BASE+0x7c, 0x00b000f2);  // EQ_TH_MMA0x000000b2
	apb_write_reg(QAM_BASE+0x80, 0x0003a09d);  // EQ_TH_SMMA0
	apb_write_reg(QAM_BASE+0x84, 0x000f8150);  // EQ_TH_SMMA1
	apb_write_reg(QAM_BASE+0x88, 0x001a51f8);  // EQ_TH_SMMA2
	apb_write_reg(QAM_BASE+0x8c, 0x00000000);  // EQ_TH_SMMA3  
	apb_write_reg(QAM_BASE+0x94, 0x7f800d2c);   // AGC_CTRL
    }
    else if (qam_id==4) { // 256QAM

#ifdef PHS_LOOP_OPEN  
	apb_write_reg(QAM_BASE+0x54, 0xa2580588);  // EQ_FIR_CTL, 
#else
	apb_write_reg(QAM_BASE+0x54, 0xa4660666);  // EQ_FIR_CTL, 
#endif    

	apb_write_reg(QAM_BASE+0x68, 0x01e00220);  // EQ_CRTH_SNR

	apb_write_reg(QAM_BASE+0x74, 0x03480340);    // EQ_TH_LMS  26.5db  26db

	apb_write_reg(QAM_BASE+0x7c, 0x00f001a5);  // EQ_TH_MMA
	apb_write_reg(QAM_BASE+0x80, 0x0002c077);  // EQ_TH_SMMA0
	apb_write_reg(QAM_BASE+0x84, 0x000bc0fe);  // EQ_TH_SMMA1
	apb_write_reg(QAM_BASE+0x88, 0x0013f17e);  // EQ_TH_SMMA2
	apb_write_reg(QAM_BASE+0x8c, 0x001bc1f9);  // EQ_TH_SMMA3
	apb_write_reg(QAM_BASE+0x94, 0x7f800d2c);   // AGC_CTRL
    }

    apb_write_reg(QAM_BASE+0xd0, 0x3fff8); // enable interrupt
}

void qam_read_all_regs()
{
    int i, addr;
    unsigned long tmp;

    for (i=0; i<0xf0; i+=4) {
        addr = QAM_BASE + i;
	tmp = *(volatile unsigned *)addr;
	printk("QAM addr 0x%02x  value 0x%08x\n", i, tmp);        	
    }
}

void program_acf(int acf1[20], int acf2[33])
{
    int i;
    
    for (i=0; i<20; i++) 
        apb_write_reg(DVBT_BASE+(0x2c+i)*4, acf1[i]);
    for (i=0; i<33; i++) {
        apb_write_reg(DVBT_BASE+0xfe*4, i);
        apb_write_reg(DVBT_BASE+0xff*4, acf2[i]);
    }
}

void ini_acf_iireq_src_45m_8m()
{
    int acf1[] = {0x255, 0x0b5, 0x091, 0x02e, 
                  0x253, 0x0cb, 0x2cd, 0x07c, 
                  0x250, 0x0e4, 0x276, 0x05d, 
                  0x24d, 0x0f3, 0x25e, 0x05a, 
                  0x24a, 0x0fd, 0x256, 0x04b};
    int acf2[] = {0x3effff, 0x3cefbe, 0x3adf7c, 0x38bf39, 0x3696f5, 0x3466b0, 
                  0x322e69, 0x2fee21, 0x2dadd9, 0x2b6d91, 0x291d48, 0x26ccfe, 
                  0x245cb2, 0x21d463, 0x1f2410, 0x1c3bb6, 0x192b57, 0x15e2f1, 
                  0x127285, 0x0eca14, 0x0ac99b, 0x063913, 0x00c073, 0x3a3fb4, 
                  0x347ecf, 0x2ff649, 0x2a8dab, 0x2444f0, 0x1d0c1b, 0x0fc300, 
                  0x0118ce, 0x3c17c3, 0x000751};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_45m_7m()
{
    int acf1[] = {0x24b, 0x0bd, 0x04b, 0x03e, 
                  0x246, 0x0d1, 0x2a2, 0x07c, 
                  0x241, 0x0e7, 0x25b, 0x05d, 
                  0x23d, 0x0f5, 0x248, 0x05a, 
                  0x23a, 0x0fd, 0x242, 0x04b};
    int acf2[] = {0x3f07ff, 0x3cffbf, 0x3aef7e, 0x38d73c, 0x36b6f9, 0x3486b3, 
                  0x324e6d, 0x300e25, 0x2dcddd, 0x2b8594, 0x292d4b, 0x26d500, 
                  0x245cb3, 0x21cc62, 0x1f0c0d, 0x1c1bb3, 0x18fb52, 0x15b2eb, 
                  0x123a7f, 0x0e9a0e, 0x0a9995, 0x06090d, 0x00a06e, 0x3a57b3, 
                  0x34ded8, 0x309659, 0x2b75c4, 0x25350e, 0x1dec37, 0x126b28, 
                  0x031130, 0x3cffec, 0x000767};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_45m_6m()
{
    int acf1[] = {0x240, 0x0c6, 0x3f9, 0x03e, 
                  0x23a, 0x0d7, 0x27b, 0x07c, 
                  0x233, 0x0ea, 0x244, 0x05d, 
                  0x22f, 0x0f6, 0x235, 0x05a, 
                  0x22b, 0x0fd, 0x231, 0x04b};
    int acf2[] = {0x3f07ff, 0x3cffbf, 0x3aef7e, 0x38d73c, 0x36b6f8, 0x3486b3, 
                  0x32466c, 0x2ffe24, 0x2dadda, 0x2b5d90, 0x28fd45, 0x2694f9, 
                  0x2414ab, 0x217458, 0x1ea402, 0x1ba3a5, 0x187342, 0x151ad9, 
                  0x11926b, 0x0dc9f6, 0x09a178, 0x04d8eb, 0x3f4045, 0x38e785, 
                  0x337eab, 0x2f3e2d, 0x2a1599, 0x23ace1, 0x1b33fb, 0x0cd29c, 
                  0x01c0c1, 0x3cefde, 0x00076a};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_45m_5m()
{
    int acf1[] = {0x236, 0x0ce, 0x39a, 0x03e, 
                  0x22f, 0x0de, 0x257, 0x07c, 
                  0x227, 0x0ee, 0x230, 0x05d, 
                  0x222, 0x0f8, 0x225, 0x05a, 
                  0x21e, 0x0fe, 0x222, 0x04b};
    int acf2[] = {0x3effff, 0x3ce7bd, 0x3ac77a, 0x38a737, 0x367ef2, 0x344eac, 
                  0x321e66, 0x2fee20, 0x2dbdda, 0x2b8d94, 0x295d4e, 0x272508, 
                  0x24dcc0, 0x227475, 0x1fe426, 0x1d1bd1, 0x1a2374, 0x16e311, 
                  0x136aa6, 0x0fba33, 0x0ba9b8, 0x07092e, 0x01988e, 0x3b37d0, 
                  0x35aef3, 0x316673, 0x2c45de, 0x25e527, 0x1da444, 0x0deaea, 
                  0x0178bf, 0x3cb7d6, 0x000765};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_207m_8m()
{
    int acf1[] = {0x318, 0x03e, 0x1ae, 0x00e, 
                  0x326, 0x074, 0x074, 0x06f, 
                  0x336, 0x0b1, 0x3c9, 0x008, 
                  0x33f, 0x0dc, 0x384, 0x05a, 
                  0x340, 0x0f6, 0x36d, 0x04b};
    int acf2[] = {0x3f37ff, 0x3d97cc, 0x3bf798, 0x3a4f64, 0x38a72f, 0x36f6f9, 
                  0x3546c3, 0x33868c, 0x31be54, 0x2fe61a, 0x2e05df, 0x2c15a2, 
                  0x2a1562, 0x27f520, 0x25c4dc, 0x236c93, 0x20f446, 0x1e4bf4, 
                  0x1b739d, 0x185b3d, 0x14ead5, 0x111a62, 0x0cb9df, 0x079148, 
                  0x030093, 0x3f802a, 0x3b77b2, 0x36a725, 0x30ae7b, 0x285d9f, 
                  0x1abc46, 0x0f8a85, 0x000187};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_207m_7m()
{
    int acf1[] = {0x2f9, 0x04c, 0x18e, 0x00e, 
                  0x2fd, 0x07f, 0x01a, 0x06d, 
                  0x300, 0x0b8, 0x372, 0x05f, 
                  0x301, 0x0df, 0x335, 0x05a, 
                  0x2fe, 0x0f7, 0x320, 0x04b};
    int acf2[] = {0x3f37ff, 0x3d8fcc, 0x3bef97, 0x3a4762, 0x38972d, 0x36e6f7, 
                  0x352ec1, 0x336e89, 0x319e50, 0x2fce16, 0x2de5db, 0x2bf59d, 
                  0x29ed5e, 0x27d51c, 0x259cd7, 0x23448e, 0x20cc41, 0x1e23ef, 
                  0x1b4b98, 0x183339, 0x14cad1, 0x10fa5e, 0x0c99dc, 0x078145, 
                  0x02f892, 0x3f802a, 0x3b8fb3, 0x36d729, 0x310682, 0x290dae, 
                  0x1c0c67, 0x10a2ad, 0x0001a8};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_207m_6m()
{
    int acf1[] = {0x2d9, 0x05c, 0x161, 0x00e, 
                  0x2d4, 0x08b, 0x3b8, 0x06b, 
                  0x2cd, 0x0c0, 0x31e, 0x05f, 
                  0x2c7, 0x0e3, 0x2eb, 0x05a, 
                  0x2c1, 0x0f8, 0x2da, 0x04b};
    int acf2[] = {0x3f2fff, 0x3d87cb, 0x3bdf96, 0x3a2f60, 0x387f2a, 0x36c6f4, 
                  0x350ebd, 0x334684, 0x31764b, 0x2f9e11, 0x2db5d4, 0x2bbd97, 
                  0x29b557, 0x279515, 0x255ccf, 0x230c87, 0x20943a, 0x1debe8, 
                  0x1b1b91, 0x180b33, 0x14aacc, 0x10e25a, 0x0c91da, 0x078945, 
                  0x031895, 0x3fa82e, 0x3bbfb8, 0x371730, 0x31668c, 0x299dbc, 
                  0x1d1480, 0x119acf, 0x0001c4};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_207m_5m()
{
    int acf1[] = {0x2b9, 0x06e, 0x11e, 0x01e, 
                  0x2ab, 0x099, 0x351, 0x06b, 
                  0x29d, 0x0c8, 0x2d0, 0x05f, 
                  0x292, 0x0e7, 0x2a8, 0x05a, 
                  0x28a, 0x0f9, 0x29b, 0x04b};
    int acf2[] = {0x3f2fff, 0x3d7fca, 0x3bcf94, 0x3a1f5e, 0x386727, 0x36a6f0, 
                  0x34e6b8, 0x33167f, 0x314645, 0x2f660a, 0x2d75cd, 0x2b758e, 
                  0x29654e, 0x27450a, 0x2504c4, 0x22a47b, 0x20242d, 0x1d7bdb, 
                  0x1aa383, 0x178b24, 0x142abd, 0x10624a, 0x0c11ca, 0x070935, 
                  0x029885, 0x3f281e, 0x3b3fa9, 0x369720, 0x30ce7b, 0x28dda7, 
                  0x1c6464, 0x11b2c7, 0x0001cb};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_2857m_8m()
{
    int acf1[] = {0x2db, 0x05b, 0x163, 0x00e, 
                  0x2d5, 0x08b, 0x3bc, 0x06d, 
                  0x2cf, 0x0bf, 0x321, 0x008, 
                  0x2c9, 0x0e3, 0x2ee, 0x058, 
                  0x2c3, 0x0f8, 0x2dd, 0x04d};
    int acf2[] = {0x3ef7ff, 0x3d37c0, 0x3c3f94, 0x3b0f78, 0x38c73f, 0x369ef1, 
                  0x3576be, 0x33b698, 0x31164d, 0x2f1dfd, 0x2de5cf, 0x2c15a2, 
                  0x29f560, 0x27bd1b, 0x252ccf, 0x22bc7c, 0x207c34, 0x1da3e5, 
                  0x1a9b83, 0x17db27, 0x1432c6, 0x0fa23e, 0x0b91af, 0x077136, 
                  0x02c090, 0x3ec01a, 0x3a3f92, 0x354efa, 0x2fee54, 0x2a35a3, 
                  0x23f4e4, 0x1cdc12, 0x000316};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_2857m_7m()
{
    int acf1[] = {0x2c2, 0x069, 0x134, 0x00e, 
                  0x2b7, 0x095, 0x36f, 0x06d, 
                  0x2aa, 0x0c6, 0x2e5, 0x008, 
                  0x2a1, 0x0e6, 0x2ba, 0x058, 
                  0x299, 0x0f9, 0x2ac, 0x04d};
    int acf2[] = {0x3ee7ff, 0x3d1fbc, 0x3bf790, 0x3a876a, 0x388f31, 0x36c6f3, 
                  0x3536bf, 0x334689, 0x310644, 0x2ef5fd, 0x2d45c2, 0x2b7d8c, 
                  0x298550, 0x278510, 0x252ccc, 0x22847c, 0x201427, 0x1e03e0, 
                  0x1b6b9b, 0x17c336, 0x13e2b8, 0x10b246, 0x0d81e8, 0x084966, 
                  0x03089c, 0x3f0022, 0x3aaf9c, 0x360f0c, 0x312e74, 0x2c05d3, 
                  0x268d2a, 0x20bc76, 0x0003b3};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_2857m_6m()
{
    int acf1[] = {0x2a9, 0x078, 0x0f4, 0x01e, 
                  0x299, 0x0a1, 0x321, 0x078, 
                  0x288, 0x0cd, 0x2ae, 0x05f, 
                  0x27c, 0x0e9, 0x28b, 0x058, 
                  0x273, 0x0fa, 0x281, 0x04d};
    int acf2[] = {0x3f17ff, 0x3d3fc4, 0x3b7f8a, 0x39df55, 0x381720, 0x360ee2, 
                  0x342ea1, 0x32ee6e, 0x31e64e, 0x300e22, 0x2daddc, 0x2b758f, 
                  0x29ad51, 0x27ad18, 0x250ccd, 0x227476, 0x204c2a, 0x1de3e6, 
                  0x1a838a, 0x16ab12, 0x137a9d, 0x113a4a, 0x0db1f8, 0x07c15f, 
                  0x022883, 0x3df803, 0x398f79, 0x34d6e6, 0x2fd64b, 0x2a8da7, 
                  0x2504fa, 0x1f2443, 0x000382};

    program_acf(acf1, acf2);
}

void ini_acf_iireq_src_2857m_5m()
{
    int acf1[] = {0x28f, 0x088, 0x09e, 0x01e, 
                  0x27c, 0x0ad, 0x2d6, 0x078, 
                  0x268, 0x0d4, 0x27c, 0x05f, 
                  0x25b, 0x0ed, 0x262, 0x058, 
                  0x252, 0x0fb, 0x25a, 0x04d};
    int acf2[] = {0x3f17ff, 0x3d4fc5, 0x3baf8e, 0x3a3f5d, 0x38df32, 0x374703, 
                  0x354ec9, 0x333e88, 0x314e47, 0x2f860c, 0x2d9dd2, 0x2b5590, 
                  0x28cd42, 0x266cf2, 0x245cab, 0x225c6b, 0x200427, 0x1d4bd5, 
                  0x1a9b7d, 0x183b2b, 0x15b2e1, 0x122a83, 0x0d49fc, 0x07594e, 
                  0x024080, 0x3e980e, 0x3ab796, 0x368f15, 0x320e8a, 0x2d25f4, 
                  0x27ad4f, 0x219496, 0x0003c9};

    program_acf(acf1, acf2);
}

void  ini_icfo_pn_index (int mode) // 00:DVBT,01:ISDBT
{
    if (mode == 0)
    {
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000031);apb_write_reg(DVBT_BASE+0x3fc, 0x00030000);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000032);apb_write_reg(DVBT_BASE+0x3fc, 0x00057036);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000033);apb_write_reg(DVBT_BASE+0x3fc, 0x0009c08d);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000034);apb_write_reg(DVBT_BASE+0x3fc, 0x000c90c0);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000035);apb_write_reg(DVBT_BASE+0x3fc, 0x001170ff);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000036);apb_write_reg(DVBT_BASE+0x3fc, 0x0014d11a);
    }
    else if (mode == 1)
    {
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000031);apb_write_reg(DVBT_BASE+0x3fc, 0x00085046);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000032);apb_write_reg(DVBT_BASE+0x3fc, 0x0019a0e9);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000033);apb_write_reg(DVBT_BASE+0x3fc, 0x0024b1dc);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000034);apb_write_reg(DVBT_BASE+0x3fc, 0x003b3313);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000035);apb_write_reg(DVBT_BASE+0x3fc, 0x0048d409);
       apb_write_reg(DVBT_BASE+0x3f8, 0x00000036);apb_write_reg(DVBT_BASE+0x3fc, 0x00527509);
    }
}

void tfd_filter_coff_ini()
{
    int i;
    int coef[] = {
        0xf900, 0xfe00, 0x0000, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000, 
        0xfd00, 0xf700, 0x0000, 0x0000, 0x4c00, 0x0000, 0x0000, 0x0000, 
        0x2200, 0x0c00, 0x0000, 0x0000, 0xf700, 0xf700, 0x0000, 0x0000, 
        0x0300, 0x0900, 0x0000, 0x0000, 0x0600, 0x0600, 0x0000, 0x0000, 
        0xfc00, 0xf300, 0x0000, 0x0000, 0x2e00, 0x0000, 0x0000, 0x0000, 
        0x3900, 0x1300, 0x0000, 0x0000, 0xfa00, 0xfa00, 0x0000, 0x0000, 
        0x0100, 0x0200, 0x0000, 0x0000, 0xf600, 0x0000, 0x0000, 0x0000, 
        0x0700, 0x0700, 0x0000, 0x0000, 0xfe00, 0xfb00, 0x0000, 0x0000, 
        0x0900, 0x0000, 0x0000, 0x0000, 0x3200, 0x1100, 0x0000, 0x0000, 
        0x0400, 0x0400, 0x0000, 0x0000, 0xfe00, 0xfb00, 0x0000, 0x0000, 
        0x0e00, 0x0000, 0x0000, 0x0000, 0xfb00, 0xfb00, 0x0000, 0x0000, 
        0x0100, 0x0200, 0x0000, 0x0000, 0xf400, 0x0000, 0x0000, 0x0000, 
        0x3900, 0x1300, 0x0000, 0x0000, 0x1700, 0x1700, 0x0000, 0x0000, 
        0xfc00, 0xf300, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000, 0x0000, 
        0x0300, 0x0900, 0x0000, 0x0000, 0xee00, 0x0000, 0x0000, 0x0000, 
        0x2200, 0x0c00, 0x0000, 0x0000, 0x2600, 0x2600, 0x0000, 0x0000, 
        0xfd00, 0xf700, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000, 
        0xf900, 0xfe00, 0x0000, 0x0000, 0x0400, 0x0b00, 0x0000, 0x0000, 
        0xf900, 0x0000, 0x0000, 0x0000, 0x0700, 0x0200, 0x0000, 0x0000, 
        0x2100, 0x2100, 0x0000, 0x0000, 0x0200, 0x0700, 0x0000, 0x0000, 
        0xf900, 0x0000, 0x0000, 0x0000, 0x0b00, 0x0400, 0x0000, 0x0000, 
        0xfe00, 0xf900, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000, 
        0xf700, 0xfd00, 0x0000, 0x0000, 0x2600, 0x2600, 0x0000, 0x0000, 
        0x0c00, 0x2200, 0x0000, 0x0000, 0xee00, 0x0000, 0x0000, 0x0000, 
        0x0900, 0x0300, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000, 0x0000, 
        0xf300, 0xfc00, 0x0000, 0x0000, 0x1700, 0x1700, 0x0000, 0x0000, 
        0x1300, 0x3900, 0x0000, 0x0000, 0xf400, 0x0000, 0x0000, 0x0000, 
        0x0200, 0x0100, 0x0000, 0x0000, 0xfb00, 0xfb00, 0x0000, 0x0000, 
        0x0e00, 0x0000, 0x0000, 0x0000, 0xfb00, 0xfe00, 0x0000, 0x0000, 
        0x0400, 0x0400, 0x0000, 0x0000, 0x1100, 0x3200, 0x0000, 0x0000, 
        0x0900, 0x0000, 0x0000, 0x0000, 0xfb00, 0xfe00, 0x0000, 0x0000, 
        0x0700, 0x0700, 0x0000, 0x0000, 0xf600, 0x0000, 0x0000, 0x0000, 
        0x0200, 0x0100, 0x0000, 0x0000, 0xfa00, 0xfa00, 0x0000, 0x0000, 
        0x1300, 0x3900, 0x0000, 0x0000, 0x2e00, 0x0000, 0x0000, 0x0000, 
        0xf300, 0xfc00, 0x0000, 0x0000, 0x0600, 0x0600, 0x0000, 0x0000, 
        0x0900, 0x0300, 0x0000, 0x0000, 0xf700, 0xf700, 0x0000, 0x0000, 
        0x0c00, 0x2200, 0x0000, 0x0000, 0x4c00, 0x0000, 0x0000, 0x0000, 
        0xf700, 0xfd00, 0x0000, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000, 
        0xfe00, 0xf900, 0x0000, 0x0000, 0x0b00, 0x0400, 0x0000, 0x0000, 
        0xfc00, 0xfc00, 0x0000, 0x0000, 0x0200, 0x0700, 0x0000, 0x0000, 
        0x4200, 0x0000, 0x0000, 0x0000, 0x0700, 0x0200, 0x0000, 0x0000, 
        0xfc00, 0xfc00, 0x0000, 0x0000, 0x0400, 0x0b00, 0x0000, 0x0000};

    for (i=0; i<336; i++) {
        apb_write_reg(DVBT_BASE+0x99*4, (i<<16) | coef[i]);
        apb_write_reg(DVBT_BASE+0x03*4, (1<<12));
    }
}

void ofdm_initial(
    int bandwidth, // 00:8M 01:7M 10:6M 11:5M
    int samplerate,// 00:45M 01:20.8333M 10:20.7M 11:28.57 
    int IF,        // 000:36.13M 001:-5.5M 010:4.57M 011:4M 100:5M
    int mode,       // 00:DVBT,01:ISDBT
    int tc_mode     // 0: Unsigned, 1:TC
) 
{
    int tmp,tmp2;
    int ch_if;
    int adc_freq;
	printk("[ofdm_initial]bandwidth is %d,samplerate is %d,IF is %d, mode is %d,tc_mode is %d\n",bandwidth,samplerate,IF,mode,tc_mode);
    switch(IF)
    {
        case 0: ch_if = 36130;break;
        case 1: ch_if = -5500;break;
        case 2: ch_if = 4570;break;
        case 3: ch_if = 4000;break;
        case 4: ch_if = 5000;break;
        default: ch_if = 4000;break;
    }
    switch(samplerate)
    {
        case 0: adc_freq = 45000;break;
        case 1: adc_freq = 20833;break;
        case 2: adc_freq = 20700;break;
        case 3: adc_freq = 28571;break;
        default: adc_freq = 28571;break;
    }

    apb_write_reg(DVBT_BASE+(0x02<<2), 0x00800000);  // SW reset bit[23] ; write anything to zero
    apb_write_reg(DVBT_BASE+(0x00<<2), 0x00000000);

    apb_write_reg(DVBT_BASE+(0xe<<2), 0xffff); // enable interrupt

    if (mode == 0) { // DVBT
       switch (samplerate)
       {
         case 0 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x00005a00);break;  // 45MHz
         case 1 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x000029aa);break;  // 20.833
         case 2 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x00002966);break;  // 20.7
         case 3 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x00003924);break;  // 28.571
         default: apb_write_reg(DVBT_BASE+(0x08<<2), 0x00003924);break;  // 28.571
       }
    }
    else { //ISDBT
       switch (samplerate)
       {
         case 0 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x0000580d);break;  // 45MHz
         case 1 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x0000290d);break;  // 20.833 = 48/7 * 20.8333 / (512/63)
         case 2 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x000028da);break;  // 20.7
         case 3 : apb_write_reg(DVBT_BASE+(0x08<<2), 0x00003863);break;  // 28.571
         default: apb_write_reg(DVBT_BASE+(0x08<<2), 0x00003863);break;  // 28.571
       }
    }

    apb_write_reg(DVBT_BASE+(0x10<<2), 0x8f300000);

    apb_write_reg(DVBT_BASE+(0x14<<2), 0xe81c4ff6);   // AGC_TARGET 0xf0121385

    switch(samplerate)
    {
       case 0 : apb_write_reg(DVBT_BASE+(0x15<<2), 0x018c2df2); break;
       case 1 : apb_write_reg(DVBT_BASE+(0x15<<2), 0x0185bdf2); break; 
       case 2 : apb_write_reg(DVBT_BASE+(0x15<<2), 0x0185bdf2); break; 
       case 3 : apb_write_reg(DVBT_BASE+(0x15<<2), 0x0187bdf2); break; 
       default: apb_write_reg(DVBT_BASE+(0x15<<2), 0x0187bdf2); break; 
    }
    if (tc_mode == 1) apb_write_regb(DVBT_BASE+(0x15<<2),11,0);// For TC mode. Notice, For ADC input is Unsigned, For Capture Data, It is TC.
     apb_write_regb(DVBT_BASE+(0x15<<2),26,1); // [19:0] = [I , Q], I is high, Q is low. This bit is reverse I/Q. 

    apb_write_reg(DVBT_BASE+(0x16<<2), 0x00047f80);  // AGC_IFGAIN_CTRL
    apb_write_reg(DVBT_BASE+(0x17<<2), 0x00027f80);  // AGC_RFGAIN_CTRL
    apb_write_reg(DVBT_BASE+(0x18<<2), 0x00000190);  // AGC_IFGAIN_ACCUM
    apb_write_reg(DVBT_BASE+(0x19<<2), 0x00000190);  // AGC_RFGAIN_ACCUM
    if (ch_if <0) ch_if += adc_freq;
    if (ch_if > adc_freq) ch_if -= adc_freq;

    tmp = ch_if * (1<<15) / adc_freq;
    apb_write_reg(DVBT_BASE + (0x20<<2), tmp);
    
    switch (IF)
    {
       case 0  : tmp2 =  0x000066e2;  break;// 36.13M IF
       case 1  : tmp2 =  0x00005e35;  break;// -5.5M IF
       case 2  : tmp2 =  0x00001f26;  break;// 4.57M IF
       case 3  : tmp2 =  0x000011eb;  break;// 4.00M IF
       case 4  : tmp2 =  0x00001666;  break;// 5.00M IF
       default : tmp2 =  0x00001666;  break;// 5.00M IF
    }
    
    //printk(" IF Configure Old is %x, New is %x\n", tmp2, tmp);


    apb_write_reg(DVBT_BASE+(0x21<<2), 0x001ff000);  // DDC CS_FCFO_ADJ_CTRL
    apb_write_reg(DVBT_BASE+(0x22<<2), 0x00000000);  // DDC ICFO_ADJ_CTRL
    apb_write_reg(DVBT_BASE+(0x23<<2), 0x00004000);  // DDC TRACK_FCFO_ADJ_CTRL
	
    apb_write_reg(DVBT_BASE+(0x27<<2), (1<<23)|(3<<19)|(3<<15)|(1000<<4)|9); // {8'd0,1'd1,4'd3,4'd3,11'd50,4'd9});//FSM_1
    apb_write_reg(DVBT_BASE+(0x28<<2), (100<<13)|1000);//{8'd0,11'd40,13'd50});//FSM_2
	apb_write_reg(DVBT_BASE+(0x29<<2), (31<<20)|(1<<16)|(24<<9)|(3<<6)|20);//{5'd0,7'd127,1'd0,3'd0,7'd24,3'd5,6'd20});

    if (mode == 0) { // DVBT

        if (bandwidth==0) { // 8M
            switch(samplerate)
            {
               case 0: ini_acf_iireq_src_45m_8m();  apb_write_reg(DVBT_BASE+(0x44<<2), 0x004ebf2e); break;// 45M
               case 1: ini_acf_iireq_src_207m_8m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x00247551); break;//20.833M
               case 2: ini_acf_iireq_src_207m_8m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x00243999); break;//20.7M
               case 3: ini_acf_iireq_src_2857m_8m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x0031ffcd); break;//28.57M
               default: ini_acf_iireq_src_2857m_8m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x0031ffcd); break;//28.57M
            }
        }
        else if (bandwidth==1) { // 7M
            switch(samplerate)
            {
               case 0: ini_acf_iireq_src_45m_7m();  apb_write_reg(DVBT_BASE+(0x44<<2), 0x0059ff10); break;// 45M
               case 1: ini_acf_iireq_src_207m_7m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x0029aaa6); break;//20.833M
               case 2: ini_acf_iireq_src_207m_7m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x00296665); break;//20.7M
               case 3: ini_acf_iireq_src_2857m_7m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x00392491); break;//28.57M
               default: ini_acf_iireq_src_2857m_7m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x00392491); break;//28.57M
            }
        }
        else if (bandwidth==2) { // 6M
            switch(samplerate)
            {
               case 0: ini_acf_iireq_src_45m_6m();  apb_write_reg(DVBT_BASE+(0x44<<2), 0x00690000); break;// 45M
               case 1: ini_acf_iireq_src_207m_6m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x00309c3e); break;//20.833M
               case 2: ini_acf_iireq_src_207m_6m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x002eaaaa); break;//20.7M
               case 3: ini_acf_iireq_src_2857m_6m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x0042AA69); break;//28.57M
               default: ini_acf_iireq_src_2857m_6m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x0042AA69); break;//28.57M
            }
        }
        else {// 5M
            switch(samplerate)
            {
               case 0: ini_acf_iireq_src_45m_5m();  apb_write_reg(DVBT_BASE+(0x44<<2), 0x007dfbe0); break;// 45M
               case 1: ini_acf_iireq_src_207m_5m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x003a554f); break;//20.833M
               case 2: ini_acf_iireq_src_207m_5m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x0039f5c0); break;//20.7M
               case 3: ini_acf_iireq_src_2857m_5m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x004FFFFE); break;//28.57M
               default: ini_acf_iireq_src_2857m_5m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x004FFFFE); break;//28.57M
            }
        } 
    } 
    else // ISDBT
    {
        switch(samplerate)
        {
           case 0: ini_acf_iireq_src_45m_7m();  apb_write_reg(DVBT_BASE+(0x44<<2), 0x00589800);break;// 45M
           case 1: ini_acf_iireq_src_207m_7m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x002903d4);break;// 20.833M
           case 2: ini_acf_iireq_src_207m_7m(); apb_write_reg(DVBT_BASE+(0x44<<2), 0x00280ccc);break;// 20.7M
           case 3: ini_acf_iireq_src_2857m_7m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x00383fc8);break;// 28.57M
           default: ini_acf_iireq_src_2857m_7m();apb_write_reg(DVBT_BASE+(0x44<<2), 0x00383fc8);break;//28.57M
        }	        
    }

    if (mode == 0)// DVBT 
	apb_write_reg(DVBT_BASE+(0x02<<2), (bandwidth<<20)|0x10002);    
    else          // ISDBT
        apb_write_reg(DVBT_BASE+(0x02<<2), (1<<20)|0x1001a);//{0x000,2'h1,20'h1_001a});    // For ISDBT , bandwith should be 1

    apb_write_reg(DVBT_BASE+(0x45<<2), 0x00000000);  // SRC SFO_ADJ_CTRL
 	apb_write_reg(DVBT_BASE+(0x46<<2), 0x02004000);  // SRC SFO_ADJ_CTRL
    apb_write_reg(DVBT_BASE+(0x48<<2), 0x000c0287);  // DAGC_CTRL1
    apb_write_reg(DVBT_BASE+(0x49<<2), 0x00000005);  // DAGC_CTRL2
    apb_write_reg(DVBT_BASE+(0x4c<<2), 0x00000bbf);  // CCI_RP
    apb_write_reg(DVBT_BASE+(0x4d<<2), 0x00000376);  // CCI_RPSQ
    apb_write_reg(DVBT_BASE+(0x4e<<2), 0x0f0f1d09);  // CCI_CTRL 
    apb_write_reg(DVBT_BASE+(0x4f<<2), 0x00000000);  // CCI DET_INDX1
    apb_write_reg(DVBT_BASE+(0x50<<2), 0x00000000);  // CCI DET_INDX2
    apb_write_reg(DVBT_BASE+(0x51<<2), 0x00000000);  // CCI_NOTCH1_A1
    apb_write_reg(DVBT_BASE+(0x52<<2), 0x00000000);  // CCI_NOTCH1_A2
    apb_write_reg(DVBT_BASE+(0x53<<2), 0x00000000);  // CCI_NOTCH1_B1
    apb_write_reg(DVBT_BASE+(0x54<<2), 0x00000000);  // CCI_NOTCH2_A1
    apb_write_reg(DVBT_BASE+(0x55<<2), 0x00000000);  // CCI_NOTCH2_A2
    apb_write_reg(DVBT_BASE+(0x56<<2), 0x00000000);  // CCI_NOTCH2_B1
    apb_write_reg(DVBT_BASE+(0x58<<2), 0x00000885);  // MODE_DETECT_CTRL // 582
    if (mode == 0) //DVBT
    	apb_write_reg(DVBT_BASE+(0x5c<<2), 0x00001011);  // 
    else
    	apb_write_reg(DVBT_BASE+(0x5c<<2), 0x00000753);  // ICFO_EST_CTRL ISDBT ICFO thres = 2

    apb_write_reg(DVBT_BASE+(0x5f<<2), 0x0ffffe10);  // TPS_FCFO_CTRL
    apb_write_reg(DVBT_BASE+(0x61<<2), 0x0000006c);  // FWDT ctrl
 	apb_write_reg(DVBT_BASE+(0x68<<2), 0x128c3929); 
    apb_write_reg(DVBT_BASE+(0x69<<2), 0x91017f2d);  // 0x1a8
    apb_write_reg(DVBT_BASE+(0x6b<<2), 0x00442211);  // 0x1a8
    apb_write_reg(DVBT_BASE+(0x6c<<2), 0x01fc400a);  // 0x
    apb_write_reg(DVBT_BASE+(0x6d<<2), 0x0030303f);  // 0x
    apb_write_reg(DVBT_BASE+(0x73<<2), 0xffffffff);  // CCI0_PILOT_UPDATE_CTRL
    apb_write_reg(DVBT_BASE+(0x74<<2), 0xffffffff);  // CCI0_DATA_UPDATE_CTRL
    apb_write_reg(DVBT_BASE+(0x75<<2), 0xffffffff);  // CCI1_PILOT_UPDATE_CTRL
    apb_write_reg(DVBT_BASE+(0x76<<2), 0xffffffff);  // CCI1_DATA_UPDATE_CTRL

    tmp = mode==0 ? 0x000001a2 : 0x00000da2;
    apb_write_reg(DVBT_BASE+(0x78<<2), tmp);  // FEC_CTR

    apb_write_reg(DVBT_BASE+(0x7d<<2), 0x0000009d);
    apb_write_reg(DVBT_BASE+(0x7e<<2), 0x00004000);
    apb_write_reg(DVBT_BASE+(0x7f<<2), 0x00008000);
	
	
    apb_write_reg(DVBT_BASE+((0x8b+0)<<2), 0x20002000);
    apb_write_reg(DVBT_BASE+((0x8b+1)<<2), 0x20002000);
    apb_write_reg(DVBT_BASE+((0x8b+2)<<2), 0x20002000);
    apb_write_reg(DVBT_BASE+((0x8b+3)<<2), 0x20002000);
    apb_write_reg(DVBT_BASE+((0x8b+4)<<2), 0x20002000);
    apb_write_reg(DVBT_BASE+((0x8b+5)<<2), 0x20002000);
    apb_write_reg(DVBT_BASE+((0x8b+6)<<2), 0x20002000);
    apb_write_reg(DVBT_BASE+((0x8b+7)<<2), 0x20002000);

    apb_write_reg(DVBT_BASE+(0x93<<2), 0x31);
    apb_write_reg(DVBT_BASE+(0x94<<2), 0x00);
    apb_write_reg(DVBT_BASE+(0x95<<2), 0x7f1);
    apb_write_reg(DVBT_BASE+(0x96<<2), 0x20);

    apb_write_reg(DVBT_BASE+(0x98<<2), 0x03f9115a);
    apb_write_reg(DVBT_BASE+(0x9b<<2), 0x000005df);

    apb_write_reg(DVBT_BASE+(0x9c<<2), 0x00100000);// TestBus write valid, 0 is system clk valid
    apb_write_reg(DVBT_BASE+(0x9d<<2), 0x01000000);// DDR Start address
    apb_write_reg(DVBT_BASE+(0x9e<<2), 0x02000000);// DDR End   address

    apb_write_regb(DVBT_BASE+(0x9b<<2),7,0);// Enable Testbus dump to DDR
    apb_write_regb(DVBT_BASE+(0x9b<<2),8,0);// Run Testbus dump to DDR

    apb_write_reg(DVBT_BASE+(0xd6<<2), 0x00000003);  
    //apb_write_reg(DVBT_BASE+(0xd7<<2), 0x00000008); 
    apb_write_reg(DVBT_BASE+(0xd8<<2), 0x00000120);
    apb_write_reg(DVBT_BASE+(0xd9<<2), 0x01010101);

    ini_icfo_pn_index(mode);
    tfd_filter_coff_ini();

    calculate_cordic_para();

    delay_us(1);

    apb_write_reg(DVBT_BASE+(0x02<<2), apb_read_reg(DVBT_BASE+(0x02<<2)) | (1 << 0));
    apb_write_reg(DVBT_BASE+(0x02<<2), apb_read_reg(DVBT_BASE+(0x02<<2)) | (1 << 24));

}

void calculate_cordic_para()
{
    apb_write_reg(DVBT_BASE+0x0c, 0x00000040);
}

void check_demod_status_symbol()
{
    unsigned long tmp, tmp2;
    static int total_symb = 0;
    static int fec_lock_cnt = 0;
    
    tmp = apb_read_reg(DVBT_BASE+0x1c);
    //printk("  Frame           %d    Symbol %d\n", tmp>>8&0x7f, tmp&0x7f);        
    
    tmp = apb_read_reg(DVBT_BASE+0x180);
    //printk("PFS-SFO           %d  PFS-FCFO %d\n", tmp&0xfff, tmp>>12&0xfff);

    tmp = apb_read_reg(DVBT_BASE+0x28);
    //tmp2 = apb_read_reg(DVBT_BASE+0x30);
    //printk(" SNR SP           %d       TPS %d\n", tmp>>20&0x3ff, tmp>>10&0x3ff);
    ////printk(" SNR CP           %d  DemodBER %d\n", tmp&0x3ff, tmp2&0x7ff);

    //tmp = apb_read_reg(DVBT_BASE+(0x81<<2));// RS correct bits
    //tmp2 = apb_read_reg(DVBT_BASE+(0xbf<<2));// RS PER
    ////printk(" LayA RS corr Bit %x    RS Err %x\n", tmp, tmp2);

    //tmp = apb_read_reg(DVBT_BASE+(0x84<<2));// RS correct bits
    //tmp2 = apb_read_reg(DVBT_BASE+(0xbe<<2));// RS PER
    ////printk(" LayB RS corr Bit %x    RS Err %x\n", tmp, tmp2);

    //tmp = apb_read_reg(DVBT_BASE+(0x87<<2));// RS correct bits
    //tmp2 = apb_read_reg(DVBT_BASE+(0xbd<<2));// RS PER
    ////printk(" LayC RS corr Bit %x    RS Err %x\n", tmp, tmp2);
    
    tmp = apb_read_reg(DVBT_BASE+0x00);
    ////printk(" Status   %x       FEC %d\n", tmp, tmp>>11&0x3);

    if (tmp>>12&1) fec_lock_cnt++;
    else fec_lock_cnt = 0;
    total_symb++;
    //printk("FEC Locked %d  Total Symbols %d\n", fec_lock_cnt, total_symb);

    if (fec_lock_cnt == 68*4) stimulus_finish_pass();
    //if (total_symb == 68*16) stimulus_finish_fail(0);    
}

char *ofdm_fsm_name[] = {"    IDLE", 
			 "     AGC", 
			 "     CCI", 
			 "     ACQ", 
			 "    SYNC", 
			 "TRACKING", 
			 "  TIMING", 
			 " SP_SYNC", 
			 " TPS_DEC", 
			 "FEC_LOCK", 
			 "FEC_LOST"};

void check_fsm_state()
{
    unsigned long tmp;
    
    tmp = apb_read_reg(DVBT_BASE+0xa8);
    //printk(">>>>>>>>>>>>>>>>>>>>>>>>> OFDM FSM From %d        to %d\n", tmp>>4&0xf, tmp&0xf);    

    if ((tmp&0xf)==3) {
        apb_write_regb(DVBT_BASE+(0x9b<<2),8,1);//Stop dump testbus;
        apb_write_regb(DVBT_BASE+(0x0f<<2),0,1);
        tmp = apb_read_reg(DVBT_BASE+(0x9f<<2));
        //printk(">>>>>>>>>>>>>>>>>>>>>>>>> STOP DUMP DATA To DDR : End Addr %d,Is it overflow?%d\n", tmp>>1, tmp&0x1); 
    }
       
}

void ofdm_read_all_regs()
{
    int i;
    unsigned long tmp;

    for (i=0; i<0xff; i++) {
	tmp = apb_read_reg(DVBT_BASE+0x00+i*4);
	//printk("OFDM Reg (0x%x) is 0x%x\n", i, tmp);        	
    }
    
}

static int dvbt_get_status(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return apb_read_reg(DVBT_BASE+0x0)>>12&1;
}

static int dvbt_ber(void);

static int dvbt_get_ber(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
//	return dvbt_ber();/*unit: 1e-7*/
}

static int dvbt_get_snr(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return apb_read_reg(DVBT_BASE+0x0a)&0x3ff;/*dBm: bit0~bit2=decimal*/
}

static int dvbt_get_strength(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
//	int dbm = dvbt_get_ch_power(demod_sta, demod_i2c);
//	return dbm;
}

static int dvbt_get_ucblocks(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c)
{
//	return dvbt_get_per();
}

struct demod_status_ops* dvbt_get_status_ops(void)
{
	static struct demod_status_ops ops = {
		.get_status = dvbt_get_status,
		.get_ber = dvbt_get_ber,
		.get_snr = dvbt_get_snr,
		.get_strength = dvbt_get_strength,
		.get_ucblocks = dvbt_get_ucblocks,
	};

	return &ops;
}

int app_apb_read_reg(int addr)
{
	return (int)(apb_read_reg(DVBT_BASE+addr<<2));

}

void monitor_isdbt()
{
     int SNR;	
     int SNR_SP          = 500;
     int SNR_TPS         = 0;
     int SNR_CP          = 0;     
     int SNRadjBuff      = 0;
     int timeStamp       = 0;
     int SFO_residual    = 0;
     int SFO_esti        = 0;
     int FCFO_esti       = 0;
     int FCFO_residual   = 0;
     int timing_adj_buff = 0;
     int AGC_Gain        = 0;
     int RF_AGC          = 0;
     int Signal_power    = 0;
     int FECFlag         = 0;
     int EQ_seg_ratio    = 0;
     int tps_0           = 0;
     int tps_1           = 0;
     int tps_2           = 0;

     int time_stamp;
     int SFO;
     int FCFO;
     int timing_adj;
     int RS_CorrectNum;

     int cnt;
     cnt = 0;
     int tmpAGCGain;
     
//     app_apb_write_reg(0x8, app_apb_read_reg(0x8) & ~(1 << 17));  // TPS symbol index update : active high
     time_stamp      =  app_apb_read_reg(0x07)&0xffff       ;
     SNR             =  app_apb_read_reg(0x0a)              ;
     FECFlag         = (app_apb_read_reg(0x00)>>11)&0x3     ;
     SFO             =  app_apb_read_reg(0x47)&0xfff        ;
     SFO_esti        =  app_apb_read_reg(0x60)&0xfff        ;
     FCFO_esti       = (app_apb_read_reg(0x60)>>11)&0xfff   ;
     FCFO            = (app_apb_read_reg(0x26))&0xffffff    ;
     RF_AGC          =  app_apb_read_reg(0x0c)&0x1fff       ;
     timing_adj      =  app_apb_read_reg(0x6f)&0x1fff       ;
     RS_CorrectNum   =  app_apb_read_reg(0xc1)&0xfffff      ;
     Signal_power    = (app_apb_read_reg(0x1b))&0x1ff       ;
     EQ_seg_ratio    =  app_apb_read_reg(0x6e)&0x3ffff      ;    
     tps_0           =  app_apb_read_reg(0x64);
     tps_1           =  app_apb_read_reg(0x65);
     tps_2           =  app_apb_read_reg(0x66)&0xf;
     
     tmpAGCGain      = 0;

     timeStamp       = (time_stamp>>8) * 68 + (time_stamp & 0x7f);
     SFO_residual    = (SFO>0x7ff)? (SFO - 0x1000): SFO;
     FCFO_residual   = (FCFO>0x7fffff)? (FCFO - 0x1000000): FCFO;
     //RF_AGC          = (RF_AGC>0x3ff)? (RF_AGC - 0x800): RF_AGC;
     FCFO_esti       = (FCFO_esti>0x7ff)?(FCFO_esti - 0x1000):FCFO_esti;
     SNR_CP          = (SNR)&0x3ff;
     SNR_TPS         = (SNR>>10)&0x3ff;
     SNR_SP          = (SNR>>20)&0x3ff;
     SNR_SP          = (SNR_SP  > 0x1ff)? SNR_SP - 0x400: SNR_SP;
     SNR_TPS         = (SNR_TPS > 0x1ff)? SNR_TPS- 0x400: SNR_TPS; 
     SNR_CP          = (SNR_CP  > 0x1ff)? SNR_CP - 0x400: SNR_CP;     
     AGC_Gain        = tmpAGCGain >> 4;
     tmpAGCGain      = (AGC_Gain>0x3ff)?AGC_Gain-0x800:AGC_Gain;
     timing_adj      = (timing_adj > 0xfff)? timing_adj - 0x2000: timing_adj;
     EQ_seg_ratio    = (EQ_seg_ratio > 0x1ffff)? EQ_seg_ratio - 0x40000 : EQ_seg_ratio;
        
     printk("T %4x SP %3d TPS %3d CP %3d EQS %8x RSC %4d SFO %4d FCFO %4d Vit %4x Timing %3d SigP %3x FEC %x RSErr %8x ReSyn %x tps %03x%08x"
         ,app_apb_read_reg(0xbf)
         ,SNR_SP
         ,SNR_TPS
         ,SNR_CP
//         ,EQ_seg_ratio
         ,app_apb_read_reg(0x62)
         ,RS_CorrectNum
         ,SFO_residual
         ,FCFO_residual
         ,RF_AGC
         ,timing_adj
         ,Signal_power
         ,FECFlag
         ,app_apb_read_reg(0x0b)
         ,(app_apb_read_reg(0xc0)>>20)&0xff
         ,app_apb_read_reg(0x05)&0xfff
         ,app_apb_read_reg(0x04)
					);
      printk("\n");

     return 0;

}




int find_2(int data, int *table, int len)
{
	  int end;
	  int index;
	  int start;
	  int cnt=0;
	  start = 0;
	  end   = len;
	  //printf("data is %d\n",data);
	  while ((len > 1)&&(cnt < 10))
	  {
	  	 cnt ++ ;
	  	 index = (len/2);
	  	 if (data > table[start + index])
	  	 	{
	  	 	  start = start + index;
	  	 	  len   = len - index - 1;
	  	 	}
	  	 if (data < table[start + index])
	  	 	{
	  	 	  len   = index + 1;
	  	 	}
	  	 else if (data == table[start+index])
	  	 	{
	  	 		start = start + index;
	  	 	  break;
	  	 	}
	  }
	  return start;
}


int read_atsc_all_reg()
{
	int i,j,k;
	j=4;
	unsigned long data;
	printk("system agc is:");		//system agc
	for(i=0xc00;i<=0xc0c;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;
	}
	j=4;
	for(i=0xc80;i<=0xc87;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	printk("\n vsb control is:");		//vsb control
	j=4;
	for(i=0x900;i<=0x905;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x908;i<=0x912;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x917;i<=0x91b;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x980;i<=0x992;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	printk("\n vsb demod is:");		//vsb demod
	j=4;
	for(i=0x700;i<=0x711;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x716;i<=0x720;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}	
	j=4;
	for(i=0x722;i<=0x724;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x726;i<=0x72c;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x730;i<=0x732;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x735;i<=0x751;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x780;i<=0x795;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x752;i<=0x755;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	printk("\n vsb equalizer is:");		//vsb equalizer
	j=4;
	for(i=0x501;i<=0x5ff;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}	
	printk("\n vsb fec is:");		//vsb fec
	j=4;
	for(i=0x601;i<=0x601;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x682;i<=0x685;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	printk("\n qam demod is:");		//qam demod
	j=4;
	for(i=0x1;i<=0x1a;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x25;i<=0x28;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x101;i<=0x10b;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x206;i<=0x207;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	printk("\n qam equalize is:");		//qam equalize
	j=4;
	for(i=0x200;i<=0x23d;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	j=4;
	for(i=0x260;i<=0x275;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	printk("\n qam fec is:");		//qam fec
	j=4;
	for(i=0x400;i<=0x418;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}
	printk("\n system mpeg formatter is:");		//system mpeg formatter
	j=4;
	for(i=0xf00;i<=0xf09;i++){		
		data=atsc_read_reg(i);
		if(j==4){
			printk("\n[addr:0x%x]",i);
		    j=0;
		}
		printk("%02x   ",data);
		j++;	
	}	
	printk("\n\n");
	return 0;
	

}


int check_atsc_fsm_status()
{
	int SNR;	
	int atsc_snr=0;
     int SNR_dB;
     int SNR_table[56] = {0 ,7   ,9   ,11  ,14  ,17  ,22  ,27  ,34  ,43  ,54  ,
     	                  68  ,86  ,108 ,136 ,171 ,215 ,271 ,341 ,429 ,540 ,
     	                  566 ,592 ,620 ,649 ,680 ,712 ,746 ,781 ,818 ,856 ,
     	                  896 ,939 ,983 ,1029,1078,1182,1237,1237,1296,1357,
     	                  1708,2150,2707,3408,4291,5402,6800,8561,10778,13568,
     	                  16312,17081,18081,19081,65536};
     int SNR_dB_table[56] = {360,350,340,330,320,310,300,290,280,270,260,
     	                     250,240,230,220,210,200,190,180,170,160,
     	                     158,156,154,152,150,148,146,144,142,140,
     	                     138,136,134,132,130,128,126,124,122,120,
     	                     110,100,90 ,80 ,70 ,60 ,50 ,40 ,30 ,20 ,
     	                     12 ,10 ,4  ,2  ,0  };
     	                   
     int tmp[3];
     int cr;
     int ck;
     int SM;
     int tni;
     int timeStamp       = 0;

     int cnt;
     cnt = 0;
	 int ber;
	 ber=0;
	 int per;
	 per=0;
     
//     g_demod_mode    = 2;
     tni             =  atsc_read_reg((0x08)>>16);
//	 g_demod_mode    = 4;
     tmp[0]          =  atsc_read_reg(0x0511);
     tmp[1]          =  atsc_read_reg(0x0512);
     SNR             =  (tmp[0]<<8) + tmp[1];
     SNR_dB          = SNR_dB_table[find_2(SNR,SNR_table,56)]; 
     
     tmp[0]          =  atsc_read_reg(0x0780);
     tmp[1]          =  atsc_read_reg(0x0781);
     tmp[2]          =  atsc_read_reg(0x0782);
     cr              =  tmp[0]+ (tmp[1]<<8) + (tmp[2]<<16);
     tmp[0]          =  atsc_read_reg(0x0786);
     tmp[1]          =  atsc_read_reg(0x0787);
     tmp[2]          =  atsc_read_reg(0x0788);
     ck              =  (tmp[0]<<16) + (tmp[1]<<8) + tmp[2];
     ck              =  (ck > 8388608)? ck - 16777216:ck;
     SM              =  atsc_read_reg(0x0980);
//ber per
	atsc_write_reg(0x0601,atsc_read_reg(0x0601)&(~(1<<3)));
	atsc_write_reg(0x0601,atsc_read_reg(0x0601)|(1<<3));
	ber=atsc_read_reg(0x0683)+(atsc_read_reg(0x0682)<<8);
	per=atsc_read_reg(0x0685)+(atsc_read_reg(0x0684)<<8);

//	read_atsc_all_reg();
     
     printk("INT %x SNR %x SNRdB %d.%d FSM %x cr %d ck %d,ber is %d, per is %d\n"
         ,tni
         ,SNR
         ,(SNR_dB/10)
         ,(SNR_dB-(SNR_dB/10)*10)
         ,SM
         ,cr
         ,ck
         ,ber
         ,per
					);
     atsc_snr=(SNR_dB/10);				
     return atsc_snr;



 /*   unsigned long sm,snr1,snr2,snr;
    static int fec_lock_cnt = 0;

    delay_us(10000);    
    sm = atsc_read_reg(0x0980);
    snr1 = atsc_read_reg(0x0511)&0xff;
    snr2 = atsc_read_reg(0x0512)&0xff;
    snr  = (snr1 << 8) + snr2;

    printk(">>>>>>>>>>>>>>>>>>>>>>>>> OFDM FSM %x    SNR %x\n", sm&0xff, snr);  

    if (sm == 0x79) stimulus_finish_pass();*/
}
