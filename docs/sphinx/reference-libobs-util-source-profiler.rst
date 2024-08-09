Source Profiler
===============

The source profiler is used to get information about individual source's performance.

.. struct:: profiler_result

.. member:: uint64_t profiler_result.tick_avg
            uint64_t profiler_result.render_avg
   
   Average execution time of this source's tick and render functions within the sampled timeframe (5 seconds).
   
   Note that a source will be ticked only once per frame, but may be rendered multiple times.

.. member:: uint64_t profiler_result.tick_max
            uint64_t profiler_result.render_max

   Maximum execution time of this source's tick and render functions within the sampled timeframe (5 seconds).

.. member:: uint64_t profiler_result.render_gpu_avg
            uint64_t profiler_result.render_gpu_max
   
   Average and maximum execution time for GPU rendering to execute within the sampled timeframe (5 seconds).
   
   Note that GPU timing is not supported on macOS and is of limited accuracy due to variations in GPU load/clock speed.

.. member:: uint64_t profiler_result.render_sum
            uint64_t profiler_result.render_gpu_sum

   Sum of all CPU/GPU render time in a frame, averaged over the sampled timeframe (5 seconds).
   
   For example, assuming a source with perfect consistency in its render time that gets rendered twice in a frame and a value for :c:member:`profiler_result.render_avg` of `1000000` (1 ms), will have a value for :c:member:`profiler_result.render_sum` of `2000000` (2 ms).

.. member:: double profiler_result.async_fps

   Framerate calculated from average time delta between async frames submitted via :c:func:`obs_source_output_video2()`.
   
   Only valid for async sources (e.g. Media Source).

.. type:: struct profiler_result profiler_result_t

.. code:: cpp

   #include <util/source-profiler.h>


Source Profiler Functions
---------------------

.. function:: void source_profiler_enable(bool enable)

   Enables or disables the source profiler.
   The profiler will then start or stop collecting data with the next rendered frame.
   
   Note that enabling the profiler may have a small performance penalty.

   :param enable: Whether or not to enable the source profiler.

---------------------

.. function:: void source_profiler_gpu_enable(bool enable)

   Enables or disables GPU profiling (not available on macOS).
   GPU profiling will start or stop with the next frame OBS is rendering.
   
   Note that GPU profiling may have a larger performance impact.

   :param enable: Whether or not to enable GPU profiling.

---------------------

.. function:: profiler_result_t *source_profiler_get_result(obs_source_t *source)

   Returns profiling information for the provided `source`.
   
   Note that result must be freed with :c:func:`bfree()`.

   :param source: Source to get profiling information for
   :return:       Pointer to `profiler_result_t` with data, `NULL` otherwise.

---------------------

.. function:: bool source_profiler_fill_result(obs_source_t *source, profiler_result_t *result)

   Fill a preexisting `profiler_result_t` object with data for `source`.
   
   This function exists to avoid having to allocate new memory each time a profiling result is queried.

   :param source: Source to get profiling informatio for
   :param result: Result object to fill
   :return:       *true* if data for the source exists, *false* otherwise
