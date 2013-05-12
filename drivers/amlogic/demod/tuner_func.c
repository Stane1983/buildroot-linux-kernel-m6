#include <linux/kernel.h>
#include <linux/i2c.h>

#include <dvb_frontend.h>

#include "aml_demod.h"
#include "demod_func.h"

#include "mxl/MxL5007_API.h"
#include "rda/RDA5880_adp.h"
#include "si2176/si2176_func.h"
static int configure_first=-1;

#if 1

static int set_tuner_TDA18273(struct aml_demod_sta *demod_sta, 
			     struct aml_demod_i2c *adap)

{
	unsigned long ch_freq;
	int ch_if;
	int ch_bw;
	ch_freq = demod_sta->ch_freq; // kHz
	ch_if   = demod_sta->ch_if;   // kHz 
	ch_bw   = demod_sta->ch_bw / 1000; // MHz
	tda18273_tuner_set_frequncy(ch_freq*KHz,ch_bw);
}

static int set_tuner_MxL5007(struct aml_demod_sta *demod_sta, 
			     struct aml_demod_i2c *adap)
{
    MxL_ERR_MSG            Status = MxL_OK;
    int                    RFSynthLock , REFSynthLock;
    //SINT32               RF_Input_Level;
    SINT32                 out = 0;
    MxL5007_TunerConfigS   myTuner;
    unsigned long ch_freq;
    int ch_if;
    int ch_bw;

    ch_freq = demod_sta->ch_freq; // kHz
    ch_if   = demod_sta->ch_if;   // kHz 
    ch_bw   = demod_sta->ch_bw / 1000; // MHz

    printk("Set Tuner MxL5007 to %ld kHz\n", ch_freq);

    myTuner.I2C_Addr = adap->addr; // MxL_I2C_ADDR_96;
    myTuner.Mode = demod_sta->dvb_mode==0 ? 
	MxL_MODE_CABLE : MxL_MODE_DVBT;
    myTuner.IF_Freq = ch_if < 5000 ? MxL_IF_4_57_MHZ :
	ch_if < 9900 ? MxL_IF_9_1915_MHZ : MxL_IF_36_15_MHZ;

    myTuner.IF_Diff_Out_Level = -8;
    myTuner.Xtal_Freq         = MxL_XTAL_24_MHZ;
    myTuner.ClkOut_Setting    = MxL_CLKOUT_ENABLE;
    myTuner.ClkOut_Amp        = MxL_CLKOUT_AMP_0;
    myTuner.I2C_adap          = (u32)adap;

    Status = MxL_Tuner_Init(&myTuner);
    if (Status!=0) {
	printk("Error: MxL_Tuner_Init fail!\n");
	return -1;
    }
    Status = MxL_Tuner_RFTune(&myTuner, ch_freq*KHz, ch_bw);
    if (Status!=0) {
	printk("Error: MxL_Tuner_RFTune fail!\n");
	return -1;
    }
    MxL_RFSynth_Lock_Status(&myTuner,&RFSynthLock);
    MxL_REFSynth_Lock_Status(&myTuner, &REFSynthLock);

    // MxL_Stand_By(&myTuner);
    // MxL_Wake_Up(&myTuner);
	
    if (RFSynthLock!=0)
	out = out + 1;
    if (REFSynthLock!=0)
	out = out + 2;

    return (out);
}

static int set_tuner_TD1316(struct aml_demod_sta *demod_sta, 
			    struct aml_demod_i2c *adap)
{
    int ret = 0;
    unsigned char data[4];
    unsigned long ch_freq, ftmp;
    unsigned char CB1, SB, CB2, AB;
    int cp_mode = 0;
    int ch_bw;
    int agc_mode;
    struct i2c_msg msg;

    msg.addr = adap->addr;
    msg.flags = 0;
    msg.len = 4;
    msg.buf = data;
    
    ch_freq = demod_sta->ch_freq;
    ch_bw = demod_sta->ch_bw;
    agc_mode = demod_sta->agc_mode;

    printk("Set Tuner TD1316 to %ld kHz\n", ch_freq);

    ftmp = (ch_freq*1000 + 36350*1000 + 83350)/166700;
    data[0] = ftmp>>8&0xff;
    data[1] = ftmp&0xff;

    /**********************************************************
    Low CP : (MHz)
    1) 44  <= freq <=  171;  low band
    2) 132 <= freq <= 471 ;  middle band
    3) 405 <= freq <= 899 ;  high band
    Medium CP:(MHz)
    1) 44  <= freq <=  171;  low band
    2) 132 <= freq <= 471 ;  middle band
    3) 405 <= freq <= 899 ;  high band
    High CP:(MHz)
    1) 44  <= freq <=  171;  low band
    2) 132 <= freq <= 471 ;  middle band
    3) 554 <= freq <= 900 ; high band
    *********************************************************/

    // Control Data Bytes
    //printk("Choose Charge Pump? Low is 0, Medium is 1, high is others");
    //scanf("%d",&cp_mode);
    if (cp_mode == 0) {
	CB1 = 0xbc; CB2 = 0x9c; // Low CP & 166.7Khz PLL stepsize 
    }
    else if (cp_mode == 1) {
	CB1 = 0xf4; CB2 = 0xdc;  // Medium CP & 166.7Khz PLL stepsize, Recommend for PAL
    }
    else { 
	CB1 = 0xfc; CB2 = 0xdc;  // High CP & 166.7Khz PLL stepsize, 
    }
	  
    // Switchport Bytes
    SB = 0;
    if ( ((ch_freq/1000) <= 152))   SB = SB | 0x1;// Low band
    else if (ch_freq/1000 <= 438 )  SB = SB | 0x2;// mid band
    else if (ch_freq/1000 <= 900 )  SB = SB | 0x4;// high band
    else {
	SB = SB | 0x3;// standby 
	printk("Error: Tuner set to Standby mode!\n");
    }

    SB = (ch_bw==8000)? (SB | 0x8) : SB; // set 7/8M SAW filter	
    //SB = (ch_bw==8)? (SB | 0xb) : SB; // set 7/8M SAW filter	
 
    // AB 
    // AB = 0x50; // For PAL applications AGC loop top -12dB
    // AB = 0x20; // For DVB-T Medium AGC loop top -3dB
    // AB = 0x30; // For DVB-T Low    AGC loop top -6dB  
    // AB = 0x70; // The tuner internal AGC detector is disabled. 
    // With no external AGC voltage applied to the tuner, the RF-gain is always set to maximum.   
    // AB = 0x10;  // For High (0dB reference);	  

    AB = agc_mode == 1 ? 0x60 : 0x50;

    data[2] = CB1;
    data[3] = SB;
	  
    ret = am_demod_i2c_xfer(adap, &msg, 1);
    if (ret == 0) {
	return ret;
    }

    data[2]  = CB2;
    data[3]  = AB;
    ret = am_demod_i2c_xfer(adap, &msg, 1);
    return ret;
}
static int set_tuner_si2176(struct aml_demod_sta *demod_sta, 
			    struct aml_demod_i2c *adap)
{
	struct aml_tuner_sys tuner_sys;
	tuner_sys.amp=25;
	tuner_sys.mode=3;
    int ret = 0;
    si2176_cmdreplyobj_t rsp;
    si2176_common_reply_struct crsp;
    si2176_propobj_t prop;
    unsigned long ch_freq;
    ch_freq = demod_sta->ch_freq * 1000;
    printk("RF of Tuner Si2176 to %ld Hz\n", ch_freq);
    if(configure_first == -1)
    {
		init_tuner_skyworth(&tuner_sys);
    	printk("first in si2176_init\n");
    /*	  ret = si2176_init(adap, &rsp, &crsp);
        ret = si2176_configure(adap, &prop, &rsp, &crsp, (int)demod_sta->ch_mode);*/
        configure_first = 1;
    }
	printk("adap->addr is %x,adap->scl_oe is %d\n",adap->addr,adap->scl_oe);

//	adap->addr = 0x60;
//	adap->scl_oe = 0;
//	adap->i2c_priv = i2c_get_adapter(0);
//	if(!adap->i2c_priv){
//		printk("can not get i2c adapter[%d] \n");
//		return -1;
//	}

    printk("----------- si2176 configure freq start -----------\n");
    ret = si2176_tune(adap, SI2176_TUNER_TUNE_FREQ_CMD_MODE_DTV, ch_freq, &rsp, &crsp);
    printk("----------- si2176 configure freq end -----------\n");
    mdelay(1000);
    ret = si2176_tuner_status(adap, 1, &rsp);
    if(ret > 0)
        printk("read tuner status error \n");
    else
            //rsp->tuner_status.rssil    =   (( ( (rspbytebuffer[2]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSIL_LSB    ) & SI2176_TUNER_STATUS_RESPONSE_RSSIL_MASK    );
            //rsp->tuner_status.rssih    =   (( ( (rspbytebuffer[2]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSIH_LSB    ) & SI2176_TUNER_STATUS_RESPONSE_RSSIH_MASK    );
            //rsp->tuner_status.rssi     = (((( ( (rspbytebuffer[3]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSI_LSB     ) & SI2176_TUNER_STATUS_RESPONSE_RSSI_MASK) <<SI2176_TUNER_STATUS_RESPONSE_RSSI_SHIFT ) >>SI2176_TUNER_STATUS_RESPONSE_RSSI_SHIFT     );
            //rsp->tuner_status.freq     =   (( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 ) | (rspbytebuffer[6]  << 16 ) | (rspbytebuffer[7]  << 24 )) >> SI2176_TUNER_STATUS_RESPONSE_FREQ_LSB     ) & SI2176_TUNER_STATUS_RESPONSE_FREQ_MASK     );
        printk("  ****************** rssi is %ddBm, freq is %d ***************************** \n", rsp.tuner_status.rssi-256, rsp.tuner_status.freq);   
        
    return ret;
}

#endif

static int set_tuner_DCT7070X(struct aml_demod_sta *demod_sta, 
			      struct aml_demod_i2c *adap)
{
    int ret = 0;
    unsigned char data[6];
    unsigned long ch_freq, ftmp;
    struct i2c_msg msg;
    
    ch_freq = demod_sta->ch_freq;
    //printk("Set Tuner DCT7070X to %ld kHz\n", ch_freq);
		
    ftmp = (ch_freq+36125)*10/625;  // ftmp=(ch_freq+GX_IF_FREQUENCY)
    data[0] = ftmp>>8&0xff;	
    data[1] = ftmp&0xff;	
    data[2] = 0x8b;              // 62.5 kHz 
		
    if (ch_freq < 153000) 
	data[3] = 0x01;
    else if (ch_freq < 430000) 
	data[3] = 0x06;
    else 
	data[3] = 0x0c; 
		
    data[4] = 0xc3;
		
    msg.addr = adap->addr;
    msg.flags = 0; // I2C_M_IGNORE_NAK; 
    msg.len = 5;
    msg.buf = data;

    ret = am_demod_i2c_xfer(adap, &msg, 1);

    return ret;
}

int tuner_set_ch(struct aml_demod_sta *demod_sta, struct aml_demod_i2c *adap)
{
    int ret = 0;
    
    //adap->tuner = 3;// For FJ2207 Tuner

    switch (adap->tuner) {
    case 0 : // NULL
	printk("Warning: NULL Tuner\n");
	break;
	
    case 1 : // DCT
	ret = set_tuner_DCT7070X(demod_sta, adap);
	break;

    case 2 : // Maxliner
	ret = set_tuner_MxL5007(demod_sta, adap);
	break;

    case 3 : // NXP
	ret = set_tuner_fj2207(demod_sta, adap);
	break;
/*
    case 4 : // TD1316
	ret = set_tuner_TD1316(demod_sta, adap);
	break;
*/

    case 5:
	ret = set_tuner_RDA5880(demod_sta, adap);
	break;
	
	case 6:
	ret = set_tuner_TDA18273(demod_sta, adap);
	break;
	
	case 7 : //Si2176
	ret = set_tuner_si2176(demod_sta, adap);
	break;
	
    default :
	return -1;
    }

    return 0;
}

int tuner_get_ch_power(struct aml_demod_i2c *adap)
{
    int ret = -1;
    si2176_cmdreplyobj_t rsp;
    
    switch (adap->tuner) {
    case 0 : // NULL
       // printk("Warning: NULL Tuner\n");
    break;

    case 1 : // DCT
    // ret = set_tuner_DCT7070X(demod_sta, adap);
    break;
 
    case 2 : // Maxliner
    // ret = set_tuner_MxL5007(demod_sta, adap);
    break;
 
    case 3 : // NXP
        ret = get_fj2207_ch_power();
    break;
 
    case 4 : // TD1316
     // ret = set_tuner_TD1316(demod_sta, adap);
   	break;
   
    case 7 : // Si2176
        ret = si2176_tuner_status(adap, 1, &rsp);
        if(ret > 0)
            printk("read tuner status error \n");
        //else
        //    printk("  rssil is %d, rssih is %d, rssi is %ddBm, freq is %d \n", rsp.tuner_status.rssil, rsp.tuner_status.rssih, rsp.tuner_status.rssi-256, rsp.tuner_status.freq);   
        ret = rsp.tuner_status.rssi-256;
		printk("ret is %d\n",ret);
    break;

    }

    return ret;
}

struct dvb_tuner_info * tuner_get_info( int type, int mode)
{
	/*type :  0-NULL, 1-DCT7070, 2-Maxliner, 3-FJ2207, 4-TD1316, 5-RDA5880E��?6-TDA18273,7-SI2176*/
	/*mode: 0-DVBC 1-DVBT */
	static struct dvb_tuner_info tinfo_null = {};

	static struct dvb_tuner_info tinfo_MXL5003S[2] = {
		[0] = {.name = "Maxliner",},
		[1] = {/*DVBT*/
			.name = "Maxliner", 
			.frequency_min = 44000000,
			.frequency_max = 885000000,
		}
	};
	static struct dvb_tuner_info tinfo_FJ2207[2] = {
		[0] = {/*DVBC*/
			.name = "FJ2207", 
			.frequency_min = 54000000,
			.frequency_max = 870000000,
		},
		[1] = {/*DVBT*/
			.name = "FJ2207", 
			.frequency_min = 174000000,
			.frequency_max = 864000000,
		},
	};
	static struct dvb_tuner_info tinfo_DCT7070[2] = {
		[0] = {/*DVBC*/
			.name = "DCT7070", 
			.frequency_min = 51000000,
			.frequency_max = 860000000,
		},
		[1] = {.name = "DCT7070",}
	};
	static struct dvb_tuner_info tinfo_TD1316[2] = {
		[0] = {.name = "TD1316",},
		[1] = {/*DVBT*/
			.name = "TD1316", 
			.frequency_min = 51000000,
			.frequency_max = 858000000,
		}
	};
	
	static struct dvb_tuner_info tinfo_RDA5880E[2] = {
		[0] = {.name = "RDA5880E",},
		[1] = {/*DVBT*/
			.name = "RDA5880E", 
			.frequency_min = 40000000,
			.frequency_max = 860000000,
		}
	};
	static struct dvb_tuner_info tinfo_TDA18273[2] = {
		[1] = {/*DVBT*/
			.name = "TDA18273", 
			.frequency_min = 51000000,
			.frequency_max = 858000000,
		}
		
	};
	
		static struct dvb_tuner_info tinfo_SI2176[2] = {
		[0] = {/*DVBC*/
//#error please add SI2176 code
			.name = "SI2176",
			.frequency_min = 51000000,
			.frequency_max = 860000000,
		}
		
	};
	
	
	struct dvb_tuner_info *tinfo[8] = {
		&tinfo_null, 
		tinfo_DCT7070,
		tinfo_MXL5003S,
		tinfo_FJ2207,
		tinfo_TD1316,
		tinfo_RDA5880E,
		tinfo_TDA18273,
		tinfo_SI2176
	};

	if((type<0)||(type>7)||(mode<0)||(mode>1))
		return tinfo[0];
	
	return &tinfo[type][mode];
}

struct agc_power_tab * tuner_get_agc_power_table(int type) {
	/*type :  0-NULL, 1-DCT7070, 2-Maxliner, 3-FJ2207, 4-TD1316*/
	static int calcE_FJ2207[31]={87,118,138,154,172,197,245,273,292,312,
						 327,354,406,430,448,464,481,505,558,583,
						 599,616,632,653,698,725,745,762,779,801,
						 831};
	static int calcE_Maxliner[79]={543,552,562,575,586,596,608,618,627,635,
						    645,653,662,668,678,689,696,705,715,725,
						    733,742,752,763,769,778,789,800,807,816,
						    826,836,844,854,864,874,884,894,904,913,
						    923,932,942,951,961,970,980,990,1000,1012,
						    1022,1031,1040,1049,1059,1069,1079,1088,1098,1107,
						    1115,1123,1132,1140,1148,1157,1165,1173,1179,1186,
						    1192,1198,1203,1208,1208,1214,1217,1218,1220};
	static int calcE_TDA18273[27]={167,187,204,221,238,272,312,331,350,365,
								383,406,453,477,493,509,523,546,589,613,
								630,644,654,667,681,695,717};
	
	static struct agc_power_tab power_tab[] = {
		[0] = {"null", 0, 0, NULL},
		[1] = {
			.name="DCT7070",
			.level=0,
			.ncalcE=0,
			.calcE=NULL,
		},
		[2] = {
			.name="Maxlear",
			.level=-22,
			.ncalcE=sizeof(calcE_Maxliner)/sizeof(int),
			.calcE=calcE_Maxliner,
		},
		[3] = {
			.name="FJ2207",
			.level=-62,
			.ncalcE=sizeof(calcE_FJ2207)/sizeof(int),
			.calcE=calcE_FJ2207,
		},
		[4] = {
			.name="TD1316",
			.level=0,
			.ncalcE=0,
			.calcE=NULL,
		},
		[5] = {
			.name="RDA5880E",
			.level=0,
			.ncalcE=0,
			.calcE=NULL,
		},
		[6] = {
			.name="TDA18273",
			.level=-74,
			.ncalcE=sizeof(calcE_TDA18273)/sizeof(int),
			.calcE=calcE_TDA18273,
		},
		[7] = {
			.name="si2176",
			.level=0,
			.ncalcE=0,
			.calcE=NULL,
		},
		
	};
	
	if(type>=2 && type<=7)
		return &power_tab[type];
	else
		return &power_tab[3];
};
	

int  agc_power_to_dbm(int agc_gain, int ad_power, int offset, int tuner)
{
	struct agc_power_tab *ptab = tuner_get_agc_power_table(tuner);
	int est_rf_power;
	int j;

	for(j=0; j<ptab->ncalcE; j++)
		if(agc_gain<=ptab->calcE[j])
			break;

	est_rf_power = ptab->level - j - (ad_power>>4) + 12 + offset;
      
	return (est_rf_power);
}



#if 1

struct aml_demod_i2c adap;
//for si2176 tuner
int init_tuner_skyworth(struct aml_tuner_sys *tuner_sys)
{
		printk("first in si2176_init\n");
		int ret = 0;
		si2176_cmdreplyobj_t rsp;
   		si2176_common_reply_struct crsp;
    	si2176_propobj_t prop;
		int ch_mode =3;
		printk("first in si2176_init31\n");
	//	struct aml_demod_sta *demod_sta;
		
	//	struct amlfe_state *state = fe->demodulator_priv;
		struct i2c_adapter *adapter;
	    printk("first in si2176_init39\n");
		adap.i2c_id = 0;
		printk("first in si2176_init3\n");
		adapter = i2c_get_adapter(adap.i2c_id);
		adap.i2c_priv = adapter;
		adap.addr = 0x60;
		adap.scl_oe = 0;
		printk("first in si2176_init4\n");
		if(!adapter){
			printk("can not get i2c adapter[%d] \n", adap.i2c_id);
			return -1;
		}
		printk("first in si2176_init 2\n");
    	ret = si2176_init(&adap, &rsp, &crsp);
        ret = si2176_configure(&adap, &prop, &rsp, &crsp, tuner_sys);
		return ret;
}
#endif

int init_tuner_for_my_tool(struct aml_tuner_sys *tuner_sys)
{
		int ret = 0;
		si2176_cmdreplyobj_t rsp;
   		si2176_common_reply_struct crsp;
    	si2176_propobj_t prop;
		int ch_mode =3;
		unsigned long ch_freq;
	//	struct aml_demod_sta *demod_sta;
		
	//	struct amlfe_state *state = fe->demodulator_priv;
		struct i2c_adapter *adapter;
		adap.i2c_id = 1;
		adapter = i2c_get_adapter(adap.i2c_id);
		adap.i2c_priv = adapter;
		adap.addr = 0x60;
		adap.scl_oe = 0;
		if(!adapter){
			printk("can not get i2c adapter[%d] \n", adap.i2c_id);
			return -1;
		}
		printk("first in si2176_init 2\n");
    	ret = si2176_init(&adap, &rsp, &crsp);
        ret = si2176_configure(&adap, &prop, &rsp, &crsp, tuner_sys);

	ch_freq = tuner_sys->ch_freq*1000;
	 printk("----------- si2176 configure freq start -----------\n");
    ret = si2176_tune(&adap, SI2176_TUNER_TUNE_FREQ_CMD_MODE_DTV, ch_freq, &rsp, &crsp);
    printk("----------- si2176 configure freq end -----------\n");
    mdelay(1000);
    ret = si2176_tuner_status(&adap, 1, &rsp);
    if(ret > 0)
        printk("read tuner status error \n");
    else
            //rsp->tuner_status.rssil    =   (( ( (rspbytebuffer[2]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSIL_LSB    ) & SI2176_TUNER_STATUS_RESPONSE_RSSIL_MASK    );
            //rsp->tuner_status.rssih    =   (( ( (rspbytebuffer[2]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSIH_LSB    ) & SI2176_TUNER_STATUS_RESPONSE_RSSIH_MASK    );
            //rsp->tuner_status.rssi     = (((( ( (rspbytebuffer[3]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSI_LSB     ) & SI2176_TUNER_STATUS_RESPONSE_RSSI_MASK) <<SI2176_TUNER_STATUS_RESPONSE_RSSI_SHIFT ) >>SI2176_TUNER_STATUS_RESPONSE_RSSI_SHIFT     );
            //rsp->tuner_status.freq     =   (( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 ) | (rspbytebuffer[6]  << 16 ) | (rspbytebuffer[7]  << 24 )) >> SI2176_TUNER_STATUS_RESPONSE_FREQ_LSB     ) & SI2176_TUNER_STATUS_RESPONSE_FREQ_MASK     );
        printk("  ****************** rssi is %ddBm, freq is %d ***************************** \n", rsp.tuner_status.rssi-256, rsp.tuner_status.freq);   
	
}

