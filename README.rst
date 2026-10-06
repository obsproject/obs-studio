Himothee Studio
===============

Himothee Studio is a streaming and production application based on
`OBS Studio <https://github.com/obsproject/obs-studio>`_.

The project starts from **OBS Studio 32.2.2** and is focused on adding
native multi-destination streaming while retaining the mature capture,
scene, source, audio, recording, replay-buffer, encoder, and plugin
capabilities provided by OBS Studio.

Project Status
--------------

**Early development / pre-release.**

The current development baseline is OBS Studio 32.2.2
(commit ``ba2f32bdf791005443988a4955e963663e16b1ed``).

Development takes place on the ``himothee-dev`` branch. The ``master``
branch is intentionally kept close to upstream OBS so upstream changes
can be reviewed without mixing them directly into product development.

Primary Goals
-------------

* Native multistreaming to Twitch, YouTube, Kick, and custom RTMP targets.
* Shared-encoder multistreaming for low GPU overhead.
* Independent per-destination encoding as an advanced mode.
* Per-destination bitrate, server, stream key, reconnect, and status handling.
* Per-destination audio routing in a later development stage.
* Integrated production tools such as media triggers, macros, counters,
  timers, alerts, and browser-based overlays.
* A streamlined interface that remains familiar to existing OBS users.

Planned Development
-------------------

``v0.1`` - Establish a clean OBS Studio 32.2.2 baseline.

``v0.2`` - Himothee Studio application identity and branding.

``v0.3`` - Multi-output streaming core.

``v0.4`` - Multistream Manager user interface.

``v0.5`` - Shared-encoder multistreaming.

``v0.6`` - Independent per-destination encoders.

``v0.7`` - Destination health, reconnect, error reporting, and statistics.

``v0.8`` - Per-destination audio routing.

Further stages will integrate alerts, media triggers, macros, counters,
timers, browser overlays, and external control.

Documentation
-------------

Project-specific documentation lives in ``docs/himothee/``:

* ``DEVELOPMENT.md`` - development workflow and branch strategy.
* ``ROADMAP.md`` - staged implementation plan.
* ``MULTISTREAM_ARCHITECTURE.md`` - multi-output architecture.
* ``UPSTREAM.md`` - how this fork tracks OBS Studio.

Building
--------

Until the Himothee-specific build pipeline is introduced, the project
uses the OBS Studio build system inherited from the 32.2.2 baseline.

See ``docs/himothee/DEVELOPMENT.md`` before making changes.

Upstream OBS Studio
-------------------

Himothee Studio is a derivative of OBS Studio. The original OBS Studio
project is available at:

https://github.com/obsproject/obs-studio

OBS Studio is developed by the OBS Project and its contributors.
Himothee Studio is an independent fork and is not an official OBS
Project distribution.

License
-------

This repository retains the OBS Studio GNU General Public License
version 2 or later licensing. See the repository's ``COPYING`` file and
applicable source-file notices for details.

When distributing Himothee Studio builds, the corresponding GPL source
and required notices must remain available.
