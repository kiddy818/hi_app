# Device Module Refactoring

This directory will contain the refactored device module implementation for multi-camera support.

## Status

**Current Status:** Unpopulated - Awaiting Implementation  
**Target Completion:** Phase 1-2 (Weeks 1-8)

## Planned Components

### Core Classes

#### `camera_manager.h/cpp`
- **Purpose:** Central registry and factory for camera instances
- **Responsibilities:**
  - Create/destroy camera instances
  - Track active cameras
  - Enforce camera limits
  - Coordinate with resource_manager
- **Status:** Not yet implemented
- **Priority:** Critical (Phase 1, Week 3)

#### `camera_instance.h/cpp`
- **Purpose:** Represent a single camera with all its components
- **Responsibilities:**
  - Manage VI (video input) lifecycle
  - Manage VPSS (video processing) pipeline
  - Manage VENC (encoders) for multiple streams
  - Coordinate features (OSD, AIISP, etc.)
  - Implement observer pattern for stream distribution
- **Status:** Not yet implemented
- **Priority:** Critical (Phase 1, Week 3)

#### `resource_manager.h/cpp`
- **Purpose:** Track and allocate hardware resources
- **Responsibilities:**
  - VPSS group allocation (32 groups available)
  - VENC channel allocation (16 channels available)
  - VI device allocation (4 devices available)
  - VB pool management
  - Resource limit enforcement
- **Status:** Not yet implemented
- **Priority:** Critical (Phase 1, Week 2)

#### `stream_config.h/cpp`
- **Purpose:** Stream configuration and validation
- **Responsibilities:**
  - Parse stream configuration
  - Validate resolution/framerate/bitrate
  - Encoder parameter validation
- **Status:** Not yet implemented
- **Priority:** High (Phase 1, Week 3)

### Modified Classes

#### `dev_chn` (Existing) → `camera_instance` (New)
- **Changes Required:**
  - Remove static `g_chns[]` array
  - Remove `MAX_CHANNEL = 1` limitation
  - Add camera_id member
  - Use resource_manager for allocation
  - Isolate per-camera state
- **Backward Compatibility:** Wrapper class for smooth migration
- **Status:** Pending refactoring (Phase 2, Week 5)

#### `dev_venc` (Existing)
- **Changes Required:**
  - Remove static `g_vencs` list
  - Support per-camera VENC capture thread (or improved single thread)
  - Dynamic VENC allocation through resource_manager
- **Backward Compatibility:** Yes
- **Status:** Pending refactoring (Phase 2, Week 7)

#### `dev_vi` (Existing)
- **Changes Required:**
  - Support multiple VI instances
  - Dynamic VI device allocation
  - Per-camera ISP pipeline
- **Backward Compatibility:** Yes
- **Status:** Pending refactoring (Phase 2, Week 5-6)

## Hardware Resource Constraints

### HiSilicon 3519DV500 Limits
```
Resource              Total Available    Per Camera Max    Typical Usage
--------------------------------------------------------------------------------
VI Devices            4                  1                 1 per camera
VPSS Groups           32                 1-2               1 per camera
VPSS Channels         128 (4 per grp)    2-4               2-3 per camera
VENC Channels         16 total           4-8               2-3 per camera
  ├─ H.265            4-8 channels       2-4               1-2 per camera
  └─ H.264            8-12 channels      2-6               1-2 per camera
ISP Pipelines         4                  1                 1 per camera (max 4 cameras)
SVP (NNIE) Core       1 (shared)         N/A               Shared for AI
VB Memory             256-512 MB         64-128 MB         Varies by resolution
--------------------------------------------------------------------------------
Practical Limit:      4 cameras max
Recommended:          2-4 cameras with 2-4 streams each
```

## Resource Manager Design

### Interface
```cpp
class resource_manager {
public:
    // Initialization
    static bool init(const resource_limits& limits);
    static void release();
    
    // VPSS Group Management
    static bool allocate_vpss_group(int32_t& grp);
    static void free_vpss_group(int32_t grp);
    static bool is_vpss_group_available();
    
    // VENC Channel Management
    static bool allocate_venc_channel(venc_type type, int32_t& chn);
    static void free_venc_channel(int32_t chn);
    static bool is_venc_channel_available(venc_type type);
    
    // VI Device Management
    static bool allocate_vi_device(int32_t& dev);
    static void free_vi_device(int32_t dev);
    static bool is_vi_device_available();
    
    // Query Functions
    static resource_status get_status();
    static bool can_create_camera(const camera_config& cfg);
    
private:
    static std::map<int32_t, bool> m_vpss_allocated;
    static std::map<int32_t, bool> m_venc_allocated;
    static std::map<int32_t, bool> m_vi_allocated;
    static std::mutex m_mutex;
    static resource_limits m_limits;
};
```

### Resource Status
```cpp
struct resource_status {
    struct {
        int32_t total;
        int32_t allocated;
        int32_t available;
    } vi_devices, vpss_groups, venc_h264, venc_h265;
    
    size_t vb_total_size;
    size_t vb_used_size;
    size_t vb_free_size;
};
```

## Camera Manager Design

### Interface
```cpp
class camera_manager {
public:
    // Initialization
    static bool init(int32_t max_cameras);
    static void release();
    
    // Camera Lifecycle
    static std::shared_ptr<camera_instance> create_camera(
        const camera_config& cfg
    );
    static bool destroy_camera(int32_t camera_id);
    
    // Query Functions
    static std::shared_ptr<camera_instance> get_camera(int32_t camera_id);
    static std::vector<int32_t> list_cameras();
    static int32_t get_camera_count();
    static bool camera_exists(int32_t camera_id);
    
    // Validation
    static bool validate_config(const camera_config& cfg);
    static bool can_create_camera(const camera_config& cfg);
    
private:
    static std::map<int32_t, std::shared_ptr<camera_instance>> m_cameras;
    static std::mutex m_mutex;
    static int32_t m_max_cameras;
    static int32_t m_next_camera_id;
};
```

## Camera Instance Design

### Interface
```cpp
class camera_instance : public std::enable_shared_from_this<camera_instance>,
                        public ceanic::util::stream_observer {
public:
    camera_instance(int32_t camera_id, const camera_config& cfg);
    ~camera_instance();
    
    // Lifecycle
    bool start();
    void stop();
    bool is_running() const;
    
    // Stream Management
    bool create_stream(const stream_config& cfg);
    bool destroy_stream(int32_t stream_id);
    std::shared_ptr<stream_instance> get_stream(int32_t stream_id);
    std::vector<int32_t> list_streams();
    
    // Feature Management
    bool enable_feature(const std::string& feature_name, const json& config);
    bool disable_feature(const std::string& feature_name);
    
    // Info
    int32_t camera_id() const { return m_camera_id; }
    camera_config get_config() const { return m_config; }
    camera_status get_status() const;
    
    // Observer Pattern (from device to streaming)
    void on_stream_come(stream_obj_ptr obj, stream_head* head, 
                       const char* buf, int32_t len) override;
    void on_stream_error(stream_obj_ptr obj, int32_t error) override;
    
private:
    int32_t m_camera_id;
    camera_config m_config;
    bool m_is_running;
    
    // Hardware components
    std::shared_ptr<vi> m_vi;
    ot_vpss_grp m_vpss_grp;
    std::map<int32_t, std::shared_ptr<venc>> m_encoders;
    
    // Streams
    std::map<int32_t, std::shared_ptr<stream_instance>> m_streams;
    
    // Features
    std::map<std::string, std::shared_ptr<feature_plugin>> m_features;
    
    // Resources
    allocated_resources m_resources;
    
    // Thread safety
    mutable std::mutex m_mutex;
};
```

### Resource Tracking
```cpp
struct allocated_resources {
    int32_t vi_dev;
    ot_vpss_grp vpss_grp;
    std::vector<int32_t> venc_channels;
    std::vector<int32_t> vb_pools;
    
    void clear();
    bool is_allocated() const;
};
```

## Configuration Schema

### Camera Configuration
```cpp
struct camera_config {
    int32_t camera_id;
    bool enabled;
    
    struct {
        std::string name;         // "OS04A10", "OS08A20", etc.
        std::string mode;         // "liner", "2to1wdr"
    } sensor;
    
    std::vector<stream_config> streams;
    
    struct {
        bool osd_enabled;
        bool scene_auto_enabled;
        int32_t scene_mode;
        bool aiisp_enabled;
        std::string aiisp_model;
        bool yolov5_enabled;
        std::string yolov5_model;
        bool vo_enabled;
    } features;
};
```

### Stream Configuration
```cpp
struct stream_config {
    int32_t stream_id;
    std::string name;           // "main", "sub", "ai"
    
    struct {
        encoder_type type;      // H264_CBR, H265_AVBR, etc.
        int32_t width;
        int32_t height;
        int32_t framerate;
        int32_t bitrate;
    } encoder;
    
    struct {
        bool rtsp_enabled;
        std::string rtsp_url_path;
        bool rtmp_enabled;
        std::vector<std::string> rtmp_urls;
        bool mp4_enabled;
        std::string mp4_path;
        bool jpeg_enabled;
        int32_t jpeg_interval;
    } outputs;
};
```

## Refactoring Checklist

### Phase 1: Core Abstractions (Weeks 1-4)

#### Week 2: Resource Manager
- [ ] Design resource_manager interface
- [ ] Implement VPSS group tracking
- [ ] Implement VENC channel tracking
- [ ] Implement VI device tracking
- [ ] Add validation logic
- [ ] Write unit tests
- [ ] Integration testing

#### Week 3: Camera Manager & Instance
- [ ] Design camera_manager interface
- [ ] Design camera_instance class
- [ ] Implement camera lifecycle
- [ ] Implement resource allocation
- [ ] Add validation logic
- [ ] Write unit tests
- [ ] Integration testing

#### Week 4: Stream Management
- [ ] Design stream_instance class
- [ ] Integrate with camera_instance
- [ ] Implement stream creation/destruction
- [ ] Connect to RTSP/RTMP routers
- [ ] Write unit tests
- [ ] Integration testing

### Phase 2: Multi-Camera Support (Weeks 5-8)

#### Week 5: Remove MAX_CHANNEL
- [ ] Remove `#define MAX_CHANNEL 1`
- [ ] Replace `g_chns[]` array with camera_manager
- [ ] Update all references
- [ ] Refactor initialization in main.cpp
- [ ] Test with single camera (regression)

#### Week 6: Configuration System
- [ ] Design unified configuration schema
- [ ] Implement config parser
- [ ] Add validation logic
- [ ] Create migration tool
- [ ] Test configuration loading

#### Week 7: Multi-VENC Capture
- [ ] Refactor VENC capture thread
- [ ] Support multiple cameras
- [ ] Test concurrent encoding
- [ ] Performance optimization

#### Week 8: Integration Testing
- [ ] Test 2-camera setup
- [ ] Test 4-camera setup
- [ ] Performance benchmarking
- [ ] Stress testing
- [ ] Documentation

## Testing Strategy

### Unit Tests

**resource_manager_test.cpp**
- Allocate/free resources
- Exhaustion scenarios
- Concurrent access
- Validation logic

**camera_manager_test.cpp**
- Create/destroy cameras
- Camera lookup
- Limit enforcement
- Thread safety

**camera_instance_test.cpp**
- Lifecycle (start/stop)
- Stream management
- Feature management
- Observer pattern

### Integration Tests

**single_camera_test.cpp** (Regression)
- All sensors supported
- All encoding modes
- All features
- Verify backward compatibility

**dual_camera_test.cpp**
- Two cameras simultaneously
- Independent configuration
- Concurrent encoding
- Resource isolation

**multi_camera_test.cpp**
- Four cameras (max limit)
- Resource exhaustion handling
- Performance under load

### Performance Tests

**Metrics:**
- Frame rate stability (30fps target)
- Encoding latency (<50ms)
- CPU usage (<80% on quad-core)
- Memory usage (<512MB total)
- Startup time (<5 seconds per camera)

**Test Scenarios:**
- 1 camera, 4 streams
- 2 cameras, 2 streams each
- 4 cameras, 2 streams each
- All features enabled

## Migration Path

### Step 1: Wrapper Layer (Week 5)
Create compatibility wrapper for existing code:
```cpp
// dev_chn.h (deprecated)
class chn {
    std::shared_ptr<camera_instance> m_impl;
public:
    chn(const char* sensor, const char* mode, int chn_no);
    bool start(...);
    void stop();
    // ... delegate to camera_instance
};
```

### Step 2: Update main.cpp (Week 5)
```cpp
// Old way
std::shared_ptr<hisilicon::dev::chn> g_chn;
g_chn = std::make_shared<hisilicon::dev::chn>("OS04A10", "H264_CBR", 0);

// New way
camera_manager::init(4);  // Support up to 4 cameras
camera_config cfg = load_camera_config(0);
auto camera = camera_manager::create_camera(cfg);
```

### Step 3: Remove Legacy Code (Week 8+)
Once all tests pass with new implementation:
- Remove `dev_chn.{h,cpp}` wrapper
- Update all references
- Remove compatibility layer

## Known Issues & Limitations

### Current Issues (Pre-Refactoring)
1. `MAX_CHANNEL = 1` hardcoded
2. Static `g_chn` global variable
3. Static `g_chns[]` array
4. No resource tracking
5. No multi-camera support

### Post-Refactoring Improvements
1. ✅ Dynamic camera allocation
2. ✅ Resource manager enforces limits
3. ✅ Per-camera configuration
4. ✅ Independent camera lifecycle
5. ✅ Scalable to hardware limits (4 cameras)

## Performance Considerations

### CPU Usage
- Target: <80% on quad-core ARMv8
- VI/VPSS: Hardware accelerated
- VENC: Hardware accelerated
- ISP: Hardware pipeline
- Software overhead: <10% per camera

### Memory Usage
- Target: <512MB total
- Per camera: ~64-128MB (varies by resolution)
- VB pools: Pre-allocated based on config
- Stream buffers: Shared pointers (minimal copy)

### Latency
- VI capture: <10ms
- VPSS processing: <10ms
- VENC encoding: <30ms
- Total camera to encoded: <50ms

## Hardware Binding

### VI → VPSS → VENC Pipeline
```cpp
// Bind VI to VPSS
ot_mpp_chn src_chn = {OT_ID_VI, vi_dev, vi_chn};
ot_mpp_chn dest_chn = {OT_ID_VPSS, vpss_grp, 0};
ss_mpi_sys_bind(&src_chn, &dest_chn);

// Bind VPSS to VENC
src_chn = {OT_ID_VPSS, vpss_grp, vpss_chn};
dest_chn = {OT_ID_VENC, venc_chn, 0};
ss_mpi_sys_bind(&src_chn, &dest_chn);
```

### Resource Mapping Example
```
Camera 0:
  VI0 → VPSS0 → VENC0 (main), VENC1 (sub)
  
Camera 1:
  VI1 → VPSS1 → VENC2 (main), VENC3 (sub)
  
Camera 2:
  VI2 → VPSS2 → VENC4 (main), VENC5 (sub)
  
Camera 3:
  VI3 → VPSS3 → VENC6 (main), VENC7 (sub)
```

## Future Enhancements

### Phase 4+
- Hot-plug camera support
- Dynamic resolution switching
- Hardware failover (camera swap)
- Advanced ISP tuning per camera
- ROI (Region of Interest) encoding
- Smart encoding (save bandwidth)

## References

### Related Documents
- [Main Analysis](../ANALYSIS.md)
- [Refactoring Roadmap](../REFACTORING_ROADMAP.md)
- [RTSP Refactoring](../rtsp/README.md)
- [RTMP Refactoring](../rtmp/README.md)

### External Resources
- HiSilicon MPP SDK Documentation
- ISP Calibration Guide
- VPSS Configuration Manual
- VENC Optimization Guide

---

**Last Updated:** 2026-01-04  
**Status:** Planning Phase  
**Priority:** Critical Path  
**Next Review:** Week 2 Implementation Kickoff
