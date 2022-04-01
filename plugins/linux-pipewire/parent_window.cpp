/*
	SPDX-License-Identifier: LGPL-3.0-or-later
	SPDX-FileCopyrightText: 2022 David Redondo <kde@david-redondo.de>
*/

#include "parent_window.h"

#include <obs-config.h>
#include <obs-nix-platform.h>
#include <obs-frontend-api.h>

#include <QMainWindow>

#include <string>

#ifdef ENABLE_WAYLAND
#include <QGuiApplication>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QWindow>

#include <wayland-client.h>
#include <xdg-foreign-unstable-v2-client-protocol.h>
#endif

#ifdef ENABLE_WAYLAND

struct exporter_data {
	zxdg_exporter_v2 *xdg_exporter = nullptr;
	uint32_t interface_id = 0;
};

static void registry_global(void *data, wl_registry *registry, uint32_t id,
			    const char *interface, uint32_t version)
{
	if (strcmp(interface, "zxdg_exporter_v2") != 0) {
		return;
	}

	auto exporter = static_cast<exporter_data *>(data);
	void *proxy = wl_registry_bind(registry, id,
				       &zxdg_exporter_v2_interface,
				       std::min(version, 1u));
	exporter->xdg_exporter = static_cast<zxdg_exporter_v2 *>(proxy);
	exporter->interface_id = id;
}
static void registry_global_remove(void *data, wl_registry *registry,
				   uint32_t id)
{
	UNUSED_PARAMETER(registry);
	using std::exchange;
	auto exporter = static_cast<exporter_data *>(data);
	zxdg_exporter_v2_destroy(exchange(exporter->xdg_exporter, nullptr));
}

constexpr wl_registry_listener registry_listener = {
	.global = &registry_global,
	.global_remove = &registry_global_remove,
};

static void xdg_exported_handle(void *data, zxdg_exported_v2 *xdg_exported,
				const char *handle)
{
	auto result = static_cast<std::string *>(data);
	*result = handle;
}

constexpr zxdg_exported_v2_listener xdg_exported_listener = {
	.handle = &xdg_exported_handle};

std::string export_toplevel(QMainWindow *main_window)
{
	auto display =
		static_cast<wl_display *>(obs_get_nix_platform_display());
	if (!display) {
		return "";
	}

	static exporter_data exporter = [display] {
		wl_registry *registry = wl_display_get_registry(display);
		wl_registry_add_listener(registry, &registry_listener,
					 &exporter);
		wl_display_roundtrip(display);
		return exporter;
	}();

	if (!exporter.xdg_exporter) {
		return "";
	}

	QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
	QWindow *window = main_window->windowHandle();
	auto surface = native->nativeResourceForWindow("surface", window);
	if (!window || !window->isVisible() || !surface) {
		return "";
	}
	auto wayland_surface = static_cast<wl_surface *>(surface);
	auto exported = zxdg_exporter_v2_export_toplevel(exporter.xdg_exporter,
							 wayland_surface);
	std::string handle;
	zxdg_exported_v2_add_listener(exported, &xdg_exported_listener,
				      &handle);
	wl_display_roundtrip(display);
	return handle;
}
#endif

dstr parent_window_string()
{
	dstr identifier;
	dstr_init_copy(&identifier, "");
	auto window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!window) {
		return identifier;
	}

	switch (obs_get_nix_platform()) {
	case OBS_NIX_PLATFORM_X11_EGL:
	case OBS_NIX_PLATFORM_X11_GLX:
		dstr_printf(&identifier, "x11:%llx", window->winId());
		break;
#ifdef ENABLE_WAYLAND
	case OBS_NIX_PLATFORM_WAYLAND:
		const std::string handle = export_toplevel(window);
		if (!handle.empty()) {
			dstr_printf(&identifier, "wayland:%s", handle.c_str());
		}
		break;
#endif
	}
	return identifier;
}
