#ifndef IRRECEIVER_H
#define IRRECEIVER_H
#include <plat/remote.h>
/***********************************************************************
 * System timer
 **********************************************************************/
#define TIMER_E_INPUT_BIT         8
#define TIMER_D_INPUT_BIT         6
#define TIMER_C_INPUT_BIT         4
#define TIMER_B_INPUT_BIT         2
#define TIMER_A_INPUT_BIT         0
#define TIMER_E_INPUT_MASK       (7UL << TIMER_E_INPUT_BIT)
#define TIMER_D_INPUT_MASK       (3UL << TIMER_D_INPUT_BIT)
#define TIMER_C_INPUT_MASK       (3UL << TIMER_C_INPUT_BIT)
#define TIMER_B_INPUT_MASK       (3UL << TIMER_B_INPUT_BIT)
#define TIMER_A_INPUT_MASK       (3UL << TIMER_A_INPUT_BIT)
#define TIMER_UNIT_1us            0
#define TIMER_UNIT_10us           1
#define TIMER_UNIT_100us          2
#define TIMER_UNIT_1ms            3
#define TIMERE_UNIT_SYS           0
#define TIMERE_UNIT_1us           1
#define TIMERE_UNIT_10us          2
#define TIMERE_UNIT_100us         3
#define TIMERE_UNIT_1ms           4
#define TIMER_A_ENABLE_BIT        16
#define TIMER_E_ENABLE_BIT        20
#define TIMER_A_PERIODIC_BIT      12
/********** Clock Source Device, Timer-E *********/
#define MAX_PLUSE 1024
#define AM_IR_DEC_LDR_ACTIVE 0x0
#define AM_IR_DEC_LDR_IDLE 0x4
#define AM_IR_DEC_LDR_REPEAT 0x8
#define AM_IR_DEC_BIT_0     0xc 
#define AM_IR_DEC_REG0 0x10 
#define AM_IR_DEC_FRAME 0x14
#define AM_IR_DEC_STATUS 0x18
#define AM_IR_DEC_REG1 0x1c
unsigned int ir_g_remote_base = P_AO_IR_DEC_LDR_ACTIVE;
#define am_remote_write_reg(x,val) aml_write_reg32(ir_g_remote_base +x ,val)

#define am_remote_read_reg(x) aml_read_reg32(ir_g_remote_base +x)

#define am_remote_set_mask(x,val) aml_set_reg32_mask(ir_g_remote_base +x,val)

#define am_remote_clear_mask(x,val) aml_clr_reg32_mask(ir_g_remote_base +x,val)
struct ir_window {
    unsigned int winNum;
    unsigned int winArray[MAX_PLUSE];
};

#define IRRECEIVER_IOC_SEND     0x5500
#define IRRECEIVER_IOC_RECV     0x5501
#define IRRECEIVER_IOC_STUDY_S  0x5502
#define IRRECEIVER_IOC_STUDY_E  0x5503


#endif //IRRECEIVER_H

