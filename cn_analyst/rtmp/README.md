# RTMP Module Refactoring

This directory will contain the refactored RTMP module implementation for multi-camera support.

## Status

**Current Status:** Unpopulated - Awaiting Implementation  
**Target Completion:** Phase 2-3 (Weeks 6-9)

## Planned Components

### Core Classes

#### `multi_session_manager.h/cpp`
- **Purpose:** Enhanced session management for multi-camera
- **Responsibilities:**
  - Track sessions per (camera_id, stream_id, url)
  - Support URL templates
  - Handle connection failures
  - Automatic reconnection
- **Status:** Not yet implemented

#### `url_template.h/cpp`
- **Purpose:** URL template expansion
- **Responsibilities:**
  - Parse template strings
  - Substitute `{camera_id}`, `{stream_name}` variables
  - Validate expanded URLs
- **Example:** `rtmp://server/live/cam{camera_id}_{stream_name}`
- **Status:** Not yet implemented

#### `rtmp_encoder.h/cpp`
- **Purpose:** Abstract encoding logic
- **Responsibilities:**
  - H.264 → FLV encoding (existing)
  - H.265 → FLV encoding (future)
  - Codec detection and negotiation
- **Status:** Planning phase

### Modified Classes

#### `session` (Existing)
- **Changes Required:**
  - Support codec abstraction
  - Improve error recovery
  - Add connection state machine
- **Backward Compatibility:** Yes
- **Status:** Pending refactoring

#### `session_manager` (Existing)
- **Changes Required:**
  - Use URL templates
  - Per-camera session tracking
  - Dynamic session creation
- **Backward Compatibility:** Yes
- **Status:** Pending refactoring

## URL Template Design

### Template Format
```json
{
  "rtmp": {
    "enabled": true,
    "url_template": "rtmp://server/live/cam{camera_id}_{stream_name}",
    "fallback_urls": [
      "rtmp://server2/live/cam{camera_id}_{stream_name}"
    ]
  }
}
```

### Expansion Examples
```
Template: rtmp://192.168.1.100/live/cam{camera_id}_{stream_name}

camera_id=0, stream_name="main"
→ rtmp://192.168.1.100/live/cam0_main

camera_id=1, stream_name="sub"
→ rtmp://192.168.1.100/live/cam1_sub
```

### Static URLs (Backward Compatible)
```json
{
  "cameras": [
    {
      "camera_id": 0,
      "streams": [
        {
          "stream_id": 0,
          "rtmp": {
            "urls": [
              "rtmp://server/live/cam0_main",
              "rtmp://backup/live/cam0_main"
            ]
          }
        }
      ]
    }
  ]
}
```

## Key Design Decisions

### 1. H.265 Support
**Challenge:** RTMP spec doesn't officially support H.265  
**Options:**
- **A. Enhanced FLV**: Use Extended VideoTagHeader (Adobe proprietary)
- **B. WebRTC/SRT**: Alternative protocols (major refactor)
- **C. H.264 Only**: Transcode H.265→H.264 (CPU intensive)

**Recommended:** Option A (Enhanced FLV) for H.265 support  
**Status:** Research phase, low priority

### 2. Connection Management
**Current:** Manual restart on failure  
**Proposed:** Automatic reconnection with exponential backoff

```cpp
class connection_manager {
    void on_disconnect();
    bool should_reconnect();
    std::chrono::seconds get_backoff_delay();
    void reset_backoff();
};
```

**Retry Strategy:**
- Initial delay: 1 second
- Max delay: 60 seconds
- Exponential backoff: 2^attempt seconds
- Max attempts: 10 (then give up)

### 3. Multi-URL Failover
**Feature:** Support multiple RTMP servers per stream  
**Behavior:** Try primary, fallback on failure

```
URLs: [primary, backup1, backup2]
1. Connect to primary
2. On failure, connect to backup1
3. On failure, connect to backup2
4. On failure, wait and retry primary
```

### 4. Session Pooling
**Problem:** Creating sessions is expensive  
**Solution:** Reuse session objects

```cpp
class session_pool {
    std::queue<std::shared_ptr<session>> m_available;
    std::shared_ptr<session> acquire();
    void release(std::shared_ptr<session> sess);
};
```

## Refactoring Checklist

### Phase 1: Preparation (Week 6)
- [ ] Design URL template system
- [ ] Design connection state machine
- [ ] Plan H.265 support (research)
- [ ] Write unit test stubs

### Phase 2: Implementation (Week 7-8)
- [ ] Implement url_template
- [ ] Implement connection_manager
- [ ] Enhance session_manager
- [ ] Add failover support
- [ ] Write unit tests

### Phase 3: Integration (Week 9)
- [ ] Update configuration loading
- [ ] Integration with camera_manager
- [ ] Add reconnection logic
- [ ] Performance testing

### Phase 4: Validation (Week 10)
- [ ] Test single stream (regression)
- [ ] Test multi-camera
- [ ] Test failover scenarios
- [ ] Stress testing
- [ ] Documentation

## Testing Strategy

### Unit Tests
- `url_template_test.cpp`
  - Parse templates
  - Variable substitution
  - Invalid template handling
  
- `connection_manager_test.cpp`
  - Reconnection logic
  - Backoff calculation
  - State transitions

- `session_manager_test.cpp`
  - Multi-camera sessions
  - URL template expansion
  - Session lifecycle

### Integration Tests
- `multi_camera_rtmp_test.cpp`
  - Push from multiple cameras
  - Concurrent sessions
  - Failover testing

### Stress Tests
- Long-duration push (24+ hours)
- Network interruptions
- Server restarts
- Memory leak detection

## Dependencies

### Internal
- `rtmp/session.h` (existing)
- `device/camera_instance.h` (new)
- `util/stream_observer.h` (existing)

### External
- librtmp (RTMP protocol)
- OpenSSL (for RTMPS)

## API Examples

### URL Template Usage (Proposed)
```cpp
#include "url_template.h"

// Parse template
url_template tmpl("rtmp://server/live/cam{camera_id}_{stream_name}");

// Expand variables
std::map<std::string, std::string> vars = {
    {"camera_id", "0"},
    {"stream_name", "main"}
};
std::string url = tmpl.expand(vars);
// Result: "rtmp://server/live/cam0_main"
```

### Session Management (Proposed)
```cpp
#include "multi_session_manager.h"

auto mgr = multi_session_manager::instance();

// Create session for camera 0, stream 0
session_config cfg;
cfg.url_template = "rtmp://server/live/cam{camera_id}_{stream_name}";
cfg.camera_id = 0;
cfg.stream_name = "main";

mgr->create_session(cfg);

// Session automatically reconnects on failure
```

### Failover Example (Proposed)
```cpp
session_config cfg;
cfg.urls = {
    "rtmp://primary/live/stream",
    "rtmp://backup1/live/stream",
    "rtmp://backup2/live/stream"
};
cfg.retry_policy.max_attempts = 10;
cfg.retry_policy.initial_delay = 1s;
cfg.retry_policy.max_delay = 60s;

mgr->create_session(cfg);
// Automatically tries each URL on failure
```

## Migration Guide

### For Application Code
1. Replace static URLs with templates
2. Add camera_id and stream_name to config
3. Use new session_manager API

### For Configuration
```json
// Old format
{
  "rtmp": {
    "enable": 1,
    "main_url": "rtmp://server/live/stream1",
    "sub_url": "rtmp://server/live/stream2"
  }
}

// New format
{
  "rtmp": {
    "enabled": true,
    "url_template": "rtmp://server/live/cam{camera_id}_{stream_name}",
    "auto_reconnect": true,
    "max_retry_attempts": 10
  }
}
```

### Migration Tool
```bash
# Convert old config to new format
./tools/migrate_rtmp_config.py \
  --old /opt/ceanic/etc/net_service.json \
  --new /opt/ceanic/etc/system_config.json
```

## Known Issues & Limitations

### Current Issues (Pre-Refactoring)
1. H.264 only (no H.265 support)
2. Static URL configuration
3. No automatic reconnection
4. Single RTMP server per stream
5. Manual error recovery

### Post-Refactoring Improvements
1. ✅ URL templates for multi-camera
2. ✅ Automatic reconnection
3. ✅ Failover support
4. ✅ Per-camera session tracking
5. 🔄 H.265 support (future)

## Performance Considerations

### CPU Usage
- FLV encoding overhead: ~5-10% per stream
- Target: <15% CPU for RTMP (4 streams)

### Network
- Each session: ~1-10 Mbps depending on bitrate
- Monitor send buffer fullness
- Handle slow networks gracefully

### Memory
- Each session: ~2-4 MB
- 16 sessions: ~32-64 MB
- Keep within budget

### Latency
- Target: <1 second end-to-end
- librtmp buffering: ~200-500ms
- Network latency: varies

## H.265 Support Research

### Enhanced FLV Format
```
VideoTagHeader (Enhanced):
- UB[4] FrameType (1=keyframe, 2=inter)
- UB[4] CodecID (12=HEVC)
- UI8 AVCPacketType
- SI24 CompositionTime
- HEVC VideoPacket
```

**References:**
- https://github.com/veovera/enhanced-rtmp
- Adobe Flash Video File Format v10.1

**Pros:**
- Compatible with some servers (e.g., newer nginx-rtmp)
- No transcoding required
- Same FLV container

**Cons:**
- Not universally supported
- Requires server-side updates
- Proprietary extension

### Implementation Plan (Future)
1. Detect encoder codec (H.264 vs H.265)
2. Use appropriate FLV tag format
3. Server compatibility check
4. Fallback to H.264 if unsupported

## Connection State Machine

```
[Disconnected] --connect()--> [Connecting]
                                  |
                    +-------------+-------------+
                    |                           |
                success                      failure
                    |                           |
                    v                           v
              [Connected] --disconnect()--> [Disconnected]
                    |                           ^
                    |                           |
              stream data                   reconnect
                    |                           |
                    +------error---------------+
```

**States:**
- **Disconnected**: No connection
- **Connecting**: Attempting connection
- **Connected**: Active streaming
- **Error**: Temporary failure (triggers reconnect)

**Transitions:**
- connect(): Initiate connection
- success: Connection established
- failure: Connection failed
- disconnect(): Graceful shutdown
- error: Connection lost (auto-reconnect)
- reconnect(): Retry connection

## Monitoring & Metrics

### Per-Session Metrics
- Connection state
- Bitrate (current, average, peak)
- Frame count (sent, dropped)
- Error count
- Uptime / Downtime
- Last error message

### Aggregate Metrics
- Active sessions
- Total data sent
- Failed connection attempts
- Average latency

### Logging
```cpp
RTMP_WRITE_LOG_INFO("Session cam%d_stream%d connected to %s", 
                     camera_id, stream_id, url.c_str());
RTMP_WRITE_LOG_ERROR("Session failed, retry in %ds", backoff_delay);
```

## Future Enhancements

### Phase 4+
- H.265 support (Enhanced FLV)
- RTMPS support (SSL/TLS)
- SRT protocol support (low latency)
- WebRTC output (browser compatible)
- Adaptive bitrate

## References

### Related Documents
- [Main Analysis](../ANALYSIS.md)
- [Refactoring Roadmap](../REFACTORING_ROADMAP.md)

### External Resources
- RTMP Specification: https://rtmp.veriskope.com/docs/spec/
- Enhanced RTMP: https://github.com/veovera/enhanced-rtmp
- librtmp Documentation: https://rtmpdump.mplayerhq.hu/librtmp.3.html
- nginx-rtmp-module: https://github.com/arut/nginx-rtmp-module

---

**Last Updated:** 2026-01-04  
**Status:** Planning Phase  
**Next Review:** Week 6 Implementation Kickoff
