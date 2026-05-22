# Plus — Project Plan

This document is the north star for `obs-studio-plus`. When in doubt about
scope, refer back here. When this document is wrong, update it before
writing code that contradicts it.

## What we're building

**Plus** — a cross-platform desktop streaming app, owned end-to-end, that
covers three streaming modes: general (OBS-like), vertical-first
(TikTok/Shorts/Reels), and podcast/interview-style.

## Architecture

Plus is two binaries:

```
┌─ Plus.app ─────────────────────────────────────┐
│  Rust + Tauri (this repo: not yet created)      │
│  - UI shell (TypeScript/React in webview)        │
│  - Mode logic (general / vertical / podcast)     │
│  - Multi-destination fanout policy               │
│  - Talks to plus-host over IPC                   │
│                                                   │
│  Closed-source-capable (no GPL linkage)          │
└──────────────────┬──────────────────────────────┘
                   │ named pipe (Win) / Unix socket (mac/linux)
                   │ + shared GPU texture handle (later phase)
┌──────────────────▼──────────────────────────────┐
│  plus-host (THIS REPO, the OBS fork)             │
│  - libobs + every upstream plugin                │
│  - IPC server (engine/host/ipc)                  │
│  - IPC method handlers (engine/host/methods)     │
│  - No Qt frontend                                │
│                                                   │
│  GPL v2 (same as upstream OBS Studio)            │
└──────────────────────────────────────────────────┘
```

The architecture mirrors how Streamlabs Desktop works (their
`obs-studio-node` is the analog of our `plus-host`). The legal point of
running libobs in a sidecar process is that the GPL boundary stops at
the IPC pipe. The Plus app does not link libobs and is not a derived
work.

## Hard constraints

- **OBS core is read-only** — see `CONTRIBUTING.md`. Enforced in CI by
  `.github/workflows/no-core-changes.yaml`.
- **Plus must remain closed-source-capable.** No statically-linked GPL
  code. No `#include` paths into libobs from the Plus side. IPC only.
- **Upstream merges stay clean.** `master` tracks `obsproject/obs-studio`
  untouched. Quarterly merge into `dev`.

## Phases

| # | Phase                              | Weeks | What lands |
|---|-----------------------------------|-------|------------|
| 0 | Spike & validate                   | 2     | 200-LOC Rust + 400-LOC C++ end-to-end pipeline. Two binaries, one pipe, one RTMP stream. Both platforms. Throwaway. |
| 1 | Strip Qt, real IPC server          | 6     | `plus-host` builds without `frontend/`. JSON-RPC IPC server. `ping`/`version`/`shutdown`. |
| 2 | IPC protocol v0.1                  | 8     | Scene/source/output CRUD over IPC. Streaming start/stop. Stats. Settings persistence. CLI test client. |
| 3 | Shared GPU texture preview         | 6     | D3D11 shared handles (Win), IOSurface (mac), DMA-BUF (Linux). Zero-copy 60fps preview. Hardest piece. |
| 4 | Plus shell — General mode          | 8     | Tauri app. Source list, scene editor, "Go Live". Functional parity with vanilla OBS for single-destination. |
| 5 | Multi-destination (local fanout)   | 4     | N parallel `obs_output_t` instances. Per-destination bitrate. UI for adding destinations. |
| 6 | Vertical mode                      | 4     | 9:16 canvas, source layout templates, vertical default scenes. |
| 7 | Podcast mode P0                    | 12    | WebRTC mesh ≤4 participants. Guests join via web client (no install). Live composite. No per-participant local recording yet. |
| 8 | Podcast mode P1                    | 10    | Per-participant local recording in browser via MediaRecorder. Background upload. Post-session merge. The Riverside-killer piece. |
| 9 | Polish & ship                      | 8     | Code signing on both platforms. Auto-updater. Installers. Telemetry (opt-in). Marketing site. Public beta. |

**Total realistic timeline: ~80 weeks (1.5 years).** Slack accounted for.

### Decision points where the plan should bend

- **End of phase 3:** if shared GPU texture doesn't work cleanly on
  Windows, fall back to CPU-side preview. Don't let perfection kill phase 3.
- **End of phase 6:** if vertical-first turns out to be the killer mode,
  ship vertical alone as the public beta. Phases 7–8 can wait.
- **End of phase 8:** podcast mode is the riskiest. If it's not working
  at week 60, cut it. Plus ships as a 2-mode product. Podcast is v2.

## Phase 0 — feasibility spike

See `engine/spike/PLAN.md` (created at the start of phase 0).

**Goal:** prove the architecture isn't fiction. The smallest possible
end-to-end pipeline.

**Done = green checkmark on every line:**
1. ✅ `plus-host-spike` builds and runs on macOS arm64 and Windows x64
2. ✅ `plus-spike-client` (Rust) connects to it on both platforms
3. ✅ End-to-end stream works on both (verified by ffplay)
4. ✅ Both processes exit cleanly under normal shutdown
5. ✅ Total new C++ code in `plus-host-spike` < 600 LOC
6. ✅ Total Rust code < 300 LOC
7. ✅ `git diff upstream/master..HEAD -- libobs/ plugins/ frontend/cpp deps/` is empty

If 1–4 fail, re-plan (one re-plan budget). If 5–6 are massively over,
that signals the Qt-strip is bigger than expected; phase 1 estimate
doubles.

## Repo layout, target

```
obs-studio-plus/
├─ master/             ← tracks upstream OBS, untouched
├─ dev/                ← integration: branding, CI, engine entry points
├─ engine/spike/       ← phase 0, throwaway
├─ engine/host/        ← plus-host (phases 1+)
│  ├─ ipc/             ← JSON-RPC server
│  ├─ methods/         ← IPC method handlers
│  └─ main.cpp
├─ engine/protocol/    ← IPC schema (shared definitions)
├─ libobs/             ← UPSTREAM, READ-ONLY
├─ plugins/            ← UPSTREAM, READ-ONLY
├─ frontend/           ← UPSTREAM, READ-ONLY (we don't link, don't modify)
└─ ...                 ← upstream as-is
```

The Plus app itself lives in a separate repository (created at the start
of phase 4).

## Risk register (top of mind)

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| Shared GPU texture path differs subtly per platform/GPU | High | High | Phase 0 spike on real hardware. CPU-fallback path planned. |
| Stripping Qt cascades through more code than expected | Medium | High | Don't delete `frontend/` immediately — make it not link, then prune. Preserves upstream merge compat. |
| GPL boundary gets challenged | Low | Existential | Follow Streamlabs' pattern exactly: separate process, IPC only, no shared address space. Document the design decision. |
| Upstream OBS API changes break plus-host | Medium/quarter | Medium | Track `master` against upstream, merge quarterly, fix breakage. |
| Podcast WebRTC stack is its own 6-month project | High | Medium | Use existing libraries (`webrtc-rs`, `mediasoup`). Don't roll our own SFU. |
| Code signing gets stuck on dev enrollment | Medium | Low (delay) | Apply for Apple Developer Program + Windows EV cert before phase 9. |
| First 100 beta users find a thousand bugs | Certain | Medium | Phase 9 includes telemetry + crash reporting from day 1. |

## What this plan replaces

The previous OBS-Plus roadmap (multi-destination dock, AI captions plugin,
Qt-frontend feature work) is **abandoned**. The work wasn't wasted — concepts
move into the IPC protocol — but the Qt path is no longer the route to those
features. See git history on the deleted branches:
`feature/multi-output-foundation`, `feature/multi-destination-dock`,
`feature/ai-captions-plugin`.

## Updating this document

When phase boundaries change, when a phase actually ships, when the architecture
gets revised, **update this file in the same commit as the change**. A plan
doc that's out of sync with reality is worse than no plan doc at all.
