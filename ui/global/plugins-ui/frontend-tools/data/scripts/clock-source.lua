obs = obslua
bit = require("bit")

source_def = {}
source_def.id = "lua_clock_source"
source_def.output_flags = bit.bor(obs.OBS_SOURCE_VIDEO, obs.OBS_SOURCE_CUSTOM_DRAW)

function image_source_load(image, file)
	obs.obs_enter_graphics();
	obs.gs_image_file_free(image);
	obs.obs_leave_graphics();

	obs.gs_image_file_init(image, file);

	obs.obs_enter_graphics();
	obs.gs_image_file_init_texture(image);
	obs.obs_leave_graphics();

	if not image.loaded then
		print("failed to load texture " .. file);
	end
end

source_def.get_name = function()
	return "Lua Clock"
end

source_def.create = function(source, settings)
	local data = {}
	data.image = obs.gs_image_file()
	data.hour_image = obs.gs_image_file()
	data.minute_image = obs.gs_image_file()
	data.second_image = obs.gs_image_file()

	image_source_load(data.image, script_path() .. "clock-source/dial.png")
	image_source_load(data.hour_image, script_path() .. "clock-source/hour.png")
	image_source_load(data.minute_image, script_path() .. "clock-source/minute.png")
	image_source_load(data.second_image, script_path() .. "clock-source/second.png")

	return data
end

source_def.destroy = function(data)
	obs.obs_enter_graphics();
	obs.gs_image_file_free(data.image);
	obs.gs_image_file_free(data.hour_image);
	obs.gs_image_file_free(data.minute_image);
	obs.gs_image_file_free(data.second_image);
	obs.obs_leave_graphics();
end

source_def.video_render = function(data, effect)
	if not data.image.texture then
		return;
	end

	local time = os.date("*t")
	local seconds = time.sec
	local mins = time.min + seconds / 60.0;
	local hours = time.hour + (mins * 60.0) / 3600.0;

	effect = obs.obs_get_base_effect(obs.OBS_EFFECT_DEFAULT)

	obs.gs_blend_state_push()
	obs.gs_reset_blend_state()

	while obs.gs_effect_loop(effect, "Draw") do
		obs.obs_source_draw(data.image.texture, 0, 0, data.image.cx, data.image.cy, false);
	end

	obs.gs_matrix_push()
	obs.gs_matrix_translate3f(250, 250, 0)
	obs.gs_matrix_rotaa4f(0.0, 0.0, 1.0, 2 * math.pi / 60 * mins);
	obs.gs_matrix_translate3f(-250, -250, 0)

	while obs.gs_effect_loop(effect, "Draw") do
		obs.obs_source_draw(data.minute_image.texture, 0, 0, data.image.cx, data.image.cy, false);
	end

	obs.gs_matrix_pop()

	obs.gs_matrix_push()
	obs.gs_matrix_translate3f(250, 250, 0)
	obs.gs_matrix_rotaa4f(0.0, 0.0, 1.0, 2.0 * math.pi / 12 * hours);
	obs.gs_matrix_translate3f(-250, -250, 0)

	while obs.gs_effect_loop(effect, "Draw") do
		obs.obs_source_draw(data.hour_image.texture, 0, 0, data.image.cx, data.image.cy, false);
	end

	obs.gs_matrix_pop()

	obs.gs_matrix_push()
	obs.gs_matrix_translate3f(250, 250, 0)
	obs.gs_matrix_rotaa4f(0.0, 0.0, 1.0, 2 * math.pi / 60 * seconds);
	obs.gs_matrix_translate3f(-250, -250, 0)

	while obs.gs_effect_loop(effect, "Draw") do
		obs.obs_source_draw(data.second_image.texture, 0, 0, data.image.cx, data.image.cy, false);
	end

	obs.gs_matrix_pop()

	obs.gs_blend_state_pop()
end

source_def.get_width = function(data)
	return 500
end

source_def.get_height = function(data)
	return 500
end

function script_description()
	return "Adds a \"Lua Clock\" source which draws an animated analog clock."
end

obs.obs_register_source(source_def)
