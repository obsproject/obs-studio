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
#include "gs-helper.h"

#define P_SHAPE						"Shape"
#define P_SHAPE_LOOP					"Shape.Loop"
#define P_SHAPE_MODE					"Shape.Mode"
#define P_SHAPE_MODE_TRIS				"Shape.Mode.Tris"
#define P_SHAPE_MODE_TRISTRIP				"Shape.Mode.TriStrip"
#define P_SHAPE_MODE					"Shape.Mode"
#define P_SHAPE_POINTS					"Shape.Points"
#define P_SHAPE_POINT_X					"Shape.Point.X"
#define P_SHAPE_POINT_Y					"Shape.Point.Y"
#define P_SHAPE_POINT_U					"Shape.Point.U"
#define P_SHAPE_POINT_V					"Shape.Point.V"

namespace Filter {
	class Shape {
		public:
		Shape();
		~Shape();

		static const char *get_name(void *);
		static void get_defaults(obs_data_t *);
		static obs_properties_t *get_properties(void *);
		static bool modified_properties(obs_properties_t *, obs_property_t *, obs_data_t *);

		static void *create(obs_data_t *, obs_source_t *);
		static void destroy(void *);
		static uint32_t get_width(void *);
		static uint32_t get_height(void *);
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

			private:
			obs_source_t *context;
			gs_effect_t *customEffect;
			Helper::VertexBuffer *m_vertexHelper;
			gs_vertbuffer_t *m_vertexBuffer;
			gs_draw_mode drawmode;
			gs_texrender_t *m_texRender;
		};
	};
}
