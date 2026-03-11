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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/uvcvideo.h>
#include <linux/usb/video.h>
#include <errno.h>
#include <string.h>

#include "v4l2-uvc-xu-controls.h"
#include "v4l2-uvc-xu-brio.h"
#include "v4l2-uvc-xu-kiyo.h"

/**
 * Checks if device matches matches specific product.
 *
 * @param device_path path to device within /dev directory
 * @param vendor_product_to_compare vendor and product string in "XXXX:YYYY" format
 *
 * @return true if device matches vendor and product string
 */
bool v4l2_compare_vendor_product(const char *device_path, const char *vendor_product_to_compare)
{
	char vendor_path[PATH_MAX];
	char product_path[PATH_MAX];
	char vendor_product[10];
	const char *device_name;
	FILE *fp;
	int len;

	if (device_path != strstr(device_path, "/dev/")) {
		return false;
	}
	device_name = device_path + 5;

	snprintf(vendor_path, PATH_MAX, "/sys/class/video4linux/%s/../../../idVendor", device_name);
	snprintf(product_path, PATH_MAX, "/sys/class/video4linux/%s/../../../idProduct", device_name);

	if (access(vendor_path, F_OK) == 0 && access(product_path, F_OK) == 0) {
		fp = fopen(vendor_path, "r");
		len = fscanf(fp, "%4s", vendor_product);
		fclose(fp);

		fp = fopen(product_path, "r");
		len = fscanf(fp, "%4s", vendor_product + 5);
		fclose(fp);

		vendor_product[4] = ':';
		vendor_product[9] = 0;
		return vendor_product == strstr(vendor_product, vendor_product_to_compare);
	}

	return false;
}

/**
 * Find unit id for device GUID
 *
 * @param device_path path to device within /dev directory
 * @param guid GUID in binary format
 *
 * Descriptors file contains the descriptors in a binary format
 * the byte before the extension guid is the extension unit id
 *
 * @return true if device matches vendor and product string
 */
int v4l2_find_unit_id_in_sysfs(const char *device_path, char *guid)
{
	char descriptors_path[PATH_MAX];

	const char *device_name;
	FILE *fp;

	if (device_path != strstr(device_path, "/dev/")) {
		return -1;
	}
	device_name = device_path + 5;

	snprintf(descriptors_path, PATH_MAX, "/sys/class/video4linux/%s/../../../descriptors", device_name);

	char buf[0x10000];
	if (access(descriptors_path, F_OK) == 0 && access(descriptors_path, F_OK) == 0) {
		fp = fopen(descriptors_path, "r");
		int res = fread(buf, 1, sizeof(buf), fp);
		fclose(fp);

		if (res <= 0) {
			return -1;
		}

		char *pos;
		for (pos = buf; pos < buf + sizeof(buf) - 16; pos++) {
			if (0 == memcmp(pos, guid, 16)) {
				pos--;
				return *pos;
			}
		}
	}

	return -1;
}

static bool v4l2_xu_control_changed(void *data, obs_properties_t *props, obs_property_t *prop, obs_data_t *settings)
{
	(void)prop;
	(void)props;

	int dev = v4l2_open(obs_data_get_string(settings, "device_id"), O_RDWR | O_NONBLOCK);

	if (dev == -1)
		return false;

	struct menu_item *menu = data;
	char pos = obs_data_get_int(settings, menu->id);
	struct menu_item_pos *position = &menu->positions[pos];

	bool ret = false;

	for (int query_no = 0; query_no < position->queries_cnt; query_no++) {
		struct uvc_xu_control_query *xu_query = &position->queries[query_no];
		if (ioctl(dev, UVCIOC_CTRL_QUERY, xu_query) < 0) {
			const char *errstr = strerror(errno);
			blog(LOG_ERROR, "Error changing XU setting: %s", errstr);
		}
	}

	v4l2_close(dev);
	return ret;
}

int_fast32_t v4l2_xu_update_controls(char unit_id, struct menu_item *menu, size_t menu_cnt, obs_properties_t *props)
{
	obs_property_t *prop;

	for (size_t menu_pos = 0; menu_pos < menu_cnt; menu_pos++) {
		struct menu_item *menu_item = &menu[menu_pos];
		prop = obs_properties_add_list(props, menu_item->id, menu_item->label, OBS_COMBO_TYPE_LIST,
					       OBS_COMBO_FORMAT_INT);

		for (int item_pos = 0; item_pos < menu_item->positions_cnt; item_pos++) {
			struct menu_item_pos *menu_item_pos = &menu_item->positions[item_pos];
			obs_property_list_add_int(prop, menu_item_pos->label, item_pos);
			for (int query_pos = 0; query_pos < menu_item_pos->queries_cnt; query_pos++) {
				struct uvc_xu_control_query *query = &menu_item_pos->queries[query_pos];
				query->unit = unit_id;
			}
		}

		obs_property_set_modified_callback2(prop, v4l2_xu_control_changed, menu_item);
	}

	return 0;
}

int_fast32_t v4l2_update_uvc_xu_controls(const char *device_path, obs_properties_t *props)
{
	int unit_id;
	if (!props)
		return -1;

	if (v4l2_compare_vendor_product(device_path, LOGITECH_BRIO_VENDOR_PRODUCT)) {
		unit_id = v4l2_find_unit_id_in_sysfs(device_path, LOGITECH_BRIO_GUID);
		if (-1 == unit_id) {
			blog(LOG_WARNING, "Error getting unit_id for device: %s", device_path);
			return -1;
		}
		return v4l2_xu_update_controls(unit_id, BRIO_MENU, sizeof(BRIO_MENU) / sizeof(struct menu_item), props);
	}

	if (v4l2_compare_vendor_product(device_path, KIYO_PRO_VENDOR_PRODUCT)) {
		unit_id = v4l2_find_unit_id_in_sysfs(device_path, KIYO_PRO_GUID);
		if (-1 == unit_id) {
			blog(LOG_WARNING, "Error getting unit_id for device: %s", device_path);
			return -1;
		}
		return v4l2_xu_update_controls(unit_id, KIYO_PRO_MENU, sizeof(KIYO_PRO_MENU) / sizeof(struct menu_item),
					       props);
	}

	return 0;
}
