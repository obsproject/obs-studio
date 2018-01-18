obs         = obslua
source_name = ""
hotkey_id   = obs.OBS_INVALID_HOTKEY_ID

----------------------------------------------------------

-- The "Play Last Replay Buffer" hotkey callback
function play(pressed)
	if not pressed then
		return
	end

	local replay_buffer = obs.obs_frontend_get_replay_buffer_output()
	if replay_buffer == nil then
		return
	end

	local ph = obs.obs_output_get_proc_handler(replay_buffer)

	-- Call the procedure of the replay buffer named "get_last_replay" to
	-- get the last replay created by the replay buffer
	local cd = obs.calldata_create()
	obs.proc_handler_call(ph, "get_last_replay", cd)
	local path = obs.calldata_string(cd, "path")
	obs.calldata_destroy(cd)

	-- If the path is valid and the source exists, update it with the
	-- replay file to play back the replay
	if path ~= nil then
		local source = obs.obs_get_source_by_name(source_name)
		if source ~= nil then
			local settings = obs.obs_data_create()
			obs.obs_data_set_string(settings, "local_file", path)
			obs.obs_data_set_bool(settings, "is_local_file", true)
			obs.obs_data_set_bool(settings, "close_when_inactive", true)
			obs.obs_data_set_bool(settings, "restart_on_activate", true)

			-- updating will automatically cause the source to
			-- refresh if the source is currently active, otherwise
			-- the source will play whenever its scene is activated
			obs.obs_source_update(source, settings)

			obs.obs_data_release(settings)
			obs.obs_source_release(source)
		end
	end

	obs.obs_output_release(replay_buffer)
end

----------------------------------------------------------

-- A function named script_update will be called when settings are changed
function script_update(settings)
	source_name = obs.obs_data_get_string(settings, "source")
end

-- A function named script_description returns the description shown to
-- the user
function script_description()
	return "Plays the last replay buffer in the specified media source when the user presses a hotkey.\n\nMade by Jim"
end

-- A function named script_properties defines the properties that the user
-- can change for the entire script module itself
function script_properties()
	props = obs.obs_properties_create()

	local p = obs.obs_properties_add_list(props, "source", "Media Source", obs.OBS_COMBO_TYPE_EDITABLE, obs.OBS_COMBO_FORMAT_STRING)
	local sources = obs.obs_enum_sources()
	if sources ~= nil then
		for _, source in ipairs(sources) do
			source_id = obs.obs_source_get_id(source)
			if source_id == "ffmpeg_source" then
				local name = obs.obs_source_get_name(source)
				obs.obs_property_list_add_string(p, name, name)
			end
		end
	end
	obs.source_list_release(sources)

	return props
end

-- A function named script_load will be called on startup
function script_load(settings)
	hotkey_id = obs.obs_hotkey_register_frontend("instant_replay.play", "Play Last Replay Buffer", play)
	local hotkey_save_array = obs.obs_data_get_array(settings, "instant_replay.play")
	obs.obs_hotkey_load(hotkey_id, hotkey_save_array)
	obs.obs_data_array_release(hotkey_save_array)
end

-- A function named script_save will be called when the script is saved
--
-- NOTE: This function is usually used for saving extra data (such as in this
-- case, a hotkey's save data).  Settings set via the properties are saved
-- automatically.
function script_save(settings)
	local hotkey_save_array = obs.obs_hotkey_save(hotkey_id)
	obs.obs_data_set_array(settings, "instant_replay.play", hotkey_save_array)
	obs.obs_data_array_release(hotkey_save_array)
end
