/*******************************************************************************

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED

--------------------------------------------------------------------------------

  File        :  MHL_Controller.h

  Description :  Head file of MHL_Controller Interface

*******************************************************************************/

#ifndef _MHL_CONTROLLER_H
#define _MHL_CONTROLLER_H


#define MHL_BUS_TIMER_PERIOD 	100 	// 100ms

// The initial function
extern void MHL_Bus_Reset(void);

// Call this function in a loop
extern void MHL_Bus_Task(void);

// Call this function every 10ms
extern void MHL_Bus_Timer(void);


#endif // _MHL_CONTROLLER_H


