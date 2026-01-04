# Code Analysis Directory

This directory contains comprehensive analysis and refactoring documentation for the HiSilicon video application codebase.

## Contents

### Main Documentation

- **[ANALYSIS.md](./ANALYSIS.md)**: Comprehensive analysis of the current codebase
  - RTSP module architecture and functionality
  - RTMP module architecture and functionality
  - Device module architecture and functionality
  - Current system limitations
  - Multi-camera support analysis
  - Proposed architecture design
  - Detailed refactoring requirements

- **[REFACTORING_ROADMAP.md](./REFACTORING_ROADMAP.md)**: Detailed refactoring plan
  - 16-week implementation timeline
  - Phase-by-phase breakdown
  - Prioritized action items
  - Code examples and snippets
  - Success metrics
  - Risk mitigation strategies

### Subdirectories

#### `rtsp/`
Placeholder directory for refactored RTSP module code. Will contain:
- Stream router implementation
- Stream instance management
- Enhanced URL parsing
- Dynamic stream registration

**Status:** Unpopulated (awaiting refactoring)

#### `rtmp/`
Placeholder directory for refactored RTMP module code. Will contain:
- Multi-session improvements
- URL template support
- Enhanced error handling
- H.265 support investigation

**Status:** Unpopulated (awaiting refactoring)

#### `device/`
Placeholder directory for refactored device module code. Will contain:
- Camera manager implementation
- Camera instance abstraction
- Resource manager
- Multi-camera support

**Status:** Unpopulated (awaiting refactoring)

## Key Findings

### Current State
- **Single Camera Only**: MAX_CHANNEL = 1 hardcoded
- **Static Configuration**: JSON file-based with restart required
- **Fixed Stream URLs**: stream1, stream2, stream3 only
- **Tight Coupling**: Channel class manages too many responsibilities

### Target State
- **Multi-Camera Support**: 2-4 cameras simultaneously
- **Dynamic Configuration**: Runtime camera/stream creation
- **Flexible URLs**: /camera/{id}/{stream} scheme
- **Modular Design**: Separated concerns with plugin system

## Architecture Diagrams

### Current Architecture
```
Application (Single Camera)
    ↓
  chn (Channel)
    ├─ VI (Sensor)
    ├─ VPSS (Processing)
    ├─ VENC (Encoding)
    └─ Features (OSD, AIISP, etc.)
    ↓
Stream Distribution (Observer Pattern)
    ├─ RTSP Server
    ├─ RTMP Sessions
    ├─ MP4 Recorder
    └─ JPEG Snapshot
```

### Proposed Architecture
```
Application Manager
    ↓
Camera Manager
    ├─ Camera Instance 0
    │   ├─ VI/VPSS/VENC
    │   └─ Stream Instances []
    ├─ Camera Instance 1
    │   ├─ VI/VPSS/VENC
    │   └─ Stream Instances []
    └─ Camera Instance N
        ├─ VI/VPSS/VENC
        └─ Stream Instances []
    ↓
Stream Router
    ↓
Service Layer
    ├─ RTSP Service
    ├─ RTMP Service
    └─ Recording Service
```

## Critical Refactoring Points

### 1. Remove MAX_CHANNEL Limitation
**Priority:** High  
**Effort:** Medium  
**Impact:** Enables multiple camera support

```cpp
// Before
#define MAX_CHANNEL 1
static std::shared_ptr<chn> g_chns[MAX_CHANNEL];

// After
// Dynamic allocation through camera_manager
```

### 2. Implement Camera Manager
**Priority:** High  
**Effort:** High  
**Impact:** Core abstraction for multi-camera

### 3. Stream Router
**Priority:** High  
**Effort:** Medium  
**Impact:** Flexible URL mapping and stream distribution

### 4. Resource Manager
**Priority:** High  
**Effort:** High  
**Impact:** Prevents resource exhaustion, validates configurations

### 5. Configuration System
**Priority:** Medium  
**Effort:** High  
**Impact:** Unified multi-camera configuration

### 6. Feature Plugin System
**Priority:** Medium  
**Effort:** High  
**Impact:** Modular, per-camera feature control

## Implementation Phases

### Phase 1: Foundation (Weeks 1-4)
- Resource manager
- Camera manager
- Stream router
- **Goal:** Core abstractions without breaking changes

### Phase 2: Multi-Camera (Weeks 5-8)
- Remove MAX_CHANNEL limit
- Dual camera configuration
- Multi-VENC capture
- **Goal:** 2+ cameras working simultaneously

### Phase 3: Advanced Features (Weeks 9-12)
- Feature plugin system
- Dynamic reconfiguration
- RESTful API (optional)
- **Goal:** Production-ready features

### Phase 4: Optimization (Weeks 13-16)
- Performance profiling
- Code optimization
- Stress testing
- **Goal:** Production hardening

## Testing Strategy

### Unit Tests
- Resource allocation/deallocation
- Camera lifecycle management
- Stream routing logic
- Configuration validation

### Integration Tests
- Single camera (regression)
- Dual camera
- Four camera maximum
- All feature combinations

### Performance Tests
- CPU usage (target: <80%)
- Memory usage (target: <512MB)
- Frame rate stability (30fps per camera)
- Latency (<100ms RTSP)

### Stress Tests
- 7-day continuous operation
- Maximum RTSP clients
- Network interruptions
- Resource exhaustion

## Success Metrics

### Functional
- ✅ Support 2-4 cameras simultaneously
- ✅ Independent camera configuration
- ✅ Dynamic stream creation/deletion
- ✅ Multi-camera RTSP URLs
- ✅ Multi-camera RTMP support

### Performance
- ✅ 30fps per camera (1080p)
- ✅ <100ms RTSP latency
- ✅ <80% CPU usage (quad-core)
- ✅ <512MB memory
- ✅ 99.9% uptime

### Quality
- ✅ Zero critical bugs
- ✅ <0.1% frame drop
- ✅ Automatic recovery
- ✅ Complete documentation
- ✅ Comprehensive tests

## Usage

### For Developers
1. Read `ANALYSIS.md` for complete understanding
2. Review `REFACTORING_ROADMAP.md` for implementation plan
3. Follow phase-by-phase implementation
4. Write tests for each component
5. Update documentation as you go

### For Reviewers
1. Check architecture alignment with ANALYSIS.md
2. Verify refactoring follows ROADMAP priorities
3. Ensure backward compatibility maintained
4. Review test coverage
5. Validate performance targets

## Dependencies

### Hardware
- HiSilicon 3519DV500 chip
- Multiple camera sensors (OS04A10, OS08A20)
- Development board

### Software
- HiSilicon MPP SDK V2.0.2.1
- C++17 compiler
- librtmp
- jsoncpp
- log4cpp

## Related Documents

### In Repository
- `/README.md`: Main project README
- `/doc/set.md`: Configuration guide
- `/doc/debug_log.md`: Debug information

### External
- HiSilicon MPP SDK documentation
- RTSP RFC 2326
- RTP RFC 3984 (H.264)
- RTMP specification

## Contributing

When contributing refactored code:

1. **Place in appropriate subdirectory**
   - `cn_analyst/rtsp/` for RTSP refactoring
   - `cn_analyst/rtmp/` for RTMP refactoring
   - `cn_analyst/device/` for device refactoring

2. **Follow coding standards**
   - C++17 style
   - Consistent naming conventions
   - Comprehensive comments
   - Unit tests required

3. **Update documentation**
   - Update ANALYSIS.md if architecture changes
   - Update ROADMAP.md progress
   - Document new APIs

4. **Test thoroughly**
   - Unit tests for new code
   - Integration tests for interactions
   - Performance benchmarks

## Contact

For questions about this analysis:
- Review the issue tracker
- Check existing documentation
- Consult with team leads

## Version History

- **v1.0** (2026-01-04): Initial analysis and roadmap
  - Complete architecture analysis
  - Detailed refactoring plan
  - Multi-camera design

## License

Same as main project.

---

**Note:** This directory contains analysis and planning documentation. Actual refactored code will be placed in the subdirectories once implementation begins.
