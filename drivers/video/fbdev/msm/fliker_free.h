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

#ifndef _FLIKER_FREE_H
#define _FLIKER_FREE_H

#define FLIKER_FREE_KLAPSE 0


#if FLIKER_FREE_KLAPSE
 #include <linux/klapse.h>
#else
 #define MAX_SCALE 32768 /* Maximum value of RGB possible */

 #define MIN_SCALE 5120 /* Minimum value of RGB recommended */
#endif

/* Constants - Customize as needed */
static int elvss_off_threshold = 66; /* Minimum backlight value that does not fliker */


/* with this function you can set the fliker free into enabled or disabled */
void set_fliker_free(bool enabled);

/* you can use this function to remap the phisical backlight level */
u32 mdss_panel_calc_backlight(u32 bl_lvl);

/* set the minimum backlight value that does not fliker on your device */
void set_elvss_off_threshold(int value);

/* get the current elvss value */
int get_elvss_off_threshold(void);

/* get the current fliker free status (enabled or disabled) */
bool if_fliker_free_enabled(void);

#endif  /* _FLIKER_FREE_H */