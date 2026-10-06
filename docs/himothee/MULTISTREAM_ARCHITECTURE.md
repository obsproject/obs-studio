# Native Multistream Architecture

## Objective

Add first-class multi-destination streaming to the OBS frontend used by Himothee Studio without replacing the proven OBS capture, render, encoder, and output subsystems.

## Destination model

Each streaming destination should own configuration and runtime state, including:

- Stable destination ID.
- Display name.
- Enabled state.
- Service type.
- Server URL / ingest selection.
- Stream key or service credential reference.
- Encoder mode.
- Video bitrate and related output settings.
- Audio track/encoder selection.
- Reconnect policy.
- Runtime status and last error.

Secrets must be stored using the safest mechanism available in the existing OBS platform/service architecture and must never be logged.

## Mode A — Shared encoder

Preferred default when destinations accept compatible encoded video/audio.

```text
OBS render
   |
   v
shared video encoder
   |
   +--> destination A output
   +--> destination B output
   +--> destination C output
```

Benefits:

- Lowest additional GPU encode load.
- Identical video quality across destinations.
- Simple start/stop behaviour.

Constraint:

Destinations sharing an encoder must use mutually compatible codec and encode settings.

## Mode B — Independent encoders

Advanced mode for destinations requiring different output profiles.

```text
OBS render
   |
   +--> encoder A --> destination A
   +--> encoder B --> destination B
   +--> encoder C --> destination C
```

This permits different bitrate and encoding profiles but increases GPU/CPU load.

## Lifecycle

The manager should support:

- Start enabled destinations.
- Stop all destinations.
- Start/stop one destination without interrupting the others.
- Independent reconnect state.
- Partial success: one failed destination must not automatically kill healthy destinations unless the user selects that policy.

## UI direction

Expose destinations through a Multistream Manager rather than overloading the existing single Service page.

Each destination row should show at minimum:

- Name/service.
- Enabled state.
- Connection state.
- Current bitrate.
- Dropped frames/errors.
- Start/stop action.

## Compatibility principle

Initial multistream development should be additive. Existing OBS recording, replay buffer, virtual camera, and standard single-stream workflows should continue to function.
