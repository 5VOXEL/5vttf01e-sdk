# 5VTTF01E Network Protocol

This describes the TCP command protocol the 5VTTF01E (VoxelCount) device exposes
over its 10/100 Mbps Ethernet port. The constants used below are defined in
[`include/network_streaming.h`](../include/network_streaming.h).

## Connection

| Item | Value |
|---|---|
| Transport | TCP, device acts as server |
| Port | `1111` |
| Default addressing | DHCP. If no DHCP server answers within ~20 s, the device falls back to `192.168.0.100` |
| Keep-alive | Server-side TCP keep-alive, 1 s interval, 5 s idle timeout |

Status LEDs: **green** = power on, **blue** = firmware running. On the RJ45
connector, green shows link speed and amber shows activity.

## Command format

Commands are plain ASCII strings. Parameters use `name=value` with **no spaces
around `=`**. Text responses are padded to a fixed `CMD_RESP_LEN` (128 bytes).

### Streaming control

| Command | Response |
|---|---|
| `start` | `Streaming started` — starts depth sensing & streaming |
| `stop` | `Stop Streaming` — stops depth sensing & streaming |

### Frame requests

Frame commands return **binary data, not a text response**. All frames are
80 × 60 pixels, 16-bit little-endian per pixel.

| Command | Bytes | Layout |
|---|---|---|
| `get depthframe` | 9,600 | Depth |
| `get irframe` | 9,600 | IR (grayscale amplitude) |
| `get depthirframe` | 19,200 | Depth (9,600) + IR (9,600) |
| `get h_depthirframe` | 19,208 | Header start (4) + Depth (9,600) + IR (9,600) + Header end (4) |

Header framing for `get h_depthirframe`:

| Offset | Size | Content |
|---|---|---|
| 0 | 4 | `F7 F7 00 00` — start marker |
| 4 | 9,600 | Depth frame |
| 9,604 | 9,600 | IR frame |
| 19,204 | 4 | `F8 F8 00 00` — end marker |

Only the first two bytes of each marker are fixed. The last two are reserved
and may change in future firmware, so don't use them to validate frames.

The sample scales depth by `255 / 4096` and IR by `255 / 1024` for display.

### Device information

| Command | Example response | Notes |
|---|---|---|
| `get product_sn` | `PRXXXXXXXXXX` | Product serial number |
| `get hostname` | `MyDevice` | `variable not found` if never set |
| `get fw_ver` | `ES_VX.Y.Z-xxxx` | Application type, version, commit hash |
| `get fw_build_date` | `20200221` | `YYYYMMDD` |
| `get mac_addr` | `XX.XX.XX.XX.XX.XX` | |

### Configuration

Every `set` command answers `Set variable OK` on success. Values are held in
RAM until `set save_config_n_reboot` stores them to non-volatile memory and
reboots the device.

| Command | Notes |
|---|---|
| `set ip_addr=a.b.c.d` | Static IP |
| `set subnet_mask=a.b.c.d` | |
| `set default_gateway=a.b.c.d` | |
| `set dns=a.b.c.d` | Optional, defaults to `8.8.8.8` |
| `set dhcp=0` / `set dhcp=1` | `0` = static IP mode, `1` = DHCP |
| `set hostname=<name>` | User label, max 127 chars, no `=` or `;` |
| `set save_config_n_reboot` | Persist all settings and reboot |

To switch to a static IP, set `ip_addr`, `subnet_mask` and `default_gateway`,
set `dhcp=0`, then send `set save_config_n_reboot`.
`depth_camera_reconfig_static_ip()` in the sample does this interactively.

## Factory reset

1. Power off the device.
2. Hold the reset button.
3. Power on while still holding it.
4. After about 5 s the blue LED turns off and the red LED lights for about 1 s,
   then blue returns. Settings are restored and the device answers on
   `192.168.0.100` again.
