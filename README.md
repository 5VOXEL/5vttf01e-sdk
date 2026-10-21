# 5VTTF01E SDK

Sample code and protocol reference for the **5Voxel 5VTTF01E (VoxelCount)**
standalone 3D time-of-flight camera.

The 5VTTF01E combines a ToF depth sensor, a 400 MHz AI-capable MCU and a 6-axis
IMU in one device. Over Ethernet it streams **80 × 60 depth and IR frames**
through a simple TCP command protocol. This repository shows how to connect to
it, stream and display frames, and read or change the device configuration.

![platform](https://img.shields.io/badge/platform-Windows-blue)
![language](https://img.shields.io/badge/C%2B%2B-11-informational)
![license](https://img.shields.io/badge/license-BSD--3--Clause-green)

## Contents

| Path | What it is |
|---|---|
| [`include/network_streaming.h`](include/network_streaming.h) | Protocol constants: commands, frame sizes, header markers |
| [`examples/network_streaming/`](examples/network_streaming/) | Windows console app that streams and displays depth + IR |
| [`docs/protocol.md`](docs/protocol.md) | Full command reference |
| [`CMakeLists.txt`](CMakeLists.txt) | CMake build (recommended) |
| [`vs2015/`](vs2015/) | Original Visual Studio 2015 solution |

## What the sample does

`network_streaming` connects to the device on TCP port `1111`, prints the
device info, sends `start`, then requests frames in a loop:

- **Console:** hostname, serial number, MAC address, firmware version and
  build date, then the depth at the center pixel for each frame.
- **"Depth" window:** depth map drawn as a JET false-color heatmap.
- **"IR" window:** IR amplitude image in grayscale.

| Key | Action |
|---|---|
| `1` | Depth frames only (`get depthframe`) |
| `2` | IR frames only (`get irframe`) |
| `3` | Depth + IR (`get depthirframe`) |
| `4` | Depth + IR with header check (`get h_depthirframe`, default) |
| `i` | Print device info again |
| `Esc` | Stop streaming and exit |

If the connection drops, the sample reconnects automatically.

## Requirements

- Windows 10/11
- Visual Studio 2015 or newer with the C++ workload
- [OpenCV](https://opencv.org/releases/) 3.2 or newer (`core`, `imgproc`, `highgui`)
- CMake 3.16+ (Visual Studio 2019/2022 includes it)

## Build

### CMake (recommended)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DOpenCV_DIR=C:/opencv/build/x64/vc16/lib
cmake --build build --config Release
```

`OpenCV_DIR` is the folder that contains `OpenCVConfig.cmake`. With CMake 3.21+
the OpenCV DLLs are copied next to the executable automatically.

### Visual Studio 2015 solution

Set `OPENCV_DIR` to an OpenCV 3.x `build` folder (the one containing `include\`
and `x64\vc14\lib\`), as an environment variable or with `/p:OPENCV_DIR=...`,
then open `vs2015\Network_Streaming.sln`. If `OPENCV_DIR` is unset, the project
looks in `third_party\opencv\build`. Newer Visual Studio versions will offer to
retarget the project; accept that.

## Run

1. Connect the device to your PC or network and power it up. Wait for the blue LED.
2. Find its IP address. The default is DHCP. With no DHCP server it falls back to
   `192.168.0.100` after about 20 s; in that case give your PC a static IP on
   `192.168.0.x`.
3. Run the sample:

   ```bat
   build\Release\network_streaming.exe 192.168.0.100
   ```

   Without an argument it asks for the IP address.

## Using the protocol in your own code

The protocol is plain TCP, so any language works. A minimal frame request:

1. Connect to `<device-ip>:1111`.
2. Send `start` and read the text response.
3. Send `get h_depthirframe` and read exactly **19,208 bytes**.
4. Check that bytes 0–1 are `F7 F7` and bytes 19,204–19,205 are `F8 F8`.
5. Bytes 4–9,603 are depth and 9,604–19,203 are IR: 80 × 60, `uint16`,
   little-endian.

See [`docs/protocol.md`](docs/protocol.md) for every command, including network
configuration and factory reset.

## Support

- Email: support@5voxel.com
- Bugs and questions about this SDK: open a GitHub issue

## License

BSD 3-Clause. See [LICENSE](LICENSE).
