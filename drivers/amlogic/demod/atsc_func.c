#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include "aml_demod.h"
#include "demod_func.h"


typedef struct atsc_cfg {
    int adr;
    int dat;
    int rw;
} atsc_cfg_t;


#if 0
int atsc_status(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c, 
		struct aml_demod_sts *demod_sts)
{
    // if parameters are needed to calc, pass the struct to func.
    // all small funcs like read_snr() should be static.
    
    demod_sts->ch_snr = apb_read_reg(2, 0x0a);
    demod_sts->ch_per = atsc_get_per();
    demod_sts->ch_pow = atsc_get_ch_power(demod_sta, demod_i2c);
    demod_sts->ch_ber = apb_read_reg(2, 0x0b);
    demod_sts->ch_sts = apb_read_reg(2, 0);
    demod_sts->dat0   = atsc_get_avg_per();
    demod_sts->dat1   = apb_read_reg(2, 0x06);    
    return 0;
}


static int atsc_get_status(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return apb_read_reg(2, 0x0)>>12&1;

}

static int atsc_ber(void);

static int atsc_get_ber(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return atsc_ber();/*unit: 1e-7*/
}

static int atsc_get_snr(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	return apb_read_reg(2,0x0a)&0x3ff;/*dBm: bit0~bit2=decimal*/
}

static int atsc_get_strength(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c) 
{
	int dbm = atsc_get_ch_power(demod_sta, demod_i2c);
	return dbm;
}

static int atsc_get_ucblocks(struct aml_demod_sta *demod_sta, 
		struct aml_demod_i2c *demod_i2c)
{
	return atsc_get_per();
}


struct demod_status_ops* atsc_get_status_ops(void)
{
	static struct demod_status_ops ops = {
		.get_status = atsc_get_status,
		.get_ber = atsc_get_ber,
		.get_snr = atsc_get_snr,
		.get_strength = atsc_get_strength,
		.get_ucblocks = atsc_get_ucblocks,
	};

	return &ops;
}

#endif

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


int monitor_atsc()
{
     int SNR;	
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
     
//     g_demod_mode    = 2;
     tni             =  apb_read_reg(2,(0x08)>>16);
//	 g_demod_mode    = 4;
     tmp[0]          =  apb_read_reg(4,(0x0511));
     tmp[1]          =  apb_read_reg(4,(0x0512));
     SNR             =  (tmp[0]<<8) + tmp[1];
     SNR_dB          = SNR_dB_table[find_2(SNR,SNR_table,56)]; 
     
     tmp[0]          =  apb_read_reg(4,(0x0780));
     tmp[1]          =  apb_read_reg(4,(0x0781));
     tmp[2]          =  apb_read_reg(4,(0x0782));
     cr              =  tmp[0]+ (tmp[1]<<8) + (tmp[2]<<16);
     tmp[0]          =  apb_read_reg(4,(0x0786));
     tmp[1]          =  apb_read_reg(4,(0x0787));
     tmp[2]          =  apb_read_reg(4,(0x0788));
     ck              =  (tmp[0]<<16) + (tmp[1]<<8) + tmp[2];
     ck              =  (ck > 8388608)? ck - 16777216:ck;
     SM              =  apb_read_reg(4,(0x0980));
     
     printk("INT %x SNR %x SNRdB %d.%d FSM %x cr %d ck %d\n"
         ,tni
         ,SNR
         ,(SNR_dB/10)
         ,(SNR_dB-(SNR_dB/10)*10)
         ,SM
         ,cr
         ,ck
					);
					
     return 0;

}




void atsc_initial(void)
{
    int i;
    
    atsc_cfg_t list[21] = {
    {0x0733, 0x00, 0},
    {0x0734, 0xff, 0},
    {0x0716, 0x06, 0},
    {0x05e7, 0x00, 0},
    {0x05e8, 0x00, 0},
    {0x0f06, 0x00, 0},
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
            apb_read_reg(4,list[i].adr);
        else
            apb_write_reg(4,list[i].adr, list[i].dat);
    }
}



int atsc_set_ch(struct aml_demod_sta *demod_sta, 
			struct aml_demod_i2c *demod_i2c, 
			struct aml_demod_atsc *demod_atsc)
{
	int ret = 0;
	u8 demod_mode = 4;
	u8 bw, sr, ifreq, agc_mode;
	u32 ch_freq;
	demod_i2c->tuner=7;
	bw		 = demod_atsc->bw;
	sr		 = demod_atsc->sr;
	ifreq	 = demod_atsc->ifreq;
	agc_mode = demod_atsc->agc_mode;
	ch_freq  = demod_atsc->ch_freq;
	demod_mode = demod_atsc->dat0;

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
	demod_sta->symb_rate = 0; // TODO

	// Set Tuner
	tuner_set_ch(demod_sta, demod_i2c);

	if (demod_mode ==0)
	{
		printk("QAM 8M mode\n");
	}
	else if ((demod_mode == 1)||(demod_mode == 2))
	{
	//	dvbt_reg_initial(demod_sta, demod_dvbt);
		
		  printk("DVBT/ISDBT mode\n");
	}
	
	else if (demod_mode == 3)
	{
	//	  dtmb_initial();
		printk("DTMB mode\n");
	}
	else if (demod_mode == 4)
	{
  		atsc_initial();
		printk("ATSC mode\n");
	}

	return ret;
}





