-- internal functions, DO NOT EDIT
-- duplicate one of the provided save<nnn>seconds.lua
-- and edit that instead

hotkey_id = "replay_buffer.trigger" .. seconds_to_save .. "s"
obs         = obslua
hotkey_id   = obs.OBS_INVALID_HOTKEY_ID

-- hotkey callback
function replay_buffer_seconds_save(pressed)
	if not pressed then
		return
	end

	local replay_buffer = obs.obs_frontend_get_replay_buffer_output()
	if replay_buffer ~= nil then
		local ph = obs.obs_output_get_proc_handler(replay_buffer)

		local cd = obs.calldata_create()
		obs.calldata_set_int(cd, "seconds", seconds_to_save)

		obs.proc_handler_call(ph, "save_length", cd)
		obs.calldata_destroy(cd);

		obs.obs_output_release(replay_buffer)
	else
		obs.script_log(obs.LOG_WARNING, "Tried to save the replay buffer, but no active replay buffer was found")
	end
end

----------------------------------------------------------

-- A function named script_update will be called when settings are changed
function script_update(settings)
	--source_name = obs.obs_data_get_string(settings, "source")
end

-- A function named script_description returns the description shown to
-- the user
function script_description()
	return "Saves the last " .. seconds_to_save .. " seconds from Replay Buffer"
end

-- A function named script_properties defines the properties that the user
-- can change for the entire script module itself
function script_properties()
	return nil
end

-- A function named script_load will be called on startup
function script_load(settings)
	hotkey_id = obs.obs_hotkey_register_frontend(hotkey_id, "Save " .. seconds_to_save .. " seconds Instant Replay", replay_buffer_seconds_save)
	local hotkey_save_array = obs.obs_data_get_array(settings, hotkey_id)
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
	obs.obs_data_set_array(settings, hotkey_id, hotkey_save_array)
	obs.obs_data_array_release(hotkey_save_array)
end
