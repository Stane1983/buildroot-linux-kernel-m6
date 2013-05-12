/*
 * aml_m6_wm8960.c  --  SoC audio for AML M6
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/jack.h>
#include <sound/wm8960.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <linux/switch.h>

#include "../codecs/wm8960.h"
#include "aml_dai.h"
#include "aml_pcm.h"
#include "aml_audio_hw.h"

#define HP_DET                  1
#define WM8960_CAPLESS_MODE
struct wm8960_private_data {
    int bias_level;
    int clock_en;
#if HP_DET
    int timer_en;
    int detect_flag;
    struct timer_list timer;
    struct work_struct work;
    struct mutex lock;
    struct snd_soc_jack jack;
    void* data;
    struct switch_dev sdev; // for android
#endif
};

static struct wm8960_data *wm8960_snd_pdata = NULL;
static struct wm8960_private_data* wm8960_snd_priv = NULL;

static void wm8960_dev_init(void)
{
    if (wm8960_snd_pdata->device_init) {
        wm8960_snd_pdata->device_init();
    }
}

static void wm8960_dev_uninit(void)
{
    if (wm8960_snd_pdata->device_uninit) {
        wm8960_snd_pdata->device_uninit();
    }
}

static void wm8960_set_clock(int enable)
{
    /* set clock gating */
    wm8960_snd_priv->clock_en = enable;

    return ;
}

#if HP_DET
static int wm8960_detect_hp(void)
{
    int flag = -1;

    if (wm8960_snd_pdata->hp_detect)
    {
        flag = wm8960_snd_pdata->hp_detect();
    }

    return flag;
}

static void wm8960_start_timer(unsigned long delay)
{
    wm8960_snd_priv->timer.expires = jiffies + delay;
    wm8960_snd_priv->timer.data = (unsigned long)wm8960_snd_priv;
    wm8960_snd_priv->detect_flag = -1;
    add_timer(&wm8960_snd_priv->timer);
    wm8960_snd_priv->timer_en = 1;
}

static void wm8960_stop_timer(void)
{
    del_timer_sync(&wm8960_snd_priv->timer);
    cancel_work_sync(&wm8960_snd_priv->work);
    wm8960_snd_priv->timer_en = 0;
    wm8960_snd_priv->detect_flag = -1;
}

static void wm8960_work_func(struct work_struct *work)
{
    struct wm8960_private_data *pdata = NULL;
    struct snd_soc_codec *codec = NULL;
    int flag = -1;
	int status = SND_JACK_HEADPHONE;
	int jack_type = 0;

    pdata = container_of(work, struct wm8960_private_data, work);
    codec = (struct snd_soc_codec *)pdata->data;

    flag = wm8960_detect_hp();
    if(pdata->detect_flag != flag) {
        if (flag == 1) {
			snd_soc_update_bits(codec, WM8960_POWER1, 2, 2);
			msleep(100);
			jack_type = wm8960_detect_hp();
			if(jack_type == 1){
				printk("wm8960 3 ports hp pluged\n");
            	/* jack report */
            	snd_soc_jack_report(&pdata->jack, SND_JACK_HEADPHONE, SND_JACK_HEADPHONE);
				switch_set_state(&pdata->sdev, 2);
			}else{
				printk("wm8960 4 ports hs pluged\n");
            	/* jack report */
            	snd_soc_jack_report(&pdata->jack, SND_JACK_HEADSET, SND_JACK_HEADSET);
				switch_set_state(&pdata->sdev, 1);
			}
        } else {
            printk(KERN_INFO "wm8960 hp unpluged\n");
#if 0
            /* Speaker */
            snd_soc_update_bits(codec, WM8960_LOUT2, 0x70, 0x70);
            snd_soc_update_bits(codec, WM8960_ROUT2, 0x70, 0x70);
            /* Headphone */
            snd_soc_update_bits(codec, WM8960_LOUT1, 0x70, 0);
            snd_soc_update_bits(codec, WM8960_ROUT1, 0x70, 0);
 

            /* DAC Mono Mix set */
            snd_soc_update_bits(codec, WM8960_ADDCTL1, 1 << 4, 1 << 4);
#endif         

			switch_set_state(&pdata->sdev, 0);
            /* jack report */
            snd_soc_jack_report(&pdata->jack, 0, SND_JACK_HEADSET);
        }

        pdata->detect_flag = flag;
    }
}


static void wm8960_timer_func(unsigned long data)
{
    struct wm8960_private_data *pdata = (struct wm8960_private_data *)data;
    unsigned long delay = msecs_to_jiffies(200);

    schedule_work(&pdata->work);
    mod_timer(&pdata->timer, jiffies + delay);
}
#endif

static int wm8960_prepare(struct snd_pcm_substream *substream)
{
    printk(KERN_DEBUG "enter %s stream: %s\n", __func__, (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
#if HP_DET
    mutex_lock(&wm8960_snd_priv->lock);
    if (!wm8960_snd_priv->timer_en) {
        wm8960_start_timer(msecs_to_jiffies(100));
    }
    mutex_unlock(&wm8960_snd_priv->lock);
#endif
    return 0;
}

static int wm8960_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret;

    printk(KERN_DEBUG "enter %s rate: %d format: %d\n", __func__, params_rate(params), params_format(params));

    /* set codec DAI configuration */
    ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
        SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai fmt failed!\n", __func__);
        return ret;
    }

    /* set cpu DAI configuration */
    ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
        SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai fmt failed!\n", __func__);
        return ret;
    }
	/*set codec DAI sysclk divider,now 512fs for MCLK,sysclk divide 2  */
	if(MCLKFS_RATIO == 512)
		snd_soc_dai_set_clkdiv(codec_dai,WM8960_SYSCLKDIV,WM8960_SYSCLK_DIV_2);
	else
		snd_soc_dai_set_clkdiv(codec_dai,WM8960_SYSCLKDIV,WM8960_SYSCLK_DIV_1);		
    /* set cpu DAI clock */
    ret = snd_soc_dai_set_sysclk(cpu_dai, 0, params_rate(params) * MCLKFS_RATIO, SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    return 0;
}

static struct snd_soc_ops wm8960_soc_ops = {
    .prepare   = wm8960_prepare,
    .hw_params = wm8960_hw_params,
};

static int wm8960_set_bias_level(struct snd_soc_card *card,
			      enum snd_soc_bias_level level)
{
    int ret = 0;

    printk(KERN_DEBUG "enter %s level: %d\n", __func__, level);

    if (wm8960_snd_priv->bias_level == (int)level)
        return 0;

    switch (level) {
    case SND_SOC_BIAS_ON:
#if HP_DET
        /* start timer */
        mutex_lock(&wm8960_snd_priv->lock);
        if (!wm8960_snd_priv->timer_en) {
            wm8960_start_timer(msecs_to_jiffies(100));
        }
        mutex_unlock(&wm8960_snd_priv->lock);
#endif
        break;
    case SND_SOC_BIAS_PREPARE:
        /* clock enable */
        if (!wm8960_snd_priv->clock_en) {
            wm8960_set_clock(1);
        }
        break;

    case SND_SOC_BIAS_STANDBY:
        /* clock disable */
        if (wm8960_snd_priv->clock_en) {
            wm8960_set_clock(0);
        }
        break;
        
    case SND_SOC_BIAS_OFF:
    	        /* clock disable */
        if (wm8960_snd_priv->clock_en) {
            wm8960_set_clock(0);
        }
#if HP_DET
        /* stop timer */
        mutex_lock(&wm8960_snd_priv->lock);
        if (wm8960_snd_priv->timer_en) {
            wm8960_stop_timer();
        }
        mutex_unlock(&wm8960_snd_priv->lock);
#endif
        break;
    default:
        return ret;
    }

    wm8960_snd_priv->bias_level = (int)level;

    return ret;
}

#ifdef CONFIG_PM_SLEEP
static int wm8960_suspend_pre(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
#if HP_DET
    /* stop timer */
    mutex_lock(&wm8960_snd_priv->lock);
    if (wm8960_snd_priv->timer_en) {
        wm8960_stop_timer();
    }
    mutex_unlock(&wm8960_snd_priv->lock);
#endif
    return 0;
}

static int wm8960_suspend_post(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
    return 0;
}

static int wm8960_resume_pre(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
    return 0;
}

static int wm8960_resume_post(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
    return 0;
}
#else
#define wm8960_suspend_pre  NULL
#define wm8960_suspend_post NULL
#define wm8960_resume_pre   NULL
#define wm8960_resume_post  NULL
#endif

static const struct snd_soc_dapm_widget wm8960_dapm_widgets[] = {
    SND_SOC_DAPM_SPK("Ext Spk", NULL),
    SND_SOC_DAPM_HP("HP", NULL),
    SND_SOC_DAPM_MIC("Mic", NULL),
};

static const struct snd_soc_dapm_route wm8960_dapm_intercon[] = {
    {"Ext Spk", NULL, "SPK_LP"},
    {"Ext Spk", NULL, "SPK_LN"},
    {"Ext Spk", NULL, "SPK_RP"},
    {"Ext Spk", NULL, "SPK_RN"},

    {"HP", NULL, "HP_L"},
    {"HP", NULL, "HP_R"},

    {"MICB", NULL, "Mic"},

    {"LINPUT1", NULL, "MICB"},
    {"LINPUT2", NULL, "MICB"},
	{"RINPUT1", NULL, "MICB"},
    {"RINPUT2", NULL, "MICB"},
};

#if HP_DET
static struct snd_soc_jack_pin jack_pins[] = {
    {
        .pin = "HP",
        .mask = SND_JACK_HEADSET,
    }
};
#endif

static int wm8960_codec_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_codec *codec = rtd->codec;
    //struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dapm_context *dapm = &codec->dapm;
    int ret = 0;

    printk(KERN_DEBUG "enter %s wm8960_snd_pdata: %p\n", __func__, wm8960_snd_pdata);

    /* Add specific widgets */
    snd_soc_dapm_new_controls(dapm, wm8960_dapm_widgets,
                  ARRAY_SIZE(wm8960_dapm_widgets));
    /* Set up specific audio path interconnects */
    snd_soc_dapm_add_routes(dapm, wm8960_dapm_intercon, ARRAY_SIZE(wm8960_dapm_intercon));

    /* set ADCLRC/GPIO1 Pin Function Select */
    snd_soc_update_bits(codec, WM8960_IFACE2, (1 << 6), (1 << 6));

    /* not connected */
    snd_soc_dapm_nc_pin(dapm, "LINPUT3");
    snd_soc_dapm_nc_pin(dapm, "RINPUT3");
    snd_soc_dapm_nc_pin(dapm, "RINPUT2");
    snd_soc_dapm_nc_pin(dapm, "RINPUT1");
    
    snd_soc_dapm_nc_pin(dapm, "OUT3");
    snd_soc_dapm_nc_pin(dapm, "SPK_RP");
    snd_soc_dapm_nc_pin(dapm, "SPK_RN");

    /* always connected */
    snd_soc_dapm_enable_pin(dapm, "Ext Spk");
    snd_soc_dapm_enable_pin(dapm, "Mic");

    /* disable connected */
    snd_soc_dapm_disable_pin(dapm, "HP");

    snd_soc_dapm_sync(dapm);

#if HP_DET
    ret = snd_soc_jack_new(codec, "hp switch", SND_JACK_HEADSET, &wm8960_snd_priv->jack);
    if (ret) {
        printk(KERN_WARNING "Failed to alloc resource for hp switch\n");
    } else {
        ret = snd_soc_jack_add_pins(&wm8960_snd_priv->jack, ARRAY_SIZE(jack_pins), jack_pins);
        if (ret) {
            printk(KERN_WARNING "Failed to setup hp pins\n");
        }
    }
    wm8960_snd_priv->data= (void*)codec;

    init_timer(&wm8960_snd_priv->timer);
    wm8960_snd_priv->timer.function = wm8960_timer_func;
    wm8960_snd_priv->timer.data = (unsigned long)wm8960_snd_priv;

    INIT_WORK(&wm8960_snd_priv->work, wm8960_work_func);
    mutex_init(&wm8960_snd_priv->lock);
#endif

   // wm8960_start_timer(msecs_to_jiffies(100));
    printk("enter %s wm8960_snd_pdata->dis_hp_det=%d: %p\n", __func__, wm8960_snd_pdata->dis_hp_det);
	if(!wm8960_snd_pdata->dis_hp_det){
        //JD2 as headphone detect
        snd_soc_update_bits(codec,27, 0x008, 0x008);// OUT3 buffer Enabled and disabled with HPL and HPR jack detect              
        //INSERT_DELAY    [1] 
        mdelay(1);      
        ///GPIO1 output the "jack detect output"
        snd_soc_update_bits(codec,48, 0x03A, 0x03A);// JD2 used for Jack Detect Input, GPIO function = jack detect output 

		snd_soc_update_bits(codec,24, 0x040, 0x040);// HPDETECT LOW = Speaker 
	}

        snd_soc_update_bits(codec, 23, 0x1D1, 0x1D1);
        mdelay(500);

    return 0;
}

static struct snd_soc_dai_link wm8960_dai_link[] = {
    {
        .name = "WM8960",
        .stream_name = "WM8960 PCM",
        .cpu_dai_name = "aml-dai0",
        .codec_dai_name = "wm8960-hifi",
        .init = wm8960_codec_init,
        .platform_name = "aml-audio.0",
        .codec_name = "wm8960-codec.1-001a",
        .ops = &wm8960_soc_ops,
    },
};

static struct snd_soc_card snd_soc_wm8960 = {
    .name = "AML-WM8960",
    .driver_name = "SOC-Audio",
    .dai_link = &wm8960_dai_link[0],
    .num_links = ARRAY_SIZE(wm8960_dai_link),
    .set_bias_level = wm8960_set_bias_level,
#ifdef CONFIG_PM_SLEEP
    .suspend_pre    = wm8960_suspend_pre,
    .suspend_post   = wm8960_suspend_post,
    .resume_pre     = wm8960_resume_pre,
    .resume_post    = wm8960_resume_post,
#endif
};

static struct platform_device *wm8960_snd_device = NULL;

static int wm8960_audio_probe(struct platform_device *pdev)
{
    int ret = 0;

    printk(KERN_DEBUG "enter %s\n", __func__);

    wm8960_snd_pdata = pdev->dev.platform_data;
    snd_BUG_ON(!wm8960_snd_pdata);

    wm8960_snd_priv = (struct wm8960_private_data*)kzalloc(sizeof(struct wm8960_private_data), GFP_KERNEL);
    if (!wm8960_snd_priv) {
        printk(KERN_ERR "ASoC: Platform driver data allocation failed\n");
        return -ENOMEM;
    }

    wm8960_snd_device = platform_device_alloc("soc-audio", -1);
    if (!wm8960_snd_device) {
        printk(KERN_ERR "ASoC: Platform device allocation failed\n");
        ret = -ENOMEM;
        goto err;
    }

    platform_set_drvdata(wm8960_snd_device, &snd_soc_wm8960);

    ret = platform_device_add(wm8960_snd_device);
    if (ret) {
        printk(KERN_ERR "ASoC: Platform device allocation failed\n");
        goto err_device_add;
    }

    wm8960_snd_priv->bias_level = SND_SOC_BIAS_OFF;
    wm8960_snd_priv->clock_en = 0;

#if HP_DET
    wm8960_snd_priv->sdev.name = "h2w";//for report headphone to android
    ret = switch_dev_register(&wm8960_snd_priv->sdev);
    if (ret < 0){
        printk(KERN_ERR "ASoC: register switch dev failed\n");
        goto err;
    }
#endif
    wm8960_dev_init();

    return ret;

err_device_add:
    platform_device_put(wm8960_snd_device);

err:
    kfree(wm8960_snd_priv);

    return ret;
}

static int wm8960_audio_remove(struct platform_device *pdev)
{
    int ret = 0;

    wm8960_dev_uninit();

    platform_device_put(wm8960_snd_device);
    kfree(wm8960_snd_priv);

    wm8960_snd_device = NULL;
    wm8960_snd_priv = NULL;
    wm8960_snd_pdata = NULL;

    return ret;
}

static struct platform_driver aml_m6_wm8960_driver = {
    .probe  = wm8960_audio_probe,
    .remove = __devexit_p(wm8960_audio_remove),
    .driver = {
        .name = "aml_wm8960_audio",
        .owner = THIS_MODULE,
    },
};

static int __init aml_m6_wm8960_init(void)
{
    return platform_driver_register(&aml_m6_wm8960_driver);
}

static void __exit aml_m6_wm8960_exit(void)
{
    platform_driver_unregister(&aml_m6_wm8960_driver);
}

#ifdef CONFIG_DEFERRED_MODULE_INIT
deferred_module_init(aml_m6_wm8960_init);
#else
module_init(aml_m6_wm8960_init);
#endif
module_exit(aml_m6_wm8960_exit);

/* Module information */
MODULE_AUTHOR("AMLogic, Inc.");
MODULE_DESCRIPTION("AML WM8960 audio driver");
MODULE_LICENSE("GPL");
