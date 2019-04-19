#ifdef LIBOBS_IMAGEMAGICK_DIR_STYLE_6L
#include <magick/MagickCore.h>
#endif
#ifdef LIBOBS_IMAGEMAGICK_DIR_STYLE_7GE
#include <MagickCore/MagickCore.h>
#endif

#include <obs-module.h>

#include "v4l2-decompress.h"

/**
 * Performs MJPEG decompression
 *
 * @param v4l2 buffer
 * @param output
 * @param width
 * @param height
 *
 * @return bool if success
 */
bool v4l2_decompress(uint8_t *buffer, int buf_size, uint8_t *output, int width, int height) {
    bool ret_val = false;

	ImageInfo     *info;
	ExceptionInfo *exception;
	Image         *image;

	info      = CloneImageInfo(NULL);
	exception = AcquireExceptionInfo();

    image = BlobToImage(info, buffer, width * height * 4, exception);

    if (exception->severity != UndefinedException) {
        blog(LOG_WARNING, "magickcore error creating image %s", exception->reason);
    }

    if (image) {
		ExportImagePixels(image, 0, 0, width, height, "BGRA", CharPixel,
				output, exception);
		if (exception->severity != UndefinedException) {
			blog(LOG_WARNING, "magickcore warning/error getting "
			                  "pixels: %s", exception->reason);
		} else {
            ret_val = true;
        }

		DestroyImage(image);
    }

    DestroyImageInfo(info);
	DestroyExceptionInfo(exception);

    return ret_val;
}
