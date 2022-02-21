OBS Studio Backend Design
=========================
The OBS Studio backend is powered by the library libobs.  Libobs
provides the main pipeline, the video/audio subsystems, and the general
framework for all plugins.


Libobs Plugin Objects
---------------------
Libobs is designed to be modular, where adding modules will add custom
functionality.  There are four libobs objects that you can make plugins
for:

- :ref:`plugins_sources` -- Sources are used to render video and/or
  audio on stream.  Things such as capturing displays/games/audio,
  playing a video, showing an image, or playing audio.  Sources can also
  be used to implement audio and video filters.

- :ref:`plugins_outputs` -- Outputs allow the ability to output the
  currently rendering audio/video.  Streaming and recording are two
  common examples of outputs, but not the only types of outputs.
  Outputs can receive the raw data or receive encoded data.

- :ref:`plugins_encoders` -- Encoders are OBS-specific implementations
  of video/audio encoders, which are used with outputs that use
  encoders.  x264, NVENC, Quicksync are examples of encoder
  implementations.

- :ref:`plugins_services` -- Services are custom implementations of
  streaming services, which are used with outputs that stream.  For
  example, you could have a custom implementation for streaming to
  Twitch, and another for YouTube to allow the ability to log in and use
  their APIs to do things such as get the RTMP servers or control the
  channel.
  
*(Author's note: the service API is incomplete as of this writing)*


Libobs Threads
--------------
There are three primary threads spawned by libobs on initialization:

- The obs_graphics_thread_ function used exclusively for rendering in
  `libobs/obs-video.c`_

- The video_thread_ function used exclusively for video encoding/output
  in `libobs/media-io/video-io.c`_

- The audio_thread_ function used for all audio
  processing/encoding/output in `libobs/media-io/audio-io.c`_

*(Author's note: obs_graphics_thread was originally named
obs_video_thread; it was renamed as of this writing to prevent confusion
with video_thread)*


.. _output_channels:

Output Channels
---------------
Rendering video or audio starts from output channels.  You assign a
source to an output channel via the :c:func:`obs_set_output_source()`
function.  The *channel* parameter can be any number from
0..(MAX_CHANNELS_-1).  You may initially think that this is how you
display multiple sources at once; however, sources are hierarchical.
Sources such as scenes or transitions can have multiple sub-sources, and
those sub-sources in turn can have sub-sources and so on (see
:ref:`displaying_sources` for more information).  Typically, you would
use scenes to draw multiple sources as a group with specific transforms
for each source, as a scene is just another type of source.  The
"channel" design allows for highly complex video presentation setups.
The OBS Studio front-end has yet to even fully utilize this back-end
design for its rendering, and currently only uses one output channel to
render one scene at a time.  It does however utilize additional channels
for things such as global audio sources which are set in audio settings.

*(Author's note: "Output channels" are not to be confused with output
objects or audio channels.  Output channels are used to set the sources
you want to output, and output objects are used for actually
streaming/recording/etc.)*


General Video Pipeline Overview
-------------------------------
The video graphics pipeline is run from two threads: a dedicated
graphics thread that renders preview displays as well as the final mix
(the obs_graphics_thread_ function in `libobs/obs-video.c`_), and a
dedicated thread specific to video encoding/output (the video_thread_
function in `libobs/media-io/video-io.c`_).

Sources assigned to output channels will be drawn from channels
0..(MAX_CHANNELS_-1).  They are drawn on to the final texture which will
be used for output `[1]`_.  Once all sources are drawn, the final
texture is converted to whatever format that libobs is set to (typically
a YUV format).  After being converted to the back-end video format, it's
then sent along with its timestamp to the current video handler,
`obs_core_video::video`_.

It then puts that raw frame in a queue of MAX_CACHE_SIZE_ in the `video
output handler`_.  A semaphore is posted, then the video-io thread will
process frames as it's able.  If the video frame queue is full, it will
duplicate the last frame in the queue in an attempt to reduce video
encoding complexity (and thus CPU usage) `[2]`_.  This is why you may
see frame skipping when the encoder can't keep up.  Frames are sent to
any raw outputs or video encoders that are currently active `[3]`_.

If it's sent to a video encoder object (`libobs/obs-encoder.c`_), it
encodes the frame and sends the encoded packet off to the outputs that
encoder is connected to (which can be multiple).  If the output takes
both encoded video/audio, it puts the packets in an interleave queue to
ensure encoded packets are sent in monotonic timestamp order `[4]`_.

The encoded packet or raw frame is then sent to the output.


General Audio Pipeline Overview
-------------------------------
The audio pipeline is run from a dedicated audio thread in the audio
handler (the `audio_thread`_ function in `libobs/media-io/audio-io.c`_);
assuming that AUDIO_OUTPUT_FRAMES_ is set to 1024, the audio thread
"ticks" (processes audio data) once every 1024 audio samples (around
every 21 millisecond intervals at 48khz), and calls the audio_callback_
function in `libobs/obs-audio.c`_ where most of the audio processing is
accomplished.

A source with audio will output its audio via the
obs_source_output_audio_ function, and that audio data will be appended
or inserted in to the circular buffer `obs_source::audio_input_buf`_.
If the sample rate or channel count does not match what the back-end is
set to, the audio is automatically remixed/resampled via swresample
`[5]`_.  Before insertion, audio data is also run through any audio
filters attached to the source `[6]`_.

Each audio tick, the audio thread takes a reference snapshot of the
audio source tree (stores references of all sources that output/process
audio) `[7]`_.  On each audio leaf (audio source), it takes the closest
audio (relative to the current audio thread timestamp) stored in the
circular buffer `obs_source::audio_input_buf`_, and puts it in
`obs_source::audio_output_buf`_.

Then, the audio samples stored in `obs_source::audio_output_buf`_ of the
leaves get sent through their parents in the source tree snapshot for
mixing or processing at each source node in the hierarchy `[8]`_.
Sources with multiple children such as scenes or transitions will
mix/process their children's audio themselves via the
`obs_source_info::audio_render`_ callback.  This allows, for example,
transitions to fade in the audio of one source and fade in the audio of
a new source when they're transitioning between two sources.  The mix or
processed audio data is then stored in `obs_source::audio_output_buf`_
of that node similarly, and the process is repeated until the audio
reaches the root nodes of the tree.

Finally, when the audio has reached the base of the snapshot tree, the
audio of all the sources in each output channel are mixed together for a
final mix `[9]`_.  That final mix is then sent to any raw outputs or
audio encoders that are currently active `[10]`_.

If it's sent to an audio encoder object (`libobs/obs-encoder.c`_), it
encodes the audio data and sends the encoded packet off to the outputs
that encoder is connected to (which can be multiple).  If the output
takes both encoded video/audio, it puts the packets in an interleave
queue to ensure encoded packets are sent in monotonic timestamp order
`[4]`_.

The encoded packet or raw audio data is then sent to the output.

.. _obs_graphics_thread: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-video.c#L588-L651
.. _libobs/obs-audio.c: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-audio.c
.. _libobs/obs-video.c: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-video.c
.. _video_thread: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/media-io/video-io.c#L169-L195
.. _libobs/media-io/video-io.c: https://github.com/jp9000/obs-studio/blob/master/libobs/media-io/video-io.c
.. _video output handler: https://github.com/jp9000/obs-studio/blob/master/libobs/media-io/video-io.c
.. _audio_thread: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/media-io/audio-io.c#L241-L282
.. _libobs/media-io/audio-io.c: https://github.com/jp9000/obs-studio/blob/master/libobs/media-io/audio-io.c
.. _MAX_CHANNELS: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-defs.h#L20-L21
.. _[1]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-video.c#L99-L129
.. _obs_core_video::video: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-internal.h#L250
.. _MAX_CACHE_SIZE: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/media-io/video-io.c#L34
.. _[2]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/media-io/video-io.c#L431-L434
.. _[3]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/media-io/video-io.c#L115-L167
.. _libobs/obs-encoder.c: https://github.com/jp9000/obs-studio/blob/master/libobs/obs-encoder.c
.. _[4]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-output.c#L1382-L1439
.. _AUDIO_OUTPUT_FRAMES: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/media-io/audio-io.h#L30
.. _audio_callback: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-audio.c#L367-L485
.. _obs_source_output_audio: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-source.c#L2578-L2608
.. _obs_source::audio_input_buf: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-source.c#L1280-L1283
.. _[5]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-source.c#L2561-L2563
.. _[6]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-source.c#L2591
.. _[7]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-audio.c#L393-L415
.. _obs_source::audio_output_buf: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-internal.h#L580
.. _[8]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-audio.c#L417-L423
.. _obs_source_info::audio_render: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-source.h#L410-L412
.. _[9]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/obs-audio.c#L436-L453
.. _[10]: https://github.com/jp9000/obs-studio/blob/2c58185af3c85f4e594a4c067c9dfe5fa4b5b0a9/libobs/media-io/audio-io.c#L144-L165
