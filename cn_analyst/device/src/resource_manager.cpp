#include "resource_manager.h"

namespace hisilicon {
namespace device {

// Static member initialization
bool resource_manager::m_initialized = false;
resource_limits resource_manager::m_limits;
std::map<int32_t, bool> resource_manager::m_vpss_allocated;
std::map<int32_t, venc_type> resource_manager::m_venc_allocated;
std::map<int32_t, bool> resource_manager::m_vi_allocated;
std::mutex resource_manager::m_mutex;

bool resource_manager::init(const resource_limits& limits) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return false; // Already initialized
    }
    
    m_limits = limits;
    
    // Initialize allocation maps
    m_vpss_allocated.clear();
    m_venc_allocated.clear();
    m_vi_allocated.clear();
    
    // Pre-populate with available resources
    for (int32_t i = 0; i < m_limits.max_vpss_groups; ++i) {
        m_vpss_allocated[i] = false;
    }
    
    for (int32_t i = 0; i < m_limits.max_venc_channels; ++i) {
        // Channels not allocated yet, so not in map
    }
    
    for (int32_t i = 0; i < m_limits.max_vi_devices; ++i) {
        m_vi_allocated[i] = false;
    }
    
    m_initialized = true;
    return true;
}

void resource_manager::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_vpss_allocated.clear();
    m_venc_allocated.clear();
    m_vi_allocated.clear();
    m_initialized = false;
}

bool resource_manager::is_initialized() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

// VPSS Group Management

bool resource_manager::allocate_vpss_group(int32_t& grp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    // Find first available VPSS group
    for (auto& pair : m_vpss_allocated) {
        if (!pair.second) {
            pair.second = true;
            grp = pair.first;
            return true;
        }
    }
    
    return false; // No available groups
}

void resource_manager::free_vpss_group(int32_t grp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    auto it = m_vpss_allocated.find(grp);
    if (it != m_vpss_allocated.end()) {
        it->second = false;
    }
}

bool resource_manager::is_vpss_group_available() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    for (const auto& pair : m_vpss_allocated) {
        if (!pair.second) {
            return true;
        }
    }
    
    return false;
}

// VENC Channel Management

bool resource_manager::allocate_venc_channel(venc_type type, int32_t& chn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    // Count current allocations by type
    int32_t h264_count = 0;
    int32_t h265_count = 0;
    
    for (const auto& pair : m_venc_allocated) {
        if (pair.second == venc_type::H264) {
            h264_count++;
        } else if (pair.second == venc_type::H265) {
            h265_count++;
        }
    }
    
    // Check type-specific limits
    if (type == venc_type::H264 && h264_count >= m_limits.max_h264_channels) {
        return false;
    }
    if (type == venc_type::H265 && h265_count >= m_limits.max_h265_channels) {
        return false;
    }
    
    // Check total channel limit
    if (m_venc_allocated.size() >= static_cast<size_t>(m_limits.max_venc_channels)) {
        return false;
    }
    
    // Find first available channel ID
    for (int32_t i = 0; i < m_limits.max_venc_channels; ++i) {
        if (m_venc_allocated.find(i) == m_venc_allocated.end()) {
            m_venc_allocated[i] = type;
            chn = i;
            return true;
        }
    }
    
    return false;
}

void resource_manager::free_venc_channel(int32_t chn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    m_venc_allocated.erase(chn);
}

bool resource_manager::is_venc_channel_available(venc_type type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    // Count current allocations by type
    int32_t h264_count = 0;
    int32_t h265_count = 0;
    
    for (const auto& pair : m_venc_allocated) {
        if (pair.second == venc_type::H264) {
            h264_count++;
        } else if (pair.second == venc_type::H265) {
            h265_count++;
        }
    }
    
    // Check type-specific limits
    if (type == venc_type::H264 && h264_count >= m_limits.max_h264_channels) {
        return false;
    }
    if (type == venc_type::H265 && h265_count >= m_limits.max_h265_channels) {
        return false;
    }
    
    // Check total channel limit
    if (m_venc_allocated.size() >= static_cast<size_t>(m_limits.max_venc_channels)) {
        return false;
    }
    
    return true;
}

// VI Device Management

bool resource_manager::allocate_vi_device(int32_t& dev) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    // Find first available VI device
    for (auto& pair : m_vi_allocated) {
        if (!pair.second) {
            pair.second = true;
            dev = pair.first;
            return true;
        }
    }
    
    return false; // No available devices
}

void resource_manager::free_vi_device(int32_t dev) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    auto it = m_vi_allocated.find(dev);
    if (it != m_vi_allocated.end()) {
        it->second = false;
    }
}

bool resource_manager::is_vi_device_available() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    for (const auto& pair : m_vi_allocated) {
        if (!pair.second) {
            return true;
        }
    }
    
    return false;
}

// Query Functions

resource_status resource_manager::get_status() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    resource_status status;
    
    if (!m_initialized) {
        return status;
    }
    
    // VI devices
    status.vi_devices.total = m_limits.max_vi_devices;
    status.vi_devices.allocated = 0;
    for (const auto& pair : m_vi_allocated) {
        if (pair.second) {
            status.vi_devices.allocated++;
        }
    }
    status.vi_devices.available = status.vi_devices.total - status.vi_devices.allocated;
    
    // VPSS groups
    status.vpss_groups.total = m_limits.max_vpss_groups;
    status.vpss_groups.allocated = 0;
    for (const auto& pair : m_vpss_allocated) {
        if (pair.second) {
            status.vpss_groups.allocated++;
        }
    }
    status.vpss_groups.available = status.vpss_groups.total - status.vpss_groups.allocated;
    
    // VENC channels by type
    status.venc_h264.total = m_limits.max_h264_channels;
    status.venc_h264.allocated = 0;
    status.venc_h265.total = m_limits.max_h265_channels;
    status.venc_h265.allocated = 0;
    
    for (const auto& pair : m_venc_allocated) {
        if (pair.second == venc_type::H264) {
            status.venc_h264.allocated++;
        } else if (pair.second == venc_type::H265) {
            status.venc_h265.allocated++;
        }
    }
    
    status.venc_h264.available = status.venc_h264.total - status.venc_h264.allocated;
    status.venc_h265.available = status.venc_h265.total - status.venc_h265.allocated;
    
    // Total VENC channels
    status.total_venc_channels = m_limits.max_venc_channels;
    status.allocated_venc_channels = static_cast<int32_t>(m_venc_allocated.size());
    status.available_venc_channels = status.total_venc_channels - status.allocated_venc_channels;
    
    return status;
}

resource_limits resource_manager::get_limits() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_limits;
}

} // namespace device
} // namespace hisilicon
