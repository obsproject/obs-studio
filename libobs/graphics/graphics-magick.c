#include "graphics.h"

#include <magick/MagickCore.h>

void gs_init_image_deps()
{
	MagickCoreGenesis(NULL, MagickTrue);
}

void gs_free_image_deps()
{
	MagickCoreTerminus();
}

texture_t gs_create_texture_from_file(const char *file)
{
	texture_t     tex        = NULL;
	ImageInfo     *info      = CloneImageInfo(NULL);
	ExceptionInfo *exception = AcquireExceptionInfo();
	Image         *image;

	strcpy(info->filename, file);
	image = ReadImage(info, exception);
	if (image) {
		size_t  cx    = image->magick_columns;
		size_t  cy    = image->magick_rows;
		uint8_t *data = malloc(cx * cy * 4);

		ExportImagePixels(image, 0, 0, cx, cy, "BGRA", CharPixel,
				data, exception);
		if (exception->severity == UndefinedException)
			tex = gs_create_texture(cx, cy, GS_BGRA, 1,
					(const uint8**)&data, 0);
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
