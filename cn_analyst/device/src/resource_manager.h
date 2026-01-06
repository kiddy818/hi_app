#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

namespace hisilicon {
namespace device {

/**
 * @brief Encoder type for resource allocation
 */
enum class venc_type {
    H264,
    H265
};

/**
 * @brief Resource limits for the system
 */
struct resource_limits {
    int32_t max_vi_devices;        // Default: 4 (HiSilicon 3519DV500 limit)
    int32_t max_vpss_groups;       // Default: 32
    int32_t max_vpss_chns_per_grp; // Default: 4
    int32_t max_venc_channels;     // Default: 16
    int32_t max_h264_channels;     // Default: 12
    int32_t max_h265_channels;     // Default: 8
    
    resource_limits()
        : max_vi_devices(4)
        , max_vpss_groups(32)
        , max_vpss_chns_per_grp(4)
        , max_venc_channels(16)
        , max_h264_channels(12)
        , max_h265_channels(8)
    {}
};

/**
 * @brief Current resource allocation status
 */
struct resource_status {
    struct {
        int32_t total;
        int32_t allocated;
        int32_t available;
    } vi_devices, vpss_groups, venc_h264, venc_h265;
    
    int32_t total_venc_channels;
    int32_t allocated_venc_channels;
    int32_t available_venc_channels;
};

/**
 * @brief Resource Manager - Tracks and allocates hardware resources
 * 
 * This class manages allocation of HiSilicon hardware resources including:
 * - VI (Video Input) devices
 * - VPSS (Video Processing SubSystem) groups
 * - VENC (Video Encoding) channels
 * 
 * Thread-safe singleton pattern for centralized resource management.
 */
class resource_manager {
public:
    /**
     * @brief Initialize the resource manager with specified limits
     * @param limits Resource limits configuration
     * @return true if initialization successful, false otherwise
     */
    static bool init(const resource_limits& limits);
    
    /**
     * @brief Release all resources and reset the manager
     */
    static void release();
    
    /**
     * @brief Check if resource manager is initialized
     * @return true if initialized, false otherwise
     */
    static bool is_initialized();
    
    // VPSS Group Management
    
    /**
     * @brief Allocate a VPSS group
     * @param grp Output parameter for allocated group ID
     * @return true if allocation successful, false if no groups available
     */
    static bool allocate_vpss_group(int32_t& grp);
    
    /**
     * @brief Free a previously allocated VPSS group
     * @param grp VPSS group ID to free
     */
    static void free_vpss_group(int32_t grp);
    
    /**
     * @brief Check if VPSS group is available for allocation
     * @return true if at least one group is available
     */
    static bool is_vpss_group_available();
    
    // VENC Channel Management
    
    /**
     * @brief Allocate a VENC channel
     * @param type Encoder type (H264 or H265)
     * @param chn Output parameter for allocated channel ID
     * @return true if allocation successful, false if no channels available
     */
    static bool allocate_venc_channel(venc_type type, int32_t& chn);
    
    /**
     * @brief Free a previously allocated VENC channel
     * @param chn VENC channel ID to free
     */
    static void free_venc_channel(int32_t chn);
    
    /**
     * @brief Check if VENC channel is available for allocation
     * @param type Encoder type to check
     * @return true if at least one channel of the specified type is available
     */
    static bool is_venc_channel_available(venc_type type);
    
    // VI Device Management
    
    /**
     * @brief Allocate a VI device
     * @param dev Output parameter for allocated device ID
     * @return true if allocation successful, false if no devices available
     */
    static bool allocate_vi_device(int32_t& dev);
    
    /**
     * @brief Free a previously allocated VI device
     * @param dev VI device ID to free
     */
    static void free_vi_device(int32_t dev);
    
    /**
     * @brief Check if VI device is available for allocation
     * @return true if at least one device is available
     */
    static bool is_vi_device_available();
    
    // Query Functions
    
    /**
     * @brief Get current resource allocation status
     * @return Current resource status
     */
    static resource_status get_status();
    
    /**
     * @brief Get resource limits configuration
     * @return Current resource limits
     */
    static resource_limits get_limits();
    
private:
    // Prevent instantiation
    resource_manager() = delete;
    ~resource_manager() = delete;
    resource_manager(const resource_manager&) = delete;
    resource_manager& operator=(const resource_manager&) = delete;
    
    // Internal state
    static bool m_initialized;
    static resource_limits m_limits;
    static std::map<int32_t, bool> m_vpss_allocated;
    static std::map<int32_t, venc_type> m_venc_allocated;
    static std::map<int32_t, bool> m_vi_allocated;
    static std::mutex m_mutex;
};

} // namespace device
} // namespace hisilicon

#endif // RESOURCE_MANAGER_H
