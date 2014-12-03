#include "graphics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void gs_init_image_deps()
{
}

void gs_free_image_deps()
{
}

gs_texture_t *gs_texture_create_from_file(const char *file)
{
	gs_texture_t  *tex = NULL;
	int img_width;
	int img_height;
	int img_comps;
	
	if (!file || !*file)
		return NULL;
	
	
	uint8_t *img_data = stbi_load(file, &img_width, &img_height,
				      &img_comps, 4);
	if (img_data == NULL) {
		blog(LOG_WARNING, "stbi warning/error reading "
		                  "file '%s': %s", file, stbi_failure_reason());
	}
	
	tex = gs_texture_create(img_width, img_height, GS_RGBA, 1,
				(const uint8_t**)&img_data, 0);
	free(img_data);
	
	return tex;
}
