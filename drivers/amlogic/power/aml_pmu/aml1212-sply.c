/*
 * Battery charger driver for Amlogic PMU AML1212
 *
 * Copyright (C) 2012 Amlogic Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/utsname.h>

#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <mach/am_regs.h>
#include <mach/gpio.h>
#include <amlogic/aml_pmu.h>
#include <amlogic/aml1212.h>
#include <amlogic/battery_parameter.h>
#include <amlogic/aml_rtc.h>
#include <mach/irqs.h> 
#include <mach/usbclock.h>
#include <amlogic/aml1212_algorithm.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend aml_pmu_early_suspend;
int early_suspend_flag = 0;
#endif

#define MAX_BUF         100
#define CHECK_DRIVER()      \
    if (!gcharger) {        \
        AML_PMU_DBG("driver is not ready right now, wait...\n");   \
        dump_stack();       \
        return -ENODEV;     \
    }

#define CHECK_REGISTER_TEST     1
#if CHECK_REGISTER_TEST
int register_wrong_flag = 0;
#endif


static struct battery_parameter *aml_pmu_battery   = NULL;
static struct input_dev         *aml_pmu_power_key = NULL;

uint32_t charge_timeout = 0;
int      re_charge_cnt  = 0;
int      current_dir    = -1;
int      power_flag     = 0;
int      pmu_version    = 0;

struct aml_pmu_charger *gcharger  = NULL;
EXPORT_SYMBOL_GPL(gcharger);

int aml_pmu_write(uint32_t add, uint32_t val)
{
    int ret;
    uint8_t buf[3] = {};
    struct i2c_client *pdev;

    CHECK_DRIVER();
    pdev = to_i2c_client(gcharger->master);

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    buf[2] = val & 0xff;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1212_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        }
    };
    ret = i2c_transfer(pdev->adapter, msg, 1);
    if (ret < 0) {
        AML_PMU_DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_write);

int aml_pmu_write16(uint32_t add, uint32_t val)
{
    int ret;
    uint8_t buf[4] = {};
    struct i2c_client *pdev;

    CHECK_DRIVER();
    pdev = to_i2c_client(gcharger->master);

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    buf[2] = val & 0xff;
    buf[3] = (val >> 8) & 0xff;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1212_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        }
    };
    ret = i2c_transfer(pdev->adapter, msg, 1);
    if (ret < 0) {
        AML_PMU_DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_write16);

int aml_pmu_writes(uint32_t add, uint32_t len, uint8_t *buff)
{
    int ret;
    uint8_t buf[MAX_BUF] = {};
    struct i2c_client *pdev;

    CHECK_DRIVER();
    pdev = to_i2c_client(gcharger->master);

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    memcpy(buf + 2, buff, len > MAX_BUF ? MAX_BUF : len);
    struct i2c_msg msg[] = {
        {
            .addr  = AML1212_ADDR,
            .flags = 0,
            .len   = len + 2,
            .buf   = buf,
        }
    };
    ret = i2c_transfer(pdev->adapter, msg, 1);
    if (ret < 0) {
        AML_PMU_DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_writes);

int aml_pmu_read(uint32_t add, uint8_t *val)
{
    int ret;
    uint8_t buf[2] = {};
    struct i2c_client *pdev;

    CHECK_DRIVER();
    pdev = to_i2c_client(gcharger->master);

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1212_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        },
        {
            .addr  = AML1212_ADDR,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = val,
        }
    };
    ret = i2c_transfer(pdev->adapter, msg, 2);
    if (ret < 0) {
        AML_PMU_DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_read);

int aml_pmu_read16(uint32_t add, uint16_t *val)
{
    int ret;
    uint8_t buf[2] = {};
    struct i2c_client *pdev;

    CHECK_DRIVER();
    pdev = to_i2c_client(gcharger->master);

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1212_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        },
        {
            .addr  = AML1212_ADDR,
            .flags = I2C_M_RD,
            .len   = 2, 
            .buf   = val,
        }
    };
    ret = i2c_transfer(pdev->adapter, msg, 2);
    if (ret < 0) {
        AML_PMU_DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_read16);

int aml_pmu_reads(uint32_t add, uint32_t len, uint8_t *buff)
{
    int ret;
    uint8_t buf[2] = {};
    struct i2c_client *pdev;

    CHECK_DRIVER();
    pdev = to_i2c_client(gcharger->master);

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1212_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        },
        {
            .addr  = AML1212_ADDR,
            .flags = I2C_M_RD,
            .len   = len,
            .buf   = buff,
        }
    };
    ret = i2c_transfer(pdev->adapter, msg, 2);
    if (ret < 0) {
        AML_PMU_DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_reads);

int aml_pmu_set_bits(uint32_t addr, uint8_t bits, uint8_t mask)
{
    uint8_t val; 
    int ret; 
 
    ret = aml_pmu_read(addr, &val); 
    if (ret) { 
        return ret; 
    } 
    val &= ~(mask); 
    val |=  (bits & mask); 
    return aml_pmu_write(addr, val); 
} 
EXPORT_SYMBOL_GPL(aml_pmu_set_bits); 

int aml_pmu_set_dcin(int enable)
{
    uint8_t val;

    aml_pmu_read(AML1212_CHG_CTRL4, &val);
    if (enable) {
        val &= ~0x10;    
    } else {
        val |= 0x10;    
    }    
    return aml_pmu_write(AML1212_CHG_CTRL4, val);
}
EXPORT_SYMBOL_GPL(aml_pmu_set_dcin);

int aml_pmu_set_gpio(int pin, int val)
{
    int ret;
    uint32_t data;

    if (pin <= 0 || pin > 4 || val > 1 || val < 0) {
        AML_PMU_DBG("ERROR, invalid input value, pin = %d, val= %d\n", pin, val);
        return -EINVAL;
    }
    data = (1 << (pin + 11));
    if (val) {
        aml_pmu_write16(AML1212_PWR_DN_SW_ENABLE,  data);    
    } else {
        aml_pmu_write16(AML1212_PWR_UP_SW_ENABLE, data);    
    }
}
EXPORT_SYMBOL_GPL(aml_pmu_set_gpio);

int aml_pmu_get_gpio(int pin, uint8_t *val)
{
    int ret;
    uint8_t data;

    if (pin <= 0 || pin > 4 || !val) { 
        AML_PMU_DBG("ERROR, invalid input value, pin = %d, val= %d\n", pin, val);
        return -EINVAL;
    }
    ret = aml_pmu_read(AML1212_GPIO_INPUT_STATUS, &data);
    if (ret) {                                                  // read failed
        return ret;    
    }
    if (data & (1 << (pin - 1))) {
        *val = 1;    
    } else {
        *val = 0;    
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_get_gpio);

int aml_pmu_get_voltage(void)
{
    uint8_t buf[2] = {};
    int     result = 0;
    int     tmp, i;

    for (i = 0; i < 4; i++) {
        aml_pmu_write(AML1212_SAR_SW_EN_FIELD, 0x04);
        udelay(10);
        aml_pmu_reads(AML1212_SAR_RD_VBAT_ACTIVE, 2, buf);
        tmp = (((buf[1] & 0x0f) << 8) + buf[0]);
        if (pmu_version == 0x02 || pmu_version == 0x01) {       // VERSION A & B
            tmp = (tmp * 7200) / 2048; 
        } else if (pmu_version == 0x03) {                       // VERSION D
            tmp = (tmp * 4800) / 2048;
        } else {
            tmp = 0;    
        }
        result += tmp;
    }
    return result / 4;
}
EXPORT_SYMBOL_GPL(aml_pmu_get_voltage);

int aml_pmu_get_current(void)
{
    uint8_t  buf[2] = {};
    uint32_t tmp, i;
    int      result = 0;
    int      sign_bit;

    for (i = 0; i < 4; i++) {
        aml_pmu_write(AML1212_SAR_SW_EN_FIELD, 0x40);
        udelay(10);
        aml_pmu_reads(AML1212_SAR_RD_IBAT_LAST, 2, buf);
        tmp = ((buf[1] & 0x0f) << 8) + buf[0];
        sign_bit = tmp & 0x800;
        if (tmp & 0x800) {                                              // complement code
            tmp = (tmp ^ 0xfff) + 1;
        }
        result += (tmp * 4000) / 2048;                                  // LSB of IBAT ADC is 1.95mA
    }
    result /= 4;
    return result;
}
EXPORT_SYMBOL_GPL(aml_pmu_get_current);

int aml_pmu_get_coulomb_acc(void)
{
    uint8_t buf[4] = {};
    int result;
    int coulomb;

    aml_pmu_write(AML1212_SAR_SW_EN_FIELD, 0x40);
    aml_pmu_reads(AML1212_SAR_RD_IBAT_ACC, 4, buf);

    result  = (buf[0] <<  0) |
              (buf[1] <<  8) |
              (buf[2] << 16) |
              (buf[3] << 24);
    coulomb = (result) / (3600 * 100);                              // convert to mAh
    coulomb = (coulomb * 4000) / 2048;                              // LSB of current is 1.95mA
    return coulomb;
}
EXPORT_SYMBOL_GPL(aml_pmu_get_coulomb_acc);

int aml_pmu_get_ibat_cnt(void)
{
    uint8_t buf[4] = {};
    uint32_t cnt = 0;
    
    aml_pmu_write(AML1212_SAR_SW_EN_FIELD, 0x40);
    aml_pmu_reads(AML1212_SAR_RD_IBAT_CNT, 4, buf);

    cnt = (buf[0] <<  0) | 
          (buf[1] <<  8) |
          (buf[2] << 16) |
          (buf[3] << 24);
    return cnt;
}

int aml_pmu_manual_measure_current()
{
    uint8_t buf[2] = {};
    int result;
    int tmp;

    aml_pmu_write(0xA2, 0x09);                                      // MSR_SEL = 3
    aml_pmu_write(0xA9, 0x0f);                                      // select proper ADC channel
    aml_pmu_write(0xAA, 0xe0);
    udelay(20);
    aml_pmu_write(AML1212_SAR_SW_EN_FIELD, 0x08);
    udelay(20);
    aml_pmu_reads(AML1212_SAR_RD_MANUAL, 2, buf);
    tmp = ((buf[1] & 0x0f)<< 8) | buf[0];
    if (tmp & 0x800) {                                              // complement code
        tmp = (tmp ^ 0xfff) + 1;
    }
    result = tmp * 4000 / 2048;
    return result;
}

static unsigned int dcdc1_voltage_table[] = {                  // voltage table of DCDC1
    2000, 1980, 1960, 1940, 1920, 1900, 1880, 1860, 
    1840, 1820, 1800, 1780, 1760, 1740, 1720, 1700, 
    1680, 1660, 1640, 1620, 1600, 1580, 1560, 1540, 
    1520, 1500, 1480, 1460, 1440, 1420, 1400, 1380, 
    1360, 1340, 1320, 1300, 1280, 1260, 1240, 1220, 
    1200, 1180, 1160, 1140, 1120, 1100, 1080, 1060, 
    1040, 1020, 1000,  980,  960,  940,  920,  900,  
     880,  860,  840,  820,  800,  780,  760,  740
};

static unsigned int dcdc2_voltage_table[] = {                  // voltage table of DCDC2
    2160, 2140, 2120, 2100, 2080, 2060, 2040, 2020,
    2000, 1980, 1960, 1940, 1920, 1900, 1880, 1860, 
    1840, 1820, 1800, 1780, 1760, 1740, 1720, 1700, 
    1680, 1660, 1640, 1620, 1600, 1580, 1560, 1540, 
    1520, 1500, 1480, 1460, 1440, 1420, 1400, 1380, 
    1360, 1340, 1320, 1300, 1280, 1260, 1240, 1220, 
    1200, 1180, 1160, 1140, 1120, 1100, 1080, 1060, 
    1040, 1020, 1000,  980,  960,  940,  920,  900
};

int find_idx_by_voltage(int voltage, unsigned int *table)
{
    int i;

    /*
     * under this section divide(/ or %) can not be used, may cause exception
     */
    for (i = 0; i < 64; i++) {
        if (voltage >= table[i]) {
            break;    
        }
    }
    if (voltage == table[i]) {
        return i;    
    }
    return i - 1;
}

void aml_pmu_set_voltage(int dcdc, int voltage)
{
    int idx_to = 0xff;
    int idx_cur;
    unsigned char val;
    unsigned char addr;
    unsigned int *table;
    
    if (dcdc < 0 || dcdc > AML_PMU_DCDC2 || voltage > 2100 || voltage < 840) {
        return ;                                                // current only support DCDC1&2 voltage adjust
    }
    if (dcdc == AML_PMU_DCDC1) {
        addr  = 0x2f; 
        table = dcdc1_voltage_table; 
    } else if (dcdc = AML_PMU_DCDC2) {
        addr  = 0x38;    
        table = dcdc2_voltage_table; 
    }
    aml_pmu_read(addr, &val);
    idx_cur = ((val & 0xfc) >> 2);
    idx_to = find_idx_by_voltage(voltage, table);
    AML_PMU_DBG("set idx from %x to %x\n", idx_cur, idx_to);
    while (idx_cur != idx_to) {
        if (idx_cur < idx_to) {                                 // adjust to target voltage step by step
            idx_cur++;    
        } else {
            idx_cur--;
        }
        val &= ~0xfc;
        val |= (idx_cur << 2);
        aml_pmu_write(addr, val);
        udelay(100);                                            // atleast delay 100uS
    }
}
EXPORT_SYMBOL_GPL(aml_pmu_set_voltage);

void aml_pmu_poweroff(void)
{
    uint8_t buf = (1 << 5);                                     // software goto OFF state

    aml_pmu_write(0x0019, 0x10);                                // according Harry, cut usb output
    aml_pmu_write16(0x0084, 0x0001);
  //aml_pmu_write(AML1212_PIN_MUX4, 0x04);                      // according David Wang, for charge cannot stop
    aml_pmu_write(0x0078, 0x04);                                // close LDO6 before power off
    aml_pmu_set_charge_enable(0);                               // close charger before power off
    if (pmu_version == 0x03) {
        aml_pmu_set_bits(0x004a, 0x00, 0x08);                   // close clock of charger for REVD
    }
    AML_PMU_DBG("software goto OFF state\n");
    mdelay(10);
    aml_pmu_write(AML1212_GEN_CNTL1, buf);    
    AML_PMU_DBG("power off PMU failed\n");
    while (1) {
            
    }
}
EXPORT_SYMBOL_GPL(aml_pmu_poweroff);

static void aml_pmu_charger_update_state(struct aml_pmu_charger *charger);

static void aml_pmu_charger_update(struct aml_pmu_charger *charger);

static enum power_supply_property aml_pmu_battery_props[] = {
    POWER_SUPPLY_PROP_MODEL_NAME,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_TECHNOLOGY,
    POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
    POWER_SUPPLY_PROP_CAPACITY,
    POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property aml_pmu_ac_props[] = {
    POWER_SUPPLY_PROP_MODEL_NAME,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
};

static enum power_supply_property aml_pmu_usb_props[] = {
    POWER_SUPPLY_PROP_MODEL_NAME,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
};

static void aml_pmu_battery_check_status(struct aml_pmu_charger      *charger,
                                         union  power_supply_propval *val)
{
    uint8_t value;

    if (charger->bat_det) {
        if (charger->ext_valid){
            if( charger->rest_vol == 100) {
                val->intval = POWER_SUPPLY_STATUS_FULL;
            } else if (charger->rest_vol == 0 && !current_dir) {        // protect for over-discharging
                val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
            } else {
                val->intval = POWER_SUPPLY_STATUS_CHARGING;
            }
        } else {
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
        }
    } else {
        val->intval = POWER_SUPPLY_STATUS_FULL;
    }
}

static void aml_pmu_battery_check_health(struct aml_pmu_charger      *charger,
                                         union  power_supply_propval *val)
{
    // TODO: need implement
#if 0
    if (charger->fault & ) {
    } else if (charger->fault & ) {
    } else if (charger->fault & ) {
    } else {
        val->intval = POWER_SUPPLY_HEALTH_GOOD;
    }
#else
    val->intval = POWER_SUPPLY_HEALTH_GOOD;
#endif
}

static int aml_pmu_battery_get_property(struct power_supply *psy,
                                        enum   power_supply_property psp,
                                        union  power_supply_propval *val)
{
    struct aml_pmu_charger *charger;
    int ret = 0;
    charger = container_of(psy, struct aml_pmu_charger, batt);
    
    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        aml_pmu_battery_check_status(charger, val);
        break;

    case POWER_SUPPLY_PROP_HEALTH:
        aml_pmu_battery_check_health(charger, val);
        break;

    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = charger->battery_info->technology;
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
        val->intval = charger->battery_info->voltage_max_design;
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
        val->intval = charger->battery_info->voltage_min_design;
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = charger->vbat * 1000; 
        break;

    case POWER_SUPPLY_PROP_CURRENT_NOW:             // charging : +, discharging -;
        if (ABS(charger->ibat) > 20 && !charge_timeout && charger->bat_current_direction != 3) {
            val->intval = charger->ibat * 1000 * (charger->bat_current_direction == 1 ? 1 : -1);
        } else {
            val->intval = 0;                        // when charge time out, report 0
        }
        break;

    case POWER_SUPPLY_PROP_MODEL_NAME:
        val->strval = charger->batt.name;
        break;

    case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
        val->intval = charger->battery_info->energy_full_design;
        break;

    case POWER_SUPPLY_PROP_CAPACITY:
        val->intval = charger->rest_vol;
        break;

    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = charger->bat_det; 
        break;

    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = charger->bat_det;
        break;
    case POWER_SUPPLY_PROP_TEMP:
        val->intval =  300;                         // fixed to 300k, need implement dynamic values
        break;
    default:
        ret = -EINVAL;
        break;
    }
    
    return ret;
}

static int aml_pmu_ac_get_property(struct power_supply *psy,
                                   enum   power_supply_property psp,
                                   union  power_supply_propval *val)
{
    struct aml_pmu_charger *charger;
    int ret = 0;
    charger = container_of(psy, struct aml_pmu_charger, ac);

    switch(psp){
    case POWER_SUPPLY_PROP_MODEL_NAME:
        val->strval = charger->ac.name;
        break;

    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = charger->ac_det;
        break;

    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = charger->ac_det;      // charger->ac_valid; this need implement
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = 5000 * 1000;          // charger->vac * 1000;
        break;

    case POWER_SUPPLY_PROP_CURRENT_NOW:
        val->intval = 1000 * 1000;          // charger->iac * 1000;
        break;

    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int aml_pmu_usb_get_property(struct power_supply *psy,
           enum power_supply_property psp,
           union power_supply_propval *val)
{
    struct aml_pmu_charger *charger;
    int ret = 0;
    charger = container_of(psy, struct aml_pmu_charger, usb);
    
    switch(psp){
    case POWER_SUPPLY_PROP_MODEL_NAME:
        val->strval = charger->usb.name;
        break;

    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = charger->usb_det;
        break;

    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = charger->usb_det; // charger->usb_valid;
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = 5000 * 1000;      // charger->vusb * 1000;
        break;

    case POWER_SUPPLY_PROP_CURRENT_NOW:
        val->intval = 1000 * 1000;      // charger->iusb * 1000;
        break;

    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static char *supply_list[] = {
    "battery",
};

static void aml_pmu_battery_setup_psy(struct aml_pmu_charger *charger)
{
    struct power_supply      *batt = &charger->batt;
    struct power_supply      *ac   = &charger->ac;
    struct power_supply      *usb  = &charger->usb;
    struct power_supply_info *info = charger->battery_info;
    
    batt->name           = "battery";
    batt->use_for_apm    = info->use_for_apm;
    batt->type           = POWER_SUPPLY_TYPE_BATTERY;
    batt->get_property   = aml_pmu_battery_get_property;
    batt->properties     = aml_pmu_battery_props;
    batt->num_properties = ARRAY_SIZE(aml_pmu_battery_props);
    
    ac->name             = "ac";
    ac->type             = POWER_SUPPLY_TYPE_MAINS;
    ac->get_property     = aml_pmu_ac_get_property;
    ac->supplied_to      = supply_list;
    ac->num_supplicants  = ARRAY_SIZE(supply_list);
    ac->properties       = aml_pmu_ac_props;
    ac->num_properties   = ARRAY_SIZE(aml_pmu_ac_props);
    
    usb->name            = "usb";
    usb->type            = POWER_SUPPLY_TYPE_USB;
    usb->get_property    = aml_pmu_usb_get_property;
    usb->supplied_to     = supply_list,
    usb->num_supplicants = ARRAY_SIZE(supply_list),
    usb->properties      = aml_pmu_usb_props;
    usb->num_properties  = ARRAY_SIZE(aml_pmu_usb_props);
}

int aml_pmu_set_usb_current_limit(int curr, int bc_mode)
{
    uint8_t val;

    aml_pmu_read(AML1212_CHG_CTRL3, &val);
    val &= ~(0x30);
    gcharger->usb_connect_type = bc_mode;
    AML_PMU_DBG("usb connet mode:%d, current limit to:%dmA\n", bc_mode, curr);
    switch (curr) {
    case 0:
        val |= 0x30;                                    // disable limit
        break;

    case 100:
        val |= 0x00;                                    // 100mA
        break;

    case 500:
        val |= 0x10;                                    // 500mA
        break;

    case 900:
        val |= 0x20;                                    // 900mA
        break;

    default:
        AML_PMU_DBG("%s, wrong usb current limit:%d\n", __func__, curr);
        return -1; 
    }
    return aml_pmu_write(AML1212_CHG_CTRL3, val);
}
EXPORT_SYMBOL_GPL(aml_pmu_set_usb_current_limit);

int aml_pmu_get_battery_percent(void)
{
    CHECK_DRIVER();
    return gcharger->rest_vol;    
}
EXPORT_SYMBOL_GPL(aml_pmu_get_battery_percent);

int aml_pmu_set_charge_current(int chg_cur)
{
    uint8_t val;

    aml_pmu_read(AML1212_CHG_CTRL4, &val);
    switch (chg_cur) {
    case 1500000:
        val &= ~(0x01 << 5);
        break;

    case 2000000:
        val |= (0x01 << 5);
        break;

    default:
        AML_PMU_DBG("%s, Wrong charge current:%d\n", __func__, chg_cur);
        return -1;
    }
    aml_pmu_write(AML1212_CHG_CTRL4, val);

    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_set_charge_current);

int aml_pmu_set_charge_voltage(int voltage)
{
    uint8_t val;
    uint8_t tmp;
    
    if (voltage > 4400000 || voltage < 4050000) {
        AML_PMU_DBG("%s,Wrong charge voltage:%d\n", __func__, voltage);
        return -1;
    }
    tmp = ((voltage - 4050000) / 50000) & 0x07;
    aml_pmu_read(AML1212_CHG_CTRL0, &val);
    val &= ~(0x07);
    val |= tmp;
    aml_pmu_write(AML1212_CHG_CTRL0, val);

    return 0; 
}
EXPORT_SYMBOL_GPL(aml_pmu_set_charge_voltage);

int aml_pmu_set_charge_end_rate(int rate) 
{
    uint8_t val;

    aml_pmu_read(AML1212_CHG_CTRL4, &val);
    switch (rate) {
    case 10:
        val &= ~(0x01 << 3);
        break;

    case 20:
        val |= (0x01 << 3);
        break;

    default:
        AML_PMU_DBG("%s, Wrong charge end rate:%d\n", __func__, rate);
        return -1;
    }
    aml_pmu_write(AML1212_CHG_CTRL4, val);

    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_set_charge_end_rate);

int aml_pmu_set_adc_freq(int freq)
{
    uint8_t val;
    int32_t time;
    int32_t time_base;
    int32_t time_bit;

    if (freq > 1000 || freq < 10) {
        AML_PMU_DBG("%s, Wrong adc freq:%d\n", __func__, freq);    
        return -1;
    }
    time = 1000 / freq;
    if (time >= 1 && time < 10) {
        time_base = 1;
        time_bit  = 0;
    } else if (time >= 10 && time < 99) {
        time_base = 10;
        time_bit  = 1;
    } else {
        time_base = 100;
        time_bit  = 2;
    }
    time /= time_base;
    val = ((time_bit << 6) | (time - 1)) & 0xff;
    AML_PMU_DBG("%s, set reg[0xA0] to %02x\n", __func__, val);          // TEST
    aml_pmu_write(AML1212_SAR_CNTL_REG5, val);

    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_set_adc_freq);

int aml_pmu_set_precharge_time(int minute)
{
    uint8_t val;

    if (minute > 80 || minute < 30) {
        AML_PMU_DBG("%s, Wrong pre-charge time:%d\n", __func__, minute);
        return -1;
    }
    aml_pmu_read(AML1212_CHG_CTRL3, &val);
    val &= ~(0x03);
    switch (minute) {
    case 30:
        val |= 0x01;
        break;

    case 50:
        val |= 0x02;
        break;

    case 80:
        val |= 0x03;
        break;
    
    default:
        AML_PMU_DBG("%s, Wrong pre-charge time:%d\n", __func__, minute);
        return -1;
    }
    aml_pmu_write(AML1212_CHG_CTRL3, val);

    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_set_precharge_time);

int aml_pmu_set_fastcharge_time(int minute)
{
    uint8_t val;

    if (minute < 360 || minute > 720) {
        AML_PMU_DBG("%s, Wrong fast-charge time:%d\n", __func__, minute);
        return -1;
    }
    aml_pmu_read(AML1212_CHG_CTRL3, &val);
    val &= ~(0xC0);
    switch (minute) {
    case 360:
        val |= (0x01 << 6);
        break;

    case 540:
        val |= (0x02 << 6);
        break;

    case 720:
        val |= (0x03 << 6);
        break;

    default:
        AML_PMU_DBG("%s, Wrong pre-charge time:%d\n", __func__, minute);
        return -1;
    }
    aml_pmu_write(AML1212_CHG_CTRL3, val);

    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_set_fastcharge_time);

int aml_pmu_set_charge_enable(int en)
{
    uint8_t val;
    
    if (pmu_version <= 0x02 && pmu_version != 0) {                      /* reversion A or B             */
        aml_pmu_read(AML1212_OTP_GEN_CONTROL0, &val);
        if (en) {
            val |= (0x01);    
        } else {
            val &= ~(0x01);    
        }
        aml_pmu_write(AML1212_OTP_GEN_CONTROL0, val);
    } else if (pmu_version == 0x03) {                                   /* reversion D                  */
        aml_pmu_set_bits(0x0011, ((en & 0x01) << 7), 0x80); 
    }

    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_set_charge_enable);

int aml_pmu_set_usb_voltage_limit(int voltage)
{
    uint8_t val;

    if (voltage > 4600 || voltage < 4300) {
        AML_PMU_DBG("%s, Wrong usb voltage limit:%d\n", __func__, voltage);    
    }
    aml_pmu_read(AML1212_CHG_CTRL4, &val);
    val &= ~(0xc0);
    switch (voltage) {
    case 4300:
        val |= (0x01 << 6);
        break;

    case 4400:
        val |= (0x02 << 6);
        break;

    case 4500:
        val |= (0x00 << 6);
        break;

    case 4600:
        val |= (0x03 << 6);
        break;
    
    default:
        AML_PMU_DBG("%s, Wrong usb voltage limit:%d\n", __func__, voltage);
        return -1;
    }
    aml_pmu_write(AML1212_CHG_CTRL4, val);

    return 0;
}
EXPORT_SYMBOL_GPL(aml_pmu_set_usb_voltage_limit);

void aml_pmu_clear_coulomb(void)
{
	aml_pmu_write(AML1212_SAR_SW_EN_FIELD, 0x80); 
}

int aml_pmu_first_init(struct aml_pmu_charger *charger)
{
    uint8_t val;
    uint8_t irq_mask[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xf0};

    aml_pmu_write(AML1212_SAR_CNTL_REG2,   0x64);                       // select vref=2.4V
    aml_pmu_write(AML1212_SAR_CNTL_REG3,   0x14);                       // close the useless channel input
    aml_pmu_write(AML1212_SAR_CNTL_REG0,   0x0c);                       // enable IBAT_AUTO, ACCUM
    aml_pmu_writes(AML1212_IRQ_MASK_0, sizeof(irq_mask), irq_mask);     // open all IRQ

    /*
     * initialize charger from battery parameters
     */
    aml_pmu_set_charge_current (aml_pmu_battery->pmu_init_chgcur);
    aml_pmu_set_charge_voltage (aml_pmu_battery->pmu_init_chgvol);
    aml_pmu_set_charge_end_rate(aml_pmu_battery->pmu_init_chgend_rate);
    aml_pmu_set_adc_freq       (aml_pmu_battery->pmu_init_adc_freqc);
    aml_pmu_set_precharge_time (aml_pmu_battery->pmu_init_chg_pretime);
    aml_pmu_set_fastcharge_time(aml_pmu_battery->pmu_init_chg_csttime);
    aml_pmu_set_charge_enable  (aml_pmu_battery->pmu_init_chg_enabled);

    if (aml_pmu_battery->pmu_usbvol_limit) {
        aml_pmu_set_usb_voltage_limit(aml_pmu_battery->pmu_usbvol); 
    }
    if (aml_pmu_battery->pmu_usbcur_limit) {
        aml_pmu_set_usb_current_limit(500, USB_BC_MODE_SDP);            // fix to 500 when init
    }

    return 0;
}

/*
 * add for debug 
 */
static ssize_t dbg_info_show     (struct device *dev, struct device_attribute *attr, char *buf, int count);
static ssize_t dbg_info_store    (struct device *dev, struct device_attribute *attr, char *buf, int count);
static ssize_t battery_para_show (struct device *dev, struct device_attribute *attr, char *buf, int count);
static ssize_t battery_para_store(struct device *dev, struct device_attribute *attr, char *buf, int count);
static ssize_t report_delay_show (struct device *dev, struct device_attribute *attr, char *buf, int count);
static ssize_t report_delay_store(struct device *dev, struct device_attribute *attr, char *buf, int count);

static int     aml_pmu_regs_base = 0;
static ssize_t aml_pmu_reg_base_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "aml_pmu_regs_base: 0x%02x\n", aml_pmu_regs_base); 
}

static ssize_t aml_pmu_reg_base_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    int tmp = simple_strtoul(buf, NULL, 16);
    if (tmp > 255) {
        AML_PMU_DBG("Invalid input value\n");
        return -1;
    }
    aml_pmu_regs_base = tmp;
    AML_PMU_DBG("Set register base to 0x%02x\n", aml_pmu_regs_base);
    return count;
}

static ssize_t aml_pmu_reg_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint8_t data;
    aml_pmu_read(aml_pmu_regs_base, &data);
    return sprintf(buf, "reg[0x%02x] = 0x%02x\n", aml_pmu_regs_base, data);
}

static ssize_t aml_pmu_reg_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint8_t data = simple_strtoul(buf, NULL, 16);
    if (data > 255) {
        AML_PMU_DBG("Invalid input value\n");
        return -1;
    }
    aml_pmu_write(aml_pmu_regs_base, data);
    return count; 
}

static ssize_t aml_pmu_reg_16bit_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint16_t data;
    if (aml_pmu_regs_base & 1){
        AML_PMU_DBG("Invalid reg base value\n");
        return -1;
    }
    aml_pmu_read16(aml_pmu_regs_base, &data);
    return sprintf(buf, "reg[0x%04x] = 0x%04x\n", aml_pmu_regs_base, data);
}

static ssize_t aml_pmu_reg_16bit_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint16_t data = simple_strtoul(buf, NULL, 16);
    if (data > 0xffff) {
        AML_PMU_DBG("Invalid input value\n");
        return -1;
    }
    if (aml_pmu_regs_base & 1){
        AML_PMU_DBG("Invalid reg base value\n");
        return -1;
    }
    aml_pmu_write16(aml_pmu_regs_base, data);
    return count; 
}

static ssize_t aml_pmu_vddao_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint8_t data;
    aml_pmu_read(0x2f, &data);

    return sprintf(buf, "Voltage of VDD_AO = %4dmV\n", dcdc1_voltage_table[(data & 0xfc) >> 2]);
}

static ssize_t aml_pmu_vddao_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint32_t data = simple_strtoul(buf, NULL, 10);
    if (data > 2000 || data < 740) {
        AML_PMU_DBG("Invalid input value = %d\n", data);
        return -1;
    }
    AML_PMU_DBG("Set VDD_AO to %4d mV\n", data);
    aml_pmu_set_voltage(AML_PMU_DCDC1, data);
    return count; 
}

static ssize_t driver_version_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    return sprintf(buf, "Amlogic PMU Aml1212 driver version is %s, build time:%s\n", 
                   AML1212_DRIVER_VERSION, init_uts_ns.name.version);
}

static ssize_t driver_version_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    return count; 
}

static ssize_t clear_rtc_mem_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    return count;
}

static ssize_t clear_rtc_mem_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{ 
    aml_write_rtc_mem_reg(0, 0);
    aml_pmu_poweroff();
    return count; 
}

static ssize_t charge_timeout_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint8_t val = 0;

    aml_pmu_read(AML1212_CHG_CTRL3, &val);
    val >>= 6;
    if (val) {
        return sprintf(buf, "charge timeout is %d minutes\n", val * 180 + 180);   
    }
    return count;
}

static ssize_t charge_timeout_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{ 
    uint32_t data = simple_strtoul(buf, NULL, 10);
    if (data > 720 || data < 360) {
        AML_PMU_DBG("Invalid input value = %d\n", data);
        return -1;
    }
    AML_PMU_DBG("Set charge timeout to %4d minutes\n", data);
    aml_pmu_set_fastcharge_time(data); 
    return count; 
}

static struct device_attribute aml_pmu_charger_attrs[] = {
    AML_PMU_CHG_ATTR(aml_pmu_reg_base),
    AML_PMU_CHG_ATTR(aml_pmu_reg),
    AML_PMU_CHG_ATTR(aml_pmu_reg_16bit),
    AML_PMU_CHG_ATTR(aml_pmu_vddao),
    AML_PMU_CHG_ATTR(dbg_info),
    AML_PMU_CHG_ATTR(battery_para),
    AML_PMU_CHG_ATTR(report_delay),
    AML_PMU_CHG_ATTR(driver_version),
    AML_PMU_CHG_ATTR(clear_rtc_mem),
    AML_PMU_CHG_ATTR(charge_timeout),
};

int aml_pmu_charger_create_attrs(struct power_supply *psy)
{
    int j,ret;
    for (j = 0; j < ARRAY_SIZE(aml_pmu_charger_attrs); j++) {
        ret = device_create_file(psy->dev, &aml_pmu_charger_attrs[j]);
        if (ret)
            goto sysfs_failed;
    }
    goto succeed;

sysfs_failed:
    while (j--) {
        device_remove_file(psy->dev, &aml_pmu_charger_attrs[j]);
    }
succeed:
    return ret;
}

static void aml_charger_update_state(struct aml_pmu_charger *charger)
{
    uint8_t buff[5] = {};
    static int chg_gat_bat_lv = 0;

    aml_pmu_reads(AML1212_SP_CHARGER_STATUS0, sizeof(buff), buff);

    if (!(buff[3] & 0x02)) {                                            // CHG_GAT_BAT_LV = 0, discharging
        charger->bat_current_direction = 2; 
        current_dir = 0;
    } else if ((buff[3] & 0x02) && (buff[2] & 0x04)) {
        charger->bat_current_direction = 1;                             // charging
        current_dir = 1;
    } else {
        charger->bat_current_direction = 3;                             // Not charging 
        current_dir = 2;
    }
    charger->bat_det               = 1;                                // do not check register 0xdf, bug here
    charger->ac_det                = buff[2] & 0x10 ? 1 : 0;
    charger->usb_det               = buff[2] & 0x08 ? 1 : 0;
    charger->ext_valid             = buff[2] & 0x18;                   // to differ USB / AC status update 

    chg_status_reg = (buff[0] <<  0) | (buff[1] <<  8) |
                     (buff[2] << 16) | (buff[3] << 24);
    if ((!(buff[3] & 0x02)) && !chg_gat_bat_lv) {                       // according David Wang
        AML_PMU_DBG("CHG_GAT_BAT_LV is 0, limit usb current to 500mA\n");
        aml_pmu_set_usb_current_limit(500, charger->usb_connect_type); 
        chg_gat_bat_lv = 1;
    } else if (buff[3] & 0x02 && chg_gat_bat_lv) {
        chg_gat_bat_lv = 0;    
        if (charger->usb_connect_type == USB_BC_MODE_DCP || 
            charger->usb_connect_type == USB_BC_MODE_CDP) {             // reset to 900 when enough current supply
            aml_pmu_set_usb_current_limit(900, charger->usb_connect_type);    
            AML_PMU_DBG("CHG_GAT_BAT_LV is 1, limit usb current to 900mA\n");
        }
    }
    if (buff[1] & 0x40) {                                               // charge timeout detect
        AML_PMU_DBG("Charge timeout deteceted\n");
        if ((charger->charge_timeout_retry) &&
            (charger->charge_timeout_retry > re_charge_cnt)) {
            re_charge_cnt++;
            AML_PMU_DBG("reset charger due to charge timeout occured, ocv :%d, retry:%d\n", 
                        charger->ocv, re_charge_cnt);
            aml_pmu_set_fastcharge_time(360);                           // only retry charge 6 hours, for safe problem
            aml_pmu_set_charge_enable(0);
            msleep(1000);
            aml_pmu_set_charge_enable(1);
        }
        charge_timeout = 1;
    } else {
        charge_timeout = 0;    
    }
    if (charger->ext_valid && !(power_flag & 0x01)) {                   // enable charger when detect extern power
        power_flag |=  0x01;                                            // remember enabled charger 
        power_flag &= ~0x02;
    } else if (!charger->ext_valid && !(power_flag & 0x02)) {
        re_charge_cnt = 0;
        aml_pmu_set_fastcharge_time(aml_pmu_battery->pmu_init_chg_csttime);
        power_flag |=  0x02;                                            // remember disabled charger
        power_flag &= ~0x01;
    }
    if (!charger->ext_valid && charger->vbus_dcin_short_connect) {
        aml_pmu_set_dcin(0);                                            // disable DCIN when no extern power
    }
}

int aml_cal_ocv(int ibat, int vbat, int dir)
{
    int result;

    if (dir == 1) {                                                     // charging
        result = vbat - (ibat * aml_pmu_battery->pmu_battery_rdc) / 1000;
    } else if (dir == 2) {                                              // discharging
        result = vbat + (ibat * aml_pmu_battery->pmu_battery_rdc) / 1000;    
    } else {
        result = vbat;    
    }
    return result;
}
EXPORT_SYMBOL_GPL(aml_cal_ocv);

static void aml_charger_update(struct aml_pmu_charger *charger)
{
    charger->vbat = aml_pmu_get_voltage();
    charger->ibat = aml_pmu_get_current();
    charger->ocv  = aml_cal_ocv(charger->ibat, charger->vbat, charger->bat_current_direction);
}

uint8_t pre_chg_status = 0;

static ssize_t dbg_info_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    struct power_supply    *battery = dev_get_drvdata(dev);
    struct aml_pmu_charger *charger = container_of(battery, struct aml_pmu_charger, batt); 
    int size;

    size = aml_format_dbg_buffer(charger, buf);

    return size;
}

static ssize_t dbg_info_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    return count;                                           /* nothing to do        */
}

static ssize_t battery_para_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    struct power_supply    *battery = dev_get_drvdata(dev);
    struct aml_pmu_charger *charger = container_of(battery, struct aml_pmu_charger, batt); 
    int i = 0; 
    int size;

    size = sprintf(buf, "\n i,      ocv,    charge,  discharge,\n");
    for (i = 0; i < 16; i++) {
        size += sprintf(buf + size, "%2d,     %4d,       %3d,        %3d,\n",
                        i, 
                        aml_pmu_battery->pmu_bat_curve[i].ocv,
                        aml_pmu_battery->pmu_bat_curve[i].charge_percent,
                        aml_pmu_battery->pmu_bat_curve[i].discharge_percent);
    }
    size += sprintf(buf + size, "\nBattery capability:%4d@3700mAh, RDC:%3d mohm\n", 
                                aml_pmu_battery->pmu_battery_cap, 
                                aml_pmu_battery->pmu_battery_rdc);
    size += sprintf(buf + size, "Charging efficiency:%3d%%, capability now:%3d%%\n", 
                                aml_pmu_battery->pmu_charge_efficiency,
                                charger->rest_vol);
    size += sprintf(buf + size, "ocv_empty:%4d, ocv_full:%4d\n\n",
                                charger->ocv_empty, 
                                charger->ocv_full);
    return size;
}

static ssize_t battery_para_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    return count;                                           /* nothing to do        */    
}

static ssize_t report_delay_show(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    return sprintf(buf, "report_delay = %d\n", report_delay); 
}

static ssize_t report_delay_store(struct device *dev, struct device_attribute *attr, char *buf, int count)
{
    uint32_t tmp = simple_strtoul(buf, NULL, 10);

    if (tmp > 200) {
        AML_PMU_DBG("input too large, failed to set report_delay\n");    
        return count;
    }
    report_delay = tmp;
    return count;
}

static void aml_pmu_charging_monitor(struct work_struct *work)
{
    struct   aml_pmu_charger *charger;
    int      pre_rest_cap;
    uint8_t  val,v[2];
    static int check_charge_flag = 0; 

    charger = container_of(work, struct aml_pmu_charger, work.work);

    /*
     * 1. update status of PMU and all ADC value
     * 2. read ocv value and calculate ocv percent of battery
     * 3. read coulomb value and calculate movement of energy
     * 4. if battery capacity is larger than 429496 mAh, will cause over flow
     */
    pre_rest_cap = charger->rest_vol;
    aml_charger_update_state(charger);
    aml_charger_update(charger);
    aml_update_battery_capacity(charger, aml_pmu_battery);

    if (charger->ocv > 5000) {                  // SAR ADC error, only for test
        AML_PMU_DBG(">> SAR ADC error, ocv:%d, vbat:%d, ibat:%d\n", 
                    charger->ocv, charger->vbat, charger->ibat);
        charger->rest_vol = 0;
    }

    if ((charger->rest_vol - pre_rest_cap) || (pre_chg_status != charger->ext_valid) || charger->resume) {
        AML_PMU_DBG("battery vol change: %d->%d \n", pre_rest_cap, charger->rest_vol);
        power_supply_changed(&charger->batt);
        aml_post_update_process(charger);
    }
    pre_chg_status = charger->ext_valid;

    /*
     * work around for cannot stop charge problem, according David Wang
     */
    if (charger->rest_vol >= 99 && charger->ibat < 150 && charger->bat_current_direction == 1 && !check_charge_flag) {
        aml_pmu_read(AML1212_SP_CHARGER_STATUS3, v);
        if (!(v[0] & 0x08)) {
            check_charge_flag = 1;
            AML_PMU_DBG("CHG_END_DET = 0 find, close charger for 1 second\n");
            aml_pmu_set_charge_enable(0);
            msleep(1000);
            aml_pmu_set_charge_enable(1);
        }
    } else if (charger->rest_vol < 99) {
        check_charge_flag = 0;    
    }

    if (charger->aml_pmu_call_back) {                                       // reserved for user definition
        charger->aml_pmu_call_back(charger->para);
    }

    /* reschedule for the next time */
    schedule_delayed_work(&charger->work, charger->interval);
}

#if defined CONFIG_HAS_EARLYSUSPEND
static void aml_pmu_earlysuspend(struct early_suspend *h)
{
    uint8_t tmp;
    
    // add charge current limit code here
}

static void aml_pmu_lateresume(struct early_suspend *h)
{
    int     ibat = 0, vbat = 0, i;
    struct  aml_pmu_charger *charger = (struct aml_pmu_charger *)h->param;

    schedule_work(&charger->work);                                  // update for upper layer 
    aml_pmu_set_voltage(AML_PMU_DCDC1, 1100);                       // reset to 1.1V
    // add charge current limit code here
}
#endif

irqreturn_t aml_pmu_irq_handler(int irq, void *dev_id)
{
    struct   aml_pmu_charger *charger = (struct aml_pmu_charger *)dev_id;

    disable_irq_nosync(charger->irq);
    schedule_work(&charger->irq_work);

    return IRQ_HANDLED;
}

static void aml_pmu_irq_work_func(struct work_struct *work)
{
    struct aml_pmu_charger *charger = container_of(work, struct aml_pmu_charger, irq_work);
    uint8_t irq_status[6] = {};

    aml_pmu_reads(AML1212_IRQ_STATUS_CLR_0, sizeof(irq_status), irq_status);
    AML_PMU_DBG("PMU IRQ status: %02x %02x %02x %02x %02x %02x", 
                irq_status[0], irq_status[1], irq_status[2],
                irq_status[3], irq_status[4], irq_status[5]);
    aml_pmu_writes(AML1212_IRQ_STATUS_CLR_0, sizeof(irq_status), irq_status);       // clear IRQ status
    if (irq_status[5] & 0x08) {
        AML_PMU_DBG("Over Temperature is occured, shutdown system\n");
        aml_pmu_poweroff();
    }
    enable_irq(charger->irq);

    return ;
}

static void check_pmu_version(void)
{
    uint8_t val;
    aml_pmu_read(0x007f, &val);
    AML_PMU_DBG("PMU VERSION: 0x%02x\n", val);
    if (val > 0x03 || val == 0x00) {
        AML_PMU_DBG("#### ERROR: unknow pmu version:0x%02x ####\n", val);    
    } else {
        pmu_version = val;
    }
}

static int aml_pmu_battery_probe(struct platform_device *pdev)
{
    struct   aml_pmu_charger *charger;
    struct   aml_pmu_init    *pdata = pdev->dev.platform_data;
    int      ret;
    int      max_diff = 0, tmp_diff;
    uint32_t tmp2;

	AML_PMU_DBG("call %s in", __func__);
    aml_pmu_power_key = input_allocate_device();
    if (!aml_pmu_power_key) {
        kfree(aml_pmu_power_key);
        return -ENODEV;
    }

    aml_pmu_power_key->name       = pdev->name;
    aml_pmu_power_key->phys       = "m1kbd/input2";
    aml_pmu_power_key->id.bustype = BUS_HOST;
    aml_pmu_power_key->id.vendor  = 0x0001;
    aml_pmu_power_key->id.product = 0x0001;
    aml_pmu_power_key->id.version = 0x0100;
    aml_pmu_power_key->open       = NULL;
    aml_pmu_power_key->close      = NULL;
    aml_pmu_power_key->dev.parent = &pdev->dev;

    set_bit(EV_KEY, aml_pmu_power_key->evbit);
    set_bit(EV_REL, aml_pmu_power_key->evbit);
    set_bit(KEY_POWER, aml_pmu_power_key->keybit);

    ret = input_register_device(aml_pmu_power_key);

    if (pdata == NULL) {
        return -EINVAL;    
    }

#ifdef CONFIG_UBOOT_BATTERY_PARAMETERS 
    if (get_uboot_battery_para_status() == UBOOT_BATTERY_PARA_SUCCESS) {
        aml_pmu_battery = get_uboot_battery_para();
        AML_PMU_DBG("use uboot passed battery parameters\n");
    } else {
        aml_pmu_battery = pdata->board_battery; 
        AML_PMU_DBG("uboot battery parameter not get, use BSP configed battery parameters\n");
    }
#else
    aml_pmu_battery = pdata->board_battery; 
    AML_PMU_DBG("use BSP configed battery parameters\n");
#endif

    /*
     * initialize parameters for charger
     */
    charger = kzalloc(sizeof(*charger), GFP_KERNEL);
    if (charger == NULL) {
        return -ENOMEM;
    }
    charger->battery_info = kzalloc(sizeof(struct power_supply_info), GFP_KERNEL);
    if (charger->battery_info == NULL) {
        kfree(charger);
        return -ENOMEM;    
    }
    charger->master = pdev->dev.parent;

    gcharger = charger;
    check_pmu_version();
    for (tmp2 = 0; tmp2 < 16; tmp2++) {
        if (!charger->ocv_empty && aml_pmu_battery->pmu_bat_curve[tmp2].discharge_percent > 0) {
            charger->ocv_empty = aml_pmu_battery->pmu_bat_curve[tmp2-1].ocv;
        }
        if (!charger->ocv_full && aml_pmu_battery->pmu_bat_curve[tmp2].discharge_percent == 100) {
            charger->ocv_full = aml_pmu_battery->pmu_bat_curve[tmp2].ocv;    
        }
        tmp_diff = aml_pmu_battery->pmu_bat_curve[tmp2].discharge_percent -
                   aml_pmu_battery->pmu_bat_curve[tmp2].charge_percent;
        if (tmp_diff > max_diff) {
            max_diff = tmp_diff;
        }
    }

    charger->irq = aml_pmu_battery->pmu_irq_id;

    charger->battery_info->technology         = aml_pmu_battery->pmu_battery_technology;
    charger->battery_info->voltage_max_design = aml_pmu_battery->pmu_init_chgvol;
    charger->battery_info->energy_full_design = aml_pmu_battery->pmu_battery_cap;
    charger->battery_info->voltage_min_design = charger->ocv_empty * 1000;
    charger->battery_info->use_for_apm        = 1;
    charger->battery_info->name               = aml_pmu_battery->pmu_battery_name;

    charger->aml_pmu_call_back = pdata->aml_pmu_call_back;
    charger->para              = pdata->para;
    charger->soft_limit_to99   = pdata->soft_limit_to99;
    charger->charge_timeout_retry = pdata->charge_timeout_retry;
    charger->vbus_dcin_short_connect = pdata->vbus_dcin_short_connect;
    re_charge_cnt                 = pdata->charge_timeout_retry;
    if (charger->irq == AML_PMU_IRQ_NUM) {
        INIT_WORK(&charger->irq_work, aml_pmu_irq_work_func); 
        ret = request_irq(charger->irq, 
                          aml_pmu_irq_handler, 
                          IRQF_DISABLED | IRQF_SHARED,
                          AML1212_IRQ_NAME,
                          charger); 
        if (ret) {
            AML_PMU_DBG("request irq failed, ret:%d, irq:%d\n", ret, charger->irq);    
        }
    }

    aml_charger_update_state(charger); 
    ret = aml_pmu_first_init(charger);
    if (ret) {
        goto err_charger_init;
    }

    aml_pmu_battery_setup_psy(charger);
    ret = power_supply_register(&pdev->dev, &charger->batt);
    if (ret) {
        goto err_ps_register;
    }

    ret = power_supply_register(&pdev->dev, &charger->ac);
    if (ret){
        power_supply_unregister(&charger->batt);
        goto err_ps_register;
    }
    ret = power_supply_register(&pdev->dev, &charger->usb);
    if (ret){
        power_supply_unregister(&charger->ac);
        power_supply_unregister(&charger->batt);
        goto err_ps_register;
    }

    ret = aml_pmu_charger_create_attrs(&charger->batt);
    if(ret){
        return ret;
    }

    platform_set_drvdata(pdev, charger);

    charger->interval = msecs_to_jiffies(AML_PMU_WORK_CYCLE);
    INIT_DELAYED_WORK(&charger->work, aml_pmu_charging_monitor);
    schedule_delayed_work(&charger->work, charger->interval);

#ifdef CONFIG_HAS_EARLYSUSPEND
    aml_pmu_early_suspend.suspend = aml_pmu_earlysuspend;
    aml_pmu_early_suspend.resume = aml_pmu_lateresume;
    aml_pmu_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
    aml_pmu_early_suspend.param = charger;
    register_early_suspend(&aml_pmu_early_suspend);
#endif

    aml_battery_probe_process(charger, aml_pmu_battery);
    power_supply_changed(&charger->batt);                   // update battery status
    
    aml_pmu_set_gpio(1, 0);                                 // open LCD backlight, test
    aml_pmu_set_gpio(2, 0);                                 // open VCCx2, test

	AML_PMU_DBG("call %s exit, ret:%d", __func__, ret);
    return ret;

err_ps_register:
    free_irq(charger->irq, charger);
    cancel_delayed_work_sync(&charger->work);

err_charger_init:
    kfree(charger->battery_info);
    kfree(charger);
    input_unregister_device(aml_pmu_power_key);
    kfree(aml_pmu_power_key);
	AML_PMU_DBG("call %s exit, ret:%d", __func__, ret);
    return ret;
}

static int aml_pmu_battery_remove(struct platform_device *dev)
{
    struct aml_pmu_charger *charger = platform_get_drvdata(dev);

    cancel_work_sync(&charger->irq_work);
    cancel_delayed_work_sync(&charger->work);
    power_supply_unregister(&charger->usb);
    power_supply_unregister(&charger->ac);
    power_supply_unregister(&charger->batt);
    
    free_irq(charger->irq, charger);
    kfree(charger->battery_info);
    kfree(charger);
    input_unregister_device(aml_pmu_power_key);
    kfree(aml_pmu_power_key);

    return 0;
}


static int aml_pmu_suspend(struct platform_device *dev, pm_message_t state)
{
    struct aml_pmu_charger *charger = platform_get_drvdata(dev);

    cancel_delayed_work_sync(&charger->work);
    aml_pmu_set_charge_current(aml_pmu_battery->pmu_suspend_chgcur);
    if (charger->usb_connect_type != USB_BC_MODE_SDP) {
        aml_pmu_set_usb_current_limit(900, charger->usb_connect_type);  // not pc, set to 900mA when suspend
    } else {
        aml_pmu_set_usb_current_limit(500, charger->usb_connect_type);  // pc, limit to 500mA
    }
    aml_battery_suspend_process();

    return 0;
}

static int aml_pmu_resume(struct platform_device *dev)
{
    struct   aml_pmu_charger *charger = platform_get_drvdata(dev);
    int      ocv_resume = 0, i, vbat = 0, ibat = 0;

    aml_charger_update_state(charger);
    for (i = 0; i < 8; i++) {
        vbat += aml_pmu_get_voltage();
        ibat += aml_pmu_get_current();
        msleep(5);
    }
    vbat = vbat / 8;
    ibat = ibat / 8;
    ocv_resume = aml_cal_ocv(ibat, vbat, charger->bat_current_direction); 
    aml_battery_resume_process(charger, aml_pmu_battery, ocv_resume);
    schedule_work(&charger->work);
    aml_pmu_set_charge_current(aml_pmu_battery->pmu_resume_chgcur);

    return 0;
}

static void aml_pmu_shutdown(struct platform_device *dev)
{
    uint8_t tmp;
    struct aml_pmu_charger *charger = platform_get_drvdata(dev);
    
    // add code here
}

static struct platform_driver aml_pmu_battery_driver = {
    .driver = {
        .name  = AML1212_SUPPLY_NAME, 
        .owner = THIS_MODULE,
    },
    .probe    = aml_pmu_battery_probe,
    .remove   = aml_pmu_battery_remove,
    .suspend  = aml_pmu_suspend,
    .resume   = aml_pmu_resume,
    .shutdown = aml_pmu_shutdown,
};

static int aml_pmu_battery_init(void)
{
    int ret;
    ret = platform_driver_register(&aml_pmu_battery_driver);
	AML_PMU_DBG("call %s, ret = %d\n", __func__, ret);
	return ret;
}

static void aml_pmu_battery_exit(void)
{
    platform_driver_unregister(&aml_pmu_battery_driver);
}

subsys_initcall(aml_pmu_battery_init);
module_exit(aml_pmu_battery_exit);

MODULE_DESCRIPTION("Amlogic PMU AML1212 battery driver");
MODULE_AUTHOR("tao.zeng@amlogic.com, Amlogic, Inc");
MODULE_LICENSE("GPL");
