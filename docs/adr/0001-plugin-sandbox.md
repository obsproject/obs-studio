# ADR 0001 — Plugin Sandbox Model

Status: Proposed
Date: 2025-12-31

Context
-------
The current OBS plugin model loads third-party code into the host process via dlopen/LoadLibrary and requires exported symbols (e.g., `obs_module_load`). This exposes the host to stability and security risks: a faulty plugin can crash the entire application, and plugins run with full host privileges.

Decision
--------
Adopt a microservice-style plugin sandbox model as the long-term default:

- Plugin processes run in separate OS processes (or optionally WASI sandboxes) and communicate with the host through a well-defined IPC boundary. Use shared memory / external GPU memory handles for zero-copy frame passing when supported.
- Provide a compatibility shim that allows existing legacy plugin binaries to run in-process for backward compatibility while encouraging migration to the sandboxed model.
- Define a Plugin Manifest (JSON) that declares capabilities, API version, signing metadata, and optional sandboxing requirements.

Consequences
------------
- Pros:
  - Crashes in plugin processes won't bring down the host.
  - Enables stricter permission models and capability-based security.
  - Makes hot-reload, resource metering, and RBAC easier to implement.
- Cons:
  - Initial engineering cost to design IPC/compat shims and transition the ecosystem.
  - Potential latency overhead for IPC if not using zero-copy handles.

Alternatives Considered
-----------------------
1. Keep the existing dlopen model and add runtime verification/sandboxing: minimally invasive but insufficient for fault isolation.
2. WebAssembly (WASI) first approach: very portable and secure, but ecosystem (native GPU access, driver interop) requires more glue for high-performance video plugins.

Next Steps
----------
1. Draft the Plugin Manifest schema and signing spec.
2. Prototype a minimal plugin-worker binary that responds to an IPC `ping` and runs a provided plugin stub in-process within its own process.
3. Document compatibility shim requirements and migration path for top 10 most-used plugins.

Authors
-------
GitHub Copilot
