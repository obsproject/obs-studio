/******************************************************************************
    Copyright (C) 2016 by Hugh Bailey <obs.jim@gmail.com>

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

#include "image-file.h"
#include "../util/base.h"
#include "../util/platform.h"
#include "vec4.h"

#define blog(level, format, ...) \
	blog(level, "%s: " format, __FUNCTION__, __VA_ARGS__)

static void *bi_def_bitmap_create(int width, int height)
{
	return bmalloc((size_t)4 * width * height);
}

static void bi_def_bitmap_set_opaque(void *bitmap, bool opaque)
{
	UNUSED_PARAMETER(bitmap);
	UNUSED_PARAMETER(opaque);
}

static bool bi_def_bitmap_test_opaque(void *bitmap)
{
	UNUSED_PARAMETER(bitmap);
	return false;
}

static unsigned char *bi_def_bitmap_get_buffer(void *bitmap)
{
	return (unsigned char *)bitmap;
}

static void bi_def_bitmap_destroy(void *bitmap)
{
	bfree(bitmap);
}

static void bi_def_bitmap_modified(void *bitmap)
{
	UNUSED_PARAMETER(bitmap);
}

static inline int get_full_decoded_gif_size(gs_image_file_t *image)
{
	return image->gif.width * image->gif.height * 4 *
	       image->gif.frame_count;
}

static inline void *alloc_mem(gs_image_file_t *image, uint64_t *mem_usage,
			      size_t size)
{
	UNUSED_PARAMETER(image);

	if (mem_usage)
		*mem_usage += size;
	return bzalloc(size);
}

static bool init_animated_gif(gs_image_file_t *image, const char *path,
			      uint64_t *mem_usage,
			      enum gs_image_alpha_mode alpha_mode)
{
	bool is_animated_gif = true;
	gif_result result;
	uint64_t max_size;
	size_t size, size_read;
	FILE *file;

	image->bitmap_callbacks.bitmap_create = bi_def_bitmap_create;
	image->bitmap_callbacks.bitmap_destroy = bi_def_bitmap_destroy;
	image->bitmap_callbacks.bitmap_get_buffer = bi_def_bitmap_get_buffer;
	image->bitmap_callbacks.bitmap_modified = bi_def_bitmap_modified;
	image->bitmap_callbacks.bitmap_set_opaque = bi_def_bitmap_set_opaque;
	image->bitmap_callbacks.bitmap_test_opaque = bi_def_bitmap_test_opaque;

	gif_create(&image->gif, &image->bitmap_callbacks);

	file = os_fopen(path, "rb");
	if (!file) {
		blog(LOG_WARNING, "Failed to open file '%s'", path);
		goto fail;
	}

	fseek(file, 0, SEEK_END);
	size = (size_t)os_ftelli64(file);
	fseek(file, 0, SEEK_SET);

	image->gif_data = bmalloc(size);
	size_read = fread(image->gif_data, 1, size, file);
	if (size_read != size) {
		blog(LOG_WARNING, "Failed to fully read gif file '%s'.", path);
		goto fail;
	}

	do {
		result = gif_initialise(&image->gif, size, image->gif_data);
		if (result < 0) {
			blog(LOG_WARNING,
			     "Failed to initialize gif '%s', "
			     "possible file corruption",
			     path);
			goto fail;
		}
	} while (result != GIF_OK);

	if (image->gif.width > 4096 || image->gif.height > 4096) {
		blog(LOG_WARNING, "Bad texture dimensions (%dx%d) in '%s'",
		     image->gif.width, image->gif.height, path);
		goto fail;
	}

	max_size = (uint64_t)image->gif.width * (uint64_t)image->gif.height *
		   (uint64_t)image->gif.frame_count * 4LLU;

	if ((uint64_t)get_full_decoded_gif_size(image) != max_size) {
		blog(LOG_WARNING, "Gif '%s' overflowed maximum pointer size",
		     path);
		goto fail;
	}

	image->is_animated_gif = (image->gif.frame_count > 1 && result >= 0);
	if (image->is_animated_gif) {
		gif_decode_frame(&image->gif, 0);

		image->animation_frame_cache =
			alloc_mem(image, mem_usage,
				  image->gif.frame_count * sizeof(uint8_t *));
		image->animation_frame_data = alloc_mem(
			image, mem_usage, get_full_decoded_gif_size(image));

		for (unsigned int i = 0; i < image->gif.frame_count; i++) {
			if (gif_decode_frame(&image->gif, i) != GIF_OK)
				blog(LOG_WARNING,
				     "Couldn't decode frame %u "
				     "of '%s'",
				     i, path);
		}

		gif_decode_frame(&image->gif, 0);

		image->cx = (uint32_t)image->gif.width;
		image->cy = (uint32_t)image->gif.height;
		image->format = GS_RGBA;

		if (mem_usage) {
			*mem_usage += (size_t)4 * image->cx * image->cy;
			*mem_usage += size;
		}

		if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY_SRGB) {
			gs_premultiply_xyza_srgb_loop(image->gif.frame_image,
						      (size_t)image->cx *
							      image->cy);
		} else if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY) {
			gs_premultiply_xyza_loop(image->gif.frame_image,
						 (size_t)image->cx * image->cy);
		}
	} else {
		gif_finalise(&image->gif);
		bfree(image->gif_data);
		image->gif_data = NULL;
		is_animated_gif = false;
		goto not_animated;
	}

	image->loaded = true;

fail:
	if (!image->loaded)
		gs_image_file_free(image);
not_animated:
	if (file)
		fclose(file);

	return is_animated_gif;
}

static void gs_image_file_init_internal(gs_image_file_t *image,
					const char *file, uint64_t *mem_usage,
					enum gs_image_alpha_mode alpha_mode)
{
	size_t len;

	if (!image)
		return;

	memset(image, 0, sizeof(*image));

	if (!file)
		return;

	len = strlen(file);

	if (len > 4 && strcmp(file + len - 4, ".gif") == 0) {
		if (init_animated_gif(image, file, mem_usage, alpha_mode)) {
			return;
		}
	}

	image->texture_data = gs_create_texture_file_data2(
		file, alpha_mode, &image->format, &image->cx, &image->cy);

	if (mem_usage) {
		*mem_usage += image->cx * image->cy *
			      gs_get_format_bpp(image->format) / 8;
	}

	image->loaded = !!image->texture_data;
	if (!image->loaded) {
		blog(LOG_WARNING, "Failed to load file '%s'", file);
		gs_image_file_free(image);
	}
}

void gs_image_file_init(gs_image_file_t *image, const char *file)
{
	gs_image_file_init_internal(image, file, NULL, GS_IMAGE_ALPHA_STRAIGHT);
}

void gs_image_file_free(gs_image_file_t *image)
{
	if (!image)
		return;

	if (image->loaded) {
		if (image->is_animated_gif) {
			gif_finalise(&image->gif);
			bfree(image->animation_frame_cache);
			bfree(image->animation_frame_data);
		}

		gs_texture_destroy(image->texture);
	}

	bfree(image->texture_data);
	bfree(image->gif_data);
	memset(image, 0, sizeof(*image));
}

void gs_image_file2_init(gs_image_file2_t *if2, const char *file)
{
	gs_image_file_init_internal(&if2->image, file, &if2->mem_usage,
				    GS_IMAGE_ALPHA_STRAIGHT);
}

void gs_image_file3_init(gs_image_file3_t *if3, const char *file,
			 enum gs_image_alpha_mode alpha_mode)
{
	gs_image_file_init_internal(&if3->image2.image, file,
				    &if3->image2.mem_usage, alpha_mode);
	if3->alpha_mode = alpha_mode;
}

void gs_image_file_init_texture(gs_image_file_t *image)
{
	if (!image->loaded)
		return;

	if (image->is_animated_gif) {
		image->texture = gs_texture_create(
			image->cx, image->cy, image->format, 1,
			(const uint8_t **)&image->gif.frame_image, GS_DYNAMIC);

	} else {
		image->texture = gs_texture_create(
			image->cx, image->cy, image->format, 1,
			(const uint8_t **)&image->texture_data, 0);
		bfree(image->texture_data);
		image->texture_data = NULL;
	}
}

static inline uint64_t get_time(gs_image_file_t *image, int i)
{
	uint64_t val = (uint64_t)image->gif.frames[i].frame_delay * 10000000ULL;
	if (!val)
		val = 100000000;
	return val;
}

static inline int calculate_new_frame(gs_image_file_t *image,
				      uint64_t elapsed_time_ns, int loops)
{
	int new_frame = image->cur_frame;

	image->cur_time += elapsed_time_ns;
	for (;;) {
		uint64_t t = get_time(image, new_frame);
		if (image->cur_time <= t)
			break;

		image->cur_time -= t;
		if ((unsigned int)++new_frame == image->gif.frame_count) {
			if (!loops || ++image->cur_loop < loops) {
				new_frame = 0;
			} else if (image->cur_loop == loops) {
				new_frame--;
				break;
			}
		}
	}

	return new_frame;
}

static void decode_new_frame(gs_image_file_t *image, int new_frame,
			     enum gs_image_alpha_mode alpha_mode)
{
	if (!image->animation_frame_cache[new_frame]) {
		int last_frame;

		/* if looped, decode frame 0 */
		last_frame = (new_frame < image->last_decoded_frame)
				     ? 0
				     : image->last_decoded_frame + 1;

		/* decode missed frames */
		for (int i = last_frame; i < new_frame; i++) {
			if (gif_decode_frame(&image->gif, i) != GIF_OK)
				return;
		}

		/* decode actual desired frame */
		if (gif_decode_frame(&image->gif, new_frame) == GIF_OK) {
			const size_t area =
				(size_t)image->gif.width * image->gif.height;
			size_t pos = new_frame * area * 4;
			image->animation_frame_cache[new_frame] =
				image->animation_frame_data + pos;

			if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY_SRGB) {
				gs_premultiply_xyza_srgb_loop(
					image->gif.frame_image, area);
			} else if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY) {
				gs_premultiply_xyza_loop(image->gif.frame_image,
							 area);
			}

			memcpy(image->animation_frame_cache[new_frame],
			       image->gif.frame_image, area * 4);

			image->last_decoded_frame = new_frame;
		}
	}

	image->cur_frame = new_frame;
}

static bool gs_image_file_tick_internal(gs_image_file_t *image,
					uint64_t elapsed_time_ns,
					enum gs_image_alpha_mode alpha_mode)
{
	int loops;

	if (!image->is_animated_gif || !image->loaded)
		return false;

	loops = image->gif.loop_count;
	if (loops >= 0xFFFF)
		loops = 0;

	if (!loops || image->cur_loop < loops) {
		int new_frame =
			calculate_new_frame(image, elapsed_time_ns, loops);

		if (new_frame != image->cur_frame) {
			decode_new_frame(image, new_frame, alpha_mode);
			return true;
		}
	}

	return false;
}

bool gs_image_file_tick(gs_image_file_t *image, uint64_t elapsed_time_ns)
{
	return gs_image_file_tick_internal(image, elapsed_time_ns, false);
}

bool gs_image_file2_tick(gs_image_file2_t *if2, uint64_t elapsed_time_ns)
{
	return gs_image_file_tick_internal(&if2->image, elapsed_time_ns, false);
}

bool gs_image_file3_tick(gs_image_file3_t *if3, uint64_t elapsed_time_ns)
{
	return gs_image_file_tick_internal(&if3->image2.image, elapsed_time_ns,
					   if3->alpha_mode);
}

static void
gs_image_file_update_texture_internal(gs_image_file_t *image,
				      enum gs_image_alpha_mode alpha_mode)
{
	if (!image->is_animated_gif || !image->loaded)
		return;

	if (!image->animation_frame_cache[image->cur_frame])
		decode_new_frame(image, image->cur_frame, alpha_mode);

	gs_texture_set_image(image->texture,
			     image->animation_frame_cache[image->cur_frame],
			     image->gif.width * 4, false);
}

void gs_image_file_update_texture(gs_image_file_t *image)
{
	gs_image_file_update_texture_internal(image, false);
}

void gs_image_file2_update_texture(gs_image_file2_t *if2)
{
	gs_image_file_update_texture_internal(&if2->image, false);
}

void gs_image_file3_update_texture(gs_image_file3_t *if3)
{
	gs_image_file_update_texture_internal(&if3->image2.image,
					      if3->alpha_mode);
}
