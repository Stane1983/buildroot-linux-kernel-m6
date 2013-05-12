
#ifndef __AML1212_H__
#define __AML1212_H__

#define AML1212_ADDR                    0x35
#define AML1212_SUPPLY_NAME             "aml1212-supplyer"
#define AML1212_IRQ_NAME                "aml1212-irq"
#define AML_PMU_IRQ_NUM                 INT_GPIO_2
#define AML1212_SUPPLY_ID               0
#define AML1212_DRIVER_VERSION          "v0.2"

#define AML1212_OTP_GEN_CONTROL0        0x17

#define AML1212_CHG_CTRL0               0x29
#define AML1212_CHG_CTRL1               0x2A
#define AML1212_CHG_CTRL2               0x2B
#define AML1212_CHG_CTRL3               0x2C
#define AML1212_CHG_CTRL4               0x2D
#define AML1212_CHG_CTRL5               0x2E
#define AML1212_SAR_ADJ                 0x73

#define AML1212_GEN_CNTL0               0x80
#define AML1212_GEN_CNTL1               0x81
#define AML1212_PWR_UP_SW_ENABLE        0x82        // software power up
#define AML1212_PWR_DN_SW_ENABLE        0x84        // software power down
#define AML1212_GEN_STATUS0             0x86
#define AML1212_GEN_STATUS1             0x87
#define AML1212_GEN_STATUS2             0x88
#define AML1212_GEN_STATUS3             0x89
#define AML1212_GEN_STATUS4             0x8A
#define AML1212_WATCH_DOG               0x8F
#define AML1212_PWR_KEY_ADDR            0x90
#define AML1212_SAR_SW_EN_FIELD         0x9A
#define AML1212_SAR_CNTL_REG0           0x9B
#define AML1212_SAR_CNTL_REG2           0x9D
#define AML1212_SAR_CNTL_REG3           0x9E
#define AML1212_SAR_CNTL_REG5           0xA0
#define AML1212_SAR_RD_IBAT_LAST        0xAB        // battery current measure
#define AML1212_SAR_RD_VBAT_ACTIVE      0xAF        // battery voltage measure
#define AML1212_SAR_RD_MANUAL           0xB1        // manual measure
#define AML1212_SAR_RD_IBAT_ACC         0xB5        // IBAT accumulated result, coulomb
#define AML1212_SAR_RD_IBAT_CNT         0xB9        // IBAT measure count
#define AML1212_GPIO_OUTPUT_CTRL        0xC3        // GPIO output control
#define AML1212_GPIO_INPUT_STATUS       0xC4        // GPIO input status
#define AML1212_IRQ_MASK_0              0xC8        // IRQ Mask base address
#define AML1212_IRQ_STATUS_CLR_0        0xCF        // IRQ status base address
#define AML1212_SP_CHARGER_STATUS0      0xDE        // charge status0
#define AML1212_SP_CHARGER_STATUS1      0xDF        // charge status1
#define AML1212_SP_CHARGER_STATUS2      0xE0        // charge status2
#define AML1212_SP_CHARGER_STATUS3      0xE1        // charge status3
#define AML1212_SP_CHARGER_STATUS4      0xE2        // charge status4
#define AML1212_PIN_MUX4                0xF4        // pin mux select 4

#define AML_PMU_DCDC1                   0
#define AML_PMU_DCDC2                   1
#define AML_PMU_DCDC3                   2
#define AML_PMU_BOOST                   3
#define AML_PMU_LDO1                    4
#define AML_PMU_LDO2                    5
#define AML_PMU_LDO3                    6
#define AML_PMU_LDO4                    7
#define AML_PMU_LDO5                    8

/*
 * global variables
 */
extern uint32_t charge_timeout;

/*
 * function declaration here
 */
int aml_pmu_write  (uint32_t add, uint32_t val);                        // single byte write
int aml_pmu_write16(uint32_t add, uint32_t val);                        // 16 bits register write
int aml_pmu_writes (uint32_t add, uint32_t len, uint8_t *buff);         // block write
int aml_pmu_read   (uint32_t add, uint8_t  *val);
int aml_pmu_read16 (uint32_t add, uint16_t *val);
int aml_pmu_reads  (uint32_t add, uint32_t len, uint8_t *buff);
int aml_pmu_set_bits(uint32_t addr, uint8_t bits, uint8_t mask);        // set bis in mask

int aml_pmu_set_dcin(int enable);                                       // enable / disable dcin power path
int aml_pmu_set_gpio(int pin, int val);                                 // set gpio value
int aml_pmu_get_gpio(int pin, uint8_t *val);                            // get gpio value

int aml_pmu_get_voltage(void);                                          // return battery voltage
int aml_pmu_get_current(void);                                          // return battery current
int aml_pmu_get_battery_percent(void);                                  // return battery capacity now, -1 means error
int aml_pmu_get_ibat_cnt(void);                                         // return ibat acc count
int aml_pmu_get_coulomb_acc(void);                                      // return coulomb register value

// bc_mode, indicate usb is conneted to PC or adatper, please see <mach/usbclock.h>
int aml_pmu_set_usb_current_limit(int curr, int bc_mode);               // set usb current limit, in mA
int aml_pmu_set_usb_voltage_limit(int voltage);                         // set usb voltage limit, in mV
int aml_pmu_set_charge_current(int chg_cur);                            // set charge current, in uA
int aml_pmu_set_charge_voltage(int voltage);                            // set charge target voltage, in uA
int aml_pmu_set_charge_end_rate(int rate);                              // set charge end rate, 10% or 20%
int aml_pmu_set_adc_freq(int freq);                                     // SAR ADC auto-sample frequent, for coulomb
int aml_pmu_set_precharge_time(int minute);                             // set pre-charge time when battery voltage is very low
int aml_pmu_set_fastcharge_time(int minute);                            // set fast charge time when in CC period
int aml_pmu_set_charge_enable(int en);                                  // enable or disable charge 

void aml_pmu_set_voltage(int dcdc, int voltage);                        // set dcdc voltage, in mV
void aml_pmu_poweroff(void);                                            // power off PMU

int  aml_cal_ocv(int ibat, int vbat, int dir);                           // calculate ocv according ibat and vbat
void aml_pmu_clear_coulomb(void);                                       // clear coulomb register

#endif  /* __AML1212_H__ */

