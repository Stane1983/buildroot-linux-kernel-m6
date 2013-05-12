#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/sched.h>

extern void do_deferred_initcalls(void);

static int __init load_deferred_module_init(void)
{
	printk("start deferred_module_init\n");
	do_deferred_initcalls();
	printk("endof deferred_module_init\n");
	return 0;
}

static int __exit load_deferred_module_exit(void)
{
	printk("load_deferred_module_exit\n");
	return 0;
}

module_init(load_deferred_module_init);
module_exit(load_deferred_module_exit);

MODULE_LICENSE("GPL");
