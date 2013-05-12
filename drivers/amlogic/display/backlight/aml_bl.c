/*
 * AMLOGIC backlight driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Wang Han <han.wang@amlogic.com>
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/backlight.h>
#include <linux/slab.h>
#include <linux/aml_bl.h>
#include <mach/power_gate.h>
#ifdef CONFIG_ARCH_MESON6
#include <mach/mod_gate.h>
#endif /* CONFIG_ARCH_MESON6 */

#ifdef CONFIG_AML_BL_LATCH_ON_VSYNC
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <plat/fiq_bridge.h>
#endif

#ifdef MESON_BACKLIGHT_DEBUG
#define DPRINT(...) printk(KERN_INFO __VA_ARGS__)
#define DTRACE()    DPRINT(KERN_INFO "%s()\n", __FUNCTION__)
#else
#define DPRINT(...)
#define DTRACE()
#endif /* MESON_BACKLIGHT_DEBUG */

#ifdef CONFIG_AML_BL_LATCH_ON_VSYNC
static bool bl_pulsed = false;
static int bl_brightness = 0;
static spinlock_t pulse_lock;

static unsigned int bl_update_count = 0;
module_param(bl_update_count, uint, 0644);

static unsigned int bl_vsync_count = 0;
module_param(bl_vsync_count, uint, 0644);
#endif

struct aml_bl {
    const struct aml_bl_platform_data   *pdata;
    struct backlight_device         *bldev;
    struct platform_device          *pdev;
};

static int aml_bl_update_status(struct backlight_device *bd)
{
    struct aml_bl *amlbl = bl_get_data(bd);
    int brightness = bd->props.brightness;
#ifdef CONFIG_ARCH_MESON6
	static int led_pwm_off = 0;
#endif
    DPRINT("%s() brightness=%d\n", __FUNCTION__, brightness);
    DPRINT("%s() pdata->set_bl_level=%p\n", __FUNCTION__, amlbl->pdata->set_bl_level);

/*
    if (bd->props.power != FB_BLANK_UNBLANK)
        brightness = 0;
    if (bd->props.fb_blank != FB_BLANK_UNBLANK)
        brightness = 0;
*/

    if (brightness < 0)
        brightness = 0;
    else if (brightness > 255)
        brightness = 255;

#ifdef CONFIG_ARCH_MESON6
    //static int led_pwm_off = 0;
    if (led_pwm_off && brightness > 0) {
    	switch_mod_gate_by_type(MOD_LED_PWM, 1);
    	led_pwm_off = 0;
    }
#endif /* CONFIG_ARCH_MESON6 */

#ifndef CONFIG_MESON_CS_DCDC_REGULATOR
    if ((brightness > 0) && (0 == IS_CLK_GATE_ON(VGHL_PWM))) {
        CLK_GATE_ON(VGHL_PWM);
        DPRINT("%s() CLK_GATE_ON(VGHL_PWM)\n", __FUNCTION__);
    }
#endif

#ifndef CONFIG_AML_BL_LATCH_ON_VSYNC
    if (amlbl->pdata->set_bl_level)
        amlbl->pdata->set_bl_level(brightness);
#else
    spin_lock_irq(&pulse_lock);
    bl_pulsed = true;
    bl_brightness = brightness;
    bl_update_count++;
    spin_unlock_irq(&pulse_lock);
#endif


#ifndef CONFIG_MESON_CS_DCDC_REGULATOR
    if ((brightness == 0) && (IS_CLK_GATE_ON(VGHL_PWM))) {
        CLK_GATE_OFF(VGHL_PWM);
        DPRINT("%s() CLK_GATE_OFF(VGHL_PWM)\n", __FUNCTION__);
    }
#endif

#ifdef CONFIG_ARCH_MESON6
    if (brightness == 0) {
    	switch_mod_gate_by_type(MOD_LED_PWM, 0);
    	led_pwm_off = 1;
    }
#endif /* CONFIG_ARCH_MESON6 */

    return 0;
}

static int aml_bl_get_brightness(struct backlight_device *bd)
{
    struct aml_bl *amlbl = bl_get_data(bd);

    DPRINT("%s() pdata->get_bl_level=%p\n", __FUNCTION__, amlbl->pdata->get_bl_level);

    if (amlbl->pdata->get_bl_level)
        return amlbl->pdata->get_bl_level();
    else
        return 0;
}

static const struct backlight_ops aml_bl_ops = {
    .get_brightness = aml_bl_get_brightness,
    .update_status  = aml_bl_update_status,
};


#ifdef CONFIG_AML_BL_LATCH_ON_VSYNC

/*
 * vsync fiq handler
 */
static irqreturn_t bl_vsync_isr(int irq, void *dev_id)
{
	struct aml_bl *amlbl = dev_id;
	const struct aml_bl_platform_data *pdata = amlbl->pdata;

	spin_lock_irq(&pulse_lock);
	if (bl_pulsed) {
		if( pdata->set_bl_level )
		    pdata->set_bl_level(bl_brightness);
		bl_pulsed = false;

		bl_vsync_count++;
	}
	spin_unlock_irq(&pulse_lock);

	return IRQ_HANDLED;
}

#endif

static int aml_bl_probe(struct platform_device *pdev)
{
    struct backlight_properties props;
    const struct aml_bl_platform_data *pdata;
    struct backlight_device *bldev;
    struct aml_bl *amlbl;
    int retval;

    DTRACE();

    amlbl = kzalloc(sizeof(struct aml_bl), GFP_KERNEL);
    if (!amlbl)
    {
        printk(KERN_ERR "%s() kzalloc error\n", __FUNCTION__);
        return -ENOMEM;
    }

    amlbl->pdev = pdev;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        printk(KERN_ERR "%s() missing platform data\n", __FUNCTION__);
        retval = -ENODEV;
        goto err;
    }

    amlbl->pdata = pdata;

    DPRINT("%s() pdata->bl_init=%p\n", __FUNCTION__, pdata->bl_init);
    DPRINT("%s() pdata->power_on_bl=%p\n", __FUNCTION__, pdata->power_on_bl);
    DPRINT("%s() pdata->power_off_bl=%p\n", __FUNCTION__, pdata->power_off_bl);
    DPRINT("%s() pdata->set_bl_level=%p\n", __FUNCTION__, pdata->set_bl_level);
    DPRINT("%s() pdata->get_bl_level=%p\n", __FUNCTION__, pdata->get_bl_level);
    DPRINT("%s() pdata->max_brightness=%d\n", __FUNCTION__, pdata->max_brightness);
    DPRINT("%s() pdata->dft_brightness=%d\n", __FUNCTION__, pdata->dft_brightness);

    memset(&props, 0, sizeof(struct backlight_properties));
    props.max_brightness = (pdata->max_brightness > 0 ? pdata->max_brightness : 255);
    props.type = BACKLIGHT_RAW;
    bldev = backlight_device_register("aml-bl", &pdev->dev, amlbl, &aml_bl_ops, &props);
    if (IS_ERR(bldev)) {
        printk(KERN_ERR "failed to register backlight\n");
        retval = PTR_ERR(bldev);
        goto err;
    }

    amlbl->bldev = bldev;

    platform_set_drvdata(pdev, amlbl);

    bldev->props.power = FB_BLANK_UNBLANK;
    bldev->props.brightness = (pdata->dft_brightness > 0 ? pdata->dft_brightness : 200);

#ifdef CONFIG_AML_BL_LATCH_ON_VSYNC
    spin_lock_init(&pulse_lock);
    /* register vsync isr for backlight */
    retval = request_irq(INT_VIU_VSYNC, &bl_vsync_isr, IRQF_SHARED, "bl-vsync", amlbl);
#endif

    if (pdata->bl_init)
        pdata->bl_init();
    if (pdata->power_on_bl)
        pdata->power_on_bl();
    if (pdata->set_bl_level)
        pdata->set_bl_level(bldev->props.brightness);

    return 0;

err:
    kfree(amlbl);
    return retval;
}

static int __exit aml_bl_remove(struct platform_device *pdev)
{
    struct aml_bl *amlbl = platform_get_drvdata(pdev);

    DTRACE();
    backlight_device_unregister(amlbl->bldev);
    platform_set_drvdata(pdev, NULL);
    kfree(amlbl);

    return 0;
}

static struct platform_driver aml_bl_driver = {
    .driver = {
        .name = "aml-bl",
        .owner = THIS_MODULE,
    },
    .probe = aml_bl_probe,
    .remove = __exit_p(aml_bl_remove),
};

static int __init aml_bl_init(void)
{
    DTRACE();
    return platform_driver_register(&aml_bl_driver);
}

static void __exit aml_bl_exit(void)
{
    DTRACE();
    platform_driver_unregister(&aml_bl_driver);
}

module_init(aml_bl_init);
module_exit(aml_bl_exit);

MODULE_DESCRIPTION("Meson Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic, Inc.");
