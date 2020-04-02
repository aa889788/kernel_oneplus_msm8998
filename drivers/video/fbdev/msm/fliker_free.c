/*
 * An fliker free driver based on Qcom MDSS for OLED devices
 *
 * Copyright (C) 2012-2014, The Linux Foundation. All rights reserved.
 * Copyright (C) Sony Mobile Communications Inc. All rights reserved.
 * Copyright (C) 2014-2018, AngeloGioacchino Del Regno <kholk11@gmail.com>
 * Copyright (C) 2018, Devries <therkduan@gmail.com>
 * Copyright (C) 2019-2020, Tanish <tanish2k09.dev@gmail.com>
 * Copyright (C) 2020, shxyke <shxyke@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include "fliker_free.h"
#include "mdss_fb.h"
#if defined(CONFIG_MMI_PANEL_NOTIFICATIONS) && defined(CONFIG_FB)
#include <mach/mmi_panel_notifier.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#include "mdss_mdp.h"

struct mdss_panel_data *pdata;
struct mdss_mdp_ctl *fb0_ctl = 0;
struct mdp_pcc_cfg_data pcc_config;
struct mdp_pcc_data_v1_7 *payload;
struct mdp_dither_cfg_data dither_config;
struct mdp_dither_data_v1_7 *dither_payload;
u32 copyback = 0;
u32 dither_copyback = 0;
static const u32 pcc_depth[9] = {128,256,512,1024,2048,4096,8192,16384,32768};
static u32 depth = 8;
static bool pcc_enabled = false;
static bool dither_enabled = false;
static bool mdss_backlight_enable = false;
static bool use_aod = false;

static int fliker_free_push_dither(int depth)
{
	dither_config.flags = dither_enabled ?
		MDP_PP_OPS_WRITE | MDP_PP_OPS_ENABLE :
			MDP_PP_OPS_WRITE | MDP_PP_OPS_DISABLE;
	dither_config.r_cr_depth = depth;
	dither_config.g_y_depth = depth;
	dither_config.b_cb_depth = depth;
	dither_payload->len = 0;
	dither_payload->temporal_en = 0;
	dither_payload->r_cr_depth = dither_config.r_cr_depth;
	dither_payload->g_y_depth = dither_config.g_y_depth;
	dither_payload->b_cb_depth = dither_config.b_cb_depth;
	dither_config.cfg_payload = dither_payload;

	return mdss_mdp_dither_config(get_mfd_copy(),&dither_config,&dither_copyback,1);
}

static int fliker_free_push_pcc(int temp)
{
	int i,ret = 0;
	pcc_config.ops = pcc_enabled ?
		MDP_PP_OPS_WRITE | MDP_PP_OPS_ENABLE :
			MDP_PP_OPS_WRITE | MDP_PP_OPS_DISABLE;
	pcc_config.r.r = temp;
	pcc_config.g.g = temp;
	pcc_config.b.b = temp;
	payload->r.r = pcc_config.r.r;
	payload->g.g = pcc_config.g.g;
	payload->b.b = pcc_config.b.b;
	pcc_config.cfg_payload = payload;
	
	ret = mdss_mdp_pcc_config(get_mfd_copy(), &pcc_config, &copyback);
	if(mdss_backlight_enable && !ret){
		pcc_config.ops = MDP_PP_OPS_READ;
		pcc_config.r.r = 0;
		for(i=0;i<2000;i++){
			mdss_mdp_pcc_config(get_mfd_copy(), &pcc_config, &copyback);
			if(pcc_config.r.r == temp) return 0;
		}
	}
	return ret;
}

static int set_brightness(int backlight)
{
    int temp = backlight * (MAX_SCALE - MIN_SCALE) / elvss_off_threshold + MIN_SCALE;
	temp = clamp_t(int, temp, MIN_SCALE, MAX_SCALE);
	#if FLIKER_FREE_KLAPSE
    klapse_kcal_push(temp,temp,temp);
	return 0;
	#else 
	for (depth = 8;depth >= 1;depth--){
		if(temp >= pcc_depth[depth]) break;
	}
	fliker_free_push_dither(depth);
	return fliker_free_push_pcc(temp);
	#endif
}

u32 mdss_panel_calc_backlight(u32 bl_lvl)
{
	if (mdss_backlight_enable && bl_lvl != 0 && bl_lvl < elvss_off_threshold) {
        printk("fliker free mode on\n");
		printk("elvss_off = %d\n", elvss_off_threshold);
		if(!set_brightness(bl_lvl))
			return elvss_off_threshold;
	}else{
		if(bl_lvl)
			set_brightness(elvss_off_threshold);
	}
	return bl_lvl;
}


void set_fliker_free(bool enabled)
{
	if(mdss_backlight_enable == enabled) return;
	mdss_backlight_enable = enabled;
	pcc_enabled = enabled;
	dither_enabled = enabled;
	if (get_mfd_copy())
		pdata = dev_get_platdata(&get_mfd_copy()->pdev->dev);
	else return;
	if (enabled){
		if ((pdata) && (pdata->set_backlight))
			pdata->set_backlight(pdata, mdss_panel_calc_backlight(get_bkl_lvl()));
		else return;
	}else{
		if ((pdata) && (pdata->set_backlight)){
			pdata->set_backlight(pdata,get_bkl_lvl());
			mdss_panel_calc_backlight(get_bkl_lvl());
		}else return;
		
	}
	
} 

void set_elvss_off_threshold(int value)
{
	elvss_off_threshold = value;
}

int get_elvss_off_threshold(void)
{
	return elvss_off_threshold;
}

bool if_fliker_free_enabled(void)
{
	return mdss_backlight_enable;
}

bool get_useaod(void)
{
	return use_aod;
}

void set_useaod(bool aod)
{
	use_aod = aod;
}

static int __init fliker_free_init(void)
{
	memset(&pcc_config, 0, sizeof(struct mdp_pcc_cfg_data));
	pcc_config.version = mdp_pcc_v1_7;
	pcc_config.block = MDP_LOGICAL_BLOCK_DISP_0;
	payload = kzalloc(sizeof(struct mdp_pcc_data_v1_7),GFP_USER);
	memset(&dither_config, 0, sizeof(struct mdp_dither_cfg_data));
	dither_config.version = mdp_dither_v1_7;
	dither_config.block = MDP_LOGICAL_BLOCK_DISP_0;
	dither_payload = kzalloc(sizeof(struct mdp_dither_data_v1_7),GFP_USER);
	return 0;
}

static void __exit fliker_free_exit(void)
{
	kfree(payload);
	kfree(dither_payload);
}

late_initcall(fliker_free_init);
module_exit(fliker_free_exit);