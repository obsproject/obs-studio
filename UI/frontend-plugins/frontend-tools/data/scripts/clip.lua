obs         = obslua
hotkey_id   = obs.OBS_INVALID_HOTKEY_ID
duration    = 30

function save_clip(pressed)
  if not pressed then
    return
  end

  local replay_buffer = obs.obs_frontend_get_replay_buffer_output()
  if replay_buffer == nil then
    return
  end

  local cd = obs.calldata_create()
  local ph = obs.obs_output_get_proc_handler(replay_buffer)
  obs.calldata_set_int(cd, "usec", duration * 1000 * 1000)
  obs.proc_handler_call(ph, "save_duration", cd)

  obs.calldata_destroy(cd)
  obs.obs_output_release(replay_buffer)
end

-- A function named script_update will be called when settings are changed
function script_update(settings)
  duration = obs.obs_data_get_int(settings, "Clip.Duration")
end

-- A function named script_description returns the description shown to
-- the user
function script_description()
	return "When the \"Save Clip\" hotkey is triggered, save a clip of the configured duration from the replay buffer"
end

-- A function named script_properties defines the properties that the user
-- can change for the entire script module itself
function script_properties()
	local props = obs.obs_properties_create()
  obs.obs_properties_add_int(props, "Clip.Duration", "Clip Duration in seconds", 5, 3600, 1);
	return props
end

-- A function named script_load will be called on startup
function script_load(settings)
	hotkey_id = obs.obs_hotkey_register_frontend("Clip.Save", "Save Clip", save_clip)
	local hotkey_save_array = obs.obs_data_get_array(settings, "Clip.Save")
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
	obs.obs_data_set_array(settings, "Clip.Save", hotkey_save_array)
	obs.obs_data_array_release(hotkey_save_array)
end
