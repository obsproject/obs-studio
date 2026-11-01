.. _image_file_helper:

Image File Helper
=================

Helper functions/type for easily loading/managing image files, including
animated gif files.

.. code:: cpp

   #include <graphics/image-file.h>

.. struct:: gs_image_file_ex

   Image file structure

.. type:: gs_texture_t *gs_image_file_ex.texture

   Texture

.. type:: struct gs_image_file_ex gs_image_file_ex_t

   Image file type

---------------------

.. function:: void gs_image_file_ex_init(gs_image_file_ex_t *image, const char *file, enum gs_image_alpha_mode alpha_mode)

   Loads an initializes an image file helper.  Does not initialize the
   texture; call :c:func:`gs_image_file_ex_init_texture()` to initialize
   the texture.

   :param image: Image file helper to initialize
   :param file:  Path to the image file to load

---------------------

.. function:: void gs_image_file_ex_free(gs_image_file_ex_t *image)

   Frees an image file helper

   :param image: Image file helper

---------------------

.. function:: void gs_image_file_ex_init_texture(gs_image_file_ex_t *image)

   Initializes the texture of an image file helper.  This is separate
   from :c:func:`gs_image_file_ex_init()` because it allows deferring the
   graphics initialization if needed.

   :param image: Image file helper

---------------------

.. function:: bool gs_image_file_ex_tick(gs_image_file_ex_t *image, uint64_t elapsed_time_ns)

   Performs a tick operation on the image file helper (used primarily
   for animated file).  Does not update the texture until
   :c:func:`gs_image_file_ex_update_texture()` is called.

   :param image:           Image file helper
   :param elapsed_time_ns: Elapsed time in nanoseconds

---------------------

.. function:: void gs_image_file_ex_update_texture(gs_image_file_ex_t *image)

   Updates the texture (used primarily for animated files)

   :param image: Image file helper
