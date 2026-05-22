# Mosaic — Project Plan

This document is the north star for the Mosaic project. When in doubt about
scope, refer back here. When this document is wrong, update it before
writing code that contradicts it.

## What we're building

**Mosaic** — a cross-platform desktop streaming app, owned end-to-end,
that covers three streaming modes: general streaming (OBS-like), vertical-
first (TikTok/Shorts/Reels), and podcast/interview-style.

## Architecture

Mosaic is two binaries:

```
┌─ Mosaic.app ───────────────────────────────────┐
│  Rust + Tauri (separate repo, not yet created)  │
│  - UI shell (TypeScript/React in webview)        │
│  - Mode logic (general / vertical / podcast)     │
│  - Multi-destination fanout policy               │
│  - Talks to mosaic-host over IPC                 │
│                                                   │
│  Closed-source-capable (no GPL linkage)          │
└──────────────────┬──────────────────────────────┘
                   │ named pipe (Win) / Unix socket (mac/linux)
                   │ + shared GPU texture handle (later phase)
┌──────────────────▼──────────────────────────────┐
│  mosaic-host (THIS REPO, derived from OBS)       │
│  - Upstream broadcasting engine + plugins        │
│  - IPC server (engine/host/ipc)                  │
│  - IPC method handlers (engine/host/methods)     │
│  - No Qt frontend                                │
│                                                   │
│  GPL v2 (same as the upstream source)            │
└──────────────────────────────────────────────────┘
```

The architecture mirrors how Streamlabs Desktop works (their
`obs-studio-node` is the analog of our `mosaic-host`). The legal point of
running the broadcasting engine in a sidecar process is that the GPL
boundary stops at the IPC pipe. The Mosaic app does not link the
broadcasting engine and is not a derived work.

## Hard constraints

- **Upstream broadcasting code is read-only** — see `CONTRIBUTING.md`.
  Enforced in CI by `.github/workflows/no-core-changes.yaml`.
- **Mosaic must remain closed-source-capable.** No statically-linked GPL
  code in the Mosaic app. No `#include` paths into the broadcasting
  engine from the Mosaic side. IPC only.
- **Upstream merges stay clean.** `master` tracks upstream untouched.
  Quarterly merge into `dev`.
- **Trademark separation.** Mosaic uses its own name, icon, bundle ID,
  and branding. We do not redistribute upstream-branded binaries from
  this repository.

## Phases

| # | Phase                              | Weeks | What lands |
|---|-----------------------------------|-------|------------|
| 0 | Spike & validate                   | 2     | 200-LOC Rust + 400-LOC C++ end-to-end pipeline. Two binaries, one pipe, one RTMP stream. Both platforms. Throwaway. |
| 1 | Strip Qt, real IPC server          | 6     | `mosaic-host` builds without `frontend/`. JSON-RPC IPC server. `ping`/`version`/`shutdown`. |
| 2 | IPC protocol v0.1                  | 8     | Scene/source/output CRUD over IPC. Streaming start/stop. Stats. Settings persistence. CLI test client. |
| 3 | Shared GPU texture preview         | 6     | D3D11 shared handles (Win), IOSurface (mac), DMA-BUF (Linux). Zero-copy 60fps preview. Hardest piece. |
| 4 | Mosaic shell — General mode        | 8     | Tauri app. Source list, scene editor, "Go Live". Functional parity with general streaming for single-destination. |
| 5 | Multi-destination (local fanout)   | 4     | N parallel output instances. Per-destination bitrate. UI for adding destinations. |
| 6 | Vertical mode                      | 4     | 9:16 canvas, source layout templates, vertical default scenes. |
| 7 | Podcast mode P0                    | 12    | WebRTC mesh ≤4 participants. Guests join via web client (no install). Live composite. No per-participant local recording yet. |
| 8 | Podcast mode P1                    | 10    | Per-participant local recording in browser via MediaRecorder. Background upload. Post-session merge. |
| 9 | Polish & ship                      | 8     | Code signing on both platforms. Auto-updater. Installers. Telemetry (opt-in). Marketing site. Public beta. |

**Total realistic timeline: ~80 weeks (1.5 years).** Slack accounted for.

### Decision points where the plan should bend

- **End of phase 3:** if shared GPU texture doesn't work cleanly on
  Windows, fall back to CPU-side preview. Don't let perfection kill phase 3.
- **End of phase 6:** if vertical-first turns out to be the killer mode,
  ship vertical alone as the public beta. Phases 7–8 can wait.
- **End of phase 8:** podcast mode is the riskiest. If it's not working
  at week 60, cut it. Mosaic ships as a 2-mode product. Podcast is v2.

## Phase 0 — feasibility spike

See `engine/spike/PLAN.md` (created at the start of phase 0).

**Goal:** prove the architecture isn't fiction. The smallest possible
end-to-end pipeline.

**Done = green checkmark on every line:**
1. ✅ `mosaic-host-spike` builds and runs on macOS arm64 and Windows x64
2. ✅ `mosaic-spike-client` (Rust) connects to it on both platforms
3. ✅ End-to-end stream works on both (verified by ffplay)
4. ✅ Both processes exit cleanly under normal shutdown
5. ✅ Total new C++ code in `mosaic-host-spike` < 600 LOC
6. ✅ Total Rust code < 300 LOC
7. ✅ `git diff upstream/master..HEAD -- libobs/ plugins/ frontend/cpp deps/` is empty

If 1–4 fail, re-plan (one re-plan budget). If 5–6 are massively over,
that signals the Qt-strip is bigger than expected; phase 1 estimate
doubles.

## Repo layout, target

```
mosaic/
├─ master/             ← tracks upstream broadcasting source, untouched
├─ dev/                ← integration: Mosaic docs, CI, engine entry points
├─ engine/spike/       ← phase 0, throwaway
├─ engine/host/        ← mosaic-host (phases 1+)
│  ├─ ipc/             ← JSON-RPC server
│  ├─ methods/         ← IPC method handlers
│  └─ main.cpp
├─ engine/protocol/    ← IPC schema (shared definitions)
├─ libobs/             ← UPSTREAM, READ-ONLY
├─ plugins/            ← UPSTREAM, READ-ONLY
├─ frontend/           ← UPSTREAM, READ-ONLY (we don't link, don't modify)
└─ ...                 ← upstream as-is
```

The Mosaic desktop app itself lives in a separate repository (created at
the start of phase 4).

## Risk register (top of mind)

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| Shared GPU texture path differs subtly per platform/GPU | High | High | Phase 0 spike on real hardware. CPU-fallback path planned. |
| Stripping Qt cascades through more code than expected | Medium | High | Don't delete `frontend/` immediately — make it not link, then prune. Preserves upstream merge compat. |
| GPL boundary gets challenged | Low | Existential | Follow Streamlabs' pattern exactly: separate process, IPC only, no shared address space. Document the design decision. |
| Trademark dispute with upstream re-emerges | Low (now) | High | Renamed 2026-05-22. Ongoing: strict branding separation in any user-facing artifact. |
| Upstream API changes break mosaic-host | Medium/quarter | Medium | Track `master` against upstream, merge quarterly, fix breakage. |
| Podcast WebRTC stack is its own 6-month project | High | Medium | Use existing libraries (`webrtc-rs`, `mediasoup`). Don't roll our own SFU. |
| Code signing gets stuck on dev enrollment | Medium | Low (delay) | Apply for Apple Developer Program + Windows EV cert before phase 9. |
| First 100 beta users find a thousand bugs | Certain | Medium | Phase 9 includes telemetry + crash reporting from day 1. |

## Project history

- **2026-05-17:** Cloned upstream OBS, started exploring as "OBS Studio Plus"
  fork.
- **2026-05-22 (early):** CI green on all 6 platforms after no-tag survival
  patches.
- **2026-05-22 (mid):** Pivot from "OBS with Qt features" to engine-for-Mosaic
  architecture.
- **2026-05-22 (late):** Renamed from "OBS Studio Plus" to **Mosaic** after
  trademark notice from the OBS Project. All public OBS-branded artifacts
  deleted from CI. Repository renamed to `OgBops/mosaic`.

## Updating this document

When phase boundaries change, when a phase actually ships, when the architecture
gets revised, **update this file in the same commit as the change**. A plan
doc that's out of sync with reality is worse than no plan doc at all.
