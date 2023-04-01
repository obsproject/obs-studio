/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

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

#include <string.h>
#include <assert.h>

#include <util/bmem.h>
#include <util/c99defs.h>

#include "obs-internal.h"

struct obs_hotkey_name_map;

typedef struct obs_hotkey_name_map_item obs_hotkey_name_map_item_t;

struct obs_hotkey_name_map_item {
	char *key;
	int val;
	UT_hash_handle hh;
};

static void obs_hotkey_name_map_insert(obs_hotkey_name_map_item_t **hmap,
				       const char *key, int v)
{
	if (!hmap || !key)
		return;

	obs_hotkey_name_map_item_t *t;
	HASH_FIND_STR(*hmap, key, t);
	if (t)
		return;

	t = bzalloc(sizeof(obs_hotkey_name_map_item_t));
	t->key = bstrdup(key);
	t->val = v;

	HASH_ADD_STR(*hmap, key, t);
}

static bool obs_hotkey_name_map_lookup(obs_hotkey_name_map_item_t *hmap,
				       const char *key, int *v)
{
	if (!hmap || !key)
		return false;

	obs_hotkey_name_map_item_t *elem;

	HASH_FIND_STR(hmap, key, elem);

	if (elem) {
		*v = elem->val;
		return true;
	}

	return false;
}

static const char *obs_key_names[] = {
#define OBS_HOTKEY(x) #x,
#include "obs-hotkeys.h"
#undef OBS_HOTKEY
};

const char *obs_key_to_name(obs_key_t key)
{
	if (key >= OBS_KEY_LAST_VALUE) {
		blog(LOG_ERROR,
		     "obs-hotkey.c: queried unknown key "
		     "with code %d",
		     (int)key);
		return "";
	}

	return obs_key_names[key];
}

static obs_key_t obs_key_from_name_fallback(const char *name)
{
#define OBS_HOTKEY(x)              \
	if (strcmp(#x, name) == 0) \
		return x;
#include "obs-hotkeys.h"
#undef OBS_HOTKEY
	return OBS_KEY_NONE;
}

static void init_name_map(void)
{
#define OBS_HOTKEY(x) obs_hotkey_name_map_insert(&obs->hotkeys.name_map, #x, x);
#include "obs-hotkeys.h"
#undef OBS_HOTKEY
}

obs_key_t obs_key_from_name(const char *name)
{
	if (!obs)
		return obs_key_from_name_fallback(name);

	if (pthread_once(&obs->hotkeys.name_map_init_token, init_name_map))
		return obs_key_from_name_fallback(name);

	int v = 0;
	if (obs_hotkey_name_map_lookup(obs->hotkeys.name_map, name, &v))
		return v;

	return OBS_KEY_NONE;
}

void obs_hotkey_name_map_free(void)
{
	if (!obs || !obs->hotkeys.name_map)
		return;

	obs_hotkey_name_map_item_t *root = obs->hotkeys.name_map;
	obs_hotkey_name_map_item_t *n, *tmp;

	HASH_ITER (hh, root, n, tmp) {
		HASH_DEL(root, n);
		bfree(n->key);
		bfree(n);
	}
}
