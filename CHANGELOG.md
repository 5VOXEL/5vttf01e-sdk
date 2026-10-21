# Changelog

## 1.1.1 — 2026-10-21

Tested on a real device with firmware ES_V2.0.9.

### Removed
- IR frame support (`get irframe`, `get depthirframe`, `get h_depthirframe`),
  the "IR" window and the `1`–`4` frame-type keys. IR images were removed from
  firmware 2.x for privacy protection; the SDK is now depth only.

### Fixed
- Sample never displayed a frame on firmware 2.x: it requested depth + IR,
  which the device answers with `Unknown Cmd`, then timed out and reconnected
  forever. It now requests `get depthframe` only.
- `depth_camera_get_frame()` recognises an `Unknown Cmd` reply instead of
  waiting for the receive timeout.
- Pressing Esc when `stop` failed reconnected in a loop instead of exiting.

### Changed
- `depth_camera_get_frame(sock, buf, size)` no longer takes a frame command.

## 1.1.0 — 2026-10-21

First public release.

### Added
- CMake build (VS 2017+, any OpenCV 3.2+).
- `set dns` and `set dhcp` commands from SFS v1.2. The static IP helper now
  sends `dhcp=0`.
- Device IP can be passed as the first command-line argument.
- Protocol reference in `docs/protocol.md`.

### Fixed
- Socket handle stored in an `int`, which overflowed on x64 builds.
- `socket()`/`connect()` failures not detected (`SOCKET` is unsigned), and
  Winsock resources leaked on every reconnect attempt.
- Reconnect loop spun without delay.
- Frame buffer clear only covered 8 bytes (`sizeof` a pointer).
- Frame header check compared byte 0 twice instead of bytes 0 and 1.
- `set` commands reported success on any response.

### Changed
- VS2015 project reads OpenCV from `OPENCV_DIR` instead of a fixed relative path.

## 1.0.0 — 2020-02-21

Internal release: depth/IR streaming, device info, static IP and hostname configuration.
