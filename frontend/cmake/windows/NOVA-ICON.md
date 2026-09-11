# Nova icon provenance

Only `build/icon.ico` was copied from Kryptographer/obs at commit
`0434c5dde764bea7d922c9ac6066fb0fcce9614b`.

`nova.ico` is byte-for-byte identical to that file, with 16, 24, 32, 48, 64,
128 and 256 pixel images. `frontend/forms/images/nova.png` is the existing
256-pixel PNG payload extracted from that ICO without modification.

The application PE resource and setup installer use the ICO; Qt application,
projector, statistics and idle tray icons use the PNG. Desktop and Start menu
shortcuts obtain their icon from the installed application executable.
No code or other implementation was imported from the obsolete repository.
