# GitHub Copilot Instructions for hi_app

## Project Overview

This is a C++17 embedded systems application for HiSilicon 3519DV500/3516DV500 platforms. It provides video streaming capabilities with RTSP/RTMP servers, AI-based image signal processing (AIISP), YOLOv5 object detection, and MP4 recording functionality.

## Technology Stack

- **Language**: C++17
- **Platform**: HiSilicon 3519DV500/3516DV500 (ARM-based embedded SoC)
- **Build System**: GNU Make (cross-compilation)
- **Key Libraries**:
  - HiSilicon MPP (Media Processing Platform) SDK
  - libevent (async networking)
  - librtmp (RTMP protocol)
  - log4cpp (logging)
  - FreeType (OSD text rendering)
  - JsonCpp (configuration)
  - HiSilicon SVP/AIISP (AI acceleration)

## Architecture

### Core Components

1. **Device Module** (`device/`): Hardware abstraction for video input (VI), encoding (VENC), video processing (VPSS), on-screen display (OSD), and video output (VO)
2. **RTSP Server** (`rtsp/`): Real-Time Streaming Protocol server with RTP serialization for H.264/H.265
3. **RTMP Client** (`rtmp/`): Real-Time Messaging Protocol client for pushing streams to nginx servers
4. **AIISP** (`aiisp/`): AI-based image signal processing (AI BNR, AI DRC, AI 3DNR)
5. **Stream Save** (`stream_save/`): MP4 file recording using HiSilicon middleware

### Data Flow
```
VI (sensor) → VPSS (processing) → VENC (encoding) → RTSP/RTMP/MP4
                ↓                      ↓
              YOLOv5              OSD/VO output
```

## Build Instructions

### Prerequisites
- HiSilicon SDK V2.0.2.0 or V2.0.2.1 must be compiled first (V2.0.2.1 recommended)
- Cross-compilation toolchain from HiSilicon SDK
- Project must be placed in: `Hi3519DV500_SDK_V{version}/smp/a55_linux/source/mpp/sample/`
  - Replace `{version}` with actual SDK version (e.g., 2.0.2.1)

### Build Commands
```bash
# Adjust SDK version path as needed (e.g., V2.0.2.0 or V2.0.2.1)
cd Hi3519DV500_SDK_V2.0.2.1/smp/a55_linux/source/mpp/sample/3516dv500_app
make          # Build the application
make clean    # Clean build artifacts
make strip    # Strip debug symbols
```

## Coding Conventions

### General Style
- Use C++17 features appropriately
- Follow existing code style (indent with spaces, not tabs based on existing files)
- Use snake_case for function names and variables
- Use PascalCase for class names
- Place opening braces on the same line for functions and classes

### Naming Conventions
- Prefix member variables with `m_` (e.g., `m_session`)
- Use descriptive names for variables and functions
- Global handles use `g_` prefix (e.g., `g_app_log`, `g_dev_log`)

### Header Files
- Use `#pragma once` or include guards
- Include `app_std.h` for common application headers
- Organize includes: system headers, third-party headers, project headers

### Logging
- Use log4cpp wrapper functions from `log/ceanic_log.h`
- Available log handles: `g_app_log`, `g_rtsp_log`, `g_rtmp_log`, `g_dev_log`
- Log levels: DEBUG, INFO, WARN, ERROR

### Error Handling
- Check return values from HiSilicon MPI functions (return `HI_SUCCESS` or error code)
- Use try-catch blocks for C++ exceptions when appropriate
- Clean up resources properly in error paths

### JSON Configuration
- All runtime configuration is in JSON files under `/opt/ceanic/etc/`
- Use JsonCpp library for parsing (see `json/` directory)
- Configuration files: `vi.json`, `venc.json`, `net_service.json`, etc.

## HiSilicon SDK Specifics

### Prefixes and Types
- `HI_` prefix for HiSilicon types and constants
- `OT_` prefix for OpenThings types (newer SDK naming)
- Channel management: VI→VPSS→VENC pipeline binding

### Key MPI Modules
- **VI**: Video Input (sensor interface)
- **VPSS**: Video Processing SubSystem (scaling, cropping)
- **VENC**: Video Encoding (H.264/H.265)
- **VO**: Video Output (display interface like BT1120)
- **SVP**: Smart Vision Platform (AI inference)

### Memory Management
- Use `HI_MPI_SYS_Mmap`/`HI_MPI_SYS_Munmap` for accessing video buffers
- Always release video frames after processing
- Be mindful of memory constraints in embedded environment

## Testing

### Manual Testing
There is no automated test suite. Testing is performed on actual hardware:

1. **Build**: Cross-compile on development machine
2. **Deploy**: Copy `rootfs/opt/ceanic` to device `/opt/` directory
3. **Load Kernel Modules**: Run `./load3519dv500 -i` from `/opt/ceanic/ko/`
4. **Run Application**: Execute `./ceanic_app` from `/opt/ceanic/bin/`
5. **Test Streams**:
   - RTSP: `rtsp://{device-ip}/stream1`, `stream2`, `stream3` (replace `{device-ip}` with actual device IP)
   - RTMP: Configure nginx server, set URLs in `net_service.json`
   - Use VLC to view streams

### Validation
- Verify video streams play correctly in VLC
- Check log files for errors in `/opt/ceanic/log/`
- Monitor memory usage on embedded device
- Verify proper cleanup on application exit

## Dependencies

### Adding New Dependencies
- Avoid adding new dependencies unless absolutely necessary (embedded environment)
- Place third-party libraries in `thirdlibrary/` directory
- Update `Makefile` with new include paths and library paths
- Prefer static linking (`.a` files) over dynamic linking

### Existing Third-Party Libraries
Located in `thirdlibrary/`:
- `freetype-2.7.1`
- `log4cpp`
- `libevent-2.0.18-stable`
- `rtmpdump`
- `hisilicon_mp4`

## Security Considerations

### Embedded Security Best Practices
- Validate all JSON configuration inputs
- Check array bounds when accessing video frame data
- Avoid buffer overflows in string operations
- Properly close network sockets and file handles
- Use secure defaults for network services

### Network Security
- RTSP runs on port 554 (configurable)
- RTMP pushes to external nginx server
- No authentication implemented by default—add if needed
- Be cautious with exposing services on public networks

## Documentation

### Code Documentation
- Document complex algorithms and HiSilicon-specific logic
- Add comments for non-obvious hardware interactions
- Keep README.md updated with new features
- Chinese comments are acceptable (project is maintained by Chinese team)

### Configuration Documentation
- Update `doc/set.md` when adding new configuration options
- Document JSON schema and valid values
- Provide examples for each configuration option

## Common Tasks

### Adding a New Sensor
1. Create sensor driver files in `device/sensor/`
2. Implement CMOS parameter functions
3. Add sensor-specific VI initialization in `device/dev_vi_*.cpp`
4. Update `vi.json` with new sensor name
5. Update Makefile to include new source files

### Adding a New Video Stream
1. Extend stream manager in `rtsp/stream/stream_manager.cpp`
2. Register new stream path in RTSP server
3. Create VENC channel in device module
4. Update configuration files if needed

### Modifying Video Pipeline
1. Understand VI→VPSS→VENC→RTSP flow
2. Modify channel initialization in `device/dev_chn.cpp`
3. Update VPSS group/channel configuration
4. Ensure proper resource cleanup

## Constraints and Limitations

- **Single Camera**: Currently hardcoded `MAX_CHANNEL = 1`
- **Static Configuration**: Requires application restart for config changes
- **Fixed Stream URLs**: Only `stream1`, `stream2`, `stream3` supported
- **Memory Limited**: Embedded platform with constrained resources
- **RTMP H.264 Only**: RTMP currently only supports H.264, not H.265
- **Cross-Platform**: Code only compiles with HiSilicon SDK toolchain

## Future Roadmap

See `cn_analyst/REFACTORING_ROADMAP.md` for detailed refactoring plans:
- Multi-camera support (2-4 cameras simultaneously)
- Dynamic configuration at runtime
- Flexible URL routing (`/camera/{id}/{stream}`)
- Modular plugin architecture
- Enhanced error handling and recovery

## Contact

Project maintained by:
- 深圳思尼克技术有限公司 (Shenzhen Ceanic Technology Co., Ltd.)
- Contact: jiajun.ma@ceanic.com

## Additional Resources

- HiSilicon SDK documentation in SDK package
- RTSP RFC 2326/RFC 7826
- RTMP specification
- Project analysis: `cn_analyst/ANALYSIS.md`
- Debug logs: `doc/debug_log.md`
