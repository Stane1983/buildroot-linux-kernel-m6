/*
 * aml_m6_rt5631.c  --  SoC audio for AML M6
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
#include <sound/rt5631.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include <linux/switch.h>

#include "../codecs/rt5631.h"
#include "aml_dai.h"
#include "aml_pcm.h"
#include "aml_audio_hw.h"

#define HP_DET                  1

struct rt5631_private_data {
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

static struct rt5631_platform_data *rt5631_snd_pdata = NULL;
static struct rt5631_private_data* rt5631_snd_priv = NULL;

static void rt5631_dev_init(void)
{
    if (rt5631_snd_pdata->device_init) {
        rt5631_snd_pdata->device_init();
    }
}

static void rt5631_dev_uninit(void)
{
    if (rt5631_snd_pdata->device_uninit) {
        rt5631_snd_pdata->device_uninit();
    }
}

static void rt5631_set_clock(int enable)
{
    /* set clock gating */
    rt5631_snd_priv->clock_en = enable;

    return ;
}

#if 0
static void rt5631_set_output(struct snd_soc_codec *codec)
{
    struct snd_soc_dapm_context *dapm = &codec->dapm;

    if (rt5631_snd_pdata->spk_output != RT5631_SPK_STEREO) {
        if (rt5631_snd_pdata->spk_output == RT5631_SPK_RIGHT) {
            snd_soc_dapm_nc_pin(dapm, "SPOL");

            snd_soc_update_bits(codec, RT5631_SPK_MONO_OUT_CTRL,
                0xf000,
                RT5631_M_SPKVOL_L_TO_SPOL_MIXER | RT5631_M_SPKVOL_R_TO_SPOL_MIXER);
        } else {
            snd_soc_dapm_nc_pin(dapm, "SPOR");

            snd_soc_update_bits(codec, RT5631_SPK_MONO_OUT_CTRL,
                0xf000,
                RT5631_M_SPKVOL_L_TO_SPOR_MIXER | RT5631_M_SPKVOL_R_TO_SPOR_MIXER);
        }

        snd_soc_update_bits(codec, RT5631_SPK_MONO_HP_OUT_CTRL,
            RT5631_SPK_L_MUX_SEL_MASK | RT5631_SPK_R_MUX_SEL_MASK | RT5631_HP_L_MUX_SEL_MASK | RT5631_HP_R_MUX_SEL_MASK,
            RT5631_SPK_L_MUX_SEL_SPKMIXER_L | RT5631_SPK_R_MUX_SEL_SPKMIXER_R | RT5631_HP_L_MUX_SEL_HPVOL_L | RT5631_HP_R_MUX_SEL_HPVOL_R);
    } else {
        snd_soc_update_bits(codec, RT5631_SPK_MONO_OUT_CTRL,
            0xf000,
            RT5631_M_SPKVOL_R_TO_SPOL_MIXER | RT5631_M_SPKVOL_L_TO_SPOR_MIXER);

        snd_soc_update_bits(codec, RT5631_SPK_MONO_HP_OUT_CTRL,
            RT5631_SPK_L_MUX_SEL_MASK | RT5631_SPK_R_MUX_SEL_MASK | RT5631_HP_L_MUX_SEL_MASK | RT5631_HP_R_MUX_SEL_MASK,
            RT5631_SPK_L_MUX_SEL_SPKMIXER_L | RT5631_SPK_R_MUX_SEL_SPKMIXER_R | RT5631_HP_L_MUX_SEL_HPVOL_L | RT5631_HP_R_MUX_SEL_HPVOL_R);
    }
}

static void rt5631_set_input(struct snd_soc_codec *codec)
{
    if (rt5631_snd_pdata->mic_input == RT5631_MIC_SINGLEENDED) {
        /* single-ended input mode */
        snd_soc_update_bits(codec, RT5631_MIC_CTRL_1,
            RT5631_MIC1_DIFF_INPUT_CTRL,
            0);
    } else {
        /* differential input mode */
        snd_soc_update_bits(codec, RT5631_MIC_CTRL_1,
            RT5631_MIC1_DIFF_INPUT_CTRL,
            RT5631_MIC1_DIFF_INPUT_CTRL);
    }
}

#endif
#if HP_DET
static int rt5631_detect_hp(void)
{
    int flag = -1;

    if (rt5631_snd_pdata->hp_detect)
    {
        flag = rt5631_snd_pdata->hp_detect();
    }

    return flag;
}

static void rt5631_start_timer(unsigned long delay)
{
    rt5631_snd_priv->timer.expires = jiffies + delay;
    rt5631_snd_priv->timer.data = (unsigned long)rt5631_snd_priv;
    rt5631_snd_priv->detect_flag = -1;
    add_timer(&rt5631_snd_priv->timer);
    rt5631_snd_priv->timer_en = 1;
}

static void rt5631_stop_timer(void)
{
    del_timer_sync(&rt5631_snd_priv->timer);
    cancel_work_sync(&rt5631_snd_priv->work);
    rt5631_snd_priv->timer_en = 0;
    rt5631_snd_priv->detect_flag = -1;
}

static void rt5631_work_func(struct work_struct *work)
{
    struct rt5631_private_data *pdata = NULL;
    struct snd_soc_codec *codec = NULL;
    int jack_type = 0;
    int flag = -1;
	int status = SND_JACK_HEADPHONE;

    pdata = container_of(work, struct rt5631_private_data, work);
    codec = (struct snd_soc_codec *)pdata->data;

    flag = rt5631_detect_hp();
    if(pdata->detect_flag != flag) {
        if (flag == 1) {
            if ((&pdata->sdev!=NULL)&&(pdata->sdev.dev!=NULL))
				switch_set_state(&pdata->sdev, 2);  // 1 :have mic  2: no mic
            else
            	printk(KERN_INFO "rt5631 kernel NULL pointer error\n");
            jack_type = rt5631_headset_detect(codec, 1);
            printk(KERN_INFO "rt5631 hp pluged jack_type: %d\n", jack_type);
            snd_soc_jack_report(&pdata->jack, status, SND_JACK_HEADPHONE);
        } else {
            printk(KERN_INFO "rt5631 hp unpluged\n");
            if ((&pdata->sdev!=NULL)&&(pdata->sdev.dev!=NULL))
            	switch_set_state(&pdata->sdev, 0);
            else
            	printk(KERN_INFO "rt5631 kernel NULL pointer error\n");
            rt5631_headset_detect(codec, 0);
            snd_soc_jack_report(&pdata->jack, 0, SND_JACK_HEADPHONE);
        }

        pdata->detect_flag = flag;
    }
}


static void rt5631_timer_func(unsigned long data)
{
    struct rt5631_private_data *pdata = (struct rt5631_private_data *)data;
    unsigned long delay = msecs_to_jiffies(200);

    schedule_work(&pdata->work);
    mod_timer(&pdata->timer, jiffies + delay);
}
#endif

static int rt5631_prepare(struct snd_pcm_substream *substream)
{
    printk(KERN_DEBUG "enter %s stream: %s\n", __func__, (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
#if HP_DET
    mutex_lock(&rt5631_snd_priv->lock);
    if (!rt5631_snd_priv->timer_en) {
        rt5631_start_timer(msecs_to_jiffies(100));
    }
    mutex_unlock(&rt5631_snd_priv->lock);
#endif
    return 0;
}

static int rt5631_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret;

    printk(KERN_DEBUG "enter %s stream: %s rate: %d format: %d\n", __func__, (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture", params_rate(params), params_format(params));

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

    /* set codec DAI clock */
    ret = snd_soc_dai_set_sysclk(codec_dai, 0, params_rate(params) * MCLKFS_RATIO, SND_SOC_CLOCK_IN);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    /* set cpu DAI clock */
    ret = snd_soc_dai_set_sysclk(cpu_dai, 0, params_rate(params) * MCLKFS_RATIO, SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    return 0;
}

static struct snd_soc_ops rt5631_soc_ops = {
    .prepare   = rt5631_prepare,
    .hw_params = rt5631_hw_params,
};

static int rt5631_set_bias_level(struct snd_soc_card *card,
			      enum snd_soc_bias_level level)
{
    int ret = 0;

    printk(KERN_DEBUG "enter %s level: %d\n", __func__, level);

    if (rt5631_snd_priv->bias_level == (int)level)
        return 0;

    switch (level) {
    case SND_SOC_BIAS_ON:
#if HP_DET
        mutex_lock(&rt5631_snd_priv->lock);
        if (!rt5631_snd_priv->timer_en) {
            rt5631_start_timer(msecs_to_jiffies(100));
        }
        mutex_unlock(&rt5631_snd_priv->lock);
#endif
        break;
    case SND_SOC_BIAS_PREPARE:
        /* clock enable */
        if (!rt5631_snd_priv->clock_en) {
            rt5631_set_clock(1);
        }
        break;

    case SND_SOC_BIAS_OFF:
        if (rt5631_snd_priv->clock_en) {
            rt5631_set_clock(0);
        }
#if HP_DET
        /* stop timer */
        mutex_lock(&rt5631_snd_priv->lock);
        if (rt5631_snd_priv->timer_en) {
            rt5631_stop_timer();
        }
        mutex_unlock(&rt5631_snd_priv->lock);
#endif
        break;
    case SND_SOC_BIAS_STANDBY:
        /* clock disable */
        if (rt5631_snd_priv->clock_en) {
            rt5631_set_clock(0);
        }
#if HP_DET
        /* stop timer */
        mutex_lock(&rt5631_snd_priv->lock);
        if (rt5631_snd_priv->timer_en) {
           // rt5631_stop_timer();
        }
        mutex_unlock(&rt5631_snd_priv->lock);
#endif
        break;
    default:
        return ret;
    }

    rt5631_snd_priv->bias_level = (int)level;

    return ret;
}

#ifdef CONFIG_PM_SLEEP
static int rt5631_suspend_pre(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
#if HP_DET
    /* stop timer */
    mutex_lock(&rt5631_snd_priv->lock);
    if (rt5631_snd_priv->timer_en) {
       // rt5631_stop_timer();
    }
    mutex_unlock(&rt5631_snd_priv->lock);
#endif
    return 0;
}

static int rt5631_suspend_post(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
    return 0;
}

static int rt5631_resume_pre(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
    return 0;
}

static int rt5631_resume_post(struct snd_soc_card *card)
{
    printk(KERN_DEBUG "enter %s\n", __func__);
    return 0;
}
#else
#define rt5631_suspend_pre  NULL
#define rt5631_suspend_post NULL
#define rt5631_resume_pre   NULL
#define rt5631_resume_post  NULL
#endif

static const struct snd_soc_dapm_widget rt5631_dapm_widgets[] = {
    SND_SOC_DAPM_SPK("Ext Spk", NULL),
    SND_SOC_DAPM_HP("HP", NULL),
    SND_SOC_DAPM_MIC("MAIN MIC", NULL),
};

static const struct snd_soc_dapm_route rt5631_dapm_intercon[] = {
    {"Ext Spk", NULL, "SPOL"},
    {"Ext Spk", NULL, "SPOR"},

    {"HP", NULL, "HPOL"},
    {"HP", NULL, "HPOR"},
	{"MIC1", NULL, "MIC Bias1"},
    //{"MIC2", NULL, "MAIN MIC"},
    {"MIC Bias1", NULL, "MAIN MIC"},
    //{"MIC Bias2", NULL, "MAIN MIC"},
};

#if HP_DET
static struct snd_soc_jack_pin jack_pins[] = {
    {
        .pin = "HP",
        .mask = SND_JACK_HEADPHONE,
    }
};
#endif

static int rt5631_codec_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_codec *codec = rtd->codec;
    //struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dapm_context *dapm = &codec->dapm;
    int ret = 0;

    printk(KERN_DEBUG "enter %s rt5631_snd_pdata: %p\n", __func__, rt5631_snd_pdata);

    /* Add specific widgets */
    snd_soc_dapm_new_controls(dapm, rt5631_dapm_widgets,
                  ARRAY_SIZE(rt5631_dapm_widgets));
    /* Set up specific audio path interconnects */
    snd_soc_dapm_add_routes(dapm, rt5631_dapm_intercon, ARRAY_SIZE(rt5631_dapm_intercon));

    /* Setup spk/hp/mono output */
  //  rt5631_set_output(codec);

    /* Setuo mic input */
  //  rt5631_set_input(codec);

    /* not connected */
    snd_soc_dapm_nc_pin(dapm, "MONO");
    snd_soc_dapm_nc_pin(dapm, "AUXO2");

    snd_soc_dapm_nc_pin(dapm, "DMIC");
    snd_soc_dapm_nc_pin(dapm, "AXIL");
    snd_soc_dapm_nc_pin(dapm, "AXIR");

    /* always connected */
    snd_soc_dapm_enable_pin(dapm, "Ext Spk");
	snd_soc_dapm_enable_pin(dapm, "MAIN MIC");

    /* disable connected */
    snd_soc_dapm_disable_pin(dapm, "HP");

    snd_soc_dapm_sync(dapm);

#if HP_DET
    ret = snd_soc_jack_new(codec, "hp switch", SND_JACK_HEADPHONE, &rt5631_snd_priv->jack);
    if (ret) {
        printk(KERN_WARNING "Failed to alloc resource for hp switch\n");
    } else {
        ret = snd_soc_jack_add_pins(&rt5631_snd_priv->jack, ARRAY_SIZE(jack_pins), jack_pins);
        if (ret) {
            printk(KERN_WARNING "Failed to setup hp pins\n");
        }
    }
    rt5631_snd_priv->data= (void*)codec;

    init_timer(&rt5631_snd_priv->timer);
    rt5631_snd_priv->timer.function = rt5631_timer_func;
    rt5631_snd_priv->timer.data = (unsigned long)rt5631_snd_priv;

    INIT_WORK(&rt5631_snd_priv->work, rt5631_work_func);
    mutex_init(&rt5631_snd_priv->lock);
#endif

    return 0;
}

static struct snd_soc_dai_link rt5631_dai_link[] = {
    {
        .name = "RT5631",
        .stream_name = "RT5631 PCM",
        .cpu_dai_name = "aml-dai0",
        .codec_dai_name = "rt5631-hifi",
        .init = rt5631_codec_init,
        .platform_name = "aml-audio.0",
        .codec_name = "rt5631.1-001a",
        .ops = &rt5631_soc_ops,
    },
};

static struct snd_soc_card snd_soc_rt5631 = {
    .name = "AML-RT5631",
    .driver_name = "SOC-Audio",
    .dai_link = &rt5631_dai_link[0],
    .num_links = ARRAY_SIZE(rt5631_dai_link),
    .set_bias_level = rt5631_set_bias_level,
#ifdef CONFIG_PM_SLEEP
	.suspend_pre    = rt5631_suspend_pre,
	.suspend_post   = rt5631_suspend_post,
	.resume_pre     = rt5631_resume_pre,
	.resume_post    = rt5631_resume_post,
#endif
};

static struct platform_device *rt5631_snd_device = NULL;

static int rt5631_audio_probe(struct platform_device *pdev)
{
    int ret = 0;

    printk(KERN_DEBUG "enter %s\n", __func__);

    rt5631_snd_pdata = pdev->dev.platform_data;
    snd_BUG_ON(!rt5631_snd_pdata);

    rt5631_snd_priv = (struct rt5631_private_data*)kzalloc(sizeof(struct rt5631_private_data), GFP_KERNEL);
    if (!rt5631_snd_priv) {
        printk(KERN_ERR "ASoC: Platform driver data allocation failed\n");
        return -ENOMEM;
    }

    rt5631_snd_device = platform_device_alloc("soc-audio", -1);
    if (!rt5631_snd_device) {
        printk(KERN_ERR "ASoC: Platform device allocation failed\n");
        ret = -ENOMEM;
        goto err;
    }

    platform_set_drvdata(rt5631_snd_device, &snd_soc_rt5631);
	rt5631_snd_device->dev.platform_data = rt5631_snd_pdata;

    ret = platform_device_add(rt5631_snd_device);
    if (ret) {
        printk(KERN_ERR "ASoC: Platform device allocation failed\n");
        goto err_device_add;
    }

    rt5631_snd_priv->bias_level = SND_SOC_BIAS_OFF;
    rt5631_snd_priv->clock_en = 0;

#if HP_DET
    rt5631_snd_priv->sdev.name = "h2w";//for report headphone to android
    ret = switch_dev_register(&rt5631_snd_priv->sdev);
    if (ret < 0){
            printk(KERN_ERR "ASoC: register switch dev failed\n");
            goto err;
    }
#endif
    rt5631_dev_init();

    return ret;

err_device_add:
    platform_device_put(rt5631_snd_device);

err:
    kfree(rt5631_snd_priv);

    return ret;
}

static int rt5631_audio_remove(struct platform_device *pdev)
{
    int ret = 0;

    rt5631_dev_uninit();

    platform_device_put(rt5631_snd_device);
    kfree(rt5631_snd_priv);

    rt5631_snd_device = NULL;
    rt5631_snd_priv = NULL;
    rt5631_snd_pdata = NULL;

    return ret;
}

static struct platform_driver aml_m6_rt5631_driver = {
    .probe  = rt5631_audio_probe,
    .remove = __devexit_p(rt5631_audio_remove),
    .driver = {
        .name = "aml_rt5631_audio",
        .owner = THIS_MODULE,
    },
};

static int __init aml_m6_rt5631_init(void)
{
    return platform_driver_register(&aml_m6_rt5631_driver);
}

static void __exit aml_m6_rt5631_exit(void)
{
    platform_driver_unregister(&aml_m6_rt5631_driver);
}

#ifdef CONFIG_DEFERRED_MODULE_INIT
deferred_module_init(aml_m6_rt5631_init);
#else
module_init(aml_m6_rt5631_init);
#endif
module_exit(aml_m6_rt5631_exit);

/* Module information */
MODULE_AUTHOR("AMLogic, Inc.");
MODULE_DESCRIPTION("AML RT5631 audio driver");
MODULE_LICENSE("GPL");
