/*
 * Codec driver for ST STA381xx 2.1-channel high-efficiency digital audio system
 *
 * Copyright: 2011 Raumfeld GmbH
 * Author: Johannes Stezenbach <js@sig21.net>
 *
 * based on code from:
 *	Wolfson Microelectronics PLC.
 *	  Mark Brown <broonie@opensource.wolfsonmicro.com>
 *	Freescale Semiconductor, Inc.
 *	  Timur Tabi <timur@freescale.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#define CODEC_DEBUG printk
#define pr_fmt(fmt) KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__
#define DEBUG 1 

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "sta380x.h"

#define STA381xx_RATES (SNDRV_PCM_RATE_32000 | \
		      SNDRV_PCM_RATE_44100 | \
		      SNDRV_PCM_RATE_48000 | \
		      SNDRV_PCM_RATE_88200 | \
		      SNDRV_PCM_RATE_96000 | \
		      SNDRV_PCM_RATE_176400 | \
		      SNDRV_PCM_RATE_192000)

#define STA381xx_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE  | SNDRV_PCM_FMTBIT_S16_BE  | \
	 SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S18_3BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S24_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE  | SNDRV_PCM_FMTBIT_S24_BE  | \
	 SNDRV_PCM_FMTBIT_S32_LE  | SNDRV_PCM_FMTBIT_S32_BE)

/* Power-up register defaults */
static const u8 STA381xx_regs[STA381xx_REGISTER_COUNT] = {
	0x67, 0x80, 0x97, 0x18, 0x82, 0x5c, 0x10, 0xff, 0x60, 0x60,
	0x60, 0x80, 0x00, 0x00, 0x00, 0x40, 0x80, 0x77, 0x6a, 0x69,
	0x6a, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2d,
	0xc0, 0xf3, 0x33, 0x00, 0x0c,
};

/* regulator power supply names */
static const char *STA381xx_supply_names[] = {
	"Vdda",	/* analog supply, 3.3VV */
	"Vdd3",	/* digital supply, 3.3V */
	"Vcc"	/* power amp spply, 10V - 36V */
};

/* codec private data */
struct STA381xx_priv {
	struct regulator_bulk_data supplies[ARRAY_SIZE(STA381xx_supply_names)];
	struct snd_soc_codec *codec;
	struct sta381xx_platform_data *pdata;

	unsigned int mclk;
	unsigned int format;

	u32 coef_shadow[STA381xx_COEF_COUNT];
	struct delayed_work watchdog_work;
	int shutdown;
};

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -12700, 50, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -7950, 50, 1);
static const DECLARE_TLV_DB_SCALE(tone_tlv, -120, 200, 0);

static const char *STA381xx_drc_ac[] = {
	"Anti-Clipping", "Dynamic Range Compression" };
/*static const char *STA381xx_auto_eq_mode[] = {
	"User", "Preset", "Loudness" };
static const char *STA381xx_auto_gc_mode[] = {
	"User", "AC no clipping", "AC limited clipping (10%)",
	"DRC nighttime listening mode" };
*/
static const char *STA381xx_auto_xo_mode[] = {
	"User", "80Hz", "100Hz", "120Hz", "140Hz", "160Hz", "180Hz", "200Hz",
	"220Hz", "240Hz", "260Hz", "280Hz", "300Hz", "320Hz", "340Hz", "360Hz" };
/*static const char *STA381xx_preset_eq_mode[] = {
	"Flat", "Rock", "Soft Rock", "Jazz", "Classical", "Dance", "Pop", "Soft",
	"Hard", "Party", "Vocal", "Hip-Hop", "Dialog", "Bass-boost #1",
	"Bass-boost #2", "Bass-boost #3", "Loudness 1", "Loudness 2",
	"Loudness 3", "Loudness 4", "Loudness 5", "Loudness 6", "Loudness 7",
	"Loudness 8", "Loudness 9", "Loudness 10", "Loudness 11", "Loudness 12",
	"Loudness 13", "Loudness 14", "Loudness 15", "Loudness 16" };
*/
static const char *STA381xx_limiter_select[] = {
	"Limiter Disabled", "Limiter #1", "Limiter #2" };
static const char *STA381xx_limiter_attack_rate[] = {
	"3.1584", "2.7072", "2.2560", "1.8048", "1.3536", "0.9024",
	"0.4512", "0.2256", "0.1504", "0.1123", "0.0902", "0.0752",
	"0.0645", "0.0564", "0.0501", "0.0451" };
static const char *STA381xx_limiter_release_rate[] = {
	"0.5116", "0.1370", "0.0744", "0.0499", "0.0360", "0.0299",
	"0.0264", "0.0208", "0.0198", "0.0172", "0.0147", "0.0137",
	"0.0134", "0.0117", "0.0110", "0.0104" };

static const unsigned int STA381xx_limiter_ac_attack_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 7, TLV_DB_SCALE_ITEM(-1200, 200, 0),
	8, 16, TLV_DB_SCALE_ITEM(300, 100, 0),
};

static const unsigned int STA381xx_limiter_ac_release_tlv[] = {
	TLV_DB_RANGE_HEAD(5),
	0, 0, TLV_DB_SCALE_ITEM(TLV_DB_GAIN_MUTE, 0, 0),
	1, 1, TLV_DB_SCALE_ITEM(-2900, 0, 0),
	2, 2, TLV_DB_SCALE_ITEM(-2000, 0, 0),
	3, 8, TLV_DB_SCALE_ITEM(-1400, 200, 0),
	8, 16, TLV_DB_SCALE_ITEM(-700, 100, 0),
};

static const unsigned int STA381xx_limiter_drc_attack_tlv[] = {
	TLV_DB_RANGE_HEAD(3),
	0, 7, TLV_DB_SCALE_ITEM(-3100, 200, 0),
	8, 13, TLV_DB_SCALE_ITEM(-1600, 100, 0),
	14, 16, TLV_DB_SCALE_ITEM(-1000, 300, 0),
};

static const unsigned int STA381xx_limiter_drc_release_tlv[] = {
	TLV_DB_RANGE_HEAD(5),
	0, 0, TLV_DB_SCALE_ITEM(TLV_DB_GAIN_MUTE, 0, 0),
	1, 2, TLV_DB_SCALE_ITEM(-3800, 200, 0),
	3, 4, TLV_DB_SCALE_ITEM(-3300, 200, 0),
	5, 12, TLV_DB_SCALE_ITEM(-3000, 200, 0),
	13, 16, TLV_DB_SCALE_ITEM(-1500, 300, 0),
};

static const struct soc_enum STA381xx_drc_ac_enum =
	SOC_ENUM_SINGLE(STA381xx_CONFD, STA381xx_CONFD_DRC_SHIFT,
			2, STA381xx_drc_ac);
//static const struct soc_enum STA381xx_auto_eq_enum =
//	SOC_ENUM_SINGLE(STA381xx_AUTO1, STA381xx_AUTO1_AMEQ_SHIFT,
//			3, STA381xx_auto_eq_mode);
//static const struct soc_enum STA381xx_auto_gc_enum =
//	SOC_ENUM_SINGLE(STA381xx_AUTO1, STA381xx_AUTO1_AMGC_SHIFT,
//			4, STA381xx_auto_gc_mode);
static const struct soc_enum STA381xx_auto_xo_enum =
	SOC_ENUM_SINGLE(STA381xx_AUTO2, STA381xx_AUTO2_XO_SHIFT,
			16, STA381xx_auto_xo_mode);
//static const struct soc_enum STA381xx_preset_eq_enum =
//	SOC_ENUM_SINGLE(STA381xx_AUTO3, STA381xx_AUTO3_PEQ_SHIFT,
//			32, STA381xx_preset_eq_mode);
static const struct soc_enum STA381xx_limiter_ch1_enum =
	SOC_ENUM_SINGLE(STA381xx_C1CFG, STA381xx_CxCFG_LS_SHIFT,
			3, STA381xx_limiter_select);
static const struct soc_enum STA381xx_limiter_ch2_enum =
	SOC_ENUM_SINGLE(STA381xx_C2CFG, STA381xx_CxCFG_LS_SHIFT,
			3, STA381xx_limiter_select);
static const struct soc_enum STA381xx_limiter_ch3_enum =
	SOC_ENUM_SINGLE(STA381xx_C3CFG, STA381xx_CxCFG_LS_SHIFT,
			3, STA381xx_limiter_select);
static const struct soc_enum STA381xx_limiter1_attack_rate_enum =
	SOC_ENUM_SINGLE(STA381xx_L1AR, STA381xx_LxA_SHIFT,
			16, STA381xx_limiter_attack_rate);
static const struct soc_enum STA381xx_limiter2_attack_rate_enum =
	SOC_ENUM_SINGLE(STA381xx_L2AR, STA381xx_LxA_SHIFT,
			16, STA381xx_limiter_attack_rate);
static const struct soc_enum STA381xx_limiter1_release_rate_enum =
	SOC_ENUM_SINGLE(STA381xx_L1AR, STA381xx_LxR_SHIFT,
			16, STA381xx_limiter_release_rate);
static const struct soc_enum STA381xx_limiter2_release_rate_enum =
	SOC_ENUM_SINGLE(STA381xx_L2AR, STA381xx_LxR_SHIFT,
			16, STA381xx_limiter_release_rate);

/* byte array controls for setting biquad, mixer, scaling coefficients;
 * for biquads all five coefficients need to be set in one go,
 * mixer and pre/postscale coefs can be set individually;
 * each coef is 24bit, the bytes are ordered in the same way
 * as given in the STA381xx data sheet (big endian; b1, b2, a1, a2, b0)
 */

static int STA381xx_coefficient_info(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_info *uinfo)
{
	int numcoef = kcontrol->private_value >> 16;
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = 3 * numcoef;
	return 0;
}

static int STA381xx_coefficient_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int numcoef = kcontrol->private_value >> 16;
	int index = kcontrol->private_value & 0xffff;
	unsigned int cfud;
	int i;

	/* preserve reserved bits in STA381xx_CFUD */
	cfud = snd_soc_read(codec, STA381xx_CFUD) & 0xf0;
	/* chip documentation does not say if the bits are self clearing,
	 * so do it explicitly */
	snd_soc_write(codec, STA381xx_CFUD, cfud);

	snd_soc_write(codec, STA381xx_CFADDR2, index);
	if (numcoef == 1)
		snd_soc_write(codec, STA381xx_CFUD, cfud | 0x04);
	else if (numcoef == 5)
		snd_soc_write(codec, STA381xx_CFUD, cfud | 0x08);
	else
		return -EINVAL;
	for (i = 0; i < 3 * numcoef; i++)
		ucontrol->value.bytes.data[i] =
			snd_soc_read(codec, STA381xx_B1CF1 + i);

	return 0;
}

static int STA381xx_coefficient_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);
	int numcoef = kcontrol->private_value >> 16;
	int index = kcontrol->private_value & 0xffff;
	unsigned int cfud;
	int i;

	/* preserve reserved bits in STA381xx_CFUD */
	cfud = snd_soc_read(codec, STA381xx_CFUD) & 0xf0;
	/* chip documentation does not say if the bits are self clearing,
	 * so do it explicitly */
	snd_soc_write(codec, STA381xx_CFUD, cfud);

	snd_soc_write(codec, STA381xx_CFADDR2, index);
	for (i = 0; i < numcoef && (index + i < STA381xx_COEF_COUNT); i++)
		STA381xx->coef_shadow[index + i] =
			  (ucontrol->value.bytes.data[3 * i] << 16)
			| (ucontrol->value.bytes.data[3 * i + 1] << 8)
			| (ucontrol->value.bytes.data[3 * i + 2]);
	for (i = 0; i < 3 * numcoef; i++)
		snd_soc_write(codec, STA381xx_B1CF1 + i,
			      ucontrol->value.bytes.data[i]);
	if (numcoef == 1)
		snd_soc_write(codec, STA381xx_CFUD, cfud | 0x01);
	else if (numcoef == 5)
		snd_soc_write(codec, STA381xx_CFUD, cfud | 0x02);
	else
		return -EINVAL;

	return 0;
}

static int STA381xx_sync_coef_shadow(struct snd_soc_codec *codec)
{
	struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);
	unsigned int cfud;
	int i;

	/* preserve reserved bits in STA381xx_CFUD */
	cfud = snd_soc_read(codec, STA381xx_CFUD) & 0xf0;

	for (i = 0; i < STA381xx_COEF_COUNT; i++) {
		snd_soc_write(codec, STA381xx_CFADDR2, i);
		snd_soc_write(codec, STA381xx_B1CF1,
			      (STA381xx->coef_shadow[i] >> 16) & 0xff);
		snd_soc_write(codec, STA381xx_B1CF2,
			      (STA381xx->coef_shadow[i] >> 8) & 0xff);
		snd_soc_write(codec, STA381xx_B1CF3,
			      (STA381xx->coef_shadow[i]) & 0xff);
		/* chip documentation does not say if the bits are
		 * self-clearing, so do it explicitly */
		snd_soc_write(codec, STA381xx_CFUD, cfud);
		snd_soc_write(codec, STA381xx_CFUD, cfud | 0x01);
	}
	return 0;
}

static int STA381xx_cache_sync(struct snd_soc_codec *codec)
{
	unsigned int mute;
	int rc;

	if (!codec->cache_sync)
		return 0;

	/* mute during register sync */
	mute = snd_soc_read(codec, STA381xx_MMUTE);
	snd_soc_write(codec, STA381xx_MMUTE, mute | STA381xx_MMUTE_MMUTE);
	STA381xx_sync_coef_shadow(codec);
	rc = snd_soc_cache_sync(codec);
	snd_soc_write(codec, STA381xx_MMUTE, mute);
	return rc;
}

#define SINGLE_COEF(xname, index) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = STA381xx_coefficient_info, \
	.get = STA381xx_coefficient_get,\
	.put = STA381xx_coefficient_put, \
	.private_value = index | (1 << 16) }

#define BIQUAD_COEFS(xname, index) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = STA381xx_coefficient_info, \
	.get = STA381xx_coefficient_get,\
	.put = STA381xx_coefficient_put, \
	.private_value = index | (5 << 16) }

static const struct snd_kcontrol_new STA381xx_snd_controls[] = {
SOC_SINGLE_TLV("Master Volume", STA381xx_MVOL, 0, 0xff, 1, mvol_tlv),
SOC_SINGLE("Master Switch", STA381xx_MMUTE, 0, 1, 1),
SOC_SINGLE("Ch1 Switch", STA381xx_MMUTE, 1, 1, 1),
SOC_SINGLE("Ch2 Switch", STA381xx_MMUTE, 2, 1, 1),
SOC_SINGLE("Ch3 Switch", STA381xx_MMUTE, 3, 1, 1),
SOC_SINGLE_TLV("Ch1 Volume", STA381xx_C1VOL, 0, 0xff, 1, chvol_tlv),
SOC_SINGLE_TLV("Ch2 Volume", STA381xx_C2VOL, 0, 0xff, 1, chvol_tlv),
SOC_SINGLE_TLV("Ch3 Volume", STA381xx_C3VOL, 0, 0xff, 1, chvol_tlv),
SOC_SINGLE("De-emphasis Filter Switch", STA381xx_CONFD, STA381xx_CONFD_DEMP_SHIFT, 1, 0),
SOC_ENUM("Compressor/Limiter Switch", STA381xx_drc_ac_enum),//??
SOC_SINGLE("Miami Mode Switch", STA381xx_CONFD, STA381xx_CONFD_MME_SHIFT, 1, 0),
SOC_SINGLE("Zero Cross Switch", STA381xx_CONFE, STA381xx_CONFE_ZCE_SHIFT, 1, 0),
SOC_SINGLE("Soft Ramp Switch", STA381xx_CONFE, STA381xx_CONFE_SVE_SHIFT, 1, 0),
SOC_SINGLE("Auto-Mute Switch", STA381xx_CONFF, STA381xx_CONFF_IDE_SHIFT, 1, 0),
//SOC_ENUM("Automode EQ", STA381xx_auto_eq_enum),
//SOC_ENUM("Automode GC", STA381xx_auto_gc_enum),
SOC_ENUM("Automode XO", STA381xx_auto_xo_enum),
//SOC_ENUM("Preset EQ", STA381xx_preset_eq_enum),
SOC_SINGLE("Ch1 Tone Control Bypass Switch", STA381xx_C1CFG, STA381xx_CxCFG_TCB_SHIFT, 1, 0),
SOC_SINGLE("Ch2 Tone Control Bypass Switch", STA381xx_C2CFG, STA381xx_CxCFG_TCB_SHIFT, 1, 0),
SOC_SINGLE("Ch1 EQ Bypass Switch", STA381xx_C1CFG, STA381xx_CxCFG_EQBP_SHIFT, 1, 0),
SOC_SINGLE("Ch2 EQ Bypass Switch", STA381xx_C2CFG, STA381xx_CxCFG_EQBP_SHIFT, 1, 0),
SOC_SINGLE("Ch1 Master Volume Bypass Switch", STA381xx_C1CFG, STA381xx_CxCFG_VBP_SHIFT, 1, 0),
SOC_SINGLE("Ch2 Master Volume Bypass Switch", STA381xx_C1CFG, STA381xx_CxCFG_VBP_SHIFT, 1, 0),
SOC_SINGLE("Ch3 Master Volume Bypass Switch", STA381xx_C1CFG, STA381xx_CxCFG_VBP_SHIFT, 1, 0),
SOC_ENUM("Ch1 Limiter Select", STA381xx_limiter_ch1_enum),
SOC_ENUM("Ch2 Limiter Select", STA381xx_limiter_ch2_enum),
SOC_ENUM("Ch3 Limiter Select", STA381xx_limiter_ch3_enum),
SOC_SINGLE_TLV("Bass Tone Control", STA381xx_TONE, STA381xx_TONE_BTC_SHIFT, 15, 0, tone_tlv),
SOC_SINGLE_TLV("Treble Tone Control", STA381xx_TONE, STA381xx_TONE_TTC_SHIFT, 15, 0, tone_tlv),
SOC_ENUM("Limiter1 Attack Rate (dB/ms)", STA381xx_limiter1_attack_rate_enum),
SOC_ENUM("Limiter2 Attack Rate (dB/ms)", STA381xx_limiter2_attack_rate_enum),
SOC_ENUM("Limiter1 Release Rate (dB/ms)", STA381xx_limiter1_release_rate_enum),
SOC_ENUM("Limiter2 Release Rate (dB/ms)", STA381xx_limiter1_release_rate_enum),

/* depending on mode, the attack/release thresholds have
 * two different enum definitions; provide both
 */
SOC_SINGLE_TLV("Limiter1 Attack Threshold (AC Mode)", STA381xx_L1ATRT, STA381xx_LxA_SHIFT,
	       16, 0, STA381xx_limiter_ac_attack_tlv),
SOC_SINGLE_TLV("Limiter2 Attack Threshold (AC Mode)", STA381xx_L2ATRT, STA381xx_LxA_SHIFT,
	       16, 0, STA381xx_limiter_ac_attack_tlv),
SOC_SINGLE_TLV("Limiter1 Release Threshold (AC Mode)", STA381xx_L1ATRT, STA381xx_LxR_SHIFT,
	       16, 0, STA381xx_limiter_ac_release_tlv),
SOC_SINGLE_TLV("Limiter2 Release Threshold (AC Mode)", STA381xx_L2ATRT, STA381xx_LxR_SHIFT,
	       16, 0, STA381xx_limiter_ac_release_tlv),
SOC_SINGLE_TLV("Limiter1 Attack Threshold (DRC Mode)", STA381xx_L1ATRT, STA381xx_LxA_SHIFT,
	       16, 0, STA381xx_limiter_drc_attack_tlv),
SOC_SINGLE_TLV("Limiter2 Attack Threshold (DRC Mode)", STA381xx_L2ATRT, STA381xx_LxA_SHIFT,
	       16, 0, STA381xx_limiter_drc_attack_tlv),
SOC_SINGLE_TLV("Limiter1 Release Threshold (DRC Mode)", STA381xx_L1ATRT, STA381xx_LxR_SHIFT,
	       16, 0, STA381xx_limiter_drc_release_tlv),
SOC_SINGLE_TLV("Limiter2 Release Threshold (DRC Mode)", STA381xx_L2ATRT, STA381xx_LxR_SHIFT,
	       16, 0, STA381xx_limiter_drc_release_tlv),

BIQUAD_COEFS("Ch1 - Biquad 1", 0),
BIQUAD_COEFS("Ch1 - Biquad 2", 5),
BIQUAD_COEFS("Ch1 - Biquad 3", 10),
BIQUAD_COEFS("Ch1 - Biquad 4", 15),
BIQUAD_COEFS("Ch2 - Biquad 1", 20),
BIQUAD_COEFS("Ch2 - Biquad 2", 25),
BIQUAD_COEFS("Ch2 - Biquad 3", 30),
BIQUAD_COEFS("Ch2 - Biquad 4", 35),
BIQUAD_COEFS("High-pass", 40),
BIQUAD_COEFS("Low-pass", 45),
SINGLE_COEF("Ch1 - Prescale", 50),
SINGLE_COEF("Ch2 - Prescale", 51),
SINGLE_COEF("Ch1 - Postscale", 52),
SINGLE_COEF("Ch2 - Postscale", 53),
SINGLE_COEF("Ch3 - Postscale", 54),
SINGLE_COEF("Thermal warning - Postscale", 55),
SINGLE_COEF("Ch1 - Mix 1", 56),
SINGLE_COEF("Ch1 - Mix 2", 57),
SINGLE_COEF("Ch2 - Mix 1", 58),
SINGLE_COEF("Ch2 - Mix 2", 59),
SINGLE_COEF("Ch3 - Mix 1", 60),
SINGLE_COEF("Ch3 - Mix 2", 61),
};

static const struct snd_soc_dapm_widget STA381xx_dapm_widgets[] = {
SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
SND_SOC_DAPM_OUTPUT("LEFT"),
SND_SOC_DAPM_OUTPUT("RIGHT"),
SND_SOC_DAPM_OUTPUT("SUB"),
};

static const struct snd_soc_dapm_route STA381xx_dapm_routes[] = {
	{ "LEFT", NULL, "DAC" },
	{ "RIGHT", NULL, "DAC" },
	{ "SUB", NULL, "DAC" },
};

/* MCLK interpolation ratio per fs */
static struct {
	int fs;
	int ir;
} interpolation_ratios[] = {
	{ 32000, 0 },
	{ 44100, 0 },
	{ 48000, 0 },
	{ 88200, 1 },
	{ 96000, 1 },
	{ 176400, 2 },
	{ 192000, 2 },
};

/* MCLK to fs clock ratios */
static struct {
	int ratio;
	int mcs;
} mclk_ratios[3][7] = {
	{ { 768, 0 }, { 512, 1 }, { 384, 2 }, { 256, 3 },
	  { 128, 4 }, { 576, 5 }, { 0, 0 } },
	{ { 384, 2 }, { 256, 3 }, { 192, 4 }, { 128, 5 }, {64, 0 }, { 0, 0 } },
	{ { 384, 2 }, { 256, 3 }, { 192, 4 }, { 128, 5 }, {64, 0 }, { 0, 0 } },
};


/**
 * STA381xx_set_dai_sysclk - configure MCLK
 * @codec_dai: the codec DAI
 * @clk_id: the clock ID (ignored)
 * @freq: the MCLK input frequency
 * @dir: the clock direction (ignored)
 *
 * The value of MCLK is used to determine which sample rates are supported
 * by the STA381xx, based on the mclk_ratios table.
 *
 * This function must be called by the machine driver's 'startup' function,
 * otherwise the list of supported sample rates will not be available in
 * time for ALSA.
 *
 * For setups with variable MCLKs, pass 0 as 'freq' argument. This will cause
 * theoretically possible sample rates to be enabled. Call it again with a
 * proper value set one the external clock is set (most probably you would do
 * that from a machine's driver 'hw_param' hook.
 */
static int STA381xx_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);
	int i, j, ir, fs;
	unsigned int rates = 0;
	unsigned int rate_min = -1;
	unsigned int rate_max = 0;
CODEC_DEBUG("~~~~%s\n", __func__);

	pr_debug("mclk=%u\n", freq);
	STA381xx->mclk = freq;

	if (STA381xx->mclk) {
		for (i = 0; i < ARRAY_SIZE(interpolation_ratios); i++) {
			ir = interpolation_ratios[i].ir;
			fs = interpolation_ratios[i].fs;
			for (j = 0; mclk_ratios[ir][j].ratio; j++) {
				if (mclk_ratios[ir][j].ratio * fs == freq) {
					rates |= snd_pcm_rate_to_rate_bit(fs);
					if (fs < rate_min)
						rate_min = fs;
					if (fs > rate_max)
						rate_max = fs;
					break;
				}
			}
		}
		/* FIXME: soc should support a rate list */
		rates &= ~SNDRV_PCM_RATE_KNOT;

		if (!rates) {
			dev_err(codec->dev, "could not find a valid sample rate\n");
			return -EINVAL;
		}
	} else {
		/* enable all possible rates */
		rates = STA381xx_RATES;
		rate_min = 32000;
		rate_max = 192000;
	}

	codec_dai->driver->playback.rates = rates;
	codec_dai->driver->playback.rate_min = rate_min;
	codec_dai->driver->playback.rate_max = rate_max;
	return 0;
}

/**
 * STA381xx_set_dai_fmt - configure the codec for the selected audio format
 * @codec_dai: the codec DAI
 * @fmt: a SND_SOC_DAIFMT_x value indicating the data format
 *
 * This function takes a bitmask of SND_SOC_DAIFMT_x bits and programs the
 * codec accordingly.
 */
static int STA381xx_set_dai_fmt(struct snd_soc_dai *codec_dai,
			      unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);
	u8 confb = snd_soc_read(codec, STA381xx_CONFB);
	CODEC_DEBUG("~~~~%s\n", __func__);

	pr_debug("\n");
	confb &= ~(STA381xx_CONFB_C1IM | STA381xx_CONFB_C2IM);

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
		STA381xx->format = fmt & SND_SOC_DAIFMT_FORMAT_MASK;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		confb |= STA381xx_CONFB_C2IM;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		confb |= STA381xx_CONFB_C1IM;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, STA381xx_CONFB, confb);
	return 0;
}

/**
 * STA381xx_hw_params - program the STA381xx with the given hardware parameters.
 * @substream: the audio stream
 * @params: the hardware parameters to set
 * @dai: the SOC DAI (ignored)
 *
 * This function programs the hardware with the values provided.
 * Specifically, the sample rate and the data format.
 */
static int STA381xx_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);
	unsigned int rate;
	int i, mcs = -1, ir = -1;
	u8 confa, confb;
	CODEC_DEBUG("~~~~%s\n", __func__);

	rate = params_rate(params);
	pr_debug("rate: %u\n", rate);
	for (i = 0; i < ARRAY_SIZE(interpolation_ratios); i++)
		if (interpolation_ratios[i].fs == rate) {
			ir = interpolation_ratios[i].ir;
			break;
		}
	if (ir < 0)
		return -EINVAL;
	for (i = 0; mclk_ratios[ir][i].ratio; i++)
		if (mclk_ratios[ir][i].ratio * rate == STA381xx->mclk) {
			mcs = mclk_ratios[ir][i].mcs;
			break;
		}
	if (mcs < 0)
		return -EINVAL;

	confa = snd_soc_read(codec, STA381xx_CONFA);
	confa &= ~(STA381xx_CONFA_MCS_MASK | STA381xx_CONFA_IR_MASK);
	confa |= (ir << STA381xx_CONFA_IR_SHIFT) | (mcs << STA381xx_CONFA_MCS_SHIFT);

	confb = snd_soc_read(codec, STA381xx_CONFB);
	confb &= ~(STA381xx_CONFB_SAI_MASK | STA381xx_CONFB_SAIFB);
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_3BE:
		pr_debug("24bit\n");
		/* fall through */
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S32_BE:
		pr_debug("24bit or 32bit\n");
		switch (STA381xx->format) {
		case SND_SOC_DAIFMT_I2S:
			confb |= 0x0;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			confb |= 0x1;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			confb |= 0x2;
			break;
		}

		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S20_3BE:
		pr_debug("20bit\n");
		switch (STA381xx->format) {
		case SND_SOC_DAIFMT_I2S:
			confb |= 0x4;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			confb |= 0x5;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			confb |= 0x6;
			break;
		}

		break;
	case SNDRV_PCM_FORMAT_S18_3LE:
	case SNDRV_PCM_FORMAT_S18_3BE:
		pr_debug("18bit\n");
		switch (STA381xx->format) {
		case SND_SOC_DAIFMT_I2S:
			confb |= 0x8;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			confb |= 0x9;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			confb |= 0xa;
			break;
		}

		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		pr_debug("16bit\n");
		switch (STA381xx->format) {
		case SND_SOC_DAIFMT_I2S:
			confb |= 0x0;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			confb |= 0xd;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			confb |= 0xe;
			break;
		}

		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, STA381xx_CONFA, confa);
	snd_soc_write(codec, STA381xx_CONFB, confb);
	return 0;
}

/**
 * STA381xx_set_bias_level - DAPM callback
 * @codec: the codec device
 * @level: DAPM power level
 *
 * This is called by ALSA to put the codec into low power mode
 * or to wake it up.  If the codec is powered off completely
 * all registers must be restored after power on.
 */
static int STA381xx_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	int ret;
	//struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);

	pr_debug("level = %d\n", level);
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		/* Full power on */
		snd_soc_update_bits(codec, STA381xx_CONFF,
				    STA381xx_CONFF_PWDN | STA381xx_CONFF_EAPD,
				    STA381xx_CONFF_PWDN | STA381xx_CONFF_EAPD);
		break;

	case SND_SOC_BIAS_STANDBY:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
			//ret = regulator_bulk_enable(ARRAY_SIZE(STA381xx->supplies),
			//			    STA381xx->supplies);
			if (ret != 0) {
				dev_err(codec->dev,
					"Failed to enable supplies: %d\n", ret);
				return ret;
			}

			STA381xx_cache_sync(codec);
		}

		/* Power up to mute */
		/* FIXME */
		snd_soc_update_bits(codec, STA381xx_CONFF,
				    STA381xx_CONFF_PWDN | STA381xx_CONFF_EAPD,
				    STA381xx_CONFF_PWDN | STA381xx_CONFF_EAPD);

		break;

	case SND_SOC_BIAS_OFF:
		/* The chip runs through the power down sequence for us. */
		snd_soc_update_bits(codec, STA381xx_CONFF,
				    STA381xx_CONFF_PWDN | STA381xx_CONFF_EAPD,
				    STA381xx_CONFF_PWDN);
		msleep(300);
		//STA381xx_watchdog_stop(STA381xx);
		//regulator_bulk_disable(ARRAY_SIZE(STA381xx->supplies),
		//		       STA381xx->supplies);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static const struct snd_soc_dai_ops STA381xx_dai_ops = {
	.hw_params	= STA381xx_hw_params,
	.set_sysclk	= STA381xx_set_dai_sysclk,
	.set_fmt	= STA381xx_set_dai_fmt,
};

static struct snd_soc_dai_driver STA381xx_dai = {
	.name = "STA381xx",
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = STA381xx_RATES,
		.formats = STA381xx_FORMATS,
	},
	.ops = &STA381xx_dai_ops,
};

#ifdef CONFIG_PM
static int STA381xx_suspend(struct snd_soc_codec *codec)
{
	STA381xx_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int STA381xx_resume(struct snd_soc_codec *codec)
{
	snd_soc_write(codec, STA381xx_MAPSEL, 0x00);
	STA381xx_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}
#else
#define STA381xx_suspend NULL
#define STA381xx_resume NULL
#endif

static int STA381xx_probe(struct snd_soc_codec *codec)
{
	struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);
	int i, ret = 0;

    ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_I2C);
    if (ret != 0) {
        dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
        return ret;
    }

	STA381xx->codec = codec;
	/* Chip documentation explicitly requires that the reset values
	 * of reserved register bits are left untouched.
	 * Write the register default value to cache for reserved registers,
	 * so the write to the these registers are suppressed by the cache
	 * restore code when it skips writes of default registers.
	 */
	 
	snd_soc_write(codec, STA381xx_MAPSEL, 0x00);
	snd_soc_write(codec, STA381xx_CONFA, 0x67);
	snd_soc_write(codec, STA381xx_CONFB, 0x80);
	snd_soc_write(codec, STA381xx_CONFC, 0x9F);
	snd_soc_write(codec, STA381xx_CONFD, 0x18);
	snd_soc_write(codec, STA381xx_CONFE, 0x82);
	snd_soc_write(codec, STA381xx_CONFF, 0x5D);
	snd_soc_write(codec, STA381xx_MMUTE, 0x40);
	snd_soc_write(codec, STA381xx_MVOL, 0xFE);
	snd_soc_write(codec, STA381xx_C1VOL, 0x47);
	snd_soc_write(codec, STA381xx_C2VOL, 0x47);
	snd_soc_write(codec, STA381xx_C3VOL, 0x56);
	snd_soc_write(codec, STA381xx_C1CFG, 0x00);
	snd_soc_write(codec, STA381xx_C2CFG, 0x40);
	snd_soc_write(codec, STA381xx_C3CFG, 0x80);
	snd_soc_write(codec, STA381xx_C1CFG, 0x00);
	//subwoofer
	snd_soc_write(codec, STA381xx_TONE, 0x7F);
	snd_soc_write(codec, STA381xx_AUTO2, 0x70);

	snd_soc_update_bits(codec, STA381xx_CONFF,
		STA381xx_CONFF_EAPD, STA381xx_CONFF_EAPD);
	snd_soc_write(codec, STA381xx_MVOL, 0x00);
	
	//snd_soc_write(codec, STA381xx_F3XCON2, 0x6D);
	//snd_soc_write(codec, STA381xx_HPCONFIG, 0x09);
	
	/* set thermal warning adjustment and recovery */
	//if (!(STA381xx->pdata->thermal_conf & STA381xx_THERMAL_ADJUSTMENT_ENABLE))
	//	thermal |= STA381xx_CONFA_TWAB;
	//if (!(STA381xx->pdata->thermal_conf & STA381xx_THERMAL_RECOVERY_ENABLE))
	//	thermal |= STA381xx_CONFA_TWRB;
	//snd_soc_update_bits(codec, STA381xx_CONFA,
	//		    STA381xx_CONFA_TWAB | STA381xx_CONFA_TWRB,
	//		    thermal);
#if 0
	/* select output configuration  */
	snd_soc_update_bits(codec, STA381xx_CONFF,
			    STA381xx_CONFF_OCFG_MASK,
			    STA381xx->pdata->output_conf
			    << STA381xx_CONFF_OCFG_SHIFT);

	/* channel to output mapping */
	snd_soc_update_bits(codec, STA381xx_C1CFG,
			    STA381xx_CxCFG_OM_MASK,
			    STA381xx->pdata->ch1_output_mapping
			    << STA381xx_CxCFG_OM_SHIFT);
	snd_soc_update_bits(codec, STA381xx_C2CFG,
			    STA381xx_CxCFG_OM_MASK,
			    STA381xx->pdata->ch2_output_mapping
			    << STA381xx_CxCFG_OM_SHIFT);
	snd_soc_update_bits(codec, STA381xx_C3CFG,
			    STA381xx_CxCFG_OM_MASK,
			    STA381xx->pdata->ch3_output_mapping
			    << STA381xx_CxCFG_OM_SHIFT);
#endif
	/* initialize coefficient shadow RAM with reset values */
	for (i = 4; i <= 39; i += 5)
		STA381xx->coef_shadow[i] = 0x100000;
	for (i = 44; i <= 49; i += 5)
		STA381xx->coef_shadow[i] = 0x400000;
	for (i = 50; i <= 54; i++)
		STA381xx->coef_shadow[i] = 0x7fffff;
	
	STA381xx->coef_shadow[55] = 0x5a9df7;
	STA381xx->coef_shadow[56] = 0x7fffff;
	STA381xx->coef_shadow[59] = 0x7fffff;
	STA381xx->coef_shadow[60] = 0x400000;
	STA381xx->coef_shadow[61] = 0x400000;

	STA381xx_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	/* Bias level configuration will have done an extra enable */
	//regulator_bulk_disable(ARRAY_SIZE(STA381xx->supplies), STA381xx->supplies);

	return 0;
}

static int STA381xx_remove(struct snd_soc_codec *codec)
{
	struct STA381xx_priv *STA381xx = snd_soc_codec_get_drvdata(codec);

	//STA381xx_watchdog_stop(STA381xx);
	STA381xx_set_bias_level(codec, SND_SOC_BIAS_OFF);
	//regulator_bulk_disable(ARRAY_SIZE(STA381xx->supplies), STA381xx->supplies);
	//regulator_bulk_free(ARRAY_SIZE(STA381xx->supplies), STA381xx->supplies);

	return 0;
}

static int STA381xx_reg_is_volatile(struct snd_soc_codec *codec,
				  unsigned int reg)
{
	switch (reg) {
	case STA381xx_CONFA ... STA381xx_L2ATRT:
	case STA381xx_MPCC1 ... STA381xx_FDRC2:
		return 0;
	}
	return 1;
}

static const struct snd_soc_codec_driver STA381xx_codec = {
	.probe =		STA381xx_probe,
	.remove =		STA381xx_remove,
	.suspend =		STA381xx_suspend,
	.resume =		STA381xx_resume,
	.reg_cache_size =	STA381xx_REGISTER_COUNT,
	.reg_word_size =	sizeof(u8),
	.reg_cache_default =	STA381xx_regs,
	.volatile_register =	STA381xx_reg_is_volatile,
	.set_bias_level =	STA381xx_set_bias_level,
	.controls =		STA381xx_snd_controls,
	.num_controls =		ARRAY_SIZE(STA381xx_snd_controls),
	.dapm_widgets =		STA381xx_dapm_widgets,
	.num_dapm_widgets =	ARRAY_SIZE(STA381xx_dapm_widgets),
	.dapm_routes =		STA381xx_dapm_routes,
	.num_dapm_routes =	ARRAY_SIZE(STA381xx_dapm_routes),
};

static __devinit int STA381xx_i2c_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	struct STA381xx_priv *STA381xx;
	int ret;
printk("~~~~~~STA381xx_i2c_probe~~~~~~~~~\n");
	STA381xx = devm_kzalloc(&i2c->dev, sizeof(struct STA381xx_priv),
			      GFP_KERNEL);
	if (!STA381xx)
		return -ENOMEM;

	i2c_set_clientdata(i2c, STA381xx);

	ret = snd_soc_register_codec(&i2c->dev, &STA381xx_codec, 
			&STA381xx_dai, 1);
	if (ret != 0){
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);
		devm_kfree(&i2c->dev, STA381xx);
	}
	return ret;
}

static __devexit int STA381xx_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	devm_kfree(&client->dev, i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id STA381xx_i2c_id[] = {
	{ "sta381BW", 0 },
	{ "sta380BW", 0 },
	{ "sta381BWS", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, STA381xx_i2c_id);

static struct i2c_driver STA381xx_i2c_driver = {
	.driver = {
		.name = "sta381xx",
		.owner = THIS_MODULE,
	},
	.probe =    STA381xx_i2c_probe,
	.remove =   __devexit_p(STA381xx_i2c_remove),
	.id_table = STA381xx_i2c_id,
};

static int __init STA381xx_init(void)
{
	return i2c_add_driver(&STA381xx_i2c_driver);
}
module_init(STA381xx_init);

static void __exit STA381xx_exit(void)
{
	i2c_del_driver(&STA381xx_i2c_driver);
}
module_exit(STA381xx_exit);

MODULE_DESCRIPTION("ASoC STA381xx driver");
MODULE_AUTHOR("Johannes Stezenbach <js@sig21.net>");
MODULE_LICENSE("GPL");
