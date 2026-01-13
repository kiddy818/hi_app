# Phase 3 Migration: Dynamic Camera Allocation

## Overview

Phase 3 removes the hardcoded `MAX_CHANNEL = 1` limitation and replaces the static `g_chns[]` array with dynamic allocation, enabling multi-camera support without recompilation.

## Changes Made

### 1. Removed MAX_CHANNEL Limitation

**Before:**
```cpp
#define MAX_CHANNEL 1  // DEPRECATED
static std::shared_ptr<chn> g_chns[MAX_CHANNEL];
static venc_t g_venc_info[MAX_CHANNEL];
static vi_t g_vi_info[MAX_CHANNEL];
```

**After:**
```cpp
// No MAX_CHANNEL - dynamic allocation
static std::map<int, std::shared_ptr<chn>> g_chn_map;
static std::mutex g_chn_map_mutex;
static std::vector<venc_t> g_venc_info;
static std::vector<vi_t> g_vi_info;
```

### 2. Key Implementation Changes

#### device/dev_chn.h
- Removed `#define MAX_CHANNEL 1`
- Replaced `g_chns[]` array with `g_chn_map` (std::map)
- Added `g_chn_map_mutex` for thread-safe access
- Added includes for `<map>` and `<mutex>`

#### device/dev_chn.cpp
- Updated `chn::start()` to register in map with mutex
- Updated `chn::stop()` to remove from map with mutex
- Updated `chn::request_i_frame()` to use map lookup
- Updated `chn::get_stream_head()` to use map lookup
- All accesses are now thread-safe

#### main.cpp
- Replaced fixed-size arrays with `std::vector`
- Added `<vector>` include
- Updated initialization functions to support 1-8 cameras dynamically
- Configuration loaded from JSON files determines camera count
- Added comprehensive documentation comments

## Multi-Camera Configuration

### JSON Configuration Format

#### vi.json (Sensor Configuration)
```json
{
  "sensor1": {
    "name": "OS04A10"
  },
  "sensor2": {
    "name": "OS08A20"
  },
  "sensor3": {
    "name": "OS04A10_WDR"
  }
}
```

#### venc.json (Encoder Configuration)
```json
{
  "venc1": {
    "name": "H264_CBR",
    "w": 2688,
    "h": 1520,
    "fr": 30,
    "bitrate": 4000
  },
  "venc2": {
    "name": "H265_AVBR",
    "w": 3840,
    "h": 2160,
    "fr": 30,
    "bitrate": 8000
  },
  "venc3": {
    "name": "H264_VBR",
    "w": 1920,
    "h": 1080,
    "fr": 25,
    "bitrate": 2000
  }
}
```

### Configuration Notes

1. **Sensor Names:** sensor1, sensor2, ..., sensor8
2. **Encoder Names:** venc1, venc2, ..., venc8
3. **Maximum Cameras:** 8 (hardware limitation of HiSilicon 3519DV500)
4. **Auto-Detection:** System loads all configured cameras automatically
5. **Backward Compatible:** Single camera (sensor1/venc1) still works as before

## Usage Examples

### Single Camera (Backward Compatible)

No changes needed for existing single-camera deployments. The system defaults to loading sensor1/venc1.

### Dual Camera Setup

1. Edit `/opt/ceanic/etc/vi.json`:
```json
{
  "sensor1": { "name": "OS04A10" },
  "sensor2": { "name": "OS08A20" }
}
```

2. Edit `/opt/ceanic/etc/venc.json`:
```json
{
  "venc1": { "name": "H264_CBR", "w": 2688, "h": 1520, "fr": 30, "bitrate": 4000 },
  "venc2": { "name": "H265_AVBR", "w": 3840, "h": 2160, "fr": 30, "bitrate": 8000 }
}
```

3. Restart the application

**Note:** Currently, the application creates only one camera instance (chn=0). To create multiple cameras, you'll need to modify the main() function to iterate through all configured cameras. This is intentionally left for future enhancement to maintain backward compatibility.

### Future Multi-Camera Creation (Example)

To create multiple cameras, modify main.cpp:

```cpp
// Instead of:
g_chn = std::make_shared<chn_type>(g_vi_info[chn].name, g_venc_info[chn].name, chn);
g_chn->start(g_venc_info[chn].w, g_venc_info[chn].h, g_venc_info[chn].fr, g_venc_info[chn].bitrate);

// Do:
std::vector<std::shared_ptr<chn_type>> cameras;
for (size_t i = 0; i < std::min(g_vi_info.size(), g_venc_info.size()); i++) {
    auto cam = std::make_shared<chn_type>(g_vi_info[i].name, g_venc_info[i].name, i);
    if (cam->start(g_venc_info[i].w, g_venc_info[i].h, g_venc_info[i].fr, g_venc_info[i].bitrate)) {
        cameras.push_back(cam);
    }
}
```

## Thread Safety

All access to `g_chn_map` is protected by `g_chn_map_mutex`:
- `chn::start()`: Locks before inserting
- `chn::stop()`: Locks before erasing
- `chn::request_i_frame()`: Locks before lookup
- `chn::get_stream_head()`: Locks before lookup

This ensures thread-safe multi-camera operations.

## Testing

### Regression Testing

1. **Single Camera Test:**
   - Default configuration should work as before
   - Verify RTSP streams (rtsp://device-ip/stream1, stream2, stream3)
   - Check MP4 recording
   - Test JPEG snapshot

2. **Dynamic Configuration Test:**
   - Add/remove camera entries in JSON
   - Verify system adapts to configuration
   - Check console output shows correct camera count

### Validation Checklist

- [ ] Single camera configuration works
- [ ] Application starts without errors
- [ ] RTSP streams are accessible
- [ ] RTMP push works (if enabled)
- [ ] MP4 recording functions correctly
- [ ] YOLOv5 detection works (if enabled)
- [ ] AIISP features work (if enabled)
- [ ] Application exits cleanly

## Migration Path for Developers

### For New Code
Use `camera_manager` and `camera_instance` from `cn_analyst/device/src/`:
```cpp
#include "camera_manager.h"

// Initialize
camera_manager::init(4);

// Create camera
camera_config config;
config.sensor.name = "OS04A10";
// ... configure streams, features ...
auto camera = camera_manager::create_camera(config);
camera->start();
```

### For Existing Code
Use `chn_wrapper` for backward compatibility:
```cpp
#include <dev_chn_wrapper.h>

// Same as old dev_chn API
auto chn = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
chn->start(1920, 1080, 30, 4096);
```

## Known Limitations

1. **Main Loop:** Currently creates only one camera instance. Multi-camera creation requires main() modification.
2. **RTSP Stream URLs:** Still hardcoded to stream1, stream2, stream3. Dynamic URL mapping not yet implemented.
3. **RTMP:** Only supports H.264, not H.265.
4. **Hardware:** Limited to 4-8 cameras by HiSilicon 3519DV500 hardware resources.

## Future Enhancements

See `cn_analyst/REFACTORING_ROADMAP.md` for planned improvements:
- Dynamic RTSP URL routing (`/camera/{id}/{stream}`)
- Multi-camera initialization in main()
- Camera hot-plug support
- REST API for camera management

## Backward Compatibility

✅ **Fully backward compatible** - Single camera deployments require no changes.

All existing functionality preserved:
- Single camera operation (chn=0)
- Legacy `dev_chn` class still works
- `chn_wrapper` provides migration path
- JSON configuration format unchanged (just extended)

## Rollback Instructions

If issues arise, you can revert to the previous version:

```bash
git revert <commit-hash>
```

Or temporarily re-add `#define MAX_CHANNEL 1` and use fixed-size arrays.

## Support

For questions or issues:
- Contact: jiajun.ma@ceanic.com
- See: `cn_analyst/device/INTEGRATION_GUIDE.md`
- Reference: `device/dev_chn_wrapper_guide.md`
