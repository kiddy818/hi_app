# dev_chn_wrapper Migration Guide

## Overview

The `dev_chn_wrapper` class provides a backward-compatible interface for the legacy `dev_chn` class while using the new multi-camera architecture internally. This enables gradual migration from the single-camera codebase to the multi-camera system without breaking existing code.

## Purpose

- **Backward Compatibility**: Maintains the exact same API as `dev_chn`
- **Smooth Migration**: Allows existing code to work with minimal changes
- **Dual Mode**: Can use either new architecture or fall back to legacy implementation
- **Zero Risk**: Existing functionality continues to work during transition

## Architecture

```
┌─────────────────────────────────────────────────────┐
│              Application Code                        │
│         (No changes required)                        │
└──────────────────┬──────────────────────────────────┘
                   │
                   │ Same API as dev_chn
                   │
┌──────────────────▼──────────────────────────────────┐
│          dev_chn_wrapper                             │
│  ┌───────────────────────────────────────────────┐  │
│  │  Adapter Pattern                              │  │
│  │  - Translates old API to new API             │  │
│  │  - Falls back to legacy if needed            │  │
│  └───────────────┬───────────────────────────────┘  │
└──────────────────┼──────────────────────────────────┘
                   │
         ┌─────────┴─────────┐
         │                   │
         ▼                   ▼
┌─────────────────┐  ┌──────────────┐
│ camera_manager  │  │   dev_chn    │
│ camera_instance │  │   (legacy)   │
│  (new arch)     │  │              │
└─────────────────┘  └──────────────┘
```

## Usage Examples

### Example 1: Drop-in Replacement (Minimal Changes)

**Before (using dev_chn):**
```cpp
#include "dev_chn.h"
using namespace hisilicon::dev;

std::shared_ptr<chn> g_chn;

int main() {
    // Initialize system
    chn::init(OT_VI_ONLINE_VPSS_ONLINE);
    
    // Create camera
    g_chn = std::make_shared<chn>("OS04A10", "H264_CBR", 0);
    
    // Start
    g_chn->start(1920, 1080, 30, 4096);
    
    // Use camera...
    
    // Cleanup
    g_chn->stop();
    chn::release();
    
    return 0;
}
```

**After (using dev_chn_wrapper):**
```cpp
#include "dev_chn_wrapper.h"  // Only this line changes!
using namespace hisilicon::dev;

std::shared_ptr<chn_wrapper> g_chn;  // Change type

int main() {
    // Initialize system (same call)
    chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE);
    
    // Create camera (same call)
    g_chn = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
    
    // Start (same call)
    g_chn->start(1920, 1080, 30, 4096);
    
    // Use camera (same API)...
    
    // Cleanup (same call)
    g_chn->stop();
    chn_wrapper::release();
    
    return 0;
}
```

**Changes Required:**
1. Change `#include "dev_chn.h"` to `#include "dev_chn_wrapper.h"`
2. Change `chn` to `chn_wrapper` in type declarations
3. That's it! All method calls remain the same.

### Example 2: Using Type Alias (Even Cleaner)

```cpp
#include "dev_chn_wrapper.h"
using namespace hisilicon::dev;

// Type alias for easy switching
using chn_type = chn_wrapper;  // Change this line to switch implementations

std::shared_ptr<chn_type> g_chn;

int main() {
    chn_type::init(OT_VI_ONLINE_VPSS_ONLINE);
    g_chn = std::make_shared<chn_type>("OS04A10", "H264_CBR", 0);
    g_chn->start(1920, 1080, 30, 4096);
    // ... rest of code unchanged ...
    g_chn->stop();
    chn_type::release();
    return 0;
}
```

### Example 3: Feature Management

All existing feature methods work identically:

```cpp
auto camera = std::make_shared<chn_wrapper>("OS08A20", "H265_AVBR", 0);
camera->start(2688, 1520, 30, 6000);

// AIISP
if (aiisp_enabled) {
    camera->aiisp_start("/opt/ceanic/aiisp/model/aibnr.bin", 0);
}

// YOLOv5
if (yolov5_enabled) {
    camera->yolov5_start("/opt/ceanic/yolov5/model.om");
}

// Video Output
if (vo_enabled) {
    camera->vo_start("BT1120", "1080P60");
}

// MP4 Recording
if (mp4_enabled) {
    camera->start_save("/mnt/recording.mp4");
}

// JPEG Snapshot
camera->trigger_jpg("/tmp/snapshot.jpg", 90, "测试快照");

// Cleanup automatically handles all features
camera->stop();
```

### Example 4: Advanced - Accessing New Architecture

For advanced use cases, you can access the underlying camera_instance:

```cpp
auto camera = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
camera->start(1920, 1080, 30, 4096);

// Get underlying camera instance (new architecture)
auto camera_instance = camera->get_camera_instance();
if (camera_instance) {
    // Use new API for advanced features
    auto status = camera_instance->get_status();
    std::cout << "Camera " << status.camera_id << std::endl;
    std::cout << "Streams: " << status.stream_count << std::endl;
    
    // Can enable additional features via new API
    camera_instance->enable_feature("custom_feature", "config");
} else {
    // Wrapper is using legacy implementation
    std::cout << "Using legacy mode" << std::endl;
}
```

## Migration Strategies

### Strategy 1: Immediate (Recommended for Testing)

1. Replace `dev_chn` with `dev_chn_wrapper` in existing code
2. Test thoroughly to ensure backward compatibility
3. Deploy and monitor in production
4. Once stable, proceed to Strategy 2

**Pros:**
- Minimal code changes
- Low risk
- Can be done incrementally

**Cons:**
- Still using old API style
- Not taking advantage of new features

### Strategy 2: Gradual Refactoring

1. Start with Strategy 1 (use wrapper)
2. Identify isolated components
3. Refactor those components to use `camera_manager` directly
4. Test each refactored component
5. Gradually expand to more components

**Pros:**
- Controlled, incremental migration
- Can be done over multiple releases
- Lower risk than full rewrite

**Cons:**
- Temporary code duplication
- Longer migration period

### Strategy 3: Complete Rewrite (For New Code)

For completely new features or applications:

```cpp
#include "camera_manager.h"
#include "camera_instance.h"

using namespace hisilicon::device;

int main() {
    // Initialize managers
    resource_limits limits;
    limits.max_vi_devices = 4;
    resource_manager::init(limits);
    camera_manager::init(4);
    
    // Create camera configuration
    camera_config config;
    config.sensor = sensor_config("OS04A10", "liner");
    
    // Configure streams
    stream_config main_stream;
    main_stream.type = encoder_type::H264_CBR;
    main_stream.width = 1920;
    main_stream.height = 1080;
    main_stream.framerate = 30;
    main_stream.bitrate = 4096;
    config.streams.push_back(main_stream);
    
    // Create and start camera
    auto camera = camera_manager::create_camera(config);
    camera->start();
    
    // Multi-camera support
    camera_config config2;
    config2.sensor = sensor_config("OS08A20", "liner");
    config2.streams.push_back(main_stream);
    auto camera2 = camera_manager::create_camera(config2);
    camera2->start();
    
    // Cleanup
    camera_manager::release();
    resource_manager::release();
    
    return 0;
}
```

## Fallback Behavior

The wrapper automatically falls back to legacy implementation if:

1. `camera_manager` is not initialized via `chn_wrapper::init()`
2. Camera creation fails in new architecture
3. Hardware resources are exhausted

This ensures that the code continues to work even if the new architecture has issues.

## Testing Recommendations

### Unit Tests

```cpp
void test_backward_compatibility() {
    // Test 1: Basic operations
    chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE);
    auto camera = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
    assert(camera->start(1920, 1080, 30, 4096));
    assert(camera->is_start());
    camera->stop();
    chn_wrapper::release();
    
    // Test 2: Feature compatibility
    chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE);
    auto camera2 = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
    camera2->start(1920, 1080, 30, 4096);
    camera2->aiisp_start("/path/to/model", 0);
    camera2->yolov5_start("/path/to/yolo");
    camera2->stop();
    chn_wrapper::release();
}
```

### Integration Tests

1. **RTSP Streaming**: Verify streams work with VLC
2. **RTMP Push**: Verify push to nginx server
3. **Recording**: Verify MP4 files are created correctly
4. **Snapshot**: Verify JPEG snapshots are captured
5. **Features**: Test OSD, AIISP, YOLOv5, VO independently

### Performance Tests

Compare wrapper performance vs legacy:

```cpp
// Measure startup time
auto start = std::chrono::steady_clock::now();
camera->start(1920, 1080, 30, 4096);
auto end = std::chrono::steady_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Startup time: " << duration.count() << "ms" << std::endl;

// Measure frame rate
// Measure CPU usage
// Measure memory usage
```

## Troubleshooting

### Issue: Camera fails to start

**Symptoms:**
```cpp
camera->start(...) returns false
```

**Solutions:**
1. Check if `chn_wrapper::init()` was called
2. Verify sensor name is correct ("OS04A10", "OS08A20", etc.)
3. Check encoder mode is valid ("H264_CBR", "H265_AVBR", etc.)
4. Verify resolution/framerate are within sensor limits
5. Check logs for detailed error messages

### Issue: Features not working

**Symptoms:**
```cpp
camera->aiisp_start(...) returns false
```

**Solutions:**
1. Verify model file path is correct
2. Check if running in stub mode (features are placeholders)
3. Ensure hardware supports the feature
4. Check for resource conflicts (AIISP and YOLOv5 share AI core)

### Issue: Performance degradation

**Symptoms:**
- Lower frame rate
- Higher CPU usage
- Increased latency

**Solutions:**
1. Verify using online mode if possible
2. Check if falling back to legacy (legacy is optimized)
3. Profile to identify bottlenecks
4. May be initial overhead (new architecture is being developed)

### Issue: Memory leaks

**Symptoms:**
- Memory usage increases over time

**Solutions:**
1. Ensure `stop()` is called before destroying camera
2. Ensure `release()` is called at shutdown
3. Check for circular references in observers
4. Use valgrind or similar tools to identify leaks

## Best Practices

1. **Always call init/release**
   ```cpp
   chn_wrapper::init(mode);
   // ... use cameras ...
   chn_wrapper::release();
   ```

2. **Use RAII pattern**
   ```cpp
   {
       auto camera = std::make_shared<chn_wrapper>(...);
       camera->start(...);
       // Automatic cleanup when scope exits
   }
   ```

3. **Check return values**
   ```cpp
   if (!camera->start(...)) {
       // Handle error
       LOG_ERROR("Failed to start camera");
       return -1;
   }
   ```

4. **Stop before destroying**
   ```cpp
   camera->stop();
   camera.reset();  // or let it go out of scope
   ```

5. **Use appropriate VI/VPSS mode**
   - `OT_VI_ONLINE_VPSS_ONLINE`: Best performance
   - `OT_VI_ONLINE_VPSS_OFFLINE`: For JPEG capture
   - `OT_VI_OFFLINE_VPSS_OFFLINE`: For AIISP

## Frequently Asked Questions

**Q: Do I need to change my existing code?**
A: Only the include and type name. The API remains the same.

**Q: Will performance be affected?**
A: Minimal impact. The wrapper adds a thin delegation layer.

**Q: Can I use multiple cameras with the wrapper?**
A: Yes, but currently limited by hardware (4 cameras max).

**Q: What if the new architecture has bugs?**
A: The wrapper automatically falls back to legacy implementation.

**Q: When should I migrate to camera_manager directly?**
A: When you need multi-camera support or dynamic stream management.

**Q: Is the wrapper thread-safe?**
A: Yes, all operations are protected with mutexes.

## Support

For issues or questions:
- Check documentation: `/cn_analyst/device/README.md`
- Review examples: `/unit_tests/test_dev_chn_wrapper.cpp`
- Check logs: `/opt/ceanic/log/`
- Contact: jiajun.ma@ceanic.com

## Related Documentation

- [Device Module Refactoring Plan](../cn_analyst/device/README.md)
- [Integration Guide](../cn_analyst/device/INTEGRATION_GUIDE.md)
- [Implementation Summary](../cn_analyst/device/IMPLEMENTATION_SUMMARY.md)
- [Test Examples](../unit_tests/test_dev_chn_wrapper.cpp)
