/*
 * aml_m_dummy_codec.c  --  SoC audio for AML M series
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
#include <sound/dummy_codec.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include "aml_dai.h"
#include "aml_pcm.h"
#include "aml_audio_hw.h"


static struct dummy_codec_platform_data *dummy_codec_snd_pdata = NULL;

static void dummy_codec_dev_init(void)
{
    if (dummy_codec_snd_pdata->device_init) {
        dummy_codec_snd_pdata->device_init();
    }
}

static void dummy_codec_dev_uninit(void)
{
    if (dummy_codec_snd_pdata->device_uninit) {
        dummy_codec_snd_pdata->device_uninit();
    }
}

static int dummy_codec_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret;

    printk(KERN_DEBUG "enter %s rate: %d format: %d\n", __func__, params_rate(params), params_format(params));

    /* set codec DAI configuration */
    ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai fmt failed!\n", __func__);
        return ret;
    }

    /* set cpu DAI configuration */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	{
    	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	}
	else
    	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai fmt failed!\n", __func__);
        return ret;
    }
#if 0   //no audio in
    /* set codec DAI clock */
    ret = snd_soc_dai_set_sysclk(codec_dai, 0, params_rate(params) * 256, SND_SOC_CLOCK_IN);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }
#endif
    /* set cpu DAI clock */
    ret = snd_soc_dai_set_sysclk(cpu_dai, 0, params_rate(params) * 256, SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    return 0;
}

static struct snd_soc_ops dummy_codec_soc_ops = {
    .hw_params = dummy_codec_hw_params,
};

static int dummy_codec_set_bias_level(struct snd_soc_card *card,
			      enum snd_soc_bias_level level)
{
    return 0;
}

static int dummy_codec_codec_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

static struct snd_soc_dai_link dummy_codec_dai_link[] = {
    {
        .name = "DUMMY_CODEC",
        .stream_name = "DUMMY_CODEC PCM",
        .cpu_dai_name = "aml-dai0",
        .codec_dai_name = "dummy_codec",
        .init = dummy_codec_codec_init,
        .platform_name = "aml-audio.0",
        .codec_name = "dummy_codec.0",
        .ops = &dummy_codec_soc_ops,
    },
};

static struct snd_soc_card snd_soc_dummy_codec = {
    .name = "AML-DUMMY-CODEC",
    .driver_name = "SOC-Audio",
    .dai_link = &dummy_codec_dai_link[0],
    .num_links = ARRAY_SIZE(dummy_codec_dai_link),
    .set_bias_level = dummy_codec_set_bias_level,
};

static struct platform_device *dummy_codec_snd_device = NULL;

static int dummy_codec_audio_probe(struct platform_device *pdev)
{
    int ret = 0;

    //printk(KERN_DEBUG "enter %s\n", __func__);
    printk("enter %s\n", __func__);

    dummy_codec_snd_pdata = pdev->dev.platform_data;
    snd_BUG_ON(!dummy_codec_snd_pdata);
    dummy_codec_snd_device = platform_device_alloc("soc-audio", -1);
    if (!dummy_codec_snd_device) {
        printk(KERN_ERR "ASoC: Platform device allocation failed\n");
        ret = -ENOMEM;
        goto err;
    }

    platform_set_drvdata(dummy_codec_snd_device, &snd_soc_dummy_codec);

    ret = platform_device_add(dummy_codec_snd_device);
    if (ret) {
        printk(KERN_ERR "ASoC: Platform device allocation failed\n");
        goto err_device_add;
    }


    dummy_codec_dev_init();

    return ret;

err_device_add:
    platform_device_put(dummy_codec_snd_device);
err:
    return ret;
}

static int dummy_codec_audio_remove(struct platform_device *pdev)
{
    int ret = 0;

    dummy_codec_dev_uninit();

    platform_device_put(dummy_codec_snd_device);

    dummy_codec_snd_device = NULL;
    dummy_codec_snd_pdata = NULL;

    return ret;
}

static struct platform_driver aml_m_dummy_codec_driver = {
    .probe  = dummy_codec_audio_probe,
    .remove = __devexit_p(dummy_codec_audio_remove),
    .driver = {
        .name = "aml_dummy_codec_audio",
        .owner = THIS_MODULE,
    },
};

static int __init aml_m_dummy_codec_init(void)
{
    return platform_driver_register(&aml_m_dummy_codec_driver);
}

static void __exit aml_m_dummy_codec_exit(void)
{
    platform_driver_unregister(&aml_m_dummy_codec_driver);
}
#ifdef CONFIG_DEFERRED_MODULE_INIT
deferred_module_init(aml_m_dummy_codec_init);
#else
module_init(aml_m_dummy_codec_init);
#endif
module_exit(aml_m_dummy_codec_exit);

/* Module information */
MODULE_AUTHOR("AMLogic, Inc.");
MODULE_DESCRIPTION("AML dummy_codec audio driver");
MODULE_LICENSE("GPL");
