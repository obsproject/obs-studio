# OBS Studio Plus — Roadmap

This roadmap tracks the planned feature set for OBS Studio Plus, the goals
behind each anchor feature, and the current status. Things will change as we
build.

## Guiding principles

- **Don't break upstream compatibility.** Scenes, profiles, plugins, and
  collections from upstream OBS must continue to load. We add capability
  rather than diverge.
- **Local-first.** Anything that touches a streamer's content (audio, video,
  metadata) runs on their machine by default. Cloud features are opt-in and
  clearly disclosed.
- **Stay mergeable with upstream.** `master` tracks upstream OBS Studio. All
  fork-specific work happens on `dev` and feature branches that periodically
  merge `master`. We will surface upstream-equivalent features the moment
  upstream ships them, and keep our changes small enough to rebase.
- **Plugins over core hacks.** Whenever a feature can ship as a plugin, it
  ships as a plugin. The only changes that go into `libobs` and the frontend
  are ones that genuinely require it (e.g., multi-canvas UI surfacing).

## Anchor features

These three are the reason this fork exists. Everything else is a stretch
goal.

### 1. Multi-destination streaming (Status: planned, v0.1 target)

Stream to N services simultaneously from one OBS instance, with per-destination
bitrate, resolution, and encoder settings.

**Why now:** Upstream OBS supports multi-output through `obs_output_t`'s API,
and the `WHIPSimulcastEncoders` pattern in
`frontend/utility/WHIPSimulcastEncoders.hpp` already creates parallel encoder
chains. The core supports it. The UI doesn't.

**Approach:**
- Vectorize `streamOutput` in `frontend/utility/BasicOutputHandler.{hpp,cpp}`
  from a single output to a `std::vector<StreamTarget>`.
- New dock: `frontend/widgets/MultiDestinationDock.{hpp,cpp}` listing each
  target with live status (connected / bitrate / dropped frames).
- Settings UI: extend `frontend/settings/OBSBasicSettings_Stream.cpp` to a
  list of services rather than a single service.
- Optional: shared encoder mode (one encode, N muxers) when destinations
  match resolution; per-destination encoders otherwise.

**Risk:** Encoder licensing and per-platform service rules (e.g., Twitch
ToS on simultaneous streaming). Document, don't enforce.

### 2. Native multi-canvas (Status: planned, v0.2 target)

Run a 16:9 horizontal canvas and a 9:16 vertical canvas simultaneously from
one OBS instance. Stream each independently. Switch scenes per-canvas.

**Why now:** The upstream `obs_canvas_t` API in `libobs/obs-canvas.c` is
complete. `OBSBasic.hpp` already declares `std::vector<OBS::Canvas> canvases`.
The C++ wrapper (`frontend/utility/OBSCanvas.{hpp,cpp}`) is in place.
`obs_view_add2` accepts a per-canvas `obs_video_info`. The frontend UI is the
last missing piece.

**Approach:**
- Surface canvas management UI in `frontend/widgets/OBSBasic_Canvases.cpp`
  (currently 62 lines of barebones plumbing).
- Multi-preview: extend `frontend/widgets/OBSBasic_Preview.cpp` to render N
  canvases (tabbed or side-by-side).
- Per-canvas scene/source lists in
  `frontend/widgets/OBSBasic_{Scenes,SceneItems}.cpp`.
- Audit every implicit `obs_get_video()` call in the frontend and route
  through the active canvas instead.

**Risk:** Hundreds of frontend call sites assume one canvas. The work is
diffuse, not deep. Estimate 4-6 weeks for one engineer.

### 3. Modern UI + AI plugin suite (Status: planned, v0.2 target)

A refreshed visual identity plus a starter set of local-AI features that
replace third-party paid plugins.

**Why now:** Streamlabs and Streamerbot are eating OBS's mindshare on
polish. NVIDIA Broadcast and Aitum Vertical are paid third-party tools doing
things OBS could do natively.

**Approach:**

- **Theme:** new `.ovt` theme file in `frontend/data/themes/` based on the
  existing Yami variable system. Refreshed icon set.
- **AI Captions plugin** (`plugins/obs-ai-captions/`): audio source filter
  wrapping `whisper.cpp`. Real-time captions overlaid on canvas, optional
  translation. Modeled after `plugins/obs-filters/noise-suppress-filter.c`.
- **AI Auto-framing plugin** (`plugins/obs-ai-autoframe/`): video source
  filter running a small subject-detection model (MediaPipe or YOLO-nano)
  and feeding the result into the source's transform.
- **DeepFilterNet noise suppression**: extend
  `plugins/obs-filters/noise-suppress-filter.c` with a third backend
  alongside RNNoise and NVIDIA NoiseRemoval.

**Risk:** Model size and licensing. Default to small, permissively-licensed
models. Larger models behind opt-in download.

## Stretch features (not committed)

- Auto-highlight detection from stream recordings (post-stream offline tool).
- Smart scene switching (replace Advanced Scene Switcher rule trees with a
  small classifier).
- Unified Go-Live panel that combines stream key entry, multi-destination
  config, and pre-stream checks into one dialog.
- Built-in chat overlay with multi-platform aggregation (Twitch + YouTube +
  Kick into one feed).

## Versioning

- **v0.1** — Multi-destination streaming. Modern theme. Renamed brand
  (this README, splash screen, application name).
- **v0.2** — Native multi-canvas UI. AI captions plugin shipping by default.
- **v0.3** — Auto-framing plugin. DeepFilterNet noise suppression. Unified
  Go-Live panel.

Versions are aspirational, not contractual. We ship when it's stable.

## How to contribute

See `README.rst`. Open issues against this fork for fork-specific work; open
issues against upstream OBS for bugs that exist there too.
