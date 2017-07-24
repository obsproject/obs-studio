/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#pragma once
#include "plugin.h"

extern "C" {
#pragma warning (push)
#pragma warning (disable: 4201)
#include <libobs/obs-source.h>
#include <libobs/util/platform.h>
#pragma warning (pop)
}

#include <string>

#define P_FILTER_DISPLACEMENT				"Filter.Displacement"
#define P_FILTER_DISPLACEMENT_FILE			"Filter.Displacement.File"
#define P_FILTER_DISPLACEMENT_FILE_TYPES		"Filter.Displacement.File.Types"
#define P_FILTER_DISPLACEMENT_RATIO			"Filter.Displacement.Ratio"
#define P_FILTER_DISPLACEMENT_SCALE			"Filter.Displacement.Scale"

namespace Filter {
	class Displacement {
		public:
		Displacement();
		~Displacement();

		static const char *get_name(void *);

		static void *create(obs_data_t *, obs_source_t *);
		static void destroy(void *);
		static uint32_t get_width(void *);
		static uint32_t get_height(void *);
		static void get_defaults(obs_data_t *);
		static obs_properties_t *get_properties(void *);
		static void update(void *, obs_data_t *);
		static void activate(void *);
		static void deactivate(void *);
		static void show(void *);
		static void hide(void *);
		static void video_tick(void *, float);
		static void video_render(void *, gs_effect_t *);

		private:
		obs_source_info sourceInfo;

		private:
		class Instance {
			public:
			Instance(obs_data_t*, obs_source_t*);
			~Instance();

			void update(obs_data_t*);
			uint32_t get_width();
			uint32_t get_height();
			void activate();
			void deactivate();
			void show();
			void hide();
			void video_tick(float);
			void video_render(gs_effect_t*);

			std::string get_file();

			private:
			void updateDisplacementMap(std::string file);

			obs_source_t *context;
			gs_effect_t *customEffect;
			float_t distance;
			vec2 displacementScale;
			struct {
				std::string file;

				gs_texture_t* texture;
				time_t createTime,
					modifiedTime;
				size_t size;
			} dispmap;

			float_t timer;
		};
	};
}
