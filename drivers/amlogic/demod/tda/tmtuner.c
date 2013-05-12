#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include "../aml_demod.h"
#include "../demod_func.h"

#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmFrontEnd.h"
#include "tmbslFrontEndTypes.h"
#include "tmsysFrontEndTypes.h"
#include "tmsysScanXpress.h"
#include "tmbslTDA18273.h"

//*--------------------------------------------------------------------------------------
//* Include Driver files
//*--------------------------------------------------------------------------------------
#include "tmsysOM3912.h"

struct aml_demod_i2c *tda18273_i2cadap;

static tmErrorCode_t 	UserWrittenI2CRead(tmUnitSelect_t tUnit,UInt32 AddrSize, UInt8* pAddr,UInt32 ReadLen, UInt8* pData);
static tmErrorCode_t 	UserWrittenI2CWrite (tmUnitSelect_t tUnit, UInt32 AddrSize, UInt8* pAddr,UInt32 WriteLen, UInt8* pData);
static tmErrorCode_t 	UserWrittenWait(tmUnitSelect_t tUnit, UInt32 tms);
static tmErrorCode_t 	UserWrittenPrint(UInt32 level, const char* format, ...);
static tmErrorCode_t  	UserWrittenMutexInit(ptmbslFrontEndMutexHandle *ppMutexHandle);
static tmErrorCode_t  	UserWrittenMutexDeInit( ptmbslFrontEndMutexHandle pMutex);
static tmErrorCode_t  	UserWrittenMutexAcquire(ptmbslFrontEndMutexHandle pMutex, UInt32 timeOut);
static tmErrorCode_t  	UserWrittenMutexRelease(ptmbslFrontEndMutexHandle pMutex);


int init_tuner_tda18273(struct aml_demod_sta *demod_sta, 
		     struct aml_demod_i2c *adap)
{
   tmErrorCode_t err = TM_OK;
   tmbslFrontEndDependency_t sSrvTunerFunc;
   
   tda18273_i2cadap = adap;

/* Low layer struct set-up to link with user written functions */
   sSrvTunerFunc.sIo.Write             = UserWrittenI2CWrite;
   sSrvTunerFunc.sIo.Read              = UserWrittenI2CRead;
   sSrvTunerFunc.sTime.Get             = Null;
   sSrvTunerFunc.sTime.Wait            = UserWrittenWait;
   sSrvTunerFunc.sDebug.Print          = UserWrittenPrint;
   sSrvTunerFunc.sMutex.Init           = UserWrittenMutexInit;
   sSrvTunerFunc.sMutex.DeInit         = UserWrittenMutexDeInit;
   sSrvTunerFunc.sMutex.Acquire        = UserWrittenMutexAcquire;
   sSrvTunerFunc.sMutex.Release        = UserWrittenMutexRelease;
   sSrvTunerFunc.dwAdditionalDataSize  = 0;
   sSrvTunerFunc.pAdditionalData       = Null;
   
    err = tmsysOM3912Init(0, &sSrvTunerFunc);
   if(err != TM_OK)
       return err;

  err = tmsysOM3912SetPowerState(0, tmPowerOn);
	if(err != TM_OK)
			return err;
		
    err = tmsysOM3912Reset(0);
   if(err != TM_OK)
       return err;

       return err;
}



int tda18273_tuner_set_frequncy(unsigned int dwFrequency,unsigned int dwStandard)
{
		tmTunerOnlyRequest_t TuneRequest;
		tmErrorCode_t err = TM_OK;

		if(dwStandard == 8)
			TuneRequest.dwStandard = tmTDA18273_DVBT_8MHz;
		else if(dwStandard == 7)
			TuneRequest.dwStandard = tmTDA18273_DVBT_7MHz;
		else if(dwStandard == 6)
			TuneRequest.dwStandard = tmTDA18273_DVBT_6MHz;
		else
			return TM_ERR_BAD_PARAMETER;
		
	  /* OM3912 Send Request 546 MHz standard DVB-T  8 MHz*/
    TuneRequest.dwFrequency = dwFrequency;
    //TuneRequest.dwStandard = dwStandard;//This is a bug?
    printk("TuneRequest.dwStandard %d!\n",TuneRequest.dwStandard);
    err = tmsysOM3912SendRequest(0,&TuneRequest,sizeof(TuneRequest), TRT_TUNER_ONLY );
    if(err != TM_OK)
       return err;
    return TM_OK;
}


int tda18273_tuner_get_lockstatus(unsigned int LockStatus)
{
	tmErrorCode_t err = TM_OK;
   err = tmsysOM3912GetLockStatus(0,&LockStatus);
    if(err != TM_OK)
       return err;
}


int _set_read_addr(unsigned char* regaddr,unsigned int addrsize)
{
	tmErrorCode_t  nRetCode = TM_OK;
	#define MAX_REG_VALUE_LEN 	(4)
	unsigned char buffer[MAX_REG_VALUE_LEN+1];
	struct i2c_msg msg;
	
	memcpy(buffer,regaddr,addrsize);

	memset(&msg, 0, sizeof(msg));
	msg.addr = tda18273_i2cadap->addr;
	msg.flags &=  ~I2C_M_RD;  //write  I2C_M_RD=0x01
	msg.len = 1;
	msg.buf = buffer;
	nRetCode = i2c_transfer(tda18273_i2cadap->i2c_priv, &msg, 1);
	
	if(nRetCode != 1)
	{
		printk("_set_read_addr reg %s failure,ret %d!\n",regaddr,nRetCode);
		return TM_ERR_WRITE;
	}
	return TM_OK;   //success
}

//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenI2CRead
//* Object              : 
//* Input Parameters    : 	tmUnitSelect_t tUnit
//* 						UInt32 AddrSize,
//* 						UInt8* pAddr,
//* 						UInt32 ReadLen,
//* 						UInt8* pData
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t UserWrittenI2CRead(tmUnitSelect_t tUnit,	UInt32 AddrSize, UInt8* pAddr,
UInt32 ReadLen, UInt8* pData)
{
    /* Variable declarations */
	tmErrorCode_t nRetCode = TM_OK;
	struct i2c_msg msg;
	int i = 0;
	
	if(pData == 0 || ReadLen == 0)
	{
		printk("gx1001 read register parameter error !!\n");
		return 0;
	}

	nRetCode = _set_read_addr(pAddr,AddrSize) ; //set reg address first
	/*
	for (i = 0; i < AddrSize; i++)
	{
		printk("%x\t", pAddr[i]);
	}
	printk(" %s: writereg, errno is %d AddrSize (%d)\n", __FUNCTION__,nRetCode, AddrSize);*/
	if(nRetCode != TM_OK)
	{
		printk("UserWrittenI2CRead reg %s failure!\n",pAddr);
		return TM_ERR_WRITE;
	}
	//read real data 
	memset(&msg, 0, sizeof(msg));
	msg.addr = (tda18273_i2cadap->addr);
	msg.flags |=  I2C_M_RD;  //write  I2C_M_RD=0x01
	msg.len = ReadLen;
	msg.buf = pData;
	
	nRetCode = i2c_transfer(tda18273_i2cadap->i2c_priv, &msg, 1);
      
	if(nRetCode != 1)
	{
		printk("UserWrittenI2CRead reg  %s failure!\n",pAddr);
		return TM_ERR_READ;
	}
	return TM_OK;

}

//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenI2CWrite
//* Object              : 
//* Input Parameters    : 	tmUnitSelect_t tUnit
//* 						UInt32 AddrSize,
//* 						UInt8* pAddr,
//* 						UInt32 WriteLen,
//* 						UInt8* pData
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t UserWrittenI2CWrite (tmUnitSelect_t tUnit, 	UInt32 AddrSize, UInt8* pAddr,
UInt32 WriteLen, UInt8* pData)
{

    /* Variable declarations */
    tmErrorCode_t err = TM_OK;
		
		int ret = 0;
		unsigned char regbuf[4];			/*8 bytes reg addr, regbuf 1 byte*/
		struct i2c_msg msg[2];			/*construct 2 msgs, 1 for reg addr, 1 for reg value, send together*/
            int i = 0;
            
		memcpy(regbuf,pAddr,AddrSize);
		
		memset(msg, 0, sizeof(msg));
  	
		/*write reg address*/
		msg[0].addr = (tda18273_i2cadap->addr);					
		msg[0].flags = 0;
		msg[0].buf = regbuf;
		msg[0].len = AddrSize;
  	
		/*write value*/
		msg[1].addr = (tda18273_i2cadap->addr);
		msg[1].flags = I2C_M_NOSTART;	/*i2c_transfer will emit a stop flag, so we should send 2 msg together,
																	 * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/
		msg[1].buf = pData;
		msg[1].len = WriteLen;
		
		ret = i2c_transfer(tda18273_i2cadap->i2c_priv, msg, 2);
		
		/*
		for ( i = 0; i < AddrSize; i++)
		{
			printk("%x\t", pAddr[i]);
		}
		printk(" %s: writereg, errno is %d AddrSize (%d)\n", __FUNCTION__,ret, AddrSize);*/
		if(ret<0){
			printk(" %s: writereg error, errno is %d \n", __FUNCTION__, ret);
			return TM_ERR_WRITE;
		}
		else
			return TM_OK;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenWait
//* Object              : 
//* Input Parameters    : 	tmUnitSelect_t tUnit
//* 						UInt32 tms
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t UserWrittenWait(tmUnitSelect_t tUnit, UInt32 tms)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

   	msleep(tms);
    /* End of Customer code here */

    return err;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenPrint
//* Object              : 
//* Input Parameters    : 	UInt32 level, const char* format, ...
//* 						
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t 			UserWrittenPrint(UInt32 level, const char* format, ...)
{
    /* Variable declarations */
	
	#define PRINT_TEMP_BUF_SIZE 			(4096)
    tmErrorCode_t err = TM_OK;
	static char trace_buffer[PRINT_TEMP_BUF_SIZE];
	    va_list args;
    int len = 0;
    int avail = PRINT_TEMP_BUF_SIZE;
    char *pos = 0;
    pos = &trace_buffer[0];

    /* Write message. */

    va_start(args, format);

    len    = vsnprintf(pos, avail, format, args);
    pos   += len;
    avail -= len;

    va_end(args);

    if (avail <= 0)
    {
        trace_buffer[PRINT_TEMP_BUF_SIZE - 1] = '\0';
        pos = &trace_buffer[PRINT_TEMP_BUF_SIZE - 1];
    }


    printk("%s", trace_buffer);


    /* End of Customer code here */

    return err;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenMutexInit
//* Object              : 
//* Input Parameters    : 	ptmbslFrontEndMutexHandle *ppMutexHandle
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t  UserWrittenMutexInit(ptmbslFrontEndMutexHandle *ppMutexHandle)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

    /* Customer code here */
    /* ...*/

    /* ...*/
    /* End of Customer code here */

    return err;
}


//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenMutexDeInit
//* Object              : 
//* Input Parameters    : 	 ptmbslFrontEndMutexHandle pMutex
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t  UserWrittenMutexDeInit( ptmbslFrontEndMutexHandle pMutex)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

    return err;
}



//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenMutexAcquire
//* Object              : 
//* Input Parameters    : 	ptmbslFrontEndMutexHandle pMutex, UInt32 timeOut
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t  UserWrittenMutexAcquire(ptmbslFrontEndMutexHandle pMutex, UInt32 timeOut)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

    return err;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenMutexRelease
//* Object              : 
//* Input Parameters    : 	ptmbslFrontEndMutexHandle pMutex
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
static tmErrorCode_t  UserWrittenMutexRelease(ptmbslFrontEndMutexHandle pMutex)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

    return err;
}


