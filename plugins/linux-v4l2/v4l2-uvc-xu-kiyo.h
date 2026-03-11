/*
Copyright (C) 2022 by Grzegorz Godlewski

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "v4l2-uvc-xu-controls.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KIYO_PRO_VENDOR_PRODUCT "1532:0e05"
#define KIYO_PRO_GUID \
	"\xd0\x9e\xe4\x23\x78\x11\x31\x4f\xae\x52\xd2\xfb\x8a\x8d\x3b\x48"

#define EU1_SET_ISP 0x01

struct uvc_xu_control_query KIYO_QUERIES_FOV_NARROW[] = {{.selector = EU1_SET_ISP,
							  .query = UVC_SET_CUR,
							  .size = 8,
							  .data = (unsigned char *)"\xff\x01\x00\x03\x01\x00\x00\x00"},
							 {.selector = EU1_SET_ISP,
							  .query = UVC_SET_CUR,
							  .size = 8,
							  .data = (unsigned char *)"\xff\x01\x01\x03\x01\x00\x00\x00"}};

struct uvc_xu_control_query KIYO_QUERIES_FOV_MEDIUM[] = {{.selector = EU1_SET_ISP,
							  .query = UVC_SET_CUR,
							  .size = 8,
							  .data = (unsigned char *)"\xff\x01\x00\x03\x02\x00\x00\x00"},
							 {.selector = EU1_SET_ISP,
							  .query = UVC_SET_CUR,
							  .size = 8,
							  .data = (unsigned char *)"\xff\x01\x01\x03\x02\x00\x00\x00"}};

struct uvc_xu_control_query KIYO_QUERIES_FOV_WIDE[] = {{.selector = EU1_SET_ISP,
							.query = UVC_SET_CUR,
							.size = 8,
							.data = (unsigned char *)"\xff\x01\x00\x03\x00\x00\x00\x00"}};

struct menu_item_pos KIYO_PRO_MENU_FOV[] = {
	{.label = "Narrow",
	 .queries_cnt = sizeof(KIYO_QUERIES_FOV_NARROW) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_FOV_NARROW},
	{.label = "Medium",
	 .queries_cnt = sizeof(KIYO_QUERIES_FOV_MEDIUM) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_FOV_MEDIUM},
	{.label = "Wide",
	 .queries_cnt = sizeof(KIYO_QUERIES_FOV_WIDE) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_FOV_WIDE}};

struct uvc_xu_control_query KIYO_QUERIES_AF_RESPONSIVE[] = {
	{.selector = EU1_SET_ISP,
	 .query = UVC_SET_CUR,
	 .size = 8,
	 .data = (unsigned char *)"\xff\x06\x00\x00\x00\x00\x00\x00"}};

struct uvc_xu_control_query KIYO_QUERIES_AF_PASSIVE[] = {{.selector = EU1_SET_ISP,
							  .query = UVC_SET_CUR,
							  .size = 8,
							  .data = (unsigned char *)"\xff\x06\x01\x00\x00\x00\x00\x00"}};

struct menu_item_pos KIYO_PRO_MENU_AF[] = {
	{.label = "Responsive",
	 .queries_cnt = sizeof(KIYO_QUERIES_AF_RESPONSIVE) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_AF_RESPONSIVE},
	{.label = "Passive",
	 .queries_cnt = sizeof(KIYO_QUERIES_AF_PASSIVE) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_AF_PASSIVE}};

struct uvc_xu_control_query KIYO_QUERIES_HDR_OFF[] = {{.selector = EU1_SET_ISP,
						       .query = UVC_SET_CUR,
						       .size = 8,
						       .data = (unsigned char *)"\xff\x02\x00\x00\x00\x00\x00\x00"}};

struct uvc_xu_control_query KIYO_QUERIES_HDR_ON[] = {{.selector = EU1_SET_ISP,
						      .query = UVC_SET_CUR,
						      .size = 8,
						      .data = (unsigned char *)"\xff\x02\x01\x00\x00\x00\x00\x00"}};

struct uvc_xu_control_query KIYO_QUERIES_HDR_DARK[] = {{.selector = EU1_SET_ISP,
							.query = UVC_SET_CUR,
							.size = 8,
							.data = (unsigned char *)"\xff\x07\x00\x00\x00\x00\x00\x00"}};

struct uvc_xu_control_query KIYO_QUERIES_HDR_BRIGHT[] = {
	{.selector = EU1_SET_ISP, .query = UVC_SET_CUR, .size = 8, .data = (__u8 *)"\xff\x07\x01\x00\x00\x00\x00\x00"}};

struct menu_item_pos KIYO_PRO_MENU_HDR[] = {
	{.label = "On",
	 .queries_cnt = sizeof(KIYO_QUERIES_HDR_ON) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_HDR_ON},
	{.label = "Off",
	 .queries_cnt = sizeof(KIYO_QUERIES_HDR_OFF) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_HDR_OFF}};

struct menu_item_pos KIYO_PRO_MENU_HDR_MODE[] = {
	{.label = "Bright",
	 .queries_cnt = sizeof(KIYO_QUERIES_HDR_BRIGHT) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_HDR_BRIGHT},
	{.label = "Dark",
	 .queries_cnt = sizeof(KIYO_QUERIES_HDR_DARK) / sizeof(struct uvc_xu_control_query),
	 .queries = KIYO_QUERIES_HDR_DARK}};

struct menu_item KIYO_PRO_MENU[] = {{.id = "kiyo_af",
				     .label = "AF Mode",
				     .positions_cnt = sizeof(KIYO_PRO_MENU_AF) / sizeof(struct menu_item_pos),
				     .positions = KIYO_PRO_MENU_AF},
				    {.id = "kiyo_hdr",
				     .label = "HDR",
				     .positions_cnt = sizeof(KIYO_PRO_MENU_HDR) / sizeof(struct menu_item_pos),
				     .positions = KIYO_PRO_MENU_HDR},
				    {.id = "kiyo_hdr_mode",
				     .label = "HDR Mode",
				     .positions_cnt = sizeof(KIYO_PRO_MENU_HDR_MODE) / sizeof(struct menu_item_pos),
				     .positions = KIYO_PRO_MENU_HDR_MODE},
				    {.id = "kiyo_fov",
				     .label = "FOV",
				     .positions_cnt = sizeof(KIYO_PRO_MENU_FOV) / sizeof(struct menu_item_pos),
				     .positions = KIYO_PRO_MENU_FOV}};

#ifdef __cplusplus
}
#endif
