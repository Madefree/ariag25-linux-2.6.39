/*
 * sam9x5_wm8731   --	SoC audio for AT91SAM9X5-based boards
 * 			that are using WM8731 as codec.
 *
 *  Copyright (C) 2011 Atmel,
 *  		  Nicolas Ferre <nicolas.ferre@atmel.com>
 *
 * Based on sam9g20_wm8731.c by:
 * Sedji Gaouaou <sedji.gaouaou@atmel.com>
 *
 * GPL
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>

#include <linux/atmel-ssc.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/gpio.h>

#include "../codecs/wm8731.h"
#include "atmel-pcm.h"
#include "atmel_ssc_dai.h"

#define MCLK_RATE 12000000

#define ENABLE_MIC_INPUT
#define ENABLE_LINE_IN

static struct clk *mclk;

static int at91sam9x5ek_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops at91sam9x5ek_ops = {
	.hw_params = at91sam9x5ek_hw_params,
};

static int at91sam9g20ek_set_bias_level(struct snd_soc_card *card,
					enum snd_soc_bias_level level)
{
	static int mclk_on;
	int ret = 0;

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		if (!mclk_on)
			ret = clk_enable(mclk);
		if (ret == 0)
			mclk_on = 1;
		break;

	case SND_SOC_BIAS_OFF:
	case SND_SOC_BIAS_STANDBY:
		if (mclk_on)
			clk_disable(mclk);
		mclk_on = 0;
		break;
	}

	return ret;
}

static const struct snd_soc_dapm_widget at91sam9x5ek_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Int Mic", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
};

static const struct snd_soc_dapm_route intercon[] = {

	/* speaker connected to LHPOUT */
	{"Ext Spk", NULL, "LHPOUT"},

	/* mic is connected to Mic Jack, with WM8731 Mic Bias */
	{"MICIN", NULL, "Mic Bias"},
	{"Mic Bias", NULL, "Int Mic"},
};

/*
 * Logic for a wm8731 as connected on a at91sam9x5 based board.
 */
static int at91sam9x5ek_wm8731_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int ret;

	printk(KERN_DEBUG
			"ASoC: at91sam9x5_wm8731"
			": at91sam9x5ek_wm8731_init() called\n");

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_MCLK,
		MCLK_RATE, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		printk(KERN_ERR "Failed to set WM8731 SYSCLK: %d\n", ret);
		return ret;
	}

	/* Add specific widgets */
	snd_soc_dapm_new_controls(dapm, at91sam9x5ek_dapm_widgets,
				  ARRAY_SIZE(at91sam9x5ek_dapm_widgets));
	/* Set up specific audio path interconnects */
	snd_soc_dapm_add_routes(dapm, intercon, ARRAY_SIZE(intercon));

    /* Set connections */
#ifdef ENABLE_LINE_IN
	snd_soc_dapm_enable_pin(dapm, "RLINEIN");
	snd_soc_dapm_enable_pin(dapm, "LLINEIN");
#else
 	snd_soc_dapm_nc_pin(dapm, "RLINEIN");
 	snd_soc_dapm_nc_pin(dapm, "LLINEIN");
#endif

#ifdef ENABLE_MIC_INPUT
	snd_soc_dapm_enable_pin(dapm, "Int Mic");
#else
	snd_soc_dapm_nc_pin(dapm, "Int Mic");
#endif

	/* always connected */
	snd_soc_dapm_enable_pin(dapm, "Ext Spk");

	snd_soc_dapm_sync(dapm);

	return 0;
}

static struct snd_soc_dai_link at91sam9x5ek_dai = {
	.name = "WM8731",
	.stream_name = "WM8731 PCM",
	.cpu_dai_name = "atmel-ssc-dai.0",
	.codec_dai_name = "wm8731-hifi",
	.init = at91sam9x5ek_wm8731_init,
	.platform_name = "atmel-pcm-audio",
	.codec_name = "wm8731-codec.0-001a",
	.ops = &at91sam9x5ek_ops,
};

static struct snd_soc_card snd_soc_at91sam9x5ek = {
	.name = "AT91SAM9X5",
	.dai_link = &at91sam9x5ek_dai,
	.num_links = 1,
	.set_bias_level = at91sam9g20ek_set_bias_level,
};

static struct platform_device *at91sam9x5ek_snd_device;

static int __init at91sam9x5ek_init(void)
{
    struct clk *pllb;
	int ret;

	if (!machine_is_at91sam9x5ek())
		return -ENODEV;

	ret = atmel_ssc_set_audio(0);
	if (ret != 0) {
		pr_err("ASoC: Failed to set SSC 0 for audio: %d\n", ret);
		return ret;
	}
	
	/*
	 * Codec MCLK is supplied by PCK0 - set it up.
	 */
	mclk = clk_get(NULL, "pck0");
	if (IS_ERR(mclk)) {
		printk(KERN_ERR "ASoC: Failed to get MCLK\n");
		ret = PTR_ERR(mclk);
		goto err;
	}

	pllb = clk_get(NULL, "pllb");
	if (IS_ERR(pllb)) {
		printk(KERN_ERR "ASoC: Failed to get PLLB\n");
		ret = PTR_ERR(pllb);
		goto err_mclk;
	}
	ret = clk_set_parent(mclk, pllb);
	clk_put(pllb);
	if (ret != 0) {
		printk(KERN_ERR "ASoC: Failed to set MCLK parent\n");
		goto err_mclk;
	}

	clk_set_rate(mclk, MCLK_RATE);

	at91sam9x5ek_snd_device = platform_device_alloc("soc-audio", -1);
	if (!at91sam9x5ek_snd_device) {
		printk(KERN_ERR "ASoC: Platform device allocation failed\n");
		ret = -ENOMEM;
		goto err_mclk;
	}

	platform_set_drvdata(at91sam9x5ek_snd_device,
			&snd_soc_at91sam9x5ek);

	ret = platform_device_add(at91sam9x5ek_snd_device);
	if (ret) {
		printk(KERN_ERR "ASoC: Platform device allocation failed\n");
		goto err_device_add;
	}

	printk(KERN_INFO "ASoC: at91sam9x5ek_init ok\n");

	return ret;

err_device_add:
	platform_device_put(at91sam9x5ek_snd_device);
err_mclk:
	clk_put(mclk);
	mclk = NULL;
err:
	return ret;
}

static void __exit at91sam9x5ek_exit(void)
{
	platform_device_unregister(at91sam9x5ek_snd_device);
	at91sam9x5ek_snd_device = NULL;
	clk_put(mclk);
	mclk = NULL;
}

module_init(at91sam9x5ek_init);
module_exit(at91sam9x5ek_exit);

/* Module information */
MODULE_AUTHOR("Nicolas Ferre <nicolas.ferre@atmel.com>");
MODULE_DESCRIPTION("ALSA SoC machine driver for AT91SAM9x5 - WM8731");
MODULE_LICENSE("GPL");
