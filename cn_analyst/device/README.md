# 设备模块重构

本目录将包含用于多摄像头支持的重构设备模块实现。

## 状态

**当前状态：** 未填充 - 等待实施  
**目标完成：** 阶段 1-2（第 1-8 周）

## 计划组件

### 核心类

#### `camera_manager.h/cpp`
- **目的：** 摄像头实例的中央注册表和工厂
- **职责：**
  - 创建/销毁 摄像头实例
  - 跟踪活动摄像头
  - 强制执行摄像头限制
  - 与 resource_manager 协调
- **状态：** 尚未实施
- **优先级：** 关键 (阶段 1, 第 3 周)

#### `camera_instance.h/cpp`
- **目的：** 代表单个摄像头及其所有组件
- **职责：**
  - 管理 VI（视频输入）生命周期
  - 管理 VPSS（视频处理）管道
  - 管理 VENC（编码器）用于多个流
  - 协调特性（OSD、AIISP 等）
  - 实现用于流分发的观察者模式
- **状态：** 尚未实施
- **优先级：** 关键 (阶段 1, 第 3 周)

#### `resource_manager.h/cpp`
- **目的：** 跟踪和分配硬件资源
- **职责：**
  - VPSS 组分配（32 个组可用）
  - VENC 通道分配（16 个通道可用）
  - VI 设备分配（4 个设备可用）
  - VB 池管理
  - 资源限制强制执行
- **状态：** 尚未实施
- **优先级：** 关键 (阶段 1, 第 2 周)

#### `stream_config.h/cpp`
- **目的：** 流配置和验证
- **职责：**
  - 解析流配置
  - 验证分辨率/帧率/比特率
  - 编码器参数验证
- **状态：** 尚未实施
- **优先级：** 高 (阶段 1, 第 3 周)

### 修改的类

#### `dev_chn` （现有） → `camera_instance` （新建）
- **所需更改：**
  - 移除静态 `g_chns[]` 数组
  - 移除 `MAX_CHANNEL = 1` 限制
  - 添加 camera_id 成员
  - 使用 resource_manager 进行分配
  - 隔离每个摄像头状态
- **向后兼容性：** 用于平滑迁移的包装类
- **状态：** 等待重构 (阶段 2, 第 5 周)

#### `dev_venc` （现有）
- **所需更改：**
  - 移除 静态 `g_vencs` 列表
  - 支持每摄像头 VENC 捕获线程 (或改进的单线程)
  - 通过 resource_manager 动态 VENC 分配
- **向后兼容性：** 是
- **状态：** 等待重构 (阶段 2, 第 7 周)

#### `dev_vi` （现有）
- **所需更改：**
  - 支持多个 VI 实例
  - 动态 VI 设备分配
  - 每摄像头 ISP 管道
- **向后兼容性：** 是
- **状态：** 等待重构 (阶段 2, 第 5 周-6)

## 硬件资源约束

### 海思 3519DV500 限制
```
Resource              Total Available    每摄像头 Max    Typical Usage
--------------------------------------------------------------------------------
VI Devices            4                  1                 1 每摄像头
VPSS Groups           32                 1-2               1 每摄像头
VPSS Channels         128 (4 per grp)    2-4               2-3 每摄像头
VENC Channels         16 total           4-8               2-3 每摄像头
  ├─ H.265            4-8 channels       2-4               1-2 每摄像头
  └─ H.264            8-12 channels      2-6               1-2 每摄像头
ISP Pipelines         4                  1                 1 每摄像头 (max 4 cameras)
SVP (NNIE) Core       1 (shared)         N/A               Shared for AI
VB Memory             256-512 MB         64-128 MB         Varies by resolution
--------------------------------------------------------------------------------
Practical Limit:      4 cameras max
Recommended:          2-4 cameras with 2-4 streams each
```

## Resource Manager 设计

### 接口
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
    静态 resource_status get_status();
    static bool can_create_camera(const camera_config& cfg);
    
private:
    static std::map<int32_t, bool> m_vpss_allocated;
    static std::map<int32_t, bool> m_venc_allocated;
    static std::map<int32_t, bool> m_vi_allocated;
    static std::mutex m_mutex;
    静态 resource_limits m_limits;
};
```

### 资源状态
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

## Camera Manager 设计

### 接口
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

## Camera 实例 设计

### 接口
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
    
    // 线程 safety
    mutable std::mutex m_mutex;
};
```

### 资源跟踪
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

## 配置架构

### 摄像头配置
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

### 流配置
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

## 重构ing Checklist

### 阶段 1: Core Abstractions （第 1-4 周）

#### 第 2 周: Resource Manager
- [ ] 设计 resource_manager 接口
- [ ] 实现 VPSS group tracking
- [ ] 实现 VENC channel tracking
- [ ] 实现 VI device tracking
- [ ] 添加 验证 logic
- [ ] 编写 unit tests
- [ ] Integration testing

#### 第 3 周: Camera Manager & 实例
- [ ] 设计 camera_manager 接口
- [ ] 设计 camera_instance class
- [ ] 实现 camera lifecycle
- [ ] 实现 资源分配
- [ ] 添加 验证 logic
- [ ] 编写 unit tests
- [ ] Integration testing

#### 第 4 周: Stream Management
- [ ] 设计 stream_instance class
- [ ] Integrate with camera_instance
- [ ] 实现 stream creation/destruction
- [ ] Connect to RTSP/RTMP routers
- [ ] 编写 unit tests
- [ ] Integration testing

### 阶段 2: Multi-Camera Support （第 5-8 周）

#### 第 5 周: 移除 MAX_CHANNEL
- [ ] 移除 `#define MAX_CHANNEL 1`
- [ ] Replace `g_chns[]` 数组 with camera_manager
- [ ] 更新 all references
- [ ] 重构 initialization in main.cpp
- [ ] 测试 with 单摄像头 (regression)

#### 第 6 周: Configuration System
- [ ] 设计 unified 配置 schema
- [ ] 实现 config parser
- [ ] 添加 验证 logic
- [ ] 创建 migration tool
- [ ] 测试 配置 loading

#### 第 7 周: Multi-VENC Capture
- [ ] 重构 VENC capture 线程
- [ ] 支持多个 cameras
- [ ] 测试 concurrent encoding
- [ ] Performance optimization

#### 第 8 周: Integration 测试ing
- [ ] 测试 2-camera setup
- [ ] 测试 4-camera setup
- [ ] Performance benchmarking
- [ ] Stress testing
- [ ] 文档化ation

## 测试ing Strategy

### Unit 测试s

**resource_manager_test.cpp**
- Allocate/free resources
- Exhaustion scenarios
- Concurrent access
- Validation logic

**camera_manager_test.cpp**
- 创建/销毁 cameras
- Camera lookup
- Limit enforcement
- 线程 safety

**camera_instance_test.cpp**
- Lifecycle (start/stop)
- Stream management
- Feature management
- Observer pattern

### Integration 测试s

**single_camera_test.cpp** (Regression)
- All sensors supported
- All encoding modes
- All features
- Verify 向后兼容性

**dual_camera_test.cpp**
- Two cameras simultaneously
- Independent 配置
- Concurrent encoding
- Resource isolation

**multi_camera_test.cpp**
- Four cameras (max limit)
- Resource exhaustion handling
- Performance under load

### Performance 测试s

**Metrics:**
- Frame rate stability (30fps target)
- Encoding latency (<50ms)
- CPU usage (<80% on quad-core)
- Memory usage (<512MB total)
- Startup time (<5 seconds 每摄像头)

**测试 Scenarios:**
- 1 camera, 4 streams
- 2 cameras, 2 streams each
- 4 cameras, 2 streams each
- All features enabled

## 迁移路径

### Step 1: Wrapper Layer (第 5 周)
创建 compatibility wrapper for existing code:
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

### Step 2: 更新 main.cpp (第 5 周)
```cpp
// Old way
std::shared_ptr<hisilicon::dev::chn> g_chn;
g_chn = std::make_shared<hisilicon::dev::chn>("OS04A10", "H264_CBR", 0);

// New way
camera_manager::init(4);  // 支持up to 4 cameras
camera_config cfg = load_camera_config(0);
auto camera = camera_manager::create_camera(cfg);
```

### Step 3: 移除 Legacy Code (第 8 周+)
Once all tests pass with new 实现:
- 移除 `dev_chn.{h,cpp}` wrapper
- 更新 all references
- 移除 compatibility layer

## 已知问题与限制

### Current Issues (Pre-重构ing)
1. `MAX_CHANNEL = 1` 硬编码的
2. 静态 `g_chn` global variable
3. 静态 `g_chns[]` 数组
4. 否 resource tracking
5. 否 多摄像头 support

### Post-重构ing Improvements
1. ✅ 动态 camera 分配
2. ✅ Resource manager enforces limits
3. ✅ 每摄像头 配置
4. ✅ Independent camera lifecycle
5. ✅ Scalable to hardware limits (4 cameras)

## 性能考虑

### CPU 使用
- Target: <80% on quad-core ARMv8
- VI/VPSS: Hardware accelerated
- VENC: Hardware accelerated
- ISP: Hardware 管道
- Software overhead: <10% 每摄像头

### 内存使用
- Target: <512MB total
- 每摄像头: ~64-128MB (varies by resolution)
- VB pools: Pre-allocated based on config
- Stream buffers: Shared pointers (minimal copy)

### 延迟
- VI capture: <10ms
- VPSS processing: <10ms
- VENC encoding: <30ms
- Total camera to 编码的: <50ms

## 硬件绑定

### VI → VPSS → VENC 管道
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

## 未来增强

### 阶段 4+
- Hot-plug camera support
- 动态 resolution switching
- Hardware failover (camera swap)
- Advanced ISP tuning 每摄像头
- ROI (Region of Interest) encoding
- Smart encoding (save bandwidth)

## 参考资料

### Related 文档化s
- [Main Analysis](../ANALYSIS.md)
- [重构ing Roadmap](../REFACTORING_ROADMAP.md)
- [RTSP 重构ing](../rtsp/README.md)
- [RTMP 重构ing](../rtmp/README.md)

### 外部资源
- HiSilicon MPP SDK 文档化ation
- ISP Calibration Guide
- VPSS Configuration Manual
- VENC Optimization Guide

---

**Last 更新d:** 2026-01-04  
**状态：** Planning Phase  
**优先级：** 关键 Path  
**Next Review:** 第 2 周 实现ation Kickoff
