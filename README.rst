OBS Studio Plus
===============

|build-status|

This repository is the **engine** for the Plus desktop app — a future
cross-platform streaming application that covers general streaming,
vertical-first content (TikTok / Shorts / Reels), and podcast/interview
workflows.

It is a fork of `OBS Studio <https://obsproject.com>`_ that exists for one
specific purpose: to produce a headless ``plus-host`` binary which runs
``libobs`` in a sidecar process and exposes its functionality over IPC.
The Plus desktop app (separate repository, written in Rust + Tauri) talks
to ``plus-host`` over a named pipe.

This is the same architecture Streamlabs Desktop uses with their
``obs-studio-node`` — running libobs out of process keeps the GPL boundary
at the IPC layer so the Plus app itself can be developed under any
license, while the engine here remains GPL v2 like upstream OBS.

Status
------

**Pre-alpha.** The fork has just pivoted from "OBS with extra Qt features"
to "engine for a separate app." Phase 0 (a 2-week feasibility spike) is
the current work. There is no shippable product yet.

Today, a build from ``dev`` looks and behaves exactly like vanilla OBS
Studio — that is intentional. The engine wrapper and the Plus app land in
later phases. See `PLUS_PLAN.md <PLUS_PLAN.md>`_ for the full plan.

Repository layout
-----------------

================================  ===============================================
Branch / path                     What it is
================================  ===============================================
``master``                        Tracks upstream OBS Studio, untouched.
``dev``                           Integration branch. Branding, CI fixes for
                                  the no-tags state, engine entry points land
                                  here. CI builds all 6 platforms green.
``engine/spike``                  Phase 0 spike. Created at the start of phase 0,
                                  deleted after the retro.
``engine/host``                   ``plus-host`` proper (created in phase 1).
``libobs/``, ``plugins/``,        Read-only mirror of upstream OBS. Modifying
``frontend/`` C++                 these files fails CI.
================================  ===============================================

The hard rule: see `CONTRIBUTING.md <CONTRIBUTING.md>`_. OBS core is
read-only in this fork. The CI workflow ``no-core-changes.yaml`` enforces
the rule by diffing against ``upstream/master``.

Downloads
---------

CI produces prebuilt binaries for every push to ``dev`` and ``engine/**``.
Today these are vanilla OBS builds. Artifacts (macOS arm64/x86_64 ``.dmg``,
Ubuntu 24.04 ``.deb``, Flatpak, Windows x64/arm64 zips) are attached to
each successful run on the
`Actions tab <https://github.com/OgBops/obs-studio-plus/actions>`_ and
expire 90 days after the build.

There is no signed release yet. macOS builds are ad-hoc signed; clear the
quarantine attribute after downloading::

    xattr -dr com.apple.quarantine /path/to/OBS.app

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

License
-------

The engine code in this repository (everything that links ``libobs``,
including any code we add under ``engine/``) is distributed under the GNU
General Public License v2 (or any later version), the same license as
upstream OBS Studio. See ``COPYING`` for details.

The Plus desktop app — which lives in a separate repository — does not
link ``libobs``. It communicates with ``plus-host`` over IPC. That
separation is load-bearing for keeping the Plus app closed-source-capable.
Don't break it.

Quick links
-----------

- This fork: https://github.com/OgBops/obs-studio-plus
- Plan: `PLUS_PLAN.md <PLUS_PLAN.md>`_
- Fork rules: `CONTRIBUTING.md <CONTRIBUTING.md>`_
- Upstream OBS Studio: https://github.com/obsproject/obs-studio
- Upstream developer/API documentation: https://obsproject.com/docs

Acknowledgements
----------------

OBS Studio Plus is built on top of the work of the OBS Studio team and its
hundreds of contributors. See the ``AUTHORS`` file for the full list. This
fork would not exist without their work.

If you would like to support upstream OBS Studio (which is recommended,
since this fork depends on their continued work), see
https://obsproject.com/contribute.

.. |build-status| image:: https://github.com/OgBops/obs-studio-plus/actions/workflows/push.yaml/badge.svg?branch=dev
   :target: https://github.com/OgBops/obs-studio-plus/actions/workflows/push.yaml
   :alt: dev branch build status
