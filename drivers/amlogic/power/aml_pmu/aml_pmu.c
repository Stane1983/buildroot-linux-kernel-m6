#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <mach/am_regs.h>
#include <mach/gpio.h>
#include <amlogic/aml_pmu.h>

#ifdef CONFIG_AML1212
#include <amlogic/aml1212.h>
#endif

static const struct i2c_device_id aml_pmu_id_table[] = {
	{ "aml1212", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, aml_pmu_id_table);

static int __devinit aml_pmu_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
    struct platform_device *pdev;
	int ret;
    
    /*
     * allocate and regist AML1212 devices, then kernel will probe driver for AML1212
     */
#ifdef CONFIG_AML1212
    pdev = platform_device_alloc(AML1212_SUPPLY_NAME, AML1212_SUPPLY_ID);
    if (pdev == NULL) {
        printk(">> %s, allocate platform device failed\n", __func__);
        return -ENOMEM;
    }
    pdev->dev.parent        = &client->dev;
    pdev->dev.platform_data =  client->dev.platform_data;
    ret = platform_device_add(pdev);
    if (ret) {
        printk(">> %s, add platform device failed\n", __func__);
        platform_device_del(pdev);
        return -EINVAL;
    }
    i2c_set_clientdata(client, pdev); 
#endif

	return 0;
}

static int __devexit aml_pmu_remove(struct i2c_client *client)
{
    struct platform_device *pdev = i2c_get_clientdata(client);

    platform_device_del(pdev);

	return 0;
}

static struct i2c_driver aml_pmu_driver = {
	.driver	= {
		.name	= "amlogic_pmu",
		.owner	= THIS_MODULE,
	},
	.probe		= aml_pmu_probe,
	.remove		= __devexit_p(aml_pmu_remove),
	.id_table	= aml_pmu_id_table,
};

static int __init aml_pmu_init(void)
{
	return i2c_add_driver(&aml_pmu_driver);
}
arch_initcall(aml_pmu_init);

static void __exit aml_pmu_exit(void)
{
	i2c_del_driver(&aml_pmu_driver);
}
module_exit(aml_pmu_exit);

MODULE_DESCRIPTION("Amlogic PMU device driver");
MODULE_AUTHOR("tao.zeng@amlogic.com");
MODULE_LICENSE("GPL");
