/******************************************************************************
    Copyright (C) 2019 by Jason Francis <cycl0ps@tuta.io>

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
******************************************************************************/

#include "obs-internal.h"
#include "obs-nix-platform.h"
#include "obs-nix-wayland.h"

#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

// X11 only supports 256 scancodes, most keyboards dont have 256 keys so this should be reasonable.
#define MAX_KEYCODES 256
// X11 keymaps only have 4 shift levels, im not sure xkbcommon supports a way to shift the state into a higher level anyway.
#define MAX_SHIFT_LEVELS 4

struct obs_hotkeys_platform {
	struct wl_display *display;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;
	xkb_keysym_t key_to_sym[MAX_SHIFT_LEVELS][MAX_KEYCODES];
	xkb_keysym_t obs_to_key[OBS_KEY_LAST_VALUE];
	uint32_t current_layout;
};

static obs_key_t obs_nix_wayland_key_from_virtual_key(int sym);

static void load_keymap_data(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
	obs_hotkeys_platform_t *plat = (obs_hotkeys_platform_t *)data;
	if (key >= MAX_KEYCODES)
		return;

	const xkb_keysym_t *syms;
	for (int level = 0; level < MAX_SHIFT_LEVELS; level++) {
		int nsyms = xkb_keymap_key_get_syms_by_level(keymap, key, plat->current_layout, level, &syms);
		if (nsyms < 1)
			continue;

		obs_key_t obs_key = obs_nix_wayland_key_from_virtual_key(syms[0]);
		// This avoids ambiguity where multiple scancodes produce the same symbols.
		// e.g. LSGT and Shift+AB08 produce `<` on default US layout.
		if (!plat->obs_to_key[obs_key])
			plat->obs_to_key[obs_key] = key;
		plat->key_to_sym[level][key] = syms[0];
	}
}

static void rebuild_keymap_data(obs_hotkeys_platform_t *plat)
{
	memset(plat->key_to_sym, 0, sizeof(xkb_keysym_t) * MAX_SHIFT_LEVELS * MAX_KEYCODES);
	memset(plat->obs_to_key, 0, sizeof(xkb_keysym_t) * OBS_KEY_LAST_VALUE);
	xkb_keymap_key_for_each(plat->xkb_keymap, load_keymap_data, plat);
}

static void platform_keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd,
				     uint32_t size)
{
	UNUSED_PARAMETER(keyboard);
	UNUSED_PARAMETER(format);
	obs_hotkeys_platform_t *plat = (obs_hotkeys_platform_t *)data;

	char *keymap_shm = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (keymap_shm == MAP_FAILED) {
		close(fd);
		return;
	}

	struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(
		plat->xkb_context, keymap_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(keymap_shm, size);
	close(fd);

	// cleanup old keymap and state
	xkb_keymap_unref(plat->xkb_keymap);
	xkb_state_unref(plat->xkb_state);

	plat->xkb_keymap = xkb_keymap;
	plat->xkb_state = xkb_state_new(xkb_keymap);
	rebuild_keymap_data(plat);
}

static void platform_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial,
					uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
					uint32_t group)
{
	UNUSED_PARAMETER(keyboard);
	UNUSED_PARAMETER(serial);
	obs_hotkeys_platform_t *plat = (obs_hotkeys_platform_t *)data;
	xkb_state_update_mask(plat->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);

	if (plat->current_layout != group) {
		plat->current_layout = group;
		rebuild_keymap_data(plat);
	}
}

static void platform_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time,
				  uint32_t key, uint32_t state)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(keyboard);
	UNUSED_PARAMETER(serial);
	UNUSED_PARAMETER(time);
	UNUSED_PARAMETER(key);
	UNUSED_PARAMETER(state);
	// We have access to the keyboard input here, but behave like other
	// platforms and let Qt inform us of key events through the platform
	// callbacks.
}

static void platform_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial,
				    struct wl_surface *surface, struct wl_array *keys)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(keyboard);
	UNUSED_PARAMETER(serial);
	UNUSED_PARAMETER(surface);
	UNUSED_PARAMETER(keys);
	// Nothing to do here.
}

static void platform_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial,
				    struct wl_surface *surface)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(keyboard);
	UNUSED_PARAMETER(serial);
	UNUSED_PARAMETER(surface);
	// Nothing to do.
}

static void platform_keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(keyboard);
	UNUSED_PARAMETER(rate);
	UNUSED_PARAMETER(delay);
	// Nothing to do.
}

const struct wl_keyboard_listener keyboard_listener = {
	.keymap = platform_keyboard_keymap,
	.enter = platform_keyboard_enter,
	.leave = platform_keyboard_leave,
	.key = platform_keyboard_key,
	.modifiers = platform_keyboard_modifiers,
	.repeat_info = platform_keyboard_repeat_info,
};

static void platform_seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities)
{
	UNUSED_PARAMETER(seat);
	obs_hotkeys_platform_t *plat = (obs_hotkeys_platform_t *)data;

	bool kb_present = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

	if (kb_present && plat->keyboard == NULL) {
		plat->keyboard = wl_seat_get_keyboard(plat->seat);
		wl_keyboard_add_listener(plat->keyboard, &keyboard_listener, plat);
	} else if (!kb_present && plat->keyboard != NULL) {
		wl_keyboard_release(plat->keyboard);
		plat->keyboard = NULL;
	}
}
static void platform_seat_name(void *data, struct wl_seat *seat, const char *name)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(seat);
	UNUSED_PARAMETER(name);
	// Nothing to do.
}

const struct wl_seat_listener seat_listener = {
	.capabilities = platform_seat_capabilities,
	.name = platform_seat_name,
};

static void platform_registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface,
				      uint32_t version)
{
	obs_hotkeys_platform_t *plat = (obs_hotkeys_platform_t *)data;

	if (strcmp(interface, wl_seat_interface.name) == 0) {
		if (version < 4) {
			blog(LOG_WARNING, "[wayland] hotkeys disabled, compositor is too old");
			return;
		}
		// Only negotiate up to version 7, the current wl_seat at time of writing.
		plat->seat = wl_registry_bind(registry, id, &wl_seat_interface, version <= 7 ? version : 7);
		wl_seat_add_listener(plat->seat, &seat_listener, plat);
	}
}

static void platform_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(registry);
	UNUSED_PARAMETER(id);
	// Nothing to do.
}

const struct wl_registry_listener registry_listener = {
	.global = platform_registry_handler,
	.global_remove = platform_registry_remover,
};

void obs_nix_wayland_log_info(void)
{
	struct wl_display *display = obs_get_nix_platform_display();
	if (display == NULL) {
		blog(LOG_INFO, "Unable to connect to Wayland server");
		return;
	}
	//TODO: query some information about the wayland server if possible
	blog(LOG_INFO, "Connected to Wayland server");
}

static bool obs_nix_wayland_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys)
{
	struct wl_display *display = obs_get_nix_platform_display();
	hotkeys->platform_context = bzalloc(sizeof(obs_hotkeys_platform_t));
	hotkeys->platform_context->display = display;
	hotkeys->platform_context->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, hotkeys->platform_context);
	wl_display_roundtrip(display);
	return true;
}

static void obs_nix_wayland_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys)
{
	obs_hotkeys_platform_t *plat = hotkeys->platform_context;
	xkb_context_unref(plat->xkb_context);
	xkb_keymap_unref(plat->xkb_keymap);
	xkb_state_unref(plat->xkb_state);
	bfree(plat);
}

static bool obs_nix_wayland_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *context, obs_key_t key)
{
	UNUSED_PARAMETER(context);
	UNUSED_PARAMETER(key);
	// This function is only used by the hotkey thread for capturing out of
	// focus hotkey triggers. Since wayland never delivers key events when out
	// of focus we leave this blank intentionally.
	return false;
}

static void obs_nix_wayland_key_to_str(obs_key_t key, struct dstr *dstr)
{
	if (key >= OBS_KEY_MOUSE1 && key <= OBS_KEY_MOUSE29) {
		if (obs->hotkeys.translations[key]) {
			dstr_copy(dstr, obs->hotkeys.translations[key]);
		} else {
			dstr_printf(dstr, "Mouse %d", (int)(key - OBS_KEY_MOUSE1 + 1));
		}
		return;
	}

	if (key >= OBS_KEY_NUM0 && key <= OBS_KEY_NUM9) {
		if (obs->hotkeys.translations[key]) {
			dstr_copy(dstr, obs->hotkeys.translations[key]);
		} else {
			dstr_printf(dstr, "Numpad %d", (int)(key - OBS_KEY_NUM0));
		}
		return;
	}

#define translate_key(key, def) dstr_copy(dstr, obs_get_hotkey_translation(key, def))

	switch (key) {
	case OBS_KEY_INSERT:
		return translate_key(key, "Insert");
	case OBS_KEY_DELETE:
		return translate_key(key, "Delete");
	case OBS_KEY_HOME:
		return translate_key(key, "Home");
	case OBS_KEY_END:
		return translate_key(key, "End");
	case OBS_KEY_PAGEUP:
		return translate_key(key, "Page Up");
	case OBS_KEY_PAGEDOWN:
		return translate_key(key, "Page Down");
	case OBS_KEY_NUMLOCK:
		return translate_key(key, "Num Lock");
	case OBS_KEY_SCROLLLOCK:
		return translate_key(key, "Scroll Lock");
	case OBS_KEY_CAPSLOCK:
		return translate_key(key, "Caps Lock");
	case OBS_KEY_BACKSPACE:
		return translate_key(key, "Backspace");
	case OBS_KEY_TAB:
		return translate_key(key, "Tab");
	case OBS_KEY_PRINT:
		return translate_key(key, "Print");
	case OBS_KEY_PAUSE:
		return translate_key(key, "Pause");
	case OBS_KEY_LEFT:
		return translate_key(key, "Left");
	case OBS_KEY_RIGHT:
		return translate_key(key, "Right");
	case OBS_KEY_UP:
		return translate_key(key, "Up");
	case OBS_KEY_DOWN:
		return translate_key(key, "Down");
	case OBS_KEY_SHIFT:
		return translate_key(key, "Shift");
	case OBS_KEY_ALT:
		return translate_key(key, "Alt");
	case OBS_KEY_CONTROL:
		return translate_key(key, "Control");
	case OBS_KEY_META:
		return translate_key(key, "Super");
	case OBS_KEY_MENU:
		return translate_key(key, "Menu");
	case OBS_KEY_NUMASTERISK:
		return translate_key(key, "Numpad *");
	case OBS_KEY_NUMPLUS:
		return translate_key(key, "Numpad +");
	case OBS_KEY_NUMMINUS:
		return translate_key(key, "Numpad -");
	case OBS_KEY_NUMCOMMA:
		return translate_key(key, "Numpad ,");
	case OBS_KEY_NUMPERIOD:
		return translate_key(key, "Numpad .");
	case OBS_KEY_NUMSLASH:
		return translate_key(key, "Numpad /");
	case OBS_KEY_SPACE:
		return translate_key(key, "Space");
	case OBS_KEY_ESCAPE:
		return translate_key(key, "Escape");
	default:;
	}

	if (key >= OBS_KEY_F1 && key <= OBS_KEY_F35) {
		dstr_printf(dstr, "F%d", (int)(key - OBS_KEY_F1 + 1));
		return;
	}

	obs_hotkeys_platform_t *plat = obs->hotkeys.platform_context;
	// Translate the obs key back down to shift level 1 and then back to obs key.
	xkb_keycode_t keycode = plat->obs_to_key[key];
	xkb_keysym_t base_sym = plat->key_to_sym[0][keycode];
	if (base_sym != 0) {
		char buf[16] = {0};
		if (xkb_keysym_to_utf8(base_sym, buf, 15)) {
			// Normally obs uses capital letters but we are shift level 1 (lower case).
			dstr_copy(dstr, buf);
		}
	}

	if (key != OBS_KEY_NONE && dstr_is_empty(dstr)) {
		dstr_copy(dstr, obs_key_to_name(key));
	}
}

static obs_key_t obs_nix_wayland_key_from_virtual_key(int sym)
{
	switch (sym) {
	case XKB_KEY_0:
		return OBS_KEY_0;
	case XKB_KEY_1:
		return OBS_KEY_1;
	case XKB_KEY_2:
		return OBS_KEY_2;
	case XKB_KEY_3:
		return OBS_KEY_3;
	case XKB_KEY_4:
		return OBS_KEY_4;
	case XKB_KEY_5:
		return OBS_KEY_5;
	case XKB_KEY_6:
		return OBS_KEY_6;
	case XKB_KEY_7:
		return OBS_KEY_7;
	case XKB_KEY_8:
		return OBS_KEY_8;
	case XKB_KEY_9:
		return OBS_KEY_9;
	case XKB_KEY_A:
		return OBS_KEY_A;
	case XKB_KEY_a:
		return OBS_KEY_A;
	case XKB_KEY_Aacute:
		return OBS_KEY_AACUTE;
	case XKB_KEY_aacute:
		return OBS_KEY_AACUTE;
	case XKB_KEY_Acircumflex:
		return OBS_KEY_ACIRCUMFLEX;
	case XKB_KEY_acircumflex:
		return OBS_KEY_ACIRCUMFLEX;
	case XKB_KEY_acute:
		return OBS_KEY_ACUTE;
	case XKB_KEY_Adiaeresis:
		return OBS_KEY_ADIAERESIS;
	case XKB_KEY_adiaeresis:
		return OBS_KEY_ADIAERESIS;
	case XKB_KEY_AE:
		return OBS_KEY_AE;
	case XKB_KEY_ae:
		return OBS_KEY_AE;
	case XKB_KEY_Agrave:
		return OBS_KEY_AGRAVE;
	case XKB_KEY_agrave:
		return OBS_KEY_AGRAVE;
	case XKB_KEY_ampersand:
		return OBS_KEY_AMPERSAND;
	case XKB_KEY_apostrophe:
		return OBS_KEY_APOSTROPHE;
	case XKB_KEY_Aring:
		return OBS_KEY_ARING;
	case XKB_KEY_aring:
		return OBS_KEY_ARING;
	case XKB_KEY_asciicircum:
		return OBS_KEY_ASCIICIRCUM;
	case XKB_KEY_asciitilde:
		return OBS_KEY_ASCIITILDE;
	case XKB_KEY_asterisk:
		return OBS_KEY_ASTERISK;
	case XKB_KEY_at:
		return OBS_KEY_AT;
	case XKB_KEY_Atilde:
		return OBS_KEY_ATILDE;
	case XKB_KEY_atilde:
		return OBS_KEY_ATILDE;
	case XKB_KEY_B:
		return OBS_KEY_B;
	case XKB_KEY_b:
		return OBS_KEY_B;
	case XKB_KEY_backslash:
		return OBS_KEY_BACKSLASH;
	case XKB_KEY_BackSpace:
		return OBS_KEY_BACKSPACE;
	case XKB_KEY_BackTab:
		return OBS_KEY_BACKTAB;
	case XKB_KEY_bar:
		return OBS_KEY_BAR;
	case XKB_KEY_braceleft:
		return OBS_KEY_BRACELEFT;
	case XKB_KEY_braceright:
		return OBS_KEY_BRACERIGHT;
	case XKB_KEY_bracketleft:
		return OBS_KEY_BRACKETLEFT;
	case XKB_KEY_bracketright:
		return OBS_KEY_BRACKETRIGHT;
	case XKB_KEY_brokenbar:
		return OBS_KEY_BROKENBAR;
	case XKB_KEY_C:
		return OBS_KEY_C;
	case XKB_KEY_c:
		return OBS_KEY_C;
	case XKB_KEY_Cancel:
		return OBS_KEY_CANCEL;
	case XKB_KEY_Ccedilla:
		return OBS_KEY_CCEDILLA;
	case XKB_KEY_ccedilla:
		return OBS_KEY_CCEDILLA;
	case XKB_KEY_cedilla:
		return OBS_KEY_CEDILLA;
	case XKB_KEY_cent:
		return OBS_KEY_CENT;
	case XKB_KEY_Clear:
		return OBS_KEY_CLEAR;
	case XKB_KEY_Codeinput:
		return OBS_KEY_CODEINPUT;
	case XKB_KEY_colon:
		return OBS_KEY_COLON;
	case XKB_KEY_comma:
		return OBS_KEY_COMMA;
	case XKB_KEY_copyright:
		return OBS_KEY_COPYRIGHT;
	case XKB_KEY_currency:
		return OBS_KEY_CURRENCY;
	case XKB_KEY_D:
		return OBS_KEY_D;
	case XKB_KEY_d:
		return OBS_KEY_D;
	case XKB_KEY_dead_abovedot:
		return OBS_KEY_DEAD_ABOVEDOT;
	case XKB_KEY_dead_abovering:
		return OBS_KEY_DEAD_ABOVERING;
	case XKB_KEY_dead_acute:
		return OBS_KEY_DEAD_ACUTE;
	case XKB_KEY_dead_belowdot:
		return OBS_KEY_DEAD_BELOWDOT;
	case XKB_KEY_dead_breve:
		return OBS_KEY_DEAD_BREVE;
	case XKB_KEY_dead_caron:
		return OBS_KEY_DEAD_CARON;
	case XKB_KEY_dead_cedilla:
		return OBS_KEY_DEAD_CEDILLA;
	case XKB_KEY_dead_circumflex:
		return OBS_KEY_DEAD_CIRCUMFLEX;
	case XKB_KEY_dead_diaeresis:
		return OBS_KEY_DEAD_DIAERESIS;
	case XKB_KEY_dead_doubleacute:
		return OBS_KEY_DEAD_DOUBLEACUTE;
	case XKB_KEY_dead_grave:
		return OBS_KEY_DEAD_GRAVE;
	case XKB_KEY_dead_hook:
		return OBS_KEY_DEAD_HOOK;
	case XKB_KEY_dead_horn:
		return OBS_KEY_DEAD_HORN;
	case XKB_KEY_dead_iota:
		return OBS_KEY_DEAD_IOTA;
	case XKB_KEY_dead_macron:
		return OBS_KEY_DEAD_MACRON;
	case XKB_KEY_dead_ogonek:
		return OBS_KEY_DEAD_OGONEK;
	case XKB_KEY_dead_semivoiced_sound:
		return OBS_KEY_DEAD_SEMIVOICED_SOUND;
	case XKB_KEY_dead_tilde:
		return OBS_KEY_DEAD_TILDE;
	case XKB_KEY_dead_voiced_sound:
		return OBS_KEY_DEAD_VOICED_SOUND;
	case XKB_KEY_degree:
		return OBS_KEY_DEGREE;
	case XKB_KEY_Delete:
		return OBS_KEY_DELETE;
	case XKB_KEY_diaeresis:
		return OBS_KEY_DIAERESIS;
	case XKB_KEY_division:
		return OBS_KEY_DIVISION;
	case XKB_KEY_dollar:
		return OBS_KEY_DOLLAR;
	case XKB_KEY_Down:
		return OBS_KEY_DOWN;
	case XKB_KEY_E:
		return OBS_KEY_E;
	case XKB_KEY_e:
		return OBS_KEY_E;
	case XKB_KEY_Eacute:
		return OBS_KEY_EACUTE;
	case XKB_KEY_eacute:
		return OBS_KEY_EACUTE;
	case XKB_KEY_Ecircumflex:
		return OBS_KEY_ECIRCUMFLEX;
	case XKB_KEY_ecircumflex:
		return OBS_KEY_ECIRCUMFLEX;
	case XKB_KEY_Ediaeresis:
		return OBS_KEY_EDIAERESIS;
	case XKB_KEY_ediaeresis:
		return OBS_KEY_EDIAERESIS;
	case XKB_KEY_Egrave:
		return OBS_KEY_EGRAVE;
	case XKB_KEY_egrave:
		return OBS_KEY_EGRAVE;
	case XKB_KEY_Eisu_Shift:
		return OBS_KEY_EISU_SHIFT;
	case XKB_KEY_Eisu_toggle:
		return OBS_KEY_EISU_TOGGLE;
	case XKB_KEY_End:
		return OBS_KEY_END;
	case XKB_KEY_equal:
		return OBS_KEY_EQUAL;
	case XKB_KEY_Escape:
		return OBS_KEY_ESCAPE;
	case XKB_KEY_Eth:
		return OBS_KEY_ETH;
	case XKB_KEY_eth:
		return OBS_KEY_ETH;
	case XKB_KEY_exclam:
		return OBS_KEY_EXCLAM;
	case XKB_KEY_exclamdown:
		return OBS_KEY_EXCLAMDOWN;
	case XKB_KEY_Execute:
		return OBS_KEY_EXECUTE;
	case XKB_KEY_F:
		return OBS_KEY_F;
	case XKB_KEY_f:
		return OBS_KEY_F;
	case XKB_KEY_F1:
		return OBS_KEY_F1;
	case XKB_KEY_F10:
		return OBS_KEY_F10;
	case XKB_KEY_F11:
		return OBS_KEY_F11;
	case XKB_KEY_F12:
		return OBS_KEY_F12;
	case XKB_KEY_F13:
		return OBS_KEY_F13;
	case XKB_KEY_F14:
		return OBS_KEY_F14;
	case XKB_KEY_F15:
		return OBS_KEY_F15;
	case XKB_KEY_F16:
		return OBS_KEY_F16;
	case XKB_KEY_F17:
		return OBS_KEY_F17;
	case XKB_KEY_F18:
		return OBS_KEY_F18;
	case XKB_KEY_F19:
		return OBS_KEY_F19;
	case XKB_KEY_F2:
		return OBS_KEY_F2;
	case XKB_KEY_F20:
		return OBS_KEY_F20;
	case XKB_KEY_F21:
		return OBS_KEY_F21;
	case XKB_KEY_F22:
		return OBS_KEY_F22;
	case XKB_KEY_F23:
		return OBS_KEY_F23;
	case XKB_KEY_F24:
		return OBS_KEY_F24;
	case XKB_KEY_F25:
		return OBS_KEY_F25;
	case XKB_KEY_F26:
		return OBS_KEY_F26;
	case XKB_KEY_F27:
		return OBS_KEY_F27;
	case XKB_KEY_F28:
		return OBS_KEY_F28;
	case XKB_KEY_F29:
		return OBS_KEY_F29;
	case XKB_KEY_F3:
		return OBS_KEY_F3;
	case XKB_KEY_F30:
		return OBS_KEY_F30;
	case XKB_KEY_F31:
		return OBS_KEY_F31;
	case XKB_KEY_F32:
		return OBS_KEY_F32;
	case XKB_KEY_F33:
		return OBS_KEY_F33;
	case XKB_KEY_F34:
		return OBS_KEY_F34;
	case XKB_KEY_F35:
		return OBS_KEY_F35;
	case XKB_KEY_F4:
		return OBS_KEY_F4;
	case XKB_KEY_F5:
		return OBS_KEY_F5;
	case XKB_KEY_F6:
		return OBS_KEY_F6;
	case XKB_KEY_F7:
		return OBS_KEY_F7;
	case XKB_KEY_F8:
		return OBS_KEY_F8;
	case XKB_KEY_F9:
		return OBS_KEY_F9;
	case XKB_KEY_Find:
		return OBS_KEY_FIND;
	case XKB_KEY_G:
		return OBS_KEY_G;
	case XKB_KEY_g:
		return OBS_KEY_G;
	case XKB_KEY_greater:
		return OBS_KEY_GREATER;
	case XKB_KEY_guillemotleft:
		return OBS_KEY_GUILLEMOTLEFT;
	case XKB_KEY_guillemotright:
		return OBS_KEY_GUILLEMOTRIGHT;
	case XKB_KEY_H:
		return OBS_KEY_H;
	case XKB_KEY_h:
		return OBS_KEY_H;
	case XKB_KEY_Hangul:
		return OBS_KEY_HANGUL;
	case XKB_KEY_Hangul_Banja:
		return OBS_KEY_HANGUL_BANJA;
	case XKB_KEY_Hangul_End:
		return OBS_KEY_HANGUL_END;
	case XKB_KEY_Hangul_Hanja:
		return OBS_KEY_HANGUL_HANJA;
	case XKB_KEY_Hangul_Jamo:
		return OBS_KEY_HANGUL_JAMO;
	case XKB_KEY_Hangul_Jeonja:
		return OBS_KEY_HANGUL_JEONJA;
	case XKB_KEY_Hangul_PostHanja:
		return OBS_KEY_HANGUL_POSTHANJA;
	case XKB_KEY_Hangul_PreHanja:
		return OBS_KEY_HANGUL_PREHANJA;
	case XKB_KEY_Hangul_Romaja:
		return OBS_KEY_HANGUL_ROMAJA;
	case XKB_KEY_Hangul_Special:
		return OBS_KEY_HANGUL_SPECIAL;
	case XKB_KEY_Hangul_Start:
		return OBS_KEY_HANGUL_START;
	case XKB_KEY_Hankaku:
		return OBS_KEY_HANKAKU;
	case XKB_KEY_Help:
		return OBS_KEY_HELP;
	case XKB_KEY_Henkan:
		return OBS_KEY_HENKAN;
	case XKB_KEY_Hiragana:
		return OBS_KEY_HIRAGANA;
	case XKB_KEY_Hiragana_Katakana:
		return OBS_KEY_HIRAGANA_KATAKANA;
	case XKB_KEY_Home:
		return OBS_KEY_HOME;
	case XKB_KEY_Hyper_L:
		return OBS_KEY_HYPER_L;
	case XKB_KEY_Hyper_R:
		return OBS_KEY_HYPER_R;
	case XKB_KEY_hyphen:
		return OBS_KEY_HYPHEN;
	case XKB_KEY_I:
		return OBS_KEY_I;
	case XKB_KEY_i:
		return OBS_KEY_I;
	case XKB_KEY_Iacute:
		return OBS_KEY_IACUTE;
	case XKB_KEY_iacute:
		return OBS_KEY_IACUTE;
	case XKB_KEY_Icircumflex:
		return OBS_KEY_ICIRCUMFLEX;
	case XKB_KEY_icircumflex:
		return OBS_KEY_ICIRCUMFLEX;
	case XKB_KEY_Idiaeresis:
		return OBS_KEY_IDIAERESIS;
	case XKB_KEY_idiaeresis:
		return OBS_KEY_IDIAERESIS;
	case XKB_KEY_Igrave:
		return OBS_KEY_IGRAVE;
	case XKB_KEY_igrave:
		return OBS_KEY_IGRAVE;
	case XKB_KEY_Insert:
		return OBS_KEY_INSERT;
	case XKB_KEY_J:
		return OBS_KEY_J;
	case XKB_KEY_j:
		return OBS_KEY_J;
	case XKB_KEY_K:
		return OBS_KEY_K;
	case XKB_KEY_k:
		return OBS_KEY_K;
	case XKB_KEY_Kana_Lock:
		return OBS_KEY_KANA_LOCK;
	case XKB_KEY_Kana_Shift:
		return OBS_KEY_KANA_SHIFT;
	case XKB_KEY_Kanji:
		return OBS_KEY_KANJI;
	case XKB_KEY_Katakana:
		return OBS_KEY_KATAKANA;
	case XKB_KEY_L:
		return OBS_KEY_L;
	case XKB_KEY_l:
		return OBS_KEY_L;
	case XKB_KEY_Left:
		return OBS_KEY_LEFT;
	case XKB_KEY_less:
		return OBS_KEY_LESS;
	case XKB_KEY_M:
		return OBS_KEY_M;
	case XKB_KEY_m:
		return OBS_KEY_M;
	case XKB_KEY_macron:
		return OBS_KEY_MACRON;
	case XKB_KEY_masculine:
		return OBS_KEY_MASCULINE;
	case XKB_KEY_Massyo:
		return OBS_KEY_MASSYO;
	case XKB_KEY_Menu:
		return OBS_KEY_MENU;
	case XKB_KEY_minus:
		return OBS_KEY_MINUS;
	case XKB_KEY_Mode_switch:
		return OBS_KEY_MODE_SWITCH;
	case XKB_KEY_mu:
		return OBS_KEY_MU;
	case XKB_KEY_Muhenkan:
		return OBS_KEY_MUHENKAN;
	case XKB_KEY_MultipleCandidate:
		return OBS_KEY_MULTIPLECANDIDATE;
	case XKB_KEY_multiply:
		return OBS_KEY_MULTIPLY;
	case XKB_KEY_Multi_key:
		return OBS_KEY_MULTI_KEY;
	case XKB_KEY_N:
		return OBS_KEY_N;
	case XKB_KEY_n:
		return OBS_KEY_N;
	case XKB_KEY_nobreakspace:
		return OBS_KEY_NOBREAKSPACE;
	case XKB_KEY_notsign:
		return OBS_KEY_NOTSIGN;
	case XKB_KEY_Ntilde:
		return OBS_KEY_NTILDE;
	case XKB_KEY_ntilde:
		return OBS_KEY_NTILDE;
	case XKB_KEY_numbersign:
		return OBS_KEY_NUMBERSIGN;
	case XKB_KEY_O:
		return OBS_KEY_O;
	case XKB_KEY_o:
		return OBS_KEY_O;
	case XKB_KEY_Oacute:
		return OBS_KEY_OACUTE;
	case XKB_KEY_oacute:
		return OBS_KEY_OACUTE;
	case XKB_KEY_Ocircumflex:
		return OBS_KEY_OCIRCUMFLEX;
	case XKB_KEY_ocircumflex:
		return OBS_KEY_OCIRCUMFLEX;
	case XKB_KEY_Odiaeresis:
		return OBS_KEY_ODIAERESIS;
	case XKB_KEY_odiaeresis:
		return OBS_KEY_ODIAERESIS;
	case XKB_KEY_Ograve:
		return OBS_KEY_OGRAVE;
	case XKB_KEY_ograve:
		return OBS_KEY_OGRAVE;
	case XKB_KEY_onehalf:
		return OBS_KEY_ONEHALF;
	case XKB_KEY_onequarter:
		return OBS_KEY_ONEQUARTER;
	case XKB_KEY_onesuperior:
		return OBS_KEY_ONESUPERIOR;
	case XKB_KEY_Ooblique:
		return OBS_KEY_OOBLIQUE;
	case XKB_KEY_ooblique:
		return OBS_KEY_OOBLIQUE;
	case XKB_KEY_ordfeminine:
		return OBS_KEY_ORDFEMININE;
	case XKB_KEY_Otilde:
		return OBS_KEY_OTILDE;
	case XKB_KEY_otilde:
		return OBS_KEY_OTILDE;
	case XKB_KEY_P:
		return OBS_KEY_P;
	case XKB_KEY_p:
		return OBS_KEY_P;
	case XKB_KEY_paragraph:
		return OBS_KEY_PARAGRAPH;
	case XKB_KEY_parenleft:
		return OBS_KEY_PARENLEFT;
	case XKB_KEY_parenright:
		return OBS_KEY_PARENRIGHT;
	case XKB_KEY_Pause:
		return OBS_KEY_PAUSE;
	case XKB_KEY_percent:
		return OBS_KEY_PERCENT;
	case XKB_KEY_period:
		return OBS_KEY_PERIOD;
	case XKB_KEY_periodcentered:
		return OBS_KEY_PERIODCENTERED;
	case XKB_KEY_plus:
		return OBS_KEY_PLUS;
	case XKB_KEY_plusminus:
		return OBS_KEY_PLUSMINUS;
	case XKB_KEY_PreviousCandidate:
		return OBS_KEY_PREVIOUSCANDIDATE;
	case XKB_KEY_Print:
		return OBS_KEY_PRINT;
	case XKB_KEY_Q:
		return OBS_KEY_Q;
	case XKB_KEY_q:
		return OBS_KEY_Q;
	case XKB_KEY_question:
		return OBS_KEY_QUESTION;
	case XKB_KEY_questiondown:
		return OBS_KEY_QUESTIONDOWN;
	case XKB_KEY_quotedbl:
		return OBS_KEY_QUOTEDBL;
	case XKB_KEY_quoteleft:
		return OBS_KEY_QUOTELEFT;
	case XKB_KEY_R:
		return OBS_KEY_R;
	case XKB_KEY_r:
		return OBS_KEY_R;
	case XKB_KEY_Redo:
		return OBS_KEY_REDO;
	case XKB_KEY_registered:
		return OBS_KEY_REGISTERED;
	case XKB_KEY_Return:
		return OBS_KEY_RETURN;
	case XKB_KEY_Right:
		return OBS_KEY_RIGHT;
	case XKB_KEY_Romaji:
		return OBS_KEY_ROMAJI;
	case XKB_KEY_S:
		return OBS_KEY_S;
	case XKB_KEY_s:
		return OBS_KEY_S;
	case XKB_KEY_section:
		return OBS_KEY_SECTION;
	case XKB_KEY_Select:
		return OBS_KEY_SELECT;
	case XKB_KEY_semicolon:
		return OBS_KEY_SEMICOLON;
	case XKB_KEY_SingleCandidate:
		return OBS_KEY_SINGLECANDIDATE;
	case XKB_KEY_slash:
		return OBS_KEY_SLASH;
	case XKB_KEY_space:
		return OBS_KEY_SPACE;
	case XKB_KEY_ssharp:
		return OBS_KEY_SSHARP;
	case XKB_KEY_sterling:
		return OBS_KEY_STERLING;
	case XKB_KEY_T:
		return OBS_KEY_T;
	case XKB_KEY_t:
		return OBS_KEY_T;
	case XKB_KEY_Tab:
		return OBS_KEY_TAB;
	case XKB_KEY_Thorn:
		return OBS_KEY_THORN;
	case XKB_KEY_thorn:
		return OBS_KEY_THORN;
	case XKB_KEY_threequarters:
		return OBS_KEY_THREEQUARTERS;
	case XKB_KEY_threesuperior:
		return OBS_KEY_THREESUPERIOR;
	case XKB_KEY_Touroku:
		return OBS_KEY_TOUROKU;
	case XKB_KEY_twosuperior:
		return OBS_KEY_TWOSUPERIOR;
	case XKB_KEY_U:
		return OBS_KEY_U;
	case XKB_KEY_u:
		return OBS_KEY_U;
	case XKB_KEY_Uacute:
		return OBS_KEY_UACUTE;
	case XKB_KEY_uacute:
		return OBS_KEY_UACUTE;
	case XKB_KEY_Ucircumflex:
		return OBS_KEY_UCIRCUMFLEX;
	case XKB_KEY_ucircumflex:
		return OBS_KEY_UCIRCUMFLEX;
	case XKB_KEY_Udiaeresis:
		return OBS_KEY_UDIAERESIS;
	case XKB_KEY_udiaeresis:
		return OBS_KEY_UDIAERESIS;
	case XKB_KEY_Ugrave:
		return OBS_KEY_UGRAVE;
	case XKB_KEY_ugrave:
		return OBS_KEY_UGRAVE;
	case XKB_KEY_underscore:
		return OBS_KEY_UNDERSCORE;
	case XKB_KEY_Undo:
		return OBS_KEY_UNDO;
	case XKB_KEY_Up:
		return OBS_KEY_UP;
	case XKB_KEY_V:
		return OBS_KEY_V;
	case XKB_KEY_v:
		return OBS_KEY_V;
	case XKB_KEY_W:
		return OBS_KEY_W;
	case XKB_KEY_w:
		return OBS_KEY_W;
	case XKB_KEY_X:
		return OBS_KEY_X;
	case XKB_KEY_x:
		return OBS_KEY_X;
	case XKB_KEY_Y:
		return OBS_KEY_Y;
	case XKB_KEY_y:
		return OBS_KEY_Y;
	case XKB_KEY_Yacute:
		return OBS_KEY_YACUTE;
	case XKB_KEY_yacute:
		return OBS_KEY_YACUTE;
	case XKB_KEY_Ydiaeresis:
		return OBS_KEY_YDIAERESIS;
	case XKB_KEY_ydiaeresis:
		return OBS_KEY_YDIAERESIS;
	case XKB_KEY_yen:
		return OBS_KEY_YEN;
	case XKB_KEY_Z:
		return OBS_KEY_Z;
	case XKB_KEY_z:
		return OBS_KEY_Z;
	case XKB_KEY_Zenkaku:
		return OBS_KEY_ZENKAKU;
	case XKB_KEY_Zenkaku_Hankaku:
		return OBS_KEY_ZENKAKU_HANKAKU;

	case XKB_KEY_Page_Up:
		return OBS_KEY_PAGEUP;
	case XKB_KEY_Page_Down:
		return OBS_KEY_PAGEDOWN;

	case XKB_KEY_KP_End:
		return OBS_KEY_NUMEND;
	case XKB_KEY_KP_Multiply:
		return OBS_KEY_NUMASTERISK;
	case XKB_KEY_KP_Add:
		return OBS_KEY_NUMPLUS;
	case XKB_KEY_KP_Separator:
		return OBS_KEY_NUMCOMMA;
	case XKB_KEY_KP_Subtract:
		return OBS_KEY_NUMMINUS;
	case XKB_KEY_KP_Decimal:
		return OBS_KEY_NUMPERIOD;
	case XKB_KEY_KP_Divide:
		return OBS_KEY_NUMSLASH;
	case XKB_KEY_KP_Enter:
		return OBS_KEY_ENTER;

	case XKB_KEY_KP_0:
		return OBS_KEY_NUM0;
	case XKB_KEY_KP_1:
		return OBS_KEY_NUM1;
	case XKB_KEY_KP_2:
		return OBS_KEY_NUM2;
	case XKB_KEY_KP_3:
		return OBS_KEY_NUM3;
	case XKB_KEY_KP_4:
		return OBS_KEY_NUM4;
	case XKB_KEY_KP_5:
		return OBS_KEY_NUM5;
	case XKB_KEY_KP_6:
		return OBS_KEY_NUM6;
	case XKB_KEY_KP_7:
		return OBS_KEY_NUM7;
	case XKB_KEY_KP_8:
		return OBS_KEY_NUM8;
	case XKB_KEY_KP_9:
		return OBS_KEY_NUM9;

	case XKB_KEY_XF86AudioPlay:
		return OBS_KEY_VK_MEDIA_PLAY_PAUSE;
	case XKB_KEY_XF86AudioStop:
		return OBS_KEY_VK_MEDIA_STOP;
	case XKB_KEY_XF86AudioPrev:
		return OBS_KEY_VK_MEDIA_PREV_TRACK;
	case XKB_KEY_XF86AudioNext:
		return OBS_KEY_VK_MEDIA_NEXT_TRACK;
	case XKB_KEY_XF86AudioMute:
		return OBS_KEY_VK_VOLUME_MUTE;
	case XKB_KEY_XF86AudioRaiseVolume:
		return OBS_KEY_VK_VOLUME_DOWN;
	case XKB_KEY_XF86AudioLowerVolume:
		return OBS_KEY_VK_VOLUME_UP;
	}
	return OBS_KEY_NONE;
}

static int obs_nix_wayland_key_to_virtual_key(obs_key_t key)
{
	switch (key) {
	case OBS_KEY_0:
		return XKB_KEY_0;
	case OBS_KEY_1:
		return XKB_KEY_1;
	case OBS_KEY_2:
		return XKB_KEY_2;
	case OBS_KEY_3:
		return XKB_KEY_3;
	case OBS_KEY_4:
		return XKB_KEY_4;
	case OBS_KEY_5:
		return XKB_KEY_5;
	case OBS_KEY_6:
		return XKB_KEY_6;
	case OBS_KEY_7:
		return XKB_KEY_7;
	case OBS_KEY_8:
		return XKB_KEY_8;
	case OBS_KEY_9:
		return XKB_KEY_9;
	case OBS_KEY_A:
		return XKB_KEY_A;
	case OBS_KEY_AACUTE:
		return XKB_KEY_Aacute;
	case OBS_KEY_ACIRCUMFLEX:
		return XKB_KEY_Acircumflex;
	case OBS_KEY_ACUTE:
		return XKB_KEY_acute;
	case OBS_KEY_ADIAERESIS:
		return XKB_KEY_Adiaeresis;
	case OBS_KEY_AE:
		return XKB_KEY_AE;
	case OBS_KEY_AGRAVE:
		return XKB_KEY_Agrave;
	case OBS_KEY_AMPERSAND:
		return XKB_KEY_ampersand;
	case OBS_KEY_APOSTROPHE:
		return XKB_KEY_apostrophe;
	case OBS_KEY_ARING:
		return XKB_KEY_Aring;
	case OBS_KEY_ASCIICIRCUM:
		return XKB_KEY_asciicircum;
	case OBS_KEY_ASCIITILDE:
		return XKB_KEY_asciitilde;
	case OBS_KEY_ASTERISK:
		return XKB_KEY_asterisk;
	case OBS_KEY_AT:
		return XKB_KEY_at;
	case OBS_KEY_ATILDE:
		return XKB_KEY_Atilde;
	case OBS_KEY_B:
		return XKB_KEY_B;
	case OBS_KEY_BACKSLASH:
		return XKB_KEY_backslash;
	case OBS_KEY_BACKSPACE:
		return XKB_KEY_BackSpace;
	case OBS_KEY_BACKTAB:
		return XKB_KEY_BackTab;
	case OBS_KEY_BAR:
		return XKB_KEY_bar;
	case OBS_KEY_BRACELEFT:
		return XKB_KEY_braceleft;
	case OBS_KEY_BRACERIGHT:
		return XKB_KEY_braceright;
	case OBS_KEY_BRACKETLEFT:
		return XKB_KEY_bracketleft;
	case OBS_KEY_BRACKETRIGHT:
		return XKB_KEY_bracketright;
	case OBS_KEY_BROKENBAR:
		return XKB_KEY_brokenbar;
	case OBS_KEY_C:
		return XKB_KEY_C;
	case OBS_KEY_CANCEL:
		return XKB_KEY_Cancel;
	case OBS_KEY_CCEDILLA:
		return XKB_KEY_Ccedilla;
	case OBS_KEY_CEDILLA:
		return XKB_KEY_cedilla;
	case OBS_KEY_CENT:
		return XKB_KEY_cent;
	case OBS_KEY_CLEAR:
		return XKB_KEY_Clear;
	case OBS_KEY_CODEINPUT:
		return XKB_KEY_Codeinput;
	case OBS_KEY_COLON:
		return XKB_KEY_colon;
	case OBS_KEY_COMMA:
		return XKB_KEY_comma;
	case OBS_KEY_COPYRIGHT:
		return XKB_KEY_copyright;
	case OBS_KEY_CURRENCY:
		return XKB_KEY_currency;
	case OBS_KEY_D:
		return XKB_KEY_D;
	case OBS_KEY_DEAD_ABOVEDOT:
		return XKB_KEY_dead_abovedot;
	case OBS_KEY_DEAD_ABOVERING:
		return XKB_KEY_dead_abovering;
	case OBS_KEY_DEAD_ACUTE:
		return XKB_KEY_dead_acute;
	case OBS_KEY_DEAD_BELOWDOT:
		return XKB_KEY_dead_belowdot;
	case OBS_KEY_DEAD_BREVE:
		return XKB_KEY_dead_breve;
	case OBS_KEY_DEAD_CARON:
		return XKB_KEY_dead_caron;
	case OBS_KEY_DEAD_CEDILLA:
		return XKB_KEY_dead_cedilla;
	case OBS_KEY_DEAD_CIRCUMFLEX:
		return XKB_KEY_dead_circumflex;
	case OBS_KEY_DEAD_DIAERESIS:
		return XKB_KEY_dead_diaeresis;
	case OBS_KEY_DEAD_DOUBLEACUTE:
		return XKB_KEY_dead_doubleacute;
	case OBS_KEY_DEAD_GRAVE:
		return XKB_KEY_dead_grave;
	case OBS_KEY_DEAD_HOOK:
		return XKB_KEY_dead_hook;
	case OBS_KEY_DEAD_HORN:
		return XKB_KEY_dead_horn;
	case OBS_KEY_DEAD_IOTA:
		return XKB_KEY_dead_iota;
	case OBS_KEY_DEAD_MACRON:
		return XKB_KEY_dead_macron;
	case OBS_KEY_DEAD_OGONEK:
		return XKB_KEY_dead_ogonek;
	case OBS_KEY_DEAD_SEMIVOICED_SOUND:
		return XKB_KEY_dead_semivoiced_sound;
	case OBS_KEY_DEAD_TILDE:
		return XKB_KEY_dead_tilde;
	case OBS_KEY_DEAD_VOICED_SOUND:
		return XKB_KEY_dead_voiced_sound;
	case OBS_KEY_DEGREE:
		return XKB_KEY_degree;
	case OBS_KEY_DELETE:
		return XKB_KEY_Delete;
	case OBS_KEY_DIAERESIS:
		return XKB_KEY_diaeresis;
	case OBS_KEY_DIVISION:
		return XKB_KEY_division;
	case OBS_KEY_DOLLAR:
		return XKB_KEY_dollar;
	case OBS_KEY_DOWN:
		return XKB_KEY_Down;
	case OBS_KEY_E:
		return XKB_KEY_E;
	case OBS_KEY_EACUTE:
		return XKB_KEY_Eacute;
	case OBS_KEY_ECIRCUMFLEX:
		return XKB_KEY_Ecircumflex;
	case OBS_KEY_EDIAERESIS:
		return XKB_KEY_Ediaeresis;
	case OBS_KEY_EGRAVE:
		return XKB_KEY_Egrave;
	case OBS_KEY_EISU_SHIFT:
		return XKB_KEY_Eisu_Shift;
	case OBS_KEY_EISU_TOGGLE:
		return XKB_KEY_Eisu_toggle;
	case OBS_KEY_END:
		return XKB_KEY_End;
	case OBS_KEY_EQUAL:
		return XKB_KEY_equal;
	case OBS_KEY_ESCAPE:
		return XKB_KEY_Escape;
	case OBS_KEY_ETH:
		return XKB_KEY_ETH;
	case OBS_KEY_EXCLAM:
		return XKB_KEY_exclam;
	case OBS_KEY_EXCLAMDOWN:
		return XKB_KEY_exclamdown;
	case OBS_KEY_EXECUTE:
		return XKB_KEY_Execute;
	case OBS_KEY_F:
		return XKB_KEY_F;
	case OBS_KEY_F1:
		return XKB_KEY_F1;
	case OBS_KEY_F10:
		return XKB_KEY_F10;
	case OBS_KEY_F11:
		return XKB_KEY_F11;
	case OBS_KEY_F12:
		return XKB_KEY_F12;
	case OBS_KEY_F13:
		return XKB_KEY_F13;
	case OBS_KEY_F14:
		return XKB_KEY_F14;
	case OBS_KEY_F15:
		return XKB_KEY_F15;
	case OBS_KEY_F16:
		return XKB_KEY_F16;
	case OBS_KEY_F17:
		return XKB_KEY_F17;
	case OBS_KEY_F18:
		return XKB_KEY_F18;
	case OBS_KEY_F19:
		return XKB_KEY_F19;
	case OBS_KEY_F2:
		return XKB_KEY_F2;
	case OBS_KEY_F20:
		return XKB_KEY_F20;
	case OBS_KEY_F21:
		return XKB_KEY_F21;
	case OBS_KEY_F22:
		return XKB_KEY_F22;
	case OBS_KEY_F23:
		return XKB_KEY_F23;
	case OBS_KEY_F24:
		return XKB_KEY_F24;
	case OBS_KEY_F25:
		return XKB_KEY_F25;
	case OBS_KEY_F26:
		return XKB_KEY_F26;
	case OBS_KEY_F27:
		return XKB_KEY_F27;
	case OBS_KEY_F28:
		return XKB_KEY_F28;
	case OBS_KEY_F29:
		return XKB_KEY_F29;
	case OBS_KEY_F3:
		return XKB_KEY_F3;
	case OBS_KEY_F30:
		return XKB_KEY_F30;
	case OBS_KEY_F31:
		return XKB_KEY_F31;
	case OBS_KEY_F32:
		return XKB_KEY_F32;
	case OBS_KEY_F33:
		return XKB_KEY_F33;
	case OBS_KEY_F34:
		return XKB_KEY_F34;
	case OBS_KEY_F35:
		return XKB_KEY_F35;
	case OBS_KEY_F4:
		return XKB_KEY_F4;
	case OBS_KEY_F5:
		return XKB_KEY_F5;
	case OBS_KEY_F6:
		return XKB_KEY_F6;
	case OBS_KEY_F7:
		return XKB_KEY_F7;
	case OBS_KEY_F8:
		return XKB_KEY_F8;
	case OBS_KEY_F9:
		return XKB_KEY_F9;
	case OBS_KEY_FIND:
		return XKB_KEY_Find;
	case OBS_KEY_G:
		return XKB_KEY_G;
	case OBS_KEY_GREATER:
		return XKB_KEY_greater;
	case OBS_KEY_GUILLEMOTLEFT:
		return XKB_KEY_guillemotleft;
	case OBS_KEY_GUILLEMOTRIGHT:
		return XKB_KEY_guillemotright;
	case OBS_KEY_H:
		return XKB_KEY_H;
	case OBS_KEY_HANGUL:
		return XKB_KEY_Hangul;
	case OBS_KEY_HANGUL_BANJA:
		return XKB_KEY_Hangul_Banja;
	case OBS_KEY_HANGUL_END:
		return XKB_KEY_Hangul_End;
	case OBS_KEY_HANGUL_HANJA:
		return XKB_KEY_Hangul_Hanja;
	case OBS_KEY_HANGUL_JAMO:
		return XKB_KEY_Hangul_Jamo;
	case OBS_KEY_HANGUL_JEONJA:
		return XKB_KEY_Hangul_Jeonja;
	case OBS_KEY_HANGUL_POSTHANJA:
		return XKB_KEY_Hangul_PostHanja;
	case OBS_KEY_HANGUL_PREHANJA:
		return XKB_KEY_Hangul_PreHanja;
	case OBS_KEY_HANGUL_ROMAJA:
		return XKB_KEY_Hangul_Romaja;
	case OBS_KEY_HANGUL_SPECIAL:
		return XKB_KEY_Hangul_Special;
	case OBS_KEY_HANGUL_START:
		return XKB_KEY_Hangul_Start;
	case OBS_KEY_HANKAKU:
		return XKB_KEY_Hankaku;
	case OBS_KEY_HELP:
		return XKB_KEY_Help;
	case OBS_KEY_HENKAN:
		return XKB_KEY_Henkan;
	case OBS_KEY_HIRAGANA:
		return XKB_KEY_Hiragana;
	case OBS_KEY_HIRAGANA_KATAKANA:
		return XKB_KEY_Hiragana_Katakana;
	case OBS_KEY_HOME:
		return XKB_KEY_Home;
	case OBS_KEY_HYPER_L:
		return XKB_KEY_Hyper_L;
	case OBS_KEY_HYPER_R:
		return XKB_KEY_Hyper_R;
	case OBS_KEY_HYPHEN:
		return XKB_KEY_hyphen;
	case OBS_KEY_I:
		return XKB_KEY_I;
	case OBS_KEY_IACUTE:
		return XKB_KEY_Iacute;
	case OBS_KEY_ICIRCUMFLEX:
		return XKB_KEY_Icircumflex;
	case OBS_KEY_IDIAERESIS:
		return XKB_KEY_Idiaeresis;
	case OBS_KEY_IGRAVE:
		return XKB_KEY_Igrave;
	case OBS_KEY_INSERT:
		return XKB_KEY_Insert;
	case OBS_KEY_J:
		return XKB_KEY_J;
	case OBS_KEY_K:
		return XKB_KEY_K;
	case OBS_KEY_KANA_LOCK:
		return XKB_KEY_Kana_Lock;
	case OBS_KEY_KANA_SHIFT:
		return XKB_KEY_Kana_Shift;
	case OBS_KEY_KANJI:
		return XKB_KEY_Kanji;
	case OBS_KEY_KATAKANA:
		return XKB_KEY_Katakana;
	case OBS_KEY_L:
		return XKB_KEY_L;
	case OBS_KEY_LEFT:
		return XKB_KEY_Left;
	case OBS_KEY_LESS:
		return XKB_KEY_less;
	case OBS_KEY_M:
		return XKB_KEY_M;
	case OBS_KEY_MACRON:
		return XKB_KEY_macron;
	case OBS_KEY_MASCULINE:
		return XKB_KEY_masculine;
	case OBS_KEY_MASSYO:
		return XKB_KEY_Massyo;
	case OBS_KEY_MENU:
		return XKB_KEY_Menu;
	case OBS_KEY_MINUS:
		return XKB_KEY_minus;
	case OBS_KEY_MODE_SWITCH:
		return XKB_KEY_Mode_switch;
	case OBS_KEY_MU:
		return XKB_KEY_mu;
	case OBS_KEY_MUHENKAN:
		return XKB_KEY_Muhenkan;
	case OBS_KEY_MULTI_KEY:
		return XKB_KEY_Multi_key;
	case OBS_KEY_MULTIPLECANDIDATE:
		return XKB_KEY_MultipleCandidate;
	case OBS_KEY_MULTIPLY:
		return XKB_KEY_multiply;
	case OBS_KEY_N:
		return XKB_KEY_N;
	case OBS_KEY_NOBREAKSPACE:
		return XKB_KEY_nobreakspace;
	case OBS_KEY_NOTSIGN:
		return XKB_KEY_notsign;
	case OBS_KEY_NTILDE:
		return XKB_KEY_Ntilde;
	case OBS_KEY_NUMBERSIGN:
		return XKB_KEY_numbersign;
	case OBS_KEY_O:
		return XKB_KEY_O;
	case OBS_KEY_OACUTE:
		return XKB_KEY_Oacute;
	case OBS_KEY_OCIRCUMFLEX:
		return XKB_KEY_Ocircumflex;
	case OBS_KEY_ODIAERESIS:
		return XKB_KEY_Odiaeresis;
	case OBS_KEY_OGRAVE:
		return XKB_KEY_Ograve;
	case OBS_KEY_ONEHALF:
		return XKB_KEY_onehalf;
	case OBS_KEY_ONEQUARTER:
		return XKB_KEY_onequarter;
	case OBS_KEY_ONESUPERIOR:
		return XKB_KEY_onesuperior;
	case OBS_KEY_OOBLIQUE:
		return XKB_KEY_Ooblique;
	case OBS_KEY_ORDFEMININE:
		return XKB_KEY_ordfeminine;
	case OBS_KEY_OTILDE:
		return XKB_KEY_Otilde;
	case OBS_KEY_P:
		return XKB_KEY_P;
	case OBS_KEY_PARAGRAPH:
		return XKB_KEY_paragraph;
	case OBS_KEY_PARENLEFT:
		return XKB_KEY_parenleft;
	case OBS_KEY_PARENRIGHT:
		return XKB_KEY_parenright;
	case OBS_KEY_PAUSE:
		return XKB_KEY_Pause;
	case OBS_KEY_PERCENT:
		return XKB_KEY_percent;
	case OBS_KEY_PERIOD:
		return XKB_KEY_period;
	case OBS_KEY_PERIODCENTERED:
		return XKB_KEY_periodcentered;
	case OBS_KEY_PLUS:
		return XKB_KEY_plus;
	case OBS_KEY_PLUSMINUS:
		return XKB_KEY_plusminus;
	case OBS_KEY_PREVIOUSCANDIDATE:
		return XKB_KEY_PreviousCandidate;
	case OBS_KEY_PRINT:
		return XKB_KEY_Print;
	case OBS_KEY_Q:
		return XKB_KEY_Q;
	case OBS_KEY_QUESTION:
		return XKB_KEY_question;
	case OBS_KEY_QUESTIONDOWN:
		return XKB_KEY_questiondown;
	case OBS_KEY_QUOTEDBL:
		return XKB_KEY_quotedbl;
	case OBS_KEY_QUOTELEFT:
		return XKB_KEY_quoteleft;
	case OBS_KEY_R:
		return XKB_KEY_R;
	case OBS_KEY_REDO:
		return XKB_KEY_Redo;
	case OBS_KEY_REGISTERED:
		return XKB_KEY_registered;
	case OBS_KEY_RETURN:
		return XKB_KEY_Return;
	case OBS_KEY_RIGHT:
		return XKB_KEY_Right;
	case OBS_KEY_ROMAJI:
		return XKB_KEY_Romaji;
	case OBS_KEY_S:
		return XKB_KEY_S;
	case OBS_KEY_SECTION:
		return XKB_KEY_section;
	case OBS_KEY_SELECT:
		return XKB_KEY_Select;
	case OBS_KEY_SEMICOLON:
		return XKB_KEY_semicolon;
	case OBS_KEY_SINGLECANDIDATE:
		return XKB_KEY_SingleCandidate;
	case OBS_KEY_SLASH:
		return XKB_KEY_slash;
	case OBS_KEY_SPACE:
		return XKB_KEY_space;
	case OBS_KEY_SSHARP:
		return XKB_KEY_ssharp;
	case OBS_KEY_STERLING:
		return XKB_KEY_sterling;
	case OBS_KEY_T:
		return XKB_KEY_T;
	case OBS_KEY_TAB:
		return XKB_KEY_Tab;
	case OBS_KEY_THORN:
		return XKB_KEY_THORN;
	case OBS_KEY_THREEQUARTERS:
		return XKB_KEY_threequarters;
	case OBS_KEY_THREESUPERIOR:
		return XKB_KEY_threesuperior;
	case OBS_KEY_TOUROKU:
		return XKB_KEY_Touroku;
	case OBS_KEY_TWOSUPERIOR:
		return XKB_KEY_twosuperior;
	case OBS_KEY_U:
		return XKB_KEY_U;
	case OBS_KEY_UACUTE:
		return XKB_KEY_Uacute;
	case OBS_KEY_UCIRCUMFLEX:
		return XKB_KEY_Ucircumflex;
	case OBS_KEY_UDIAERESIS:
		return XKB_KEY_Udiaeresis;
	case OBS_KEY_UGRAVE:
		return XKB_KEY_Ugrave;
	case OBS_KEY_UNDERSCORE:
		return XKB_KEY_underscore;
	case OBS_KEY_UNDO:
		return XKB_KEY_Undo;
	case OBS_KEY_UP:
		return XKB_KEY_Up;
	case OBS_KEY_V:
		return XKB_KEY_V;
	case OBS_KEY_W:
		return XKB_KEY_W;
	case OBS_KEY_X:
		return XKB_KEY_X;
	case OBS_KEY_Y:
		return XKB_KEY_Y;
	case OBS_KEY_YACUTE:
		return XKB_KEY_Yacute;
	case OBS_KEY_YDIAERESIS:
		return XKB_KEY_Ydiaeresis;
	case OBS_KEY_YEN:
		return XKB_KEY_yen;
	case OBS_KEY_Z:
		return XKB_KEY_Z;
	case OBS_KEY_ZENKAKU:
		return XKB_KEY_Zenkaku;
	case OBS_KEY_ZENKAKU_HANKAKU:
		return XKB_KEY_Zenkaku_Hankaku;

	case OBS_KEY_PAGEUP:
		return XKB_KEY_Page_Up;
	case OBS_KEY_PAGEDOWN:
		return XKB_KEY_Page_Down;

	case OBS_KEY_NUMEQUAL:
		return XKB_KEY_KP_Equal;
	case OBS_KEY_NUMASTERISK:
		return XKB_KEY_KP_Multiply;
	case OBS_KEY_NUMPLUS:
		return XKB_KEY_KP_Add;
	case OBS_KEY_NUMCOMMA:
		return XKB_KEY_KP_Separator;
	case OBS_KEY_NUMMINUS:
		return XKB_KEY_KP_Subtract;
	case OBS_KEY_NUMPERIOD:
		return XKB_KEY_KP_Decimal;
	case OBS_KEY_NUMSLASH:
		return XKB_KEY_KP_Divide;
	case OBS_KEY_ENTER:
		return XKB_KEY_KP_Enter;

	case OBS_KEY_NUM0:
		return XKB_KEY_KP_0;
	case OBS_KEY_NUM1:
		return XKB_KEY_KP_1;
	case OBS_KEY_NUM2:
		return XKB_KEY_KP_2;
	case OBS_KEY_NUM3:
		return XKB_KEY_KP_3;
	case OBS_KEY_NUM4:
		return XKB_KEY_KP_4;
	case OBS_KEY_NUM5:
		return XKB_KEY_KP_5;
	case OBS_KEY_NUM6:
		return XKB_KEY_KP_6;
	case OBS_KEY_NUM7:
		return XKB_KEY_KP_7;
	case OBS_KEY_NUM8:
		return XKB_KEY_KP_8;
	case OBS_KEY_NUM9:
		return XKB_KEY_KP_9;

	case OBS_KEY_VK_MEDIA_PLAY_PAUSE:
		return XKB_KEY_XF86AudioPlay;
	case OBS_KEY_VK_MEDIA_STOP:
		return XKB_KEY_XF86AudioStop;
	case OBS_KEY_VK_MEDIA_PREV_TRACK:
		return XKB_KEY_XF86AudioPrev;
	case OBS_KEY_VK_MEDIA_NEXT_TRACK:
		return XKB_KEY_XF86AudioNext;
	case OBS_KEY_VK_VOLUME_MUTE:
		return XKB_KEY_XF86AudioMute;
	case OBS_KEY_VK_VOLUME_DOWN:
		return XKB_KEY_XF86AudioRaiseVolume;
	case OBS_KEY_VK_VOLUME_UP:
		return XKB_KEY_XF86AudioLowerVolume;
	default:
		break;
	}
	return 0;
}

static const struct obs_nix_hotkeys_vtable wayland_hotkeys_vtable = {
	.init = obs_nix_wayland_hotkeys_platform_init,
	.free = obs_nix_wayland_hotkeys_platform_free,
	.is_pressed = obs_nix_wayland_hotkeys_platform_is_pressed,
	.key_to_str = obs_nix_wayland_key_to_str,
	.key_from_virtual_key = obs_nix_wayland_key_from_virtual_key,
	.key_to_virtual_key = obs_nix_wayland_key_to_virtual_key,

};

const struct obs_nix_hotkeys_vtable *obs_nix_wayland_get_hotkeys_vtable(void)
{
	return &wayland_hotkeys_vtable;
}
