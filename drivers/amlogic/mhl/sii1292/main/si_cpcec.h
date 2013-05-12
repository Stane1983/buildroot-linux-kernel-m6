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


#ifndef _CECHANDLER_H_
#define _CECHANDLER_H_

#include "../api/si_datatypes.h"

//------------------------------------------------------------------------------
// API Function Templates
//------------------------------------------------------------------------------

BOOL CpArcEnable( uint8_t mode );
void CpHecEnable( BOOL enable );

char *CpCecTranslateLA( uint8_t bLogAddr );
char *CpCecTranslateOpcodeName( SI_CpiData_t *pMsg );
BOOL CpCecPrintCommand( SI_CpiData_t *pMsg, BOOL isTX );

BOOL CpCecRxMsgHandler( SI_CpiData_t *pCpi );

#endif // _CECHANDLER_H_




