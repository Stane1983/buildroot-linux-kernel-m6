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

#include "../api/si_datatypes.h"
#include "../api/si_api1292.h"
#include "../api/si_regio.h"
#include "../api/si_1292regs.h"
#include "../cbus/si_apicbus.h"
#include "../cbus/si_cbus_regs.h"
#include "../main/si_cp1292.h"
#include "../hal/si_hal.h"
#include "../msc/si_apimsc.h"


//------------------------------------------------------------------------------
// Function:    CbusConnectionCheck
// Description: Display any change in CBUS connection state and enable
//              CBUS heartbeat if channel has been connected.
// Parameters:  channel - CBUS channel to check
//------------------------------------------------------------------------------
static void CbusConnectionCheck ( uint8_t channel )
{
	static bool_t busConnected[ MHL_MAX_CHANNELS ] = {false};
	static bool_t hpd[ MHL_MAX_CHANNELS ] = {false};
	static bool_t rsen[ MHL_MAX_CHANNELS ] = {false};
	static uint32_t count = 0;
	static bool_t timer_set = false;
	uint8_t result;
	cbus_out_queue_t req;

	/* If CBUS connection state has changed for this channel,   */
	/* update channel state and hardware.                       */

	if ( busConnected[ channel ] != SI_CbusChannelConnected( channel ))
	{
        	busConnected[ channel ] = SI_CbusChannelConnected( channel );

		if ( !busConnected[ channel ] )
		{
			SiIRegioModify(REG_SRST, BIT_CBUS_RST, BIT_CBUS_RST);	// Reset CBUS block after CBUS disconnection
			SiIRegioModify(REG_SRST, BIT_CBUS_RST, 0);
			SI_CbusInitParam(channel);

			hpd[channel] = false;
			rsen[channel] = false;
			count = 0;

//			DEBUG_PRINT(MSG_DBG, ("after reset CBUS,CBUS PAGE: 0x3C = %02X\n", (int)SiIRegioCbusRead(REG_CBUS_LINK_LAYER_CTRL12,channel)));
//			fixed when abort packet received, not wait 2s.
//			write  REG_RX_MISC again when CBUS is reset;
			SiIRegioModify(REG_RX_MISC, BIT_PSCTRL_OEN|BIT_PSCTRL_OUT|BIT_PSCTRL_AUTO|BIT_HPD_RSEN_ENABLE, 0);

		}

#ifdef SUPPORT_MSCMSG_IGNORE_LIST
		SI_CbusResetMscMsgIgnoreList(channel, CBUS_OUT);
#endif
		SI_CbusInitDevCap(channel);
//      SiIRegioCbusWrite(REG_CBUS_LINK_LAYER_CTRL12,channel,0xA3);  //tiger add this ,request from jason 2011-8-4
//      //CBUS 0X3C IS SET TO DEFAULTS ,when CBUS connection status changed;

		SiIRegioWrite(REG_CBUS_INTR_STATUS, BIT_MSC_MSG_RCV);//tiger , 12-07-2-11, bi3 hardware defualt value is 1,clear it when init;
	}

	if ( busConnected[ channel] )
	{
		bool_t connect_wait, hpd_state, path_enable, connect_ready, dcap_rdy, read_info;

		connect_wait = SI_CbusGetConnectWait(channel);
		hpd_state = SI_CbusGetHpdState(channel);
		path_enable  = SI_CbusGetPathEnable(channel);
		connect_ready= SI_CbusGetConnectReady(channel);
		dcap_rdy = SI_CbusGetDcapRdy(channel);
		read_info = SI_CbusGetReadInfo(channel);

		if ( !connect_wait )
		{
			if (!timer_set)
			{
				timer_set = true;
				count = jiffies;
			}
			else
			{
				if( HalTimerDelay(count, DEM_MHL_WAIT_TIME) )
				{
					SI_CbusSetConnectWait(channel, true);
					count = 0;
					timer_set= false;
				}
			}
		}

		if ( connect_wait )
		{
			if ( hpd[channel] != g_HPD_IN )
			{
				hpd[channel] = g_HPD_IN;

				if ( hpd[channel] )
				{
					DEBUG_PRINT(MSG_ALWAYS, "CBUS:: SET HPD\n");
					req.command = MSC_SET_HPD;
					req.dataRetHandler = SI_MscSetHpd;
					req.retry = 5;	// retry 2 times if timeout or abort for important MSC commands
					result = SI_CbusPushReqInOQ( channel, &req, true );

					if ( result != SUCCESS )
					{
						DEBUG_PRINT(MSG_DBG, "CBUS:: Error found in SI_CbusPushReqInOQ: %02X\n", (int)result);
						hpd[channel] = !g_HPD_IN;
					}
				}
				else
				{
					DEBUG_PRINT(MSG_ALWAYS, "CBUS:: CLR HPD\n");
					req.command = MSC_CLR_HPD;
					req.dataRetHandler = SI_MscClrHpd;	// will send disable path in SI_MscClrHpd
					req.retry = 5;	// retry 2 times if timeout or abort for important MSC commands
					result = SI_CbusPushReqInOQ( channel, &req, true );

					if ( result != SUCCESS )
					{
						DEBUG_PRINT(MSG_DBG, "CBUS:: Error found in SI_CbusPushReqInOQ: %02X\n", (int)result);
						hpd[channel] = !g_HPD_IN;
					}
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
					SI_CbusResetMscMsgIgnoreList(channel, CBUS_IN);
#endif
				}
			}
		}

		/*if ( hpd_state )
		{
			if (rsen[channel] != g_TX_RSEN_Valid)
			{
				rsen[channel] = g_TX_RSEN_Valid;

				if( rsen[channel] )
				{
					DEBUG_PRINT(MSG_ALWAYS, ("CBUS:: Path Enable\n"));
					req.command     = MSC_WRITE_STAT;
					req.offsetData  = MHL_DEVICE_STATUS_LINK_MODE_REG_OFFSET;
					req.msgData[0]	= PATH_EN;
					req.dataRetHandler = SI_MscPathEnable;
					req.retry = 2;	// retry 2 times if timeout or abort for important MSC commands
					result = SI_CbusPushReqInOQ( channel, &req, true );

					if ( result != SUCCESS )
					{
						DEBUG_PRINT(MSG_DBG, ("CBUS:: Error found in SI_CbusPushReqInOQ: %02X\n", (int)result));
						rsen[channel] = !g_TX_RSEN_Valid;
					}
				}
			}
		}*/

		if ( path_enable )
		{
			if ( !connect_ready )
			{
				DEBUG_PRINT(MSG_ALWAYS, "CBUS:: Connected Ready\n");
				req.command     = MSC_WRITE_STAT;
				req.offsetData  = MHL_DEVICE_STATUS_CONNECTED_RDY_REG_OFFSET;
				req.msgData[0]	= DCAP_RDY;
				req.dataRetHandler = SI_MscConnectReady;
				req.retry = 2;	// retry 2 times if timeout or abort for important MSC commands
				result = SI_CbusPushReqInOQ( channel, &req, true );

				if ( result != SUCCESS )
				{
					DEBUG_PRINT(MSG_DBG, "CBUS:: Error found in SI_CbusPushReqInOQ: %02X\n", (int)result);
				}
				else
				{
					SI_CbusSetConnectReady(channel, true);
				}
			}
		}

		if ( connect_ready)
		{
			if ((!( read_info & DCAP_READ_DONE )) && dcap_rdy)
			{
				SI_MscStartGetDevInfo(channel);
			}
		}
	}

}

//------------------------------------------------------------------------------
// Function:    CpCbusProcessPrivateMessage
// Description: Get the result of the last message sent and use it appropriately
//              or process a request from the connected device.
// Parameters:  channel - CBUS channel that has message data for us.
//------------------------------------------------------------------------------
static uint8_t CpCbusProcessPrivateMessage ( uint8_t channel )
{
	uint8_t result, result1, result2;

	result1 = SI_CbusIQReqStatusCheck(channel);
	result2 = SI_CbusOQReqStatusCheck(channel);
	result = result1 | result2;

	return (result);
}



//------------------------------------------------------------------------------
// Function:    CpCbusHandler
// Description: Polls the send/receive state of the CBUS hardware.
//------------------------------------------------------------------------------
void CpCbusHandler (void)
{
	uint8_t channel, status;

	/* Check the current input to see if it switch between HDMI and MHL */

	if ( (g_currentInputMode == HDMI) )
		return;

	/* Monitor all CBUS channels.   */

	for ( channel = 0; channel < MHL_MAX_CHANNELS; channel++ )
	{
		/* Update CBUS status.  */
		SI_CbusUpdateBusStatus( channel );
		CbusConnectionCheck( channel );

		/* Monitor CBUS interrupts. */
		status = SI_CbusHandler( channel );
		if ( status != SUCCESS )
		{
			//DEBUG_PRINT(MSG_DBG, ("Error returned from SI_CbusHandler: %02X\n", (int)status));
		}
		status = CpCbusProcessPrivateMessage( channel );
		if ( status != SUCCESS )
		{
			//DEBUG_PRINT(MSG_DBG, ("Error returned from CpCbusProcessPrivateMessage: %02X\n", (int)status));
		}
	}
}

//------------------------------------------------------------------------------
// Function:    CpCbusInitialize
// Description: Initialize the CBUS subsystem and enabled the default channels
//------------------------------------------------------------------------------
void CpCbusInitialize ( void )
{
	SI_CbusInitialize();
	SI_MscInitialize();
}

