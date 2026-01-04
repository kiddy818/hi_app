# RTSP 模块重构

本目录将包含用于多摄像头支持的重构 RTSP 模块实现。

## Status

**当前状态：** 未填充 - 等待实施  
**目标完成：** 阶段 2-3（第 4-9 周）

## 计划组件

### 核心类

#### `stream_router.h/cpp`
- **Purpose:** 多摄像头流的中央路由
- **Responsibilities:**
  - Map (camera_id, stream_id) to stream instances
  - URL path to stream resolution
  - Dynamic stream registration/unregistration
- **Status:** Not yet implemented

#### `stream_instance.h/cpp`
- **Purpose:** 封装单个流状态
- **Responsibilities:**
  - Hold stream configuration
  - Manage RTSP stock
  - Track stream observers
  - Lifecycle management
- **Status:** Not yet implemented

#### `url_parser.h/cpp`
- **Purpose:** 解析和验证 RTSP URL
- **Responsibilities:**
  - Parse `/camera/{id}/{stream}` format
  - Support legacy `/streamN` format
  - Extract camera_id and stream_name
- **Status:** Not yet implemented

### 修改的类

#### `stream_manager` (Existing)
- **Changes Required:**
  - Use stream_router internally
  - Remove hardcoded stream assumptions
  - Support dynamic registration
- **Backward Compatibility:** Yes
- **Status:** Pending refactoring

#### `rtsp_request_handler` (Existing)
- **Changes Required:**
  - 更新 URL parsing logic
  - Route requests through stream_router
  - Handle new URL schemes
- **Backward Compatibility:** Yes
- **Status:** Pending refactoring

## URL Scheme 设计

### Proposed URLs
```
rtsp://ip:port/camera/0/main
rtsp://ip:port/camera/0/sub
rtsp://ip:port/camera/1/main
rtsp://ip:port/camera/1/sub
rtsp://ip:port/camera/1/ai
```

### Legacy Support (Backward Compatible)
```
rtsp://ip:port/stream1  → /camera/0/main (alias)
rtsp://ip:port/stream2  → /camera/0/sub  (alias)
rtsp://ip:port/stream3  → /camera/0/ai   (alias)
```

### Alternative Compact Format
```
rtsp://ip:port/cam0_main
rtsp://ip:port/cam1_sub
```

## Key 设计 Decisions

### 1. Stream Identification
**Decision:** Use (camera_id, stream_id) pair as key  
**Rationale:** Numeric IDs are efficient, string names are user-friendly  
**实现ation:** Map both numeric and string identifiers

### 2. URL Parsing Strategy
**Decision:** Support multiple URL formats simultaneously  
**Rationale:** Backward compatibility while enabling new features  
**实现ation:** Pattern matching with fallback chain

### 3. Stream Lifecycle
**Decision:** Lazy creation on first DESCRIBE request  
**Rationale:** Reduce resource usage for unused streams  
**Alternative:** Pre-create all configured streams (more predictable)

### 4. Observer Pattern
**Decision:** Keep existing observer pattern  
**Rationale:** Proven design, supports multiple clients efficiently  
**Enhancement:** 添加 back-pressure handling for slow clients

## 重构ing Checklist

### Phase 1: Preparation (Week 4)
- [ ] 设计 stream_router interface
- [ ] 设计 stream_instance class
- [ ] Plan URL parsing strategy
- [ ] 编写 unit test stubs

### Phase 2: 实现ation (Week 5-6)
- [ ] 实现 stream_router
- [ ] 实现 stream_instance
- [ ] 实现 url_parser
- [ ] 编写 unit tests
- [ ] Integration with existing code

### Phase 3: Migration (Week 7)
- [ ] 更新 stream_manager 以使用 router
- [ ] 更新 rtsp_request_handler
- [ ] 添加 backward compatibility layer
- [ ] 更新 configuration loading
- [ ] Integration testing

### Phase 4: Validation (Week 8)
- [ ] 测试 single camera (regression)
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
  - Thread safety
  
- `stream_instance_test.cpp`
  - Lifecycle (create, start, stop)
  - Observer management
  - Configuration

- `url_parser_test.cpp`
  - Parse valid URLs
  - Reject invalid URLs
  - Extract components
  - Legacy format support

### Integration 测试s
- `multi_camera_rtsp_test.cpp`
  - Connect to multiple cameras
  - Multiple streams per camera
  - Concurrent clients
  - Stream switching

### Performance 测试s
- Throughput (streams/second)
- Latency (DESCRIBE to first frame)
- CPU usage (idle vs. loaded)
- Memory usage per stream

## Dependencies

### Internal
- `rtsp/stream/stream_stock.h` (existing)
- `rtsp/stream/stream_handler.h` (existing)
- `util/stream_observer.h` (existing)

### External
- Standard C++17 library
- POSIX threads
- Network sockets

## API Examples

### Registering a Stream (Proposed)
```cpp
#include "stream_router.h"
#include "stream_instance.h"

// 创建 stream instance
auto stream_inst = std::make_shared<stream_instance>(
    camera_id,
    stream_id,
    stream_config
);

// Register with router
auto router = stream_router::instance();
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
Client                  RTSP Handler           Stream Router          Stream Instance
  |                           |                      |                        |
  |--DESCRIBE /camera/0/main->|                      |                        |
  |                           |--get_stream_by_url-->|                        |
  |                           |                      |--find mapping--------->|
  |                           |                      |<-stream_instance-------|
  |                           |<-stream instance-----|                        |
  |                           |                      |                        |
  |                           |--get_stream_head---->|                        |
  |                           |<-SDP info------------|                        |
  |<-200 OK (SDP)-------------|                      |                        |
```

## Migration Guide

### For Application Code
1. Replace direct stream_manager calls with stream_router
2. 更新 URL generation 以使用 new format
3. 测试 backward compatibility with old URLs

### For Configuration
1. 将 camera_id 添加到 stream definitions
2. 添加 URL path customization
3. Maintain legacy URL aliases

### For 测试s
1. 更新 URLs in test cases
2. 添加 multi-camera test scenarios
3. Verify legacy URL support

## Known Issues & Limitations

### Current Issues (Pre-重构ing)
1. Hardcoded stream URLs (stream1, stream2, stream3)
2. Single camera assumption
3. No URL validation
4. No dynamic stream creation

### Post-重构ing Improvements
1. ✅ Flexible URL scheme
2. ✅ Multi-camera support
3. ✅ URL validation
4. ✅ Dynamic stream management

## Performance Considerations

### Lookup Performance
- Use `std::unordered_map` for O(1) lookups
- Cache frequently accessed streams
- Minimize lock contention

### Memory Usage
- Stream instances allocated on-demand
- Shared pointers for lifetime management
- Limit maximum concurrent streams

### Scalability
- Support 4 cameras × 4 streams = 16 total streams
- Each stream supports 10+ concurrent clients
- Total: 160+ concurrent RTSP sessions

## Future Enhancements

### Phase 4+
- Stream aliasing (multiple URLs → same stream)
- Stream groups (aggregate multiple streams)
- Dynamic stream transcoding
- Adaptive bitrate switching

## References

### Related 文档化s
- [Main Analysis](../ANALYSIS.md)
- [重构ing Roadmap](../REFACTORING_ROADMAP.md)

### External Resources
- RTSP RFC 2326: https://tools.ietf.org/html/rfc2326
- RTP RFC 3984 (H.264): https://tools.ietf.org/html/rfc3984
- RTP RFC 7798 (H.265): https://tools.ietf.org/html/rfc7798

---

**Last 更新d:** 2026-01-04  
**Status:** Planning Phase  
**Next 审查:** Week 4 实现ation Kickoff
