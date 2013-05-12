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

#include "../main/si_cp1292.h"
#include "../cec/si_apicec.h"
//#include <si_apiHeac.h>

#if defined(CONFIG_MHL_SII1292_CEC)

//------------------------------------------------------------------------------
// Function:    CecViewOn
// Description: Take the HDMI switch and/or sink device out of standby and
//              enable it to display video.
//------------------------------------------------------------------------------

static void CecViewOn ( SI_CpiData_t *pCpi )
{
    pCpi = pCpi;

    SI_CecSetPowerState( CEC_POWERSTATUS_ON );
}

//------------------------------------------------------------------------------
// Function:    CpCecRxMsgHandler
// Description: Parse received messages and execute response as necessary
//              Only handle the messages needed at the top level to interact
//              with the Port Switch hardware.  The SI_API message handler
//              handles all messages not handled here.
//
// Warning:     This function is referenced by the Silicon Image CEC API library
//              and must be present for it to link properly.  If not used,
//              it should return 0 (false);
//
//              Returns true if message was processed by this handler
//------------------------------------------------------------------------------

BOOL CpCecRxMsgHandler ( SI_CpiData_t *pCpi )
{
    BOOL            processedMsg, isDirectAddressed;

    isDirectAddressed = !((pCpi->srcDestAddr & 0x0F ) == CEC_LOGADDR_UNREGORBC );

    processedMsg = true;
    if ( isDirectAddressed )
    {
        switch ( pCpi->opcode )
        {
            case CECOP_IMAGE_VIEW_ON:       // In our case, respond the same to both these messages
            case CECOP_TEXT_VIEW_ON:
                CecViewOn( pCpi );
                break;

            case CECOP_INACTIVE_SOURCE:
                break;
            default:
                processedMsg = false;
                break;
        }
    }

    /* Respond to broadcast messages.   */

    else
    {
        switch ( pCpi->opcode )
        {
            case CECOP_ACTIVE_SOURCE:
                break;
            default:
                processedMsg = false;
            break;
    }
    }

    return( processedMsg );
}

#endif

