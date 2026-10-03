# WHIP fork review — 2026-10-03

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
