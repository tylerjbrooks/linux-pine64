/*
 * sunxi_pcm512x.c  --  SoC audio for Allwinner
 *
 * Driver for Allwinner pcm512x audio
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <linux/of.h>
#include "sunxi_tdm_utils.h"
#include "codec-utils.h"

#if 1
#define	DBG(x...)	printk(KERN_INFO x)
#else
#define	DBG(x...)
#endif

static int sunxi_pcm512x_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq = 22579200;

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct sunxi_tdm_info  *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned long sample_rate = params_rate(params);

	printk("TJB: sunxi_pcm512x_hw_params\n");

	/*Select sample frequency*/
	switch (sample_rate) {
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
			freq = 24576000;
			break;
	}

	/*Codec is slave*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		printk("TJB: sunxi_pcm512x_hw_params - failed codec is slave\n");
		return ret;
	}

	/*Cpu is master*/
	ret = snd_soc_dai_set_fmt(cpu_dai, (sunxi_daudio->audio_format| (sunxi_daudio->signal_inversion <<8) | (sunxi_daudio->daudio_master <<12)));
	if (ret < 0) {
		printk("TJB: sunxi_pcm512x_hw_params - failed cpu is master\n");
		return ret;
	}

	/*Set the sysclk for the cpu side*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, 0);
	if (ret < 0) {
		printk("TJB: sunxi_pcm512x_hw_params - failed set cpu freq\n");
		return ret;
	}

	/*Set the cpu clock dividers*/
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		printk("TJB: sunxi_pcm512x_hw_params - failed set cpu clkdiv\n");
		return ret;
	}

	return 0;
}

static int sunxi_pcm512x_init(struct snd_soc_pcm_runtime *rtd)
{
	printk("TJB: sunxi_pcm512x_init\n");
	return 0;
}

static struct snd_soc_ops sunxi_pcm512x_ops = {
	  .hw_params = sunxi_pcm512x_hw_params,
};

static struct snd_soc_dai_link sunxi_pcm512x_dai = {
	.name = "pcm512x",
	.stream_name = "SUNXI-TDM0",
	.cpu_dai_name = "sunxi-daudio",
	.platform_name = "sunxi-daudio",
	.init = sunxi_pcm512x_init,
	.ops = &sunxi_pcm512x_ops,
};

static struct snd_soc_card sunxi_pcm512x_snd_card = {
	.name = "sndpcm512x",
	.dai_link = &sunxi_pcm512x_dai,
	.num_links = 1,
};

static int sunxi_pcm512x_audio_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &sunxi_pcm512x_snd_card;

	printk("TJB: sunxi_pcm512x_audio_probe: pdev->name = %s\n", pdev->name);

	//	Link the CPU
	sunxi_pcm512x_dai.cpu_dai_name = NULL;
	sunxi_pcm512x_dai.cpu_of_node = of_parse_phandle(np, "sunxi,pcm512x-controller", 0);
	if (!sunxi_pcm512x_dai.cpu_of_node) {
		dev_err(&pdev->dev,
			"Property 'sunxi,pcm512x-controller' missing or invalid\n");
		return -EINVAL;
	}

	//	Link the platform
	sunxi_pcm512x_dai.platform_name = NULL;
	sunxi_pcm512x_dai.platform_of_node = sunxi_pcm512x_dai.cpu_of_node;

	//	Link the codec
	sunxi_pcm512x_dai.codec_name = NULL;
	sunxi_pcm512x_dai.codec_dai_name = "pcm512x";
	sunxi_pcm512x_dai.codec_of_node = of_parse_phandle(np, "digispeaker,codec", 0);
	if (!sunxi_pcm512x_dai.codec_of_node) {
		dev_err(&pdev->dev,
			"Property 'digispeaker,codec' missing or invalid");
		return -EINVAL;
	}

	//	Register the sound card
	card->dev = &pdev->dev;
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n", ret);
	}
	return ret;
}

static int sunxi_pcm512x_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	snd_soc_unregister_card(card);
	return 0;
}

static const struct of_device_id sunxi_pcm512x_of_match[] = {
	{ .compatible = "allwinner,sunxi-pcm512x-machine", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_pcm512x_of_match);

static struct platform_driver sunxi_pcm512x_audio_driver = {
	.driver         = {
		.name   = "sndpcm512x",
		.owner  = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(sunxi_pcm512x_of_match),
	},
	.probe	= sunxi_pcm512x_audio_probe,
	.remove	= sunxi_pcm512x_audio_remove,
};

static int __init sunxi_sndpcm512x_init(void)
{
	int err = 0;
	if ((err = platform_driver_register(&sunxi_pcm512x_audio_driver)) < 0)
		return err;
	return 0;
}
module_init(sunxi_sndpcm512x_init);

static void __exit sunxi_sndpcm512x_exit(void)
{
	platform_driver_unregister(&sunxi_pcm512x_audio_driver);
}
module_exit(sunxi_sndpcm512x_exit);

/* Module information */
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("Allwinner tdm ASoC Interface");
MODULE_LICENSE("GPL");

