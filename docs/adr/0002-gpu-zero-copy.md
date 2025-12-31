# ADR 0002 — GPU First / Zero-Copy Buffer Model

Status: Proposed
Date: 2025-12-31

Context
-------
The legacy pipeline performs frequent CPU↔GPU data transfers and relies on platform-specific texture handling. This causes performance bottlenecks, higher latency, and complicates high-throughput AI inference on frames.

Decision
--------
Define and standardize a GPU-first buffer model across the core pipeline:

- Use cross-platform GPU abstraction (primary implementation via `wgpu` / Vulkan / DX12) with explicit external memory handle support for zero-copy sharing of frames between processes and across APIs.
- Establish a clear buffer ownership and lifetime model (producer/consumer counts, fence/synchronization primitives) and expose it over FFI to enable language bindings (Rust/C++/WASM). 
- Provide shader asset manifest and pipeline descriptors; support hot-reload of shaders and precompiled SPIR-V/metal/dx blobs.

Consequences
------------
- Pros:
  - Substantial reduction in CPU overhead and per-frame latency.
  - Enables direct-to-GPU AI inference and efficient composition.
- Cons:
  - Requires careful cross-platform testing (external memory semantics differ between drivers).
  - Initial complexity in FFI synchronization and lifetime rules.

Alternatives Considered
-----------------------
1. Incremental optimization of current CPU copy paths: low risk but limits performance ceiling.  
2. Only support zero-copy on selected platforms: reduces coverage and increases fragmentation.

Next Steps
----------
1. Create an ADR detailing external-handle semantics per platform and a list of supported handle types (VkProc, DXGI, etc.).
2. Implement a minimal Rust `wgpu` scaffold with a FFI that initializes a device and allocates an external buffer for a test ping.
3. Add GPU headless tests to CI to assert correct synchronization under stress.

Authors
-------
GitHub Copilot
