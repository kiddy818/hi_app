# Refactoring Roadmap for Multi-Camera Support

## Overview

This document provides a prioritized, actionable roadmap for refactoring the codebase to support multiple cameras and multi-stream configurations.

---

## Phase 1: Foundation & Infrastructure (Weeks 1-4)

### Goal: Establish core abstractions without breaking existing functionality

#### Week 1: Analysis & Design

**Tasks:**
1. ✅ Complete architecture analysis
2. ✅ Document current limitations
3. ✅ Design new architecture
4. [ ] Review with team
5. [ ] Finalize API contracts

**Deliverables:**
- Architecture documentation (ANALYSIS.md)
- API specifications
- Resource allocation strategy

#### Week 2: Resource Manager

**Tasks:**
1. Create `device/resource_manager.h/cpp`
2. Implement VPSS group allocation/tracking
3. Implement VENC channel allocation/tracking
4. Add resource limit validation
5. Write unit tests

**Files to Create:**
- `device/resource_manager.h`
- `device/resource_manager.cpp`
- `device/resource_manager_test.cpp`

**Code Snippet:**
```cpp
class resource_manager {
public:
    static bool init(const system_config& cfg);
    static bool allocate_vpss_group(int32_t& grp);
    static void free_vpss_group(int32_t grp);
    static bool allocate_venc_channel(int32_t& chn);
    static void free_venc_channel(int32_t chn);
    static resource_status get_status();
private:
    static std::map<int32_t, bool> m_vpss_allocated;
    static std::map<int32_t, bool> m_venc_allocated;
    static std::mutex m_mutex;
};
```

**Testing:**
- Test allocation and deallocation
- Test exhaustion scenarios
- Test concurrent access

**Acceptance Criteria:**
- [ ] All resources tracked correctly
- [ ] Cannot over-allocate
- [ ] Thread-safe operations
- [ ] Unit tests pass

#### Week 3: Camera Manager

**Tasks:**
1. Create `device/camera_manager.h/cpp`
2. Create `device/camera_instance.h/cpp`
3. Refactor `dev_chn` into `camera_instance`
4. Implement camera registry
5. Add lifecycle management

**Files to Create:**
- `device/camera_manager.h/cpp`
- `device/camera_instance.h/cpp`

**Files to Modify:**
- `device/dev_chn.h/cpp` (deprecate, wrap in camera_instance)
- `main.cpp` (use camera_manager)

**Code Snippet:**
```cpp
class camera_instance {
public:
    camera_instance(int32_t camera_id, const camera_config& cfg);
    ~camera_instance();
    
    bool start();
    void stop();
    bool is_running() const;
    
    int32_t camera_id() const { return m_camera_id; }
    bool create_stream(const stream_config& cfg);
    bool destroy_stream(int32_t stream_id);
    
private:
    int32_t m_camera_id;
    std::shared_ptr<vi> m_vi;
    ot_vpss_grp m_vpss_grp;
    std::map<int32_t, std::shared_ptr<venc>> m_encoders;
    allocated_resources m_resources;
};

class camera_manager {
public:
    static bool init(int32_t max_cameras);
    static std::shared_ptr<camera_instance> create_camera(const camera_config& cfg);
    static bool destroy_camera(int32_t camera_id);
    static std::shared_ptr<camera_instance> get_camera(int32_t camera_id);
    static std::vector<int32_t> list_cameras();
    
private:
    static std::map<int32_t, std::shared_ptr<camera_instance>> m_cameras;
    static std::mutex m_mutex;
    static int32_t m_max_cameras;
};
```

**Testing:**
- Create/destroy single camera
- Create multiple cameras
- Resource cleanup on destroy
- Thread safety

**Acceptance Criteria:**
- [ ] Camera lifecycle managed
- [ ] Resources properly allocated/freed
- [ ] Backward compatible with single camera
- [ ] Integration tests pass

#### Week 4: Stream Routing

**Tasks:**
1. Create `streaming/stream_router.h/cpp`
2. Create `streaming/stream_instance.h/cpp`
3. Refactor `rtsp/stream/stream_manager` to use router
4. Update RTSP URL parsing

**Files to Create:**
- `streaming/stream_router.h/cpp`
- `streaming/stream_instance.h/cpp`

**Files to Modify:**
- `rtsp/stream/stream_manager.cpp`
- `rtsp/rtsp_request_handler.cpp`

**Code Snippet:**
```cpp
class stream_instance {
public:
    stream_instance(int32_t camera_id, int32_t stream_id, const stream_config& cfg);
    
    std::pair<int32_t, int32_t> get_key() const;
    std::shared_ptr<stream_stock> get_rtsp_stock();
    void add_rtmp_session(const std::string& url);
    
private:
    int32_t m_camera_id;
    int32_t m_stream_id;
    std::string m_stream_name;
    std::shared_ptr<venc> m_encoder;
    std::shared_ptr<stream_stock> m_rtsp_stock;
    std::vector<std::shared_ptr<rtmp::session>> m_rtmp_sessions;
};

class stream_router {
public:
    bool register_stream(std::shared_ptr<stream_instance> stream);
    bool unregister_stream(int32_t camera_id, int32_t stream_id);
    std::shared_ptr<stream_instance> get_stream(int32_t camera_id, int32_t stream_id);
    std::shared_ptr<stream_instance> get_stream_by_url(const std::string& url_path);
    
private:
    std::map<std::pair<int32_t, int32_t>, std::shared_ptr<stream_instance>> m_streams;
    std::map<std::string, std::pair<int32_t, int32_t>> m_url_mapping;
    std::mutex m_mutex;
};
```

**URL Parsing:**
```
/camera/0/main  → camera_id=0, stream_name="main"
/camera/1/sub   → camera_id=1, stream_name="sub"
/stream1        → camera_id=0, stream_id=0 (legacy)
```

**Acceptance Criteria:**
- [ ] Dynamic stream registration works
- [ ] URL routing correct
- [ ] Backward compatible URLs
- [ ] RTSP clients can connect

---

## Phase 2: Multi-Camera Implementation (Weeks 5-8)

### Goal: Enable multiple cameras to work simultaneously

#### Week 5: Remove MAX_CHANNEL Limitation

**Tasks:**
1. Change `MAX_CHANNEL` from constant to variable
2. Update all array accesses to use dynamic allocation
3. Refactor `g_chn` global to use camera_manager
4. Update configuration loading

**Files to Modify:**
- `device/dev_chn.h` (remove `#define MAX_CHANNEL 1`)
- `main.cpp` (refactor initialization)

**Before:**
```cpp
#define MAX_CHANNEL 1
static std::shared_ptr<chn> g_chns[MAX_CHANNEL];
std::shared_ptr<chn> g_chn;
```

**After:**
```cpp
// In system config
int32_t max_cameras = 4;  // From configuration
// Use camera_manager instead
```

**Acceptance Criteria:**
- [ ] No hardcoded MAX_CHANNEL
- [ ] Support 2+ cameras in config
- [ ] All tests pass with new structure

#### Week 6: Dual Camera Configuration

**Tasks:**
1. Design new configuration schema
2. Implement config parser for multi-camera
3. Create migration tool for old configs
4. Add validation logic

**New Configuration Format:**
```json
{
  "system": {
    "max_cameras": 2,
    "log_level": "INFO"
  },
  "cameras": [
    {
      "camera_id": 0,
      "enabled": true,
      "sensor": {
        "name": "OS04A10",
        "mode": "liner"
      },
      "streams": [
        {
          "stream_id": 0,
          "name": "main",
          "encoder": {
            "type": "H264_CBR",
            "width": 1920,
            "height": 1080,
            "framerate": 30,
            "bitrate": 4000
          },
          "outputs": {
            "rtsp": {
              "enabled": true,
              "url_path": "/camera/0/main"
            },
            "rtmp": {
              "enabled": true,
              "urls": ["rtmp://server/live/cam0_main"]
            },
            "mp4": {
              "enabled": false
            }
          }
        }
      ],
      "features": {
        "osd": {
          "enabled": true,
          "position": "top_left"
        },
        "scene_auto": {
          "enabled": true,
          "mode": 0
        }
      }
    },
    {
      "camera_id": 1,
      "enabled": true,
      "sensor": {
        "name": "OS08A20",
        "mode": "liner"
      },
      "streams": [ ... ]
    }
  ]
}
```

**Files to Create:**
- `config/camera_config.h/cpp`
- `config/config_parser.h/cpp`
- `config/config_validator.h/cpp`
- `tools/migrate_config.py`

**Acceptance Criteria:**
- [ ] Parse multi-camera config
- [ ] Validate before loading
- [ ] Migration tool works
- [ ] Backward compatible

#### Week 7: Multi-VENC Capture Thread

**Tasks:**
1. Refactor static VENC capture thread
2. Support multiple cameras in select loop
3. Add per-camera VENC tracking
4. Test concurrent encoding

**Files to Modify:**
- `device/dev_venc.cpp`

**Current Design:**
```cpp
static std::list<venc_ptr> g_vencs;
static std::thread g_capture_thread;
```

**New Design Option 1 (Single Thread):**
```cpp
// Keep single thread, improve select loop
class venc_capture_manager {
    static std::map<int32_t, std::vector<venc_ptr>> m_camera_vencs;
    static void on_capturing();  // Handle all cameras
};
```

**New Design Option 2 (Thread per Camera):**
```cpp
class camera_venc_capture {
    std::vector<venc_ptr> m_vencs;
    std::thread m_thread;
    void on_capturing();  // Handle one camera
};
```

**Recommendation:** Option 1 for simplicity, Option 2 for isolation

**Acceptance Criteria:**
- [ ] All cameras encode simultaneously
- [ ] No frame drops
- [ ] CPU usage acceptable
- [ ] Resource cleanup correct

#### Week 8: Integration Testing

**Tasks:**
1. Test 2-camera setup end-to-end
2. Verify RTSP access to all streams
3. Test RTMP with multiple cameras
4. Performance benchmarking

**Test Scenarios:**
- 2 cameras, 2 streams each (4 total)
- Multiple RTSP clients per stream
- RTMP + RTSP simultaneously
- Camera start/stop/restart
- Resource exhaustion handling

**Performance Targets:**
- Frame rate: 30fps stable per camera
- Latency: <100ms RTSP, <1s RTMP
- CPU: <80% on quad-core
- Memory: <512MB

**Acceptance Criteria:**
- [ ] All test scenarios pass
- [ ] Performance targets met
- [ ] No memory leaks
- [ ] Stable 24-hour run

---

## Phase 3: Advanced Features (Weeks 9-12)

### Goal: Add feature plugins and dynamic configuration

#### Week 9: Feature Plugin System

**Tasks:**
1. Design plugin interface
2. Refactor OSD as plugin
3. Refactor AIISP as plugin
4. Refactor YOLOv5 as plugin

**Plugin Interface:**
```cpp
class feature_plugin {
public:
    virtual ~feature_plugin() = default;
    
    virtual bool init(const json& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;
    
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
};

class camera_feature_manager {
    std::map<std::string, std::shared_ptr<feature_plugin>> m_plugins;
    
    bool load_plugin(const std::string& name, const json& config);
    bool unload_plugin(const std::string& name);
    std::shared_ptr<feature_plugin> get_plugin(const std::string& name);
};
```

**Plugins to Create:**
- `features/osd/osd_plugin.h/cpp`
- `features/aiisp/aiisp_plugin.h/cpp`
- `features/yolov5/yolov5_plugin.h/cpp`

**Acceptance Criteria:**
- [ ] Plugins load/unload dynamically
- [ ] Per-camera plugin configuration
- [ ] Backward compatible
- [ ] All existing features work

#### Week 10: Dynamic Reconfiguration

**Tasks:**
1. Add configuration reload support
2. Implement safe camera restart
3. Add RTSP client notification
4. Test configuration changes

**API:**
```cpp
class camera_instance {
    bool reload_config(const camera_config& new_config);
    bool restart_stream(int32_t stream_id, const stream_config& new_cfg);
};
```

**Workflow:**
1. Client requests config change
2. Validate new configuration
3. Stop affected streams
4. Apply new configuration
5. Restart streams
6. Notify RTSP clients (ANNOUNCE)

**Acceptance Criteria:**
- [ ] Encoder settings changeable
- [ ] Stream resolution changeable
- [ ] RTSP clients reconnect
- [ ] No service interruption for other cameras

#### Week 11: RESTful API (Optional)

**Tasks:**
1. Add HTTP server (e.g., cpp-httplib)
2. Implement REST endpoints
3. Add authentication
4. Create API documentation

**Endpoints:**
```
GET    /api/cameras
GET    /api/cameras/{id}
POST   /api/cameras
DELETE /api/cameras/{id}
PUT    /api/cameras/{id}

GET    /api/cameras/{id}/streams
POST   /api/cameras/{id}/streams
DELETE /api/cameras/{id}/streams/{stream_id}

GET    /api/system/resources
GET    /api/system/status
POST   /api/system/reload
```

**Example:**
```bash
# List cameras
curl http://device/api/cameras

# Create stream
curl -X POST http://device/api/cameras/0/streams \
  -H "Content-Type: application/json" \
  -d '{"name": "new_stream", "encoder": {...}}'
```

**Acceptance Criteria:**
- [ ] All endpoints functional
- [ ] Authentication working
- [ ] API documentation complete
- [ ] Swagger/OpenAPI spec

#### Week 12: Documentation & Training

**Tasks:**
1. Write user manual
2. Create configuration guide
3. Record video tutorials
4. Conduct team training

**Documents to Create:**
- USER_MANUAL.md
- CONFIGURATION_GUIDE.md
- API_REFERENCE.md
- TROUBLESHOOTING.md
- MIGRATION_GUIDE.md

**Training Topics:**
- New architecture overview
- Multi-camera configuration
- REST API usage
- Troubleshooting common issues
- Performance tuning

**Acceptance Criteria:**
- [ ] All documentation complete
- [ ] Team trained
- [ ] Knowledge base updated
- [ ] Support tickets prepared

---

## Phase 4: Optimization & Hardening (Weeks 13-16)

### Goal: Performance optimization and production readiness

#### Week 13: Performance Profiling

**Tasks:**
1. Profile CPU usage with perf/gprof
2. Identify bottlenecks
3. Optimize hot paths
4. Memory leak detection

**Tools:**
- perf (Linux performance counter)
- valgrind (memory profiler)
- gprof (call graph profiler)
- oprofile

**Focus Areas:**
- VENC capture loop
- Stream distribution
- RTSP session handling
- Memory allocations

**Acceptance Criteria:**
- [ ] Profiling data collected
- [ ] Bottlenecks identified
- [ ] Optimization targets set
- [ ] Baseline performance documented

#### Week 14: Code Optimization

**Tasks:**
1. Implement zero-copy optimizations
2. Reduce memory allocations
3. Improve thread synchronization
4. Cache optimization

**Optimizations:**
- Use shared_ptr for stream data (avoid memcpy)
- Pool allocations for RTSP/RTP packets
- Lock-free queues where possible
- Aligned memory for SIMD

**Example:**
```cpp
// Before: Copy data for each observer
for (auto& observer : observers) {
    char* buf = new char[len];
    memcpy(buf, data, len);
    observer->on_stream_come(buf, len);
    delete[] buf;
}

// After: Share data with reference counting
auto shared_data = std::make_shared<std::vector<char>>(data, data + len);
for (auto& observer : observers) {
    observer->on_stream_come(shared_data);
}
```

**Acceptance Criteria:**
- [ ] 20% CPU reduction
- [ ] 30% memory reduction
- [ ] No performance regressions
- [ ] Benchmarks improved

#### Week 15: Stress Testing

**Tasks:**
1. Load testing (4 cameras, max streams)
2. Endurance testing (7-day continuous)
3. Fault injection testing
4. Recovery testing

**Test Scenarios:**
- Maximum concurrent RTSP clients
- Network interruptions (RTMP)
- Sensor disconnection/reconnection
- Configuration errors
- Resource exhaustion
- Thermal throttling

**Metrics to Monitor:**
- Frame drops
- Latency spikes
- Memory leaks
- Error rates
- Recovery time

**Acceptance Criteria:**
- [ ] 7-day continuous operation
- [ ] <0.1% frame drop rate
- [ ] Automatic recovery from faults
- [ ] No crashes or hangs

#### Week 16: Final Testing & Release Prep

**Tasks:**
1. Regression testing (all features)
2. Documentation review
3. Code review
4. Release notes
5. Deployment planning

**Regression Test Suite:**
- Single camera (legacy)
- Dual camera
- Four camera
- All encoding modes
- All features (OSD, AIISP, YOLOv5)
- RTSP + RTMP + MP4
- Configuration migration

**Release Checklist:**
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Performance targets met
- [ ] Documentation complete
- [ ] Code reviewed
- [ ] Release notes written
- [ ] Deployment guide ready
- [ ] Rollback plan prepared

---

## Success Metrics

### Functional Requirements
- [ ] Support 4 cameras simultaneously
- [ ] 2-4 streams per camera
- [ ] Dynamic camera/stream creation
- [ ] RTSP multi-camera URLs
- [ ] RTMP multi-camera support
- [ ] Configuration reload
- [ ] Feature plugins working

### Performance Requirements
- [ ] 30fps per camera (1080p)
- [ ] <100ms RTSP latency
- [ ] <80% CPU usage (4-core)
- [ ] <512MB memory usage
- [ ] 99.9% uptime

### Quality Requirements
- [ ] Zero known critical bugs
- [ ] <0.1% frame drop rate
- [ ] Automatic error recovery
- [ ] Graceful degradation
- [ ] Complete test coverage

---

## Risk Mitigation

### Technical Risks

**Risk: Performance degradation**
- **Mitigation:** Early profiling, optimization phase
- **Fallback:** Thread-per-camera design

**Risk: Resource exhaustion**
- **Mitigation:** Resource manager with limits
- **Fallback:** Graceful degradation, reject new cameras

**Risk: Breaking changes**
- **Mitigation:** Backward compatibility, migration tools
- **Fallback:** Maintain old API alongside new

### Project Risks

**Risk: Timeline delays**
- **Mitigation:** Phased delivery, MVP approach
- **Fallback:** Reduce scope, focus on core features

**Risk: Hardware limitations**
- **Mitigation:** Early testing on target hardware
- **Fallback:** Reduce max cameras, optimize performance

---

## Rollout Strategy

### Stage 1: Internal Testing (Week 13)
- Deploy to development environment
- Team testing with 2 cameras
- Identify issues, iterate

### Stage 2: Beta Testing (Week 14-15)
- Deploy to beta test environment
- Selected users test 4-camera setup
- Collect feedback, fix issues

### Stage 3: Gradual Rollout (Week 16)
- Deploy to 10% of production
- Monitor metrics closely
- Increase to 50%, then 100%

### Stage 4: Full Production (Week 17+)
- All systems using new code
- Monitor for issues
- Iterative improvements

---

## Maintenance Plan

### Ongoing Tasks
- Monitor performance metrics
- Address bug reports
- Security updates
- Feature enhancements

### Support
- Dedicated support channel
- Known issues tracker
- Regular status updates
- Performance reports

---

## Appendix: Quick Reference

### File Changes Summary

**New Files:**
- `device/resource_manager.{h,cpp}`
- `device/camera_manager.{h,cpp}`
- `device/camera_instance.{h,cpp}`
- `streaming/stream_router.{h,cpp}`
- `streaming/stream_instance.{h,cpp}`
- `config/camera_config.{h,cpp}`
- `features/plugin_interface.h`
- `features/osd/osd_plugin.{h,cpp}`
- `cn_analyst/ANALYSIS.md`
- `cn_analyst/REFACTORING_ROADMAP.md`

**Modified Files:**
- `device/dev_chn.{h,cpp}` (deprecated → camera_instance)
- `device/dev_venc.cpp` (multi-camera capture)
- `rtsp/stream/stream_manager.cpp` (use stream_router)
- `rtsp/rtsp_request_handler.cpp` (new URL parsing)
- `rtmp/session_manager.cpp` (URL templates)
- `main.cpp` (use camera_manager)

**Deprecated Files:**
- None (maintain backward compatibility)

### Configuration Migration

**Old:**
```
/opt/ceanic/etc/vi.json
/opt/ceanic/etc/venc.json
/opt/ceanic/etc/net_service.json
```

**New:**
```
/opt/ceanic/etc/system_config.json (unified)
```

**Migration Command:**
```bash
./tools/migrate_config.py \
  --old-config /opt/ceanic/etc/ \
  --new-config /opt/ceanic/etc/system_config.json
```

---

## Document Version

- **Version:** 1.0
- **Date:** 2026-01-04
- **Author:** Code Analysis System
- **Status:** Approved for Implementation

**End of Refactoring Roadmap**
