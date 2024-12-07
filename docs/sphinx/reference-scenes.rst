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

.. struct:: obs_transform_info

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

.. struct:: obs_sceneitem_crop

   Scene item crop structure.

.. member:: int obs_sceneitem_crop.left

   Left crop value.

.. member:: int obs_sceneitem_crop.top

   Top crop value.

.. member:: int obs_sceneitem_crop.right

   Right crop value.

.. member:: int obs_sceneitem_crop.bottom

   Bottom crop value.


Scene Item Order Info Structure (\*obs_sceneitem_order_info)
------------------------------------------------------------

.. struct:: obs_sceneitem_order_info

   Scene item order info structure.

.. member:: obs_sceneitem_t *obs_sceneitem_order_info.group

   Specifies the group this scene item belongs to, or *NULL* if none.

.. member:: obs_sceneitem_t *obs_sceneitem_order_info.item

   Specifies the scene item.



.. _scene_signal_reference:

Scene Signals
-------------

**item_add** (ptr scene, ptr item)

   Called when a scene item has been added to the scene.

**item_remove** (ptr scene, ptr item)

   Called when a scene item has been removed from the scene.

**reorder** (ptr scene)

   Called when scene items have been reordered in the scene.

**refresh** (ptr scene)

   Called when the entire scene item list needs to be refreshed.
   Usually this is only used when groups have changed.

**item_visible** (ptr scene, ptr item, bool visible)

   Called when a scene item's visibility state changes.

**item_locked** (ptr scene, ptr item, bool locked)

   Called when a scene item has been locked or unlocked.

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

.. function:: obs_scene_t *obs_scene_get_ref(obs_scene_t *scene)

   Returns an incremented reference if still valid, otherwise returns
   *NULL*. Release with :c:func:`obs_scene_release()`.

---------------------

.. function:: void obs_scene_release(obs_scene_t *scene)

   Releases a reference to a scene.

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

.. function:: obs_sceneitem_t *obs_scene_find_source_recursive(obs_scene_t *scene, const char *name)

   Same as obs_scene_find_source, but also searches groups within the
   scene.

   :param name: The name of the source to find
   :return:     The scene item if found, otherwise *NULL* if not found

---------------------

.. function:: obs_sceneitem_t *obs_scene_find_sceneitem_by_id(obs_scene_t *scene, int64_t id)

   :param id: The unique numeric identifier of the scene item
   :return:   The scene item if found, otherwise *NULL* if not found

---------------------

.. function:: void obs_scene_enum_items(obs_scene_t *scene, bool (*callback)(obs_scene_t*, obs_sceneitem_t*, void*), void *param)

   Enumerates scene items within a scene in order of the bottommost scene item
   to the topmost scene item.

   Callback function returns true to continue enumeration, or false to end
   enumeration.

   Use :c:func:`obs_sceneitem_addref()` if you want to retain a
   reference after obs_scene_enum_items finishes.

   For scripting, use :py:func:`obs_scene_enum_items`.

---------------------

.. function:: bool obs_scene_reorder_items(obs_scene_t *scene, obs_sceneitem_t * const *item_order, size_t item_order_size)

   Reorders items within a scene.

---------------------

.. function:: bool obs_scene_reorder_items2(obs_scene_t *scene, struct obs_sceneitem_order_info *item_order, size_t item_order_size)

   Reorders items within a scene with groups and group sub-items.

---------------------

.. function:: void obs_scene_prune_sources(obs_scene_t *scene)

   Releases all sources from a scene that have been marked as removed by obs_source_remove.

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

.. function:: obs_sceneitem_t *obs_scene_sceneitem_from_source(obs_scene_t *scene, obs_source_t *source)

   This will add a reference to the sceneitem.

   :return: The sceneitem associated with a source in a scene. Returns NULL if not found.

   .. deprecated:: 31.0
      This function is problematic because there can be multiple items of the same source in a scene.
      In that case, which of those this function will return is undefined.
      If this is the behavior you need, manually use :c:func:`obs_scene_enum_items` instead.

---------------------

.. function:: void obs_sceneitem_set_id(obs_sceneitem_t *item);

   Sets the unique numeric identifier of the sceneitem. This is dangerous function and should not
   normally be used. It can cause errors within obs.

---------------------

.. function:: int64_t obs_sceneitem_get_id(const obs_sceneitem_t *item)

   Gets the numeric identifier of the sceneitem.

   :return: Gets the unique numeric identifier of the scene item.

---------------------

.. function:: obs_data_t *obs_scene_save_transform_states(obs_scene_t *scene, bool all_items)
.. function:: void obs_scene_load_transform_states(const char *states)

   Saves all the transformation states for the sceneitems in scene. When all_items is false, it
   will only save selected items

   :return: Data containing transformation states for all* sceneitems in scene

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

   Changes the sceneitem's order index.

---------------------

.. function:: int obs_sceneitem_get_order_position(obs_sceneitem_t *item)

   :return: Gets position of sceneitem in its scene.

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

.. function:: void obs_sceneitem_set_info2(obs_sceneitem_t *item, const struct obs_transform_info *info)
              void obs_sceneitem_get_info2(const obs_sceneitem_t *item, struct obs_transform_info *info)

   Sets/gets the transform information of the scene item.
   This version of the function also sets the `crop_to_bounds` member of `obs_transform_info`.

   .. versionadded:: 30.1

---------------------

.. function:: void obs_sceneitem_get_draw_transform(const obs_sceneitem_t *item, struct matrix4 *transform)

   Gets the transform matrix of the scene item used for drawing the
   source.

---------------------

.. function:: void obs_sceneitem_get_box_transform(const obs_sceneitem_t *item, struct matrix4 *transform)

   Gets the transform matrix of the scene item used for the bounding box
   or edges of the scene item.

---------------------

.. function:: void obs_sceneitem_select(obs_sceneitem_t *item, bool select)
              bool obs_sceneitem_selected(const obs_sceneitem_t *item)

   Sets/gets the selection state of the scene item. Note that toggling
   the selected state will not affect the selected state of other scene items,
   as multiple scene items can be selected.

---------------------

.. function:: bool obs_sceneitem_set_visible(obs_sceneitem_t *item, bool visible)
              bool obs_sceneitem_visible(const obs_sceneitem_t *item)

   Sets/gets the visibility state of the scene item.

---------------------

.. function:: bool obs_sceneitem_set_locked(obs_sceneitem_t *item, bool locked)
              bool obs_sceneitem_locked(const obs_sceneitem_t *item)

   Sets/gets the locked/unlocked state of the scene item.

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

.. function:: void obs_sceneitem_set_blending_method(obs_sceneitem_t *item, enum obs_blending_method method)
              enum obs_blending_method obs_sceneitem_get_blending_method(obs_sceneitem_t *item)

   Sets/gets the blending method used for the scene item.

   :param method: | Can be one of the following values:
                  | OBS_BLEND_METHOD_DEFAULT
                  | OBS_BLEND_METHOD_SRGB_OFF

---------------------

.. function:: void obs_sceneitem_set_blending_mode(obs_sceneitem_t *item, enum obs_blending_type type)
              enum obs_blending_type obs_sceneitem_get_blending_mode(obs_sceneitem_t *item)

   Sets/gets the blending mode used for the scene item.

   :param type: | Can be one of the following values:
                | OBS_BLEND_NORMAL
                | OBS_BLEND_ADDITIVE
                | OBS_BLEND_SUBTRACT
                | OBS_BLEND_SCREEN
                | OBS_BLEND_MULTIPLY
                | OBS_BLEND_LIGHTEN
                | OBS_BLEND_DARKEN

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
            which is saved with the scene item. Release with
            :c:func:`obs_data_release()`.

---------------------

.. function:: void obs_sceneitem_set_transition(obs_sceneitem_t *item, bool show, obs_source_t *transition)

   Sets a transition for showing or hiding a scene item.

   :param item:       The target scene item
   :param show:       If *true*, this will set the show transition.
                      If *false*, this will set the hide transition.
   :param transition: The transition to set. Pass *NULL* to remove the transition.

---------------------

.. function:: obs_source_t *obs_sceneitem_get_transition(obs_sceneitem_t *item, bool show)

   :param item: The target scene item
   :param show: If *true*, this will return the show transition.
                If *false*, this will return the hide transition.
   :return:     The transition for showing or hiding a scene item. *NULL* if no transition is set.

---------------------

---------------------

.. function:: void obs_sceneitem_set_transition_duration(obs_sceneitem_t *item, bool show, uint32_t duration_ms)

   Sets the transition duration for showing or hiding a scene item.

   :param item:        The target scene item
   :param show:        If *true*, this will set the duration of the show transition.
                       If *false*, this will set the duration of the hide transition.
   :param duration_ms: The transition duration in milliseconds

---------------------

.. function:: uint32_t obs_sceneitem_get_transition_duration(obs_sceneitem_t *item, bool show)

   Gets the transition duration for showing or hiding a scene item.

   :param item: The target scene item
   :param show: If *true*, this will return the duration of the show transition.
                If *false*, this will return the duration of the hide transition.
   :return:     The transition duration in milliseconds

---------------------

.. function:: void obs_sceneitem_do_transition(obs_sceneitem_t *item, bool visible)

   Start the transition for showing or hiding a scene item.

---------------------


.. _scene_item_group_reference:

Scene Item Group Functions
--------------------------

.. function:: obs_sceneitem_t *obs_scene_add_group(obs_scene_t *scene, const char *name)

   Adds a group with the specified name.  Does not signal the scene with
   the *refresh* signal.

   :param scene: Scene to add the group to
   :param name:  Name of the group
   :return:      The new group's scene item

---------------------

.. function:: obs_sceneitem_t *obs_scene_add_group2(obs_scene_t *scene, const char *name, bool signal)

   Adds a group with the specified name.

   :param scene:  Scene to add the group to
   :param name:   Name of the group
   :param signal: If *true*, signals the scene with the *refresh*
                  signal
   :return:       The new group's scene item

---------------------

.. function:: obs_sceneitem_t *obs_scene_insert_group(obs_scene_t *scene, const char *name, obs_sceneitem_t **items, size_t count)

   Creates a group out of the specified scene items.  The group will be
   inserted at the top scene item.  Does not signal the scene with the
   *refresh* signal.

   :param scene: Scene to add the group to
   :param name:  Name of the group
   :param items: Array of scene items to put in a group
   :param count: Number of scene items in the array
   :return:      The new group's scene item

---------------------

.. function:: obs_sceneitem_t *obs_scene_insert_group2(obs_scene_t *scene, const char *name, obs_sceneitem_t **items, size_t count, bool signal)

   Creates a group out of the specified scene items.  The group will be
   inserted at the top scene item.  Does not signal a refresh.

   :param scene: Scene to add the group to
   :param name:  Name of the group
   :param items: Array of scene items to put in a group
   :param count: Number of scene items in the array
   :param signal: If *true*, signals the scene with the *refresh*
                  signal
   :return:      The new group's scene item

---------------------

.. function:: obs_sceneitem_t *obs_scene_get_group(obs_scene_t *scene, const char *name)

   Finds a group within a scene by its name.

   :param scene: Scene to find the group within
   :param name:  The name of the group to find
   :return:      The group scene item, or *NULL* if not found

---------------------

.. function:: obs_scene_t *obs_group_from_source(const obs_source_t *source)

   :return: The group context, or *NULL* if not a group.  Does not
            increase the reference

---------------------

.. function:: obs_scene_t *obs_group_or_scene_from_source(const obs_source_t *source)

   :return: The context for the source, regardless of if it is a
            group or a scene.  *NULL* if neither.  Does not increase
            the reference

---------------------

.. function:: bool obs_sceneitem_is_group(obs_sceneitem_t *item)

   :param item: Scene item
   :return:     *true* if scene item is a group, *false* otherwise

---------------------

.. function:: obs_scene_t *obs_sceneitem_group_get_scene(const obs_sceneitem_t *group)

   :param group: Group scene item
   :return:      Scene of the group, or *NULL* if not a group

---------------------

.. function:: void obs_sceneitem_group_ungroup(obs_sceneitem_t *group)

   Ungroups the specified group.  Scene items within the group will be
   placed where the group was.  Does not signal the scene with the
   *refresh* signal.

---------------------

.. function:: void obs_sceneitem_group_ungroup2(obs_sceneitem_t *group, bool signal)

   Ungroups the specified group.  Scene items within the group will be
   placed where the group was.

   :param group: Group scene item
   :param signal: If *true*, signals the scene with the *refresh*
                  signal

---------------------

.. function:: void obs_sceneitem_group_add_item(obs_sceneitem_t *group, obs_sceneitem_t *item)

   Adds a scene item to a group.

---------------------

.. function:: void obs_sceneitem_group_remove_item(obs_sceneitem_t *item)

   Removes a scene item from a group.  The item will be placed before
   the group in the main scene.

---------------------

.. function:: obs_sceneitem_t *obs_sceneitem_get_group(obs_scene_t *scene, obs_sceneitem_t *item)

   Returns the parent group of a scene item.

   :param scene: Scene to find the group within
   :param item: Scene item to get the group of
   :return:     The parent group of the scene item, or *NULL* if not in
                a group

---------------------

.. function:: void obs_sceneitem_group_enum_items(obs_sceneitem_t *group, bool (*callback)(obs_scene_t*, obs_sceneitem_t*, void*), void *param)

   Enumerates scene items within a group.

   Callback function returns true to continue enumeration, or false to end
   enumeration.

   Use :c:func:`obs_sceneitem_addref()` if you want to retain a
   reference after obs_sceneitem_group_enum_items finishes.


---------------------

.. function:: void obs_sceneitem_defer_group_resize_begin(obs_sceneitem_t *item)
.. function:: void obs_sceneitem_defer_group_resize_end(obs_sceneitem_t *item)

   Allows the ability to call any one of the transform functions on
   scene items within a group without updating the internal matrices of
   the group until obs_sceneitem_defer_group_resize_end has been called.

   This is necessary if the user is resizing items while they are within
   a group, as the group's transform will automatically update its
   transform every frame otherwise.
