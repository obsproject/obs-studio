# RFC 0001 — `obs-source.c` / `obs-scene.c` Decomposition

Status: Draft
Date: 2025-12-31

Summary
-------
Propose a minimal, low-risk decomposition of `libobs/obs-source.c` and `libobs/obs-scene.c` into smaller modules that can be extracted incrementally and tested independently.

Approach
--------
1. Call-graph analysis: Use `scripts/generate-callgraph.sh` to cluster functions into cohesive groups (frame I/O, texture management, filter chain, audio buffer handling, scene tree ops).
2. Define public APIs for the first extraction: `source_api.h` (frame ingestion / query), `scene_api.h` (scene manipulation / lookup) and test harnesses.
3. Extract small, self-contained helper functions first with unit tests. Ensure that every extraction is accompanied by a compatibility shim in `libobs/` for a smooth transition.

Acceptance Criteria
-------------------
- Build keeps passing with extracted modules linked in.
- Unit tests for extracted modules are added and pass in CI.
- No change in visible behavior for the canonical integration tests (pixel output, audio sync).

Risks & Mitigations
-------------------
- Risk: Hidden coupling discovered during extraction. Mitigation: aggressive call-graph analysis and short refactor cycles.

Next Steps
----------
- Run the callgraph generator and review the clusters.
- Open small PRs to extract the first helper under `modules/sources/` (e.g., frame size validators, planar format helpers).
