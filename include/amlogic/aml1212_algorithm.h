#ifndef __AML1212_ALGORITHM_H__
#define __AML1212_ALGORITHM_H__

extern uint32_t chg_status_reg;
extern uint32_t report_delay;

struct aml_charger;
struct battery_parameter;

extern ssize_t aml_format_dbg_buffer(struct aml_charger *charger, char *buf);
extern void aml_battery_resume_process(struct aml_charger *charger, struct battery_parameter *aml_pmu_battery, int ocv_resume);
extern void aml_battery_suspend_process(void);
extern void aml_battery_probe_process(struct aml_charger *charger, struct battery_parameter *aml_pmu_battery);
extern void aml_post_update_process(struct aml_charger * charger);
extern void aml_update_battery_capacity(struct aml_charger *charger, struct battery_parameter *aml_pmu_battery);

#endif /* __AML1212_ALGORITHM_H__ */
