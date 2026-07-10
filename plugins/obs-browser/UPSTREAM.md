# Vendored `obs-browser`

This directory is intentionally vendored by `Loggableim/obs-studio-webgpu`.

- Upstream repository: <https://github.com/obsproject/obs-browser>
- Imported commit: `3f0a2cdf378939ebe3c6f9ab36d4ea100c25aac2`
- Imported version: `2.26.9`

When syncing upstream, start from the imported commit, preserve all files in
this directory that are specific to the WebGPU fork, and run the Windows
WebGPU regression suite before accepting the update. Do not turn this
directory back into a Git submodule: the fork needs the browser source and
its CEF compatibility changes to land atomically with the OBS changes.
