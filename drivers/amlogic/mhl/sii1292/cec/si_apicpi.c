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


#include <linux/string.h>

#include "../msc/si_apircp.h"
#include "../msc/si_apirap.h"
#include "../cec/si_apicpi.h"
#include "../cec/si_apicec.h"
#include "../cec/si_cpi_regs.h"
#include "../cec/si_cec_enums.h"
#include "../cbus/si_apicbus.h"
#include "../hal/si_hal.h"
#include "../api/si_regio.h"
#include "../api/si_1292regs.h"
#include "../api/si_api1292.h"
#include "../main/si_cp1292.h"



#if defined(CONFIG_MHL_SII1292_CEC)

//------------------------------------------------------------------------------
// Function:    SI_CpiSetLogicalAddr
// Description: Configure the CPI subsystem to respond to a specific CEC
//              logical address.
//------------------------------------------------------------------------------

BOOL SI_CpiSetLogicalAddr ( uint8_t logicalAddress )
{
    uint8_t capture_address[2];
    uint8_t capture_addr_sel = 0x01;

#if 0//Keno20120301, disable RK design, because it will effect that cannot display
	if( logicalAddress != CEC_LOGADDR_UNREGORBC)
	{
	    uint8_t cecStatus[2];

		SI_CpiSendPing( logicalAddress );
		DEBUG_PRINT(MSG_ALWAYS, "Ping...[%X] %s...", (int)logicalAddress, CpCecTranslateLA( logicalAddress ));

		// Wait for SEND_POLL's Ack, if there's ACK, return false to try another address
        HalTimerWait( 1000 );
	    SiIRegioReadBlock( REG_CEC_INT_STATUS_0, cecStatus, 2);
        if ( cecStatus[0] & BIT_TX_MESSAGE_SENT )
		{
			DEBUG_PRINT(MSG_ALWAYS, "----->Enum Ack\nTry next Address\n");
			return false;
		}
		else
		{
			DEBUG_PRINT(MSG_ALWAYS, "----->Enum No Ack\nThis address can be allocated\n");

		}
	}
#endif
	capture_address[0] = 0;
    capture_address[1] = 0;
    if( logicalAddress < 8 )
    {
        capture_addr_sel = capture_addr_sel << logicalAddress;
        capture_address[0] = capture_addr_sel;
    }
    else
    {
        capture_addr_sel   = capture_addr_sel << ( logicalAddress - 8 );
        capture_address[1] = capture_addr_sel;
    }

        // Set Capture Address

    SiIRegioWriteBlock(REG_CEC_CAPTURE_ID0, capture_address, 2 );
    SiIRegioWrite( REG_CEC_TX_INIT, logicalAddress );

	DEBUG_PRINT( MSG_ALWAYS, "CEC Logical Address: 0x%02X\n", (int)logicalAddress );

    return( true );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiSendPing
// Description: Initiate sending a ping, and used for checking available
//                       CEC devices
//------------------------------------------------------------------------------

void SI_CpiSendPing ( uint8_t bCECLogAddr )
{
    SiIRegioWrite( REG_CEC_TX_DEST, BIT_SEND_POLL | bCECLogAddr );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiWrite
// Description: Send CEC command via CPI register set
//------------------------------------------------------------------------------

BOOL SI_CpiWrite( SI_CpiData_t *pCpi )
{
    uint8_t cec_int_status_reg[2];
#if defined(CONFIG_MHL_SII1292_CEC)
    if ( !(SiIRegioRead( REG_CEC_INT_STATUS_0 ) & BIT_TX_BUFFER_FULL ))
    {
#endif
#if (INCLUDE_CEC_NAMES > CEC_NO_TEXT)
    CpCecPrintCommand( pCpi, true );
#endif
    SiIRegioModify( REG_CEC_DEBUG_3, BIT_FLUSH_TX_FIFO, BIT_FLUSH_TX_FIFO );  // Clear Tx Buffer

        /* Clear Tx-related buffers; write 1 to bits to be cleared. */

    cec_int_status_reg[0] = 0x64 ; // Clear Tx Transmit Buffer Full Bit, Tx msg Sent Event Bit, and Tx FIFO Empty Event Bit
    cec_int_status_reg[1] = 0x02 ; // Clear Tx Frame Retranmit Count Exceeded Bit.
    SiIRegioWriteBlock( REG_CEC_INT_STATUS_0, cec_int_status_reg, 2 );

        /* Send the command */

    SiIRegioWrite( REG_CEC_TX_DEST, pCpi->srcDestAddr & 0x0F );           // Destination
    SiIRegioWrite( REG_CEC_TX_COMMAND, pCpi->opcode );
    SiIRegioWriteBlock( REG_CEC_TX_OPERAND_0, pCpi->args, pCpi->argCount );
    SiIRegioWrite( REG_CEC_TRANSMIT_DATA, BIT_TRANSMIT_CMD | pCpi->argCount );
#if defined(CONFIG_MHL_SII1292_CEC)
		HalTimerWait(25);
		SiIRegioModify( REG_CEC_DEBUG_3, BIT_FLUSH_TX_FIFO, BIT_FLUSH_TX_FIFO );  // Clear Tx Buffer
    }
    else
    {
        DEBUG_PRINT( MSG_DBG,  "\nSI_CpiWrite:: CEC Write buffer full!\n" );
    }
#endif
    return( true );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiRead
// Description: Reads a CEC message from the CPI read FIFO, if present.
//------------------------------------------------------------------------------

BOOL SI_CpiRead( SI_CpiData_t *pCpi )
{
    BOOL    error = false;
    uint8_t argCount;

    argCount = SiIRegioRead( REG_CEC_RX_COUNT );

    if ( argCount & BIT_MSG_ERROR )
    {
        error = true;
    }
    else
    {
        pCpi->argCount = argCount & 0x0F;
        pCpi->srcDestAddr = SiIRegioRead( REG_CEC_RX_CMD_HEADER );
        pCpi->opcode = SiIRegioRead( REG_CEC_RX_OPCODE );
        if ( pCpi->argCount )
        {
            SiIRegioReadBlock( REG_CEC_RX_OPERAND_0, pCpi->args, pCpi->argCount );
        }
    }

        // Clear CLR_RX_FIFO_CUR;
        // Clear current frame from Rx FIFO

    SiIRegioModify( REG_CEC_RX_CONTROL, BIT_CLR_RX_FIFO_CUR, BIT_CLR_RX_FIFO_CUR );

#if (INCLUDE_CEC_NAMES > CEC_NO_TEXT)
    if ( !error )
    {
        CpCecPrintCommand( pCpi, false );
    }
#endif
    return( error );
}


//------------------------------------------------------------------------------
// Function:    SI_CpiStatus
// Description: Check CPI registers for a CEC event
//------------------------------------------------------------------------------

BOOL SI_CpiStatus( SI_CpiStatus_t *pStatus )
{
    uint8_t cecStatus[2];

    pStatus->txState    = 0;
    pStatus->cecError   = 0;
    pStatus->rxState    = 0;

    SiIRegioReadBlock( REG_CEC_INT_STATUS_0, cecStatus, 2);

    if ( (cecStatus[0] & 0x7F) || cecStatus[1] )
    {
        DEBUG_PRINT(MSG_STAT,"\nCEC Status: %02X %02X\n", (int) cecStatus[0], (int) cecStatus[1]);

            // Clear interrupts

        if ( cecStatus[1] & BIT_FRAME_RETRANSM_OV )
        {
            //DEBUG_PRINT(MSG_DBG,("\n!TX retry count exceeded! [%02X][%02X]\n",(int) cecStatus[0], (int) cecStatus[1]));

                /* Flush TX, otherwise after clearing the BIT_FRAME_RETRANSM_OV */
                /* interrupt, the TX command will be re-sent.                   */

            SiIRegioModify( REG_CEC_DEBUG_3,BIT_FLUSH_TX_FIFO, BIT_FLUSH_TX_FIFO );
        }

            // Clear set bits that are set

        SiIRegioWriteBlock( REG_CEC_INT_STATUS_0, cecStatus, 2 );

            // RX Processing

        if ( cecStatus[0] & BIT_RX_MSG_RECEIVED )
        {
            pStatus->rxState = 1;   // Flag caller that CEC frames are present in RX FIFO
        }

            // RX Errors processing

        if ( cecStatus[1] & BIT_SHORT_PULSE_DET )
        {
            pStatus->cecError |= SI_CEC_SHORTPULSE;
        }

        if ( cecStatus[1] & BIT_START_IRREGULAR )
        {
            pStatus->cecError |= SI_CEC_BADSTART;
        }

        if ( cecStatus[1] & BIT_RX_FIFO_OVERRUN )
        {
            pStatus->cecError |= SI_CEC_RXOVERFLOW;
        }

            // TX Processing

        if ( cecStatus[0] & BIT_TX_FIFO_EMPTY )
        {
            pStatus->txState = SI_TX_WAITCMD;
        }
        if ( cecStatus[0] & BIT_TX_MESSAGE_SENT )
        {
            pStatus->txState = SI_TX_SENDACKED;
        }
        if ( cecStatus[1] & BIT_FRAME_RETRANSM_OV )
        {
            pStatus->txState = SI_TX_SENDFAILED;
        }
    }

    return( true );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiGetLogicalAddr
// Description: Get Logical Address
//------------------------------------------------------------------------------

uint8_t SI_CpiGetLogicalAddr( void )
{
	return SiIRegioRead( REG_CEC_TX_INIT);
}

//------------------------------------------------------------------------------
// Function:    SI_CpiInit
// Description: Initialize the CPI subsystem for communicating via CEC
//------------------------------------------------------------------------------

BOOL SI_CpiInit( void )
{
    uint8_t capture_address[2];
    uint8_t capture_addr_sel = 0x01;

    // Turn off CEC auto response to <Abort> command.
    //Remove this for pass CEC CTS
    //SiIRegioWrite( CEC_OP_ABORT_31, CLEAR_BITS );

    SiIRegioWrite( REG_CEC_CONFIG_CPI, 0x00 );
	SiIRegioBitToggle( REG_SRST, BIT_CEC_RST );

	// Jin: software workaround For Bug #20040
	// Disable CDC registers
	SiIRegioWrite( 0x8E3, 0x00);
	SiIRegioWrite( 0x8E4, 0x00);

    // initialized he CEC CEC_LOGADDR_TV logical address
#if 0//Keno20120301, disable RK design, because it will effect that cannot display
    if ( !SI_CpiSetLogicalAddr( CEC_LOGADDR_TV ))
    {
       	if ( !SI_CpiSetLogicalAddr( CEC_LOGADDR_SPECIFICUSE ))
	    {
			SI_CpiSetLogicalAddr( CEC_LOGADDR_UNREGORBC );
			g_cecAddress = CEC_LOGADDR_UNREGORBC;
		}
		else
			g_cecAddress = CEC_LOGADDR_SPECIFICUSE;
    }
	else
		g_cecAddress = CEC_LOGADDR_TV;
#endif
		g_cecAddress = CEC_LOGADDR_PLAYBACK1;	// Set CEC_LOGADDR_PLAYBACK1 at initial time

    capture_address[0] = 0;
    capture_address[1] = 0;
    if( g_cecAddress < 8 )
    {
        capture_addr_sel = capture_addr_sel << g_cecAddress;
        capture_address[0] = capture_addr_sel;
    }
    else
    {
        capture_addr_sel   = capture_addr_sel << ( g_cecAddress - 8 );
        capture_address[1] = capture_addr_sel;
    }

        // Set Capture Address

    SiIRegioWriteBlock(REG_CEC_CAPTURE_ID0, capture_address, 2 );
    SiIRegioWrite( REG_CEC_TX_INIT, g_cecAddress );

	DEBUG_PRINT( MSG_ALWAYS, "CEC Logical Address: 0x%02X\n", (int)g_cecAddress );


    return( true );
}
#endif

