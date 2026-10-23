# Reverse Trickle ICE — Investigation & Implementation Plan

## 1. Executive Summary

The OBS WHIP plugin currently supports **client→server trickle ICE** (OBS sends its
ICE candidates to the server via PATCH after the initial POST). It does **not** support
**server→client trickle ICE** (receiving the remote peer's ICE candidates via PATCH
response bodies). This document:

- Confirms the RFC 9725 spec position on reverse trickle
- Explains why the 2-second delay exists and whether reverse trickle helps
- Lists exact code changes needed across three repositories
- Rates each change by risk / complexity

---

## 2. Spec Analysis — RFC 9725 § 4.3.2

> "…the WHIP session cannot signal additional ICE candidates to the WHIP client after
> the SDP answer has been sent."
> — RFC 9725 Section 4.3.2

**Takeaway**: Standard WHIP requires the server to deliver a *complete* answer SDP (all
ICE candidates included). Reverse / server→client trickle is explicitly **not defined**
in the spec.

RFC 8840 (trickle-ice-sdpfrag format) defines only the wire format for candidate
exchange, not who sends it.

### What the OBS plugin currently does

| Direction              | Status          |
|------------------------|-----------------|
| Client→Server (PATCH)  | ✅ Implemented  |
| Server→Client (PATCH response body) | ❌ Not implemented |

---

## 3. Root Cause of the 2-Second Delay

The delay is **not** a trickle ICE problem. Here is the full POST flow:

```
OBS                  whip.js server          Browser (lib.js)
 │──── POST /steve ───────►│                      │
 │                         │── WebSocket msg ──────►│
 │                         │   {sdp: offer, get: pid}│
 │                         │                      │  setRemoteDescription(offer)
 │                         │                      │  createAnswer()
 │                         │                      │  setLocalDescription(answer)
 │                         │                      │  ←── ICE gathering (500ms–2s)
 │                         │◄── WebSocket reply ──│
 │                         │   {callback:{get:pid, result: answerSDP}}
 │◄── 201 + full SDP ──────│                      │
 │  setRemoteDescription   │                      │
 │  (all remote candidates │                      │
 │   already in SDP)       │                      │
```

The browser uses the native WebRTC API and waits for `iceGatheringState === 'complete'`
(or all ICE candidates from `onicecandidate`) before sending back the answer SDP. This
waiting period (ICE STUN resolution, TURN allocation) is the 1–3 second gap.

The 10-second timeout in whip.js (`processRequest`) is the worst case; typical is 1–2s.

---

## 4. Would Reverse Trickle Actually Help?

Yes, but only if we also make the **browser send an early/partial answer**. Here's why:

### Current flow (blocking)
The browser sends answer only after **ICE gathering complete**. This adds 1–3s.

### Proposed trickle flow
```
OBS                  whip.js server          Browser (lib.js)
 │──── POST /steve ───────►│                      │
 │                         │── WebSocket msg ──────►│
 │                         │   {sdp: offer, get: pid}│
 │                         │                      │  setRemoteDescription(offer)
 │                         │                      │  createAnswer()          ← fast
 │                         │                      │  setLocalDescription()   ← fast
 │                         │◄── WebSocket reply ──│  answer (no candidates yet)
 │◄── 201 + bare SDP ──────│                      │
 │  setRemoteDescription() │                      │  onicecandidate fires…
 │  (no remote candidates) │                      │──► WS {type:candidate, …}
 │──── PATCH (my cand) ───►│ returns browser cand │
 │   addRemoteCandidate()  │◄─ response body ─────│  (pending remote cands)
 │──── PATCH (my cand) ───►│ returns more cand    │
 │                         │                      │
```

**The POST now completes almost instantly** (~50ms) instead of 1–3s, because the
browser sends the answer before ICE gathering is done.

The tradeoff: OBS gets zero remote candidates from the 201 answer. All remote
candidates arrive later via PATCH response bodies. This requires OBS to:
1. Collect PATCH response bodies
2. Parse them as `application/trickle-ice-sdpfrag`
3. Call `peer_connection->addRemoteCandidate()` for each

### Risk assessment

| Concern | Detail |
|---------|--------|
| Spec compliance | Non-standard extension. Safe to deploy as opt-in on both sides. |
| ICE failure fallback | If OBS ignores PATCH response bodies, ICE will still work if the server eventually gets all browser candidates into the answer. Browser trickle can be optional. |
| TURN candidate delay | With reverse trickle, OBS gets TURN candidates from the browser later. ICE may start with host-only candidates and converge once TURN candidates arrive. This is fine. |
| ETag handling | PATCH response with `200 OK` + body must update the ETag. OBS already handles ETag updates on successful PATCH. |

---

## 5. Required Changes

### 5.1 OBS: `whip-output.cpp` — Read PATCH Response Bodies

**File**: `plugins/obs-webrtc/whip-output.cpp`
**Risk**: Low — purely additive; falls back gracefully if body is absent or malformed.

#### 5.1.1 Add response body collection to `SendTrickleIcePatch`

Current setup of the PATCH curl handle (line ~1095) has no write callback.
Add one alongside the existing header callback:

```cpp
std::string patch_response_body;

curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_writefunction);
curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&patch_response_body);
```

`curl_writefunction` is the same function already used in `Connect()`.

#### 5.1.2 After a 2xx response, inspect Content-Type and process body

After the existing `response_code` check in `SendTrickleIcePatch` (line ~1112), add:

```cpp
if (response_code >= 200 && response_code < 300 && !patch_response_body.empty()) {
    // Check if server sent back remote candidates
    bool has_trickle_body = false;
    for (auto &hdr : http_headers) {
        auto ct = value_for_header("content-type", hdr);
        if (ct.find("application/trickle-ice-sdpfrag") != std::string::npos) {
            has_trickle_body = true;
            break;
        }
    }
    if (has_trickle_body) {
        ApplyIncomingRemoteCandidates(patch_response_body);
    }
}
```

#### 5.1.3 New method: `ApplyIncomingRemoteCandidates`

```cpp
void WHIPOutput::ApplyIncomingRemoteCandidates(const std::string &sdp_frag)
{
    std::string current_mid;
    std::istringstream stream(sdp_frag);
    std::string line;

    while (std::getline(stream, line)) {
        line = trim_string(line);
        if (line.empty())
            continue;

        if (line.rfind("a=mid:", 0) == 0) {
            current_mid = line.substr(6); // after "a=mid:"
        } else if (line.rfind("a=candidate:", 0) == 0) {
            // libdatachannel Candidate(string candidate, string mid)
            // candidate string should NOT include the "a=" prefix
            std::string cand_str = line.substr(2); // strip "a="
            try {
                rtc::Candidate remote_cand(cand_str, current_mid);
                if (peer_connection) {
                    peer_connection->addRemoteCandidate(remote_cand);
                    do_log(LOG_DEBUG, "Added remote candidate (mid=%s): %s",
                           current_mid.c_str(), cand_str.c_str());
                }
            } catch (const std::exception &e) {
                do_log(LOG_WARNING, "Failed to add remote candidate: %s", e.what());
            }
        }
        // a=end-of-candidates: libdatachannel handles this automatically
        // when setRemoteDescription was called with an empty candidate list.
        // No explicit API needed.
    }
}
```

**Header** (`whip-output.h`): Add declaration:
```cpp
void ApplyIncomingRemoteCandidates(const std::string &sdp_frag);
```

#### 5.1.4 Verify `rtc::Candidate` constructor

From `candidate.hpp`:
```cpp
Candidate(string candidate);           // candidate string only
Candidate(string candidate, string mid); // candidate string + mid
```

The `candidate` parameter is the raw ICE candidate string **without** the `a=` prefix
(e.g., `"candidate:1234 1 udp 2113937151 192.168.1.1 54321 typ host ..."`).
The `candidate()` accessor returns the same format.

This is consistent with how `onLocalCandidate` fires: `candidate.candidate()` returns
the string without `a=`. So stripping `"a="` from `"a=candidate:..."` is correct.

---

### 5.2 `whip.js` — Always Return Pending Remote Candidates in PATCH Response

**File**: `C:\Users\steve\Code\whip\whip.js`
**Risk**: Low — changes only the PATCH response when remote candidates are queued.

#### Current code (line 678–690)

```js
const responseBody = buildTrickleIceResponse(state);
if (responseBody && ENABLE_LEGACY_PATCH_RESPONSE_BODY) {
    res.status(200);
    res.set('Content-Type', 'application/trickle-ice-sdpfrag');
    return res.send(responseBody);
}
if (responseBody) {
    console.log("PATCH drained queued remote candidates but returning 204 (spec mode)");
}
return res.status(204).end();
```

The problem: `buildTrickleIceResponse(state)` **always drains** `pendingRemoteCandidates`
regardless of whether the body is sent. In spec mode, those candidates are silently
discarded. The OBS client never gets them.

#### Proposed change

Remove the `ENABLE_LEGACY_PATCH_RESPONSE_BODY` gate and always include the body
when there are pending remote candidates. Use `200 OK` (not `204`) when a body
is present, consistent with RFC 9725 § 4.3.3 (ICE restart response semantics):

```js
const responseBody = buildTrickleIceResponse(state);
res.header('Access-Control-Expose-Headers', EXPOSE_HEADERS);
res.set('ETag', formatEtag(currentEtag));

if (responseBody) {
    // Return browser-side candidates to OBS via response body.
    // Non-standard WHIP extension; OBS will ignore if it doesn't support it.
    res.status(200);
    res.set('Content-Type', 'application/trickle-ice-sdpfrag');
    return res.send(responseBody);
}
return res.status(204).end();
```

This is backward-compatible: older OBS versions (or any WHIP client) that don't read
the PATCH response body will still function. The ETag is unchanged, so no
re-synchronization is needed on the client side.

> **Note on `ENABLE_LEGACY_PATCH_RESPONSE_BODY`**: Remove this env flag and the
> associated check. The behavior it enabled is now the default. The `console.warn`
> at startup can also be removed.

---

### 5.3 `lib.js` — Send Early Answer Before ICE Gathering Completes

**File**: VDO.Ninja `lib.js` (browser client)
**Risk**: Medium — changes the WHIP viewer answer flow. Needs careful testing.

> **Note**: The actual `lib.js` file was not available locally at review time.
> The following describes the **pattern** to apply, based on the whip.js WebSocket
> protocol and standard browser WebRTC API. The actual line numbers and function
> names must be confirmed against the live codebase.

#### Current behavior (presumed)

When the browser receives a WHIP offer via WebSocket (`{sdp: offer, get: pid, type: "post"}`),
the current code likely does something like:

```js
// Simplified current flow
const pc = new RTCPeerConnection(config);
await pc.setRemoteDescription({ type: 'offer', sdp: msg.sdp });
const answer = await pc.createAnswer();
await pc.setLocalDescription(answer);

// Wait for ICE gathering to complete (adds 1–3 seconds)
await new Promise(resolve => {
    if (pc.iceGatheringState === 'complete') return resolve();
    pc.addEventListener('icegatheringstatechange', () => {
        if (pc.iceGatheringState === 'complete') resolve();
    });
});

// Now send the complete answer (with all candidates embedded)
sendCallback(msg.get, pc.localDescription.sdp);
```

#### Proposed change — early answer + candidate trickling

```js
const pc = new RTCPeerConnection(config);
await pc.setRemoteDescription({ type: 'offer', sdp: msg.sdp });
const answer = await pc.createAnswer();
await pc.setLocalDescription(answer);

// Send the answer IMMEDIATELY — don't wait for ICE gathering.
// Candidates will be sent separately as they arrive.
sendCallback(msg.get, pc.localDescription.sdp);

// Trickle candidates via WebSocket as they're gathered
pc.addEventListener('icecandidate', (event) => {
    if (!event.candidate) {
        // End of candidates
        ws.send(JSON.stringify({
            type: 'end-of-candidates',
            streamID: msg.streamID
        }));
        return;
    }
    ws.send(JSON.stringify({
        type: 'candidate',
        candidate: {
            candidate: event.candidate.candidate,
            sdpMid: event.candidate.sdpMid,
            sdpMLineIndex: event.candidate.sdpMLineIndex,
            usernameFragment: event.candidate.usernameFragment
        },
        streamID: msg.streamID
    }));
});
```

The WebSocket server (`whip.js`) already handles `type: "candidate"` messages from the
browser (lines 1101–1151) and stores them in `state.pendingRemoteCandidates`. With the
PATCH response change from §5.2, those candidates will be delivered to OBS.

#### lib.js also needs to handle incoming remote candidates

When OBS sends trickle candidates via PATCH, the server forwards them to the browser
via WebSocket (`{type: "candidate", candidate: {...}, streamID: room}`). The browser must
call `pc.addIceCandidate()` for each:

```js
// In the WebSocket message handler:
if (msg.type === 'candidate' && msg.candidate) {
    try {
        await pc.addIceCandidate(new RTCIceCandidate(msg.candidate));
    } catch (e) {
        console.warn('addIceCandidate failed:', e);
    }
}
if (msg.type === 'end-of-candidates') {
    try {
        await pc.addIceCandidate(null); // signals end-of-candidates to browser
    } catch (e) {}
}
```

This is likely **already implemented** (the server already forwards OBS candidates to
the browser). Confirm by searching `lib.js` for `addIceCandidate`.

---

## 6. Data Flow After All Changes

```
OBS                       whip.js                   Browser (lib.js)
─────                     ───────                   ───────────────
POST /room ──────────────►│
                          │── WS: {sdp:offer} ──────►│
                          │                          │  setRemoteDescription()
                          │                          │  createAnswer()    (fast)
                          │                          │  setLocalDescription()
                          │◄─ WS: {callback: answer} │  ← IMMEDIATE, no ICE wait
◄──── 201 + bare SDP ─────│                          │
setRemoteDescription()    │                          │  onicecandidate fires...
(0 remote candidates)     │                          │──►WS: {type:candidate, ...}
                          │  pendingRemoteCandidates │──►WS: {type:candidate, ...}
                          │  accumulates...          │──►WS: {type:end-of-cands}

PATCH (OBS cand 1) ──────►│◄── browser cands in body─┘
  addRemoteCandidate()    │  (drained from state)
PATCH (OBS cand 2) ──────►│  (more if available)
  addRemoteCandidate()    │
PATCH (end-of-cands) ────►│

ICE connectivity checks run on both sides → CONNECTED
```

---

## 7. Fallback Behavior (Backward Compatibility)

| Scenario | whip.js #2 | lib.js #3 | OBS #1 | Result |
|----------|-----------|-----------|--------|--------|
| All old  | ❌ | ❌ | ❌ | Current behavior. Works. |
| Only #2 deployed | ✅ | ❌ | ❌ | PATCH returns body, old OBS ignores it. lib.js still sends full answer. Works. ✅ |
| Only #3 deployed | ❌ | ✅ | ❌ | lib.js sends bare answer. whip.js discards pendingRemoteCandidates. OBS gets zero remote candidates. **ICE FAILS** ❌ |
| #2 + #3, no #1 | ✅ | ✅ | ❌ | lib.js sends bare answer. whip.js returns browser candidates in PATCH body. OBS ignores body. **ICE FAILS** ❌ |
| #1 + #2, no #3 | ✅ | ❌ | ✅ | lib.js sends full answer (all candidates). PATCH body empty. Works as before. ✅ |
| All three | ✅ | ✅ | ✅ | Full reverse trickle. Fast POST, all candidates exchanged via PATCH. ✅ |

**The dangerous scenario is deploying #3 without #1 being in users' hands.**
whip.js (#2) is safe to deploy at any time — it never breaks anyone.

---

## 8. Implementation Order & Deployment Reality

The three changes have very different deployment velocities:

| Change | Who controls rollout | Reaches users |
|--------|----------------------|---------------|
| `whip.js` (#2) | Server deploy — instant | 100% of users immediately |
| `lib.js` (#3) | VDO.Ninja alpha/beta push | Subset of users soon |
| `whip-output.cpp` (#1) | OBS plugin update | Small subset, weeks/months later |

### Why this ordering matters

**#3 has a hard dependency on #1.** If lib.js sends an early/bare answer (no ICE
candidates) and OBS cannot read PATCH response bodies, ICE fails completely. There is
no fallback — OBS calls `setRemoteDescription()` with zero remote candidates and never
adds any, because it discards the PATCH response body.

Deploying #3 to VDO.Ninja beta before #1 is widely distributed would break connections
for all beta users who are still on a standard OBS install.

### Safe deployment sequence

**Phase 1 — Ship #2 now (zero risk)**
Deploy the `whip.js` change that always returns pending remote candidates in the PATCH
response body. Old OBS ignores the body. New OBS (once it exists) benefits. No user is
broken.

**Phase 2 — Ship #1 and wait**
Ship the `whip-output.cpp` change as part of the OBS plugin. Wait for meaningful
adoption. Because this is a plugin update (not OBS core), adoption is slow. There is no
reliable way to know when "enough" users have it.

**Phase 3 — Ship #3 with a capability gate**
To avoid breaking users who haven't updated their OBS, the early-answer behavior in
lib.js must be gated on whether OBS has signaled support. One practical approach:

OBS adds a custom header to the initial POST:
```
X-WHIP-Trickle-In: 1
```

whip.js stores this flag on the room state. When forwarding the offer to the browser
via WebSocket, it includes `tricklePatchSupported: true` in the message payload.
lib.js only uses the early-answer path if `tricklePatchSupported === true` in the
incoming offer message. Without the flag, lib.js uses the current blocking behavior —
full ICE gathering before sending the answer — which continues to work for all users.

This requires a minor additional change to `whip-output.cpp` (add one header) and
`whip.js` (relay the flag), but makes #3 safe to ship to all users at any time,
since users without the new OBS simply continue on the old path.

**Phase 3 alternative — URL opt-in**
Gate the early-answer behavior behind `?tricklein=1` on the VDO.Ninja viewer URL.
Users who know they have the new OBS plugin can opt in manually. Simpler to implement
but requires manual coordination.

---

## 9. Testing Checklist

- [ ] OBS → whip.vdo.ninja → Browser: connection establishes in < 500ms (POST response)
- [ ] OBS logs show "Added remote candidate" entries after PATCH responses
- [ ] ICE connects successfully (PeerConnection state: Connected)
- [ ] Audio/video streams correctly
- [ ] End-of-candidates is received and processed
- [ ] OBS with old whip.js (no PATCH body): still works (regression test)
- [ ] Browser with old lib.js (blocking answer): still works (regression test)
- [ ] ETag is correctly maintained across PATCH sequence
- [ ] 412/428 from server disables trickle correctly

---

## 10. Open Questions

1. **Does lib.js already call `addIceCandidate` for incoming WebSocket candidates?**
   Search `lib.js` for `addIceCandidate`. If yes, the browser-side candidate ingestion
   is already done and only the early-answer change is needed.

2. **Does the browser include host candidates in the immediate answer?**
   `setLocalDescription(answer)` triggers ICE agent to start. `pc.localDescription.sdp`
   at that moment typically includes only the `a=ice-ufrag` / `a=ice-pwd` lines and
   the format, not actual candidates. This is fine — OBS can set the remote description
   and start ICE, then add candidates as they arrive.

3. **Should `a=end-of-candidates` in the PATCH response trigger anything special in OBS?**
   libdatachannel's `addRemoteCandidate` with an empty candidate string may signal EoC.
   Verify against libdatachannel source or test behavior.

4. **What happens to pending remote candidates that arrive before the first PATCH?**
   They will sit in `state.pendingRemoteCandidates` until the first PATCH. If OBS never
   PATCHes (no trickle candidates), they will be lost. A possible mitigation: whip.js
   sends a notification via a secondary channel (e.g., a new header in the 201 response)
   telling OBS to do an empty PATCH to collect remote candidates.

---

## 11. Summary Table

| Component | Change | Risk | Required |
|-----------|--------|------|----------|
| `whip-output.cpp` | Collect PATCH response body | Low | For reverse trickle |
| `whip-output.cpp` | Parse trickle-ice-sdpfrag body | Low | For reverse trickle |
| `whip-output.cpp` | Call `addRemoteCandidate()` | Low | For reverse trickle |
| `whip-output.h` | Add `ApplyIncomingRemoteCandidates` decl | Low | For reverse trickle |
| `whip.js` | Always send PATCH response body | Low | For reverse trickle |
| `whip.js` | Remove `ENABLE_LEGACY_PATCH_RESPONSE_BODY` flag | Low | Cleanup |
| `lib.js` | Send early answer (no ICE wait) | Medium | For speed improvement |
| `lib.js` | Trickle outgoing candidates via WebSocket | Low | For speed improvement |
| `lib.js` | Handle incoming candidates (addIceCandidate) | Low | Likely already done |
