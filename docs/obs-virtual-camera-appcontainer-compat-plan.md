# OBS Virtual Camera — AppContainer Compatibility

## Context

This document describes the minimal OBS source change needed so that
**Microsoft Teams** (which runs in a low-IL AppContainer) can successfully
open and read from the OBS virtual camera shared memory.

- **OBS Studio** runs at medium IL (normal process) and is the virtual camera *producer*.
- **Microsoft Teams** runs in a low-IL AppContainer and is the virtual camera *consumer*.

Teams' AppContainer process instantiates the OBS DirectShow virtual camera filter
(an in-process COM server DLL), which then opens the named file mapping that OBS
created to read video frames. The fix is: ensure Teams can open that file mapping.

---

## Architecture: IPC Between OBS and Teams

The virtual camera uses exactly one named kernel object for producer–consumer
communication:

| Object | Name | Type | OBS creates | Teams opens |
|--------|------|------|-------------|-------------|
| Video shared memory | `OBSVirtualCamVideo` | Named file mapping | `CreateFileMappingW` with `PAGE_READWRITE` | `OpenFileMappingW` with `FILE_MAP_READ` |

There are no other shared named kernel objects. State transitions
(starting → ready → stopping) are communicated through a volatile `uint32_t state`
field inside the shared memory — no named events, mutexes, or semaphores are used.
The ring buffer uses volatile `write_idx` / `read_idx` fields with no external
synchronisation primitives.

---

## The Code Change

**File:** `shared/obs-shared-memory-queue/shared-memory-queue.c`
**Function:** `video_queue_create()`

Add `#include <sddl.h>` and replace the bare `CreateFileMappingW` call with one
that sets an explicit DACL granting `GENERIC_READ | GENERIC_EXECUTE` to
`ALL_APP_PACKAGES` (`S-1-15-2-1`):

```c
/* Build a security descriptor that grants read access to ALL_APP_PACKAGES
 * (S-1-15-2-1) so the DirectShow virtual camera filter can open the
 * mapping from inside an AppContainer (e.g. Microsoft Teams). The DACL
 * grants:
 *   GA (Generic All)  to CO (Creator Owner)
 *   GA                to SY (Local System)
 *   GRGX              to WD (Everyone / World)
 *   GRGX              to S-1-15-2-1 (ALL_APP_PACKAGES)
 */
PSECURITY_DESCRIPTOR psd = NULL;
SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, FALSE};
if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;CO)(A;;GA;;;SY)(A;;GRGX;;;WD)(A;;GRGX;;;S-1-15-2-1)",
        SDDL_REVISION_1, &psd, NULL)) {
    sa.lpSecurityDescriptor = psd;
}
vq.handle = CreateFileMappingW(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, size, VIDEO_NAME);
if (psd)
    LocalFree(psd);
```

`S-1-15-2-1` is the well-known SID for `ALL_APP_PACKAGES`, covering every
AppContainer process regardless of which specific container. This grants
`GENERIC_READ | GENERIC_EXECUTE`, which satisfies `FILE_MAP_READ`.

### Why this is the correct file

Both `win-dshow` and `obs-virtualcam-module` link against the CMake interface
library `OBS::shared-memory-queue`, whose source is
`shared/obs-shared-memory-queue/shared-memory-queue.c`.

`plugins/win-dshow/shared-memory-queue.c` is **not listed in any CMake target**
and is never compiled. Edits to that file have no effect.

---

## Why Nothing Else Needs to Change

| Component | Why it already works |
|-----------|---------------------|
| COM registration (HKCR) | AppContainer processes can read HKCR and load in-process COM servers. No special AppX flags needed for in-process activation. |
| DLL file access (`obs-virtualcam-module64.dll`) | Installed to `Program Files\obs-studio\…`, which has ACLs that include `ALL_APP_PACKAGES` read access by default. |
| Volatile shared memory state | Polled directly through the mapped view; no kernel synchronisation objects needed. |
| Frame ring buffer | Lock-free via volatile indices inside the shared memory. No external primitives. |
| Teams `webcam` capability | Teams already declares the `webcam` capability in its AppContainer manifest. |

---

## Pre-conditions (Not Code Changes)

1. **OBS Virtual Camera must be installed** — the COM server
   (`obs-virtualcam-module64.dll`) must be registered in HKCR before Teams starts.
   This is handled by the OBS installer.
2. **OBS must have the virtual camera active** (via Tools → Virtual Camera) before
   Teams selects it. This creates the file mapping that Teams reads.

---

## Build and Deploy

```bat
cmake --preset windows-x64
cmake --build build_x64 --target win-dshow --config RelWithDebInfo
```

To deploy, stop OBS then replace the installed DLL:

```bat
copy /Y build_x64\rundir\RelWithDebInfo\obs-plugins\64bit\win-dshow.dll ^
    "C:\Program Files\obs-studio\obs-plugins\64bit\win-dshow.dll"
```

---

## Verification

### Step 1 — Verify the DACL (independent of Teams)

1. Start OBS. Add any video source. **Tools → Start Virtual Camera**.
2. Open **Process Explorer** (Sysinternals, run as Administrator).
3. **Find → Find Handle or DLL** (Ctrl+F), search for `OBSVirtualCamVideo`.
4. Double-click the result → **Security** tab.
5. Confirm `ALL APPLICATION PACKAGES` is listed with **Read** permission.

Alternative via PowerShell (run after starting the virtual camera):

```powershell
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class FM {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr OpenFileMapping(uint access, bool inherit, string name);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
}
"@
$h = [FM]::OpenFileMapping(4, $false, "OBSVirtualCamVideo")   # 4 = FILE_MAP_READ
if ($h -ne [IntPtr]::Zero) { Write-Host "OPEN OK"; [FM]::CloseHandle($h) }
else { Write-Host "FAILED: 0x$(([ComponentModel.Win32Exception]::new()).NativeErrorCode.ToString('X'))" }
```

### Step 2 — End-to-end Teams test

1. Start OBS with a video source active, then **Tools → Start Virtual Camera**.
2. Open **Microsoft Teams**.
3. Go to **Settings → Devices → Camera**.
4. Select **OBS Virtual Camera** in the dropdown.
5. The preview thumbnail should show the OBS canvas content in real time.
6. Use **Settings → Devices → Make a test call** to confirm video in a call.

### Step 3 — Diagnose failures with Process Monitor

If Teams cannot open the camera, capture a Procmon trace:

1. Start **Process Monitor**, filter: `Process Name is ms-teams.exe`, Operation is `OpenFileMappingW`.
2. Start OBS virtual camera, then switch to Teams camera settings.
3. Find the `OpenFileMappingW` call for `OBSVirtualCamVideo` in the output.
4. Check the **Result** column:
   - `ACCESS DENIED` → DACL missing `S-1-15-2-1`; verify you edited
     `shared/obs-shared-memory-queue/shared-memory-queue.c` and rebuilt.
   - `NAME NOT FOUND` → OBS virtual camera is not running.
   - `SUCCESS` → mapping opened; if Teams still shows no video, check the
     filter's frame thread via the `video_queue_state()` polling loop.

---

## Summary

The only OBS source change needed is adding `ALL_APP_PACKAGES (S-1-15-2-1)` to
the SDDL of the `CreateFileMappingW` call in
`shared/obs-shared-memory-queue/shared-memory-queue.c`. No other modifications
are required for Microsoft Teams to consume OBS Studio's virtual camera from
within its low-IL AppContainer.
