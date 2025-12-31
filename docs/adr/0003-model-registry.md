# ADR 0003 — Model Registry & Inference Service

Status: Proposed
Date: 2025-12-31

Context
-------
AI features will be a first-class part of OBS-Next. Currently there is no standardized mechanism for plugins or core subsystems to discover, load, and use AI models or backends (ONNXRT, TensorRT, OpenVINO). Without a standard, features will be inconsistent and hard to reason about.

Decision
--------
Create a pluggable Model Registry and Inference Service:

- A central service (in-process or separate) that manages model metadata, capabilities, and backend bindings.
- Capabilities-based model descriptions (input shapes, quantization, preferred backends, latency/throughput hints) and model versioning.
- An async inference API (RPC or in-process async calls) that supports batching, priority queuing, and event-gating (StreamMind-inspired), with result caching and fallbacks.

Consequences
------------
- Pros:
  - Standardizes model use across plugins and core features (background removal, scene understanding).
  - Allows dynamic backend switching based on available hardware and policy.
- Cons:
  - Initial complexity to design capability negotiation and cache coherency.

Alternatives Considered
-----------------------
1. Let plugins manage models independently: simpler but duplicates effort and prevents cross-plugin sharing.
2. Hard-code a single backend (ONNXRT): faster to ship but limits optimization for specific hardware.

Next Steps
----------
1. Define a minimal model description schema and capability set.  
2. Prototype an ONNX Runtime worker that accepts model load requests and provides an async predict API.  
3. Add tests for event-gating and priority-based scheduling.

Authors
-------
GitHub Copilot
