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


#ifndef __HAL_H__
#define __HAL_H__

#include <linux/jiffies.h>

#include "../api/si_datatypes.h"


#define API_DEBUG_CODE 1
//#define TIME_STAMP
//-------------------------------------------------------------------------------
//  HAL Macros
//-------------------------------------------------------------------------------
extern uint32_t g_pass;

//choose one, cannot both defined to 1.
#define BOARD_CP1292//using CP1292 board
//#define BOARD_JULIPORT//using RK1292 board

#ifdef BOARD_CP1292
//#define HWI2C//NOTE: please be sure that R27 and R29 is weld in CP1292 start kit
#endif

#if 0
#if (API_DEBUG_CODE==1)
    extern  uint8_t     g_halMsgLevel;

    #ifndef DEBUG_PRINT
	#ifdef TIME_STAMP
    	#define DEBUG_PRINT(l,x)        if (l<=g_halMsgLevel) {printf("[%d]: ", (int)g_pass);  printf x;}
	#else
        #define DEBUG_PRINT(l,x)        if (l<=g_halMsgLevel) printf x
	#endif
    #endif
    #define _DEBUG_(x)              x

#else
    #ifndef DEBUG_PRINT
        #define DEBUG_PRINT(l,x)
    #endif
    #define _DEBUG_(x)
#endif
#endif

#if (API_DEBUG_CODE==1)
        #define DEBUG_PRINT(l, m, a...) printk(l "[MHL] %s" m, __func__, ##a)
#else
        #define DEBUG_PRINT(l, m, a...)
#endif


//-------------------------------------------------------------------------------
//  HAL Constants
//-------------------------------------------------------------------------------

#define I2C_BUS0        0
#define I2C_BUS1        1
#define I2C_BUS2        2
#define I2C_BUS3        3

    /* Rotary switch values.    */

#define RSW_POS_0           0x00
#define RSW_POS_1           0x01
#define RSW_POS_2           0x02
#define RSW_POS_3           0x03
#define RSW_POS_4           0x04
#define RSW_POS_5           0x05
#define RSW_POS_6           0x06
#define RSW_POS_7           0x07
#define RSW_NO_CHANGE       0xFF    // Rotary switch has not changed

//-------------------------------------------------------------------------------
//  Required before use of some HAL modules
//-------------------------------------------------------------------------------

bool_t    HalInitialize( void );
uint8_t HalVersion( bool_t wantMajor );
uint8_t HalVersionFPGA( bool_t wantMajor );

//-------------------------------------------------------------------------------
//  I2C Master BUS Interface
//-------------------------------------------------------------------------------

uint8_t HalI2cBus0ReadByte( uint8_t device_id, uint8_t addr );
void HalI2cBus0WriteByte( uint8_t deviceID, uint8_t offset, uint8_t value );
bool_t HalI2cBus0ReadBlock( uint8_t deviceID, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2cBus0WriteBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2cBus016ReadBlock( uint8_t device_id, uint16_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2cBus016WriteBlock( uint8_t device_id, uint16_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2CBus0WaitForAck( uint8_t device_id );
uint8_t HalI2cBus0GetStatus( void );

uint8_t HalI2cBus1ReadByte( uint8_t device_id, uint8_t addr );
void HalI2cBus1WriteByte( uint8_t deviceID, uint8_t offset, uint8_t value );
bool_t HalI2cBus1ReadBlock( uint8_t deviceID, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2cBus1WriteBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
uint8_t HalI2cBus1GetStatus( void );

uint8_t HalI2cBus2ReadByte( uint8_t device_id, uint8_t addr );
void HalI2cBus2WriteByte( uint8_t deviceID, uint8_t offset, uint8_t value );
bool_t HalI2cBus2ReadBlock( uint8_t deviceID, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2cBus2WriteBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
uint8_t HalI2cBus2GetStatus( void );

uint8_t HalI2cBus3ReadByte( uint8_t device_id, uint8_t addr );
void HalI2cBus3WriteByte( uint8_t deviceID, uint8_t offset, uint8_t value );
bool_t HalI2cBus3ReadBlock( uint8_t deviceID, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2cBus3WriteBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
uint8_t HalI2cBus3GetStatus( void );

extern void twi_init(void);
extern unsigned char twi_DDC_read( unsigned char seg, unsigned char slave_addr, unsigned char reg_addr, unsigned short len, unsigned char *buf );
extern unsigned char twi_read( unsigned char slave_addr, unsigned char reg_addr, unsigned short len, unsigned char *buf );
extern unsigned char twi_write( unsigned char slave_addr, unsigned char reg_addr, unsigned short len, unsigned char *buf );
extern unsigned char twi_read_byte( unsigned char slave_addr, unsigned char reg_addr );
extern unsigned char twi_write_byte( unsigned char slave_addr, unsigned char reg_addr, unsigned char byte );
extern unsigned char twi_read_block( unsigned char slave_addr, unsigned char reg_addr, unsigned char *buf, unsigned short len );
extern unsigned char twi_write_block( unsigned char slave_addr, unsigned char reg_addr, unsigned char *buf, unsigned short len );
extern unsigned char twi_16read( unsigned char slave_addr, unsigned short reg_addr, unsigned short len, unsigned char *buf );
extern unsigned char twi_16write( unsigned char slave_addr, unsigned short reg_addr, unsigned short len, unsigned char *buf );
extern unsigned char twi_wait_ack( unsigned char slave_addr, unsigned char timeoutMs );
//-------------------------------------------------------------------------------
//  I2C Local Bus
//  Macros to select the I2C bus that is used by this board's local I2C bus.
//-------------------------------------------------------------------------------

#ifdef HWI2C
#define HalI2cReadByte( device_id, addr )                       twi_read_byte( device_id, addr )
#define HalI2cWriteByte( device_id, addr, value )               twi_write_byte( device_id, addr, value )
#define HalI2cReadBlock( device_id, addr, p_data, nbytes )      (0 == twi_read_block( device_id, addr, p_data, nbytes ))
#define HalI2cWriteBlock( device_id, addr, p_data, nbytes )     (0 == twi_write_block( device_id, addr, p_data, nbytes ))
#define HalI2c16ReadBlock( device_id, addr, p_data, nbytes )    (0 == twi_16read( device_id, addr, nbytes, p_data ))
#define HalI2c16WriteBlock( device_id, addr, p_data, nbytes )   (0 == twi_16write( device_id, addr, nbytes, p_data ))
#define HalI2CWaitForAck( device_id)                (0 == twi_wait_ack( device_id, 1 ))
#else
#define HalI2cReadByte( device_id, addr )                       HalI2cBus0ReadByte( device_id, addr )
#define HalI2cWriteByte( device_id, addr, value )               HalI2cBus0WriteByte( device_id, addr, value )
#define HalI2cReadBlock( device_id, addr, p_data, nbytes )      HalI2cBus0ReadBlock( device_id, addr, p_data, nbytes )
#define HalI2cWriteBlock( device_id, addr, p_data, nbytes )     HalI2cBus0WriteBlock( device_id, addr, p_data, nbytes )
#define HalI2c16ReadBlock( device_id, addr, p_data, nbytes )    HalI2cBus016ReadBlock( device_id, addr, p_data, nbytes )
#define HalI2c16WriteBlock( device_id, addr, p_data, nbytes )   HalI2cBus016WriteBlock( device_id, addr, p_data, nbytes )
#define HalI2CWaitForAck( device_id )                           HalI2CBus0WaitForAck( device_id )
#define HalI2cGetStatus()                                       HalI2cBus0GetStatus()

#endif

//-------------------------------------------------------------------------------
//  I2C Slave Interface
//
//  Implemented as a set of registers that a master device can read and write
//  as if it was a device
//-------------------------------------------------------------------------------

#define HAL_SLAVE_I2C_ADDR 		0x82    // Address of device as it appears to external master

#define HAL_SLAVE_REG_COUNT     4       // Total number of slave registers
#define HAL_SLAVE_REG_ACTIVE    0       // Number of slave registers that cause an indirect call into application code

void HalSlaveI2CInit( void );
uint8_t HalSlaveI2CRead( uint8_t regOffset );
void HalSlaveI2CWrite( uint8_t regOffset, uint8_t value );

//-------------------------------------------------------------------------------
//  UART functions
//-------------------------------------------------------------------------------

#if 0
#define MSG_ALWAYS              0x00
#define MSG_STAT                0x01
#define MSG_DBG                 0x02
#endif
#define MSG_PRINT_ALL           0xFF

//extern  uint8_t     g_halMsgLevel;

void    HalUartInit( void );
//uint8_t putchar( uint8_t c );
//uint8_t _getkey( void );

//-------------------------------------------------------------------------------
//  REMOTE functions
//-------------------------------------------------------------------------------

bool_t HAL_RemoteRequestHandler( void );

//-------------------------------------------------------------------------------
//  Timer Functions
//-------------------------------------------------------------------------------

#define ELAPSED_TIMER               0xFF
#define ELAPSED_TIMER1              0xFE
#define TIMER_0                     0   // DO NOT USE - reserved for TimerWait()
#define TIMER_POLLING               1   // Reserved for main polling loop
#define TIMER_2                     2   // Available
#define TIMER_3                     3   // Available

void    HalTimerInit( void );
void    HalTimerSet( uint8_t timer, uint16_t m_sec );
uint8_t HalTimerExpired( uint8_t timer );
void    HalTimerWait( uint16_t m_sec );
uint16_t HalTimerElapsed( uint8_t index );
bool_t HalTimerDelay(uint32_t baseTime, uint32_t delay);

//-------------------------------------------------------------------------------
//  Extended GPIO Functions
//-------------------------------------------------------------------------------

#define DEF_EX_DIRECTION    0x00    // All GPIOs output
#define DEF_EX_POLARITY     0x00    // All GPIO outputs not inverted
#define DEF_EX_DEFAULT_VAL  0xE0    // Default output state

#define EX_GPIO_DEVICE_ID   0x38
#define EX_GPIO_INPUT       0
#define EX_GPIO_OUTPUT      1
#define EX_GPIO_POLARITY    2
#define EX_GPIO_CONFIG      3

void    HalGpioInit( void );
uint8_t HalGpioExRead( void );
void    HalGpioExWrite( uint8_t newValue );

uint8_t HalGpioReadRotarySwitch ( uint8_t i_want_it_now );

bool_t HalGpioReadWakeUpPin(void);


//-------------------------------------------------------------------------------
//  Internal EEPROM functions
//-------------------------------------------------------------------------------

uint8_t HalEEintReadByte( uint16_t addr );
void    HalEEintWriteByte( uint16_t addr, uint8_t value );
bool_t    HalEEintReadBlock( uint16_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t    HalEEintWriteBlock( uint16_t addr, uint8_t *p_data, uint16_t nbytes );

//-------------------------------------------------------------------------------
//  External EEPROM functions
//-------------------------------------------------------------------------------

uint8_t HalEEextReadByte( uint16_t addr );
void    HalEEextWriteByte( uint16_t addr, uint8_t value );
bool_t    HalEEextReadBlock( uint16_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t    HalEEextWriteBlock( uint16_t addr, uint8_t *p_data, uint16_t nbytes );

//-------------------------------------------------------------------------------
//  IR Interface functions
//-------------------------------------------------------------------------------

enum
{
    NO_EVENT,  //!< nothing happened
    KEY_DOWN,  //!< remote control key pressed down
    KEY_UP,    //!< remote control key released
};

typedef struct
{
    uint8_t eventType;  //!< UI event type
    uint8_t command;    //!< UI command
} IrCommand_t;

typedef struct
{
    uint8_t rc5Command;         //!< rc protocol command code
    uint8_t uiCommand;          //!< UI command code
} UiRc5Conversion_t;

typedef struct
{
    uint8_t rc5address;
    uint8_t uiRc5convTableLength;
    const UiRc5Conversion_t* uiRc5convTable;

} UiRC5CommandMap_t;

// user will provide this for the lib( no need to change lib when customizing applications ).

extern const UiRC5CommandMap_t irCommandMap;

IrCommand_t HalGetIRCommand( void );
void HalIRInit( void );
bool_t HalIRReturnRC5( bool_t returnRC5 );


#endif  // _HAL_H__

