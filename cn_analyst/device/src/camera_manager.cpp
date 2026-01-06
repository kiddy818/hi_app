#include "camera_manager.h"
#include <algorithm>
#include <sstream>

namespace hisilicon {
namespace device {

// Static member initialization
bool camera_manager::m_initialized = false;
int32_t camera_manager::m_max_cameras = 4;
int32_t camera_manager::m_next_camera_id = 0;
std::map<int32_t, std::shared_ptr<camera_instance>> camera_manager::m_cameras;
std::mutex camera_manager::m_mutex;

bool camera_manager::init(int32_t max_cameras) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return false; // Already initialized
    }
    
    if (max_cameras <= 0 || max_cameras > 4) {
        return false; // Invalid max_cameras (HiSilicon 3519DV500 supports up to 4 cameras)
    }
    
    m_max_cameras = max_cameras;
    m_next_camera_id = 0;
    m_cameras.clear();
    m_initialized = true;
    
    return true;
}

void camera_manager::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Stop and destroy all cameras
    for (auto& pair : m_cameras) {
        if (pair.second) {
            pair.second->stop();
        }
    }
    
    m_cameras.clear();
    m_next_camera_id = 0;
    m_initialized = false;
}

bool camera_manager::is_initialized() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

std::shared_ptr<camera_instance> camera_manager::create_camera(const camera_config& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return nullptr;
    }
    
    // Check if we've reached the maximum number of cameras
    if (static_cast<int32_t>(m_cameras.size()) >= m_max_cameras) {
        return nullptr;
    }
    
    // Validate configuration
    std::string error_msg;
    if (!validate_config(config, error_msg)) {
        // In real implementation, would log error_msg
        return nullptr;
    }
    
    // Check if camera ID is already in use (if specified)
    int32_t camera_id = config.camera_id;
    if (camera_id >= 0 && m_cameras.find(camera_id) != m_cameras.end()) {
        return nullptr; // Camera ID already exists
    }
    
    // Allocate a new camera ID if not specified
    if (camera_id < 0) {
        camera_id = allocate_camera_id();
        if (camera_id < 0) {
            return nullptr; // Failed to allocate ID
        }
    }
    
    // Create camera configuration with assigned ID
    camera_config final_config = config;
    final_config.camera_id = camera_id;
    
    // Create camera instance
    auto camera = std::make_shared<camera_instance>(camera_id, final_config);
    
    // Add to camera map
    m_cameras[camera_id] = camera;
    
    return camera;
}

bool camera_manager::destroy_camera(int32_t camera_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    auto it = m_cameras.find(camera_id);
    if (it == m_cameras.end()) {
        return false; // Camera not found
    }
    
    // Stop camera
    if (it->second) {
        it->second->stop();
    }
    
    // Remove from map
    m_cameras.erase(it);
    
    return true;
}

void camera_manager::destroy_all_cameras() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    // Stop all cameras
    for (auto& pair : m_cameras) {
        if (pair.second) {
            pair.second->stop();
        }
    }
    
    // Clear map
    m_cameras.clear();
}

std::shared_ptr<camera_instance> camera_manager::get_camera(int32_t camera_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return nullptr;
    }
    
    auto it = m_cameras.find(camera_id);
    if (it != m_cameras.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<int32_t> camera_manager::list_cameras() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<int32_t> camera_ids;
    for (const auto& pair : m_cameras) {
        camera_ids.push_back(pair.first);
    }
    
    return camera_ids;
}

int32_t camera_manager::get_camera_count() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int32_t>(m_cameras.size());
}

bool camera_manager::camera_exists(int32_t camera_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_cameras.find(camera_id) != m_cameras.end());
}

bool camera_manager::validate_config(const camera_config& config, std::string& error_msg) {
    std::ostringstream oss;
    
    // Check if enabled
    if (!config.enabled) {
        error_msg = "Camera is disabled";
        return false;
    }
    
    // Validate sensor configuration
    if (!validate_sensor_config(config.sensor, error_msg)) {
        return false;
    }
    
    // Validate streams
    if (config.streams.empty()) {
        error_msg = "At least one stream must be configured";
        return false;
    }
    
    for (const auto& stream : config.streams) {
        if (!stream_config_helper::validate(stream, error_msg)) {
            return false;
        }
    }
    
    // Check for duplicate stream IDs
    std::vector<int32_t> stream_ids;
    for (const auto& stream : config.streams) {
        if (std::find(stream_ids.begin(), stream_ids.end(), stream.stream_id) != stream_ids.end()) {
            oss << "Duplicate stream ID: " << stream.stream_id;
            error_msg = oss.str();
            return false;
        }
        stream_ids.push_back(stream.stream_id);
    }
    
    return true;
}

bool camera_manager::can_create_camera(const camera_config& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    // Check camera count limit
    if (static_cast<int32_t>(m_cameras.size()) >= m_max_cameras) {
        return false;
    }
    
    // Check if VI device is available
    if (!resource_manager::is_vi_device_available()) {
        return false;
    }
    
    // Check if VPSS group is available
    if (!resource_manager::is_vpss_group_available()) {
        return false;
    }
    
    // Check if VENC channels are available for all streams
    for (const auto& stream : config.streams) {
        venc_type vtype = stream_config_helper::is_h264(stream.type) 
            ? venc_type::H264 : venc_type::H265;
        
        if (!resource_manager::is_venc_channel_available(vtype)) {
            return false;
        }
    }
    
    return true;
}

int32_t camera_manager::get_max_cameras() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_max_cameras;
}

// Private helper methods

int32_t camera_manager::allocate_camera_id() {
    // Find first available ID
    for (int32_t i = 0; i < m_max_cameras * 10; ++i) {
        int32_t candidate_id = m_next_camera_id;
        m_next_camera_id = (m_next_camera_id + 1) % (m_max_cameras * 10);
        
        if (m_cameras.find(candidate_id) == m_cameras.end()) {
            return candidate_id;
        }
    }
    
    return -1; // Failed to find available ID
}

bool camera_manager::validate_sensor_config(const sensor_config& sensor, std::string& error_msg) {
    // Check sensor name
    if (sensor.name.empty()) {
        error_msg = "Sensor name cannot be empty";
        return false;
    }
    
    // Check against supported sensors
    static const std::vector<std::string> SUPPORTED_SENSORS = {
        "OS04A10", "OS04A10_WDR", "OS08A20", "OS08A20_WDR"
    };
    
    bool supported = false;
    for (const auto& supported_name : SUPPORTED_SENSORS) {
        if (sensor.name == supported_name) {
            supported = true;
            break;
        }
    }
    
    if (!supported) {
        std::ostringstream oss;
        oss << "Unsupported sensor: " << sensor.name;
        error_msg = oss.str();
        return false;
    }
    
    // Check sensor mode
    if (sensor.mode.empty()) {
        error_msg = "Sensor mode cannot be empty";
        return false;
    }
    
    // Validate mode based on sensor name
    if (sensor.name.find("WDR") != std::string::npos) {
        if (sensor.mode != "2to1wdr") {
            error_msg = "WDR sensors require mode '2to1wdr'";
            return false;
        }
    } else {
        if (sensor.mode != "liner") {
            error_msg = "Non-WDR sensors require mode 'liner'";
            return false;
        }
    }
    
    return true;
}

} // namespace device
} // namespace hisilicon
