/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "obs-scripting-lua.h"

#define ls_get_libobs_obj(type, lua_index, obs_obj) \
	ls_get_libobs_obj_(script, #type " *", lua_index, obs_obj, NULL, __FUNCTION__, __LINE__)
#define ls_push_libobs_obj(type, obs_obj, ownership) \
	ls_push_libobs_obj_(script, #type " *", obs_obj, ownership, NULL, __FUNCTION__, __LINE__)
#define call_func(func, args, rets) call_func_(script, cb->reg_idx, args, rets, #func, "frontend API")

/* ----------------------------------- */

static int get_scene_names(lua_State *script)
{
	char **names = obs_frontend_get_scene_names();
	char **name = names;
	int i = 0;

	lua_newtable(script);

	while (name && *name) {
		lua_pushstring(script, *name);
		lua_rawseti(script, -2, ++i);
		name++;
	}

	bfree(names);
	return 1;
}

static int get_scenes(lua_State *script)
{
	struct obs_frontend_source_list list = {0};
	obs_frontend_get_scenes(&list);

	lua_newtable(script);

	for (size_t i = 0; i < list.sources.num; i++) {
		obs_source_t *source = list.sources.array[i];
		ls_push_libobs_obj(obs_source_t, source, false);
		lua_rawseti(script, -2, (int)(i + 1));
	}

	da_free(list.sources);
	return 1;
}

static int get_current_scene(lua_State *script)
{
	obs_source_t *source = obs_frontend_get_current_scene();
	ls_push_libobs_obj(obs_source_t, source, false);
	return 1;
}

static int set_current_scene(lua_State *script)
{
	obs_source_t *source = NULL;
	ls_get_libobs_obj(obs_source_t, 1, &source);
	obs_frontend_set_current_scene(source);
	return 0;
}

static int get_transitions(lua_State *script)
{
	struct obs_frontend_source_list list = {0};
	obs_frontend_get_transitions(&list);

	lua_newtable(script);

	for (size_t i = 0; i < list.sources.num; i++) {
		obs_source_t *source = list.sources.array[i];
		ls_push_libobs_obj(obs_source_t, source, false);
		lua_rawseti(script, -2, (int)(i + 1));
	}

	da_free(list.sources);
	return 1;
}

static int get_current_transition(lua_State *script)
{
	obs_source_t *source = obs_frontend_get_current_transition();
	ls_push_libobs_obj(obs_source_t, source, false);
	return 1;
}

static int set_current_transition(lua_State *script)
{
	obs_source_t *source = NULL;
	ls_get_libobs_obj(obs_source_t, 1, &source);
	obs_frontend_set_current_transition(source);
	return 0;
}

static int get_transition_duration(lua_State *script)
{
	int duration = obs_frontend_get_transition_duration();
	lua_pushinteger(script, duration);
	return 1;
}

static int set_transition_duration(lua_State *script)
{
	if (lua_isnumber(script, 1)) {
		int duration = (int)lua_tointeger(script, 1);
		obs_frontend_set_transition_duration(duration);
	}
	return 0;
}

static int get_scene_collections(lua_State *script)
{
	char **names = obs_frontend_get_scene_collections();
	char **name = names;
	int i = 0;

	lua_newtable(script);

	while (name && *name) {
		lua_pushstring(script, *name);
		lua_rawseti(script, -2, ++i);
		name++;
	}

	bfree(names);
	return 1;
}

static int get_current_scene_collection(lua_State *script)
{
	char *name = obs_frontend_get_current_scene_collection();
	lua_pushstring(script, name);
	bfree(name);
	return 1;
}

static int set_current_scene_collection(lua_State *script)
{
	if (lua_isstring(script, 1)) {
		const char *name = lua_tostring(script, 1);
		obs_frontend_set_current_scene_collection(name);
	}
	return 0;
}

static int get_profiles(lua_State *script)
{
	char **names = obs_frontend_get_profiles();
	char **name = names;
	int i = 0;

	lua_newtable(script);

	while (name && *name) {
		lua_pushstring(script, *name);
		lua_rawseti(script, -2, ++i);
		name++;
	}

	bfree(names);
	return 1;
}

static int get_current_profile(lua_State *script)
{
	char *name = obs_frontend_get_current_profile();
	lua_pushstring(script, name);
	bfree(name);
	return 1;
}

static int set_current_profile(lua_State *script)
{
	if (lua_isstring(script, 1)) {
		const char *name = lua_tostring(script, 1);
		obs_frontend_set_current_profile(name);
	}
	return 0;
}

/* ----------------------------------- */

static void frontend_event_callback(enum obs_frontend_event event, void *priv)
{
	struct lua_obs_callback *cb = priv;
	lua_State *script = cb->script;

	if (script_callback_removed(&cb->base)) {
		obs_frontend_remove_event_callback(frontend_event_callback, cb);
		return;
	}

	lock_callback();

	lua_pushinteger(script, (int)event);
	call_func(frontend_event_callback, 1, 0);

	unlock_callback();
}

static int remove_event_callback(lua_State *script)
{
	if (!verify_args1(script, is_function))
		return 0;

	struct lua_obs_callback *cb = find_lua_obs_callback(script, 1);
	if (cb) {
		remove_lua_obs_callback(cb);
	}
	return 0;
}

static void add_event_callback_defer(void *cb)
{
	obs_frontend_add_event_callback(frontend_event_callback, cb);
}

static int add_event_callback(lua_State *script)
{
	if (!verify_args1(script, is_function))
		return 0;

	struct lua_obs_callback *cb = add_lua_obs_callback(script, 1);
	defer_call_post(add_event_callback_defer, cb);
	return 0;
}

/* ----------------------------------- */

static void frontend_save_callback(obs_data_t *save_data, bool saving, void *priv)
{
	struct lua_obs_callback *cb = priv;
	lua_State *script = cb->script;

	if (script_callback_removed(&cb->base)) {
		obs_frontend_remove_save_callback(frontend_save_callback, cb);
		return;
	}

	lock_callback();

	ls_push_libobs_obj(obs_data_t, save_data, false);
	lua_pushboolean(script, saving);
	call_func(frontend_save_callback, 2, 0);

	unlock_callback();
}

static int remove_save_callback(lua_State *script)
{
	if (!verify_args1(script, is_function))
		return 0;

	struct lua_obs_callback *cb = find_lua_obs_callback(script, 1);
	if (cb) {
		remove_lua_obs_callback(cb);
	}
	return 0;
}

static void add_save_callback_defer(void *cb)
{
	obs_frontend_add_save_callback(frontend_save_callback, cb);
}

static int add_save_callback(lua_State *script)
{
	if (!verify_args1(script, is_function))
		return 0;

	struct lua_obs_callback *cb = add_lua_obs_callback(script, 1);
	defer_call_post(add_save_callback_defer, cb);
	return 0;
}

/* ----------------------------------- */

typedef struct lua_function_tuple {
	const char *name;
	lua_CFunction function;
} obs_frontend_func;

static const obs_frontend_func functions[] = {
	{.name = "obs_frontend_get_scene_names", .function = get_scene_names},
	{.name = "obs_frontend_get_scenes", .function = get_scenes},
	{.name = "obs_frontend_get_current_scene", .function = get_current_scene},
	{.name = "obs_frontend_set_current_scene", .function = set_current_scene},
	{.name = "obs_frontend_get_transitions", .function = get_transitions},
	{.name = "obs_frontend_get_current_transition", .function = get_current_transition},
	{.name = "obs_frontend_set_current_transition", .function = set_current_transition},
	{.name = "obs_frontend_get_transition_duration", .function = get_transition_duration},
	{.name = "obs_frontend_set_transition_duration", .function = set_transition_duration},
	{.name = "obs_frontend_get_scene_collections", .function = get_scene_collections},
	{.name = "obs_frontend_get_current_scene_collection", .function = get_current_scene_collection},
	{.name = "obs_frontend_set_current_scene_collection", .function = set_current_scene_collection},
	{.name = "obs_frontend_get_profiles", .function = get_profiles},
	{.name = "obs_frontend_get_current_profile", .function = get_current_profile},
	{.name = "obs_frontend_set_current_profile", .function = set_current_profile},
	{.name = "obs_frontend_remove_event_callback", .function = remove_event_callback},
	{.name = "obs_frontend_add_event_callback", .function = add_event_callback},
	{.name = "obs_frontend_remove_save_callback", .function = remove_save_callback},
	{.name = "obs_frontend_add_save_callback", .function = add_save_callback},
};

void add_lua_frontend_funcs(lua_State *script)
{
	lua_getglobal(script, "obslua");

	size_t num_items = OBS_COUNTOF(functions);

	for (size_t i = 0; i < num_items; i++) {
		lua_pushstring(script, functions[i].name);
		lua_pushcfunction(script, functions[i].function);
		lua_rawset(script, -3);
	}
	lua_pop(script, 1);
}
