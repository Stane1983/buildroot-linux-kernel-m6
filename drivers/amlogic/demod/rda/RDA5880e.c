
/*
5580e C version driver

version 2.0     data:2011.4.26      author:wang hongxin
 The primary version.

 version 2.1  data:2011.6.15      author:wang hongxin
 Modify the tun_rda5880e_sdm_freq(void)

 version 2.2  data:2011.6.21     author:wang hongxin
 disable the rfpll_open_en   2bit of register 0x16

 version 2.3  data:2011.6.24     author:wang hongxin
 Modify tmx gain form 1110 to 1010<register 0x23[12:15]

 version 2.4  data:2011.6.29    author:wang hongxin
 disable the print for enhance efficiency
 set the sdm frequency to 140M(280/2) at the 191.5MHz for enhance the sensitivity
 
 version 2.4.1  data:2011.8.8    author:wang hongxin
a modify the value of register 0x0d from 0x0152 to 0x0157  in init  for fix the adc_cal_refi_bit
b add the register 0x2e which value is 0x00c3 in init for set pllbb
c modify the value of register 0x21 from 0x11b0 to 0x11b1 in if_open for tmx_dac_offset_dr
  add the register 0x1f and 0x20 in if_open for settmx_dac_offset_i and tmx_dac_offset_q
d modify two registers's address in get_agc for written mistake
e Modify tmx gain form 1010 to 1000 <register 0x23[12:15]> in initial
f Modify lna_d_lg_3 and lna_d_lg_2 from 10 to 00<register 0x24[8:11]>
g Modify the agc_dig_thd from 101 to 105 <register 0x55[8:15]>
 */
#include "RDA5880_adp.h"

#define TUNER_RDA5880 1
#define FRONTEND_TUNER_TYPE TUNER_RDA5880


#if (FRONTEND_TUNER_TYPE == TUNER_RDA5880)
//#define delay       SPHEDVBT_DelayMS
static MS_U8	last_band=0xff;
#define 	tuner_crystal	27000 //KHz
void  tun_rda5880e_if_open(void);
void  tun_rda5880e_rxon_open_if(void);



void  tun_rda5880e_init(void)
{
	
//soft reset
	RDA5880e_Write_single(0x30,0x00,0x04);	
	RDA5880e_Write_single(0x30,0x00,0x05);
	
//ldo voltage
       RDA5880e_Write_single(0x1a,0x14,0x84);
       RDA5880e_Write_single(0x1b,0x02,0x10);
       RDA5880e_Write_single(0x1c,0x04,0x44);

//ana
       RDA5880e_Write_single(0x01,0x4a,0x40);
       RDA5880e_Write_single(0x04,0x02,0xa0);
       RDA5880e_Write_single(0x05,0xa4,0x45);
       RDA5880e_Write_single(0x06,0x32,0xff);
       RDA5880e_Write_single(0x08,0x0f,0xc3);
       RDA5880e_Write_single(0x09,0xb3,0x24);	   
       RDA5880e_Write_single(0x0a,0x22,0xaa);	
       RDA5880e_Write_single(0x0b,0x38,0x20);
       RDA5880e_Write_single(0x0c,0x38,0x20);
       RDA5880e_Write_single(0x0d,0x01,0x57);  //modify by rda 2011.8.8
       RDA5880e_Write_single(0x0e,0x18,0x20);
       RDA5880e_Write_single(0x10,0x24,0x40);
       RDA5880e_Write_single(0x3b,0x09,0x27);
       
//rfpll
       RDA5880e_Write_single(0x11,0x00,0x01);
       RDA5880e_Write_single(0x12,0x24,0x88);
       RDA5880e_Write_single(0x13,0x66,0x08);
       RDA5880e_Write_single(0x14,0x00,0x18);
       RDA5880e_Write_single(0x15,0xc2,0x8b);
       RDA5880e_Write_single(0x16,0x20,0x64);
	RDA5880e_Write_single(0x17,0x01,0x9a); //modify by rda 2011.10.30
//       RDA5880e_Write_single(0x17,0x02,0x22);
       RDA5880e_Write_single(0x31,0x00,0x00);  
       RDA5880e_Write_single(0x3f,0x1a,0x3e);
       
//pd       
        RDA5880e_Write_single(0x2d,0x00,0x00);      
        RDA5880e_Write_single(0x2e,0x00,0xc3);  //modify by rda 2011.8.8
        MsOS_DelayTask(10);                               //modify by rda 2011.8.8 
        RDA5880e_Write_single(0x2e,0x00,0xc2);              
       
       
//rst       
         RDA5880e_Write_single(0x35,0x28,0x07);            
         RDA5880e_Write_single(0x38,0xd1,0x5f);   
         
//rfpll sdm                      
         RDA5880e_Write_single(0x45,0x00,0xe2);         
       
//pllbb_sdm       
        RDA5880e_Write_single(0x46,0x05,0x00);      
        RDA5880e_Write_single(0x47,0x00,0x00);      
        RDA5880e_Write_single(0x48,0x0f,0xde);      
        RDA5880e_Write_single(0x4b,0x30,0x00);      
        RDA5880e_Write_single(0x4d,0x00,0x00);      
       
//dsp
        RDA5880e_Write_single(0x5b,0x00,0x03);  
        RDA5880e_Write_single(0x5d,0x00,0x3f);  
        RDA5880e_Write_single(0x5e,0x10,0x00);  
        RDA5880e_Write_single(0x5f,0x60,0x00);  
        RDA5880e_Write_single(0x60,0x10,0x00);  
        RDA5880e_Write_single(0x61,0xf0,0x00);  
        RDA5880e_Write_single(0x63,0x30,0x00);  
        RDA5880e_Write_single(0x64,0x00,0x00);  
        RDA5880e_Write_single(0x65,0xc1,0x5c);  
        RDA5880e_Write_single(0x68,0xef,0xde);  
        RDA5880e_Write_single(0x79,0x01,0x00);  
   }

/*****************************************************************************

*****************************************************************************/
void  tun_rda5880e_gain_initial(void)
{    
	 RDA5880e_Write_single(0x1d,0x20,0x00);  
        RDA5880e_Write_single(0x1e,0x47,0xe0);  
        RDA5880e_Write_single(0x1f,0x30,0x00);  
        RDA5880e_Write_single(0x20,0x00,0x00);  
        RDA5880e_Write_single(0x21,0x11,0xb1);  
        RDA5880e_Write_single(0x22,0x11,0x00);  
        RDA5880e_Write_single(0x23,0x80,0x00);  //modify by rda 2011.8.8
//        RDA5880e_Write_single(0x23,0xe0,0x00);  //modify by rda 2011.10.31
        RDA5880e_Write_single(0x24,0x00,0x00);  
        RDA5880e_Write_single(0x25,0x1f,0x00);  
        RDA5880e_Write_single(0x26,0x1f,0x00);  
        RDA5880e_Write_single(0x27,0x11,0x7f);  
        RDA5880e_Write_single(0x28,0x30,0xfe);   //i2v 3 +6db
        RDA5880e_Write_single(0x29,0x31,0x7c);   //i2v 2 +6db
        RDA5880e_Write_single(0x2a,0x33,0x38);   //i2v 1 +6db
        RDA5880e_Write_single(0x2b,0x17,0x10);   //i2v 0 +6db modify by rda 2011.8.8
        RDA5880e_Write_single(0x30,0x00,0x5d);  
        RDA5880e_Write_single(0x4b,0x02,0x5c);  
        RDA5880e_Write_single(0x4c,0x2f,0xf8);  
        RDA5880e_Write_single(0x4d,0x00,0x00);  
        RDA5880e_Write_single(0x4e,0x80,0x00);  
        RDA5880e_Write_single(0x4f,0x82,0x07);  
        RDA5880e_Write_single(0x50,0x3f,0xfc);          
        RDA5880e_Write_single(0x51,0x00,0x08);  
        RDA5880e_Write_single(0x52,0x00,0x05);  
        RDA5880e_Write_single(0x53,0xa0,0x00);  
        RDA5880e_Write_single(0x54,0x1e,0x88);  
        RDA5880e_Write_single(0x55,0xe0,0x00);  
        RDA5880e_Write_single(0x56,0xff,0x40);  
        RDA5880e_Write_single(0x57,0x7f,0x00);  
        RDA5880e_Write_single(0x58,0x00,0x00);  
        RDA5880e_Write_single(0x59,0x40,0x00);  
        RDA5880e_Write_single(0x5a,0x9d,0xc0);  
        RDA5880e_Write_single(0x5c,0x87,0x80);  
        RDA5880e_Write_single(0x62,0x00,0x00);                             
        RDA5880e_Write_single(0x64,0x10,0x00);                             
        RDA5880e_Write_single(0x65,0xc5,0x5c);                             
        RDA5880e_Write_single(0x73,0x00,0x00);                             
        RDA5880e_Write_single(0x74,0x00,0x00);                             
        RDA5880e_Write_single(0x75,0x00,0x00);                             
        RDA5880e_Write_single(0x76,0x0b,0x00);                             
        RDA5880e_Write_single(0x77,0xf0,0x0f);                             
        RDA5880e_Write_single(0x78,0x98,0x86);   //i2v_th_low=134,i2v_th_high=152,modify by rda 2011.9.8                          
        RDA5880e_Write_single(0x7a,0xf1,0x00);                             
   
// reset dsp	
        RDA5880e_Write_single(0x63,0xb0,0x00);                             
        RDA5880e_Write_single(0x64,0x14,0x00);	
}

  void tun_rda5880e_rxon_close_if(void)
{

	RDA5880e_Write_single(0x30,0x00,0x55); 	//rxon disable
	RDA5880e_Write_single(0x16,0x20,0x60); 	//rfpll_refmulti2_en=0 rfpll_open_en=0
	RDA5880e_Write_single(0x50,0x3f,0xfc);  //agc_vga_loop_u_ct_slow	set fast
	RDA5880e_Write_single(0x54,0x14,0x88);  //agc_vga_update_ct_slow  set fast
}
void  tun_rda5880e_control_if(MS_U32 freq, MS_U32 IF_freq,MS_U32 bandwidth)		
{	
	MS_U8 cur_band;
	MS_U16 data_tmp;
	MS_U32 tmp,tmp1;
	MS_U32 N;
	MS_U32 N1;
	MS_U32 N2;
	MS_U32 frac1;
	MS_U32 frac2;
	MS_U16 data_buf[3];
	MS_U8 high;
	MS_U8 low;
  //tun_rda5880e_rxon_close_if();

	RDA5880e_Write_single(0x30,0x00,0x55); 	//rxon disable
	RDA5880e_Write_single(0x16,0x20,0x60); 	//rfpll_refmulti2_en=0 rfpll_open_en=0
	RDA5880e_Write_single(0x50,0x3f,0xfc);  //agc_vga_loop_u_ct_slow	set fast
	RDA5880e_Write_single(0x54,0x14,0x88);  //agc_vga_update_ct_slow  set fast
  
  //tun_rda5880e_selband(freq, *cur_band);
  //…Ë÷√agc  ∑÷∂Œ
	if(freq<=400000)        //vhf
		cur_band=0;			
	else if (freq<=590000)  //474~590MHz
		cur_band=1;	            
	else if (freq<=686000)  //591~695MHz
		cur_band=2;			        
	else                             //696~858MHz
		cur_band=3;		

  //tun_rda5880e_set_crystal();
  //{3aH [14:0] } = dec2bin(f_xtal  ) f_xtal KHz
	tmp=(MS_U32)(tuner_crystal);
	
	data_buf[0]=RDA5880e_Read(0x3a); 
	data_buf[1]=data_buf[0]&0x8000; 
	
	//data_tmp=(MS_U32)(tmp&data_buf[1]);
	////LGH change 2011_6_2
	data_tmp=(MS_U32)((data_buf[1]&0x8000)|(tmp&0x00007fff));
	high=(MS_U8)((data_tmp&0xff00)>>8);
	low=(MS_U8)(data_tmp&0x00ff);
  	RDA5880e_Write_single(0x3a,high,low);   	
	
    //tun_rda5880e_sdm_freq(void);
  //{46H [11:0] &47H [15:0]}= dec2bin((270*2^23)/f_xtal)
  //N=(MS_U32)(33750 / tuner_crystal);
	if(191500==freq)
	{
       N=(MS_U32)(280*2000 / tuner_crystal);  //modify by rda 2011.6.29
       frac1=280*2000-N*tuner_crystal;           //modify by rda 2011.6.29
		}
	else
	{
       N=(MS_U32)(270*2000 / tuner_crystal);  //modify by rda 2011.6.5
        frac1=270*2000-N*tuner_crystal;          //modify by rda 2011.6.5
	     }
	N1=(MS_U32)(frac1*2048/ tuner_crystal);
	frac2=frac1*2048 -N1*tuner_crystal;
	N2=(MS_U32)(frac2*2048/ tuner_crystal);
	tmp=N*4194304+N1*2048+N2;
	
	data_buf[0]=RDA5880e_Read(0x46); 
	data_buf[1]=data_buf[0]&0xf000; 

  	//data_tmp=(MS_U16)(((tmp>>16)&0x0fff)|data_buf[1]);
  	////LGH change 2011_6_2///////////////////////////////////////////////////////////////////
  	tmp=tmp&0xFFFFFFF;
	tmp1=data_buf[1];
	tmp=(MS_U32)(((tmp1<<16)&0xf0000000)|tmp);
	//////////////////////////////////////////////////////////////////////////////////////
  	data_tmp=(MS_U16)((tmp>>16)&0xffff);
	//printf("\n#######2temp:0x%x,0x%x,0x%x,0x%x",tmp,data_buf[0],data_buf[1],data_tmp);
  	high=(MS_U8)((data_tmp&0xff00)>>8);
	low=(MS_U8)(data_tmp&0x00ff);
	RDA5880e_Write_single(0x46,high,low);
	//printf("\n#####control_if46:0x%x",data_tmp);
	data_tmp=(MS_U16)(tmp&0xffff);
	high=(MS_U8)((data_tmp&0xff00)>>8);
	low=(MS_U8)(data_tmp&0x00ff);
  RDA5880e_Write_single(0x47,high,low);
//  printf("\n#####control_if47:0x%x",data_tmp);
  //tun_rda5880e_rf_freq(MS_U32 freq);
   //{02H [15:0]&03H [15:0] }= dec2bin (f*1000*2^10)
   if(866000==freq)
   	{
   	 RDA5880e_Write_single(0x02,0x34,0xac);
	 RDA5880e_Write_single(0x03,0x60,0x00);
	 RDA5880e_Write_single(0x30,0x00,0x55); 	//rxon disable
	 MsOS_DelayTask(10);
	 RDA5880e_Write_single(0x30,0x00,0x5d); 	//rxon able
	 data_buf[2]=RDA5880e_Read(0x18);
	 data_tmp= data_buf[2]+3;
	 high=(MS_U8)(((data_tmp & 0xff00) | 0x0400)>>8);
	 low= (MS_U8) (data_tmp & 0x00ff);
	 RDA5880e_Write_single(0x18,high,low);    //fix vco band 
	 RDA5880e_Write_single(0x19,0x01,0x90); //fix vco high and rfpll_sel_800m_reg
	 RDA5880e_Write_single(0x32,0x60,0x12); //rf_req=866
	 RDA5880e_Write_single(0x33,0xf6,0x84); 
   	}
   else
   	{
   	RDA5880e_Write_single(0x18,0x00,0x00);    //release the dr bit. 
	RDA5880e_Write_single(0x19,0x00,0x00); //release the dr bit. 
	RDA5880e_Write_single(0x32,0x00,0x00); //release the dr bit. 
	tmp=(MS_U32)(freq*1024+103);//modify by rda 2011.8.22
	
	data_tmp=(MS_U16)((tmp>>16)&0xffff);
	high=(MS_U8)((data_tmp&0xff00)>>8);
	low=(MS_U8)(data_tmp&0x00ff);
	RDA5880e_Write_single(0x02,high,low);  
//   printf("\n#####control_if02:0x%x",data_tmp);
	data_tmp=(MS_U16)(tmp&0xffff);
	high=(MS_U8)((data_tmp&0xff00)>>8);
	low=(MS_U8)(data_tmp&0x00ff);
  	RDA5880e_Write_single(0x03,high,low);
  	}
  //printf("\n#####control_if03:0x%x",data_tmp);
  //tun_rda5880e_if_freq(MS_U32 IF_freq)
  	//{66H [11:0] &67H [15:0]}= dec2bin((if*8*2^23)/f_xtal)
       N=(MS_U32)(IF_freq*16 / tuner_crystal);
       frac1=IF_freq*16-N*tuner_crystal;
	N1=(MS_U32)(frac1*2048/ tuner_crystal);
	frac2=frac1*2048 -N1*tuner_crystal;
	N2=(MS_U32)(frac2*2048/ tuner_crystal);
	tmp=N*4194304+N1*2048+N2;
	
	data_buf[0]=RDA5880e_Read(0x66); 
	data_buf[1]=data_buf[0]&0xf000; 
////LGH change 2011_6_2///////////////////////////////////////////////////////////////////
  tmp=tmp&0xFFFFFFF;
	tmp1=data_buf[1];
	tmp=(MS_U32)(((tmp1<<16)&0xf0000000)|tmp);
////////////////////////////////////////////////////////////////////////////////////////
  	data_tmp=(MS_U16)((tmp>>16)&0xffff);
       high=(MS_U8)((data_tmp&0xff00)>>8);
	low=(MS_U8)(data_tmp&0x00ff);
	RDA5880e_Write_single(0x66,high,low);
//	printf("\n#####control_if66:0x%x",data_tmp);
	data_tmp=(MS_U16)(tmp&0xffff);
	high=(MS_U8)((data_tmp&0xff00)>>8);
	low=(MS_U8)(data_tmp&0x00ff);
  RDA5880e_Write_single(0x67,high,low);         
 // printf("\n#####control_if67:0x%x",data_tmp);

  
  //RDA5880e_Write_single(0x47,high,low);  //  MTC change
  //tun_rda5880e_set_gain(MS_U32 freq,cur_band);
  if (freq<=400000)  //vhf setting
 {
    if(cur_band!=last_band)
   {                           
 //       RDA5880e_Write_single(0x24,0xf0,0x00);            ////modify by rda 2011.8.8,lna_d_lg_3=00                                   
       RDA5880e_Write_single(0x4f,0x9b,0x07);            //agc_lna_thd=155                                 
       RDA5880e_Write_single(0x54,0x14,0x8c);            //agc_vga_thd=20                                  
       RDA5880e_Write_single(0x55,0xd8,0x00);            //agc_dig_thd=108                                 
       RDA5880e_Write_single(0x61,0x70,0x00);            //dig filter=0100                                 
       RDA5880e_Write_single(0x78,0x98,0x86);            // i2v_th_low=134,i2v_th_high=152                 
 
   }		
 }
else  //uhf
{
	if(cur_band!=last_band)
	{
        RDA5880e_Write_single(0x04,0x42,0xa0);            //filter cap=1000                                 
        RDA5880e_Write_single(0x4f,0x91,0x07);            //agc_lna_thd=145                 
        RDA5880e_Write_single(0x54,0x14,0x8c);            //agc_vga_thd=20                  
        RDA5880e_Write_single(0x55,0xd8,0x00);            //agc_dig_thd=108                                 
        RDA5880e_Write_single(0x78,0x98,0x86);            // i2v_th_low=134,i2v_th_high=152 	
      }
    

 //if (freq<=590000)   //474~590MHz
 //{
 //     RDA5880e_Write_single(0x24,0xfa,0x00);            //lna_d_lg_3=10      	
 // }  
 // else if (freq<=695000) //591~695MHz
 // {
 //      RDA5880e_Write_single(0x24,0xf5,0x00);            //lna_d_lg_3=01      	
 // }     
 //  else //696~858MHz
 // {
 //       RDA5880e_Write_single(0x24,0xf0,0x00);            //lna_d_lg_3=00 
 //   	} */     
 }
   last_band=cur_band;	
   
  //tun_rda5880e_set_bandwidth(MS_U32 bandwidth);
  if(bandwidth==8000)  
	{
        RDA5880e_Write_single(0x04,0x42,0xa0);            //filter cap=1000                                              
        RDA5880e_Write_single(0x61,0xf0,0x00);            //dig filter=1111                     
	}     
  	if(bandwidth==7000)  
	{
        RDA5880e_Write_single(0x04,0x42,0xb0);            //filter cap=1100                                                                                   
        RDA5880e_Write_single(0x61,0x70,0x00);            //dig filter=0111                      
	}   
	if(bandwidth==6000)  
	{
        RDA5880e_Write_single(0x04,0x42,0xb0);            //filter cap=1100                                                                                   
        RDA5880e_Write_single(0x61,0x30,0x00);            //dig filter=0011    
        RDA5880e_Write_single(0x24,0xc0,0x00);            //lna_d_lg_1&2=00, for aci N+1&N-1       
	}   	
	
  tun_rda5880e_if_open();
  tun_rda5880e_rxon_open_if();	
}

void  tun_rda5880e_if_open(void)
{
        RDA5880e_Write_single(0x21,0x11,0xb1);    //modify by rda 2011.8.8,tmx_dac_offset_dr=1 
	 RDA5880e_Write_single(0x1f,0x10,0x00);   //modify by rda 2011.8.8
	 RDA5880e_Write_single(0x20,0x00,0x00);   //modify by rda 2011.8.8         
        RDA5880e_Write_single(0x30,0x00,0x75);   //if_mode=1,open if                                         
        RDA5880e_Write_single(0x5c,0x83,0x80);    //dccancel_bypass=0
	 RDA5880e_Write_single(0x55,0xD2,0x00);    //modify by rda 2011.8.8,agc_dig_thd=105

}

void  tun_rda5880e_rxon_open_if(void)
{
	
   RDA5880e_Write_single(0x30,0x00,0x7d); 	//rxon open
   RDA5880e_Write_single(0x16,0x60,0x60); 	//rfpll_refmulti2_en=1 rfpll_open_en
   RDA5880e_Write_single(0x5a,0x80,0x00);  //agc_dig_rssi set fast
   
   RDA5880e_Write_single(0x63,0xb0,0x00);   //modify by rda 2011.8.8
   MsOS_DelayTask(1);
   RDA5880e_Write_single(0x63,0x30,0x00);   //dps reset
   
   RDA5880e_Write_single(0x64,0x14,0x00);
   MsOS_DelayTask(10);
   RDA5880e_Write_single(0x64,0x10,0x00);   //agc_lna_reset   
   
   MsOS_DelayTask(28);
   RDA5880e_Write_single(0x50,0x3f,0xfe);  //agc_vga_loop_u_ct_slow	set slow
   RDA5880e_Write_single(0x54,0x14,0x8e);  //modify by rda 2011.8.8,agc_vga_update_ct_slow  set slow
   RDA5880e_Write_single(0x5a,0x9d,0xc0);  //agc_dig_rssi set slow
   MsOS_DelayTask(2);
}




void  tun_rda5880e_IntoPwrDwn_with_loop(void)
{
      RDA5880e_Write_single(0x21,0x19,0x31);  //close gain table
      RDA5880e_Write_single(0x23,0xef,0xff);  //set lna gain to max
	  RDA5880e_Write_single(0x30,0x00,0xc5);  // enable direct_reg
	  RDA5880e_Write_single(0x2f,0x7f,0xff);  //pd_ldo_lna_s=0
	  RDA5880e_Write_single(0x2e,0x01,0xe6);  //pd_loopout_dr=0	
          //into sleep with loop,current = 12mA
}

void  tun_rda5880e_IntoPwrDwn_without_loop(void)
{
	  RDA5880e_Write_single(0x30,0x00,0xc5); // enable direct_reg
	  RDA5880e_Write_single(0x2f,0xff,0xff); //pd_ldo_lna_s=1
	  RDA5880e_Write_single(0x2e,0x01,0xf6); //pd_loopout_dr=1
          //into sleep without loop,current =1.5mA
}


int  tun_rda5880e_get_agc(void)
{
	MS_U16 data;
	MS_U8 cur_band;
	MS_U32 gain_lna,gain_i2v,gain_filter,gain_vga,gain_rf,gain_dig_p,gain_dig_n,total_gain,total_power;
	MS_U16	 tmp;
	MS_U8 gain_dig_polarity;
	MS_U8    i2v_index[5]={0x00,0x01,0x03,0x07,0x0f};	
	MS_U8    filter_index[5]={0x00,0x08,0x0c,0x0e,0x0f};	

	//lna_gain
      data=RDA5880e_Read(0x23); 
	tmp=(data&0x03ff);
		if(cur_band==0)        //vhf
		{
		 if(tmp>=830)
		 { gain_lna=42;}
		  else
		  {gain_lna=tmp*42/830;}
	  }		
	  else                  //uhf
		{
			if(tmp>=830)
		  {gain_lna=35;}
		  else
		 { gain_lna=tmp*35/830;}
		}
		
//I2v_gain		
	data=RDA5880e_Read(0x27); 
	tmp=((data&0x0010)>>4)+((data&0x0020)>>5)+((data&0x0040)>>6)+((data&0x0080)>>7);
	gain_i2v=6*tmp;
	
			
//filter_gain		
	data=RDA5880e_Read(0x27); 
	tmp=(data&0x0001)+((data&0x0002)>>1)+((data&0x0004)>>2)+((data&0x0008)>>3);
	gain_filter=6*tmp;
	

	//gain vga
       data=RDA5880e_Read(0x1d); //modify by rda 2011.8.8
	tmp=(data&0x03ff);
	gain_vga=tmp*23/1023;

  //gain_rf
  		if(cur_band==0)        //vhf
		{
			gain_rf=gain_lna+gain_i2v+gain_filter+gain_vga-25;    //rf gain
		}		
	    else                  //uhf
		{
			gain_rf=gain_lna+gain_i2v+gain_filter+gain_vga-20;    //rf gain
		}
		
	//gain_dig
	data=RDA5880e_Read(0x59); //modify by rda 2011.8.8
	tmp=(data&0x0fc0)>>6;
	gain_dig_polarity=(data&0x1000)>>12;
	if(gain_dig_polarity==0x00)  //postive gain
	{
		gain_dig_p=tmp;
		gain_dig_n=0;
	}
	else                         //negative gain	
	{
		gain_dig_p=0;
		gain_dig_n=64-tmp;	
  }
    
  total_gain=gain_rf+gain_dig_p-gain_dig_n;
  
  //total power
  total_power=total_gain+3;
 	return total_power;
}
#endif

