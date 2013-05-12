/*
 *  drivers/cpufreq/cpufreq_conservative.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *            (C)  2009 Alexander Clouter <alex@digriz.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define DEBUG

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/timer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/earlysuspend.h>
#include <linux/reboot.h>
#include <linux/syscore_ops.h>

#ifdef CONFIG_CPU_FREQ_GOV_CONSERVATIVE_BOOST
	#define ENABLE_BOOST 1
#else
	#define ENABLE_BOOST 0
#endif

static int halt=0;
static int conserv_system_reboot(struct notifier_block *nb, unsigned long event, void *unused)
{

    pr_debug("%s %lu\n", __FUNCTION__, event);
	switch (event) {
	case SYS_RESTART:
	case SYS_HALT:
	case SYS_POWER_OFF:
	default:
        halt = 1;
	}
	return NOTIFY_DONE;
}
static struct notifier_block conserv_reboot_notifier = {
	.notifier_call = conserv_system_reboot,
};

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define DEF_FREQUENCY_UP_THRESHOLD		(80)
#define DEF_FREQUENCY_DOWN_THRESHOLD		(20)

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */

/* skip change unless number of samples passed or load is very high */
#define USE_LOAD_LONG 1
#define DEF_CHANGE_SKIPS	(1)
static int change_skip_samples;

#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

#define DEF_SCREEN_OFF_MAX			(408*1000)

#define DEF_BOOST_OFF_MAX_FREQ      (1000*1000)  //in kHz
#define DEF_BOOST_MAX_TIME          (20*1000)    //in ms
#define DEF_BOOST_RESET_TIME        (20*1000)	//in ms

static void do_dbs_timer(struct work_struct *work);

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	cputime64_t prev_cpu_idle_long;
	cputime64_t prev_cpu_wall_long;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	unsigned int requested_freq;
	int cpu;
	unsigned int enable:1;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
#if ENABLE_BOOST
	unsigned long boost_timeout;
#endif
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, cs_cpu_dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int ignore_nice;
	unsigned int freq_step;
	unsigned int screen_off_max;
	unsigned int change_skips;
#if ENABLE_BOOST
	unsigned int boost_off_max_freq;
	unsigned int boost_max_time;
	unsigned int boost_reset_time;
	unsigned int boost_enable;
#endif
} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.down_threshold = DEF_FREQUENCY_DOWN_THRESHOLD,
	.ignore_nice = 0,
	.freq_step = 5,
	.screen_off_max = DEF_SCREEN_OFF_MAX,
	.change_skips = DEF_CHANGE_SKIPS,
#if ENABLE_BOOST
	.boost_off_max_freq = DEF_BOOST_OFF_MAX_FREQ,
	.boost_max_time = DEF_BOOST_MAX_TIME,
	.boost_reset_time = DEF_BOOST_RESET_TIME,
	.boost_enable = 0,
#endif

};


static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
							cputime64_t *wall)
{
	cputime64_t idle_time;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
	busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
			kstat_cpu(cpu).cpustat.system);

	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

	idle_time = cputime64_sub(cur_wall_time, busy_time);
	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, wall);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);

	return idle_time;
}

/* keep track of frequency transitions */
static int
dbs_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
		     void *data)
{
	struct cpufreq_freqs *freq = data;
	struct cpu_dbs_info_s *this_dbs_info = &per_cpu(cs_cpu_dbs_info,
							freq->cpu);

	struct cpufreq_policy *policy;

	if (!this_dbs_info->enable)
		return 0;

	policy = this_dbs_info->cur_policy;

	/*
	 * we only care if our internally tracked freq moves outside
	 * the 'valid' ranges of freqency available to us otherwise
	 * we do not change it
	*/
	if (this_dbs_info->requested_freq > policy->max
			|| this_dbs_info->requested_freq < policy->min)
		this_dbs_info->requested_freq = freq->new;

	return 0;
}

static struct notifier_block dbs_cpufreq_notifier_block = {
	.notifier_call = dbs_cpufreq_notifier
};

/************************** sysfs interface ************************/
static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);


/* cpufreq_conservative Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(up_threshold, up_threshold);
show_one(down_threshold, down_threshold);
show_one(ignore_nice_load, ignore_nice);
show_one(freq_step, freq_step);
show_one(screen_off_max, screen_off_max);
show_one(change_skips, change_skips);
#if ENABLE_BOOST
show_one(boost_off_max_freq, boost_off_max_freq);
show_one(boost_max_time, boost_max_time);
show_one(boost_reset_time, boost_reset_time);
show_one(boost_enable, boost_enable);
#endif

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.sampling_rate = max(input, min_sampling_rate);
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 ||
			input <= dbs_tuners_ins.down_threshold)
		return -EINVAL;

	dbs_tuners_ins.up_threshold = input;
	return count;
}

static ssize_t store_down_threshold(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 1 otherwise freq will not fall */
	if (ret != 1 || input < 1 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold = input;
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) /* nothing to do */
		return count;

	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(cs_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
	}
	return count;
}

static ssize_t store_freq_step(struct kobject *a, struct attribute *b,
			       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 100)
		input = 100;

	/* no need to test here if freq_step is zero as the user might actually
	 * want this, they would be crazy though :) */
	dbs_tuners_ins.freq_step = input;
	return count;
}

static ssize_t store_screen_off_max(struct kobject *a, struct attribute *b,
                                    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	struct cpu_dbs_info_s *dbs_info = &per_cpu(cs_cpu_dbs_info, 0);
	struct cpufreq_policy *policy = dbs_info->cur_policy;

	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;
	if (policy && (input < policy->min || input > policy->max))
		return -EINVAL;

	dbs_tuners_ins.screen_off_max = input;
	return count;
}

static ssize_t store_change_skips(struct kobject *a, struct attribute *b,
                                    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	dbs_tuners_ins.change_skips = input;
	return count;
}

#if ENABLE_BOOST
static ssize_t store_boost_off_max_freq(struct kobject *a, struct attribute *b,
                                        const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input == 0)
		return -EINVAL;

	mutex_lock(&dbs_mutex);
	dbs_tuners_ins.boost_off_max_freq = input;
	mutex_unlock(&dbs_mutex);

	return count;
}

static ssize_t store_boost_max_time(struct kobject *a, struct attribute *b,
                                    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input == 0)
		return -EINVAL;

	mutex_lock(&dbs_mutex);
	dbs_tuners_ins.boost_max_time = input;
	mutex_unlock(&dbs_mutex);

	return count;
}

static ssize_t store_boost_reset_time(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input == 0)
		return -EINVAL;

	mutex_lock(&dbs_mutex);
	dbs_tuners_ins.boost_reset_time = input;
	mutex_unlock(&dbs_mutex);

	return count;
}

static ssize_t store_boost_enable(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	mutex_lock(&dbs_mutex);
	dbs_tuners_ins.boost_enable = input ? 1 : 0;
	mutex_unlock(&dbs_mutex);

	return count;
}
#endif //ENABLE_BOOST

define_one_global_rw(sampling_rate);
define_one_global_rw(up_threshold);
define_one_global_rw(down_threshold);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(freq_step);
define_one_global_rw(screen_off_max);
define_one_global_rw(change_skips);
#if ENABLE_BOOST
define_one_global_rw(boost_off_max_freq);
define_one_global_rw(boost_max_time);
define_one_global_rw(boost_reset_time);
define_one_global_rw(boost_enable);
#endif

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&down_threshold.attr,
	&ignore_nice_load.attr,
	&freq_step.attr,
	&screen_off_max.attr,
	&change_skips.attr,
#if ENABLE_BOOST
	&boost_off_max_freq.attr,
	&boost_max_time.attr,
	&boost_reset_time.attr,
	&boost_enable.attr,
#endif
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "conservative",
};

/************************** sysfs end ************************/

/* number of samples before enabling early suspend changes (screen off max) */
#define DEF_EARLY_SUSPEND_SAMPLES (10)
static int early_suspend;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend cpufreq_conservative_es;

static void cpufreq_conservative_early_suspend(struct early_suspend *h)
{
	pr_devel("%s\n", __FUNCTION__);
	early_suspend = 1;
}
static void cpufreq_conservative_late_resume(struct early_suspend *h)
{
	pr_devel("%s\n", __FUNCTION__);
	early_suspend = 0;
}
#endif

static void early_suspend_init(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
    pr_devel("%s dbs_enable=%d\n", __FUNCTION__, dbs_enable);
    cpufreq_conservative_es.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
    cpufreq_conservative_es.suspend = cpufreq_conservative_early_suspend;
    cpufreq_conservative_es.resume = cpufreq_conservative_late_resume;
    register_early_suspend(&cpufreq_conservative_es);
#endif
}

static void early_suspend_uninit(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	pr_devel("%s dbs_enable=%d\n", __FUNCTION__, dbs_enable);
	unregister_early_suspend(&cpufreq_conservative_es);
#endif
}

static void cpufreq_conservative_resume(void)
{
	pr_devel("%s\n", __FUNCTION__);
	// reset early suspend delay
	if (early_suspend > 0)
		early_suspend = 1;
}

static struct syscore_ops cpufreq_conservative_syscore_ops = {
    .suspend    = NULL,
    .resume     = cpufreq_conservative_resume,
};

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	unsigned int load = 0;
	unsigned int max_load = 0;
	unsigned int load_long = 0;
	unsigned int max_load_long = 0;
	unsigned int freq_target;

	struct cpufreq_policy *policy;
	unsigned int j;	
#if ENABLE_BOOST
	unsigned long now;
	bool allow_boost;
	bool reset_boost_timeout = false;
#endif

    if (halt) {
        return;
    }

	policy = this_dbs_info->cur_policy;

	if (change_skip_samples != -1 && ++change_skip_samples > dbs_tuners_ins.change_skips)
		change_skip_samples = 0;

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate*sampling_down_factor, we check, if current
	 * idle time is more than 80%, then we try to decrease frequency
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of maximum frequency
	 */

	/* Get Absolute Load */
	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;

		j_dbs_info = &per_cpu(cs_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		if (dbs_tuners_ins.ignore_nice) {
			cputime64_t cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = cputime64_sub(kstat_cpu(j).cpustat.nice,
					 j_dbs_info->prev_cpu_nice);
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

#if USE_LOAD_LONG
		if (change_skip_samples != -1 && change_skip_samples == dbs_tuners_ins.change_skips) {
			unsigned int idle_time_long, wall_time_long;

			wall_time_long = (unsigned int) cputime64_sub(cur_wall_time,
					j_dbs_info->prev_cpu_wall_long);
			j_dbs_info->prev_cpu_wall_long = cur_wall_time;

			idle_time_long = (unsigned int) cputime64_sub(cur_idle_time,
					j_dbs_info->prev_cpu_idle_long);
			j_dbs_info->prev_cpu_idle_long = cur_idle_time;

			if (likely(wall_time_long && wall_time_long >= idle_time_long)) {
				load_long = 100* (wall_time_long - idle_time_long) / wall_time_long;
				if (load_long > max_load_long)
					max_load_long = load_long;
			}
		}
#endif //USE_LOAD_LONG

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;

		if (load > max_load)
			max_load = load;
	}
#if USE_LOAD_LONG
	if (max_load_long == 0)
		max_load_long = max_load;
#endif

	/*
	 * break out if we 'cannot' reduce the speed as the user might
	 * want freq_step to be zero
	 */
	if (dbs_tuners_ins.freq_step == 0)
		return;
		
#if ENABLE_BOOST
	if (dbs_tuners_ins.boost_enable) {
		/* Check if boost allowed */
		now = jiffies;
		if (time_before(now, this_dbs_info->boost_timeout)) {
			// allow boost before boost timeout
			allow_boost = true;
		} else if (time_after(now, this_dbs_info->boost_timeout + msecs_to_jiffies(dbs_tuners_ins.boost_reset_time))) {
			// allow (new) boost if reset time has passed since last boost timeout
			// boost timeout is reset only at the start of a boost period
			allow_boost = true;
			reset_boost_timeout = true;
		} else {
			// disallow boost
			allow_boost = false;
		}
	}
#endif

	if (early_suspend && early_suspend < DEF_EARLY_SUSPEND_SAMPLES) {
		early_suspend++;
	}

	/* check if load > 95 every sample */
	if (change_skip_samples != -1 && change_skip_samples < dbs_tuners_ins.change_skips) {
		if (max_load > 95 && early_suspend < DEF_EARLY_SUSPEND_SAMPLES) {
                if (halt) {
                    return;
                }
				this_dbs_info->requested_freq = policy->max;
				pr_debug("UP %uMHz -> %uMHz max_load=%u\n", 
					policy->cur / 1000, this_dbs_info->requested_freq / 1000, max_load);
				__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
					CPUFREQ_RELATION_H);
		}
		return;
	}

	/* check for normal freq changes every change_skips samples */

	/* Check for frequency increase */
	if (max_load > 95 ||
			max_load_long > dbs_tuners_ins.up_threshold) {

		/* if we are already at full speed then break out early */
#if ENABLE_BOOST
		if (this_dbs_info->requested_freq == policy->max && !dbs_tuners_ins.boost_enable)
			return;
#else
		if (this_dbs_info->requested_freq == policy->max)
			return;
#endif


		if ((max_load > 95 || max_load_long > 95) &&
				early_suspend < DEF_EARLY_SUSPEND_SAMPLES)
			this_dbs_info->requested_freq = policy->max;
		else {
			if (early_suspend < DEF_EARLY_SUSPEND_SAMPLES)
				freq_target = (4 * dbs_tuners_ins.freq_step * policy->max) / 100;
			else
				freq_target = (dbs_tuners_ins.freq_step * policy->max) / 100;

			/* max freq cannot be less than 100. But who knows.... */
			if (unlikely(freq_target == 0))
				freq_target = 5;

			this_dbs_info->requested_freq += freq_target;
			if (this_dbs_info->requested_freq > policy->max)
				this_dbs_info->requested_freq = policy->max;
		}
		if (early_suspend >= DEF_EARLY_SUSPEND_SAMPLES &&
				this_dbs_info->requested_freq > dbs_tuners_ins.screen_off_max &&
				policy->cur != dbs_tuners_ins.screen_off_max) {
			this_dbs_info->requested_freq = dbs_tuners_ins.screen_off_max;
		}
		if (halt) {
			return;
		}
		
#if ENABLE_BOOST
		if (dbs_tuners_ins.boost_enable) {
			if (!allow_boost && this_dbs_info->requested_freq > dbs_tuners_ins.boost_off_max_freq) {
				this_dbs_info->requested_freq = dbs_tuners_ins.boost_off_max_freq;
			}
			if (allow_boost && reset_boost_timeout &&
					this_dbs_info->requested_freq > dbs_tuners_ins.boost_off_max_freq) {
				// this is the start of a boost period.  reset boost timeout.
				this_dbs_info->boost_timeout = now + msecs_to_jiffies(dbs_tuners_ins.boost_max_time);
			}
			if (this_dbs_info->requested_freq > policy->max)
				this_dbs_info->requested_freq = policy->max;
		}
#endif //ENABLE_BOOST

		pr_debug("UP %uMHz -> %uMHz max_load=%u max_load_long=%u\n", policy->cur / 1000, this_dbs_info->requested_freq / 1000, max_load, max_load_long);
		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
			CPUFREQ_RELATION_H);
		return;
	}

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (max_load_long < (dbs_tuners_ins.down_threshold)) {
		this_dbs_info->requested_freq = max_load_long * policy->cur / 70;
		if (this_dbs_info->requested_freq < policy->min)
			this_dbs_info->requested_freq = policy->min;

		/*
		 * if we cannot reduce the frequency anymore, break out early
		 */
		if (policy->cur == policy->min || policy->cur == this_dbs_info->requested_freq)
			return;

		if (halt) {
			return;
		}
		pr_debug("DOWN %uMHz -> %uMHz max_load=%u max_load_long=%u\n", policy->cur / 1000, this_dbs_info->requested_freq / 1000, max_load, max_load_long);
		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_H);
		return;
	}
	pr_devel("max_load_long=%u\n", max_load_long);

#if ENABLE_BOOST
	/* If freq did not need to be changed, still check if freq needs to be lowered
	 * if boost is not allowed at this time.
	 */
	if (dbs_tuners_ins.boost_enable) {
		if (!allow_boost && this_dbs_info->requested_freq > dbs_tuners_ins.boost_off_max_freq) {
			this_dbs_info->requested_freq = dbs_tuners_ins.boost_off_max_freq;
			pr_devel("= b=%d f=%u\n", allow_boost, this_dbs_info->requested_freq);
			__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
					CPUFREQ_RELATION_H);
			return;
		}
	}
#endif //ENABLE_BOOST
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;

	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

	delay -= jiffies % delay;

	mutex_lock(&dbs_info->timer_mutex);

	dbs_check_cpu(dbs_info);

	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	delay -= jiffies % delay;

	dbs_info->enable = 1;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	dbs_info->enable = 0;
	cancel_delayed_work_sync(&dbs_info->work);
}

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;

	this_dbs_info = &per_cpu(cs_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(cs_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice) {
				j_dbs_info->prev_cpu_nice =
						kstat_cpu(j).cpustat.nice;
			}
		}
#if ENABLE_BOOST
		this_dbs_info->boost_timeout = jiffies; /*+ dbs_tuners_ins.boost_max_time;*/
#endif
		this_dbs_info->requested_freq = policy->cur;

		mutex_init(&this_dbs_info->timer_mutex);
		dbs_enable++;
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency, predef_rate;
			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;

			if (dbs_tuners_ins.screen_off_max < policy->min ||
					dbs_tuners_ins.screen_off_max > policy->max) {
				dbs_tuners_ins.screen_off_max = policy->min;
			}

			register_syscore_ops(&cpufreq_conservative_syscore_ops);
			early_suspend_init();

#ifndef CONFIG_NO_HZ
			/*
			 * conservative does not implement micro like ondemand
			 * governor, thus we are bound to jiffes/HZ
			 */
			min_sampling_rate =
				MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
#else
			min_sampling_rate = 10000;
#endif
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			predef_rate = dbs_tuners_ins.sampling_rate;
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			if (predef_rate) {
				// if sampling rate was set at bootup
				dbs_tuners_ins.sampling_rate = min(predef_rate,
					dbs_tuners_ins.sampling_rate);
			}

			cpufreq_register_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);
		}
		mutex_unlock(&dbs_mutex);

		dbs_timer_init(this_dbs_info);

		break;

	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
		dbs_enable--;
		mutex_destroy(&this_dbs_info->timer_mutex);

		/*
		 * Stop the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 0) {
			cpufreq_unregister_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);
			early_suspend_uninit();
			unregister_syscore_ops(&cpufreq_conservative_syscore_ops);
		}

		mutex_unlock(&dbs_mutex);

		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);

		break;
	}
	return 0;
}

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_CONSERVATIVE
static
#endif
struct cpufreq_governor cpufreq_gov_conservative = {
	.name			= "conservative",
	.governor		= cpufreq_governor_dbs,
	.max_transition_latency	= TRANSITION_LATENCY_LIMIT,
	.owner			= THIS_MODULE,
};

static int __init cpufreq_gov_dbs_init(void)
{
	int rc;
    pr_debug("%s\n", __FUNCTION__);
	rc = sysfs_create_group(cpufreq_global_kobject,
	                        &dbs_attr_group);
	rc += cpufreq_register_governor(&cpufreq_gov_conservative);
	if (register_reboot_notifier(&conserv_reboot_notifier) != 0)
		printk("can't register reboot notifier\n");
	return rc;
}

static void __exit cpufreq_gov_dbs_exit(void)
{
    pr_debug("%s\n", __FUNCTION__);
	sysfs_remove_group(cpufreq_global_kobject,
	                   &dbs_attr_group);
	cpufreq_unregister_governor(&cpufreq_gov_conservative);
	(void)unregister_reboot_notifier(&conserv_reboot_notifier);
}


MODULE_AUTHOR("Alexander Clouter <alex@digriz.org.uk>");
MODULE_DESCRIPTION("'cpufreq_conservative' - A dynamic cpufreq governor for "
		"Low Latency Frequency Transition capable processors "
		"optimised for use in a battery environment");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_CONSERVATIVE
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
