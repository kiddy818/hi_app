#include "camera_instance.h"
#include <algorithm>

#include "dev_log.h"

namespace hisilicon {
namespace device {

// Stream instance placeholder (minimal implementation for now)
class stream_instance {
public:
    stream_instance(int32_t stream_id, const stream_config& config)
        : m_stream_id(stream_id), m_config(config), m_is_running(false) {}
    
    int32_t stream_id() const { return m_stream_id; }
    stream_config get_config() const { return m_config; }
    bool is_running() const { return m_is_running; }
    
    bool start() {
        m_is_running = true;
        return true;
    }
    
    void stop() {
        m_is_running = false;
    }
    
private:
    int32_t m_stream_id;
    stream_config m_config;
    bool m_is_running;
};

// Camera Instance Implementation

camera_instance::camera_instance(int32_t camera_id, const camera_config& config)
    : m_camera_id(camera_id)
    , m_config(config)
    , m_is_running(false)
{
    m_vi_ptr = nullptr;
}

camera_instance::~camera_instance() {
    stop();
}

bool camera_instance::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_is_running) {
        return false; // Already running
    }
    
    // Step 1: Allocate hardware resources
    if (!allocate_resources()) {
        free_resources();
        return false;
    }
    
    // Step 2: Initialize VI (Video Input)
    if (!init_vi()) {
        free_resources();
        return false;
    }
    
    // Step 3: Initialize VPSS (Video Processing)
    if (!init_vpss()) {
        free_resources();
        return false;
    }
    
    // Step 4: Initialize streams and encoders
    if (!init_streams()) {
        free_resources();
        return false;
    }
    
    // Step 5: Initialize features (OSD, AIISP, etc.)
    if (!init_features()) {
        // Features are optional, so we don't fail if they can't be initialized
        // Just log a warning (would be done in init_features)
    }
    
    m_is_running = true;
    return true;
}

void camera_instance::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_is_running) {
        return;
    }
    
    // Stop all streams
    for (auto& pair : m_streams) {
        if (pair.second) {
            pair.second->stop();
        }
    }
    m_streams.clear();
    
    // Disable all features
    m_enabled_features.clear();
    
    // Free hardware resources
    free_resources();
    
    m_is_running = false;
}

bool camera_instance::is_running() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_is_running;
}

bool camera_instance::create_stream(const stream_config& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return create_stream_internal(config);
}

bool camera_instance::create_stream_internal(const stream_config& config) {
    // Validate stream config
    std::string error_msg;
    if (!stream_config_helper::validate(config, error_msg)) {
        // In real implementation, would log error_msg
        return false;
    }
    
    // Check if stream already exists
    if (m_streams.find(config.stream_id) != m_streams.end()) {
        return false; // Stream already exists
    }
    
    // Allocate VENC channel
    int32_t venc_chn;
    venc_type vtype = stream_config_helper::is_h264(config.type) 
        ? venc_type::H264 : venc_type::H265;
    
    if (!resource_manager::allocate_venc_channel(vtype, venc_chn)) {
        return false; // No available VENC channels
    }
    
    // Create stream instance
    auto stream = std::make_shared<stream_instance>(config.stream_id, config);
    
    // Start the stream if camera is running
    if (m_is_running) {
        if (!stream->start()) {
            resource_manager::free_venc_channel(venc_chn);
            return false;
        }
    }
    
    // Add to streams map
    m_streams[config.stream_id] = stream;
    m_resources.venc_channels.push_back(venc_chn);
    
    return true;
}

bool camera_instance::destroy_stream(int32_t stream_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_streams.find(stream_id);
    if (it == m_streams.end()) {
        return false; // Stream not found
    }
    
    // Stop and remove stream
    if (it->second) {
        it->second->stop();
    }
    m_streams.erase(it);
    
    // Free VENC channel (in real implementation, would track which channel belongs to which stream)
    // For now, we just note that a channel was freed
    
    return true;
}

std::shared_ptr<stream_instance> camera_instance::get_stream(int32_t stream_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_streams.find(stream_id);
    if (it != m_streams.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<int32_t> camera_instance::list_streams() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<int32_t> stream_ids;
    for (const auto& pair : m_streams) {
        stream_ids.push_back(pair.first);
    }
    
    return stream_ids;
}

bool camera_instance::enable_feature(const std::string& feature_name, const std::string& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Placeholder implementation
    // In real implementation, would initialize the specific feature
    m_enabled_features[feature_name] = true;
    return true;
}

bool camera_instance::disable_feature(const std::string& feature_name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_enabled_features.find(feature_name);
    if (it != m_enabled_features.end()) {
        m_enabled_features.erase(it);
        return true;
    }
    
    return false;
}

bool camera_instance::is_feature_enabled(const std::string& feature_name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_enabled_features.find(feature_name);
    return (it != m_enabled_features.end() && it->second);
}

camera_status camera_instance::get_status() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    camera_status status;
    status.camera_id = m_camera_id;
    status.is_running = m_is_running;
    status.sensor = m_config.sensor;
    status.stream_count = static_cast<int32_t>(m_streams.size());
    
    for (const auto& pair : m_streams) {
        status.stream_ids.push_back(pair.first);
    }
    
    status.allocated_resources.vi_device = m_resources.vi_dev;
    status.allocated_resources.vpss_group = m_resources.vpss_grp;
    status.allocated_resources.venc_channels = m_resources.venc_channels;
    
    return status;
}

// Private methods

bool camera_instance::allocate_resources() {
    // Allocate VI device
    if (!resource_manager::allocate_vi_device(m_resources.vi_dev)) {
        return false;
    }
    
    // Allocate VPSS group
    if (!resource_manager::allocate_vpss_group(m_resources.vpss_grp)) {
        resource_manager::free_vi_device(m_resources.vi_dev);
        m_resources.vi_dev = -1;
        return false;
    }
    
    // VENC channels will be allocated per-stream in create_stream()
    
    return true;
}

void camera_instance::free_resources() {
    // Free VENC channels
    for (int32_t chn : m_resources.venc_channels) {
        resource_manager::free_venc_channel(chn);
    }
    m_resources.venc_channels.clear();
    
    // Free VPSS group
    if (m_resources.vpss_grp >= 0) {
        resource_manager::free_vpss_group(m_resources.vpss_grp);
        m_resources.vpss_grp = -1;
    }
    
    // Free VI device
    if (m_resources.vi_dev >= 0) {
        resource_manager::free_vi_device(m_resources.vi_dev);
        m_resources.vi_dev = -1;
    }
}

bool camera_instance::init_vi() {
    // Check and release any existing VI component
    if (m_vi_ptr)
    {
        m_vi_ptr.reset();
        m_vi_ptr = nullptr;
    }

    // Select the VI component based on camera configuration
    const auto& sensor_type = m_config.sensor.name;;
    const auto& work_mode = m_config.sensor.mode;

    if (sensor_type == "os04a10")
    {
        if (work_mode == "linear")
        {
            m_vi_ptr = std::make_shared<vi_os04a10_liner>();
        }
        else
        {
            m_vi_ptr = std::make_shared<vi_os04a10_2to1wdr>();
        }
    }
    else if (sensor_type == "os08a20")
    {
        if (work_mode == "linear")
        {
            m_vi_ptr = std::make_shared<vi_os08a20_liner>();
        }
        else
        {
            m_vi_ptr = std::make_shared<vi_os08a20_2to1wdr>();
        }
    }
    else
    {
        return false;
    }

    if(!m_vi_ptr->start())
    {
        DEV_WRITE_LOG_ERROR("vi start failed");
        m_vi_ptr.reset();
        m_vi_ptr = nullptr;
        return false;
    }

    return true;
}

bool camera_instance::init_vpss() {
    // Placeholder: In real implementation, would initialize VPSS hardware
    return true;
}

bool camera_instance::init_streams() {
    // Create all configured streams
    for (const auto& stream_cfg : m_config.streams) {
        if (!create_stream_internal(stream_cfg)) {
            // Cleanup already created streams
            for (auto& pair : m_streams) {
                if (pair.second) {
                    pair.second->stop();
                }
            }
            m_streams.clear();
            return false;
        }
    }
    
    return true;
}

bool camera_instance::init_features() {
    // Placeholder: In real implementation, would initialize features
    // based on m_config.features
    
    if (m_config.features.osd_enabled) {
        m_enabled_features["osd"] = true;
    }
    
    if (m_config.features.aiisp_enabled) {
        m_enabled_features["aiisp"] = true;
    }
    
    if (m_config.features.yolov5_enabled) {
        m_enabled_features["yolov5"] = true;
    }
    
    if (m_config.features.vo_enabled) {
        m_enabled_features["vo"] = true;
    }
    
    return true;
}

} // namespace device
} // namespace hisilicon
