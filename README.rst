Mosaic
======

|build-status|

Mosaic is a cross-platform desktop streaming application targeting three
modes: general streaming, vertical-first content (TikTok / Shorts / Reels),
and podcast/interview workflows.

This repository hosts ``mosaic-host``, the engine. It is a fork of
`OBS Studio <https://obsproject.com>`_ stripped to a headless binary that
runs the upstream broadcasting engine in a sidecar process and exposes its
functionality over local IPC. The Mosaic desktop app (separate repository,
written in Rust + Tauri) talks to ``mosaic-host`` over a named pipe.

Running the broadcasting engine out of process keeps the GPL boundary at
the IPC layer so the Mosaic app itself can be developed under any license,
while ``mosaic-host`` remains GPL v2 like the upstream code it links.

This is **not** a re-distribution of OBS Studio. Mosaic is a separate
product. See the `OBS Studio project <https://obsproject.com>`_ for the
original software.

Status
------

**Pre-alpha.** The repository is in the middle of a foundational rebuild.
Phase 0 (a 2-week feasibility spike) is the current work. There is no
shippable product yet, and no public binary releases.

Today, a build from ``dev`` is just the upstream broadcasting source —
that is intentional. The engine wrapper and the Mosaic app land in later
phases. See `MOSAIC_PLAN.md <MOSAIC_PLAN.md>`_ for the full plan.

Repository layout
-----------------

================================  ===============================================
Branch / path                     What it is
================================  ===============================================
``master``                        Tracks upstream OBS Studio, untouched.
``dev``                           Integration branch. Mosaic-specific docs, CI
                                  fixes, engine entry points land here.
``engine/spike``                  Phase 0 spike. Created at the start of phase 0,
                                  deleted after the retro.
``engine/host``                   ``mosaic-host`` proper (created in phase 1).
``libobs/``, ``plugins/``,        Read-only mirror of the upstream broadcasting
``frontend/`` C++/Qt              source. Modifying these files fails CI.
================================  ===============================================

The hard rule: see `CONTRIBUTING.md <CONTRIBUTING.md>`_. Upstream broadcasting
core code is read-only in this fork. The CI workflow ``no-core-changes.yaml``
enforces the rule by diffing against ``upstream/master``.

Downloads
---------

There are no public binary releases at this time. CI builds the upstream
broadcasting source on every push to ``dev`` and ``engine/**`` for our own
verification, but those builds are not redistributed. Public Mosaic
releases will begin once the Mosaic desktop app exists in its own
repository under its own application name.

Building from source
--------------------

Build instructions for ``mosaic-host`` follow upstream OBS Studio's build
process. The upstream wiki covers platform setup:
https://github.com/obsproject/obs-studio/wiki/Install-Instructions

Quick start (macOS, requires full Xcode.app)::

    cmake --preset macos
    cmake --build --preset macos

Quick start (Linux)::

    cmake --preset ubuntu-x86_64
    cmake --build --preset ubuntu-x86_64

License
-------

The engine code in this repository (everything that links the upstream
broadcasting engine, including any code we add under ``engine/``) is
distributed under the GNU General Public License v2 (or any later
version), the same license as the upstream source. See ``COPYING`` for
details.

The Mosaic desktop app — which lives in a separate repository — does not
link the upstream engine. It communicates with ``mosaic-host`` over IPC.
That separation is load-bearing for keeping the Mosaic app
closed-source-capable. Don't break it.

Trademark notice
----------------

"OBS" and "OBS Studio" are trademarks of the OBS Project. Mosaic is an
independent project and is not affiliated with, endorsed by, or sponsored
by the OBS Project. References to OBS Studio in this repository refer
solely to the upstream open-source project from which the engine code is
derived.

Quick links
-----------

- This repository: https://github.com/OgBops/mosaic
- Plan: `MOSAIC_PLAN.md <MOSAIC_PLAN.md>`_
- Fork rules: `CONTRIBUTING.md <CONTRIBUTING.md>`_
- Upstream OBS Studio: https://github.com/obsproject/obs-studio

Acknowledgements
----------------

Mosaic is built on top of the work of the OBS Studio team and its
hundreds of contributors. See the ``AUTHORS`` file for the full list.
This project would not exist without their work.

If you would like to support upstream OBS Studio (which is recommended,
since this fork depends on their continued work), see
https://obsproject.com/contribute.

.. |build-status| image:: https://github.com/OgBops/mosaic/actions/workflows/push.yaml/badge.svg?branch=dev
   :target: https://github.com/OgBops/mosaic/actions/workflows/push.yaml
   :alt: dev branch build status
