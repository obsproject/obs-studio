OBS Studio Plus
===============

|build-status|

OBS Studio Plus is a personal fork of `OBS Studio <https://obsproject.com>`_
that explores three additions on top of upstream:

1. **Native multi-destination streaming** — push the same stream to several
   services in parallel, with per-destination configuration.
2. **Native multi-canvas** — surface the existing ``obs_canvas_t`` engine
   (already complete in ``libobs/obs-canvas.c``) so users can compose 16:9 +
   9:16 outputs from a single OBS instance.
3. **Local-first AI plugins** — captions (whisper.cpp), auto-framing, and
   improved noise suppression as optional drop-in plugins. No cloud
   dependencies.

Compatibility with upstream OBS plugins, scenes, and profiles is preserved.

Current status
--------------

Early development. The fork is being built incrementally; today's branches
contain plumbing rather than user-visible features.

================================  ==============================================
Branch                            Status
================================  ==============================================
``master``                        Tracks upstream OBS, untouched
``dev``                           Integration branch. Fork branding, CI fixes
                                  for the no-tags state, and other plumbing.
                                  CI builds all 6 platforms green.
``feature/multi-output-foundation`` Header-only structural seam
                                  (``additionalStreamOutputs`` vector +
                                  ``AllStreamOutputs()`` helper) for the
                                  multi-destination work to land on.
``feature/multi-destination-dock``  Qt dock scaffold (~316 LOC). Compiles in
                                  isolation; not wired into ``OBSBasic`` yet.
``feature/ai-captions-plugin``    Plugin scaffold (~379 LOC) with a no-op
                                  backend. Pending whisper.cpp integration.
================================  ==============================================

Right now, a binary built from ``dev`` looks and behaves exactly like upstream
OBS — that is intentional. The user-visible features land once the foundation
is in place.

Downloads
---------

CI produces prebuilt binaries for every push to ``dev`` and ``feature/**``.
Artifacts (macOS arm64/x86_64 ``.dmg``, Ubuntu 24.04 ``.deb``, Flatpak,
Windows x64/arm64 zips) are attached to each successful run on the
`Actions tab <https://github.com/OgBops/obs-studio-plus/actions>`_ and expire
90 days after the build.

There is no signed release yet. macOS builds are ad-hoc signed; clear the
quarantine attribute after downloading::

    xattr -dr com.apple.quarantine /path/to/OBS.app

Roadmap
-------

See `ROADMAP.md <ROADMAP.md>`_ for the planned feature set and milestone
ordering.

Building from source
--------------------

Build instructions are identical to upstream OBS Studio. The upstream wiki
covers platform setup:
https://github.com/obsproject/obs-studio/wiki/Install-Instructions

Quick start (macOS, requires full Xcode.app)::

    cmake --preset macos
    cmake --build --preset macos

Quick start (Linux)::

    cmake --preset ubuntu-x86_64
    cmake --build --preset ubuntu-x86_64

A Command-Line-Tools-only macOS build using Ninja is in progress on
``feature/macos-clt-build`` but is not currently the recommended local path.

Contributing
------------

This is a personal fork. External contributions are welcome but not actively
solicited at this stage. If you do open a pull request:

- Read the upstream coding guidelines:
  https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.md
- Follow the upstream code style:
  https://github.com/obsproject/obs-studio/blob/master/CODESTYLE.md
- Open work-in-progress changes against ``dev``, not ``master``.
  ``master`` tracks upstream OBS Studio so upstream changes can be pulled in
  cleanly.
- Run the project's formatters before pushing — CI rejects unformatted
  changes::

      ./build-aux/run-format clang-format <files>
      ./build-aux/run-gersemi <files>

License
-------

OBS Studio Plus is distributed under the GNU General Public License v2 (or
any later version), the same license as upstream OBS Studio. See ``COPYING``
for details.

Quick links
-----------

- This fork: https://github.com/OgBops/obs-studio-plus
- Upstream OBS Studio: https://github.com/obsproject/obs-studio
- Upstream OBS website: https://obsproject.com
- Upstream developer/API documentation: https://obsproject.com/docs

Acknowledgements
----------------

OBS Studio Plus is built on top of the work of the OBS Studio team and its
hundreds of contributors. See the ``AUTHORS`` file for the full list. This
fork would not exist without their work.

If you would like to support upstream OBS Studio (which is recommended, since
this fork depends on their continued work), see
https://obsproject.com/contribute.

.. |build-status| image:: https://github.com/OgBops/obs-studio-plus/actions/workflows/push.yaml/badge.svg?branch=dev
   :target: https://github.com/OgBops/obs-studio-plus/actions/workflows/push.yaml
   :alt: dev branch build status
