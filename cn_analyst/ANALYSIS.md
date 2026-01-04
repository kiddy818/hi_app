# Codebase Analysis: RTSP, RTMP, and Device Modules

## Executive Summary

This document provides a comprehensive analysis of the HiSilicon 3519DV500 video application's architecture, focusing on the `rtsp`, `rtmp`, and `device` directories. The analysis identifies the current design limitations for multi-camera support and proposes a refactored architecture optimized for scalability and modularity.

**Current Status**: Single camera support (MAX_CHANNEL = 1)  
**Target Goal**: Multi-camera and multi-stream support with improved modularity

---

## 1. Current Architecture Analysis

### 1.1 RTSP Module (`rtsp/`)

#### Purpose
The RTSP module implements a complete RTSP (Real-Time Streaming Protocol) server for streaming video over network.

#### Key Components

**Server Layer:**
- `server.h/cpp`: Main RTSP server implementation
  - Uses select-based event loop for handling multiple client connections
  - Listens on configurable port (default: 554)
  - Manages session lifecycle and timeout handling
  - Single-threaded server design with fd_set for multiplexing

- `session.h/cpp`: Base session class for connection management
  - Abstract base class defining session interface
  - Handles socket operations and timeouts
  - Provides idle processing hooks

- `rtsp_session.h/cpp`: RTSP-specific session implementation
  - Handles RTSP protocol parsing and request processing
  - Manages session timeout (MAX_SESSION_TIMEOUT + 3)
  - Integrates with rtsp_request_handler for protocol handling

**Protocol Layer:**
- `request.h`: RTSP request structure definition
- `request_parser.h/cpp`: RTSP protocol parser
- `rtsp_request_handler.h/cpp`: Request processing and response generation
  - Handles DESCRIBE, SETUP, PLAY, TEARDOWN, PAUSE methods
  - Manages RTP session creation and teardown

**Stream Management:**
- `stream/stream_manager.h/cpp`: Central stream registry and dispatcher
  - Singleton pattern for global stream management
  - Maps (chn, stream_id) pairs to stream_stock objects
  - Implements stream health checking (5-second timeout)
  - Callbacks to device layer via function pointers

- `stream/stream_stock.h/cpp`: Stream buffer and observer pattern implementation
  - Maintains list of observers (RTSP clients) for each stream
  - Buffers stream data and distributes to all observers
  - Tracks last stream timestamp for health monitoring

- `stream/stream_handler.h/cpp`: Base handler for stream processing
- `stream/stream_video_handler.h/cpp`: Video stream handler
- `stream/stream_audio_handler.h/cpp`: Audio stream handler

**RTP Layer:**
- `rtp_session/`: RTP session implementations
  - `rtp_session.h/cpp`: Base RTP session
  - `rtp_tcp_session.h/cpp`: RTP over TCP (interleaved)
  - `rtp_udp_session.h/cpp`: RTP over UDP

- `rtp_serialize/`: Codec-specific RTP packetizers
  - `h264_rtp_serialize.h/cpp`: H.264 RTP packetization
  - `h265_rtp_serialize.h/cpp`: H.265/HEVC RTP packetization
  - `aac_rtp_serialize.h/cpp`: AAC audio RTP packetization
  - `pcmu_rtp_serialize.h/cpp`: PCMU audio RTP packetization

#### Current Limitations
1. **Single Server Instance**: Only one RTSP server can run
2. **Static Stream Mapping**: Hardcoded URLs (stream1, stream2, stream3)
3. **Channel Limitation**: Tied to MAX_CHANNEL = 1
4. **Stream ID Convention**: Fixed IDs (0=main, 1=sub, 2=ai)
5. **No Dynamic Stream Creation**: Streams must be pre-registered

#### Interaction with Other Modules
- Receives encoded video data from `device/venc` via observer pattern
- Uses callback functions to request I-frames and query stream metadata
- Independent of transport (TCP/UDP) through abstraction layers

---

### 1.2 RTMP Module (`rtmp/`)

#### Purpose
The RTMP module provides video streaming to RTMP servers (e.g., nginx-rtmp) for live broadcasting.

#### Key Components

**Session Management:**
- `session.h/cpp`: RTMP client session implementation
  - Connects to remote RTMP server (push mode only)
  - Manages connection lifecycle (connect, stream, disconnect)
  - Handles H.264 video encoding to FLV format
  - Processes SPS/PPS NAL units separately
  - AAC audio support
  - Uses librtmp for protocol implementation

- `session_manager.h/cpp`: Multi-session coordinator
  - Manages multiple RTMP push sessions
  - Maps (chn, stream_id, url) tuples to session instances
  - Distributes stream data to all active sessions
  - Handles session creation and cleanup

**Data Flow:**
- Receives NAL units from device/venc via observer pattern
- Buffers data in circular buffer (stream_buf)
- Separate thread processes and sends data to RTMP server
- Wait for I-frame before starting transmission
- Sends SPS/PPS before each I-frame
- Sends AAC spec before audio frames

#### Current Limitations
1. **H.264 Only**: RTMP only supports H.264 (not H.265)
2. **Push Only**: No RTMP server/pull capability
3. **Static Configuration**: URLs configured via JSON file
4. **Limited Error Recovery**: Connection failures require manual restart
5. **Single Stream per Session**: One URL per (chn, stream_id) pair

#### Dependencies
- librtmp: External RTMP protocol library
- stream_buf: Circular buffer utility
- Tightly coupled to device module for stream data

---

### 1.3 Device Module (`device/`)

#### Purpose
The device module interfaces with HiSilicon hardware for video capture, encoding, and processing.

#### Key Components

**System Management:**
- `dev_sys.h/cpp`: System initialization and resource allocation
  - VB (Video Buffer) pool management
  - System binding management
  - Channel allocation (VI, VPSS, VENC)

**Video Input (VI):**
- `dev_vi.h/cpp`: Base VI (Video Input) class
  - Abstract interface for different sensors
  - VPSS (Video Processing SubSystem) integration
  - Frame rate and resolution management

- Sensor Implementations:
  - `dev_vi_os04a10_liner.h/cpp`: OS04A10 linear mode (2688x1520@30fps)
  - `dev_vi_os04a10_2to1wdr.h/cpp`: OS04A10 WDR mode
  - `dev_vi_os08a20_liner.h/cpp`: OS08A20 linear mode (3840x2160@30fps)
  - `dev_vi_os08a20_2to1wdr.h/cpp`: OS08A20 WDR mode

- `dev_vi_isp.h/cpp`: ISP (Image Signal Processing) configuration
  - AE (Auto Exposure), AWB (Auto White Balance)
  - 3A algorithm integration

**Video Encoding (VENC):**
- `dev_venc.h/cpp`: Video encoder management
  - H.264/H.265 encoding support
  - CBR (Constant Bit Rate) and AVBR (Adaptive Variable Bit Rate)
  - Implements observer pattern as stream_post
  - Single capture thread for all encoders (select-based)
  - Bound to VPSS output channels

**Channel Management:**
- `dev_chn.h/cpp`: High-level channel orchestrator
  - Combines VI, VPSS, VENC, OSD into single channel
  - Static array of channels: `g_chns[MAX_CHANNEL]`
  - **Critical Limitation**: MAX_CHANNEL = 1
  - Manages scene auto, AIISP, rate auto features
  - Implements stream_observer interface

**Additional Features:**
- `dev_osd.h/cpp`: On-Screen Display (timestamp overlay)
- `dev_snap.h/cpp`: JPEG snapshot capture
- `dev_vo.h/cpp`, `dev_vo_bt1120.h/cpp`: Video output support
- `dev_svp.h/cpp`, `dev_svp_yolov5.h/cpp`: AI inference (YOLOv5)

**Sensor Drivers:**
- `sensor/omnivision_os04a10/`: OS04A10 sensor driver (C code)
- `sensor/omnivision_os08a20/`: OS08A20 sensor driver (C code)

#### Current Limitations
1. **Single Channel**: `MAX_CHANNEL = 1` hardcoded
2. **Static Configuration**: All settings via JSON files
3. **Fixed Stream IDs**: 0=main, 1=sub, 2=ai
4. **Tight Coupling**: chn class manages too many responsibilities
5. **Global State**: Singleton patterns and static arrays
6. **No Dynamic Reconfiguration**: Requires restart for changes

#### Interaction Flow
```
Sensor → VI → VPSS → VENC → Observer Pattern → RTSP/RTMP/MP4/SNAP
                 ↓
               AIISP (optional)
                 ↓
               VO (optional)
```

---

## 2. Current System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                         │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────────────┐ │
│  │   main.cpp  │  │ Config Files │  │  Signal Handlers       │ │
│  │  (1 camera) │  │  (JSON)      │  │  (SIGINT/SIGTERM)      │ │
│  └──────┬──────┘  └──────────────┘  └────────────────────────┘ │
└─────────┼───────────────────────────────────────────────────────┘
          │
          ├─────────────────────┬──────────────────────┬──────────────────┐
          ▼                     ▼                      ▼                  ▼
┌─────────────────┐   ┌──────────────────┐  ┌──────────────────┐  ┌─────────────┐
│  RTSP Server    │   │  RTMP Sessions   │  │  Device Channel  │  │  Features   │
│  (Port 554)     │   │  (Push to nginx) │  │  (MAX_CHANNEL=1) │  │             │
├─────────────────┤   ├──────────────────┤  ├──────────────────┤  ├─────────────┤
│ • rtsp_server   │   │ • session_mgr    │  │ • chn class      │  │ • MP4 Save  │
│ • Multiple      │   │ • Multiple URLs  │  │ • VI (sensor)    │  │ • JPEG Snap │
│   clients       │   │   per stream     │  │ • VPSS (scale)   │  │ • Scene     │
│ • TCP select()  │   ├──────────────────┤  │ • VENC (encode)  │  │ • AIISP     │
├─────────────────┤   │ Data Flow:       │  │ • OSD (time)     │  │ • YOLOv5    │
│ Stream Manager: │   │ H264/AAC → FLV   │  ├──────────────────┤  │ • VO Output │
│ • stream_stock  │   │ → librtmp →      │  │ Stream Observer  │  └─────────────┘
│ • stream1 (0,0) │   │   RTMP Server    │  │ Pattern:         │
│ • stream2 (0,1) │   └──────────────────┘  │ ┌──────────────┐ │
│ • stream3 (0,2) │                          │ │ on_stream_   │ │
│ • Observer list │                          │ │   come()     │ │
├─────────────────┤                          │ └──────────────┘ │
│ RTP Sessions:   │                          │ Notifies:        │
│ • TCP/UDP       │                          │ • RTSP streams   │
│ • H264/H265/AAC │◄─────────────────────────┤ • RTMP sessions  │
└─────────────────┘                          │ • MP4 recorder   │
                                             └──────────────────┘
                                                      │
                                                      ▼
                                             ┌──────────────────┐
                                             │ HiSilicon MPP    │
                                             │ (Hardware)       │
                                             ├──────────────────┤
                                             │ • VI (capture)   │
                                             │ • VPSS (process) │
                                             │ • VENC (encode)  │
                                             │ • VO (output)    │
                                             │ • SVP (AI)       │
                                             └──────────────────┘
```

**Key Observations:**
1. Single camera channel (chn=0) feeds all streams
2. Stream differentiation by stream_id only (0=main, 1=sub, 2=ai)
3. Observer pattern enables 1-to-N distribution
4. RTSP and RTMP modules are independent consumers
5. Configuration is static and file-based

---

## 3. Multi-Camera Support Limitations

### 3.1 Current Multi-Stream Support

**What Works:**
- ✅ Single camera with multiple encoded streams (main, sub, ai)
- ✅ Multiple RTSP clients can connect to each stream
- ✅ Multiple RTMP destinations per stream
- ✅ Different resolutions/bitrates per stream (via VPSS)

**What Doesn't Work:**
- ❌ Multiple physical cameras (MAX_CHANNEL = 1)
- ❌ Independent camera configurations
- ❌ Dynamic stream creation/deletion
- ❌ Per-camera feature toggles (AIISP, YOLOv5)
- ❌ Scalable resource allocation

### 3.2 Identified Bottlenecks

#### Code-Level Constraints

1. **Hard-coded Channel Limit**
   ```cpp
   // device/dev_chn.h
   #define MAX_CHANNEL 1
   static std::shared_ptr<chn> g_chns[MAX_CHANNEL];
   ```

2. **Global Singleton State**
   ```cpp
   // main.cpp
   std::shared_ptr<hisilicon::dev::chn> g_chn;  // Single global channel
   ```

3. **Static Configuration Arrays**
   ```cpp
   static venc_t g_venc_info[MAX_CHANNEL];
   static vi_t g_vi_info[MAX_CHANNEL];
   ```

4. **VENC Capture Thread**
   ```cpp
   // device/dev_venc.cpp
   static std::list<venc_ptr> g_vencs;  // All encoders in single list
   static std::thread g_capture_thread; // One thread for all
   ```

5. **Stream Manager Limitations**
   ```cpp
   // rtsp/stream/stream_manager.cpp
   // No enforcement of channel limits, but assumes single channel
   ```

#### Resource Allocation Issues

1. **VB Pool**: Single shared pool for all channels
2. **ISP**: One ISP pipeline per VI device
3. **VPSS Groups**: Limited number of groups (typically 16-32)
4. **VENC Channels**: Limited hardware encoders (8-16 depending on chip)
5. **Scene/AIISP**: Global configuration, not per-channel

### 3.3 Multi-Camera Scenarios

**Scenario 1: Dual Camera System**
- Camera 1: OS04A10, 2688x1520@30fps, H.264
  - Main stream (1920x1080@30fps) → RTSP stream1
  - Sub stream (720x576@30fps) → RTSP stream2
- Camera 2: OS08A20, 3840x2160@30fps, H.265
  - Main stream (3840x2160@30fps) → RTSP stream3
  - Sub stream (1920x1080@30fps) → RTSP stream4

**Current Blockers:**
1. MAX_CHANNEL = 1
2. g_chn is single instance
3. Static resource allocation
4. Fixed stream URL mapping

---

## 4. Proposed Architecture for Multi-Camera Support

### 4.1 Design Principles

1. **Scalability**: Support N cameras with M streams each
2. **Modularity**: Loosely coupled components
3. **Configurability**: Runtime configuration changes
4. **Resource Management**: Dynamic allocation based on availability
5. **Backward Compatibility**: Maintain existing APIs where possible

### 4.2 Refactored Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     Application Layer (Refactored)                      │
│  ┌─────────────────┐  ┌──────────────────┐  ┌────────────────────────┐│
│  │  Application    │  │  Configuration   │  │  Resource Manager      ││
│  │  Manager        │  │  Manager         │  │  (VB/VPSS/VENC pools)  ││
│  │  - Lifecycle    │  │  - Dynamic load  │  │  - Allocation tracking ││
│  │  - Multi-thread │  │  - Validation    │  │  - Hardware limits     ││
│  └─────────────────┘  └──────────────────┘  └────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────┘
           │                       │                         │
           ├───────────────────────┼─────────────────────────┤
           ▼                       ▼                         ▼
┌──────────────────────┐  ┌──────────────────────┐  ┌──────────────────────┐
│  Streaming Services  │  │  Camera Manager      │  │  Feature Manager     │
│  (Service Layer)     │  │  (Device Layer)      │  │  (Plugin System)     │
├──────────────────────┤  ├──────────────────────┤  ├──────────────────────┤
│ RTSP Service:        │  │ Camera Registry:     │  │ Feature Registry:    │
│ • Multi-instance     │  │ • camera_instance[]  │  │ • Per-camera enable  │
│ • Dynamic URLs       │  │ • MAX_CAMERA_NUM=16  │  │ • AIISP (per VI)     │
│ • /camera/<id>/<st>  │  │                      │  │ • YOLOv5 (per cam)   │
│                      │  │ Camera Instance:     │  │ • OSD (per stream)   │
│ RTMP Service:        │  │ ┌────────────────┐  │  │ • Scene (per VI)     │
│ • URL templates      │  │ │ camera_id      │  │  │ • MP4/SNAP (per st)  │
│ • <cam>/<stream>     │  │ │ vi_instance    │  │  └──────────────────────┘
│                      │  │ │ vpss_grp       │  │
│ Stream Router:       │  │ │ venc_list[]    │  │
│ • Maps (cam, st) →   │  │ │ stream_list[]  │  │
│   stream_stock       │  │ │ feature_ctx    │  │
│ • Observer registry  │  │ └────────────────┘  │
└──────────────────────┘  └──────────────────────┘
           │                        │
           └────────────┬───────────┘
                        ▼
           ┌────────────────────────────┐
           │   Stream Distribution      │
           │   (Pub-Sub Pattern)        │
           ├────────────────────────────┤
           │ • stream_stock per stream  │
           │ • Multi-consumer support   │
           │ • Back-pressure handling   │
           └────────────────────────────┘
                        │
                        ▼
           ┌────────────────────────────┐
           │   HiSilicon HAL            │
           │   (Hardware Abstraction)   │
           ├────────────────────────────┤
           │ • Multi-VI support         │
           │ • VPSS group pooling       │
           │ • VENC channel allocation  │
           │ • Resource arbitration     │
           └────────────────────────────┘
```

### 4.3 Key Architectural Changes

#### 4.3.1 Camera Abstraction Layer

**New Classes:**
```cpp
// camera_manager.h
class camera_instance {
    int32_t camera_id;
    std::string sensor_name;
    std::shared_ptr<vi> vi_instance;
    ot_vpss_grp vpss_grp;
    std::vector<std::shared_ptr<stream_instance>> streams;
    std::map<std::string, std::shared_ptr<feature_plugin>> features;
};

class camera_manager {
    static std::map<int32_t, std::shared_ptr<camera_instance>> cameras;
    static std::mutex camera_mutex;
    
    static bool create_camera(camera_config& config);
    static bool delete_camera(int32_t camera_id);
    static std::shared_ptr<camera_instance> get_camera(int32_t camera_id);
};
```

**Benefits:**
- Dynamic camera creation/deletion
- Independent camera lifecycles
- Per-camera configuration isolation
- Scalable to hardware limits

#### 4.3.2 Stream Abstraction

**New Classes:**
```cpp
// stream_instance.h
class stream_instance {
    int32_t camera_id;
    int32_t stream_id;
    std::string stream_name;  // e.g., "main", "sub", "ai"
    std::shared_ptr<venc> encoder;
    stream_config config;
    
    std::shared_ptr<stream_stock> rtsp_stock;
    std::list<std::shared_ptr<rtmp_session>> rtmp_sessions;
    std::shared_ptr<mp4_recorder> recorder;
};

class stream_router {
    std::map<std::pair<int32_t, int32_t>, std::shared_ptr<stream_instance>> streams;
    
    bool register_stream(stream_instance& stream);
    bool unregister_stream(int32_t camera_id, int32_t stream_id);
    std::shared_ptr<stream_instance> get_stream(int32_t camera_id, int32_t stream_id);
};
```

**Benefits:**
- Dynamic stream creation per camera
- Flexible stream naming
- Independent stream configuration
- Stream lifecycle management

#### 4.3.3 Resource Manager

**New Classes:**
```cpp
// resource_manager.h
class resource_manager {
    struct resource_limits {
        int32_t max_cameras;
        int32_t max_vpss_groups;
        int32_t max_venc_channels;
        int32_t max_vi_devices;
        vb_pool_config vb_config;
    };
    
    resource_limits limits;
    std::map<int32_t, bool> vpss_group_allocated;
    std::map<int32_t, bool> venc_channel_allocated;
    
    bool allocate_vpss_group(int32_t& grp);
    void free_vpss_group(int32_t grp);
    bool allocate_venc_channel(int32_t& chn);
    void free_venc_channel(int32_t chn);
};
```

**Benefits:**
- Centralized resource tracking
- Prevents over-allocation
- Validates configurations before creation
- Hardware limits enforced

#### 4.3.4 RTSP URL Scheme

**Current:**
```
rtsp://ip:port/stream1   → (chn=0, stream_id=0)
rtsp://ip:port/stream2   → (chn=0, stream_id=1)
rtsp://ip:port/stream3   → (chn=0, stream_id=2)
```

**Proposed:**
```
rtsp://ip:port/camera/0/main    → (camera_id=0, stream="main")
rtsp://ip:port/camera/0/sub     → (camera_id=0, stream="sub")
rtsp://ip:port/camera/1/main    → (camera_id=1, stream="main")
rtsp://ip:port/camera/1/sub     → (camera_id=1, stream="sub")

Or alternatively:
rtsp://ip:port/cam0_main
rtsp://ip:port/cam0_sub
rtsp://ip:port/cam1_main
```

**Benefits:**
- Intuitive URL structure
- Self-documenting camera/stream mapping
- Supports arbitrary stream names
- Backward compatibility via aliases

#### 4.3.5 Configuration Structure

**Current:** Multiple JSON files (vi.json, venc.json, etc.)

**Proposed:** Unified configuration with per-camera sections

```json
{
  "system": {
    "max_cameras": 4,
    "vb_pool": { ... }
  },
  "cameras": [
    {
      "camera_id": 0,
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
          "rtsp": {
            "enabled": true,
            "url_path": "camera/0/main"
          },
          "rtmp": {
            "enabled": true,
            "urls": ["rtmp://server/live/cam0_main"]
          }
        },
        {
          "stream_id": 1,
          "name": "sub",
          ...
        }
      ],
      "features": {
        "osd": { "enabled": true },
        "aiisp": { "enabled": false },
        "yolov5": { "enabled": false }
      }
    },
    {
      "camera_id": 1,
      ...
    }
  ]
}
```

### 4.4 Migration Strategy

**Phase 1: Abstraction Layer (Non-breaking)**
- Introduce camera_manager and camera_instance classes
- Wrap existing g_chn in camera_instance
- Keep MAX_CHANNEL = 1 initially
- Refactor access through camera_manager

**Phase 2: Multi-Channel Support**
- Increase MAX_CHANNEL to 4-8
- Implement dynamic camera creation
- Update main.cpp to create multiple cameras
- Test with 2-camera setup

**Phase 3: Stream Refactoring**
- Implement stream_instance and stream_router
- Refactor stream_manager to use stream_router
- Update RTSP URL parsing for new scheme
- Maintain backward compatibility

**Phase 4: Configuration Migration**
- Implement unified configuration format
- Add config validation and migration tool
- Support both old and new config formats
- Update documentation

**Phase 5: Feature Plugins**
- Extract OSD, AIISP, YOLOv5 into plugins
- Implement per-camera feature management
- Add runtime enable/disable support

---

## 5. Refactoring Action Items

### 5.1 High Priority (Core Multi-Camera Support)

#### Device Module Refactoring

**Item 1: Remove MAX_CHANNEL Hardcoding**
- **File:** `device/dev_chn.h`
- **Change:** Replace `#define MAX_CHANNEL 1` with dynamic limit
- **Impact:** Enables multiple channel instances
- **Effort:** Medium
- **Dependencies:** Must update all array accesses

**Item 2: Create camera_manager Class**
- **File:** New `device/camera_manager.h/cpp`
- **Purpose:** Central registry for camera instances
- **Interface:**
  ```cpp
  class camera_manager {
      static bool init(int max_cameras);
      static std::shared_ptr<camera_instance> create_camera(camera_config& cfg);
      static bool destroy_camera(int camera_id);
      static std::shared_ptr<camera_instance> get_camera(int camera_id);
  };
  ```
- **Effort:** High
- **Dependencies:** Requires camera_instance class

**Item 3: Refactor chn Class → camera_instance**
- **File:** `device/dev_chn.h/cpp` → `device/camera_instance.h/cpp`
- **Changes:**
  - Remove global static `g_chns[]` array
  - Remove singleton pattern
  - Add camera_id member variable
  - Isolate per-camera state
- **Effort:** High
- **Dependencies:** Affects main.cpp initialization

**Item 4: Implement Resource Manager**
- **File:** New `device/resource_manager.h/cpp`
- **Purpose:** Track and allocate hardware resources
- **Features:**
  - VPSS group allocation
  - VENC channel allocation
  - VB pool management
  - Resource limit enforcement
- **Effort:** High

**Item 5: Multi-Camera VENC Capture**
- **File:** `device/dev_venc.cpp`
- **Current:** Single static thread and venc list
- **Change:** One thread per camera or improved select loop
- **Consideration:** Thread overhead vs. complexity
- **Effort:** Medium

#### RTSP Module Refactoring

**Item 6: Dynamic Stream Registration**
- **File:** `rtsp/stream/stream_manager.cpp`
- **Change:** Remove hardcoded stream assumptions
- **Interface:**
  ```cpp
  bool register_stream(int camera_id, std::string stream_name, stream_config cfg);
  bool unregister_stream(int camera_id, std::string stream_name);
  ```
- **Effort:** Medium

**Item 7: URL Parser Enhancement**
- **File:** `rtsp/rtsp_request_handler.cpp`
- **Change:** Parse `/camera/<id>/<stream>` URLs
- **Backward Compatibility:** Support legacy `/stream1` format
- **Effort:** Low

**Item 8: Stream Instance Class**
- **File:** New `rtsp/stream/stream_instance.h/cpp`
- **Purpose:** Encapsulate stream state and lifecycle
- **Effort:** Medium

#### RTMP Module Refactoring

**Item 9: Dynamic URL Template Support**
- **File:** `rtmp/session_manager.cpp`
- **Change:** Support `{camera_id}` and `{stream_name}` placeholders
- **Example:** `rtmp://server/live/cam{camera_id}_{stream_name}`
- **Effort:** Low

**Item 10: H.265 Support Investigation**
- **File:** `rtmp/session.cpp`
- **Current:** H.264 only
- **Challenge:** RTMP spec doesn't officially support H.265
- **Options:** FLV extended format or alternative container
- **Effort:** High (requires protocol research)

### 5.2 Medium Priority (Enhanced Features)

**Item 11: Configuration System Redesign**
- **Files:** New `config/config_manager.h/cpp`
- **Features:**
  - Unified JSON schema
  - Runtime reload support
  - Validation before apply
  - Migration from old format
- **Effort:** High

**Item 12: Feature Plugin System**
- **Files:** New `feature/feature_plugin.h` (interface)
- **Plugins:** osd_plugin, aiisp_plugin, yolov5_plugin
- **Benefits:**
  - Per-camera feature control
  - Runtime enable/disable
  - Reduced coupling
- **Effort:** High

**Item 13: Stream Health Monitoring**
- **Enhancement:** Improve 5-second timeout detection
- **Features:**
  - Bitrate monitoring
  - Frame drop detection
  - Automatic recovery
- **Effort:** Medium

**Item 14: Dynamic Reconfiguration**
- **Capability:** Change encoder settings without restart
- **Requirements:**
  - Stop stream → Reconfigure → Restart stream
  - Client notification (RTSP ANNOUNCE)
- **Effort:** High

### 5.3 Low Priority (Optimization & Polish)

**Item 15: Thread Pool for VENC Capture**
- **Replace:** Multiple threads or single thread per camera
- **Benefits:** Better CPU utilization
- **Effort:** Medium

**Item 16: Zero-Copy Optimization**
- **Area:** Stream data distribution
- **Technique:** Shared pointers instead of memcpy
- **Effort:** Medium

**Item 17: Metrics & Telemetry**
- **Add:** Prometheus-style metrics endpoint
- **Metrics:** Bitrate, FPS, client count, errors
- **Effort:** Medium

**Item 18: RESTful API**
- **Purpose:** Runtime control and monitoring
- **Endpoints:**
  - GET /api/cameras
  - POST /api/cameras/{id}/streams
  - GET /api/system/resources
- **Effort:** High

---

## 6. Multi-Camera Design Details

### 6.1 Resource Constraints (HiSilicon 3519DV500)

**Hardware Limits:**
- VI Devices: 4 (dev0-dev3)
- VPSS Groups: 32 (grp0-grp31)
- VENC Channels: 16 total (4 H.265 + 12 H.264, or similar mix)
- ISP: 4 pipelines (one per VI)
- SVP: 1 NNIE core (shared for AIISP/YOLOv5)
- VB Pools: 256MB-512MB typical

**Practical Limits:**
- **Max Cameras:** 4 (limited by VI devices)
- **Max Streams per Camera:** 2-4 (limited by VENC channels)
- **Total Streams:** 8-12 realistic (encoding performance)

### 6.2 Example Multi-Camera Configuration

**Scenario: 2-Camera Surveillance System**

**Camera 0 (Front Door):**
- Sensor: OS04A10 (2688x1520@30fps)
- Main Stream: 1920x1080@30fps, H.264 CBR 4Mbps → RTSP, RTMP
- Sub Stream: 720x576@25fps, H.264 CBR 1Mbps → RTSP
- Features: OSD timestamp, 24/7 MP4 recording

**Camera 1 (Parking Lot):**
- Sensor: OS08A20 (3840x2160@30fps)
- Main Stream: 3840x2160@30fps, H.265 CBR 8Mbps → RTSP
- Sub Stream: 1920x1080@30fps, H.265 CBR 3Mbps → RTSP, RTMP
- AI Stream: 640x640@30fps → YOLOv5 → RTSP (bbox overlay)
- Features: OSD, YOLOv5 detection, event-triggered JPEG

**Resource Allocation:**
```
Camera 0:
- VI0 (OS04A10) → VPSS0 → VENC0 (main), VENC1 (sub)

Camera 1:
- VI1 (OS08A20) → VPSS1 → VENC2 (main), VENC3 (sub), VENC4 (ai)
                        → SVP (YOLOv5)

Total Resources:
- VI: 2/4
- VPSS Groups: 2/32
- VENC: 5/16
- SVP: 1/1
```

### 6.3 Data Flow in Multi-Camera System

```
Camera 0                              Camera 1
   │                                      │
   ├─[VI0]──[ISP0]──[VPSS0]              ├─[VI1]──[ISP1]──[VPSS1]──[SVP]
   │         │        ├─[Chn0]───[VENC0] │         │        ├─[Chn0]───[VENC2]
   │         │        ├─[Chn1]───[VENC1] │         │        ├─[Chn1]───[VENC3]
   │         │        └─[Chn2]───[OSD0]  │         │        ├─[Chn2]───[VENC4]
   │         │                            │         │        └─[Chn3]───[VO]
   │         └────────[Scene Auto]       │         └────────[Scene Auto]
   │                                      │
   └──────────[stream_observer]──────────┴──────────[stream_observer]
                      │                                     │
              ┌───────┴─────────┐                  ┌───────┴─────────┐
              │                 │                  │                 │
         [RTSP Server]    [RTMP Session]      [RTSP Server]    [RTMP Session]
         /camera/0/main   cam0_main           /camera/1/main   cam1_main
         /camera/0/sub    -                   /camera/1/sub    cam1_sub
                                              /camera/1/ai     -
```

---

## 7. Testing Strategy

### 7.1 Unit Tests

**Device Module:**
- camera_manager: create, destroy, get operations
- resource_manager: allocation, deallocation, limit enforcement
- camera_instance: lifecycle, stream management

**RTSP Module:**
- URL parser: multi-camera URL schemes
- stream_manager: dynamic registration
- stream_router: lookup and dispatch

**RTMP Module:**
- URL template expansion
- Multi-session management

### 7.2 Integration Tests

**Single Camera (Regression):**
- Verify existing functionality still works
- Test all supported sensors
- Verify all encoding modes
- Test RTSP, RTMP, MP4, JPEG

**Dual Camera:**
- Create two camera instances
- Verify independent operation
- Test concurrent encoding
- Verify RTSP multi-camera access

**Resource Limits:**
- Attempt to exceed VPSS group limit
- Attempt to exceed VENC channel limit
- Verify proper error handling

### 7.3 Performance Tests

**Metrics:**
- Frame rate stability
- Encoding latency
- Network throughput
- CPU usage (multi-core utilization)
- Memory consumption

**Load Tests:**
- 4 cameras simultaneously
- Multiple RTSP clients per stream
- RTMP + RTSP + MP4 + YOLOv5 concurrently

---

## 8. Risk Assessment

### 8.1 Technical Risks

**Risk 1: Hardware Resource Exhaustion**
- **Probability:** High
- **Impact:** High
- **Mitigation:** Resource manager with strict limits, graceful degradation

**Risk 2: Performance Degradation**
- **Probability:** Medium
- **Impact:** High
- **Mitigation:** Profiling, optimization, load testing

**Risk 3: Breaking Changes**
- **Probability:** Medium
- **Impact:** Medium
- **Mitigation:** Phased migration, backward compatibility layer

**Risk 4: Configuration Complexity**
- **Probability:** Medium
- **Impact:** Low
- **Mitigation:** Validation, clear documentation, examples

### 8.2 Implementation Risks

**Risk 5: Development Timeline**
- **Probability:** High
- **Impact:** Medium
- **Mitigation:** Prioritized refactoring, incremental delivery

**Risk 6: Testing Coverage**
- **Probability:** Medium
- **Impact:** Medium
- **Mitigation:** Automated test suite, hardware test bench

---

## 9. Conclusion

### 9.1 Summary

The current codebase is well-structured for single-camera operation but has fundamental limitations preventing multi-camera support:

1. **MAX_CHANNEL = 1** hardcoded limit
2. **Global singleton state** (g_chn)
3. **Static resource allocation**
4. **Tight coupling** between layers

The proposed architecture addresses these through:

1. **Dynamic camera management** (camera_manager)
2. **Resource abstraction** (resource_manager)
3. **Stream router** for flexible mapping
4. **Plugin system** for features
5. **Unified configuration** schema

### 9.2 Recommended Approach

**Immediate (1-2 months):**
- Implement camera_manager and camera_instance
- Remove MAX_CHANNEL hardcoding
- Test with 2-camera setup

**Short-term (3-6 months):**
- Refactor RTSP URL scheme
- Implement resource_manager
- Add configuration migration

**Long-term (6-12 months):**
- Feature plugin system
- RESTful API
- Performance optimization

### 9.3 Success Criteria

- ✅ Support 2+ cameras simultaneously
- ✅ Independent camera configuration
- ✅ Dynamic stream creation
- ✅ Backward compatibility maintained
- ✅ Performance within 10% of single camera
- ✅ Resource limits enforced
- ✅ Comprehensive test coverage

---

## Appendices

### Appendix A: File Structure

**Current Structure:**
```
hi_app/
├── rtsp/           (90 files, ~10K LOC)
├── rtmp/           (6 files, ~1K LOC)
├── device/         (42 files, ~15K LOC)
├── util/           (Common utilities)
├── json/           (JSON parser)
├── log/            (Logging)
├── main.cpp        (1K LOC)
└── Makefile
```

**Proposed Structure:**
```
hi_app/
├── core/
│   ├── camera_manager/
│   ├── resource_manager/
│   ├── config_manager/
│   └── application.cpp
├── device/
│   ├── hal/           (Hardware abstraction)
│   ├── vi/            (Video input)
│   ├── venc/          (Video encoding)
│   ├── vpss/          (Video processing)
│   └── camera_instance/
├── streaming/
│   ├── rtsp/
│   ├── rtmp/
│   └── stream_router/
├── features/
│   ├── osd/
│   ├── aiisp/
│   ├── yolov5/
│   └── recorder/
└── util/
```

### Appendix B: Key Data Structures

**Stream Identification:**
```cpp
struct stream_key {
    int32_t camera_id;
    int32_t stream_id;      // Or string name
};
```

**Resource Tracking:**
```cpp
struct allocated_resources {
    int32_t vi_dev;
    int32_t vpss_grp;
    int32_t venc_chn;
    std::vector<int32_t> vb_pools;
};
```

**Stream Configuration:**
```cpp
struct stream_config {
    std::string name;
    encoder_type type;      // H264_CBR, H265_AVBR, etc.
    int32_t width, height;
    int32_t framerate;
    int32_t bitrate;
    bool rtsp_enabled;
    std::vector<std::string> rtmp_urls;
};
```

### Appendix C: Configuration Examples

See `CONFIGURATION_EXAMPLES.md` for complete configuration examples.

### Appendix D: API Reference

See `API_REFERENCE.md` for detailed API documentation.

---

## Document Metadata

- **Author:** Code Analysis System
- **Date:** 2026-01-04
- **Version:** 1.0
- **Repository:** kiddy818/hi_app
- **Target Platform:** HiSilicon 3519DV500
- **SDK Version:** V2.0.2.1

---

**End of Analysis Document**
