/***************************************************************************************
*					             
*                           (c) Copyright 2011, LegendSilicon, beijing, China
*
*                                        All Rights Reserved
*
* Description :
* Notice:
***************************************************************************************/
#ifndef _LGS_DEMOD_H_
#define _LGS_DEMOD_H_

#ifdef __cplusplus
extern "C" {
#endif 

#include "LGS_TYPES.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
typedef struct _st_DEMOD_CONFIG
{
	UINT8	GuardIntvl;						// Guard interval	0-420, 1-595, 2-945
	UINT8	SubCarrier;						/// Sub carrier		0-4QAM, 1-4QAM_NR, 2-16QAM, 3-32QAM, 4-64QAM 正交幅度调制
	UINT8	FecRate;						// FEC			0-0.4, 1-0.6, 2-0.8
	UINT8	TimeDeintvl;					// TIM			0-240, 1-720
	UINT8	PnNumber;						// PN Sequence Phase序列阶段	0-VPN, 1-CPN
	UINT8	CarrierMode;					// Carrier mode	0-MC, 1-SC
	UINT8	IsMpegClockInvert;			    // MPEG Clock		0-normal, 1-inverted
	UINT8	AdType;							// AD type		0-internal AD, 1-external AD
	UINT32	BCHCount;						// BCH Count (read only)
	UINT32	BCHPktErrCount;				    // BCH Packet Error Count (read only)
	UINT32	AFCPhase;						// AFC Phase value (read only)
	UINT32	AFCPhaseInit;					// Initial AFC Phase value

	UINT8	GuardIntvl2;					// Guard interval	0-420, 1-595, 2-945
	UINT8	SubCarrier2;					// Sub carrier		0-4QAM, 1-4QAM_NR, 2-16QAM, 3-32QAM, 4-64QAM
	UINT8	FecRate2;						// FEC			0-0.4, 1-0.6, 2-0.8
	UINT8	TimeDeintvl2;					// TIM			0-240, 1-720
	UINT8	PnNumber2;						// PN Sequence Phase	0-VPN, 1-CPN
	UINT8	CarrierMode2;					// Carrier mode	0-MC, 1-SC
	UINT8	IsMpegClockInvert2;			    // MPEG Clock		0-normal, 1-inverted
	UINT8	AdType2;						// AD type		0-internal AD, 1-external AD
	UINT32	BCHCount2;						// BCH Count (read only)
	UINT32	BCHPktErrCount2;				// BCH Packet Error Count (read only)
	UINT32	AFCPhase2;						// AFC Phase value (read only)
	UINT32	AFCPhaseInit2;					// Initial AFC Phase value
} DemodConfig;

typedef enum 
{
    DEMOD_WORK_MODE_ANT1_DTMB = 0,
    DEMOD_WORK_MODE_ANT1_DVBC,
    DEMOD_WORK_MODE_INVALID
} DEMOD_WORK_MODE;

typedef enum
{
	TS_Output_Parallel=0,                   //纯并行ts流
	TS_Output_Serial,                       //纯串行ts流 
	TS_Output_INVALID
} TSOutputType;

//modulation mode, Quadrature Amplitude Modulation
typedef enum 
{
	QAM16 = 0,
    QAM32,
    QAM64,
    QAM128,
    QAM256,
    QAM_INVALID
} DVBC_QAM;

typedef struct _st_DVBC_PARA
{
	DVBC_QAM		dvbcQam;
	double			dvbcSymbolRate;	//Mhz
	UINT8			dvbcIFPolarity; //0: Positive;  1: Negative;
} DVBCPara;

typedef struct _st_DEMOD_INIT_PARA
{
	DEMOD_WORK_MODE	workMode;
	TSOutputType	tsOutputType;
	DVBC_QAM		dvbcQam;        //Modulation Mode
	UINT32			IF;			//Mhz,First and Second Tuner's out IF value
	UINT32			dvbcSymbolRate;	//Mhz
	UINT8			dvbcIFPolarity; //0: Positive;  1: Negative;
	UINT8			dtmbIFSelect;	//0: Tuner is high IF;  1: Tuner is low IF
	UINT8           SampleClock;    //0 = 30.4MHz, 1 = 60.8MHz ,2 = 24.0MHz demod采样钟频率
} DemodInitPara;

typedef struct
{
	BOOL  PNMDDone; //ture or false
	UINT8 pn_mode;   //PN Guard Interval,0=PN420,1=PN595,2=PN945
	BOOL  pn_phase;  //0=Variable PN, 1=Constant PN
	BOOL  CAlock;    //ture or false
	BOOL  CarrierMode; //false表示MC,True表示SC
}PN_PARA;

#define PARA_IGNORE	0xFF

/// Initialize all members of DemodParameters to PARA_IGNORE
#define DemodParaIgnoreAll(_x_)		memset(_x_, PARA_IGNORE, sizeof(DemodConfig))

/// Modulation index value
#define PARA_MODE_4QAM		0
#define PARA_MODE_4QAM_NR	1
#define PARA_MODE_16QAM		2
#define PARA_MODE_32QAM		3
#define PARA_MODE_64QAM		4

/// Time De-interleaver index value
#define PARA_TI_240	0
#define PARA_TI_720	1

/// GuardIntvl index value
#define PARA_GI_420	0
#define PARA_GI_595	1
#define PARA_GI_945	2

/// PN Sequence Phase index value
#define PARA_PN_VPN	0
#define PARA_PN_CPN	1

/// Carrier mode index value
#define PARA_CARRIER_MC		0
#define PARA_CARRIER_SC		1
//Dvbc signal polarity
//0-Positive, 1-Negative,2-UNKNOWN
#define PARA_DVBC_SIG_POSITIVE 0
#define PARA_DVBC_SIG_NEGATIVE 1
#define PARA_DVBC_SIG_UNKNOWN  2


//UINT8 reg_B1_value;
/*
if you want to get signal strength, Please read 0x3A address,0xB1 register, 
get register value reg_B1_value and Define MACRO SS_LEVEL1 ...SS_LEVEL10;
*/

//-88dBm

#define reg_B1_value 0xFF

#define SS_LEVEL1 reg_B1_value 

//-85dBm
#define SS_LEVEL2 reg_B1_value

//-82dBm
#define SS_LEVEL3 reg_B1_value

//-80dBm
#define SS_LEVEL4 reg_B1_value

//-78dBm
#define SS_LEVEL5 reg_B1_value

//-75dBm
#define SS_LEVEL6 reg_B1_value

//-72dBm
#define SS_LEVEL7 reg_B1_value

//-68dBm
#define SS_LEVEL8 reg_B1_value

//-65dBm
#define SS_LEVEL9 reg_B1_value

//-60dBm 
#define SS_LEVEL10 reg_B1_value



// Register interface prototype
////////////////////////////////////////////////////////////////
void LGS_DemodRegisterRegisterAccess(LGS_REGISTER_READ pread,	
									 LGS_REGISTER_WRITE pwrite,
				LGS_REGISTER_READ_MULTIBYTE preadm,	
				LGS_REGISTER_WRITE_MULTIBYTE pwritem);

void LGS_DemodRegisterWait(LGS_WAIT wait);

#ifdef __cplusplus
} 
#endif
#endif
