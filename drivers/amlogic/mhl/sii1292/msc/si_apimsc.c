/*
 * Silicon Image SiI1292 Device Driver
 *
 * Author: Amlogic, Inc.
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#include "../cbus/si_apicbus.h"
#include "../cbus/si_cbus_regs.h"
#include "../msc/si_apimsc.h"
#include "../hal/si_hal.h"
#include "../main/si_cp1292.h"
#include "../api/si_api1292.h"



/***** public functions *******************************************************/

/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: CLR_HPD
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscClrHpd(uint8_t channel, uint8_t uData)
{
	cbus_out_queue_t req;
	uData = uData;

	SI_CbusSetHpdState(channel, false);
	SI_CbusOQCleanActiveReq(channel);

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: Path Disable\n");
	req.command     = MSC_WRITE_STAT;
	req.offsetData  = MHL_DEVICE_STATUS_LINK_MODE_REG_OFFSET;
	req.msgData[0]	= 0;
	req.dataRetHandler = SI_MscPathDisable;
	req.retry = 2;	// retry 2 times if timeout or abort for important MSC commands
	SI_CbusPushReqInOQ( channel, &req, true );

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: SET_HPD
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscSetHpd(uint8_t channel, uint8_t uData)
{
	cbus_out_queue_t req;
	uData = uData;

 	//Jin: Fix for Bug#22149
    //SiIRegioCbusWrite( REG_CBUS_DEVICE_CAP_0, channel, POWER_STATE_ACTIVE);

	SI_CbusSetHpdState(channel, true);
	SI_CbusOQCleanActiveReq(channel);

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: Path Enable\n");
	req.command     = MSC_WRITE_STAT;
	req.offsetData  = MHL_DEVICE_STATUS_LINK_MODE_REG_OFFSET;
	req.msgData[0]	= PATH_EN;
	req.dataRetHandler = SI_MscPathEnable;
	req.retry = 2;	// retry 2 times if timeout or abort for important MSC commands
	SI_CbusPushReqInOQ( channel, &req, true );

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: SET_INT, GRT_WRT
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI9292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscGrtWrt(uint8_t channel, uint8_t uData)
{
	uData = uData;

	SI_CbusSetGrtWrt(channel, true);
	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: WRITE_STAT, Path enable
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscPathEnable(uint8_t channel, uint8_t uData)
{
	uData = uData;

	SI_CbusSetPathEnable(channel, true);
	SI_CbusOQCleanActiveReq(channel);

	return (true);
}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: WRITE_STAT, Path enable
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscPathDisable(uint8_t channel, uint8_t uData)
{
	uData = uData;

	SI_CbusSetPathEnable(channel, false);
	SI_CbusOQCleanActiveReq(channel);

	return (true);

}

/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: SET_INT, DCAP_CHG
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI9292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscDevCapChange(uint8_t channel, uint8_t uData)
{
	uData = uData;

	SI_CbusOQCleanActiveReq(channel);

	return (true);
}

/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: WRITE_STAT, Connect Ready
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscConnectReady(uint8_t channel, uint8_t uData)
{
    cbus_out_queue_t req;
	uData = uData;

	SI_CbusOQCleanActiveReq(channel);

    //after send DCAP_RDY, send DCAP_CHG to inform source to read devcap , tiger qin 08-10-2011

    req.command     = MSC_SET_INT;
    req.offsetData  = MHL_REG_CHANGE_INT_OFFSET;
    req.msgData[0]	= CBUS_INT_BIT_DCAP_CHG;
    req.dataRetHandler = SI_MscDevCapChange;
    req.retry = 2;	// retry 2 times if timeout or abort for important MSC commands

    SI_CbusPushReqInOQ( channel, &req, true );
    DEBUG_PRINT(MSG_ALWAYS, "CBUS:: Self Device Capability Registers Change\n");
	return (true);

}

/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, bandwidth info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadBandwidth(uint8_t channel, uint8_t uData)
{
	uint32_t frequency;
//	uint8_t  tmp = 0 ;
	PeerDevCap[9] = uData;

	if ( uData == 0 )
	{
		DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: No Bandwidth is indicated\n");
	}
	else
	{
		frequency = uData * 5;
		DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Bandwidth: %dMHz\n", (int)frequency);
	}

	SI_CbusSetReadInfo(channel, DCAP_READ_DONE);//finish read
	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, video type info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadVideoType(uint8_t channel, uint8_t uData)
{
	PeerDevCap[7] = uData;

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Support Video Type: ");

	if ( uData & SUPP_VT )
	{
		if( uData & VT_GRAPHICS)
			DEBUG_PRINT(MSG_ALWAYS, "Graphics type of video content; ");
		if( uData & VT_PHOTO)
			DEBUG_PRINT(MSG_ALWAYS, "Photo type of video content; ");
		if( uData & VT_CINEMA)
			DEBUG_PRINT(MSG_ALWAYS, "Cinema type of video content; ");
		if( uData & VT_GAME)
			DEBUG_PRINT(MSG_ALWAYS, "Game type of video content; ");
	}
	else
	{
		DEBUG_PRINT(MSG_ALWAYS, "None");
	}

	DEBUG_PRINT(MSG_ALWAYS, "\n");

	SI_CbusSetReadInfo(channel, ((SI_CbusGetReadInfo(channel)&DCAP_ITEM_INDEX_MASK) +1) &(~ DCAP_ITEM_READ_DOING));//finsh read

	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, audio link mode info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadAudioLinkMode(uint8_t channel, uint8_t uData)
{
	PeerDevCap[6] = uData;

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Audio Link Mode: ");

	if ( uData == 0 )
	{
		DEBUG_PRINT(MSG_ALWAYS, "None");
	}
	else
	{
		if( uData & AUD_2CH )
			DEBUG_PRINT(MSG_ALWAYS, "2-Channel PCM audio streams (with Layout 0 audio data packets) ");
		if( uData & AUD_8CH )
			DEBUG_PRINT(MSG_ALWAYS, "8-channel PCM audio streams (with Layout 1 audio data packets) ");
	}

	DEBUG_PRINT(MSG_ALWAYS, "\n");

	SI_CbusSetReadInfo(channel, ((SI_CbusGetReadInfo(channel)&DCAP_ITEM_INDEX_MASK) +1) &(~ DCAP_ITEM_READ_DOING));//finsh read

	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, video link mode info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadVideoLinkMode(uint8_t channel, uint8_t uData)
{
	PeerDevCap[5] = uData;

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Video Link Mode: ");

	if ( uData == 0 )
	{
		DEBUG_PRINT(MSG_ALWAYS, "None");
	}
	else
	{
		if( uData & SUPP_RGB444 )
			DEBUG_PRINT(MSG_ALWAYS, "RGB 4:4:4 ");
		if( uData & SUPP_YCBCR444 )
			DEBUG_PRINT(MSG_ALWAYS, "YCBCR 4:4:4 ");
		if( uData & SUPP_YCBCR422 )
			DEBUG_PRINT(MSG_ALWAYS, "YCBCR 4:2:2 ");
		if( uData & SUPP_PPIXEL )
			DEBUG_PRINT(MSG_ALWAYS, "Packed Pixel ");
		if( uData & SUPP_ISLANDS)
			DEBUG_PRINT(MSG_ALWAYS, "Data Islands ");
	}

	DEBUG_PRINT(MSG_ALWAYS, "\n");
	SI_CbusSetReadInfo(channel, ((SI_CbusGetReadInfo(channel)&DCAP_ITEM_INDEX_MASK) +1) &(~ DCAP_ITEM_READ_DOING));//finsh read
	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, Logical device map info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadLogDevMap(uint8_t channel, uint8_t uData)
{
	PeerDevCap[8] = uData;

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Log Dev Map: ");

	if ( uData == 0 )
	{
		DEBUG_PRINT(MSG_ALWAYS, "None");
	}
	else
	{
		if( uData & LD_DISPLAY )
			DEBUG_PRINT(MSG_ALWAYS, "Display ");
		if( uData & LD_VIDEO )
			DEBUG_PRINT(MSG_ALWAYS, "Video ");
		if( uData & LD_AUDIO )
			DEBUG_PRINT(MSG_ALWAYS, "Audio ");
		if( uData & LD_MEDIA )
			DEBUG_PRINT(MSG_ALWAYS, "Media ");
		if( uData & LD_TUNER )
			DEBUG_PRINT(MSG_ALWAYS, "Tuner ");
		if( uData & LD_RECORD )
			DEBUG_PRINT(MSG_ALWAYS, "Record ");
		if( uData & LD_SPEAKER )
			DEBUG_PRINT(MSG_ALWAYS, "Speaker ");
		if( uData & LD_GUI )
			DEBUG_PRINT(MSG_ALWAYS, "Gui ");
	}

	DEBUG_PRINT(MSG_ALWAYS, "\n");

	SI_CbusSetReadInfo(channel, ((SI_CbusGetReadInfo(channel)&DCAP_ITEM_INDEX_MASK) +1) &(~ DCAP_ITEM_READ_DOING));//finsh read

	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, device category info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadDevCat(uint8_t channel, uint8_t uData)
{
	PeerDevCap[2] = uData;

	switch (uData)
	{
		case SOURCE_DEV:
			DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Category: Source Device\n");
			break;

		case OTHER_DEV_CAT:
			DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Category: Other Device\n");
			break;

		default:
			DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Category: Incorrect\n");
			break;

	}

	SI_CbusSetReadInfo(channel, ((SI_CbusGetReadInfo(channel)&DCAP_ITEM_INDEX_MASK) +1) &(~ DCAP_ITEM_READ_DOING));//finsh read

	SI_CbusOQCleanActiveReq(channel);
	return (true);
}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, MHL version info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadMhlVersion(uint8_t channel, uint8_t uData)
{
	uint8_t mhl_major, mhl_minor;

	PeerDevCap[1] = uData;

	mhl_major = (uData & 0xF0)>>4;
	mhl_minor = uData & 0x0F;

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: MHL Version %d.%d\n", (int)mhl_major, (int)mhl_minor);

	SI_CbusSetReadInfo(channel, ((SI_CbusGetReadInfo(channel)&DCAP_ITEM_INDEX_MASK) +1) &(~ DCAP_ITEM_READ_DOING));//finsh read

	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus MSC command transfer done handler: MSC command: READ_DEVCAP, feature flag info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          uData			The return data of the command when transfer done
 *
 *  @return             Result
 *  @retval             true, Success
 *  @retval             false, Failure
 *
 *****************************************************************************/
bool_t SI_MscReadFeatureFlag(uint8_t channel, uint8_t uData)
{
//	uint8_t result;
//	cbus_out_queue_t req;
	bool_t rcp_support, rap_support;

	PeerDevCap[10] = uData;

	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Feature: RCP Support: %s\n", (uData & RCP_SUPPORT)?"Yes":"No");
    rcp_support = (uData & RCP_SUPPORT) ? true : false;
	SI_CbusSetRcpSupport(channel, rcp_support);
	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: MHL Source Device Info: Feature: RAP Support: %s\n", (uData & RAP_SUPPORT)?"Yes":"No");
    rap_support = (uData & RAP_SUPPORT) ? true : false;
	SI_CbusSetRapSupport(channel, rap_support);

	SI_CbusSetReadInfo(channel, ((SI_CbusGetReadInfo(channel)&DCAP_ITEM_INDEX_MASK) +1) &(~ DCAP_ITEM_READ_DOING));//finsh read

	SI_CbusOQCleanActiveReq(channel);

	return (true);

}



/*****************************************************************************/
/**
 *  Cbus operation: start to get the peer's device cap registers' info
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *
 *  @return             void
 *
 *****************************************************************************/
typedef struct tagDevcapRead
{
    uint8_t addressoffset;
    dataRetHandler_t dataRetHandler;
}DevcapReadItem_t;

DevcapReadItem_t  DevcapReadArray[]=
{
    {MHL_DEV_CAP_FEATURE_FLAG_REG_OFFSET,   SI_MscReadFeatureFlag},
    {MHL_DEV_CAP_MHL_VERSION_REG_OFFSET,    SI_MscReadMhlVersion},
    {MHL_DEV_CAP_DEV_CAT_REG_OFFSET,        SI_MscReadDevCat},
	{MHL_DEV_CAP_LOG_DEV_MAP_REG_OFFSET,    SI_MscReadLogDevMap},
	{MHL_DEV_CAP_VID_LINK_MODE_REG_OFFSET,  SI_MscReadVideoLinkMode},
	{MHL_DEV_CAP_AUD_LINK_MODE_REG_OFFSET,  SI_MscReadAudioLinkMode},
	{MHL_DEV_CAP_VIDEO_TYPE_REG_OFFSET,     SI_MscReadVideoType},
	{MHL_DEV_CAP_BANDWIDTH_REG_OFFSET,      SI_MscReadBandwidth},
};

#define DevcapReadItemCount (sizeof(DevcapReadArray)/sizeof(DevcapReadArray[0]))

void SI_MscStartGetDevInfo(uint8_t channel)
{
	uint8_t result;
	cbus_out_queue_t req;
	uint8_t index, read_info;

    read_info = SI_CbusGetReadInfo(channel);
	index = read_info & DCAP_ITEM_INDEX_MASK;
	read_info &= (~DCAP_ITEM_INDEX_MASK);

	if(read_info & DCAP_READ_DONE )
	{
//        DEBUG_PRINT(MSG_ALWAYS, ("CBUS:: DCAP_READ_DONE \n"));
		return ;
	}

	if(read_info & DCAP_ITEM_READ_DOING )
	{
//        DEBUG_PRINT(MSG_ALWAYS, ("CBUS:: current DCAP READ ITEM not done yet\n"));
        return;
	}

	if(index >=DevcapReadItemCount)
	{
//        DEBUG_PRINT(MSG_ALWAYS, ("CBUS:: index error \n"));
        return;
	}

    DEBUG_PRINT(MSG_ALWAYS, "CBUS:: Read MHL Source Device infomation\n");
    DEBUG_PRINT(MSG_ALWAYS, "CBUS:: current_item_index = %d\n",(int)index);

//    DEBUG_PRINT(MSG_ALWAYS, ("CBUS:: DevcapReadItemCount = %d\n",(int)DevcapReadItemCount));

	req.command     = MSC_READ_DEVCAP;
	req.offsetData  = DevcapReadArray[index].addressoffset;
	req.dataRetHandler = DevcapReadArray[index].dataRetHandler;
	req.retry = 2;	// retry 2 times if timeout or abort for important MSC commands
	result = SI_CbusPushReqInOQ( channel, &req, true );		// make sure the first Read_Dev_Cap can be put in the queue

	if ( result != SUCCESS )
	{
		DEBUG_PRINT(MSG_DBG, "Cbus:: SI_MscStartGetDevInfo, Error found in SI_CbusPushReqInOQ: %02X\n", (int)result);
	}
    else
    {
        SI_CbusSetReadInfo(channel,index | DCAP_ITEM_READ_DOING );
    }
}

/*****************************************************************************/
/**
 *  Cbus operation: Initialize the parameters of MSC related variables
 *
 *  @return             void
 *
 *****************************************************************************/
void SI_MscInitialize()
{
	uint8_t		channel;

    /* Initialize MSC variables of all channels.    */
    for ( channel = 0; channel < MHL_MAX_CHANNELS; channel++ )
    {
		SI_CbusInitParam(channel);
    }
}

bool_t  SI_WriteBurstDataDone(uint8_t channel, uint8_t uData)
{
	DEBUG_PRINT(MSG_ALWAYS, "CBUS:: WriteBurstDataDone\n");
	SI_CbusOQCleanActiveReq(channel);
	return true;
}
