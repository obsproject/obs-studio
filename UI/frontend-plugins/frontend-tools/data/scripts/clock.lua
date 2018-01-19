obs = obslua

source_def = {}
source_def.id = "clock_source"
source_def.output_flags = obs.OBS_SOURCE_VIDEO

source_def.get_name = function()
	return "Lua Clock"
end

source_def.create = function(source, settings)
	local data = {}
	data.image = obs.gs_image_file()
	data.hour_image = obs.gs_image_file()
	data.minute_image = obs.gs_image_file()
	data.second_image = obs.gs_image_file()

	image_source_load(data.image, script_path() .. "clock/dial.png")
	image_source_load(data.hour_image, script_path() .. "clock/hour.png")
	image_source_load(data.minute_image, script_path() .. "clock/minute.png")
	image_source_load(data.second_image, script_path() .. "clock/second.png")

	return data
end

source_def.video_tick = function(data, seconds)

end

image_source_load = function(image, file)
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

source_def.update = function(data, settings)
	image_source_load(data)
end

source_def.video_render = function(data, effect)
	if not data.image.texture then
		return;
	end

	local seconds = os.time()
	local hours = math.floor(seconds/3600);
	local mins = math.floor(seconds/60 - (hours*60));
	seconds = seconds % 60

	obs.gs_effect_set_texture(obs.gs_effect_get_param_by_name(effect, "image"), data.image.texture);
	obs.gs_draw_sprite(data.image.texture, 0, data.image.cx, data.image.cy);

	obs.gs_matrix_push()
	obs.gs_matrix_translate3f(500, 500, 0)
	obs.gs_matrix_rotaa4f(0.0, 0.0, 1.0, 2*math.pi/60 * mins);
	obs.gs_matrix_translate3f(-500, -500, 0)

	obs.gs_effect_set_texture(obs.gs_effect_get_param_by_name(effect, "image"), data.minute_image.texture);
	obs.gs_draw_sprite(data.minute_image.texture, 0, data.image.cx, data.image.cy);

	obs.gs_matrix_pop()

	obs.gs_matrix_push()
	obs.gs_matrix_translate3f(500, 500, 0)
	obs.gs_matrix_rotaa4f(0.0, 0.0, 1.0, 2*math.pi/60 * hours);
	obs.gs_matrix_translate3f(-500, -500, 0)

	obs.gs_effect_set_texture(obs.gs_effect_get_param_by_name(effect, "image"), data.hour_image.texture);
	obs.gs_draw_sprite(data.hour_image.texture, 0, data.image.cx, data.image.cy);

	obs.gs_matrix_pop()

	obs.gs_matrix_push()
	obs.gs_matrix_translate3f(500, 500, 0)
	obs.gs_matrix_rotaa4f(0.0, 0.0, 1.0, 2*math.pi/60 * seconds);
	obs.gs_matrix_translate3f(-500, -500, 0)

	obs.gs_effect_set_texture(obs.gs_effect_get_param_by_name(effect, "image"), data.second_image.texture);
	obs.gs_draw_sprite(data.second_image.texture, 0, data.image.cx, data.image.cy);

	obs.gs_matrix_pop()
end

source_def.get_width = function(data)
	return 1000
end

source_def.get_height = function(data)
	return 1000
end

obs.obs_register_source(source_def)
