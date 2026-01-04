# RTSP 模块重构

本目录将包含用于多摄像头支持的重构 RTSP 模块实现。

## 状态

**当前状态：** 未填充 - 等待实施  
**目标完成：** 阶段 2-3（第 4-9 周）

## 计划组件

### 核心类

#### `stream_router.h/cpp`
- **Purpose:** 多摄像头流的中央路由
- **Responsibilities:**
  - Map (camera_id, stream_id) to stream 实例
  - URL path to stream resolution
  - 动态 stream registration/unregistration
- **Status:** 尚未实施

#### `stream_instance.h/cpp`
- **Purpose:** 封装单个流状态
- **Responsibilities:**
  - Hold stream 配置
  - Manage RTSP stock
  - Track stream observers
  - Lifecycle management
- **Status:** 尚未实施

#### `url_parser.h/cpp`
- **Purpose:** 解析和验证 RTSP URL
- **Responsibilities:**
  - Parse `/camera/{id}/{stream}` format
  - 支持legacy `/streamN` format
  - Extract camera_id and stream_name
- **Status:** 尚未实施

### 修改的类

#### `stream_manager` （现有）
- **Changes Required:**
  - Use stream_router internally
  - 移除 硬编码的 stream assumptions
  - 支持动态 registration
- **Backward Compatibility:** Yes
- **Status:** 等待重构

#### `rtsp_request_handler` （现有）
- **Changes Required:**
  - 更新 URL parsing logic
  - Route requests 通过 stream_router
  - Handle new URL schemes
- **Backward Compatibility:** Yes
- **Status:** 等待重构

## URL Scheme 设计

### 提议的 URL
```
rtsp://ip:port/camera/0/main
rtsp://ip:port/camera/0/sub
rtsp://ip:port/camera/1/main
rtsp://ip:port/camera/1/sub
rtsp://ip:port/camera/1/ai
```

### 传统支持 (Backward Compatible)
```
rtsp://ip:port/stream1  → /camera/0/main (alias)
rtsp://ip:port/stream2  → /camera/0/sub  (alias)
rtsp://ip:port/stream3  → /camera/0/ai   (alias)
```

### 替代紧凑格式
```
rtsp://ip:port/cam0_main
rtsp://ip:port/cam1_sub
```

## Key 设计 Decisions

### 1. 流识别
**Decision:** Use (camera_id, stream_id) pair as key  
**Rationale:** Numeric IDs are efficient, string names are user-friendly  
**实现:** Map both numeric and string identifiers

### 2. URL 解析策略
**Decision:** 支持多个 URL formats simultaneously  
**Rationale:** Backward compatibility while enabling new features  
**实现:** Pattern matching with fallback chain

### 3. 流生命周期
**Decision:** Lazy creation on first DESCRIBE request  
**Rationale:** Reduce resource usage for unused streams  
**Alternative:** Pre-创建 all configured streams (more predictable)

### 4. 观察者模式
**Decision:** Keep existing 观察者模式  
**Rationale:** Proven 设计, supports 多个 clients efficiently  
**Enhancement:** 添加 back-pressure handling for slow clients

## 重构检查清单

### 阶段 1: Preparation (第 4 周)
- [ ] 设计 stream_router 接口
- [ ] 设计 stream_instance class
- [ ] Plan URL parsing strategy
- [ ] 编写 unit 测试 stubs

### 阶段 2: 实现 (第 5 周-6)
- [ ] 实现 stream_router
- [ ] 实现 stream_instance
- [ ] 实现 url_parser
- [ ] 编写 unit tests
- [ ] Integration with existing code

### 阶段 3: Migration (第 7 周)
- [ ] 更新 stream_manager 以使用 router
- [ ] 更新 rtsp_request_handler
- [ ] 添加 向后兼容性 layer
- [ ] 更新 配置 loading
- [ ] Integration testing

### 阶段 4: Validation (第 8 周)
- [ ] 测试 单摄像头 (regression)
- [ ] 测试 dual camera
- [ ] 测试 URL parsing (all formats)
- [ ] Performance benchmarking
- [ ] 文档化ation

## 测试ing Strategy

### Unit 测试s
- `stream_router_test.cpp`
  - Register/unregister streams
  - Lookup by (camera_id, stream_id)
  - Lookup by URL path
  - 线程 safety
  
- `stream_instance_test.cpp`
  - Lifecycle (创建, start, stop)
  - Observer management
  - Configuration

- `url_parser_test.cpp`
  - Parse valid URLs
  - Reject invalid URLs
  - Extract components
  - Legacy format support

### Integration 测试s
- `multi_camera_rtsp_test.cpp`
  - Connect to 多个 cameras
  - 多个 streams 每摄像头
  - Concurrent clients
  - Stream switching

### Performance 测试s
- 通过put (streams/second)
- Latency (DESCRIBE to first frame)
- CPU usage (idle vs. loaded)
- Memory usage per stream

## 依赖项

### 内部
- `rtsp/stream/stream_stock.h` (existing)
- `rtsp/stream/stream_handler.h` (existing)
- `util/stream_observer.h` (existing)

### 外部
- Standard C++17 library
- POSIX threads
- Network sockets

## API 示例

### Registering a Stream (Proposed)
```cpp
#include "stream_router.h"
#include "stream_instance.h"

// 创建 stream 实例
auto stream_inst = std::make_shared<stream_instance>(
    camera_id,
    stream_id,
    stream_config
);

// Register with router
auto router = stream_router::实例();
router->register_stream(stream_inst);

// Also register URL alias
router->register_url_alias("/camera/0/main", camera_id, stream_id);
router->register_url_alias("/stream1", camera_id, stream_id); // Legacy
```

### Looking Up a Stream (Proposed)
```cpp
// By camera and stream ID
auto stream = router->get_stream(0, 0);

// By URL path
auto stream = router->get_stream_by_url("/camera/0/main");

// Legacy URL
auto stream = router->get_stream_by_url("/stream1");
```

### RTSP Request Flow (更新d)
```
Client                  RTSP Handler           Stream Router          Stream 实例
  |                           |                      |                        |
  |--DESCRIBE /camera/0/main->|                      |                        |
  |                           |--get_stream_by_url-->|                        |
  |                           |                      |--find mapping--------->|
  |                           |                      |<-stream_instance-------|
  |                           |<-stream 实例-----|                        |
  |                           |                      |                        |
  |                           |--get_stream_head---->|                        |
  |                           |<-SDP info------------|                        |
  |<-200 OK (SDP)-------------|                      |                        |
```

## 迁移指南

### 应用程序代码
1. Replace direct stream_manager calls with stream_router
2. 更新 URL generation 以使用 new format
3. 测试 向后兼容性 with old URLs

### 配置
1. 将 camera_id 添加到 stream definitions
2. 添加 URL path customization
3. Maintain legacy URL aliases

### For 测试s
1. 更新 URLs in 测试 cases
2. 添加 多摄像头 测试 scenarios
3. Verify legacy URL support

## 已知问题与限制

### Current Issues (Pre-重构ing)
1. 硬编码的 stream URLs (stream1, stream2, stream3)
2. Single camera assumption
3. No URL 验证
4. No 动态 stream creation

### Post-重构ing Improvements
1. ✅ Flexible URL scheme
2. ✅ Multi-camera support
3. ✅ URL 验证
4. ✅ 动态 stream management

## 性能考虑

### Lookup Performance
- Use `std::unordered_map` for O(1) lookups
- Cache frequently accessed streams
- Minimize lock contention

### 内存使用
- Stream 实例 allocated on-demand
- Shared pointers for lifetime management
- Limit maximum concurrent streams

### Scalability
- 支持4 cameras × 4 streams = 16 total streams
- Each stream supports 10+ concurrent clients
- Total: 160+ concurrent RTSP sessions

## 未来增强

### 阶段 4+
- Stream aliasing (多个 URLs → same stream)
- Stream groups (aggregate 多个 streams)
- 动态 stream transcoding
- Adaptive bitrate switching

## 参考资料

### Related 文档化s
- [Main Analysis](../ANALYSIS.md)
- [重构ing Roadmap](../REFACTORING_ROADMAP.md)

### 外部 Resources
- RTSP RFC 2326: https://tools.ietf.org/html/rfc2326
- RTP RFC 3984 (H.264): https://tools.ietf.org/html/rfc3984
- RTP RFC 7798 (H.265): https://tools.ietf.org/html/rfc7798

---

**Last 更新d:** 2026-01-04  
**Status:** Planning Phase  
**Next 审查:** 第 4 周 实现 Kickoff
