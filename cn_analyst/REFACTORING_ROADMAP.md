# 多摄像头支持的重构路线图

## 概述

本文档提供优先级行动路线图，用于重构代码库以支持多摄像头和多流配置。

---

## 阶段 1：基础和基础设施（第 1-4 周）

### 目标：建立核心抽象，不破坏现有功能

#### 第 1 周：分析与设计

**任务：**
1. ✅ 完成架构分析
2. ✅ 记录当前限制
3. ✅ 设计新架构
4. [ ] 与团队审查
5. [ ] 最终确定 API 合同

**交付成果：**
- 架构文档（ANALYSIS.md）
- API 规范
- 资源分配策略

#### 第 2 周：资源管理器

**任务：**
1. 创建 `device/resource_manager.h/cpp`
2. 实现 VPSS 组分配/跟踪
3. 实现 VENC 通道分配/跟踪
4. 添加资源限制验证
5. 编写单元测试

**要创建的文件：**
- `device/resource_manager.h`
- `device/resource_manager.cpp`
- `device/resource_manager_test.cpp`

**代码片段：**
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

**测试：**
- 测试分配和释放
- 测试耗尽场景
- 测试并发访问

**验收标准：**
- [ ] 所有资源正确跟踪
- [ ] 无法过度分配
- [ ] 线程安全操作
- [ ] 单元测试通过

#### 第 3 周：摄像头管理器

**任务：**
1. 创建 `device/camera_manager.h/cpp`
2. 创建 `device/camera_instance.h/cpp`
3. 重构 `dev_chn` into `camera_instance`
4. 实现 camera registry
5. 添加 lifecycle management

**要创建的文件：**
- `device/camera_manager.h/cpp`
- `device/camera_instance.h/cpp`

**要修改的文件：**
- `device/dev_chn.h/cpp` (deprecate, wrap in camera_instance)
- `main.cpp` (use camera_manager)

**代码片段：**
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

**测试：**
- 创建/destroy single camera
- 创建 multiple cameras
- Resource cleanup on destroy
- 线程安全

**验收标准：**
- [ ] 摄像头生命周期管理
- [ ] 资源正确分配/释放
- [ ] 与单摄像头向后兼容
- [ ] 集成测试通过

#### 第 4 周：流路由

**任务：**
1. 创建 `streaming/stream_router.h/cpp`
2. 创建 `streaming/stream_instance.h/cpp`
3. 重构 `rtsp/stream/stream_manager` 以使用 router
4. 更新 RTSP URL parsing

**要创建的文件：**
- `streaming/stream_router.h/cpp`
- `streaming/stream_instance.h/cpp`

**要修改的文件：**
- `rtsp/stream/stream_manager.cpp`
- `rtsp/rtsp_request_handler.cpp`

**代码片段：**
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

**验收标准：**
- [ ] 动态流注册工作
- [ ] URL 路由正确
- [ ] 向后兼容的 URL
- [ ] RTSP 客户端可以连接

---

## Phase 2: Multi-Camera 实现ation (Weeks 5-8)

### Goal: Enable multiple cameras to work simultaneously

#### 第 5 周：移除 MAX_CHANNEL 限制

**任务：**
1. Change `MAX_CHANNEL` from constant to variable
2. 更新 all array accesses 以使用 dynamic allocation
3. 重构 `g_chn` global 以使用 camera_manager
4. 更新 configuration loading

**要修改的文件：**
- `device/dev_chn.h` (remove `#define MAX_CHANNEL 1`)
- `main.cpp` (refactor initialization)

**之前：**
```cpp
#define MAX_CHANNEL 1
static std::shared_ptr<chn> g_chns[MAX_CHANNEL];
std::shared_ptr<chn> g_chn;
```

**之后：**
```cpp
// In system config
int32_t max_cameras = 4;  // From configuration
// Use camera_manager instead
```

**验收标准：**
- [ ] 无硬编码的 MAX_CHANNEL
- [ ] 配置中支持 2+ 摄像头
- [ ] 所有测试通过新结构

#### 第 6 周：双摄像头配置

**任务：**
1. 设计 new configuration schema
2. 实现 config parser for multi-camera
3. 创建 migration tool for old configs
4. 添加 validation logic

**新配置格式：**
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

**要创建的文件：**
- `config/camera_config.h/cpp`
- `config/config_parser.h/cpp`
- `config/config_validator.h/cpp`
- `tools/migrate_config.py`

**验收标准：**
- [ ] 解析多摄像头配置
- [ ] 加载前验证
- [ ] Migration tool works
- [ ] 向后兼容

#### 第 7 周：多 VENC 捕获线程

**任务：**
1. 重构 static VENC capture thread
2. Support multiple cameras in select loop
3. 添加 per-camera VENC tracking
4. 测试 concurrent encoding

**要修改的文件：**
- `device/dev_venc.cpp`

**Current 设计:**
```cpp
static std::list<venc_ptr> g_vencs;
static std::thread g_capture_thread;
```

**New 设计 Option 1 (Single Thread):**
```cpp
// Keep single thread, improve select loop
class venc_capture_manager {
    static std::map<int32_t, std::vector<venc_ptr>> m_camera_vencs;
    static void on_capturing();  // Handle all cameras
};
```

**New 设计 Option 2 (Thread per Camera):**
```cpp
class camera_venc_capture {
    std::vector<venc_ptr> m_vencs;
    std::thread m_thread;
    void on_capturing();  // Handle one camera
};
```

**Recommendation:** Option 1 for simplicity, Option 2 for isolation

**验收标准：**
- [ ] 所有摄像头同时编码
- [ ] 无丢帧
- [ ] CPU 使用可接受
- [ ] 资源清理正确

#### 第 8 周：集成测试

**任务：**
1. 测试 2-camera setup end-to-end
2. Verify RTSP access to all streams
3. 测试 RTMP with multiple cameras
4. Performance benchmarking

**测试场景：**
- 2 cameras, 2 streams each (4 total)
- Multiple RTSP clients per stream
- RTMP + RTSP simultaneously
- Camera start/stop/restart
- Resource exhaustion handling

**性能目标：**
- Frame rate: 30fps stable per camera
- Latency: <100ms RTSP, <1s RTMP
- CPU: <80% on quad-core
- Memory: <512MB

**验收标准：**
- [ ] 所有测试场景通过
- [ ] 性能目标达成
- [ ] 无内存泄漏
- [ ] 稳定24小时运行

---

## Phase 3: Advanced Features (Weeks 9-12)

### Goal: 添加 feature plugins and dynamic configuration

#### 第 9 周：特性插件系统

**任务：**
1. 设计 plugin interface
2. 重构 OSD as plugin
3. 重构 AIISP as plugin
4. 重构 YOLOv5 as plugin

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

**Plugins to 创建:**
- `features/osd/osd_plugin.h/cpp`
- `features/aiisp/aiisp_plugin.h/cpp`
- `features/yolov5/yolov5_plugin.h/cpp`

**验收标准：**
- [ ] 插件动态加载/卸载
- [ ] 每摄像头插件配置
- [ ] 向后兼容
- [ ] 所有现有特性工作

#### 第 10 周：动态重新配置

**任务：**
1. 添加 configuration reload support
2. 实现 safe camera restart
3. 添加 RTSP client notification
4. 测试 configuration changes

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

**验收标准：**
- [ ] Encoder settings changeable
- [ ] Stream resolution changeable
- [ ] RTSP clients reconnect
- [ ] No service interruption for other cameras

#### 第 11 周：RESTful API（可选）

**任务：**
1. 添加 HTTP server (e.g., cpp-httplib)
2. 实现 REST endpoints
3. 添加 authentication
4. 创建 API documentation

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

**示例：**
```bash
# List cameras
curl http://device/api/cameras

# 创建 stream
curl -X POST http://device/api/cameras/0/streams \
  -H "Content-Type: application/json" \
  -d '{"name": "new_stream", "encoder": {...}}'
```

**验收标准：**
- [ ] All endpoints functional
- [ ] Authentication working
- [ ] API documentation complete
- [ ] Swagger/OpenAPI spec

#### 第 12 周：文档与培训

**任务：**
1. 编写 user manual
2. 创建 configuration guide
3. Record video tutorials
4. Conduct team training

**文档化s to 创建:**
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

**验收标准：**
- [ ] All documentation complete
- [ ] Team trained
- [ ] Knowledge base updated
- [ ] Support tickets prepared

---

## Phase 4: Optimization & Hardening (Weeks 13-16)

### Goal: Performance optimization and production readiness

#### 第 13 周：性能分析

**任务：**
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

**验收标准：**
- [ ] Profiling data collected
- [ ] Bottlenecks identified
- [ ] Optimization targets set
- [ ] Baseline performance documented

#### 第 14 周：代码优化

**任务：**
1. 实现 zero-copy optimizations
2. Reduce memory allocations
3. Improve thread synchronization
4. Cache optimization

**优化：**
- Use shared_ptr for stream data (avoid memcpy)
- Pool allocations for RTSP/RTP packets
- Lock-free queues where possible
- Aligned memory for SIMD

**示例：**
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

**验收标准：**
- [ ] 20% CPU reduction
- [ ] 30% memory reduction
- [ ] No performance regressions
- [ ] Benchmarks improved

#### 第 15 周：压力测试

**任务：**
1. Load testing (4 cameras, max streams)
2. Endurance testing (7-day continuous)
3. Fault injection testing
4. Recovery testing

**测试场景：**
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

**验收标准：**
- [ ] 7-day continuous operation
- [ ] <0.1% frame drop rate
- [ ] Automatic recovery from faults
- [ ] No crashes or hangs

#### 第 16 周：最终测试与发布准备

**任务：**
1. Regression testing (all features)
2. 文档化ation review
3. Code review
4. Release notes
5. Deployment planning

**Regression 测试 Suite:**
- Single camera (legacy)
- Dual camera
- Four camera
- All encoding modes
- All features (OSD, AIISP, YOLOv5)
- RTSP + RTMP + MP4
- Configuration migration

**发布检查清单：**
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] 性能目标达成
- [ ] 文档化ation complete
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

### Stage 1: Internal 测试ing (Week 13)
- Deploy to development environment
- Team testing with 2 cameras
- Identify issues, iterate

### Stage 2: Beta 测试ing (Week 14-15)
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
- 添加ress bug reports
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

## 文档化 Version

- **Version:** 1.0
- **Date:** 2026-01-04
- **Author:** Code Analysis System
- **Status:** Approved for 实现ation

**End of 重构ing Roadmap**
