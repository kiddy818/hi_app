#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "camera_instance.h"
#include <map>
#include <mutex>
#include <memory>
#include <vector>

namespace hisilicon {
namespace device {

/**
 * @brief Camera Manager - Central registry and factory for camera instances
 * 
 * Manages the lifecycle of all camera instances in the system.
 * Enforces camera count limits and coordinates with resource_manager
 * for hardware resource allocation.
 * 
 * Thread-safe singleton pattern.
 */
class camera_manager {
public:
    /**
     * @brief Initialize the camera manager
     * @param max_cameras Maximum number of cameras supported (default: 4)
     * @return true if initialization successful, false otherwise
     */
    static bool init(int32_t max_cameras = 4);
    
    /**
     * @brief Release all cameras and reset the manager
     */
    static void release();
    
    /**
     * @brief Check if camera manager is initialized
     * @return true if initialized, false otherwise
     */
    static bool is_initialized();
    
    // Camera Lifecycle
    
    /**
     * @brief Create a new camera
     * @param config Camera configuration
     * @return Camera instance pointer or nullptr if creation failed
     */
    static std::shared_ptr<camera_instance> create_camera(const camera_config& config);
    
    /**
     * @brief Destroy a camera
     * @param camera_id Camera ID to destroy
     * @return true if successful, false if camera not found
     */
    static bool destroy_camera(int32_t camera_id);
    
    /**
     * @brief Destroy all cameras
     */
    static void destroy_all_cameras();
    
    // Query Functions
    
    /**
     * @brief Get a camera instance by ID
     * @param camera_id Camera ID
     * @return Camera instance pointer or nullptr if not found
     */
    static std::shared_ptr<camera_instance> get_camera(int32_t camera_id);
    
    /**
     * @brief List all camera IDs
     * @return Vector of camera IDs
     */
    static std::vector<int32_t> list_cameras();
    
    /**
     * @brief Get number of active cameras
     * @return Camera count
     */
    static int32_t get_camera_count();
    
    /**
     * @brief Check if a camera exists
     * @param camera_id Camera ID
     * @return true if camera exists, false otherwise
     */
    static bool camera_exists(int32_t camera_id);
    
    // Validation
    
    /**
     * @brief Validate camera configuration
     * @param config Camera configuration to validate
     * @param error_msg Output parameter for error message if validation fails
     * @return true if valid, false otherwise
     */
    static bool validate_config(const camera_config& config, std::string& error_msg);
    
    /**
     * @brief Check if a new camera can be created
     * @param config Camera configuration
     * @return true if camera can be created, false if resources insufficient
     */
    static bool can_create_camera(const camera_config& config);
    
    /**
     * @brief Get maximum number of cameras supported
     * @return Maximum camera count
     */
    static int32_t get_max_cameras();
    
private:
    // Prevent instantiation
    camera_manager() = delete;
    ~camera_manager() = delete;
    camera_manager(const camera_manager&) = delete;
    camera_manager& operator=(const camera_manager&) = delete;
    
    // Internal helper methods
    static int32_t allocate_camera_id();
    static bool validate_sensor_config(const sensor_config& sensor, std::string& error_msg);
    
    // Internal state
    static bool m_initialized;
    static int32_t m_max_cameras;
    static int32_t m_next_camera_id;
    static std::map<int32_t, std::shared_ptr<camera_instance>> m_cameras;
    static std::mutex m_mutex;
};

} // namespace device
} // namespace hisilicon

#endif // CAMERA_MANAGER_H
