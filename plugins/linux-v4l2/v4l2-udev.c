/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

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

#include <libudev.h>

#include <util/threading.h>
#include <util/darray.h>
#include <obs.h>

#include "v4l2-udev.h"

#define UDEV_DATA(voidptr) struct v4l2_udev_mon_t *m \
		= (struct v4l2_udev_mon_t *) voidptr;

/** udev action enum */
enum udev_action {
	UDEV_ACTION_ADDED,
	UDEV_ACTION_REMOVED,
	UDEV_ACTION_UNKNOWN
};

/** monitor object holding the callbacks */
struct v4l2_udev_mon_t {
	/* data for the device added callback */
	void *dev_added_userdata;
	v4l2_device_added_cb dev_added_cb;
	/* data for the device removed callback */
	void *dev_removed_userdata;
	v4l2_device_removed_cb dev_removed_cb;
};

/* global data */
static uint_fast32_t udev_refs    = 0;
static pthread_mutex_t udev_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t udev_thread;
static os_event_t *udev_event;

static DARRAY(struct v4l2_udev_mon_t) udev_clients;

/**
 * udev gives us the device action as string, so we convert it here ...
 *
 * @param action the udev action as string
 * @return the udev action as enum value
 */
static enum udev_action udev_action_to_enum(const char *action)
{
	if (!action)
		return UDEV_ACTION_UNKNOWN;

	if (!strncmp("add", action, 3))
		return UDEV_ACTION_ADDED;
	if (!strncmp("remove", action, 6))
		return UDEV_ACTION_REMOVED;

	return UDEV_ACTION_UNKNOWN;
}

/**
 * Call all registered callbacks with the event
 *
 * @param dev udev device that had an event occuring
 */
static inline void udev_call_callbacks(struct udev_device *dev)
{
	const char *node;
	enum udev_action action;

	pthread_mutex_lock(&udev_mutex);

	node   = udev_device_get_devnode(dev);
	action = udev_action_to_enum(udev_device_get_action(dev));

	for (size_t idx = 0; idx < udev_clients.num; idx++) {
		struct v4l2_udev_mon_t *c = &udev_clients.array[idx];

		switch (action) {
		case UDEV_ACTION_ADDED:
			if (!c->dev_added_cb)
				continue;
			c->dev_added_cb(node, c->dev_added_userdata);
			break;
		case UDEV_ACTION_REMOVED:
			if (!c->dev_removed_cb)
				continue;
			c->dev_removed_cb(node, c->dev_removed_userdata);
			break;
		default:
			break;
		}
	}

	pthread_mutex_unlock(&udev_mutex);
}

/**
 * Event listener thread
 */
static void *udev_event_thread(void *vptr)
{
	UNUSED_PARAMETER(vptr);

	int fd;
	fd_set fds;
	struct timeval tv;
	struct udev *udev;
	struct udev_monitor *mon;
	struct udev_device *dev;

	/* set up udev monitoring */
	udev = udev_new();
	mon  = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(
			mon, "video4linux", NULL);
	if (udev_monitor_enable_receiving(mon) < 0)
		return NULL;

	/* set up fds */
	fd = udev_monitor_get_fd(mon);

	while (os_event_try(udev_event) == EAGAIN) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec  = 1;
		tv.tv_usec = 0;

		if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
			continue;

		dev = udev_monitor_receive_device(mon);
		if (!dev)
			continue;

		udev_call_callbacks(dev);

		udev_device_unref(dev);
	}

	udev_monitor_unref(mon);
	udev_unref(udev);

	return NULL;
}

void *v4l2_init_udev(void)
{
	struct v4l2_udev_mon_t *ret = NULL;

	pthread_mutex_lock(&udev_mutex);

	/* set up udev */
	if (udev_refs == 0) {
		if (os_event_init(&udev_event, OS_EVENT_TYPE_MANUAL) != 0)
			goto fail;
		if (pthread_create(&udev_thread, NULL, udev_event_thread,
				NULL) != 0)
			goto fail;
		da_init(udev_clients);
	}
	udev_refs++;

	/* create monitor object */
	ret = da_push_back_new(udev_clients);
fail:
	pthread_mutex_unlock(&udev_mutex);
	return ret;
}

void v4l2_unref_udev(void *monitor)
{
	UDEV_DATA(monitor);
	pthread_mutex_lock(&udev_mutex);

	/* clean up monitor object */
	da_erase_item(udev_clients, m);

	/* unref udev monitor */
	udev_refs--;
	if (udev_refs == 0) {
		os_event_signal(udev_event);
		pthread_join(udev_thread, NULL);
		os_event_destroy(udev_event);
		da_free(udev_clients);
	}

	pthread_mutex_unlock(&udev_mutex);
}

void v4l2_set_device_added_callback(void *monitor, v4l2_device_added_cb cb,
		void *userdata)
{
	UDEV_DATA(monitor);
	if (!m)
		return;

	m->dev_added_cb = cb;
	m->dev_added_userdata = userdata;
}

void v4l2_set_device_removed_callback(void *monitor, v4l2_device_removed_cb cb,
		void *userdata)
{
	UDEV_DATA(monitor);
	if (!m)
		return;

	m->dev_removed_cb = cb;
	m->dev_removed_userdata = userdata;
}
