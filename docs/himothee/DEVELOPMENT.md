# Himothee Studio Development

## Baseline

Himothee Studio development is based on OBS Studio **32.2.2**, commit:

`ba2f32bdf791005443988a4955e963663e16b1ed`

The goal is to keep product-specific changes isolated from the upstream-sync branch so that future OBS updates can be evaluated cleanly.

## Branch strategy

- `master` — upstream-facing fork branch. Avoid Himothee-specific feature work here.
- `himothee-dev` — integration branch for Himothee Studio.
- `feature/branding` — application identity, names, assets, config paths, and installer branding.
- `feature/multistream-core` — native multi-output streaming engine.
- Additional feature branches should be created from `himothee-dev` and merged back through pull requests.

## Development rules

1. Keep each change focused.
2. Do not remove OBS attribution or licensing notices.
3. Avoid large unrelated refactors while multistream support is being established.
4. Keep the original single-stream path working while adding multi-output support.
5. Prefer extending existing OBS abstractions over duplicating encoder, service, or output logic.
6. Add logging for destination lifecycle events and failures.
7. Never commit stream keys, OAuth tokens, passwords, certificates, or other secrets.

## Build baseline

Until Himothee-specific automation is introduced, use the build system inherited from OBS Studio 32.2.2.

For Windows development, follow the build instructions shipped with the checked-out OBS source and keep the dependency/toolchain versions compatible with this baseline.

## Pull request expectations

Every Himothee feature PR should explain:

- What problem it solves.
- Which subsystem it changes.
- How it was tested.
- Whether single-stream OBS behavior changed.
- Any migration or configuration impact.
- Known limitations.

## Versioning

Himothee development versions begin at `0.x` while the fork is experimental. The underlying OBS baseline should always be documented separately from the Himothee Studio product version.
