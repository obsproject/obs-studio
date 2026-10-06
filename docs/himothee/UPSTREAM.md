# OBS Upstream Strategy

Himothee Studio is derived from OBS Studio and should keep a clear relationship with the upstream project.

## Current baseline

- Upstream repository: `obsproject/obs-studio`
- Baseline release: `32.2.2`
- Baseline commit: `ba2f32bdf791005443988a4955e963663e16b1ed`

## Branch policy

`master` is reserved as the upstream-facing branch in this fork.

`himothee-dev` is the Himothee integration branch and begins from the exact 32.2.2 baseline.

Do not routinely press GitHub's **Sync fork** and then merge the result straight into `himothee-dev`. New upstream releases should be reviewed as explicit upgrade work.

## Upgrading OBS later

When adopting a newer OBS release:

1. Record the target OBS tag and commit.
2. Create an upgrade branch from `himothee-dev`.
3. Compare Himothee-specific changes against upstream changes.
4. Resolve conflicts subsystem by subsystem.
5. Build and test capture, audio, recording, streaming, replay buffer, browser sources, plugins, and multistreaming.
6. Update the documented OBS baseline only after the upgrade passes testing.

## Attribution and licensing

Retain upstream copyright notices, license files, and source-file notices. Himothee branding must not imply that the fork is an official OBS Project release.
