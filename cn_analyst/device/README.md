# 设备模块重构

本目录将包含用于多摄像头支持的重构设备模块实现。

## 状态

**当前状态：** 阶段 2 进行中 - 迁移包装器已实现  
**目标完成：** 阶段 1-2（第 1-8 周）

**阶段 1 完成：** ✅ 核心抽象（resource_manager、camera_manager、camera_instance）已实现并测试  
**阶段 2 进展：** 🔄 迁移包装器（dev_chn_wrapper）已创建，正在集成测试

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
资源                  总量                最多/每摄像头       典型用法
--------------------------------------------------------------------------------
VI Devices            4                  1                 1 每摄像头
VPSS Groups           32                 1-2               1 每摄像头
VPSS Channels         128 (4 per grp)    2-4               2-3 每摄像头
VENC Channels         16 total           4-8               2-3 每摄像头
  ├─ H.265            4-8 channels       2-4               1-2 每摄像头
  └─ H.264            8-12 channels      2-6               1-2 每摄像头
ISP Pipelines         4                  1                 1 每摄像头
SVP (NNIE) Core       1 (shared)         N/A               AI 共享
VB Memory             256-512 MB         64-128 MB         根据方案变化
--------------------------------------------------------------------------------
实际限定: 最多4个摄像头
推荐: 2-4个摄像头，2-4流/摄像头
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

## 重构 检查表

### 阶段 1: Core Abstractions （第 1-4 周）

#### 第 2 周: Resource Manager
- [x] 设计 resource_manager 接口
- [x] 实现 VPSS group 追踪
- [x] 实现 VENC channel 追踪
- [x] 实现 VI device 追踪
- [x] 添加 验证逻辑
- [x] 编写 单元测试
- [x] 集成测试

#### 第 3 周: Camera Manager & 实例
- [x] 设计 camera_manager 接口
- [x] 设计 camera_instance 类
- [x] 实现摄像头生命周期
- [x] 实现资源分配
- [x] 添加验证逻辑
- [x] 编写单元测试
- [x] 集成测试

#### 第 4 周: Stream Management
- [ ] 设计 stream_instance 类
- [ ] 和 camera_instance 集成
- [ ] 实现 stream 创建/销毁
- [ ] 连接到 RTSP/RTMP 的路由
- [ ] 编写单元测试
- [ ] 集成测试

### 阶段 2: Multi-Camera Support （第 5-8 周）

#### 第 5 周: 移除 MAX_CHANNEL
- [x] 创建迁移包装器 `dev_chn_wrapper`
- [x] 标记 `dev_chn` 为已弃用
- [x] 更新 Makefile 以包含新设备模块
- [ ] 移除 `#define MAX_CHANNEL 1`
- [ ] 用 camera_manager 替换 `g_chns[]` 数组
- [ ] 更新 所有引用
- [ ] 重构 main.cpp 中的初始化 
- [ ] 回归测试 单摄像头

#### 第 6 周: Configuration System
- [ ] 设计 通用配置
- [ ] 实现 配置解析
- [ ] 添加 验证逻辑
- [ ] 创建 迁移工具
- [ ] 测试 配置加载

#### 第 7 周: Multi-VENC Capture
- [ ] 重构 VENC 捕获线程
- [ ] 支持多个摄像头
- [ ] 测试并发编码
- [ ] 性能优化

#### 第 8 周: 集成测试
- [ ] 测试创建2个摄像头
- [ ] 测试创建4个摄像头
- [ ] 性能摸底
- [ ] 压力测试
- [ ] 文档

## 测试策略

### 单元测试

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

### 集成测试

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

### 性能测试

**指标:**
- Frame rate stability (30fps target)
- Encoding latency (<50ms)
- CPU usage (<80% on quad-core)
- Memory usage (<512MB total)
- Startup time (<5 seconds per camera)

**测试场景:**
- 1 camera, 4 streams
- 2 cameras, 2 streams each
- 4 cameras, 2 streams each
- All features enabled

## 迁移路径

### Step 1: 封装层 (第 5 周) ✅ 已完成
为当前代码创建兼容封装适配层:
```cpp
// device/dev_chn_wrapper.h (已实现)
class chn_wrapper : public ceanic::util::stream_observer {
    std::shared_ptr<camera_instance> m_camera_instance;
    std::shared_ptr<chn> m_legacy_chn;  // 回退支持
public:
    chn_wrapper(const char* sensor, const char* mode, int chn_no);
    bool start(...);
    void stop();
    // ... delegate to camera_instance or legacy chn
};
```

**已实现功能:**
- ✅ 完整的 dev_chn API 兼容性
- ✅ 自动回退到旧实现（当 camera_manager 未初始化时）
- ✅ 透明委托到 camera_manager/camera_instance
- ✅ 支持所有现有功能（AIISP、YOLOv5、VO 等）

### Step 2: 更新 main.cpp (第 5-6 周) 🔄 进行中
```cpp
// Option 1: 使用包装器（最小更改）
#include "dev_chn_wrapper.h"
using chn_type = hisilicon::dev::chn_wrapper;  // 简单替换

std::shared_ptr<chn_type> g_chn;
chn_type::init(mode);  // 初始化新架构
g_chn = std::make_shared<chn_type>("OS04A10", "H264_CBR", 0);
g_chn->start(...);

// Option 2: 直接使用新 API（推荐用于新代码）
camera_manager::init(4);  // 支持up to 4 cameras
camera_config cfg = load_camera_config(0);
auto camera = camera_manager::create_camera(cfg);
camera->start();
```

### Step 3: 移除 Legacy Code (第 8 周+)
新的实现都测试确认后，逐步移除旧的代码:
- 移除封装 `dev_chn.{h,cpp}`
- 更新相关参考
- 移除兼容性层
- 完全迁移到新架构

## 已知问题与限制

### Current Issues (预重构分析)
1. 硬编码的 `MAX_CHANNEL = 1`
2. 静态全局变量 `g_chn`
3. 静态数组 `g_chns[]`
4. 不支持资源追踪
5. 不支持多摄像头 

### 重构分析后的改进
1. ✅ 动态摄像头分配
2. ✅ 资源管理执行限制
3. ✅ 每摄像头配置
4. ✅ 独立的摄像头生命周期
5. ✅ 可扩展的硬件限制（4个摄像头）

## 性能考虑

### CPU 使用
- Target: <80% on quad-core ARMv8
- VI/VPSS: Hardware accelerated
- VENC: Hardware accelerated
- ISP: Hardware pipeline
- Software overhead: <10% per camera

### 内存使用
- Target: <512MB total
- Per camera: ~64-128MB (varies by resolution)
- VB pools: Pre-allocated based on config
- Stream buffers: Shared pointers (minimal copy)

### 延迟
- VI capture: <10ms
- VPSS processing: <10ms
- VENC encoding: <30ms
- Total camera to encoded: <50ms

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

### 资源映射举例
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
- dynamic resolution switching
- Hardware failover (camera swap)
- Advanced ISP tuning for each camera
- ROI (Region of Interest) encoding
- Smart encoding (save bandwidth)

## 参考资料

### Related 文档化s
- [整体分析](../ANALYSIS.md)
- [重构路径](../REFACTORING_ROADMAP.md)
- [RTSP重构](../rtsp/README.md)
- [RTMP重构](../rtmp/README.md)

### 外部资源
- HiSilicon MPP SDK Documentation
- ISP Calibration Guide
- VPSS Configuration Manual
- VENC Optimization Guide

---

**Last 更新d:** 2026-01-06  
**状态：** 阶段 2 进行中 - 迁移包装器已完成  
**优先级：** 关键 Path  
**Next Review:** 第 5-6 周 main.cpp 重构和集成测试
