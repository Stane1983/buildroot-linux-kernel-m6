#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <plat/hdmi_config.h>

#define DEVICE_NAME     "amhdmitx"

static struct platform_device* amhdmi_tx_device = NULL;

static int __init amhdmi_tx_device_init(void)
{
    amhdmi_tx_device = platform_device_alloc(DEVICE_NAME,0);
    
    if (!amhdmi_tx_device) {
	printk(KERN_ERR "HDMI: " "failed to alloc amhdmi_tx_device\n");
        return -ENOMEM;
    }
    
    if(platform_device_add(amhdmi_tx_device)){
        platform_device_put(amhdmi_tx_device);
        printk(KERN_ERR "HDMI: ""failed to add amhdmi_tx_device\n");
	return -ENODEV;
    }
    
    return 0;
}

core_initcall(amhdmi_tx_device_init);

int setup_hdmi_dev_platdata(void* platform_data)
{
    if(amhdmi_tx_device){
        platform_device_add_data(amhdmi_tx_device, platform_data, sizeof(struct hdmi_config_platform_data));
        return 1;
    }
    
    return 0;
}
EXPORT_SYMBOL(setup_hdmi_dev_platdata);
