/***************************************************************************************
*
*           (c) Copyright 2012, LegendSilicon, beijing, China
*
*                        All Rights Reserved
*
* Description : for LGS9701_B1
*
* Notice:
* Date:    2012-08-28
* Version: v4.0
* 
* 
***************************************************************************************/
#include "LGS_9X.h"
#include "LGS_9X_FW.h"

static UINT8 g_AdType = 0;
static UINT8 g_is_DownloadFirmware = 0; //1: firmware had been downloaded, 0: need download firmware
static UINT8 g_is_rom_boot = 0;         //1: rom boot, 0: ram boot
static UINT8 g_FF_reg = 0;

#define LGS_FIRMWARE_SIZE (sizeof(firmwareData))

LGS_RESULT LGS9X_DownloadFirmware(void)
{
    UINT8       addr = LGS9XSECADDR1;
    UINT8       data = 0x0;
    UINT32      i = 0;

    if (g_is_DownloadFirmware == 0)
    {
        //Disable Firmware
        LGS_ReadRegister(addr, 0x39, &data);
        data &= 0xFC;
        LGS_WriteRegister(addr, 0x39, data);

        //Enable Firmware
        LGS_ReadRegister(addr, 0x39, &data);
        if (g_is_rom_boot != 0)
        {
            //rom boot
            data |= 0x4;
            LGS_WriteRegister(addr, 0x39, data);
        }
        else
        {
            //ram boot
            data &= ~0x4;
            LGS_WriteRegister(addr, 0x39, data);

            //download firmware
            LGS_WriteRegister(addr, 0x3A, 0x00);
            LGS_WriteRegister(addr, 0x3B, 0x00);

            for (i = 0; i < LGS_FIRMWARE_SIZE; i++)
        	{
                LGS_WriteRegister(addr, 0x3C, firmwareData[i]);
        	}
        }
        data |= 0x2;
        LGS_WriteRegister(addr, 0x39, data);
        data |= 0x1;
        LGS_WriteRegister(addr, 0x39, data);

        g_is_DownloadFirmware++;
    }
    return LGS_NO_ERROR;
}

LGS_RESULT LGS9X_Init(DemodInitPara *para)
{
    int         i;
    UINT32      u32Afc = 0;
    UINT32      u32SymbolRate = 0;
    LGS_RESULT  err = LGS_NO_ERROR;
    UINT8       addr = LGS9XSECADDR1;
    UINT8       data = 0x0;
    UINT8       reg_03 = 0x0;
    UINT8       reg_07 = 0x0;
    UINT8       reg_08 = 0x0;    //设置IF 四个字节
    UINT8       reg_09 = 0x0;
    UINT8       reg_0A = 0x0;
    UINT8       reg_0B = 0x0;
    UINT8       reg_10 = 0x0;
    UINT8       reg_11 = 0x0;
    UINT8       reg_12 = 0x0;
    UINT8       reg_1C = 0x0;
    UINT8       reg_30 = 0x0;
    UINT8       reg_31 = 0x0;
    UINT8       reg_D6 = 0x80;

    if (para == NULL)
        return LGS_PARA_ERROR;
    
    if (g_AdType == 1) //if is extern AD
    {
        reg_03 = 0x80;      //extern ADC
        reg_07 = 0x02;      //extern ADC output two’s-complement
    }

    if (para->workMode >= DEMOD_WORK_MODE_INVALID)
        return LGS_PARA_ERROR;

    if (para->tsOutputType >= TS_Output_INVALID)
        return LGS_PARA_ERROR;

    if (para->workMode== DEMOD_WORK_MODE_ANT1_DVBC)
    {
        if (para->dvbcQam >= QAM_INVALID)
            return LGS_PARA_ERROR;

        if (para->dvbcIFPolarity >= 2)
            return LGS_PARA_ERROR;

        if ((para->dvbcSymbolRate == 0) || (para->dvbcSymbolRate > 7600))
            return LGS_PARA_ERROR;
    }

    LGS9X_DownloadFirmware();

    if (para->IF >= 4 && para->IF <= 10)
    {
        reg_07 |= 0x15;

        u32Afc = 655360 / 304 * 65536 * para->IF;
        reg_08  = (UINT8)(u32Afc & 0xff);
        reg_09  = (UINT8)((u32Afc >> 8) & 0xff);
        reg_0A  = (UINT8)((u32Afc >> 16) & 0xff);
        reg_0B  = (UINT8)((u32Afc >> 24) & 0xff);
    }
    else if (para->IF >= 35 && para->IF <= 41)
    {
        reg_07 |= 0x11;
        u32Afc = 655360 / 304 * 65536 * (para->IF - 30);
        reg_08  = (UINT8)(u32Afc & 0xff);
        reg_09  = (UINT8)((u32Afc >> 8) & 0xff);
        reg_0A  = (UINT8)((u32Afc >> 16) & 0xff);
        reg_0B  = (UINT8)((u32Afc >> 24) & 0xff);
    }
    else if (para->IF == 0)
    {
        reg_07 |= 0x1D;
        reg_08  = 0x0;
        reg_09  = 0x0;
        reg_0A  = 0x0;
        reg_0B  = 0x0;
    }
    else
    {
        return LGS_PARA_ERROR;
    }

    if (para->dtmbIFSelect == 0)
        reg_07 &= 0xFB;
    else
        reg_07 |= 0x4;

    LGS_WriteRegister(addr, 0x35, 0x40);
    LGS_WriteRegister(addr, 0x83, 0xC0);

    if (para->SampleClock==2)
    {
        LGS_WriteRegister(addr, 0x02, 0x00);
        LGS_WriteRegister(addr, 0xF8, 0x04);
        LGS_WriteRegister(addr, 0x24, 0x72);
        LGS_WriteRegister(addr, 0x25, 0x05);
        LGS_ReadRegister(addr, 0x39, &data);
        data |= 0xC0;
        LGS_WriteRegister(addr, 0x39, data);
        data &= ~0xC0;
        data |= 0x80;
        LGS_WriteRegister(addr, 0x39, data);
        LGS_WriteRegister(addr, 0x02, 0x01);
        LGS_WriteRegister(addr, 0x16, 0xA2);
    }

    if( para->workMode==DEMOD_WORK_MODE_ANT1_DTMB)
    {
        LGS_WriteRegister(addr, 0x1D, 0x94);
        LGS_WriteRegister(addr, 0x79, 0x18);
        LGS_WriteRegister(addr, 0x91, 0x01);
    }

    switch(para->workMode)
    {
        case DEMOD_WORK_MODE_ANT1_DTMB:
        {
            //GB
            LGS_WriteRegister(addr, 0x03, reg_03 | 0x01);
            LGS_WriteRegister(addr, 0xD6, reg_D6);
            LGS_WriteRegister(addr, 0xFF, 0x00);
            LGS_WriteRegister(addr, 0x07, reg_07);
            LGS_WriteRegister(addr, 0x08, reg_08);
            LGS_WriteRegister(addr, 0x09, reg_09);
            LGS_WriteRegister(addr, 0x0A, reg_0A);
            LGS_WriteRegister(addr, 0x0B, reg_0B);
            LGS_WriteRegister(addr, 0x12, 0x00);
            LGS_WriteRegister(addr, 0x1C, 0x00);


            //disable ant 2 DVBC
            LGS_WriteRegister(addr, 0xFF, 0x01);
            LGS_WriteRegister(addr, 0x1C, 0x00);
            LGS_WriteRegister(addr, 0xFF, 0x00);

            //Disable MCU,Enable MCU,Reset MCU
            LGS_ReadRegister(addr, 0x39, &data);
            data &=0xF8;
            LGS_WriteRegister(addr, 0x39, data);
            data |= 0x2;
            LGS_WriteRegister(addr, 0x39, data);
            data |= 0x1;
            LGS_WriteRegister(addr, 0x39, data);

            reg_30 = (para->tsOutputType == TS_Output_Parallel)? 0x92 : 0x93;
            LGS_WriteRegister(addr, 0x30, reg_30);
            LGS_WriteRegister(addr, 0x31, 0x02);
            break;
        }
        case DEMOD_WORK_MODE_ANT1_DVBC:
        {
            //DVBC
            LGS_WriteRegister(addr, 0xFF, 0x01);
            LGS_WriteRegister(addr, 0x1C, 0x00);
            LGS_WriteRegister(addr, 0xFF, 0x00);

            LGS_WriteRegister(addr, 0x03, reg_03 | 0x01);
            LGS_WriteRegister(addr, 0xD6, reg_D6);
            LGS_WriteRegister(addr, 0x08, reg_08);
            LGS_WriteRegister(addr, 0x09, reg_09);
            LGS_WriteRegister(addr, 0x0A, reg_0A);
            LGS_WriteRegister(addr, 0x0B, reg_0B);
            LGS_WriteRegister(addr, 0x1C, 0x80);

            reg_30 = (para->tsOutputType == TS_Output_Parallel)? 0x92 : 0x93;
            LGS_WriteRegister(addr, 0x30, reg_30);
            reg_31 = (para->tsOutputType == TS_Output_Parallel)? 0x32 : 0x33;
            LGS_WriteRegister(addr, 0x31, reg_31);
            break;
        }
        
    }

    switch (para->workMode)
    {
        case DEMOD_WORK_MODE_ANT1_DVBC:
        {
            //modify DVBC noise gate
            LGS_WriteRegister(LGS9XSECADDR2, 0xFF, 0x0C);
            LGS_WriteRegister(LGS9XSECADDR2, 0x34, 0x60);    //C page 0x34 default value is 0xF0

            //modify for DVBC256QAM phase noise 
            LGS_WriteRegister(LGS9XSECADDR2, 0x7C, 0x32);    //C page 0x7C default value is 0x64

			LGS_WriteRegister(LGS9XSECADDR2, 0xFF, 0x0D);
            LGS_WriteRegister(LGS9XSECADDR2, 0xCB, 0x00);

            reg_1C = 0x0;

            if (para->dvbcSymbolRate >= 3800 && para->dvbcSymbolRate <= 7600)
        	{
                reg_1C |= 0x80;

                u32SymbolRate = (7600 / para->dvbcSymbolRate - 1) * 4194304;
        	}
            else if (para->dvbcSymbolRate >= 1900 && para->dvbcSymbolRate <= 3800)
        	{
                reg_1C |= 0x81;

                u32SymbolRate = (3800 / para->dvbcSymbolRate - 1) * 4194304;
        	}
            else if (para->dvbcSymbolRate < 1900)
        	{
                reg_1C |= 0x82;

                u32SymbolRate = (1900 / para->dvbcSymbolRate - 1) * 4194304;
        	}
            else
                return LGS_PARA_ERROR;

            reg_10 = (UINT8)(u32SymbolRate & 0xff);
            reg_11 = (UINT8)((u32SymbolRate >> 8) & 0xff);
            reg_12 = (UINT8)((u32SymbolRate >> 16) & 0xff);
            reg_12 |= 0x80;

            switch (para->dvbcQam)
        	{
                case QAM16:
        		{
                    reg_1C |= 0x02 << 4;
                    break;
        		}
                case QAM32:
        		{
                    reg_1C |= 0x03 << 4;
                    break;
        		}
                case QAM64:
        		{
                    reg_1C |= 0x04 << 4;
                    break;
        		}
                case QAM128:
        		{
                    reg_1C |= 0x05 << 4;
                    break;
        		}
                case QAM256:
        		{
                    reg_1C |= 0x06 << 4;
                    break;
        		}
        	}

            switch (para->dvbcIFPolarity)
        	{
                case 0:
        		{
                    reg_07 &= 0xFB;
                    break;
        		}
                case 1:
        		{
                    reg_07 |= 0x4;
                    break;
        		}
        	}

            LGS_WriteRegister(addr, 0x07, reg_07);
            LGS_WriteRegister(addr, 0x10, reg_10);
            LGS_WriteRegister(addr, 0x11, reg_11);
            LGS_WriteRegister(addr, 0x12, reg_12);
            LGS_WriteRegister(addr, 0x1C, reg_1C);

            for (i = 0; i < 2; i++)
        	{
                LGS9X_SoftReset();

                if (para->dvbcSymbolRate >= 5000 && para->dvbcSymbolRate <= 7600)
        		{
                    LGS_Wait(300);
        		}
                else if (para->dvbcSymbolRate >= 3500 && para->dvbcSymbolRate <= 5000)
        		{
                    LGS_Wait(500);
        		}
                else if (para->dvbcSymbolRate >= 2500 && para->dvbcSymbolRate <= 3500)
        		{
                    LGS_Wait(800);
        		}
                else if (para->dvbcSymbolRate >= 1500 && para->dvbcSymbolRate <= 2500)
        		{
                    LGS_Wait(1100);
        		}
                else if (para->dvbcSymbolRate < 1500)
        		{
                    LGS_Wait(1500);
        		}

                //check DVBC lock
                LGS_ReadRegister(addr, 0xDA, &data);
                if (data & 0x80)
                    break;  //lock
        	}
        	
            break;
        }
        default:
        {
            LGS9X_SoftReset();
            break;
        }
    }
    LGS_WriteRegister(addr, 0xFF, 0x00);
	LGS_WriteRegister(addr, 0x33, 0x90); 
    return LGS_NO_ERROR;

}

LGS_RESULT ClearBER()
{
    UINT8   addr = LGS9XSECADDR1;

    // PKTRUN: 1-running and can be cleared, 0-stoped and can be read but not cleared
    LGS_RegisterSetBit( addr, 0x30, 0x10 );    // running
    LGS_RegisterSetBit( addr, 0x30, 0x08 );    // clear
    LGS_RegisterClrBit( addr, 0x30, 0x08 );    // normal

    return LGS_NO_ERROR;

}

LGS_RESULT LGS9X_SetPnmdAutoMode(DEMOD_WORK_MODE workMode)
{
   LGS_RESULT   err = LGS_NO_ERROR;

   return err;
}

LGS_RESULT LGS9X_CheckLocked(DEMOD_WORK_MODE workMode, UINT8 *locked1, UINT8 *locked2, UINT16 waitms)
{
    UINT8   err = LGS_NO_ERROR;
    UINT8   addr = LGS9XSECADDR1;
    UINT8   data = 0x0;

    if (workMode != DEMOD_WORK_MODE_ANT1_DVBC)
    {
        if (locked1 == NULL)
            return LGS_PARA_ERROR;
        else
            *locked1 = 1;       //unlock
    }
    if (workMode != DEMOD_WORK_MODE_ANT1_DTMB)
    {
        if (locked2 == NULL)
            return LGS_PARA_ERROR;
        else
            *locked2 = 1;       //unlock
    }

    if (waitms != 0)
        LGS_Wait(waitms);

    switch(workMode)
    {
        case DEMOD_WORK_MODE_ANT1_DTMB:
        {
            //GB check lock status
            LGS_WriteRegister(addr, 0xFF, 0x00);
            LGS_ReadRegister(addr, 0x13, &data);

            data &= 0x90;       //Check Flag CA_Locked and AFC_Locked
            if (data == 0x90)
                *locked1 = 0;   //lock
            else
                *locked1 = 1;   //unlock
            break;
        }

        case DEMOD_WORK_MODE_ANT1_DVBC:
        {
            //DVBC check lock status
            LGS_ReadRegister(addr, 0xDA, &data);
            if (data & 0x80)
                *locked2 = 0;   //lock
            else
                *locked2 = 1;   //unlock
            break;
        }
    }
    return LGS_NO_ERROR;
}

LGS_RESULT LGS9X_I2CEchoOn()
{
    UINT8  addr = LGS9XSECADDR1;
    LGS_ReadRegister(addr, 0xFF, &g_FF_reg);
    LGS_WriteRegister(addr, 0xFF, 0x00);
    LGS_RegisterSetBit(addr, 0x01, 0x80);
    return LGS_NO_ERROR; 
}

LGS_RESULT LGS9X_I2CEchoOff()
{
    UINT8       addr = LGS9XSECADDR1;
    LGS_RegisterClrBit(addr, 0x01, 0x80);
    LGS_WriteRegister(addr, 0xFF, g_FF_reg);
    return LGS_NO_ERROR;
}

LGS_RESULT LGS9X_GetConfig(DEMOD_WORK_MODE workMode, DemodConfig *pDemodConfig)
{
    LGS_RESULT  err = LGS_NO_ERROR;
    UINT8       addr = LGS9XSECADDR1;
    UINT8       dat;
    DemodConfig *pPara = pDemodConfig;

    switch(workMode)
    {
        case DEMOD_WORK_MODE_ANT1_DTMB:
        {
            switch(workMode)
        	{
                case DEMOD_WORK_MODE_ANT1_DTMB:
            	{
                    LGS_WriteRegister(addr, 0xFF, 0x00);
                    break;
            	}
        	}

            //CarrierMode, FecRate, TimeDeintvl
            LGS_ReadRegister(addr, 0x1F, &dat);
            if ( PARA_IGNORE != pPara->SubCarrier )     pPara->SubCarrier   = (dat & 0x1C) >> 2;
            if ( PARA_IGNORE != pPara->FecRate )        pPara->FecRate      = (dat & 0x03);
            if ( PARA_IGNORE != pPara->TimeDeintvl )    pPara->TimeDeintvl  = (dat & 0x20) >> 5;

            // GuardIntvl CarrierMode, PnNumber
            LGS_ReadRegister(addr, 0x13, &dat);
            if ( PARA_IGNORE != pPara->GuardIntvl )     pPara->GuardIntvl   = (dat & 0x60) >> 5;
            if ( PARA_IGNORE != pPara->CarrierMode )    pPara->CarrierMode  = (dat & 0x08) >> 3;
            if ( PARA_IGNORE != pPara->PnNumber )       pPara->PnNumber     = (dat & 0x01);

            // IsMpegClockInvert
            LGS_ReadRegister(addr, 0x30, &dat);
            if ( PARA_IGNORE != pPara->IsMpegClockInvert )  pPara->IsMpegClockInvert    = (dat & 0x02) >> 1;

            // BCHCount, BCHPktErrCount
            if (PARA_IGNORE!=(pPara->BCHCount&0xff) || PARA_IGNORE != (pPara->BCHPktErrCount&0xff) )
        	{
                LGS_RegisterClrBit( addr, 0x31, 0x20 ); // GB receive TS block numbers
                LGS_RegisterClrBit( addr, 0x30, 0x10 ); // stop counter

                //Total block count and Error block count
                if (PARA_IGNORE!=(pPara->BCHCount & 0xff))
            	{
                    pPara->BCHCount = 0;
                    LGS_ReadRegister(addr, 0x2B, &dat);
                    pPara->BCHCount |= dat;
                    pPara->BCHCount <<= 8;
                    LGS_ReadRegister(addr, 0x2A, &dat);
                    pPara->BCHCount |= dat;
                    pPara->BCHCount <<= 8;
                    LGS_ReadRegister(addr, 0x29, &dat);
                    pPara->BCHCount |= dat;
                    pPara->BCHCount <<= 8;
                    LGS_ReadRegister(addr, 0x28, &dat);
                    pPara->BCHCount |= dat;
            	}

                if (PARA_IGNORE!= (pPara->BCHPktErrCount & 0xff))
            	{
                    pPara->BCHPktErrCount = 0;
                    LGS_ReadRegister(addr, 0x2F, &dat);
                    pPara->BCHPktErrCount |= dat;
                    pPara->BCHPktErrCount <<= 8;
                    LGS_ReadRegister(addr, 0x2E, &dat);
                    pPara->BCHPktErrCount |= dat;
                    pPara->BCHPktErrCount <<= 8;
                    LGS_ReadRegister(addr, 0x2D, &dat);
                    pPara->BCHPktErrCount |= dat;
                    pPara->BCHPktErrCount <<= 8;
                    LGS_ReadRegister(addr, 0x2C, &dat);
                    pPara->BCHPktErrCount |= dat;
            	}

                LGS_RegisterSetBit( addr, 0x30, 0x10 ); // running
        	}

            // AFCPhase
            if ( PARA_IGNORE != (pPara->AFCPhase & 0xff) )
        	{
                pPara->AFCPhase = 0;
                LGS_ReadRegister(addr, 0x23, &dat);
                pPara->AFCPhase |= dat;
                pPara->AFCPhase <<= 8;
                LGS_ReadRegister(addr, 0x22, &dat);
                pPara->AFCPhase |= dat;
                pPara->AFCPhase <<= 8;
                LGS_ReadRegister(addr, 0x21, &dat);
                pPara->AFCPhase |= dat;
        	}

            // AFCPhaseInit
            if ( PARA_IGNORE != (pPara->AFCPhaseInit & 0xff) )
        	{
                pPara->AFCPhaseInit = 0;
                LGS_ReadRegister(addr, 0x0B, &dat);
                pPara->AFCPhaseInit |= dat;
                pPara->AFCPhaseInit <<= 8;
                LGS_ReadRegister(addr, 0x0A, &dat);
                pPara->AFCPhaseInit |= dat;
                pPara->AFCPhaseInit <<= 8;
                LGS_ReadRegister(addr, 0x09, &dat);
                pPara->AFCPhaseInit |= dat;
                pPara->AFCPhaseInit <<= 8;
                LGS_ReadRegister(addr, 0x08, &dat);
                pPara->AFCPhaseInit |= dat;
        	}
            break;
        }
    }

    switch(workMode)
    {
        case DEMOD_WORK_MODE_ANT1_DVBC:
        {
            switch(workMode)
        	{
                case DEMOD_WORK_MODE_ANT1_DVBC:
            	{
                    LGS_WriteRegister(addr, 0xFF, 0x00);
                    LGS_RegisterSetBit( addr, 0x31, 0x20 ); // DVBC receive TS block numbers

                    break;
            	}
        	}
            // BCHCount, BCHPktErrCount
            if ( PARA_IGNORE != (pPara->BCHCount2 & 0xff)
                || PARA_IGNORE != (pPara->BCHPktErrCount2 & 0xff) )
        	{
                LGS_RegisterClrBit( addr, 0x30, 0x10 ); // stop counter

                //Total block count and Error block count
                if ( PARA_IGNORE != (pPara->BCHCount2 & 0xff) )
            	{
                    pPara->BCHCount2 = 0;
                    LGS_ReadRegister(addr, 0x2B, &dat);
                    pPara->BCHCount2 |= dat;
                    pPara->BCHCount2 <<= 8;
                    LGS_ReadRegister(addr, 0x2A, &dat);
                    pPara->BCHCount2 |= dat;
                    pPara->BCHCount2 <<= 8;
                    LGS_ReadRegister(addr, 0x29, &dat);
                    pPara->BCHCount2 |= dat;
                    pPara->BCHCount2 <<= 8;
                    LGS_ReadRegister(addr, 0x28, &dat);
                    pPara->BCHCount2 |= dat;
            	}

                if ( PARA_IGNORE != (pPara->BCHPktErrCount2 & 0xff) )
            	{
                    pPara->BCHPktErrCount2 = 0;
                    LGS_ReadRegister(addr, 0x2F, &dat);
                    pPara->BCHPktErrCount2 |= dat;
                    pPara->BCHPktErrCount2 <<= 8;
                    LGS_ReadRegister(addr, 0x2E, &dat);
                    pPara->BCHPktErrCount2 |= dat;
                    pPara->BCHPktErrCount2 <<= 8;
                    LGS_ReadRegister(addr, 0x2D, &dat);
                    pPara->BCHPktErrCount2 |= dat;
                    pPara->BCHPktErrCount2 <<= 8;
                    LGS_ReadRegister(addr, 0x2C, &dat);
                    pPara->BCHPktErrCount2 |= dat;
            	}

                LGS_RegisterSetBit( addr, 0x30, 0x10 ); // running
        	}

            // AFCPhase
            if ( PARA_IGNORE != (pPara->AFCPhase2 & 0xff) )
        	{
                pPara->AFCPhase2 = 0;
                LGS_ReadRegister(addr, 0x23, &dat);
                pPara->AFCPhase2 |= dat;
                pPara->AFCPhase2 <<= 8;
                LGS_ReadRegister(addr, 0x22, &dat);
                pPara->AFCPhase2 |= dat;
                pPara->AFCPhase2 <<= 8;
                LGS_ReadRegister(addr, 0x21, &dat);
                pPara->AFCPhase2 |= dat;
        	}

            // AFCPhaseInit
            if ( PARA_IGNORE != (pPara->AFCPhaseInit2 & 0xff) )
        	{
                pPara->AFCPhaseInit2 = 0;
                LGS_ReadRegister(addr, 0x0B, &dat);
                pPara->AFCPhaseInit2 |= dat;
                pPara->AFCPhaseInit2 <<= 8;
                LGS_ReadRegister(addr, 0x0A, &dat);
                pPara->AFCPhaseInit2 |= dat;
                pPara->AFCPhaseInit2 <<= 8;
                LGS_ReadRegister(addr, 0x09, &dat);
                pPara->AFCPhaseInit2 |= dat;
                pPara->AFCPhaseInit2 <<= 8;
                LGS_ReadRegister(addr, 0x08, &dat);
                pPara->AFCPhaseInit2 |= dat;
        	}
            break;
        }
    }
    return LGS_NO_ERROR;
}

LGS_RESULT LGS9X_SetConfig(DEMOD_WORK_MODE workMode, DemodConfig *pDemodConfig)
{
    LGS_RESULT  err = LGS_NO_ERROR;
    UINT8       addr = LGS9XSECADDR1;
    UINT8       dat;
    UINT8       reg_14;
    DemodConfig *pPara = pDemodConfig;

    if( PARA_IGNORE != pPara->AdType)
    {
        g_AdType = pPara->AdType;
        switch (pPara->AdType)
        {
        case 0:
            // is internal ad , so far do not write any register.
            break;
        case 1:     	
             //is external ad9203 , need write any register.
            LGS_ReadRegister(addr, 0xf9, &dat);
            dat |= 0x01; // f9[0] = 1
            LGS_WriteRegister(addr, 0xf9, dat);
        	
            LGS_Wait(100);
            LGS_ReadRegister(addr, 0x14, &reg_14);
            dat = reg_14 & 0xf0;
            dat = reg_14 | 0x03;
            LGS_WriteRegister(addr, 0x14, dat);

            dat = dat    & 0xfC;
            dat = dat    | 0x02;
            LGS_WriteRegister(addr, 0x14, dat);

            dat = dat    & 0xfC;
            LGS_WriteRegister(addr, 0x14, dat);
        	
            LGS_Wait(10);
        	
            LGS_ReadRegister(addr, 0xd3, &dat);
            dat |= 0x08; // d3[3] = 1
            LGS_WriteRegister(addr, 0xd3, dat);

            LGS_ReadRegister(addr, 0x03, &dat);
            dat |= 0x80; // 03[7] = 1
            LGS_WriteRegister(addr, 0x03, dat);
            LGS_ReadRegister(addr, 0xd6, &dat);
            dat |= 0x04; // d6[2] = 1
            LGS_WriteRegister(addr, 0xd6, dat);

            LGS_ReadRegister(addr, 0x30, &dat);
            dat |= 0x01; // 30[0] = 1 
            LGS_WriteRegister(addr, 0x30, dat);

            LGS_WriteRegister(addr, 0xc7, 0x94);
            LGS_WriteRegister(addr, 0xcf, 0x55);
            LGS_WriteRegister(addr, 0x14, reg_14);
            break;
        case 2:
            // is external ad, reserved.
            break;
        default:
            break;
        }
    }
    return LGS_NO_ERROR;

}

//The parameter antNum must is 0 or 1
LGS_RESULT ReadPNPara(UINT8 antNum,PN_PARA *para)
{
    UINT8 reg=0;
    
    LGS_WriteRegister(0x3A, 0xFF, antNum);
    LGS_ReadRegister(0x3A, 0x13, &reg); //read auto detect register

    para->PNMDDone=(BOOL)reg & 0x04;
    para->pn_phase=(BOOL)reg & 0x1;
    para->CAlock=(BOOL)reg &0x80;
        
    if(reg & 0x40) //Guard Interval
    {para->pn_mode=2;}
     else
        if(reg & 0x20)
            para->pn_mode=1;
        else
            para->pn_mode=0;

    para->CarrierMode=(BOOL)(reg & 0x08);//CarrierMode

    return LGS_NO_ERROR;
}

//The parameter uAntenna must is 0 or 1
LGS_RESULT LGS9X_GetDtmbSignalQuality(DEMOD_WORK_MODE workMode,UINT8 *signalQuality)
{
    //Total Block and Error Block Counter
    DemodConfig para1, para2;    //demode core work parameter
    INT32       BCHCount=0, ErrBCHCount=0;
    UINT8 data =0,tmp=0;//,PNmode=0,CarrierMode=0;
    UINT8 uNM=0;
    UINT16 uN1=0,uN2=0;
    UINT8 uQAM=0; //
    UINT8 uTPSDone=0;
    UINT32 fBerRate=0;  //BER percent

    PN_PARA para; //current demod core parameter
    UINT8 err = LGS_NO_ERROR;

    if(workMode!=DEMOD_WORK_MODE_ANT1_DTMB)
    {
       err = LGS_PARA_ERROR;
       return err;
    }
    DemodParaIgnoreAll(&para1);
    DemodParaIgnoreAll(&para2);

    para1.BCHCount = 0;
    para1.BCHPktErrCount = 0;
    LGS9X_GetConfig(workMode, &para1);
    
    LGS_Wait(100);
    
    para2.BCHCount = 0;
    para2.BCHPktErrCount =0;
    LGS9X_GetConfig(workMode, &para2);

    if(para2.BCHCount>=para1.BCHCount)
        BCHCount =para2.BCHCount - para1.BCHCount;
    else
        BCHCount = para2.BCHCount+(0xFFFFFFFF-para1.BCHCount);
    if(para2.BCHPktErrCount>=para1.BCHPktErrCount)
        ErrBCHCount = para2.BCHPktErrCount -para1.BCHPktErrCount;
    else
        ErrBCHCount = para2.BCHPktErrCount+(0xFFFFFFFF-para1.BCHPktErrCount);

    if(BCHCount>0 && ErrBCHCount>0)
    {
        fBerRate=100* ErrBCHCount/BCHCount;
        if(fBerRate>0 && fBerRate<5)
            *signalQuality = 35;
        else if(fBerRate>=5 && fBerRate<10)
            *signalQuality = 30;
        else if(fBerRate>=10 && fBerRate<20)
            *signalQuality = 20;
        else 
            *signalQuality = 0;
        return err;
    }
    
    memset(&para, 0,sizeof(PN_PARA));
    
    if(workMode == DEMOD_WORK_MODE_ANT1_DTMB)
    {
        ReadPNPara(workMode, &para);
        if(para.PNMDDone==0 || para.CAlock==0)
        {
            *signalQuality = 0;
            return err;
        }
    }      

    LGS_ReadRegister(LGS9XSECADDR1, 0x1F, &uTPSDone);
    uTPSDone=uTPSDone & 0x80;

    if(uTPSDone==0)
    {
        *signalQuality = 0;
        return err;
    }

    //Get Noise Magnitude
    //Option First or second demod core
    LGS_WriteRegister(0x3A, 0xFF, workMode);

    if(para.pn_mode==1 && para.CarrierMode) //if Guard Interval=PN595 And CarrierMode=SC
    {
        LGS_WriteRegister(LGS9XSECADDR2,0xFF, 0x0E);
        LGS_ReadRegister(LGS9XSECADDR2, 0x80, &uNM);
    }
    else
    {
        LGS_ReadRegister(LGS9XSECADDR1, 0x34, &uNM);
    }
    //Get QAM type from 1F Register
    err=LGS_ReadRegister(LGS9XSECADDR1, 0x1F, &uQAM);
    uQAM=(uQAM & 0x1C) >> 2;
    
    switch(uQAM)
    {   case 0:
        case 1: uN1=0x06E0;uN2=9;       break;  //4QAM
        case 2: uN1=0x07BC;uN2=0x0A;    break; //16QAM
        case 3:
        case 4: uN1=0x082A; uN2=0x0B;   break;//32/64QAM
        default:
                uN1=0x07BC; uN2=0x0A;   break;
    }

    *signalQuality =((uN1-uN2*uNM)>>5) +40;
    if(*signalQuality> 100) *signalQuality= 100;
    return err;
}


//The parameter uAntenna must is 0 or 1
UINT16 LGS9X_AnalyseStrength(UINT8 reg_B1)
{
     UINT16 scale = 0;
     if (reg_B1<= SS_LEVEL10)       //-60dbm
      scale = 100;
     else if (reg_B1<=SS_LEVEL9)    //-65dbm
      scale = 90;
     else if (reg_B1<=SS_LEVEL8)    //-68dbm
      scale = 80; 	 
     else if (reg_B1<=SS_LEVEL7)    //-72dbm
      scale = 70;
     else if (reg_B1<=SS_LEVEL6)    //-75dbm
      scale = 60;   
     else if (reg_B1<=SS_LEVEL5)    //-78dbm
      scale = 50;
     else if (reg_B1<=SS_LEVEL4)    //-80dbm
      scale = 40;
     else if (reg_B1<=SS_LEVEL3)    //-82dbm
      scale = 30;
     else if (reg_B1<=SS_LEVEL2)    //-85dbm
      scale = 20;
     else if (reg_B1<=SS_LEVEL1)    //-88dbm
      scale = 10;
     else 
      scale = 0;
     return scale;
}

LGS_RESULT LGS9X_GetSignalStrength(DEMOD_WORK_MODE workMode, UINT32 *SignalStrength)
{
		 UINT8 reg_B1_0 = 0;
		 UINT8 reg_B1_1 = 0;
		 UINT8 reg_FF = 0;
		 UINT8 addr = LGS9XSECADDR1;
		 UINT8 err = LGS_NO_ERROR;
		 
		 LGS_ReadRegister(addr,0xFF,&reg_FF);
		 switch(workMode)
		 {
			 case DEMOD_WORK_MODE_ANT1_DTMB:
				 LGS_WriteRegister(addr,0xFF,0x00);
				 LGS_ReadRegister(addr, 0xB1, &reg_B1_0);
			     break;
			 
			 default:
				 err = LGS_PARA_ERROR;
			     break;
		 }
		 
		 if(workMode==DEMOD_WORK_MODE_ANT1_DTMB)
		 {
			*SignalStrength= LGS9X_AnalyseStrength(reg_B1_0);	
		 }
		 LGS_WriteRegister(addr,0xFF,reg_FF);
		 
		 return err;
			 
}


LGS_RESULT LGS9X_GetBER(DEMOD_WORK_MODE workMode,UINT16 delaytime,UINT8 *ber)
{
    LGS_RESULT  err = LGS_NO_ERROR;
    UINT8       locked1 = 0;
    UINT8       locked2 = 0;
        DemodConfig para1, para2;
    
    UINT32      BCHCount=0, ErrBCHCount=0;

    LGS9X_CheckLocked(workMode,&locked1,&locked2,10);
    if((locked1==0) || (locked2==0))
    {
            DemodParaIgnoreAll(&para1);
            DemodParaIgnoreAll(&para2);
        	
            para1.BCHCount = 0;
            para1.BCHPktErrCount = 0;
            LGS9X_GetConfig(workMode, &para1);
        	
            LGS_Wait(delaytime);
        	
            para2.BCHCount = 0;
            para2.BCHPktErrCount =0;
            LGS9X_GetConfig(workMode, &para2);

            if(para2.BCHCount>=para1.BCHCount)
                        {
                BCHCount =para2.BCHCount - para1.BCHCount;
                        }
                else
                        {
                BCHCount = para2.BCHCount+(0xFFFFFFFF-para1.BCHCount);
        	}
                        if(para2.BCHPktErrCount>=para1.BCHPktErrCount)
                        {
                ErrBCHCount = para2.BCHPktErrCount -para1.BCHPktErrCount;
                        }
                else
                        {
                ErrBCHCount = para2.BCHPktErrCount+0xFFFFFFFF-para1.BCHPktErrCount;
                        }

                if(BCHCount == 0)
                   *ber = 100;  
                else
                { 
                   *ber = 100*ErrBCHCount/BCHCount;
                }
            return err;
     }
     else
     {
         *ber = 100;
         err = LGS_PARA_ERROR;
         return err;
     }
    
}

LGS_RESULT LGS9X_SoftReset()
{
    LGS_RESULT  err = LGS_NO_ERROR;
    UINT8       addr = LGS9XSECADDR1;
    UINT8       dat;

    err = LGS_ReadRegister(addr, 0x02, &dat);
    if (err != LGS_NO_ERROR)
        return err;

    dat = dat & 0xFE;

    err = LGS_WriteRegister(addr, 0x02, dat);
    if (err != LGS_NO_ERROR)
        return err;

    dat = dat | 0x01;
    err = LGS_WriteRegister(addr, 0x02, dat);
    if (err != LGS_NO_ERROR)
        return err;

    return LGS_NO_ERROR;
}


