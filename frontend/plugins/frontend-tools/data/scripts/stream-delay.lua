obs = obslua

function get_output()
	local output = obs.obs_get_output_by_name("rtmp multitrack video")
	return output
end

-- Function to get the current delay and add 1 second
function increment_stream_delay()
    -- Get the output (stream or recording)
    local output = get_output()
    local current_delay = obs.obs_output_get_delay(output)

    -- Add 1 second to the current delay
    local new_delay = current_delay + 1
    
    obs.obs_output_set_delay(output, new_delay, 0)

    -- Log the new delay
    obs.script_log(obs.LOG_INFO, "Increased stream delay from " .. current_delay .. " to " .. new_delay .. " seconds.")
end

function decrement_stream_delay()
    -- Get the output (stream or recording)
    local output = get_output()
    local current_delay = obs.obs_output_get_delay(output)

    -- Add 1 second to the current delay
    local new_delay = current_delay - 1
    
    obs.obs_output_set_delay(output, new_delay, 0)

    -- Log the new delay
    obs.script_log(obs.LOG_INFO, "Decreased stream delay from " .. current_delay .. " to " .. new_delay .. " seconds.")
end

-- Register the function as a hotkey
hotkey_id = obs.obs_hotkey_register_frontend("increment_stream_delay", "Increase Stream Delay by 1s", increment_stream_delay)
hotkey_id = obs.obs_hotkey_register_frontend("decrement_stream_delay", "Descrease Stream Delay by 1s", decrement_stream_delay)
-- Script description (appears in the script UI in OBS)
function script_description()
    return "This script increases the stream delay by 1 second every time the hotkey is pressed."
end

-- Called when the script is unloaded
function script_unload()
    -- obs.obs_hotkey_unregister(hotkey_id)
end

