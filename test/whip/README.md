# WHIP fork review — 2026-10-03

## Reproducible compatibility builds

The `WHIP builds` GitHub workflow builds the current fork, the latest stable OBS
release, and the latest applicable prerelease on Windows x64, macOS arm64/x86_64,
and Linux x86_64. It runs on code pushes, WHIP-related pull requests, and manual
dispatch. In `steveseguin/obs-studio`, this replaces the inherited Push workflow's
build job, which does not apply the patched ICE dependency. Formatting, service,
and compatibility checks remain enabled.
Stable/preview builds apply the explicit WHIP and encoder-policy file list in
`prepare-whip-source.py` relative to upstream `cffa83ba5`; an incompatible patch
fails the job instead of silently omitting fixes.

[GitHub run 37140356507](https://github.com/steveseguin/obs-studio/actions/runs/37140356507)
passed all 12 builds and their ICE regressions on 2026-10-03: fork, stable 32.2.2,
and preview 33.0.0-beta6 across all four platform/architecture combinations.
The subsequent [master build 37145895603](https://github.com/steveseguin/obs-studio/actions/runs/37145895603)
also passed all 12 jobs at `bd9377113`. Its verified stable and preview packages
are published as permanent release assets:

- [OBS 32.2.2 with WHIP and TURN fixes (Latest)](https://github.com/steveseguin/obs-studio/releases/tag/v32.2.2-whip-relay).
- [OBS 33.0.0-beta6 with WHIP and TURN fixes (Prerelease)](https://github.com/steveseguin/obs-studio/releases/tag/v33.0.0-beta6-whip-relay).

Each release includes Windows x64, macOS arm64/x86_64, and Linux x86_64 archives,
SHA-256 checksums, and build provenance. Public downloads and uploaded hashes
were verified. Actions artifacts remain available for 14 days.

Each build explicitly enables WebRTC, rebuilds pinned libdatachannel/libjuice with
the checked-in patch, and runs the STUN integrity and ICE role-conflict tests.
The patch also fixes the controlled-role conflict comparison to use the remote
ICE-CONTROLLED tie-breaker. The added test failed before this correction.
Build manifests record the source revisions and dependency patch SHA-256.

The workflow produces **unofficial test builds** as downloadable artifacts, with
file permissions preserved inside ZIP/tar.gz archives. Release publication is a
separate step; the workflow does not publish releases or use OBS Project signing
credentials. AJA and scripting are disabled; macOS virtual camera is disabled.
macOS packages require 13.0 or later. Linux artifacts target Ubuntu 26.04 and
require its runtime libraries. Building successfully does not certify live TURN
operation.

**Mac signing and WHIP encoder correction completed (2026-10-03):** both releases
contain Developer ID-signed, Apple-notarized Mac packages for arm64 and x86_64.
The final Mac binaries use WHIP revision `05ebf73765b44a5abb7a8365cace7f154154ef48` from
[signed CI run 37167719965](https://github.com/steveseguin/obs-studio/actions/runs/37167719965); all 12 cross-platform builds passed.
Stable was prepared from upstream OBS 32.2.2 (`ba2f32bdf791005443988a4955e963663e16b1ed`),
and preview from 33.0.0-beta6 (`cffa83ba552f1ef6a0a05851c3aa07b3811d7e58`).
The pinned libdatachannel/libjuice revisions and dependency patch hash are unchanged.

The original Mac binaries were first signed without changing executable payloads.
Live tests then exposed the same Mbed TLS/Chrome DTLS failure previously fixed on
Windows. An earlier rebuild extended that exact-host `whip.vdo.ninja` active-role
workaround to macOS. Other WHIP hosts retain actpass; Linux retains its OpenSSL
behavior. No VDO.Ninja application source was changed.

The apps are signed with Steve Seguin's Developer ID Application identity (team
`H3CKR5XB3J`), secure timestamps, hardened runtime, and preserved OBS/CEF
entitlements. Apple accepted all four submissions with no issues. Strict nested
signature checks, stapled-ticket validation, and Gatekeeper assessment passed on
the extracted final distribution archives. Future signed CI builds require these
checks before emitting a notarized artifact; routine pushes cannot cancel a
signed release run.

| OBS version | Architecture | Accepted Apple submission |
| --- | --- | --- |
| 32.2.2 | arm64 | `054f7444-06ca-4f65-8085-ca25efc8113d` |
| 32.2.2 | x86_64 | `32596588-7dbb-4a33-a989-0b530f726179` |
| 33.0.0-beta6 | arm64 | `53eb13b3-6021-4ad7-8f0b-13301a41ee73` |
| 33.0.0-beta6 | x86_64 | `25068216-35ab-485c-af49-4bd287a33772` |

`BUILD-PROVENANCE.json` retains the original CI package in
`original_build_package`, prior signing records in `superseded_packages`, and the
new Mac CI artifact/manifest in `rebuild` and `manifest`. Current signing and Apple
results are separate under `signing`. Windows/Linux archives, checksums, and
provenance entries remain unchanged at WHIP revision `bd9377113d4445be6ca40e71e674511179daba2d`.
Stable remains Latest; preview remains Prerelease. Release tags still identify the
original tooling revision; use each archive's manifest for its exact sources.

The local Keychain profile is `OBS-WHIP`; the five signing/notarization repository
secrets are configured. No credentials are stored in this repository. Final CI
receipts, Apple logs, downloaded artifacts, runtime results, and public-download
verification are retained under ignored `build_whip_final/`. The intermediate
candidate investigation is retained under `build_whip_vt/`; the earlier DTLS
validation remains under `build_whip_runtime/`, and initial conversion evidence
under `build_whip_signing/`.

### Final Mac runtime checks — revision `05ebf7376`, 2026-10-03

The final Mac archives were launched on macOS 26.4.1 / Apple M1: arm64 natively and
x86_64 under Rosetta. All **54 planned media scenarios passed**, including fresh-session
repeats, and **4/4 software-HEVC rejection checks passed**. Across initial attempts and
repeats, 58/60 media attempts passed. Chrome 154.0.8037.93 and Firefox 155 received
synthetic 720p30 video and Opus audio through the production WHIP endpoint. Passes
required at least 24 decoded fps, advancing audio, a playing video element, the expected
selected route, and a successful stream stop. Final initial samples were 10 seconds per
route; fresh-session retries were 15 seconds.

Two final relay attempts in the reused Firefox profile failed during ICE gathering, with
no usable local candidates. The complete three-route matrix was then repeated twice in
fresh Firefox profiles. Both fresh repetitions passed; the earlier failures remain in
the results. This observation does not establish the underlying cause.

All four apps were tested with hardware VT H.264 in Chrome and Firefox and x264 in
Chrome, each over default/direct, TURN/UDP (`&relay`), and TURN/TLS (`&relay&tcp`).
Native builds additionally tested software VT H.264, hardware VT HEVC, and AOM AV1
across the same three routes. **16 recorded samples contained 4,071 I/P frames and zero
B-frames**, including x264 configured with a deliberate `bframes=8` override. Advanced
encoder tests disabled optional service recommendations. Apple software HEVC is
deliberately rejected for WHIP because it emitted B-slices despite frame reordering
being disabled; use hardware HEVC or H.264.

The preceding 98-attempt investigation covered eight URL-flag modes on all four apps,
explicit TURN UDP 3478/TLS 443 URLs, explicit WHIP HTTPS port 443, and one-minute TLS
observations on both native builds. Its failures are retained: an explicit UDP
allocation returned TURN 508 (both fresh retries passed), Firefox UDP failed twice
before a fresh retry passed, and two software-H.264 runs fell below the frame-rate
threshold while other builds were using the host (both cases passed twice on retry).
Software HEVC also failed the 720p30 threshold and emitted B-slices; that candidate was
not published. The final rebuild adds the rejection guard.

Default and `&turn=false` connected directly on this network. `&tcp` restricts TURN
choices but does not force a relay; `&tcp=443` has the same behavior. `&tcprelay` and
`&port=443` were not recognized as routing/port selectors. Explicit TURN URLs provide
the tested port selection. TURN/TLS here describes the browser-to-TURN leg; OBS still
uses UDP.

Video packetizer tests enforce the shared 1,200-byte fragment limit for H.264, HEVC and
AV1, with a tested maximum of 1,220-byte RTP / 1,288 bytes including conservative IPv6,
UDP, SRTP and TURN ChannelData allowances. Single-frame stereo Opus peaked at a
1,356-byte packet with the same allowance. These are packetizer tests and header
budgets, not measurements through a VPN.

Physical Intel hardware, Windows GPU encoders, Safari/mobile receivers, real VPN paths,
and a UDP-blocked OBS network were not tested in this round. The supplied BrowserStack
credential path was absent, so no BrowserStack tests ran. Browser-source rendering
remains unresolved on this host. Published Windows/Linux packages are unchanged and do
not contain the new encoder-policy guards.

The [machine-readable results](../../docs/whip/macos-validation-2026-10-03.json) include
failed attempts and candidate/release revisions.

### Earlier DTLS runtime checks — revision `786910b50`, 2026-10-03

Both versions launched on macOS 26.4.1 / Apple M1: arm64 natively and x86_64 under
Rosetta. **All 24 direct production WHIP cases passed** with automated Chrome
154.0.8037.93 and Firefox 155.0 receivers (Mozilla-listed OpenH264 2.6.0 installed
and checksum-verified). Each app/browser combination passed default routing,
`&relay` (selected TURN/UDP), and `&relay&tcp` (selected TURN/TLS).

Tests generated 720p30 video and tone audio, encoded with x264/Opus. Each pass
required advancing decoded video frames, increasing audio bytes, a playing video
element, and the expected selected relay transport. Final tests used the production
endpoint directly, without the diagnostic role-rewriting proxy used to isolate
the original failure. Temporary OBS profiles were removed from the active config
path after every run and the user's original configuration restored.

Browser-source rendering remains unresolved on this host. Local-file probes
returned black frames on signed apps and the original ad-hoc stable-arm64 baseline. HTTP probes
also returned black frames on rebuilt apps, although one earlier signed
stable-arm64 HTTP probe rendered an animated page successfully.
No general CEF rendering pass is claimed. Physical Intel hardware, hardware
capture, Linux live publishers, extended endurance, and an OBS network blocking
outbound UDP remain untested. `&relay&tcp` uses TLS on the browser-to-TURN leg;
OBS still uses UDP.

### WHIP encoder and packet policy

WHIP applies its encoder requirements at output startup, including when Advanced
Output's optional service recommendations are disabled. It forces the shared
`bf` setting to zero and VideoToolbox's boolean `bframes` setting to false.
Custom x264, NVENC and AMD AMF options cannot re-enable B-frames; NVENC UHQ is
changed to HQ because UHQ requires B-frames. Other tuning choices are preserved.
Apple's software HEVC encoder is rejected for WHIP: local bitstream tests found
B-slices even with frame reordering disabled, including separate real-time and
low-delay probes. Use hardware HEVC or H.264 instead. This restriction applies
only to WHIP; other outputs retain their existing encoder behavior.
A shared encoder already running without these requirements must be stopped
before WHIP starts; changing settings cannot remove frames already queued by an
active recording encoder.

All video encoders use the same H.264/HEVC/AV1 packetizers with a **1,200-byte
fragment limit**. This limits RTP payload fragments, not encoded video frame
size. `test/whip/packetization` tests the production limit against the exact
rebuilt libdatachannel, including MID/RID extensions, boundary sizes and large
frames up to 1 MiB. The dependency helper runs it on every build alongside the
ICE regressions. Tested video RTP packets peak at 1,220 bytes, or 1,288 bytes
allowing for IPv6, UDP, a 16-byte SRTP tag and TURN ChannelData. The maximum single
Opus frame plus TOC is tested separately: 1,288-byte RTP / 1,356-byte packet with
the same allowance. The frame/TOC bound is from
[RFC 6716 sections 3.2.1–3.2.2](https://www.rfc-editor.org/rfc/rfc6716.html#section-3.2.1).
This covers single-frame stereo Opus, not arbitrary multichannel or aggregated
Opus packets. Actual VPN encapsulation, path MTU and TCP segmentation are outside
this packetizer test; no guarantee for every VPN is implied.

To run the packetizer regression against an existing patched dependency install:

```sh
cmake -S test/whip/packetization -B build_whip_packet_tests \
  -DLibDataChannel_DIR=/absolute/path/to/whip-dependency/install/lib/cmake/LibDataChannel \
  -DCMAKE_PREFIX_PATH=/absolute/path/to/obs-deps
cmake --build build_whip_packet_tests --config Release
ctest --test-dir build_whip_packet_tests -C Release --output-on-failure
```

### Mac release signing and publication

`build-aux/sign-whip-macos.py` signs the nested Mach-O files and bundles from the
inside out, preserving their embedded OBS/CEF entitlements. It refuses ad-hoc
identities and debug entitlements. Its `release` command requires Apple's explicit
`Accepted` result, staples and validates the ticket, checks Gatekeeper, then
extracts and verifies the final tar archive again. Interrupted submissions retain
their ID and app signature hash under the evidence directory.

Configure credentials locally using Apple's secure password prompt (never put the
password in the command, repository, or chat):

```sh
xcrun notarytool store-credentials OBS-WHIP --team-id H3CKR5XB3J --apple-id YOUR_APPLE_ID
```

The initial conversion staging directories and original downloads remain in
ignored `build_whip_signing/stage/` and
`build_whip_signing/original/{stable,preview}/`. The current rebuild uses
`build_whip_final/stage/` and fresh metadata snapshots in
`build_whip_final/original/{stable,preview}/`. The initial conversion used the
following command for each signed staging directory (the stored submission ID supports resuming an interrupted run):

```sh
python3 build-aux/sign-whip-macos.py release \
  --stage build_whip_signing/stage/obs-whip-32.2.2-darwin-arm64 \
  --team-id H3CKR5XB3J --keychain-profile OBS-WHIP \
  --output build_whip_signing/signed/stable/obs-whip-32.2.2-darwin-arm64.tar.gz \
  --evidence build_whip_signing/evidence/obs-whip-32.2.2-darwin-arm64
```

Repeat for Intel and preview. For a fresh unsigned staging tree, use `all` instead
of `release` and supply `--identity 'Developer ID Application: Steve Seguin (H3CKR5XB3J)'`.
The archive retains the original `whip-build.json`; the separate
`macos-signing.json` records signing, unchanged code hashes, Apple's submission ID,
and verification results.

`build-aux/update-whip-macos-release.py` converts an existing unsigned release,
preparing updated checksums, provenance,
and notes from these verified packages. It preserves Windows/Linux package entries
and retains the entire original Mac package entry as `original_build_package`,
with signing/notarization in `signing`. It checks that the live release metadata
has not changed before uploading; `--publish` explicitly enables replacement and
public-download verification. For a corrected Mac rebuild, additionally supply
`--rebuild-run RUN_ID --release-notes REVIEWED_NOTES_FILE`. This requires a
successful WHIP CI run on master and matching notarized artifacts; it permits only
the WHIP revision to change in the manifest, preserving the OBS base and patched
dependency. Previous signed records remain in `superseded_packages`.
Supply accurate runtime findings in a text file:

```sh
python3 build-aux/update-whip-macos-release.py \
  --original build_whip_signing/original/stable \
  --signed build_whip_signing/signed/stable \
  --evidence build_whip_signing/evidence \
  --validation-notes build_whip_signing/stable-validation.txt \
  --tag v32.2.2-whip-relay
```

For a fresh unsigned release, review its generated notes, then rerun with
`--publish`. Repeat with `preview`
and `v33.0.0-beta6-whip-relay`. Stable remains Latest; preview remains Prerelease.
Publication fails if any Mac archive lacks verified notarization/stapling. The
completed releases intentionally cannot be overwritten using the stale original
metadata snapshots; the updater detects changed live metadata.

For future CI builds, manually dispatch **WHIP builds** on `master` with
`notarize_macos=true`. This requires `MACOS_SIGNING_IDENTITY`, `MACOS_SIGNING_CERT`
(base64 PKCS12), `MACOS_SIGNING_CERT_PASSWORD`, `MACOS_NOTARIZATION_USERNAME`, and
`MACOS_NOTARIZATION_PASSWORD` repository secrets. Missing credentials fail the job;
there is no fallback to ad-hoc signing in release mode. The workflow imports the
identity into a temporary Keychain, validates the notarization credentials, calls
`build-whip.py --macos-release`, and uploads an artifact ending in `-notarized`
only after all gates pass. Credentials are removed in an `always()` cleanup step.
Ordinary push/PR builds remain explicitly unofficial, unnotarized test artifacts.
The workflow does not automatically publish GitHub releases.

Run the portable release-gate regressions with:

```sh
python3 test/whip/test-macos-signing.py
python3 test/whip/test-macos-release.py
```


With the platform's OBS build prerequisites installed:

```powershell
python build-aux/build-whip.py --version 33.0.0-beta6 --generator "Visual Studio 17 2022" --jobs 4
```

On macOS/Linux omit `--generator`; use `--arch arm64` or `--arch x86_64` on macOS.
Linux additionally needs the matching CEF archive and distribution development
packages, as installed by the workflow. To configure an upstream stable checkout:

```powershell
python build-aux/prepare-whip-source.py --ref 32.2.2 --destination build_whip_stable/source
python build_whip_stable/source/build-aux/build-whip.py --version 32.2.2 --generator "Visual Studio 17 2022" --jobs 4
```

The dependency helper can also update an existing configured OBS build:

```powershell
python build-aux/build-whip-dependency.py --obs-build build_whip_review/dev --jobs 4
```

`&relay&tcp` verifies browser-to-TURN TCP/TLS transport. OBS still uses UDP with
the current libjuice backend; this is not a test of an OBS network blocking UDP.
The Windows/macOS DTLS workaround remains restricted to the exact `whip.vdo.ninja`
hostname. Mac runtime coverage is recorded above; Linux publisher runtime
interoperability still needs separate testing.

## Fresh Windows interoperability results

After the reviewed server deployment, both GitHub-built Windows versions passed
all six production Chrome/Firefox routing cases again (12/12 total), plus a
60-second TLS-relay observation, six live signaling checks and five proxy fault
scenarios. A separate pre-merge UDP check then ran `&relay` for 30 seconds in both
browsers on OBS 32.2.2 and 33.0.0-beta6: all four cases passed with the selected
candidate explicitly reporting `relayProtocol: udp`. Video advanced by 885–900
frames per case and Opus audio bytes increased. This required no VDO.Ninja
application changes. The UDP summary is retained locally at
`build_whip_review/udp-merge-1791053340355/summary.json`.

On 2026-10-03, official OBS 32.2.2 and 33.0.0-beta6 ZIPs were downloaded and
checked against their GitHub release SHA-256 values. Separate portable directories
kept the installed OBS and other running OBS instances untouched. The stable test
directory replaced only `obs-webrtc.dll` and `datachannel.dll`; the development
test used a full local build of this fork.

The downloadable Windows stable and preview artifacts from GitHub run
`37138506023` were also extracted and tested directly against production VDO.Ninja.
OBS 32.2.2 and 33.0.0-beta6 each passed all six Chrome/Firefox routing cases.
Their reports are `WhipMatrix_1791047821204/report.json` and
`WhipMatrix_1791047984986/report.json` in the same VDO.Ninja artifact directory.
These checks include the packaged patched dependency, not just local builds.

Chrome for Testing 154.0.8037.92 and Playwright Firefox 155.0 (OpenH264 2.6.0)
received synthetic H.264 720p30 video and Opus audio through the live WHIP service.
Both `https://vdo.ninja` and the supplied local frontend were tested, with no
frontend DTLS experiment enabled.

| Windows publisher | Default | `&relay` | `&relay&tcp` |
| --- | --- | --- | --- |
| Stock 32.2.2, Chrome / production | Fail | Fail | Fail |
| Stock 32.2.2, Firefox / production | Pass | Fail | Fail |
| Stock 33.0.0-beta6, Chrome / local frontend | Fail | Fail | Fail |
| Patched 32.2.2, both browsers / both frontends | 4/4 pass | 4/4 pass | 4/4 pass |
| Patched development fork, both browsers / both frontends | 4/4 pass | 4/4 pass | 4/4 pass |

All 24 patched matrix cases required advancing decoded video, increasing audio
bytes, and a playing video element. Relay cases also required the selected local
candidate to be `relay`; TCP cases required the selected TURN transport to be
TCP/TLS. The observed relay routes used UDP on port 3478 and TLS on port 443.
An additional 60-second Chrome/TLS test advanced from 17 to 1816 decoded frames
with increasing audio bytes. Typical connection time was 3–4 seconds and stream
stop took about 0.5 seconds. An ordinary browser publisher also passed both relay
routes as a positive control.

Five proxy fault probes passed: delayed POST drained late candidates with exactly
one final end-of-candidates PATCH; OPTIONS without ICE-server links still connected through relay;
absent ETag connected without PATCH; a five-second PATCH was cancelled during
stop in 502 ms; malformed answer SDP stopped the output. Every probe DELETEd
its allocated session. Header parsing tests, existing STUN vectors, eight
role/integrity cases, and six role-conflict decisions passed locally.

Evidence is retained in ignored local artifacts. In the sibling VDO.Ninja checkout,
`tests/playwright/test-results/WhipMatrix_1791045778296/report.json` contains the
stable matrix, `WhipMatrix_1791046057560/report.json` the development matrix, and
`WhipMatrix_1791046309656/report.json` the 60-second observation. OBS artifacts
include `build_whip_review/fault-summary.json` and `probe-*.json`. The VDO.Ninja
`tests/playwright/obs-whip-review.md` documents the harness invocation.

These checks prove the selected viewer TURN routes in this network, not every NAT
topology or a UDP-blocked OBS publisher. Mobile, macOS/Linux publishers, and long-duration streaming were not
retested in this fresh matrix. No VDO.Ninja application source was changed.

## Original review record

Local `master` includes origin/master `a3dbbc6a7` and OBS upstream/master
`cffa83ba5` through merge `d2360bd95`. The changes described below are local
working-tree changes; nothing was pushed. The pre-existing reverse-trickle plan
was preserved.

## Findings and changes

- The reverse-trickle extension only receives viewer candidates in replies to
  OBS PATCH requests. Candidates arriving after the last PATCH can be stranded.
  Stop advertising this extension and use the complete WHIP answer instead.
- libdatachannel permits only one gathering pass. The fork attempted another
  pass after POST and could omit end-of-candidates. Drain the original pass, or
  start gathering with POST's servers when OPTIONS supplied none.
- Candidate HTTP requests ran on PeerConnection callbacks. A dedicated,
  cancellable worker now serializes PATCH requests and end-of-candidates, and
  joins before the peer is closed.
- Header parsing missed valid `ETag:"value"` and tab-separated values, and could
  match a different header with the same prefix. Match the exact name and colon.
- SDP construction could throw outside the error handler. Reject malformed SDP
  inside the handler and DELETE the allocated session on failure.
- Windows/macOS bundled Mbed TLS rejected current Chrome's DTLS ClientHello with
  `The requested feature is not available`. For **Windows/macOS and the exact host
  `whip.vdo.ninja` only**, offer the active DTLS role. Other hosts and platforms
  retain actpass. This offer is permitted by
  [RFC 9725 section 4.4.4](https://www.rfc-editor.org/rfc/rfc9725.html#section-4.4.4).
- Chrome's TLS relay checks contained ICE-CONTROLLED with a zero tie-breaker.
  libjuice treated zero as an absent attribute and returned STUN 400. The patch
  tracks attribute presence separately from its numeric value. Existing message
  authentication remains enforced. This was confirmed with a capture restricted
  to the test OBS socket, followed by successful desktop and Android relay tests.
- Route libdatachannel warnings/errors into the OBS log for diagnosis.

## Build

Configure a Windows x64 OBS build first, then run from the repository root:

```powershell
./build-aux/build-whip-dependency.ps1 -OBSBuildDirectory build_whip_review -Jobs 4
```

The helper rebuilds libdatachannel at
`4e4f4892dccb2a57fe3a490d0c9d958de4244e74`, with libjuice
`5948a4162d37bc213d6051b67ee2876ccc5a99a6`, applies the checked-in patch, runs
the STUN regression tests, and installs into a separate dependency prefix.
These are the pins in the
[OBS 2026-08-26 dependency recipe](https://github.com/obsproject/obs-deps/blob/2026-08-26/deps.ffmpeg/70-libdatachannel.ps1).
Mbed TLS remains the TLS backend. Downloaded OBS dependencies are untouched.
The OBS build cache is pointed at this prefix and the runtime DLL is updated.
**Building only the OBS source fixes against the stock dependency leaves the
Chrome TCP/TLS relay bug present.** CI/release builds must also apply this patch
until the dependency is updated upstream.

The review build used VS 2022 Build Tools 14.43, CMake 3.31, SDK 10.0.26100,
and the upstream Windows preset with its generator overridden to VS 2022,
on Windows 11 build 26200.
Scripting, virtual camera and AJA were disabled locally. AJA's prebuilt library
required an unavailable newer MSVC runtime symbol. This is not a complete
release build or a validation of those excluded features.

## Browser results

Synthetic 720p30 H.264, 2,500 kbps, B-frames 0, four x264 threads, and Opus audio;
one viewer at a time against the production VDO.Ninja frontend and WHIP service.
Default cases used the `/whip#obs` GO workflow. Relay cases used the main viewer
with `&relay` and `&relay&tcp`. The selected ICE pair was checked, so relay passes
cannot silently mean host-only playback.

| Viewer | Default | UDP relay | TCP/TLS relay |
| --- | --- | --- | --- |
| Desktop Chrome 154 (headless, installed Chrome runtime) | Pass | Pass | Pass (TLS) |
| Desktop Firefox 149 (native browser) | Pass | Pass | Pass (TLS) |
| Pixel 4a Chrome 153 (real Android device) | Pass | Pass | Pass (TLS) |

All nine cases decoded advancing video and received advancing audio during an
eight-second observation, with a playing video element. Connections took
1.0–3.6 seconds; stops took 0.50–0.52 seconds. Screenshots were captured and the
Android relay rendering was visually inspected.

Evidence: [final matrix JSON](../../../vdoninja/tests/playwright/test-results/WhipMatrix_1791035210849/report.json).
The initial build with the OBS fixes but stock libjuice passed 7/9 cases; both
Chrome TLS cases failed. Replacing the dependency fixed both.

A subsequent 60-second Chrome TLS relay observation also passed: 1,800 additional
decoded frames, 1,142,235 additional audio bytes and a 514 ms stop. Evidence:
[one-minute relay check](../../../vdoninja/tests/playwright/test-results/WhipMatrix_1791035797710/report.json).
The final runtime's patched `datachannel.dll` SHA-256 is
`358e2cc70255663f21932e095c20e17e983260dd64b3f6a9d23076dcc4d13eb9`.

Focused Firefox fault-injection checks also passed:

| Scenario | Observed result |
| --- | --- |
| STUN-only OPTIONS; POST delayed two seconds | Video/audio played; five candidate PATCHes followed by exactly one end-of-candidates PATCH |
| No ICE servers in OPTIONS | Gathering after POST connected successfully |
| No ETag | Video/audio played; no PATCH requests were sent |
| PATCH responses delayed five seconds | Video/audio played; Stop completed in 514 ms, cancelled the pending PATCH and DELETE succeeded |
| Malformed SDP answer | OBS logged the invalid integer, remained responsive and DELETE succeeded; no media was expected |

Fault-injection traces: `build_whip_review/review/proxy-*.json`.
Matrix artifacts for those checks, respectively: `WhipMatrix_1791035577750`,
`WhipMatrix_1791035632889`, `WhipMatrix_1791035656885`,
`WhipMatrix_1791035682423`, and `WhipMatrix_1791035708419` in the adjacent
VDO.Ninja `tests/playwright/test-results` directory. The malformed-SDP artifact
deliberately reports a playback timeout; the expected verdict is rejection with
a responsive OBS process and successful resource deletion.

The reusable integration harness is in the adjacent VDO.Ninja checkout at
`tests/playwright/obs-whip-matrix.cjs`; it uses a separate portable OBS profile,
generated canvas/tone, installed browsers, and the attached Android device.
The harness expects an idle dedicated portable runtime with OBS WebSocket on
port 4464 and authentication disabled for the test. The handed-over runtime's
WebSocket server is disabled after testing. Its test profiles/scenes are archived
under `build_whip_review/review/test-config`.
`proxy.cjs` is a loopback-only HTTP fault injector for that harness; set
`WHIP_PROXY_REPORT` and point `WHIP_ENDPOINT` at `http://127.0.0.1:8101/`.
It records counts/statuses rather than SDP, tokens or addresses.

`headers.cpp` covers exact header names, casing and whitespace. `ice-roles.c`
runs libjuice's existing STUN vectors plus eight role/value combinations and
checks correct-password acceptance and wrong-password rejection.

The header test can be built from an x64 Visual Studio developer prompt after
building OBS (adjust the build/configuration paths if necessary):

```bat
cl /nologo /std:c++17 /EHsc /MD /MP1 /I libobs /I build_whip_review\config /I deps\w32-pthreads test\whip\headers.cpp /Fo:build_whip_review\headers.obj /Fe:build_whip_review\rundir\RelWithDebInfo\bin\64bit\whip-header-test.exe /link build_whip_review\libobs\RelWithDebInfo\obs.lib
build_whip_review\rundir\RelWithDebInfo\bin\64bit\whip-header-test.exe
```

Observed: zero header failures, existing STUN vectors and all eight ICE
role/integrity cases passed. The dependency helper's final build log is
`build-whip-reproducible-final.log`.

## Media timing and encoder pacing

`packetization/media.cpp` tests the production helpers in `whip-media-utils.h`.
The dependency build runs this test alongside the packet-budget test on every
Windows, macOS, and Linux CI job. It covers an hour of encoder timestamps at
24/30/60, 23.976/29.97/59.94/119.88 fps, negative preroll, Opus clock steps,
skipped time, random offsets, and repeated RTP timestamp wraps. The absolute-clock
conversion avoids the roughly 400 ms/hour error reproduced by the previous
per-frame rounding at 59.94 fps. This is a numerical regression, not an hour-long
browser synchronization measurement.

WHIP applies a two-second keyframe interval through the normal encoder settings,
preserving a shorter choice. Encoder-specific raw GOP options remain explicit
overrides; this change does not rewrite those options. Output startup applies
these settings even when the service-recommendations checkbox is disabled.

The existing packet pacer retains its 10x allowance for bitrate-controlled modes,
uses the peak for VBR, and includes all simulcast layers. Quality modes use a
100 Mbps allowance instead of an inactive saved bitrate, unless an applicable
peak limit is enabled. A 4 Mbps minimum allowance accommodates low-rate encoder
overshoot. These are transport allowances, not encoder bitrate changes or
congestion control. The existing 5 ms scheduling and NACK implementation remain;
Ninja's bounded repair queues and finer pacing need separate loss/reordering tests.
In particular, do not treat a false `Track::send()` result as a video failure:
libdatachannel's asynchronous pacing handler consumes the message list and the
outer send then returns false even when packets were successfully queued.

The adjacent VDO.Ninja `tests/playwright/obs-whip-matrix.cjs` accepts
`WHIP_ENCODER`, `WHIP_ENCODER_SETTINGS` (JSON), `WHIP_FPS_NUM`, `WHIP_FPS_DEN`, and
`WHIP_APPLY_SERVICE_SETTINGS=0` for these regressions. It records loaded module
paths/hashes and receiver freeze/concealment counters, and verifies TURN/UDP as
well as TURN/TCP or TLS. Existing `WHIP_OBS_PORTABLE`, `WHIP_CHROME_PATH`,
`WHIP_OPENH264_DIR`, `WHIP_ENGINES`, `WHIP_BASES`, `WHIP_MODES`, and
`WHIP_OBSERVE_MS` select the isolated runtime and receiver matrix.
`WHIP_MIN_DECODED_FPS` adds a performance gate; advancing frames alone can pass
while an incorrectly configured pacer reduces a 60 fps stream to about 5 fps.

### Windows media validation — revision `446e6ee4e`, 2026-10-03

The local Windows fork build passed **15 production playback cases**, each with
a 20-second observation and optional service recommendations disabled:

| Encoder settings | Receivers | Routes | Result |
| --- | --- | --- | --- |
| x264 CBR, 720p59.94, auto keyframe interval, custom `bframes=3` | Chrome, Firefox | Default, TURN/UDP, TURN/TLS | 6/6 |
| NVIDIA CQP 23, inactive bitrate 1 kbps, custom `frameIntervalP=4` | Chrome | All three | 3/3 |
| Intel Quick Sync ICQ 23, inactive bitrate 1 kbps | Chrome | All three | 3/3 |
| NVIDIA VBR, 2.5 Mbps target / 12 Mbps peak | Chrome | All three | 3/3 |

Each case decoded about 59–60 fps, received advancing Opus audio, and reported
zero video freezes and lost video packets. Loaded module paths/hashes were
checked against the newly compiled DLL. Encoder logs confirmed 119-frame x264
and 120-frame NVENC intervals, and two seconds for QSV. These are short checks,
not endurance or loss-injection tests, and no bitstream recordings were made.

A controlled CQP comparison with B-frames already disabled and the same
one-kbps inactive setting reproduced **5.40 decoded fps** in the older
`2629bcaa8` build versus **59.93 fps** in the new build. A 48 fps gate rejected
the old build and accepted the new one. An initial probe had passed the older,
weaker frame-progress check at 5.25 fps; that observation is retained.
An additional NVENC 100 kbps CBR TURN/UDP case passed the 48 fps gate and verified
the 4 Mbps pacing allowance while preserving a one-second keyframe choice. It
decoded 56.87 fps but reported one 0.99-second freeze and no packet loss; this
extreme low-rate case is not evidence of consistently smooth playback.
Fresh 30-second controls with repeated headers explicitly enabled in both builds
then decoded 59.95 fps (old) and 59.94 fps (new), with zero reported freezes or
packet loss. An earlier old-build control without explicit repeated headers
connected and received RTP but decoded no video. All observations are retained;
they do not establish the cause of the initial freeze.

Windows MSVC packetization, media-helper and ICE tests passed. The media helpers
also passed Linux Clang with address and undefined-behavior sanitizers. The full
patch applied cleanly to stable 32.2.2 and preview 33.0.0-beta6.
[CI run 37172045445](https://github.com/steveseguin/obs-studio/actions/runs/37172045445)
passed all 12 builds and their regressions: fork, stable and preview on Windows,
Linux, Mac ARM and Mac Intel. Formatting, service and compatibility checks also
passed. No live Mac or Linux publisher was tested in this Windows follow-up.
[Machine-readable results](../../docs/whip/windows-media-validation-2026-10-03.json)
retain the comparison and test limits; detailed reports and logs remain under
ignored `build_whip_review/media-*` and the adjacent VDO.Ninja test-results tree.
Only the VDO.Ninja test harness was changed for these checks, not website code.

The actual **32.2.2 Windows package from CI run 37172045445** was then downloaded,
its source manifest and loaded DLL hash verified, and tested with NVENC CQP at
59.94 fps, inactive bitrate 1 kbps, auto keyframe interval and conflicting
`frameIntervalP=4`. All three Chrome routes passed the 48 fps gate at 59.82–59.93
decoded fps, with advancing audio and no reported video freezes or packet loss.
The archive hash and selected-route evidence are included in the JSON above.
These packages are CI artifacts; this validation did not replace release assets.

## Limits

KRD's Windows 10 LTSC 1809, Broadwell QuickSync/ICQ, high bitrates, extended
endurance, Safari, and Linux publishers were not tested. Mac coverage is recorded
above. The local upstream-master build identifies as OBS 33 development, not KRD's OBS 32.2.2.
The Windows/macOS VDO DTLS workaround is deliberately scoped; other WHIP providers
have not been certified. No VDO.Ninja application or production server code was
changed. Build/test outputs and local detailed logs are under ignored build
and test-results directories.
