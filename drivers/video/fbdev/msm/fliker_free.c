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
u32 copyback = 0;
static bool enabled;
static bool mdss_backlight_enable = false;

static void fliker_free_push_pcc(int r, int g, int b)
{
	struct mdp_pcc_cfg_data pcc_config;
	struct mdp_pcc_data_v1_7 *payload;
	memset(&pcc_config, 0, sizeof(struct mdp_pcc_cfg_data));
	pcc_config.version = mdp_pcc_v1_7;
	pcc_config.block = MDP_LOGICAL_BLOCK_DISP_0;
	pcc_config.ops = enabled ?
		MDP_PP_OPS_WRITE | MDP_PP_OPS_ENABLE :
			MDP_PP_OPS_WRITE | MDP_PP_OPS_DISABLE;
	payload = kzalloc(sizeof(struct mdp_pcc_data_v1_7),GFP_USER);
	pcc_config.r.r = r;
	pcc_config.g.g = g;
	pcc_config.b.b = b;
	payload->r.r = pcc_config.r.r;
	payload->g.g = pcc_config.g.g;
	payload->b.b = pcc_config.b.b;
	pcc_config.cfg_payload = payload;
	
	mdss_mdp_pcc_config(get_mfd_copy(), &pcc_config, &copyback);
	kfree(payload);
}


static void set_rgb_brightness(int r,int g,int b)
{
	r = clamp_t(int, r, MIN_SCALE, MAX_SCALE);
	g = clamp_t(int, g, MIN_SCALE, MAX_SCALE);
	b = clamp_t(int, b, MIN_SCALE, MAX_SCALE);
	
	#if FLIKER_FREE_KLAPSE
	klapse_kcal_push(r,g,b);
	#else 
	fliker_free_push_pcc(r,g,b);
	#endif
}

static void set_brightness(int backlight)
{
    int temp = backlight * (MAX_SCALE - MIN_SCALE) / elvss_off_threshold + MIN_SCALE;
    set_rgb_brightness(temp, temp, temp);
}

u32 mdss_panel_calc_backlight(u32 bl_lvl)
{
	if (mdss_backlight_enable && bl_lvl != 0 && bl_lvl < elvss_off_threshold) {
        printk("fliker free mode on\n");
		printk("elvss_off = %d\n", elvss_off_threshold);
		set_brightness(bl_lvl);
		return elvss_off_threshold;
	}else{
		set_brightness(elvss_off_threshold);
		return bl_lvl;
	}
}


void set_fliker_free(bool enabled)
{
	mdss_backlight_enable = enabled;
	pdata = dev_get_platdata(&get_mfd_copy()->pdev->dev);
	pdata->set_backlight(pdata, mdss_panel_calc_backlight(get_bkl_lvl()));
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
