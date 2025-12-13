/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <string>
#include <obs.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(obs_source_type, {
						      {OBS_SOURCE_TYPE_INPUT, "OBS_SOURCE_TYPE_INPUT"},
						      {OBS_SOURCE_TYPE_FILTER, "OBS_SOURCE_TYPE_FILTER"},
						      {OBS_SOURCE_TYPE_TRANSITION, "OBS_SOURCE_TYPE_TRANSITION"},
						      {OBS_SOURCE_TYPE_SCENE, "OBS_SOURCE_TYPE_SCENE"},
					      })

NLOHMANN_JSON_SERIALIZE_ENUM(obs_monitoring_type,
			     {
				     {OBS_MONITORING_TYPE_NONE, "OBS_MONITORING_TYPE_NONE"},
				     {OBS_MONITORING_TYPE_MONITOR_ONLY, "OBS_MONITORING_TYPE_MONITOR_ONLY"},
				     {OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT, "OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT"},
			     })

NLOHMANN_JSON_SERIALIZE_ENUM(obs_media_state, {
						      {OBS_MEDIA_STATE_NONE, "OBS_MEDIA_STATE_NONE"},
						      {OBS_MEDIA_STATE_PLAYING, "OBS_MEDIA_STATE_PLAYING"},
						      {OBS_MEDIA_STATE_OPENING, "OBS_MEDIA_STATE_OPENING"},
						      {OBS_MEDIA_STATE_BUFFERING, "OBS_MEDIA_STATE_BUFFERING"},
						      {OBS_MEDIA_STATE_PAUSED, "OBS_MEDIA_STATE_PAUSED"},
						      {OBS_MEDIA_STATE_STOPPED, "OBS_MEDIA_STATE_STOPPED"},
						      {OBS_MEDIA_STATE_ENDED, "OBS_MEDIA_STATE_ENDED"},
						      {OBS_MEDIA_STATE_ERROR, "OBS_MEDIA_STATE_ERROR"},
					      })

NLOHMANN_JSON_SERIALIZE_ENUM(obs_bounds_type, {
						      {OBS_BOUNDS_NONE, "OBS_BOUNDS_NONE"},
						      {OBS_BOUNDS_STRETCH, "OBS_BOUNDS_STRETCH"},
						      {OBS_BOUNDS_SCALE_INNER, "OBS_BOUNDS_SCALE_INNER"},
						      {OBS_BOUNDS_SCALE_OUTER, "OBS_BOUNDS_SCALE_OUTER"},
						      {OBS_BOUNDS_SCALE_TO_WIDTH, "OBS_BOUNDS_SCALE_TO_WIDTH"},
						      {OBS_BOUNDS_SCALE_TO_HEIGHT, "OBS_BOUNDS_SCALE_TO_HEIGHT"},
						      {OBS_BOUNDS_MAX_ONLY, "OBS_BOUNDS_MAX_ONLY"},
					      })

NLOHMANN_JSON_SERIALIZE_ENUM(obs_blending_type, {
							{OBS_BLEND_NORMAL, "OBS_BLEND_NORMAL"},
							{OBS_BLEND_ADDITIVE, "OBS_BLEND_ADDITIVE"},
							{OBS_BLEND_SUBTRACT, "OBS_BLEND_SUBTRACT"},
							{OBS_BLEND_SCREEN, "OBS_BLEND_SCREEN"},
							{OBS_BLEND_MULTIPLY, "OBS_BLEND_MULTIPLY"},
							{OBS_BLEND_LIGHTEN, "OBS_BLEND_LIGHTEN"},
							{OBS_BLEND_DARKEN, "OBS_BLEND_DARKEN"},
						})

NLOHMANN_JSON_SERIALIZE_ENUM(obs_deinterlace_mode, {
							   {OBS_DEINTERLACE_MODE_DISABLE, "OBS_DEINTERLACE_MODE_DISABLE"},
							   {OBS_DEINTERLACE_MODE_DISCARD, "OBS_DEINTERLACE_MODE_DISCARD"},
							   {OBS_DEINTERLACE_MODE_RETRO, "OBS_DEINTERLACE_MODE_RETRO"},
							   {OBS_DEINTERLACE_MODE_BLEND, "OBS_DEINTERLACE_MODE_BLEND"},
							   {OBS_DEINTERLACE_MODE_BLEND_2X, "OBS_DEINTERLACE_MODE_BLEND_2X"},
							   {OBS_DEINTERLACE_MODE_LINEAR, "OBS_DEINTERLACE_MODE_LINEAR"},
							   {OBS_DEINTERLACE_MODE_LINEAR_2X, "OBS_DEINTERLACE_MODE_LINEAR_2X"},
							   {OBS_DEINTERLACE_MODE_YADIF, "OBS_DEINTERLACE_MODE_YADIF"},
							   {OBS_DEINTERLACE_MODE_YADIF_2X, "OBS_DEINTERLACE_MODE_YADIF_2X"},
						   })

NLOHMANN_JSON_SERIALIZE_ENUM(obs_deinterlace_field_order,
			     {
				     {OBS_DEINTERLACE_FIELD_ORDER_TOP, "OBS_DEINTERLACE_FIELD_ORDER_TOP"},
				     {OBS_DEINTERLACE_FIELD_ORDER_BOTTOM, "OBS_DEINTERLACE_FIELD_ORDER_BOTTOM"},
			     })

namespace Utils {
	namespace Json {
		bool JsonArrayIsValidObsArray(const json &j);
		obs_data_t *JsonToObsData(json j);
		json ObsDataToJson(obs_data_t *d, bool includeDefault = false);
		bool GetJsonFileContent(std::string fileName, json &content);
		bool SetJsonFileContent(std::string fileName, const json &content, bool makeDirs = true);
		static inline bool Contains(const json &j, std::string key)
		{
			return j.contains(key) && !j[key].is_null();
		}
	}
}
