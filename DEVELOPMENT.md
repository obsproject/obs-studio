# Developing Aerium

Aerium is currently a development fork, not a supported OBS replacement. The first baseline is **macOS on Apple Silicon**. Windows, Linux, and Intel macOS source remain inherited from OBS, but are not validated Aerium targets yet.

## Build on macOS

Prerequisites: an Apple Silicon Mac, macOS 13 or newer, full Xcode 26 with its command-line tools selected, Git, and CMake 3.28 or newer. The initial local build used Xcode 26.6 and CMake 4.4.3. Install CMake with `brew install cmake` if needed. Accept Xcode's license and finish its first-run setup in Xcode before building.

```sh
git clone --recurse-submodules https://github.com/AeriumChris/Aerium.git
cd Aerium
cmake --preset aerium-macos
cmake --build --preset aerium-macos --parallel 6 -- -quiet
```

For an existing checkout, run `git submodule update --init --recursive` before configuring. CMake downloads OBS's prebuilt dependencies, Qt, and Chromium into the ignored `.deps` directory and verifies the hashes recorded in `CMakePresets.json`. Allow several GB of disk space and a network connection for the first build.

The build produces `build_aerium/frontend/RelWithDebInfo/Aerium.app`. Use this preset rather than the inherited `macos` or `macos-ci` presets for Aerium development.

```sh
build_aerium/frontend/RelWithDebInfo/Aerium.app/Contents/MacOS/Aerium --version
build_aerium/frontend/RelWithDebInfo/Aerium.app/Contents/MacOS/Aerium --portable --only-bundled-plugins
```

Always use `--portable` for development sessions to keep test settings separate from your installed OBS profiles. Do not install development builds over OBS, import personal profiles for automated testing, or enable capture of private material. Use synthetic sources first and grant device permissions only when you intend to test those devices.

The bundle uses `io.github.aeriumchris.aerium` and ad-hoc signing for local development. OBS's updater, What's New panel, and virtual camera are disabled in the Aerium preset. OBS names, icons, internal paths, and browser-helper identifiers are still present elsewhere; this is not a completed product rebrand. Service-account integrations need Aerium-owned credentials before they can be enabled. No signing or service secrets are needed for the baseline build.

## Validation

The `Aerium CI` workflow runs on pushes to `master`, pull requests targeting `master`, and manual dispatch. It validates reStructuredText, lints its workflow, compiles the Apple Silicon app, checks its identifier and ad-hoc signature, and runs `--version` and `--help`. It also checks that the development preset has no Sparkle updater or virtual camera enabled.

CI packages the development bundle on every run to validate delivery. Only successful pushes to `master` and manual CI runs on `master` upload artifacts, retained for 14 days. Pull requests do not upload distributable artifacts. Packaging includes the portable launcher, development guide, license, matching tracked source including recursive submodules, build metadata, and SHA-256 checksums.

CI does not publish public releases, sign with a Developer ID, notarize, or grant recording permissions. Inherited OBS workflows are disabled in the Aerium repository; their source is retained for upstream comparison, not as supported Aerium release automation.

Useful local checks:

```sh
git diff --check
actionlint .github/workflows/aerium-*.yaml
shellcheck build-aux/package-aerium build-aux/launch-aerium.command
codesign --verify --deep --strict build_aerium/frontend/RelWithDebInfo/Aerium.app
```

Install Actionlint with `brew install actionlint`. Documentation CI uses `docutils==0.21.2` in an isolated Python environment. For changed C/C++, CMake, or Swift files, use the inherited format-check tools described in [build-aux/README.md](build-aux/README.md).

A successful compilation or CLI check does **not** verify capture, encoding, audio, or streaming. Before accepting changes to those paths, record the OS, hardware, commit, and results of the relevant runtime tests. Existing tests under `test/` are inherited; this baseline workflow does not claim to run an OBS unit-test suite.

## Development Delivery

Download the `aerium-macos-arm64-<commit>` artifact from a successful [Aerium CI run](https://github.com/AeriumChris/Aerium/actions/workflows/aerium-ci.yaml). Extract the artifact, run `shasum -a 256 --check SHA256SUMS` in that directory, then extract the application ZIP. Keep the launcher and app together in a writable development folder. Use `Launch-Aerium.command`, not the app directly: it keeps profiles under the extracted package's `config` directory and loads only bundled plugins.

These builds use ad-hoc signing, not a trusted Developer ID, and macOS may block them. Do not treat downloaded artifacts as supported releases or disable system-wide Gatekeeper protections. For normal development, building locally is the preferred path.

To package an existing clean checkout locally after building, run `bash build-aux/package-aerium`. Files are written under the ignored `build_aerium/delivery` directory. The source archive includes submodules but not Git history or downloaded third-party dependency packages. When building from an archive, pass `-DOBS_VERSION_OVERRIDE=<version>` to CMake using the version recorded in `build-metadata.json` without the `OBS Studio - ` prefix; dependencies still come from the pinned URLs and hashes in the presets.

### Draft Prereleases

`Aerium Development Delivery` is a manual workflow restricted to protected `master` and the `development-releases` GitHub environment. It accepts a successful Aerium CI run on `master` and a new tag in the form `v0.1.0-dev.1`. It rejects pull-request runs, runs from other repositories or workflows, unsuccessful runs, mismatched metadata, invalid checksums, and existing tags. Expired artifacts require a fresh CI run.

Run it from the [Actions page](https://github.com/AeriumChris/Aerium/actions/workflows/aerium-delivery.yaml) or the GitHub CLI:

```sh
gh workflow run aerium-delivery.yaml --ref master \
	-f run_id=<successful-ci-run-id> -f tag=v0.1.0-dev.1
```

The workflow promotes the already-tested artifacts, without rebuilding or executing downloaded code. It creates an **unpublished draft prerelease**, attaches the application, source, metadata, and checksums, and never replaces existing releases or marks them latest. No additional signing, service, or cloud credentials are required; GitHub's scoped workflow token is used.

Only the delivery job has repository write permission. The environment is restricted to protected branches; it does not require an unavailable second maintainer's approval. Public publication remains a separate human decision after the release gate below is satisfied. Creating a draft does not mean its contents are approved for public distribution.

### Manual Recording Smoke Test

1. Start the development app in portable mode with only bundled plugins.
2. Skip account connections and the automatic configuration wizard. Disable desktop and microphone audio devices for the synthetic test.
3. Create a disposable profile and scene collection, add a color source and a text source, and choose a temporary recording folder.
4. Record at least ten seconds locally, stop, and play the result. Check dimensions, duration, visible sources, and successful finalization.
5. Close and reopen the app in portable mode. Confirm the test scene persists and the regular OBS profile was not changed.
6. Test camera, screen capture, audio, and streaming separately with permission, synthetic or authorized content, and test accounts. Record anything not exercised.

## Working with Upstream

`origin` is `AeriumChris/Aerium`; `upstream` is `obsproject/obs-studio`. In a fresh clone, add the upstream remote once:

```sh
git remote add upstream https://github.com/obsproject/obs-studio.git
git config remote.pushDefault origin
```

Use topic branches and pull requests for changes. For an upstream refresh:

```sh
git fetch upstream --tags
git switch -c maintenance/obs-sync
git merge upstream/master
git submodule update --init --recursive
```

Review conflicts, licenses, dependency changes, and Aerium's disabled update/release paths, then rerun CI and relevant runtime tests before merging. Do not force-push `master` or blindly reset it to upstream. Keep original copyright notices and follow [COPYING](COPYING) and dependency licenses. AI-assisted contributions to Aerium must follow [CONTRIBUTING.md](CONTRIBUTING.md); they are not automatically eligible for upstream submission.

## First Milestone

The initial milestone is a reproducible macOS development baseline, followed by a proposal for one simplified recording workflow. Completion requires a clean CI build, recorded manual smoke-test results, and a small, reviewed interface proposal before substantial feature work. Windows/Linux CI and runtime validation are separate follow-up work, not implied by the macOS baseline.

## Public Release Gate

Do not publish a public Aerium release until all applicable items are complete:

- Finish the independent product identity: UI, icons, platform bundle/package IDs, browser helpers, profiles, install locations, and coexistence with OBS.
- Keep official OBS update endpoints disabled; establish an Aerium update feed and its own signing keys if automatic updates are introduced.
- Obtain platform signing certificates, notarization credentials, and service credentials through their legitimate providers. Store secrets in protected GitHub environments, never in source or AI prompts.
- Audit inherited OAuth, crash reporting, log upload, support, donation, and service URLs before distributing them under Aerium's name.
- Give any re-enabled virtual camera its own identifiers and properly signed installation path.
- Build and test each advertised OS/architecture, including capture permissions, recording playback, audio/video sync, streaming, upgrades, and uninstall behavior.
- Establish release versions, checksums, changelogs, matching source availability, third-party notices, security maintenance expectations, and an explicit publishing workflow.

These are release requirements, not claims that they are already implemented. There are currently no supported public Aerium releases.