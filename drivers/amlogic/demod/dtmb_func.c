
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include "aml_demod.h"
#include "demod_func.h"

extern void app_apb_write_reg(int addr, int data);
extern int app_apb_read_reg(int addr);
#define printf printk

#if 0
int dtmb_status(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c, 
		struct aml_demod_sts *demod_sts)
{
    // if parameters are needed to calc, pass the struct to func.
    // all small funcs like read_snr() should be static.
    
    demod_sts->ch_snr = apb_read_reg(2, 0x0a);
    demod_sts->ch_per = dtmb_get_per();
    demod_sts->ch_pow = dtmb_get_ch_power(demod_sta, demod_i2c);
    demod_sts->ch_ber = apb_read_reg(2, 0x0b);
    demod_sts->ch_sts = apb_read_reg(2, 0);
    demod_sts->dat0   = dtmb_get_avg_per();
    demod_sts->dat1   = apb_read_reg(2, 0x06);    
    return 0;
}


static int dtmb_get_status(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return apb_read_reg(2, 0x0)>>12&1;

}

static int dtmb_ber(void);

static int dtmb_get_ber(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return dtmb_ber();/*unit: 1e-7*/
}

static int dtmb_get_snr(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return apb_read_reg(2,0x0a)&0x3ff;/*dBm: bit0~bit2=decimal*/
}

static int dtmb_get_strength(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	int dbm = dtmb_get_ch_power(demod_sta, demod_i2c);
	return dbm;
}

static int dtmb_get_ucblocks(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c)
{
	return dtmb_get_per();
}


struct demod_status_ops* dtmb_get_status_ops(void)
{
	static struct demod_status_ops ops = {
		.get_status = dtmb_get_status,
		.get_ber = dtmb_get_ber,
		.get_snr = dtmb_get_snr,
		.get_strength = dtmb_get_strength,
		.get_ucblocks = dtmb_get_ucblocks,
	};

	return &ops;
}

#endif

static int dtmb_initial()
{
		int cmd, i, tmp,tmp1,bandwidth,IF_KHZ,MC,afifo_nco, afifo_bypass,tmp2, ret;
		int alpha, beta;
		int status[2],uart_error;
		int tb_depth,tb_start,tb_part0,tb_part1;
		//float ftmp;
		int  AD_clk, sys_clk, symbol_rate;
		int if_freq, src_nco, pnphase_sfo_modify, fe_modify, sfo_pn0,sfo_pn1,sfo_pn2,cfo_pn0, cfo_pn1,cfo_pn2;
		int dis;
		int addr;
		int read_from_file = 0;
		int times, phs_track_in_smma, SMMA_MMA;
		int reg_0x54,reg_0x50;
		int pm_select_ch_gain, pm_select_gain, pm_select_noise_gain, pm_select_noise_gain_absolut, AD_clk1, sys_clk1;
		int long_pre, long_pst, short_pre, short_pst;
		int tps_set, constell,coderate, interleavingmode, freq_reverse;
		int reg_sfo_dist,sfo_dist,sfo_sat_shift;
		bandwidth=0;//1;//
		AD_clk=25000; //20000;// KHz
		sys_clk=82000; //64000;// KHz
		alpha=28;
		beta=4;
		afifo_bypass=0;
		int size =0;
		int h_len = 360;
		int pm_gain_delta_ctrl = 1;
		int dagc_gain_ctrl = 4; // *4

		printf("bandwidth is %d, AD_clk is %d, sys_clk is %d, alpha is %d, beta is %d, afifo_bypass is %d\n", 
		bandwidth, AD_clk, sys_clk, alpha, beta, afifo_bypass);

		char acf_file_name[200];
		int acf[67];
		int acf_bitwidth[22] = {4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 8, 9, 10, 11};
		int result = 0;
		struct file* filp = NULL;
		mm_segment_t old_fs;
		char	buf[1024];
#if 0
		int testcode=0;
		int ff;
		int gg=0x10;
	
		app_apb_write_reg(gg,0x999);
		testcode=app_apb_read_reg(gg);
		printk("testcode is %x\n",testcode);
		msleep(1000);
#endif		


		app_apb_write_reg(0x01, app_apb_read_reg(0x01) | (1 << 0));//dtmb reset
		app_apb_write_reg(0x01, app_apb_read_reg(0x01) &~ (1 << 0));

		// configure ACF	 AD --25M, Bandwidth -- 8M
		app_apb_write_reg(26,	0x003782f4);
		app_apb_write_reg(27,	0x001b004c);
		app_apb_write_reg(28,	0x0000036a);
		app_apb_write_reg(29,	0x0034a3b6);
		app_apb_write_reg(30,	0x0000003a);
		app_apb_write_reg(31,	0x00070044);
		app_apb_write_reg(32,	0x00eeb6c2);
		app_apb_write_reg(33,	0x0000007c);
		app_apb_write_reg(34,	0x32381062);
		app_apb_write_reg(35,	0x526a1026);
		app_apb_write_reg(36,	0x001a3a22);
		app_apb_write_reg(37,	0x263e161a);
		app_apb_write_reg(38,	0x08322a34);
		app_apb_write_reg(39,	0x08140e3e);
		app_apb_write_reg(40,	0x0010121e);
		app_apb_write_reg(41,	0x0c0e0418);
		app_apb_write_reg(42,	0x141a040a);
		app_apb_write_reg(43,	0x061e1818);
		app_apb_write_reg(44,	0x00662422);
		app_apb_write_reg(45,	0x2620ef04);


		IF_KHZ = 5000; // KHZ

		if(bandwidth == 0) // 8M
		{
			symbol_rate = 7560; // KHZ
		}
		else if(bandwidth == 1) // 6M
		{
			symbol_rate = 5670; // KHZ
		}
		else
			printf("Bandwidth is not support\n");  

		  
		// afifo configuration
		//afifo_nco = (int)(AD_clk / sys_clk * 256.0 + 4);
		afifo_nco = (int)(AD_clk * 256 / sys_clk + 4);
		tmp = app_apb_read_reg(16) & 0x1ff;
		app_apb_write_reg(16, app_apb_read_reg(16) | 1 << 8);
		if(afifo_bypass)
			app_apb_write_reg(16, app_apb_read_reg(16) | 1 << 9);
		else
			app_apb_write_reg(16, app_apb_read_reg(16) &~ (1 << 9));
		app_apb_write_reg(16, (app_apb_read_reg(16) &~ 0xff) | (afifo_nco&0xff));

		// IF configure
		//if_freq = (int)(1.0*IF_KHZ * (1 << 15) / AD_clk + 0.5);
		if_freq = (int)(IF_KHZ * (1 << 15) / AD_clk);
		app_apb_write_reg(21, (app_apb_read_reg(21) &~ 0xffff) | (if_freq&0xffff));

		// src configuration
		//src_nco = (int)(1.0*AD_clk * (1 << 20) / symbol_rate + 0.5);
		src_nco = (int)(AD_clk * (1 << 16) / symbol_rate * (1 << 4));
		app_apb_write_reg(46, (app_apb_read_reg(46) &~ 0x7fffff) | (src_nco&0x7fffff));

		// pnphase_sfo_modify
		// pnphase_sfo_modify = (int)(1.0*AD_clk / symbol_rate * 1024.0 + 0.5);
		pnphase_sfo_modify = (int)(AD_clk * 1024 / symbol_rate); 
		app_apb_write_reg(61, (app_apb_read_reg(61) &~ 0xffff0000) | ((pnphase_sfo_modify&0xffff) << 16));

		// fe_modify
		//fe_modify = (int)(1.0*symbol_rate/AD_clk/256*(1<<24) + 0.5);
		fe_modify= (int)(symbol_rate*(1<<16)/AD_clk);
		app_apb_write_reg(59, (app_apb_read_reg(59) &~ 0xffff0000) | ((fe_modify&0xffff) << 16));
#if    1


		//sfo_dist = 32;//128; 
		reg_sfo_dist = 3; //	PN420			 PN595				 PN945	  
						  // 0: 64, 128 	128, 256			128, 256
						  // 1: 32, 64		64,  128			64, 128 
						  // 2: 16, 32		32,  64 			32, 64
						  // 3: 8,	16		16,  32 			16, 32
		// sfo cfo pn modify
		//sfo_pn0 = (int)(128.0 * 1000000 * AD_clk / 128 / 4200 / symbol_rate + 0.5);
		//sfo_pn1 = (int)(256.0 * 1000000 * AD_clk / 128 / 4375 / symbol_rate + 0.5);
		//sfo_pn2 = (int)(256.0 * 1000000 * AD_clk / 128 / 4725 / symbol_rate + 0.5);
		//cfo_pn0 = (int)(1.0*symbol_rate*1000000.0/4200/AD_clk*16+0.5);
		//cfo_pn1 = (int)(1.0*symbol_rate*1000000.0/4375/AD_clk*16+0.5);
		//cfo_pn2 = (int)(1.0*symbol_rate*1000000.0/4725/AD_clk*16+0.5);
		sfo_pn0 = (int)(238 * AD_clk / symbol_rate);
		sfo_pn1 = (int)(457 * AD_clk / symbol_rate);
		sfo_pn2 = (int)(423 * AD_clk / symbol_rate);
		cfo_pn0 = (int)(3810 * symbol_rate / AD_clk);
		cfo_pn1 = (int)(3657 * symbol_rate / AD_clk);
		cfo_pn2 = (int)(3386 * symbol_rate / AD_clk);

		app_apb_write_reg(62, ((sfo_pn0&0xffff) << 16) +  (cfo_pn0&0xffff));
		app_apb_write_reg(63, ((sfo_pn1&0xffff) << 16) +  (cfo_pn1&0xffff));
		app_apb_write_reg(64, ((sfo_pn2&0xffff) << 16) +  (cfo_pn2&0xffff));

#else            
		sfo_dist = 32;//128; 
		reg_sfo_dist = sfo_dist == 32 ? 0 : sfo_dist == 64 ? 1 : sfo_dist == 128 ? 2 : sfo_dist == 256 ? 3 : 0;
		// sfo cfo pn modify
		sfo_pn0 = (int)(128.0 * 1000000 * AD_clk / sfo_dist / 4200 / symbol_rate + 0.5);
		sfo_pn1 = (int)(256.0 * 1000000 * AD_clk / sfo_dist / 4375 / symbol_rate + 0.5);
		sfo_pn2 = (int)(256.0 * 1000000 * AD_clk / sfo_dist / 4725 / symbol_rate + 0.5);
		cfo_pn0 = (int)(symbol_rate*1000000.0/4200/AD_clk*16+0.5);
		cfo_pn1 = (int)(symbol_rate*1000000.0/4375/AD_clk*16+0.5);
		cfo_pn2 = (int)(symbol_rate*1000000.0/4725/AD_clk*16+0.5);
		app_apb_write_reg(62, ((sfo_pn0&0xffff) << 16) +  (cfo_pn0&0xffff));
		app_apb_write_reg(63, ((sfo_pn1&0xffff) << 16) +  (cfo_pn1&0xffff));
		app_apb_write_reg(64, ((sfo_pn2&0xffff) << 16) +  (cfo_pn2&0xffff));
#endif            
		// sfo_sat_shift
		sfo_sat_shift = 6;
		app_apb_write_reg(0x41, (app_apb_read_reg(0x41) &~ 0x30f) | (((sfo_sat_shift&0xf) << 0) + ((reg_sfo_dist&0x3) << 8)));

		//app_apb_write_reg(0x41, 0xac026);
		//app_apb_write_reg(0x41, 0xbd027); // step 0.5

		// pm accu time
		//app_apb_write_reg(0x44, 0x05af);	// set h_len 90
		//app_apb_write_reg(0x44, 0x0b4f);	// set h_len 180
		app_apb_write_reg(0x44, (app_apb_read_reg(0x44) &~ 0x3ff0) | (((h_len&0x3ff) << 4) + 0xf));

		// path manager gain setting
		if(read_from_file)
		{
			tmp = ((pm_select_ch_gain&0x3) << 30) + ((pm_select_gain&0x7) << 27) + ((pm_select_noise_gain&0x7) << 24) + ((pm_select_noise_gain_absolut&0x3) << 13);
			app_apb_write_reg(67, (app_apb_read_reg(67) &~ 0xff006000) | tmp);
			app_apb_write_reg(0x42, ((long_pre&0x3f) << 24) + ((long_pst&0x3f) << 16) + ((short_pre&0x1f) << 8) + ((short_pst&0x1f) << 0));
			app_apb_write_reg(0x44, (app_apb_read_reg(0x44) &~ 0x20000 ) | ((pm_gain_delta_ctrl&0x1) << 17));
		}
		else
		{
			// pm short_pre long_pre
			//app_apb_write_reg(0x42, 0x0f0f0a0a);
			pm_select_gain = 3;
			pm_select_ch_gain = 1;//0;
			pm_select_noise_gain = 6;
			pm_select_noise_gain_absolut = 1;//0;
			pm_gain_delta_ctrl = 1;
			long_pre = 15;
			long_pst = 15;
			short_pre = 10;
			short_pst = 10;
			tmp = ((pm_select_ch_gain&0x3) << 30) + ((pm_select_gain&0x7) << 27) + ((pm_select_noise_gain&0x7) << 24) + ((pm_select_noise_gain_absolut&0x3) << 13);
			app_apb_write_reg(67, (app_apb_read_reg(67) &~ 0xff006000) | tmp);
			app_apb_write_reg(0x42, ((long_pre&0x3f) << 24) + ((long_pst&0x3f) << 16) + ((short_pre&0x1f) << 8) + ((short_pst&0x1f) << 0));
			app_apb_write_reg(0x44, (app_apb_read_reg(0x44) &~ 0x20000 ) | ((pm_gain_delta_ctrl&0x1) << 17));
		}

		tmp = app_apb_read_reg(79);
		tmp = tmp | (0x1 << 29);
		tmp = tmp &~ (0x1f << 24);
		tmp = tmp | ((alpha & 0x1f) << 24);
		tmp = tmp | (0x1 << 21);
		tmp = tmp &~ (0x1f << 16);
		tmp = tmp | ((beta & 0x1f) << 16);
		app_apb_write_reg(79, tmp);    
			
		// enable dagc 
		//app_apb_write_reg(49, app_apb_read_reg(49) &~(1 << 28));
             tmp = app_apb_read_reg(49) &~ 0xff0000;
             dagc_gain_ctrl = 4; // *4
             app_apb_write_reg(49, tmp | ((dagc_gain_ctrl&0xff) << 16));
             
		//disable iqib
		app_apb_write_reg(51, app_apb_read_reg(51) | (1 << 24));
		app_apb_write_reg(51, app_apb_read_reg(51) &~ (1 << 25));

		// set ldpc iteration times auto
		app_apb_write_reg(71, app_apb_read_reg(71) | (1 << 16));
		app_apb_write_reg(71, app_apb_read_reg(71) | (1 << 20)); // fast ts mode in low sys clk, set low ts_mode in high sys clk. In FPGA, it should be fast.

               // set fec_lost_cfg 
               tmp = app_apb_read_reg(72);
               tmp = tmp &~ 0xe00000;
               tmp = tmp | (0x7 << 21);
               app_apb_write_reg(72, tmp);
            
		int che_cci_bypass = 1;
		int demap_seg_bypass = 1;
		app_apb_write_reg(0x50, (app_apb_read_reg(0x50) &~ 0x2000 ) | ((che_cci_bypass&0x1) << 13));			
		app_apb_write_reg(0x54, (app_apb_read_reg(0x54) &~ 0x1 ) | ((demap_seg_bypass&0x1) << 0));	 

		// set waiting time of ldpc sync
		app_apb_write_reg(0xd, 0x41c8258);

		printf("afifo config is %x\n", app_apb_read_reg(16) & 0x1ff);
		//printf("enable dagc \n");
		printf("disable iqib \n");
		printf("pm_ch_gain is %d, pm_gain is %d, pm_noise_gain is %d, pm_noise_gain_absolute is %d\n", (app_apb_read_reg(67)>>30)&0x3, (app_apb_read_reg(67)>>27)&0x7, (app_apb_read_reg(67)>>24)&0x7, (app_apb_read_reg(67)>>13)&0x3);
		printf("long_pre is %d, long_pst is %d, short_pre is %d, short_pst is %d\n", (app_apb_read_reg(0x42)>>24)&0x3f, (app_apb_read_reg(0x42)>>16)&0x3f, (app_apb_read_reg(0x42)>>8)&0x1f, (app_apb_read_reg(0x42)>>0)&0x1f);
		//	  read_DTMB_information();			

		app_apb_write_reg(0x01, app_apb_read_reg(0x01) | (0x3 << 0));  // internal reset not including register.
              app_apb_write_reg(0x01, app_apb_read_reg(0x01) &~ (0x3 << 0));


}




int dtmb_set_ch(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c, 
		struct aml_demod_dtmb *demod_dtmb)
{
    int ret = 0;
    u8 demod_mode = 0;
    u8 bw, sr, ifreq, agc_mode;
    u32 ch_freq;

    bw       = demod_dtmb->bw;
    sr       = demod_dtmb->sr;
    ifreq    = demod_dtmb->ifreq;
    agc_mode = demod_dtmb->agc_mode;
    ch_freq  = demod_dtmb->ch_freq;
    demod_mode = demod_dtmb->dat0;

    // Set registers
    //////////////////////////////////////
    // bw == 0 : 8M 
    //       1 : 7M
    //       2 : 6M
    //       3 : 5M
    // sr == 0 : 20.7M
    //       1 : 20.8333M
    //       2 : 28.5714M
    //       3 : 45M
    // agc_mode == 0: single AGC
    //             1: dual AGC
    //////////////////////////////////////
    if (bw > 3) {
    	printk("Error: Invalid Bandwidth option %d\n", bw);
	bw = 0;
	ret = -1;
    }

    if (sr > 3) {
    	printk("Error: Invalid Sampling Freq option %d\n", sr);
	sr = 2;
	ret = -1;
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

    // if (ret != 0) return ret;
	#ifndef CONFIG_AM_DEMOD_FPGA_VER
	    // Set DVB-T
	    (*DEMOD_REG0) |= 1;
	#endif
    //demod_sta->dvb_mode  = 1;
    demod_sta->ch_mode   = 0; // TODO
    demod_sta->agc_mode  = agc_mode;
    demod_sta->ch_freq   = ch_freq;
    demod_sta->dvb_mode  = demod_mode;
    if (demod_i2c->tuner == 1) 
	demod_sta->ch_if = 36130; 
    else if (demod_i2c->tuner == 2) 
	demod_sta->ch_if = 4570; 
    else if (demod_i2c->tuner == 3)
	demod_sta->ch_if = 4000;// It is nouse.(alan)
    else if (demod_i2c->tuner == 7)
	demod_sta->ch_if = 5000;//silab 5000kHz IF

        
    demod_sta->ch_bw     = (8-bw)*1000; 
    demod_sta->symb_rate = 0; // TODO

    // Set Tuner
    tuner_set_ch(demod_sta, demod_i2c);

    if (demod_mode ==0)
    {
    	printk("QAM 8M mode\n");
    }
    else if ((demod_mode == 1)||(demod_mode == 2))
    {
   //     dvbt_reg_initial(demod_sta, demod_dvbt);
        
    	  printk("DVBT/ISDBT mode\n");
    }
	
	else if (demod_mode == 3)
    {
	    dtmb_initial();
	    printk("DTMB mode\n");
    }
    else if (demod_mode == 4)
    {
    //	atsc_initial();
    	printk("ATSC mode\n");
    }

    return ret;
}

