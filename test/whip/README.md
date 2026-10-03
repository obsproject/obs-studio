# WHIP fork review — 2026-10-03

## Reproducible compatibility builds

The `WHIP builds` GitHub workflow builds the current fork, the latest stable OBS
release, and the latest applicable prerelease on Windows x64, macOS arm64/x86_64,
and Linux x86_64. It runs on code pushes, WHIP-related pull requests, and manual
dispatch. In `steveseguin/obs-studio`, this replaces the inherited Push workflow's
build job, which does not apply the patched ICE dependency. Formatting, service,
and compatibility checks remain enabled.
Stable/preview builds apply only the four WHIP source files changed since upstream
`cffa83ba5`; an incompatible patch fails the job instead of silently omitting fixes.

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

**Mac signing handoff (updated 2026-10-03):** the public releases still contain
ad-hoc-signed, unnotarized Mac packages. All four original Mac archives were
downloaded and checked against the published checksums and `whip-build.json`.
Local copies are now signed with Steve Seguin's Developer ID Application identity
(team `H3CKR5XB3J`), secure timestamps, and hardened runtime. Strict verification
passed for every nested Mach-O and bundle, with original entitlements preserved.
Comparisons after removing signatures and normalizing the signature segment's VM
allocation confirmed unchanged executable payloads, including patched libdatachannel.
No source rebuild was needed.

Apple notarization, stapling, Gatekeeper acceptance, and replacement of the public
assets remain pending notarization credentials. The Developer ID certificate,
encrypted certificate password, and identity are configured as GitHub repository
secrets. No notarization username/password secrets or usable local `notarytool`
profile were found. Do not describe the public packages as notarized until the
release gate below succeeds.

### Signed Mac runtime checks — 2026-10-03

Both 32.2.2 and 33.0.0-beta6 launched on macOS 26.4.1 / Apple M1, with arm64
running natively and x86_64 under Rosetta. The test used a temporary OBS
configuration, generated 720p30 video and tone audio, x264 and Opus, and the
production `whip.vdo.ninja` endpoint. The user's original OBS configuration was
restored after each run; no VDO.Ninja application source was changed.

**All 12 Chrome cases failed media delivery:** both versions / both architectures,
each with default routing, `&relay`, and `&relay&tcp`. OBS logged Mbed TLS DTLS
handshake failures (`The requested feature is not available`). Selected browser
candidates confirmed TURN/UDP and TURN/TLS on the retried Intel and preview tests,
but no decoded media followed. The existing DTLS workaround applies only to
Windows, so the prior Windows media passes must not be generalized to macOS.
Signing did not alter the executable payloads; these are runtime limitations of
the retained CI binaries.

An additional Firefox 142.0.1 check on stable arm64 received increasing Opus audio
bytes in both relay attempts, but decoded no video. This test browser reported no
H.264 receive codec, and the default attempt encountered a closed-peer statistics
error. These attempts are **not** successful full-media interoperability tests.
An additional local browser-source render probe returned black frames on all four
signed apps and on the original ad-hoc stable arm64 app. It does not establish a
signing regression or a successful CEF render test. Physical Intel hardware, a
current Firefox with H.264, and an OBS network blocking UDP were not tested. Detailed local reports and logs are retained under ignored
`build_whip_signing/smoke/`.

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

The current staging directories and original downloads are in ignored
`build_whip_signing/stage/` and `build_whip_signing/original/{stable,preview}/`.
For each already-signed package, use the corresponding staging directory:

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

`build-aux/update-whip-macos-release.py` prepares updated checksums, provenance,
and notes from these verified packages. It preserves Windows/Linux package entries
and retains the entire original Mac package entry as `original_build_package`,
with signing/notarization in `signing`. It checks that the live release metadata
has not changed before uploading; `--publish` explicitly enables replacement and
public-download verification. Supply accurate runtime findings in a text file:

```sh
python3 build-aux/update-whip-macos-release.py \
  --original build_whip_signing/original/stable \
  --signed build_whip_signing/signed/stable \
  --evidence build_whip_signing/evidence \
  --validation-notes build_whip_signing/stable-validation.txt \
  --tag v32.2.2-whip-relay
```

Review its generated notes, then rerun with `--publish`. Repeat with `preview`
and `v33.0.0-beta6-whip-relay`. Stable remains Latest; preview remains Prerelease.
Publication fails if any Mac archive lacks verified notarization/stapling.

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
The Windows DTLS workaround remains restricted to the exact `whip.vdo.ninja`
hostname. macOS/Linux publisher runtime interoperability needs separate testing.

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
- Windows' bundled Mbed TLS rejected current Chrome's DTLS ClientHello with
  `The requested feature is not available`. For **Windows and the exact host
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

## Limits

KRD's Windows 10 LTSC 1809, Broadwell QuickSync/ICQ, high bitrates, extended
endurance, Safari, and macOS/Linux publishers were not tested. The local
upstream-master build identifies as OBS 33 development, not KRD's OBS 32.2.2.
The Windows/VDO DTLS workaround is deliberately scoped; other WHIP providers
have not been certified. No VDO.Ninja application or production server code was
changed. Build/test outputs and local detailed logs are under ignored build
and test-results directories.
