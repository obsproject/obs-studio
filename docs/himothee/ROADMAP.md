# Himothee Studio Roadmap

## Foundation

### v0.1 — OBS 32.2.2 baseline

- Pin development to OBS Studio 32.2.2.
- Establish `himothee-dev`.
- Document upstream strategy.
- Confirm a clean Windows build before functional changes.

### v0.2 — Himothee identity

- Product name and window titles.
- Executable/application identity where practical.
- Himothee icons and assets.
- Independent configuration/data directory so it does not overwrite a normal OBS installation.
- About dialog and version information.

## Native multistream

### v0.3 — Multi-output core

- Destination model.
- Multiple simultaneous streaming outputs.
- Lifecycle management for start/stop/reconnect.
- Per-destination state and error reporting.
- Preserve standard OBS single-stream behavior.

### v0.4 — Multistream Manager

- Add/remove/enable destinations.
- Twitch, YouTube, Kick, and Custom RTMP presets.
- Per-destination server and stream-key configuration.
- Start All / Stop All.
- Individual destination start/stop controls.

### v0.5 — Shared encoder mode

- Encode video once.
- Feed compatible encoded packets to multiple destinations.
- Minimise extra GPU usage.
- Clearly report incompatible destination settings.

### v0.6 — Independent encoder mode

- Optional encoder per destination.
- Per-destination resolution/bitrate/encoder settings.
- GPU/CPU load warnings.

### v0.7 — Reliability and telemetry

- Reconnect strategy.
- Destination health.
- Bitrate and dropped-frame reporting.
- Clear partial-failure behaviour when one service disconnects.
- Logging suitable for troubleshooting.

### v0.8 — Audio routing

- Per-destination audio track selection.
- Shared and independent audio encoders as appropriate.
- Advanced routing UI.

## Production tools

After the multistream foundation is stable:

- Built-in alerts.
- Media triggers.
- Macros.
- Counters and timers.
- Browser overlay server.
- Stream Deck / external control.
- Additional production automation.

## Release principle

A stage is complete only when the previous OBS functionality still works and the new feature has a reproducible test path.
