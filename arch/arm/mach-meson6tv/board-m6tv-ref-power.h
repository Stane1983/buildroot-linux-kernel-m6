#ifndef __LINUX_AXP_CFG_H_
#define __LINUX_AXP_CFG_H_

#include <mach/irqs.h>

#define AXP18_ADDR          0x2C >> 1
#define AXP19_ADDR          0x68 >> 1
#define AXP20_ADDR          0x68 >> 1
#define AXP18_I2CBUS        1
#define AXP19_I2CBUS        0
#define AXP20_I2CBUS        2
#define BATRDC              150 //initial rdc
#define AXP20_IRQNO     INT_WATCHDOG //0 smp irq number base change to 32


#define LDO1SET             0  //0: LDO1SET connect AGND, 1: LDO1SET connect AIPS, for axp189 LDOSET bonding to AGND
#define DC2SET              1  //0: DC2SET connect GND, 1: DC2SET connect IPSOUT, for axp189 DC2SET bonding to IPSOUT
#define DC3SET              1  //0:DC3SET connect GND, 1:DC3SET connect IPSOUT ,for axp189 DC3SET to pin

#define AXP19LDO1           1250
#define AXP20LDO1             1300


#if !LDO1SET
#define LDO1MIN         1250
#define LDO1MAX         1250
#else
#define LDO1MIN         3300
#define LDO1MAX         3300
#endif

#if DC2SET
#define DCDC2MIN        800
#define DCDC2MAX        1400
#else
#define DCDC2MIN        1400
#define DCDC2MAX        2000
#endif

#if DC3SET
#define DCDC3MIN        2000
#define DCDC3MAX        2700
#define  LDO3MIN        1600
#define  LDO3MAX        1900
#else
#define DCDC3MIN        1300
#define DCDC3MAX        1900
#define  LDO3MIN        2300
#define  LDO3MAX        2600
#endif

#define AXP18_VOL_MAX       50//1200
#define AXP18_TIME_MAX      20//100
#define AXP18_RDC_COUNT     10
#define CHG_RDC_RATE        20//100
#define DISCHARGE_CUR_RATE  10
#define MAX_BAT_CUR         15
#define DISCHARGE_RDC_CAL   53

#define AXP19_VOL_MAX       50
#define AXP19_TIME_MAX      20
#define AXP19_AVER_MAX      10
#define AXP19_RDC_COUNT     10

#define AXP20_VOL_MAX           12 // capability buffer length
#define AXP20_TIME_MAX      20
#define AXP20_AVER_MAX      10
#define AXP20_RDC_COUNT     10

#define ABS(x)              ((x) >0 ? (x) : -(x) )

#define END_VOLTAGE_APS     3350

#define BAT_AVER_VOL        3820    //Aver Vol:3.82V

#define FUELGUAGE_LOW_VOL   3400    //<3.4v,2%
#define FUELGUAGE_VOL1      3500    //<3.5v,3%
#define FUELGUAGE_VOL2      3600
#define FUELGUAGE_VOL3      3700
#define FUELGUAGE_VOL4      3800
#define FUELGUAGE_VOL5      3900
#define FUELGUAGE_VOL6      4000
#define FUELGUAGE_VOL7      4100
#define FUELGUAGE_TOP_VOL   4160    //>4.16v,100%

#define FUELGUAGE_LOW_LEVEL 2       //<3.4v,2%
#define FUELGUAGE_LEVEL1    3       //<3.5v,3%
#define FUELGUAGE_LEVEL2    5
#define FUELGUAGE_LEVEL3    16
#define FUELGUAGE_LEVEL4    46
#define FUELGUAGE_LEVEL5    66
#define FUELGUAGE_LEVEL6    83
#define FUELGUAGE_LEVEL7    93
#define FUELGUAGE_TOP_LEVEL 100     //>4.16v,100%

#define INTLDO4                 2800000                             //initial ldo4 voltage
#define INIT_RDC                150                                     //initial rdc
#define TIMER                   20                                      //axp19 renew capability time
#define BATTERYCAP              3400                            //battery capability
#define RENEW_TIME      10                                      //axp20 renew capability time
#define INTCHGCUR               500000                              //set initial charging current limite
#define SUSCHGCUR               1000000                             //set suspend charging current limite
#define RESCHGCUR               INTCHGCUR                           //set resume charging current limite
#define CLSCHGCUR               SUSCHGCUR                           //set shutdown charging current limite
#define INTCHGVOL               4200000                             //set initial charing target voltage
#define INTCHGENDRATE       10                                      //set initial charing end current   rate
#define INTCHGENABLED       1                                         //set initial charing enabled
#define INTADCFREQ          25                                      //set initial adc frequency
#define INTADCFREQC         100                                     //set initial coulomb adc coufrequency
#define INTCHGPRETIME       50                                      //set initial pre-charging time
#define INTCHGCSTTIME       480                                     //set initial pre-charging time
#define BATMAXVOL               4200000                             //set battery max design volatge
#define BATMINVOL               3500000                             //set battery min design volatge

#define OCVREG0       0    //3.1328
#define OCVREG1       0    //3.2736
#define OCVREG2       0    //3.4144
#define OCVREG3       0    //3.5552
#define OCVREG4       0     //3.6256
#define OCVREG5       1     //3.6608
#define OCVREG6       8     //3.6960
#define OCVREG7       13   //3.7312
#define OCVREG8       22   //3.7664
#define OCVREG9       41   //3.8016
#define OCVREGA       52   //3.8368
#define OCVREGB       57   //3.8720
#define OCVREGC       71   //3.9424
#define OCVREGD       82   //4.0128
#define OCVREGE       90   //4.0832
#define OCVREGF       99   //4.1536


extern struct axp_cfg_info *axp_cfg_board;

#endif
