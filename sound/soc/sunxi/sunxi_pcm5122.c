/*
 * sunxi_pcm5122.c  --  SoC audio for Allwinner
 *
 * Driver for Allwinner pcm5122 audio
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

static int sunxi_pcm5122_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq = 22579200;

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct sunxi_tdm_info  *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned long sample_rate = params_rate(params);

	/*Codec is slave*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		printk("TJB: sunxi_pcm5122_hw_params - failed codec is slave\n");
		return ret;
	}

	/*Cpu is master*/
	ret = snd_soc_dai_set_fmt(cpu_dai, (sunxi_daudio->audio_format| (sunxi_daudio->signal_inversion <<8) | (sunxi_daudio->daudio_master <<12)));
	if (ret < 0) {
		printk("TJB: sunxi_pcm5122_hw_params - failed cpu is master\n");
		return ret;
	}

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
	printk("TJB: sunxi_pcm5122_hw_params - selected freq = %d\n", freq);

	/*Set the sysclk for the cpu side*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, 0);
	if (ret < 0) {
		printk("TJB: sunxi_pcm5122_hw_params - failed set cpu freq\n");
		return ret;
	}

//	/*Set the sysclk for the codec side*/
//	ret = snd_soc_dai_set_sysclk(codec_dai, 0 , freq, 0);
//	if (ret < 0) {
//		printk("TJB: sunxi_pcm5122_hw_params - failed set codec freq\n");
//		return ret;
//	}

	/*Set the cpu clock dividers*/
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		printk("TJB: sunxi_pcm5122_hw_params - failed set cpu clkdiv\n");
		return ret;
	}

//	/*Set the codec clock dividers*/
//	ret = snd_soc_dai_set_clkdiv(codec_dai, 0, sample_rate);
//	if (ret < 0) {
//		printk("TJB: sunxi_pcm5122_hw_params - failed set codec clkdiv\n");
//		return ret;
//	}

#if 0
	/*Set cpu clock divider*/
	snd_soc_dai_set_clkdiv(cpu_dai, SUNXI_DAUDIO_DIV_BCLK, 64-1);//bclk = 2*32*lrck; 2*32fs
	switch(params_rate(params)) {
        case 176400:		
		case 192000:
			snd_soc_dai_set_clkdiv(cpu_dai, SUNXI_DAUDIO_DIV_MCLK, 1);	
        	DBG("Enter:%s, %d, MCLK=%d BCLK=%d LRCK=%d\n",
				__FUNCTION__,__LINE__,pll_out,pll_out/2,params_rate(params));			
			break;

		default:
			snd_soc_dai_set_clkdiv(cpu_dai, SUNXI_DAUDIO_DIV_MCLK, 3);	
        	DBG("default:%s, %d, MCLK=%d BCLK=%d LRCK=%d\n",
				__FUNCTION__,__LINE__,pll_out,pll_out/4,params_rate(params));			
			break;
	}
#endif

	return 0;
}

static const struct snd_soc_dapm_widget pcm5122_dapm_widgets[] = {
	
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
	SND_SOC_DAPM_HP("Headphone Jack", NULL),

};

static const struct snd_soc_dapm_route audio_map[]={

	/* Mic Jack --> MIC_IN*/
	{"Mic Bias1", NULL, "Mic Jack"},
	{"MIC1", NULL, "Mic Bias1"},
	/* HP_OUT --> Headphone Jack */
	{"Headphone Jack", NULL, "HPOL"},
	{"Headphone Jack", NULL, "HPOR"},
	/* LINE_OUT --> Ext Speaker */
	{"Ext Spk", NULL, "SPOL"},
	{"Ext Spk", NULL, "SPOR"},

} ;

/*
 * Logic for a pcm5122 as connected on a rockchip board.
 */
static int sunxi_pcm5122_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

        DBG("Enter::%s----%d\n",__FUNCTION__,__LINE__);

        /* Add specific widgets */
	snd_soc_dapm_new_controls(dapm, pcm5122_dapm_widgets,
				  ARRAY_SIZE(pcm5122_dapm_widgets));
	DBG("Enter::%s----%d\n",__FUNCTION__,__LINE__);
        /* Set up specific audio path audio_mapnects */
        snd_soc_dapm_add_routes(dapm, audio_map, ARRAY_SIZE(audio_map));
        DBG("Enter::%s----%d\n",__FUNCTION__,__LINE__);
        snd_soc_dapm_nc_pin(dapm, "HP_L");
        DBG("Enter::%s----%d\n",__FUNCTION__,__LINE__);
	snd_soc_dapm_nc_pin(dapm, "HP_R");
	DBG("Enter::%s----%d\n",__FUNCTION__,__LINE__);
        snd_soc_dapm_sync(dapm);
        DBG("Enter::%s----%d\n",__FUNCTION__,__LINE__);

	return 0;
}

static int sunxi_pcm5122_dai_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct sunxi_tdm_info  *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);
	int ret;

    printk("TJB: sunxi_pcm5122_dai_startup.\n");

	/*Codec is slave*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		pr_warn("[daudio0],the codec_dai set set_fmt failed.\n");
	}

	/*Cpu is master*/
	ret = snd_soc_dai_set_fmt(cpu_dai, (sunxi_daudio->audio_format| (sunxi_daudio->signal_inversion <<8) | (sunxi_daudio->daudio_master <<12)));
	if (ret < 0) {
		return ret;
	}
	
#if 0
	unsigned int dai_fmt = rtd->dai_link->dai_fmt;
    printk("TJB sunxi_pcm5122_dai_startup dai_fmt %x\n", dai_fmt);
	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, dai_fmt);
	if (ret < 0) {
		printk("%s():failed to set the format for codec side\n", __FUNCTION__);
		return ret;
	}
	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, dai_fmt);
	if (ret < 0) {
		printk("%s():failed to set the format for cpu side\n", __FUNCTION__);
		return ret;
	}
#endif
	
	return 0;
}


static struct snd_soc_ops sunxi_pcm5122_ops = {
	  .hw_params = sunxi_pcm5122_hw_params,
	  .startup = sunxi_pcm5122_dai_startup,
};

static struct snd_soc_dai_link sunxi_pcm5122_dai = {
	.name = "pcm5122",
	.stream_name = "SUNXI-TDM0",
	.cpu_dai_name = "sunxi_daudio",
	.codec_name = "pcm512x.2-004c",
	.codec_dai_name = "pcm512x-hifi",
	.platform_name = "sunxi-daudio",
	.init = sunxi_pcm5122_init,
	.ops = &sunxi_pcm5122_ops,
};

static struct snd_soc_card sunxi_pcm5122_snd_card = {
	.name = "sndpcm5122",
	.dai_link = &sunxi_pcm5122_dai,
	.num_links = 1,
};

static int sunxi_pcm5122_audio_probe(struct platform_device *pdev)
{

	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &sunxi_pcm5122_snd_card;
	card->dev = &pdev->dev;
	sunxi_pcm5122_dai.cpu_dai_name = NULL;
	sunxi_pcm5122_dai.cpu_of_node = of_parse_phandle(np,
				"sunxi,pcm5122-controller", 0);

	printk("TJB: sunxi_pcm5122_audio_probe\n");
	printk("TJB:sunxi_pcm5122_dai.cpu_of_node = %p\n", sunxi_pcm5122_dai.cpu_of_node);

	if (!sunxi_pcm5122_dai.cpu_of_node) {
		dev_err(&pdev->dev,
			"Property 'sunxi,pcm5122-controller' missing or invalid\n");
			ret = -EINVAL;
	}
	sunxi_pcm5122_dai.platform_name = NULL;
	sunxi_pcm5122_dai.platform_of_node = sunxi_pcm5122_dai.cpu_of_node;

	if (sunxi_pcm5122_dai.codec_dai_name == NULL
			&& sunxi_pcm5122_dai.codec_name == NULL){
			codec_utils_probe(pdev);
			sunxi_pcm5122_dai.codec_dai_name = pdev->name;
			sunxi_pcm5122_dai.codec_name 	= pdev->name;
	}
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n", ret);
	}

	return ret;


/***		
	int ret;
	struct snd_soc_card *card = &sunxi_pcm5122_snd_card;

	card->dev = &pdev->dev;

    printk("TJB sunxi_pcm5122_audio_probe\n");
	ret = sunxi_of_get_sound_card_info(card);
	if (ret) {
		printk("%s() get sound card info failed:%d\n", __FUNCTION__, ret);
		return ret;
	}

	ret = snd_soc_register_card(card);
	if (ret)
		printk("%s() register card failed:%d\n", __FUNCTION__, ret);

	return ret;
***/
}

static int sunxi_pcm5122_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);

	return 0;
}

//#ifdef CONFIG_OF
static const struct of_device_id sunxi_pcm5122_of_match[] = {
	{ .compatible = "allwinner,sunxi-pcm5122-machine", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_pcm5122_of_match);
//#endif /* CONFIG_OF */

static struct platform_driver sunxi_pcm5122_audio_driver = {
	.driver         = {
		.name   = "sndpcm5122",
		.owner  = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(sunxi_pcm5122_of_match),
	},
	.probe	= sunxi_pcm5122_audio_probe,
	.remove	= sunxi_pcm5122_audio_remove,
};

//module_platform_driver(sunxi_pcm5122_audio_driver);

static int __init sunxi_snddaudio0_init(void)
{
	int err = 0;
	if ((err = platform_driver_register(&sunxi_pcm5122_audio_driver)) < 0)
		return err;
	return 0;
}
module_init(sunxi_snddaudio0_init);

static void __exit sunxi_snddaudio0_exit(void)
{
	platform_driver_unregister(&sunxi_pcm5122_audio_driver);
}
module_exit(sunxi_snddaudio0_exit);

/* Module information */
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("Allwinner tdm ASoC Interface");
MODULE_LICENSE("GPL");

