/*
 * Common power driver for Amlogic Devices with one or two external
 * power supplies (AC/USB) connected to main and backup batteries,
 * and optional builtin charger.
 *
 * Copyright © 2011-4-27 alex.deng <alex.deng@amlogic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/usb/otg.h>

#include <linux/rfkill.h>

#include <linux/aml_modem.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend modem_early_suspend;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static int aml_modem_earlysuspend(struct early_suspend *handler);
static int aml_modem_earlyresume(struct early_suspend *handler);

#endif

#define POWER_OFF (0)
#define POWER_ON  (1)

#define MODE_SLEEP (1)
#define MODE_WAKE (0)

static struct device *dev;
static struct aml_modem_pdata *g_pData;
static int g_modemPowerMode = MODE_WAKE ; 
static int g_modemPowerState = POWER_OFF ;
static int g_modemEnableFlag = 0 ;

#ifdef CONFIG_HAS_EARLYSUSPEND

static int aml_modem_earlysuspend(struct early_suspend *handler)
{
    //printk("aml_modem_earlysuspend !!!!!!!!!!!!!###################\n");

    #ifdef  CONFIG_AMLOGIC_MODEM_PM
        printk("do aml_modem_earlysuspend \n");
        #ifdef  CONFIG_AMLOGIC_MODEM_PM_OFF_MODE
        g_pData->power_off();
        g_modemPowerState = POWER_OFF;  
        #else
        /* AMLOGIC_MODEM_PM_SLEEP_MODE*/
        g_pData->disable();
        g_modemEnableFlag = 0 ;
        #endif
        g_modemPowerMode = MODE_SLEEP ;
    #endif
    return 0 ;
}

static int aml_modem_earlyresume(struct early_suspend *handler)
{
    //printk("aml_modem_earlyresume !!!!!!!!!!!!!###################\n");
    #ifdef  CONFIG_AMLOGIC_MODEM_PM
        printk("do aml_modem_earlyresume \n");
        #ifdef  CONFIG_AMLOGIC_MODEM_PM_OFF_MODE
        g_pData->power_on();
        g_pData->enable();
        g_modemEnableFlag = 1 ;
        g_modemPowerState = POWER_ON;  
        #else
        /* AMLOGIC_MODEM_PM_SLEEP_MODE*/
        g_pData->enable();
        g_modemEnableFlag = 1 ;
        #endif
    #endif
    g_modemPowerMode = MODE_WAKE;
    return 0 ;
}
#endif

#ifdef CONFIG_PM
int aml_modem_suspend(struct platform_device *pdev, pm_message_t state)
{
    #ifdef  CONFIG_AMLOGIC_MODEM_PM
        printk("do aml_modem_suspend \n");
        #ifdef  CONFIG_AMLOGIC_MODEM_PM_OFF_MODE
        g_pData->power_off();
        g_modemPowerState = POWER_OFF;  
        #else
        /* AMLOGIC_MODEM_PM_SLEEP_MODE*/
        g_pData->disable();
        g_modemEnableFlag = 0 ;
        #endif
    g_modemPowerMode = MODE_SLEEP ;
    #endif
    return 0;
}

int aml_modem_resume(struct platform_device *pdev)
{
    #ifdef  CONFIG_AMLOGIC_MODEM_PM
        printk("do aml_modem_resume \n");
        #ifdef  CONFIG_AMLOGIC_MODEM_PM_OFF_MODE
        g_pData->power_on();
        g_pData->enable();
        g_modemEnableFlag = 1 ;
        g_modemPowerState = POWER_ON;  
        #else
        /* AMLOGIC_MODEM_PM_SLEEP_MODE*/
        g_pData->enable();
        g_modemEnableFlag = 1 ;
        #endif
    g_modemPowerMode = MODE_WAKE ;
    #endif
    return 0 ;
}
#endif

static ssize_t get_modemPowerMode(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", g_modemPowerMode ); 
}

static ssize_t set_modemPowerMode(struct class *cla, struct class_attribute *attr, char *buf, size_t count)
{
    if(!strlen(buf)){
        printk("%s parameter is required!\n",__FUNCTION__);
        return 0;
    }
    g_modemPowerMode = (int)(buf[0]-'0');
    if(g_modemPowerMode <0 || g_modemPowerMode>3){
        printk("%s parameter is invalid! %s \n",__FUNCTION__);
        g_modemPowerMode = 3 ;
    }
        
    return count;
}

static ssize_t get_modemEnableFlag(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", g_modemEnableFlag ); 
}

static ssize_t set_modemEnableFlag(struct class *cla, struct class_attribute *attr, char *buf, size_t count)
{
    if(!strlen(buf)){
        printk("%s parameter is required!\n",__FUNCTION__);
        return 0;
    }
    g_modemEnableFlag = (int)(buf[0]-'0');
    g_modemEnableFlag = !!g_modemEnableFlag ;
    if(g_modemEnableFlag){
        g_pData->enable();
    }
    else{
        g_pData->disable() ;
    }
        
    return count;
}

ssize_t get_modemPower(void)
{
    return g_modemPowerState;
}
EXPORT_SYMBOL(get_modemPower);

static ssize_t get_modemPowerState(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", g_modemPowerState ); 
}

static ssize_t set_modemPowerState(struct class *cla, struct class_attribute *attr, char *buf, size_t count)
{
    int old_state = g_modemPowerState ;
    if(!strlen(buf)){
        printk("%s parameter is required!\n",__FUNCTION__);
        return 0;
    }
    g_modemPowerState = (int)(buf[0]-'0');
    g_modemPowerState = !!g_modemPowerState ;

    if(old_state == g_modemPowerState){
        printk("modemPowerState is already %s now \n", g_modemPowerState?"ON":"OFF");
        return count ;
    }
    else
        printk("set_modemPowerState %s \n", g_modemPowerState?"ON":"OFF");

    if(POWER_ON == g_modemPowerState){
        g_pData->power_on();
        g_pData->enable();
        //g_pData->reset();
    }
    else{
        g_pData->power_off();
        g_pData->disable();
    }
        
    return count;
}


static ssize_t set_modemReset(struct class *cla, struct class_attribute *attr, char *buf, size_t count)
{
    int ret = 0 ;
    if(!strlen(buf)){
        printk("%s parameter is required!\n",__FUNCTION__);
        return 0;
    }
    ret = (int)(buf[0]-'0');
    ret = !!ret ;
    if(ret){
        printk("set_modemReset \n");
        g_pData->reset();
    }

    return count ;
}

static struct class_attribute AmlModem_class_attrs[] = {
    __ATTR(mode,S_IRUGO|S_IWUGO,get_modemPowerMode,set_modemPowerMode),
    __ATTR(enable,S_IRUGO|S_IWUGO,get_modemEnableFlag,set_modemEnableFlag),
    __ATTR(state,S_IRUGO|S_IWUGO,get_modemPowerState,set_modemPowerState),
    __ATTR(reset,S_IWUGO,NULL,set_modemReset),
    __ATTR_NULL
};
static struct class AmlModem_class = {
    .name = "aml_modem",
    .class_attrs = AmlModem_class_attrs,
};

static int __init aml_modem_probe(struct platform_device *pdev)
{
    int ret = 0;

    printk(" aml_modem_probe  \n");

    dev = &pdev->dev;

    if (pdev->id != -1) {
    	dev_err(dev, "it's meaningless to register several "
    		"pda_powers; use id = -1\n");
    	ret = -EINVAL;
    	goto exit;
    }

    g_pData = pdev->dev.platform_data;


    if(g_pData){
        g_modemPowerState = POWER_ON ;
        g_pData->power_on();
        g_pData->enable();
        g_modemEnableFlag =  1;
    }

#ifdef CONFIG_USE_EARLYSUSPEND
    #ifdef CONFIG_HAS_EARLYSUSPEND
        modem_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        modem_early_suspend.suspend = aml_modem_earlysuspend;
        modem_early_suspend.resume = aml_modem_earlyresume;
        modem_early_suspend.param = g_pData;
        register_early_suspend(&modem_early_suspend);
    #endif
#endif    
    return 0 ;

exit:;	
    return ret;
}

static int aml_modem_remove(struct platform_device *pdev)
{
    int ret = 0;

    printk(" aml_modem_remove \n");

    dev = &pdev->dev;

    if (pdev->id != -1) {
        dev_err(dev, "it's meaningless to register several "
        	"pda_powers; use id = -1\n");
        ret = -EINVAL;
        goto exit;
    }

    g_pData = pdev->dev.platform_data;

    if (g_pData->power_off) {
        ret = g_pData->power_off();

    }


    platform_set_drvdata(pdev, NULL);

    #ifdef CONFIG_USE_EARLYSUSPEND
    #ifdef CONFIG_HAS_EARLYSUSPEND
        unregister_early_suspend(&modem_early_suspend);
    #endif
    #endif  
exit:;	
	return ret;
}


MODULE_ALIAS("platform:aml-modem");

static struct platform_driver aml_modem_pdrv = {
    .driver = {
        .name = "aml-modem",
        .owner = THIS_MODULE,	
    },
    .probe = aml_modem_probe,
    .remove = aml_modem_remove,
#ifndef CONFIG_USE_EARLYSUSPEND
    #ifdef CONFIG_PM
    .suspend = aml_modem_suspend,
    .resume = aml_modem_resume,
    #endif
#endif
};

static int __init aml_modem_init(void)
{
	printk("amlogic 3G supply init \n");
       class_register(&AmlModem_class);
	return platform_driver_register(&aml_modem_pdrv);
}

static void __exit aml_modem_exit(void)
{
      platform_driver_unregister(&aml_modem_pdrv);
      class_unregister(&AmlModem_class);
}

module_init(aml_modem_init);
module_exit(aml_modem_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Deng");
