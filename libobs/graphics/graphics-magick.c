#include "graphics.h"

#define MAGICKCORE_QUANTUM_DEPTH 16
#define MAGICKCORE_HDRI_ENABLE   0
#include <magick/MagickCore.h>

void gs_init_image_deps()
{
	MagickCoreGenesis(NULL, MagickTrue);
}

void gs_free_image_deps()
{
	MagickCoreTerminus();
}

gs_texture_t gs_texture_create_from_file(const char *file)
{
	gs_texture_t  tex = NULL;
	ImageInfo     *info;
	ExceptionInfo *exception;
	Image         *image;

	if (!file || !*file)
		return NULL;

	info      = CloneImageInfo(NULL);
	exception = AcquireExceptionInfo();

	strcpy(info->filename, file);
	image = ReadImage(info, exception);
	if (image) {
		size_t  cx    = image->magick_columns;
		size_t  cy    = image->magick_rows;
		uint8_t *data = malloc(cx * cy * 4);

		ExportImagePixels(image, 0, 0, cx, cy, "BGRA", CharPixel,
				data, exception);
		if (exception->severity == UndefinedException)
			tex = gs_texture_create(cx, cy, GS_BGRA, 1,
					(const uint8_t**)&data, 0);
		else
			blog(LOG_WARNING, "magickcore warning/error getting "
			                  "pixels from file '%s': %s", file,
			                  exception->reason);

		free(data);
		DestroyImage(image);

	} else if (exception->severity != UndefinedException) {
		blog(LOG_WARNING, "magickcore warning/error reading file "
		                  "'%s': %s", file, exception->reason);
	}

	DestroyImageInfo(info);
	DestroyExceptionInfo(exception);

	return tex;
}
