/*
	amlogic  M6TV sound card machine   driver code.
	it support multi-codec on board, one codec as the main codec,others as
	aux devices.
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

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include <linux/switch.h>
#include "aml_dai.h"
#include "aml_pcm.h"
#include "aml_audio_hw.h"
#include <sound/aml_m6tv_audio.h>

static struct platform_device *m6tv_audio_snd_device = NULL;
static struct m6tv_audio_codec_platform_data *m6tv_audio_snd_pdata = NULL;
//static struct m6tv_audio_private_data* m6tv_audio_snd_priv = NULL;
#define CODEC_DEBUG  printk
static void m6tv_audio_dev_init(void)
{
    if (m6tv_audio_snd_pdata->device_init) {
        m6tv_audio_snd_pdata->device_init();
    }
}

static void m6tv_audio_dev_uninit(void)
{
    if (m6tv_audio_snd_pdata->device_uninit) {
        m6tv_audio_snd_pdata->device_uninit();
    }
}
static int m6tv_audio_prepare(struct snd_pcm_substream *substream)
{
    CODEC_DEBUG( "enter %s stream: %s\n", __func__, (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
    return 0;
}
static int m6tv_audio_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret;
    CODEC_DEBUG( "enter %s stream: %s rate: %d format: %d\n", __func__, (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture", params_rate(params), params_format(params));

    /* set codec DAI configuration */
    ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
        SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0) {
        CODEC_DEBUG(KERN_ERR "%s: set codec dai fmt failed!\n", __func__);
        return ret;
    }

    /* set cpu DAI configuration */
    ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
        SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
    if (ret < 0) {
        CODEC_DEBUG(KERN_ERR "%s: set cpu dai fmt failed!\n", __func__);
        return ret;
    }

    /* set codec DAI clock */
    ret = snd_soc_dai_set_sysclk(codec_dai, 0, params_rate(params) * MCLKFS_RATIO, SND_SOC_CLOCK_IN);
    if (ret < 0) {
        CODEC_DEBUG(KERN_ERR "%s: set codec dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    /* set cpu DAI clock */
    ret = snd_soc_dai_set_sysclk(cpu_dai, 0, params_rate(params) * MCLKFS_RATIO, SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        CODEC_DEBUG(KERN_ERR "%s: set cpu dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    return 0;
}
static struct snd_soc_ops m6tv_audio_soc_ops = {
    .prepare   = m6tv_audio_prepare,
    .hw_params = m6tv_audio_hw_params,
};	


static int m6tv_audio_set_bias_level(struct snd_soc_card *card,
					enum snd_soc_bias_level level)
{
	int ret = 0;
    	CODEC_DEBUG( "enter %s level: %d\n", __func__, level);
	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int m6tv_audio_suspend_pre(struct snd_soc_card *card)
{
    CODEC_DEBUG( "enter %s\n", __func__);
    return 0;
}

static int m6tv_audio_suspend_post(struct snd_soc_card *card)
{
    CODEC_DEBUG( "enter %s\n", __func__);
    return 0;
}

static int m6tv_audio_resume_pre(struct snd_soc_card *card)
{
    CODEC_DEBUG( "enter %s\n", __func__);
    return 0;
}

static int m6tv_audio_resume_post(struct snd_soc_card *card)
{
    CODEC_DEBUG( "enter %s\n", __func__);
    return 0;
}
#else
#define m6tv_audio_suspend_pre  NULL
#define m6tv_audio_suspend_post NULL
#define m6tv_audio_resume_pre   NULL
#define m6tv_audio_resume_post  NULL
#endif

static int m6tv_audio_codec_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_codec *codec = rtd->codec;
    struct snd_soc_dapm_context *dapm = &codec->dapm;
    int ret = 0;

    CODEC_DEBUG( "enter %s\n", __func__);

  
    return 0;
}
#ifdef CONFIG_SND_AML_M6TV_STA380
static int m6tv_sta381xx_init(struct snd_soc_dapm_context *dapm)
{
	CODEC_DEBUG("~~~~%s\n", __func__);

	snd_soc_codec_set_sysclk(dapm->codec, 1, 48000 * MCLKFS_RATIO, 0);
	return 0;
}
#endif
#ifdef CONFIG_SND_AML_M6TV_TAS5711
static int m6tv_tas5711_init(struct snd_soc_dapm_context *dapm)
{
	struct snd_soc_codec *codec = dapm->codec;
	CODEC_DEBUG("~~~~%s\n", __func__);
	snd_soc_codec_set_sysclk(dapm->codec, 1, 48000 * MCLKFS_RATIO, 0);
	return 0;
}
#endif

#ifdef CONFIG_SND_AML_M6TV_TAS5707
static int m6tv_tas5707_init(struct snd_soc_dapm_context *dapm)
{
	struct snd_soc_codec *codec = dapm->codec;
	CODEC_DEBUG("~~~~%s\n", __func__);
	snd_soc_codec_set_sysclk(dapm->codec, 1, 48000 * MCLKFS_RATIO, 0);
	return 0;
}
#endif


static struct snd_soc_dai_link m6tv_audio_dai_link[] = {
#ifdef CONFIG_SND_AML_M6TV_SYNOPSYS9629_CODEC
    {
        .name = "syno9629",
        .stream_name = "SYNO9629 PCM",
        .cpu_dai_name = "aml-dai0",
        .codec_dai_name = "syno9629-hifi",
        .init = m6tv_audio_codec_init,
        .platform_name = "aml-audio.0",
        .codec_name = "syno9629.0",
        .ops = &m6tv_audio_soc_ops,
    },
#endif    
};
struct snd_soc_aux_dev m6tv_audio_aux_dev[] = {
#ifdef CONFIG_SND_AML_M6TV_RT5631	
	{
		.name = "rt5631",
		.codec_name = "rt5631.0-001a",
		.init = NULL,
	},
#endif	
#ifdef CONFIG_SND_AML_M6TV_STA380
	{
		.name = "sta381xx",
		.codec_name = "sta381xx.0-001c",
		.init = m6tv_sta381xx_init,
	},
#endif
#ifdef CONFIG_SND_AML_M6TV_TAS5711
	{
		.name = "tas5711",
		.codec_name = "tas5711.0-001b",
		.init = m6tv_tas5711_init,
	},
#endif
#ifdef CONFIG_SND_AML_M6TV_TAS5707
	{
		.name = "tas5707",
		.codec_name = "tas5707.0-001b",
		.init = m6tv_tas5707_init,
	},
#endif

};

static struct snd_soc_codec_conf m6tv_audio_codec_conf[] = {
#ifdef CONFIG_SND_AML_M6TV_RT5631	
	
	{
		.dev_name = "rt5631.0-001a",
		.name_prefix = "b",
	},
#endif
#ifdef CONFIG_SND_AML_M6TV_STA380	
	{
		.dev_name = "sta381xx.0-001c",
		.name_prefix = "AMP",
	},
#endif
#ifdef CONFIG_SND_AML_M6TV_TAS5711	
	{
		.dev_name = "tas5711.0-001b",
		.name_prefix = "AMP",
	},
#endif
#ifdef CONFIG_SND_AML_M6TV_TAS5707
	{
		.dev_name = "tas5707.0-001b",
		.name_prefix = "AMP",
	},
#endif

};
static struct snd_soc_card snd_soc_m6tv_audio = {
    .name = "AML-M6TV",
    .driver_name = "SOC-Audio",
    .dai_link = m6tv_audio_dai_link,
    .num_links = ARRAY_SIZE(m6tv_audio_dai_link),
    .set_bias_level = m6tv_audio_set_bias_level,
    .aux_dev = m6tv_audio_aux_dev,
    .num_aux_devs = ARRAY_SIZE(m6tv_audio_aux_dev),
    .codec_conf = m6tv_audio_codec_conf,
    .num_configs = ARRAY_SIZE(m6tv_audio_codec_conf),
    
#ifdef CONFIG_PM_SLEEP
	.suspend_pre    = m6tv_audio_suspend_pre,
	.suspend_post   = m6tv_audio_suspend_post,
	.resume_pre     = m6tv_audio_resume_pre,
	.resume_post    = m6tv_audio_resume_post,
#endif
};






static int m6tv_audio_audio_probe(struct platform_device *pdev)
{
	int ret = 0;

	CODEC_DEBUG( "enter %s\n", __func__);

	m6tv_audio_snd_pdata = pdev->dev.platform_data;
	snd_BUG_ON(!m6tv_audio_snd_pdata);
#if 0
	m6tv_audio_snd_priv = (struct m6tv_audio_private_data*)kzalloc(sizeof(struct m6tv_audio_private_data), GFP_KERNEL);
	if (!m6tv_audio_snd_priv) {
		CODEC_DEBUG(KERN_ERR "ASoC: Platform driver data allocation failed\n");
		return -ENOMEM;
	}
#endif
	m6tv_audio_snd_device = platform_device_alloc("soc-audio", -1);
	if (!m6tv_audio_snd_device) {
		CODEC_DEBUG(KERN_ERR "ASoC: Platform device allocation failed\n");
		ret = -ENOMEM;
		goto err;
	}

	platform_set_drvdata(m6tv_audio_snd_device, &snd_soc_m6tv_audio);
	m6tv_audio_snd_device->dev.platform_data = m6tv_audio_snd_pdata;

	ret = platform_device_add(m6tv_audio_snd_device);
	if (ret) {
		CODEC_DEBUG(KERN_ERR "ASoC: Platform device allocation failed\n");
		goto err_device_add;
	}

//	m6tv_audio_snd_priv->bias_level = SND_SOC_BIAS_OFF;
//	m6tv_audio_snd_priv->clock_en = 0;

	m6tv_audio_dev_init();
	return ret;
err_device_add:
	platform_device_put(m6tv_audio_snd_device);

err:
//	kfree(m6tv_audio_snd_priv);

	return ret;
}

static int m6tv_audio_audio_remove(struct platform_device *pdev)
{
    int ret = 0;
    m6tv_audio_dev_uninit();
    platform_device_put(m6tv_audio_snd_device);
//    kfree(m6tv_audio_snd_priv);
    m6tv_audio_snd_device = NULL;
//    m6tv_audio_snd_priv = NULL;
    m6tv_audio_snd_pdata = NULL;
    return ret;
}

static struct platform_driver aml_m6tv_audio_driver = {
    .probe  = m6tv_audio_audio_probe,
    .remove = __devexit_p(m6tv_audio_audio_remove),
    .driver = {
        .name = "aml_m6tv_audio",
        .owner = THIS_MODULE,
    },
};

static int __init aml_m6tv_audio_init(void)
{
	CODEC_DEBUG( "enter %s\n", __func__);
	return platform_driver_register(&aml_m6tv_audio_driver);
}

static void __exit aml_m6tv_audio_exit(void)
{
    platform_driver_unregister(&aml_m6tv_audio_driver);
}

module_init(aml_m6tv_audio_init);
module_exit(aml_m6tv_audio_exit);

/* Module information */
MODULE_AUTHOR("jian.xu@amlogic.com AMLogic, Inc.");
MODULE_DESCRIPTION("AML SYNO9629 ALSA machine layer driver");
MODULE_LICENSE("GPL");

