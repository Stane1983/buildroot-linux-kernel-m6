#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#include "tas5707.h"

#define CODEC_DEBUG printk

#define tas5707_RATES (SNDRV_PCM_RATE_8000 | \
		      SNDRV_PCM_RATE_11025 | \
		      SNDRV_PCM_RATE_16000 | \
		      SNDRV_PCM_RATE_22050 | \
		      SNDRV_PCM_RATE_32000 | \
		      SNDRV_PCM_RATE_44100 | \
		      SNDRV_PCM_RATE_48000)

#define tas5707_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE  | SNDRV_PCM_FMTBIT_S16_BE  | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE  | SNDRV_PCM_FMTBIT_S24_BE)

/* Power-up register defaults */
static const u8 tas5707_regs[DDX_NUM_BYTE_REG] = {
	0x6c, 0x70, 0x00, 0xA0, 0x05, 0x40, 0x00, 0xFF,
	0x30, 0x30,	0xFF, 0x00, 0x00, 0x00, 0x91, 0x00,//0x0F
	0x02, 0xAC, 0x54, 0xAC,	0x54, 0x00, 0x00, 0x00,//0x17
	0x00, 0x30, 0x0F, 0x82, 0x02,
};

static const u8 TAS5707_subwoofer_table[2][21]={
	//0x5A   150Hz-lowpass
	{0x5A,
	 0x00,0x00,0x03,0x1D,
	 0x00,0x00,0x06,0x3A,
	 0x00,0x00,0x03,0x1D,
	 0x00,0xFC,0x72,0x05,
	 0x0F,0x83,0x81,0x85},
	//0x5B   150HZ-10dB
	{0x5B,
	 0x00,0x81,0x50,0x89,
	 0x0F,0x03,0x68,0x8C,
	 0x00,0x7B,0x6A,0x2F,
	 0x00,0xFC,0xA3,0x83,
	 0x0F,0x83,0x51,0x56}
};

/* codec private data */
struct tas5707_priv {
	struct snd_soc_codec *codec;
	struct tas5711_platform_data *pdata;

	enum snd_soc_control_type control_type;
	void *control_data;
	unsigned mclk;
};

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -12700, 50, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 50, 1);


static const struct snd_kcontrol_new tas5707_snd_controls[] = {
	SOC_SINGLE_TLV("Master Volume", DDX_MASTER_VOLUME, 0, 0xff, 1, mvol_tlv),
	SOC_SINGLE_TLV("Ch1 Volume", DDX_CHANNEL1_VOL, 0, 0xff, 1, chvol_tlv),
	SOC_SINGLE_TLV("Ch2 Volume", DDX_CHANNEL2_VOL, 0, 0xff, 1, chvol_tlv),
	SOC_SINGLE_TLV("Ch3 Volume", DDX_CHANNEL3_VOL, 0, 0xff, 1, chvol_tlv),
	SOC_SINGLE("Ch1 Switch", DDX_SOFT_MUTE, 0, 1, 1),
	SOC_SINGLE("Ch2 Switch", DDX_SOFT_MUTE, 1, 1, 1),
	SOC_SINGLE("Ch3 Switch", DDX_SOFT_MUTE, 2, 1, 1),
};

static int tas5707_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	CODEC_DEBUG("~~~~%s\n", __func__);
	struct snd_soc_codec *codec = codec_dai->codec;
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	tas5707->mclk = freq;
	if(freq == 512* 48000)
		snd_soc_write(codec, DDX_CLOCK_CTL, 0x74);//0x74 = 512fs; 0x6c = 256fs
	else
		snd_soc_write(codec, DDX_CLOCK_CTL, 0x6c);//0x74 = 512fs; 0x6c = 256fs
	return 0;
}

static int tas5707_set_dai_fmt(struct snd_soc_dai *codec_dai,
				  unsigned int fmt)
{
	//struct snd_soc_codec *codec = codec_dai->codec;
	CODEC_DEBUG("~~~~%s\n", __func__);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int tas5707_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	//struct snd_soc_pcm_runtime *rtd = substream->private_data;
	//struct snd_soc_codec *codec = rtd->codec;
	unsigned int rate;
	CODEC_DEBUG("~~~~%s\n", __func__);

	rate = params_rate(params);
	pr_debug("rate: %u\n", rate);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
		pr_debug("24bit\n");
		/* fall through */
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S20_3BE:
		pr_debug("20bit\n");

		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		pr_debug("16bit\n");

		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int tas5707_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	pr_debug("level = %d\n", level);
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		/* Full power on */

		break;

	case SND_SOC_BIAS_STANDBY:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
		}

		/* Power up to mute */
		/* FIXME */
		break;

	case SND_SOC_BIAS_OFF:
		/* The chip runs through the power down sequence for us. */
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static const struct snd_soc_dai_ops tas5707_dai_ops = {
	.hw_params	= tas5707_hw_params,
	.set_sysclk	= tas5707_set_dai_sysclk,
	.set_fmt	= tas5707_set_dai_fmt,
};

static struct snd_soc_dai_driver tas5707_dai = {
	.name = "tas5707",
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = tas5707_RATES,
		.formats = tas5707_FORMATS,
	},
	.ops = &tas5707_dai_ops,
};

static int tas5707_init(struct snd_soc_codec *codec)
{
	int i, ret;
	unsigned char burst_data[][5]= {
		{DDX_INPUT_MUX,0x00,0x01,0x77,0x72},
		{DDX_CH4_SOURCE_SELECT,0x00,0x00,0x42,0x03},
		{DDX_PWM_MUX,0x01,0x01,0x32,0x45},
	};
	printk("~~~~tas5707_init\n");

	snd_soc_write(codec, DDX_CLOCK_CTL, 0x6c);//0x74 = 512fs; 0x6c = 256fs
	snd_soc_write(codec, DDX_SYS_CTL_1, 0xa0);
	snd_soc_write(codec, DDX_SERIAL_DATA_INTERFACE, 0x05);

/*	snd_soc_write(codec, DDX_IC_DELAY_CHANNEL_1, 0xac);
	snd_soc_write(codec, DDX_IC_DELAY_CHANNEL_2, 0x54);
	snd_soc_write(codec, DDX_IC_DELAY_CHANNEL_3, 0xac);
	snd_soc_write(codec, DDX_IC_DELAY_CHANNEL_4, 0x54);
*/
	snd_soc_write(codec, DDX_BKND_ERR, 0x02);

	snd_soc_bulk_write_raw(codec, DDX_INPUT_MUX, burst_data[0], 5);
	snd_soc_bulk_write_raw(codec, DDX_CH4_SOURCE_SELECT, burst_data[1], 5);
	snd_soc_bulk_write_raw(codec, DDX_PWM_MUX, burst_data[2], 5);

	//subwoofer
	//snd_soc_bulk_write_raw(codec, DDX_SUBCHANNEL_BQ_0, TAS5711_subwoofer_table[0], 21);
	//snd_soc_bulk_write_raw(codec, DDX_SUBCHANNEL_BQ_1, TAS5711_subwoofer_table[1], 21);

	snd_soc_write(codec, DDX_VOLUME_CONFIG, 0xD1);
	snd_soc_write(codec, DDX_SYS_CTL_2, 0x84);
	snd_soc_write(codec, DDX_START_STOP_PERIOD, 0x95);
	snd_soc_write(codec, DDX_PWM_SHUTDOWN_GROUP, 0x30);
	snd_soc_write(codec, DDX_MODULATION_LIMIT, 0x02);
	//normal operation
	snd_soc_write(codec, DDX_MASTER_VOLUME, 0x00);
	snd_soc_write(codec, DDX_CHANNEL1_VOL, 0x30);
	snd_soc_write(codec, DDX_CHANNEL2_VOL, 0x30);
	snd_soc_write(codec, DDX_CHANNEL3_VOL, 0x30);
	snd_soc_write(codec, DDX_SOFT_MUTE, 0x00);

	return 0;
}
static int tas5707_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);

	printk("~~~~~~~~~~~~%s\n", __func__);
	//codec->control_data = tas5711->control_data;
	codec->control_type = tas5707->control_type;
    ret = snd_soc_codec_set_cache_io(codec, 8, 8, tas5707->control_type);
    if (ret != 0) {
        dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
        return ret;
    }

	snd_soc_write(codec, DDX_OSC_TRIM, 0x00);
	msleep(50);
	//TODO: set the DAP
	tas5707_init(codec);

	return 0;
}

static int tas5707_remove(struct snd_soc_codec *codec)
{
	printk("~~~~~~~~~~~~%s", __func__);
	return 0;
}

#ifdef CONFIG_PM
static int tas5707_suspend(struct snd_soc_codec *codec)
{
	tas5707_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int tas5707_resume(struct snd_soc_codec *codec)
{
	tas5707_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}
#else
#define tas5707_suspend NULL
#define tas5707_resume NULL
#endif

static const struct snd_soc_codec_driver tas5707_codec = {
	.probe =		tas5707_probe,
	.remove =		tas5707_remove,
	.suspend =		tas5707_suspend,
	.resume =		tas5707_resume,
	.reg_cache_size = DDX_NUM_BYTE_REG,
	.reg_word_size = sizeof(u8),
	.reg_cache_default = tas5707_regs,
	//.volatile_register =	tas5711_reg_is_volatile,
	.set_bias_level = tas5707_set_bias_level,
	.controls =		tas5707_snd_controls,
	.num_controls =		ARRAY_SIZE(tas5707_snd_controls),

};

static __devinit int tas5707_i2c_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	struct tas5707_priv *tas5707;
	int ret;

	printk("~~~~~~tas5707_i2c_probe~~~~~~~~~\n");

	tas5707 = devm_kzalloc(&i2c->dev, sizeof(struct tas5707_priv),
			      GFP_KERNEL);
	if (!tas5707)
		return -ENOMEM;

	i2c_set_clientdata(i2c, tas5707);
	tas5707->control_type = SND_SOC_I2C;

	ret = snd_soc_register_codec(&i2c->dev, &tas5707_codec,
			&tas5707_dai, 1);
	if (ret != 0){
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);
		//devm_kfree(&i2c->dev, STA381xx);
	}
	return ret;
}

static __devexit int tas5707_i2c_remove(struct i2c_client *client)
{
	//snd_soc_unregister_codec(&client->dev);
	devm_kfree(&client->dev, i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id tas5707_i2c_id[] = {
	{ "tas5707", 0 },
	{ }
};

static struct i2c_driver tas5707_i2c_driver = {
	.driver = {
		.name = "tas5707",
		.owner = THIS_MODULE,
	},
	.probe =    tas5707_i2c_probe,
	.remove =   __devexit_p(tas5707_i2c_remove),
	.id_table = tas5707_i2c_id,
};

static int __init TAS5707_init(void)
{
	return i2c_add_driver(&tas5707_i2c_driver);
}

static void __exit TAS5707_exit(void)
{
	i2c_del_driver(&tas5707_i2c_driver);
}
module_init(TAS5707_init);
module_exit(TAS5707_exit);
