# Device Module Refactoring - Implementation

This directory contains the refactored device module implementation to support multi-camera configurations.

## Components Implemented

### 1. Resource Manager (`resource_manager.h/cpp`)
- **Purpose**: Centralized tracking and allocation of hardware resources
- **Features**:
  - Tracks VI devices (max 4)
  - Tracks VPSS groups (max 32)
  - Tracks VENC channels (max 16, with separate H.264/H.265 limits)
  - Thread-safe resource allocation and deallocation
  - Resource availability queries
  - Status reporting

### 2. Stream Configuration (`stream_config.h/cpp`)
- **Purpose**: Stream configuration validation and parsing
- **Features**:
  - Validates resolution, framerate, and bitrate
  - Parses encoder types (H.264/H.265 with CBR/VBR/AVBR)
  - Output configuration (RTSP, RTMP, MP4, JPEG)
  - Helper functions for recommended bitrates
  - RTMP limitation check (H.264 only)

### 3. Camera Instance (`camera_instance.h/cpp`)
- **Purpose**: Manages a single camera and its resources
- **Features**:
  - Lifecycle management (start/stop)
  - Stream creation and management
  - Feature enable/disable (OSD, AIISP, YOLOv5, VO)
  - Resource allocation via resource_manager
  - Status reporting
  - Thread-safe operations

### 4. Camera Manager (`camera_manager.h/cpp`)
- **Purpose**: Central registry and factory for camera instances
- **Features**:
  - Camera creation and destruction
  - Camera lookup by ID
  - Configuration validation
  - Resource availability checking
  - Maximum camera limit enforcement (1-4 cameras)
  - Thread-safe operations

## Architecture

```
camera_manager
    ├── camera_instance (camera 0)
    │   ├── VI device 0
    │   ├── VPSS group 0
    │   ├── Stream 0 (VENC channel 0)
    │   ├── Stream 1 (VENC channel 1)
    │   └── Features (OSD, AIISP, etc.)
    │
    ├── camera_instance (camera 1)
    │   ├── VI device 1
    │   ├── VPSS group 1
    │   ├── Stream 0 (VENC channel 2)
    │   └── Stream 1 (VENC channel 3)
    │
    └── ...

resource_manager (tracks all allocations)
```

## Usage Example

```cpp
#include "camera_manager.h"
#include "resource_manager.h"

using namespace hisilicon::device;

// Initialize resource manager
resource_limits limits;
limits.max_vi_devices = 4;
limits.max_vpss_groups = 32;
limits.max_venc_channels = 16;
resource_manager::init(limits);

// Initialize camera manager
camera_manager::init(4); // Support up to 4 cameras

// Create camera configuration
camera_config config;
config.camera_id = -1; // Auto-assign ID
config.enabled = true;
config.sensor = sensor_config("OS04A10", "liner");

// Configure main stream
stream_config main_stream;
main_stream.stream_id = 0;
main_stream.name = "main";
main_stream.type = encoder_type::H264_CBR;
main_stream.width = 1920;
main_stream.height = 1080;
main_stream.framerate = 30;
main_stream.bitrate = 4096;
main_stream.outputs.rtsp_enabled = true;
main_stream.outputs.rtsp_url_path = "/camera/0/main";

config.streams.push_back(main_stream);

// Create camera
auto camera = camera_manager::create_camera(config);
if (camera) {
    // Start camera
    if (camera->start()) {
        std::cout << "Camera started successfully" << std::endl;
    }
    
    // Add another stream dynamically
    stream_config sub_stream;
    sub_stream.stream_id = 1;
    sub_stream.name = "sub";
    sub_stream.type = encoder_type::H264_CBR;
    sub_stream.width = 704;
    sub_stream.height = 576;
    sub_stream.framerate = 30;
    sub_stream.bitrate = 1024;
    sub_stream.outputs.rtsp_enabled = true;
    sub_stream.outputs.rtsp_url_path = "/camera/0/sub";
    
    camera->create_stream(sub_stream);
    
    // Enable features
    camera->enable_feature("osd", "");
    camera->enable_feature("aiisp", "/opt/ceanic/model/aiisp.bin");
    
    // Get status
    camera_status status = camera->get_status();
    std::cout << "Camera " << status.camera_id 
              << " has " << status.stream_count << " streams" << std::endl;
    
    // Stop camera when done
    camera->stop();
}

// Cleanup
camera_manager::release();
resource_manager::release();
```

## Building

The implementation files are header/source pairs that can be compiled with C++17:

```bash
g++ -std=c++17 -c resource_manager.cpp -o resource_manager.o
g++ -std=c++17 -c stream_config.cpp -o stream_config.o
g++ -std=c++17 -c camera_instance.cpp -o camera_instance.o
g++ -std=c++17 -c camera_manager.cpp -o camera_manager.o
```

## Unit Tests

Unit tests are provided in `/unit_tests/cn_analyst/device/`:

```bash
cd /path/to/unit_tests/cn_analyst/device
make test
```

Tests cover:
- Resource manager allocation/deallocation
- Thread-safe concurrent access
- Camera manager lifecycle
- Configuration validation
- Resource limit enforcement

## Integration with Existing Code

### Phase 1: Parallel Implementation (Current)
- New code exists alongside old code
- No breaking changes to existing functionality
- Old `dev_chn.h/cpp` continues to work

### Phase 2: Gradual Migration (Next)
- Create wrapper layer in `dev_chn` that uses camera_manager
- Update `main.cpp` to use new API
- Test single-camera compatibility

### Phase 3: Full Migration
- Remove old static arrays
- Remove `MAX_CHANNEL` limitation
- Fully replace old implementation

## Limitations and TODOs

### Current Limitations
1. **Placeholder VI/VPSS/VENC initialization**: Real hardware initialization code not yet integrated
2. **Feature management**: Feature enable/disable is simplified
3. **Observer pattern**: Full stream distribution not yet implemented
4. **Configuration loading**: JSON parsing not yet integrated

### Future Work
1. Integrate with existing `dev_vi`, `dev_vpss`, `dev_venc` classes
2. Implement full observer pattern for stream distribution
3. Add JSON configuration loading/saving
4. Integrate with RTSP/RTMP stream routing
5. Add performance monitoring and statistics
6. Implement hot-plug camera support
7. Add dynamic resolution switching

## Hardware Resource Constraints

Based on HiSilicon 3519DV500 specifications:

| Resource | Total | Max per Camera | Typical Usage |
|----------|-------|----------------|---------------|
| VI Devices | 4 | 1 | 1 per camera |
| VPSS Groups | 32 | 1-2 | 1 per camera |
| VPSS Channels | 128 (4/grp) | 2-4 | 2-3 per camera |
| VENC Channels | 16 total | 4-8 | 2-3 per camera |
| H.265 Channels | 4-8 | 2-4 | 1-2 per camera |
| H.264 Channels | 8-12 | 2-6 | 1-2 per camera |
| ISP Pipelines | 4 | 1 | 1 per camera |

**Practical Limit**: 2-4 cameras with 2-4 streams each

## Design Decisions

1. **Singleton Pattern**: Used for resource_manager and camera_manager to ensure centralized control
2. **Thread Safety**: All public APIs are protected with mutexes
3. **Smart Pointers**: Used std::shared_ptr for camera instances to prevent memory leaks
4. **RAII**: Resources are automatically freed in destructors
5. **Validation First**: All configurations are validated before resource allocation
6. **Fail Fast**: Invalid configurations fail immediately with clear error messages

## Contact

For questions about this implementation:
- Review the code comments
- Check unit tests for usage examples
- Refer to the main project README.md
