#ifndef __AML_PMU_H__
#define __AML_PMU_H__

#include <linux/power_supply.h>

#define BATCAPCORRATE           5                                       // battery capability is very low
#define ABS(x)                  ((x) >0 ? (x) : -(x))
#define AML_PMU_WORK_CYCLE      2000                                    // PMU work cycle

/*
 * debug message control
 */
#define AML_PMU_DBG(format,args...)                 \
    if (1) printk(KERN_ERR "[AML_PMU]"format,##args) 

#define AML_PMU_CHG_ATTR(_name)                     \
{                                                   \
    .attr = { .name = #_name,.mode = 0644 },        \
    .show =  _name##_show,                          \
    .store = _name##_store,                         \
}

/*
 * @soft_limit_to99: flag for if we need to restrict battery capacity to 99% when have charge current,
 *                   even battery voltage is over ocv_full;
 * @para:            parameters for call back funtions, user implement;
 * @pmu_call_back:   call back function for axp_charging_monitor, you can add anything you want to do
 *                   in this function, this funtion will be called every 2 seconds by default
 */
struct aml_pmu_init {
    int   soft_limit_to99;                          // software limit battery volume to 99% when have charge current
    int   charge_timeout_retry;                     // retry charge count when charge timeout
    int   vbus_dcin_short_connect;                  // if VBUS and DCIN are short connected
    void  *para;                                    // used for call back
    void (*aml_pmu_call_back)(void *para);          // call back functions
    struct battery_parameter *board_battery;        // battery parameter
};

struct aml_pmu_charger {
	int32_t  interval;                                                  // PMU work cycle
	int32_t  vbat;                                                      // battery voltage
	int32_t  ibat;                                                      // battery current
	int32_t  vac;                                                       // AC adapter voltage
	int32_t  iac;                                                       // AC adapter current 
	int32_t  vusb;                                                      // VBUS voltage
	int32_t  iusb;                                                      // VBUS current
	int32_t  ocv;                                                       // open-circuit-voltage of battery
	int32_t  rest_vol;                                                  // remain capability of battery
	int32_t  ocv_rest_vol;                                              // remain capability from ocv
	int32_t  resume;                                                    // flag for suspend & resume
    int32_t  ocv_full;                                                  // which voltage we think battery is full
    int32_t  ocv_empty;                                                 // which voltage we think battery is empty
    int32_t  irq;                                                       // irq used for device
    int32_t  soft_limit_to99;                                           // see comment above
    int32_t  usb_connect_type;                                          // usb is connect to PC or adapter
    int32_t  charge_timeout_retry;                                      // retry charge count when charge timeout
    int32_t  vbus_dcin_short_connect;                                   // indicate VBUS and DCIN are short connected

	uint8_t  bat_det;                                                   // battery exist?
	uint8_t  ac_det;                                                    // ac adapter exist?
	uint8_t  usb_det;                                                   // usb adapter exist?
	uint8_t  ac_valid;                                                  // ac adapter valid
	uint8_t  usb_valid;                                                 // usb adapter valid
	uint8_t  ext_valid;                                                 // extern power valid
	uint8_t  bat_current_direction;                                     // 0: discharge, 1: charge
	uint8_t  fault;                                                     // wrong condition of battery
    
    void *para;                                                         // parameter saved for call back function
    void (*aml_pmu_call_back)(void *para);                              // call back function for charger work

	struct power_supply batt;                                           // power supply sysfs
	struct power_supply	ac;
	struct power_supply	usb;
	struct power_supply_info *battery_info;
	struct delayed_work work;                                           // work struct
    struct work_struct  irq_work;                                       // work for IRQ 

	struct device *master;
};

extern struct aml_pmu_charger *gcharger;                                // export global charger struct

#endif /* __AML_PMU_H__ */

