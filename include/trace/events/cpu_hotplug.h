#undef TRACE_SYSTEM
#define TRACE_SYSTEM cpu_hotplug

#if !defined(_TRACE_CPU_HOTPLUG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_CPU_HOTPLUG_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(cpu_hotplug,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid),

	TP_STRUCT__entry(
		__field(unsigned int, cpuid)
	),

	TP_fast_assign(
		__entry->cpuid = cpuid;
	),

	TP_printk("cpuid=%u", __entry->cpuid)
);

/* Core function of cpu hotplug */

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_down_start,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_down_end,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_up_start,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_up_end,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

/* Architecture function for cpu hotplug */

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_disable_start,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_disable_end,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_die_start,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_die_end,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_arch_up_start,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

DEFINE_EVENT(cpu_hotplug, cpu_hotplug_arch_up_end,

	TP_PROTO(unsigned int cpuid),

	TP_ARGS(cpuid)
);

#endif /* _TRACE_CPU_HOTPLUG_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
