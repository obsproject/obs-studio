Scene API Reference (obs_scene_t)
=================================

A scene is a source which contains and renders other sources using
specific transforms and/or filtering

.. type:: obs_scene_t

   A reference-counted scene object.

.. type:: obs_sceneitem_t

   A reference-counted scene item object.

.. code:: cpp

   #include <obs.h>


Scene Item Transform Structure (obs_transform_info)
---------------------------------------------------

.. type:: struct obs_transform_info

   Scene item transform structure.

.. member:: struct vec2          obs_transform_info.pos

   Position.

.. member:: float                obs_transform_info.rot

   Rotation (degrees).

.. member:: struct vec2          obs_transform_info.scale

   Scale.

.. member:: uint32_t             obs_transform_info.alignment

   The alignment of the scene item relative to its position.

   Can be 0 or a bitwise OR combination of one of the following values:

   - **OBS_ALIGN_CENTER**
   - **OBS_ALIGN_LEFT**
   - **OBS_ALIGN_RIGHT**
   - **OBS_ALIGN_TOP**
   - **OBS_ALIGN_BOTTOM**

.. member:: enum obs_bounds_type obs_transform_info.bounds_type

   Can be one of the following values:

   - **OBS_BOUNDS_NONE**            - No bounding box
   - **OBS_BOUNDS_STRETCH**         - Stretch to the bounding box without preserving aspect ratio
   - **OBS_BOUNDS_SCALE_INNER**     - Scales with aspect ratio to inner bounding box rectangle
   - **OBS_BOUNDS_SCALE_OUTER**     - Scales with aspect ratio to outer bounding box rectangle
   - **OBS_BOUNDS_SCALE_TO_WIDTH**  - Scales with aspect ratio to the bounding box width
   - **OBS_BOUNDS_SCALE_TO_HEIGHT** - Scales with aspect ratio to the bounding box height
   - **OBS_BOUNDS_MAX_ONLY**        - Scales with aspect ratio, but only to the size of the source maximum

.. member:: uint32_t             obs_transform_info.bounds_alignment

   The alignment of the source within the bounding box.

   Can be 0 or a bitwise OR combination of one of the following values:

   - **OBS_ALIGN_CENTER**
   - **OBS_ALIGN_LEFT**
   - **OBS_ALIGN_RIGHT**
   - **OBS_ALIGN_TOP**
   - **OBS_ALIGN_BOTTOM**

.. member:: struct vec2          obs_transform_info.bounds

   The bounding box (if a bounding box is enabled).


Scene Item Crop Structure (obs_sceneitem_crop)
----------------------------------------------

.. type:: struct obs_sceneitem_crop

   Scene item crop structure.

.. member:: int obs_sceneitem_crop.left

   Left crop value.

.. member:: int obs_sceneitem_crop.top

   Top crop value.

.. member:: int obs_sceneitem_crop.right

   Right crop value.

.. member:: int obs_sceneitem_crop.bottom

   Bottom crop value.


.. _scene_signal_reference:

Scene Signals
-------------

**item_add** (ptr scene, ptr item)

   Called when a scene item has been added to the scene.

**item_remove** (ptr scene, ptr item)

   Called when a scene item has been removed from the scen.

**reorder** (ptr scene)

   Called when scene items have been reoredered in the scene.

**item_visible** (ptr scene, ptr item, bool visible)

   Called when a scene item's visibility state changes.

**item_select** (ptr scene, ptr item)
**item_deselect** (ptr scene, ptr item)

   Called when a scene item has been selected/deselected.

   (Author's note: These should be replaced)

**item_transform** (ptr scene, ptr item)

   Called when a scene item's transform has changed.


General Scene Functions
-----------------------

.. function:: obs_scene_t *obs_scene_create(const char *name)

   :param name: Name of the scene source.  If it's not unique, it will
                be made unique
   :return:     A reference to a scene

---------------------

.. function:: obs_scene_t *obs_scene_create_private(const char *name)

   :param name: Name of the scene source.  Does not have to be unique,
                or can be *NULL*
   :return:     A reference to a private scene

---------------------

.. function:: obs_scene_t *obs_scene_duplicate(obs_scene_t *scene, const char *name, enum obs_scene_duplicate_type type)

   Duplicates a scene.  When a scene is duplicated, its sources can be
   just referenced, or fully duplicated.

   :param name: Name of the new scene source

   :param type:  | Type of duplication:
                 | OBS_SCENE_DUP_REFS         - Duplicates the scene, but scene items are only duplicated with references
                 | OBS_SCENE_DUP_COPY         - Duplicates the scene, and scene items are also fully duplicated when possible
                 | OBS_SCENE_DUP_PRIVATE_REFS - Duplicates with references, but the scene is a private source
                 | OBS_SCENE_DUP_PRIVATE_COPY - Fully duplicates scene items when possible, but the scene and duplicates sources are private sources

   :return:     A reference to a new scene

---------------------

.. function:: void obs_scene_addref(obs_scene_t *scene)
              void obs_scene_release(obs_scene_t *scene)

   Adds/releases a reference to a scene.

---------------------

.. function:: obs_sceneitem_t *obs_scene_add(obs_scene_t *scene, obs_source_t *source)

   :return: A new scene item for a source within a scene.  Does not
            increment the reference

---------------------

.. function:: obs_source_t *obs_scene_get_source(const obs_scene_t *scene)

   :return: The scene's source.  Does not increment the reference

---------------------

.. function:: obs_scene_t *obs_scene_from_source(const obs_source_t *source)

   :return: The scene context, or *NULL* if not a scene.  Does not
            increase the reference

---------------------

.. function:: obs_sceneitem_t *obs_scene_find_source(obs_scene_t *scene, const char *name)

   :param name: The name of the source to find
   :return:     The scene item if found, otherwise *NULL* if not found

---------------------

.. function:: obs_sceneitem_t *obs_scene_find_sceneitem_by_id(obs_scene_t *scene, int64_t id)

   :param id: The unique numeric identifier of the scene item
   :return:   The scene item if found, otherwise *NULL* if not found

---------------------

.. function:: void obs_scene_enum_items(obs_scene_t *scene, bool (*callback)(obs_scene_t*, obs_sceneitem_t*, void*), void *param)

   Enumerates scene items within a scene.

---------------------

.. function:: bool obs_scene_reorder_items(obs_scene_t *scene, obs_sceneitem_t * const *item_order, size_t item_order_size)

   Reorders items within a scene.

---------------------


.. _scene_item_reference:

Scene Item Functions
--------------------

.. function:: void obs_sceneitem_addref(obs_sceneitem_t *item)
              void obs_sceneitem_release(obs_sceneitem_t *item)

   Adds/releases a reference to a scene item.

---------------------

.. function:: void obs_sceneitem_remove(obs_sceneitem_t *item)

   Removes the scene item from the scene.

---------------------

.. function:: obs_scene_t *obs_sceneitem_get_scene(const obs_sceneitem_t *item)

   :return: The scene associated with the scene item.  Does not
            increment the reference

---------------------

.. function:: obs_source_t *obs_sceneitem_get_source(const obs_sceneitem_t *item)

   :return: The source associated with the scene item.  Does not
            increment the reference

---------------------

.. function:: void obs_sceneitem_set_pos(obs_sceneitem_t *item, const struct vec2 *pos)
              void obs_sceneitem_get_pos(const obs_sceneitem_t *item, struct vec2 *pos)

   Sets/gets the position of a scene item.

---------------------

.. function:: void obs_sceneitem_set_rot(obs_sceneitem_t *item, float rot_deg)
              float obs_sceneitem_get_rot(const obs_sceneitem_t *item)

   Sets/gets the rotation of a scene item.

---------------------

.. function:: void obs_sceneitem_set_scale(obs_sceneitem_t *item, const struct vec2 *scale)
              void obs_sceneitem_get_scale(const obs_sceneitem_t *item, struct vec2 *scale)

   Sets/gets the scaling of the scene item.

---------------------

.. function:: void obs_sceneitem_set_alignment(obs_sceneitem_t *item, uint32_t alignment)
              uint32_t obs_sceneitem_get_alignment(const obs_sceneitem_t *item)

   Sets/gets the alignment of the scene item relative to its position.

   :param alignment: | Can be any bitwise OR combination of:
                     | OBS_ALIGN_CENTER
                     | OBS_ALIGN_LEFT
                     | OBS_ALIGN_RIGHT
                     | OBS_ALIGN_TOP
                     | OBS_ALIGN_BOTTOM

---------------------

.. function:: void obs_sceneitem_set_order(obs_sceneitem_t *item, enum obs_order_movement movement)

   Changes the scene item's order relative to the other scene items
   within the scene.

   :param movement: | Can be one of the following:
                    | OBS_ORDER_MOVE_UP
                    | OBS_ORDER_MOVE_DOWN
                    | OBS_ORDER_MOVE_TOP
                    | OBS_ORDER_MOVE_BOTTOM

---------------------

.. function:: void obs_sceneitem_set_order_position(obs_sceneitem_t *item, int position)

   Changes the scene item's order index.

---------------------

.. function:: void obs_sceneitem_set_bounds_type(obs_sceneitem_t *item, enum obs_bounds_type type)
              enum obs_bounds_type obs_sceneitem_get_bounds_type(const obs_sceneitem_t *item)

   Sets/gets the bounding box type of a scene item.  Bounding boxes are
   used to stretch/position the source relative to a specific bounding
   box of a specific size.

   :param type: | Can be one of the following values:
                | OBS_BOUNDS_NONE            - No bounding box
                | OBS_BOUNDS_STRETCH         - Stretch to the bounding box without preserving aspect ratio
                | OBS_BOUNDS_SCALE_INNER     - Scales with aspect ratio to inner bounding box rectangle
                | OBS_BOUNDS_SCALE_OUTER     - Scales with aspect ratio to outer bounding box rectangle
                | OBS_BOUNDS_SCALE_TO_WIDTH  - Scales with aspect ratio to the bounding box width
                | OBS_BOUNDS_SCALE_TO_HEIGHT - Scales with aspect ratio to the bounding box height
                | OBS_BOUNDS_MAX_ONLY        - Scales with aspect ratio, but only to the size of the source maximum

---------------------

.. function:: void obs_sceneitem_set_bounds_alignment(obs_sceneitem_t *item, uint32_t alignment)
              uint32_t obs_sceneitem_get_bounds_alignment(const obs_sceneitem_t *item)

   Sets/gets the alignment of the source within the bounding box.

   :param alignment: | Can be any bitwise OR combination of:
                     | OBS_ALIGN_CENTER
                     | OBS_ALIGN_LEFT
                     | OBS_ALIGN_RIGHT
                     | OBS_ALIGN_TOP
                     | OBS_ALIGN_BOTTOM

---------------------

.. function:: void obs_sceneitem_set_bounds(obs_sceneitem_t *item, const struct vec2 *bounds)
              void obs_sceneitem_get_bounds(const obs_sceneitem_t *item, struct vec2 *bounds)

   Sets/gets the bounding box width/height of the scene item.

---------------------

.. function:: void obs_sceneitem_set_info(obs_sceneitem_t *item, const struct obs_transform_info *info)
              void obs_sceneitem_get_info(const obs_sceneitem_t *item, struct obs_transform_info *info)

   Sets/gets the transform information of the scene item.

---------------------

.. function:: void obs_sceneitem_get_draw_transform(const obs_sceneitem_t *item, struct matrix4 *transform)

   Gets the transform matrix of the scene item used for drawing the
   source.

---------------------

.. function:: void obs_sceneitem_get_box_transform(const obs_sceneitem_t *item, struct matrix4 *transform)

   Gets the transform matrix of the scene item used for the bouding box
   or edges of the scene item.

---------------------

.. function:: bool obs_sceneitem_set_visible(obs_sceneitem_t *item, bool visible)
              bool obs_sceneitem_visible(const obs_sceneitem_t *item)

   Sets/gets the visibility state of the scene item.

---------------------

.. function:: void obs_sceneitem_set_crop(obs_sceneitem_t *item, const struct obs_sceneitem_crop *crop)
              void obs_sceneitem_get_crop(const obs_sceneitem_t *item, struct obs_sceneitem_crop *crop)

   Sets/gets the cropping of the scene item.

---------------------

.. function:: void obs_sceneitem_set_scale_filter(obs_sceneitem_t *item, enum obs_scale_type filter)
              enum obs_scale_type obs_sceneitem_get_scale_filter( obs_sceneitem_t *item)

   Sets/gets the scale filter used for the scene item.

   :param filter: | Can be one of the following values:
                  | OBS_SCALE_DISABLE
                  | OBS_SCALE_POINT
                  | OBS_SCALE_BICUBIC
                  | OBS_SCALE_BILINEAR
                  | OBS_SCALE_LANCZOS

---------------------

.. function:: void obs_sceneitem_defer_update_begin(obs_sceneitem_t *item)
              void obs_sceneitem_defer_update_end(obs_sceneitem_t *item)

   Allows the ability to call any one of the transform functions without
   updating the internal matrices until obs_sceneitem_defer_update_end
   has been called.

---------------------

.. function:: obs_data_t *obs_sceneitem_get_private_settings(obs_sceneitem_t *item)

   :return: An incremented reference to the private settings of the
            scene item.  Allows the front-end to set custom information
            which is saved with the scene item
