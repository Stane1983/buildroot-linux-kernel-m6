#ifndef __WATCHDOG_H_
#define __WATCHDOG_H_
#include <linux/list.h>

#define MAX_TIME_OUT 		40000
#define MIN_TIME_OUT 		10000
#define DEFAULT_TIME_OUT	25000	

#define WATCHDOG_OFF	0
#define WATCHDOG_ON	1

#define NAME_MAX_LEN	16
#define SUSPEND_WATCHDOG "suspend_watchdog"

typedef struct {
	struct list_head entry;
	int state;
	int time_out;
	char name[NAME_MAX_LEN + 1];
} aml_watchdog_t;

int aml_disable_watchdog(aml_watchdog_t* watchdog);
int aml_enable_watchdog(aml_watchdog_t* watchdog);
int aml_set_watchdog_timeout_ms(aml_watchdog_t* watchdog, int time_out);
int aml_reset_watchdog(aml_watchdog_t* watchdog);
aml_watchdog_t* aml_get_watchdog(const char* name);
aml_watchdog_t* aml_create_watchdog(const char* name);
int aml_destroy_watchdog(aml_watchdog_t* watchdog);
int aml_emergency_restart(int time_out);

#ifdef CONFIG_SUSPEND_WATCHDOG
static aml_watchdog_t* suspend_watchdog;
#define RESET_SUSPEND_WATCHDOG \
do { \
	if (!suspend_watchdog) \
		suspend_watchdog = aml_get_watchdog(SUSPEND_WATCHDOG); \
	if (suspend_watchdog) \
		aml_reset_watchdog(suspend_watchdog); \
	suspend_watchdog = NULL; \
} while(0)

#define ENABLE_SUSPEND_WATCHDOG \
do { \
	if (!suspend_watchdog) \
		suspend_watchdog = aml_get_watchdog(SUSPEND_WATCHDOG); \
	if (suspend_watchdog) { \
		aml_set_watchdog_timeout_ms(suspend_watchdog, 20000); \
		aml_enable_watchdog(suspend_watchdog); \
		suspend_watchdog = NULL; \
	} \
} while(0)

#define DISABLE_SUSPEND_WATCHDOG \
do { \
	if (!suspend_watchdog) \
		suspend_watchdog = aml_get_watchdog(SUSPEND_WATCHDOG); \
	if (suspend_watchdog) \
		aml_disable_watchdog(suspend_watchdog); \
	suspend_watchdog = NULL; \
} while(0)
#endif /* CONFIG_SUSPEND_WATCHDOG */

#endif /* __WATCHDOG_H_ */