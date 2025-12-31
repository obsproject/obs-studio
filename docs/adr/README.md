# Architecture Decision Records (ADRs)

This directory contains Architecture Decision Records (ADRs) documenting major design choices for OBS-Next.

Current ADRs
- 0001-plugin-sandbox.md — Plugin Sandbox Model (Process-isolated plugins, manifest, signing)
- 0002-gpu-zero-copy.md — GPU First / Zero-Copy Buffer Model
- 0003-model-registry.md — Model Registry & Inference Service

How to write ADRs
1. Copy `0000-example.md` (TBD) and follow the template.
2. Submit a PR for review and link the ADR in project proposals or RFCs.

Module ADRs and RFCs: For extraction work, add a short RFC under `docs/rfc/` and reference it from the ADRs directory so reviewers can see the actionable plan.
