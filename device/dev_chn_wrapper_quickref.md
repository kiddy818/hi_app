# dev_chn_wrapper Quick Reference

## 🎯 Purpose
Backward-compatible wrapper that lets existing code use new multi-camera architecture with minimal changes.

## 🚀 Quick Start

### Minimal Migration (3 steps)

```cpp
// 1. Change include
// OLD: #include "dev_chn.h"
#include "dev_chn_wrapper.h"

// 2. Change type
// OLD: using namespace hisilicon::dev;
// OLD: std::shared_ptr<chn> g_chn;
using namespace hisilicon::dev;
std::shared_ptr<chn_wrapper> g_chn;

// 3. Use exactly as before
chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE);
g_chn = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
g_chn->start(1920, 1080, 30, 4096);
// ... everything else stays the same ...
g_chn->stop();
chn_wrapper::release();
```

## 📋 API Reference

### Construction
```cpp
chn_wrapper(const char* vi_name, const char* venc_mode, int chn_no);
```

**Sensor Names:**
- `"OS04A10"` - OmniVision 4MP liner
- `"OS04A10_WDR"` - OmniVision 4MP WDR
- `"OS08A20"` - OmniVision 8MP liner
- `"OS08A20_WDR"` - OmniVision 8MP WDR

**Encoder Modes:**
- `"H264_CBR"`, `"H264_VBR"`, `"H264_AVBR"`
- `"H265_CBR"`, `"H265_VBR"`, `"H265_AVBR"`

### Lifecycle Methods
```cpp
bool start(int w, int h, int framerate, int bitrate);
void stop();
bool is_start();
```

### Features
```cpp
// AIISP
bool aiisp_start(const char* model_file, int mode);
void aiisp_stop();

// YOLOv5
bool yolov5_start(const char* model_file);
void yolov5_stop();

// Video Output
bool vo_start(const char* intf_type, const char* intf_sync);
void vo_stop();

// MP4 Recording
bool start_save(const char* file);
void stop_save();

// JPEG Snapshot
bool trigger_jpg(const char* file, int quality, const char* info);

// ISP Info
bool get_isp_exposure_info(isp_exposure_t* val);
```

### Static Methods
```cpp
// System
static bool init(ot_vi_vpss_mode_type mode);
static void release();
static void start_capture(bool enable);

// Scene
static bool scene_init(const char* dir_path);
static bool scene_set_mode(int mode);
static void scene_release();

// Rate Auto
static bool rate_auto_init(const char* file);
static void rate_auto_release();

// Stream Utils
static bool get_stream_head(int chn, int stream, media_head* mh);
static bool request_i_frame(int chn, int stream);
```

## 🔧 Common Patterns

### Pattern 1: Basic Camera
```cpp
chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE);
auto cam = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
cam->start(1920, 1080, 30, 4096);
// ... use camera ...
cam->stop();
chn_wrapper::release();
```

### Pattern 2: With AIISP
```cpp
chn_wrapper::init(OT_VI_OFFLINE_VPSS_OFFLINE);  // AIISP needs offline mode
auto cam = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
cam->start(1920, 1080, 30, 4096);
cam->aiisp_start("/opt/ceanic/aiisp/model.bin", 0);
// ... use camera ...
cam->aiisp_stop();
cam->stop();
chn_wrapper::release();
```

### Pattern 3: With All Features
```cpp
chn_wrapper::init(OT_VI_OFFLINE_VPSS_OFFLINE);
chn_wrapper::scene_init("/opt/ceanic/scene/param/sensor_os04a10");
chn_wrapper::rate_auto_init("/opt/ceanic/etc/rate_auto.ini");

auto cam = std::make_shared<chn_wrapper>("OS04A10", "H265_AVBR", 0);
cam->start(2688, 1520, 30, 6000);

// Enable features
cam->aiisp_start("/opt/ceanic/aiisp/model.bin", 0);
cam->yolov5_start("/opt/ceanic/yolov5/model.om");
cam->vo_start("BT1120", "1080P60");
cam->start_save("/mnt/recording.mp4");

// ... use camera ...

// Cleanup
cam->stop_save();
cam->vo_stop();
cam->yolov5_stop();
cam->aiisp_stop();
cam->stop();

chn_wrapper::rate_auto_release();
chn_wrapper::scene_release();
chn_wrapper::release();
```

### Pattern 4: Type Alias (Easiest Migration)
```cpp
// At top of file
#ifdef USE_NEW_ARCHITECTURE
    #include "dev_chn_wrapper.h"
    using chn_type = hisilicon::dev::chn_wrapper;
#else
    #include "dev_chn.h"
    using chn_type = hisilicon::dev::chn;
#endif

// Rest of code uses chn_type
std::shared_ptr<chn_type> g_chn;
chn_type::init(mode);
g_chn = std::make_shared<chn_type>("OS04A10", "H264_CBR", 0);
// etc...
```

## ⚙️ VI/VPSS Modes

Choose based on your needs:

| Mode | Use Case | Performance |
|------|----------|-------------|
| `OT_VI_ONLINE_VPSS_ONLINE` | Normal streaming | Best |
| `OT_VI_ONLINE_VPSS_OFFLINE` | JPEG snapshot | Good |
| `OT_VI_OFFLINE_VPSS_OFFLINE` | AIISP/Heavy processing | Slower |

## 🐛 Troubleshooting

### Camera won't start
```cpp
if (!cam->start(...)) {
    // Check:
    // 1. Did you call chn_wrapper::init()?
    // 2. Is sensor name correct?
    // 3. Is resolution within sensor limits?
    // 4. Check logs in /opt/ceanic/log/
}
```

### Feature returns false
```cpp
if (!cam->aiisp_start(...)) {
    // Normal in stub mode - features are placeholders
    // Will work after hardware integration
}
```

### Performance issues
```cpp
// 1. Use online mode if possible
chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE);

// 2. Check if falling back to legacy
auto instance = cam->get_camera_instance();
if (!instance) {
    // Using legacy (good - it's optimized)
}
```

## 📊 Comparison

| Feature | Legacy chn | chn_wrapper |
|---------|-----------|-------------|
| API | Stable | Identical |
| Performance | Optimized | Same* |
| Multi-camera | No | Yes** |
| Migration effort | N/A | Minimal |
| Future proof | No | Yes |

*Minimal overhead, mostly identical  
**After Phase 2 completion

## 📚 More Info

- Full Guide: `device/dev_chn_wrapper_guide.md`
- Tests: `unit_tests/test_dev_chn_wrapper.cpp`
- Implementation: `cn_analyst/device/PHASE2_SUMMARY.md`

## 🤝 Support

Questions? Check:
1. Logs: `/opt/ceanic/log/ceanic_dev.log`
2. Documentation: `/cn_analyst/device/README.md`
3. Contact: jiajun.ma@ceanic.com

---

**Version:** Phase 2 Initial Release  
**Date:** 2026-01-06  
**Status:** Ready for testing
