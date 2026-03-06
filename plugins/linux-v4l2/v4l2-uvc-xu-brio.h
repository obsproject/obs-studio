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

#define LOGITECH_BRIO_VENDOR_PRODUCT "046d:085e"
#define LOGITECH_BRIO_GUID \
	"\x15\x02\xe4\x49\x34\xf4\xfe\x47\xb1\x58\x0e\x88\x50\x23\xe5\x1b"

#define LOGITECH_BRIO_FOV_SEL 0x05

struct uvc_xu_control_query BRIO_QUERIES_FOV_65[] = {
	{.selector = LOGITECH_BRIO_FOV_SEL, .query = UVC_SET_CUR, .size = 1, .data = (unsigned char *)"\x02"}};

struct uvc_xu_control_query BRIO_QUERIES_FOV_78[] = {
	{.selector = LOGITECH_BRIO_FOV_SEL, .query = UVC_SET_CUR, .size = 1, .data = (unsigned char *)"\x01"}};

struct uvc_xu_control_query BRIO_QUERIES_FOV_90[] = {
	{.selector = LOGITECH_BRIO_FOV_SEL, .query = UVC_SET_CUR, .size = 1, .data = (unsigned char *)"\x00"}};

struct menu_item_pos BRIO_MENU_POSITIONS[] = {
	{.label = "65",
	 .queries_cnt = sizeof(BRIO_QUERIES_FOV_65) / sizeof(struct uvc_xu_control_query),
	 .queries = BRIO_QUERIES_FOV_65},
	{.label = "78",
	 .queries_cnt = sizeof(BRIO_QUERIES_FOV_78) / sizeof(struct uvc_xu_control_query),
	 .queries = BRIO_QUERIES_FOV_78},
	{.label = "90",
	 .queries_cnt = sizeof(BRIO_QUERIES_FOV_90) / sizeof(struct uvc_xu_control_query),
	 .queries = BRIO_QUERIES_FOV_90}};

struct menu_item BRIO_MENU[] = {{.id = "brio_fov",
				 .label = "FOV",
				 .positions_cnt = sizeof(BRIO_MENU_POSITIONS) / sizeof(struct menu_item_pos),
				 .positions = BRIO_MENU_POSITIONS}};

#ifdef __cplusplus
}
#endif
