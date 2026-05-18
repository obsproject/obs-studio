OBS Studio Plus
===============

OBS Studio Plus is a community fork of `OBS Studio <https://obsproject.com>`_
that aims to bring native multi-canvas streaming, first-class multi-destination
output, and a modern UI to the world's most popular live-streaming software.

This fork preserves full compatibility with upstream OBS plugins, scenes, and
profiles. Everything that works in OBS Studio works in OBS Studio Plus.

Status
------

Early development. See `ROADMAP.md <ROADMAP.md>`_ for the planned feature set
and current progress.

What's different from upstream OBS?
-----------------------------------

The upstream OBS Studio project has quietly built much of the infrastructure
needed for modern streaming workflows (multi-canvas, multi-output) but has not
yet surfaced these capabilities to users. OBS Studio Plus exposes them, plus
adds new capabilities on top:

- **Native vertical + horizontal canvases** — stream 16:9 to Twitch and 9:16
  to TikTok / Shorts / Reels simultaneously, from a single OBS instance.
- **First-class multi-destination streaming** — push your stream to multiple
  services at once with per-destination bitrate and resolution settings.
- **Modern UI shell** — refreshed theming, restructured panels, unified
  Go-Live workflow.
- **Local-first AI features** — auto-framing, live captions, and improved
  noise suppression as opt-in plugins. No cloud dependencies for core features.

License
-------

OBS Studio Plus is distributed under the GNU General Public License v2 (or any
later version), the same license as upstream OBS Studio. See the accompanying
``COPYING`` file for details.

Quick Links
-----------

- This fork's repository: https://github.com/OgBops/obs-studio-plus

- Upstream OBS Studio: https://github.com/obsproject/obs-studio

- Upstream OBS website: https://obsproject.com

- Upstream build instructions:
  https://github.com/obsproject/obs-studio/wiki/Install-Instructions

- Upstream developer/API documentation: https://obsproject.com/docs

Building from Source
--------------------

Build instructions are identical to upstream OBS Studio. See the upstream wiki
for platform-specific guidance:
https://github.com/obsproject/obs-studio/wiki/Install-Instructions

Quick start (macOS)::

    cmake --preset macos
    cmake --build --preset macos

Quick start (Linux)::

    cmake --preset ubuntu-x86_64
    cmake --build --preset ubuntu-x86_64

Contributing
------------

Contributions are welcome. Before opening a pull request, please:

- Read the upstream coding guidelines:
  https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.md

- Follow the upstream code style:
  https://github.com/obsproject/obs-studio/blob/master/CODESTYLE.md

- Open work-in-progress changes against the ``dev`` branch, not ``master``.
  ``master`` tracks upstream OBS Studio so we can pull in their changes
  cleanly.

Acknowledgements
----------------

OBS Studio Plus is built on top of the work of the OBS Studio team and its
hundreds of contributors. See the ``AUTHORS`` file for the full list. This
fork would not exist without their work.

If you would like to support upstream OBS Studio (which we recommend, since
this fork depends on their continued work), see
https://obsproject.com/contribute.
