#ifndef CAMERA_INSTANCE_H
#define CAMERA_INSTANCE_H

#include "stream_config.h"
#include "resource_manager.h"
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "dev_vi_os04a10_liner.h"
#include "dev_vi_os04a10_2to1wdr.h"
#include "dev_vi_os08a20_liner.h"
#include "dev_vi_os08a20_2to1wdr.h"

#include <stream_observer.h>

using namespace hisilicon::dev;

namespace hisilicon {
namespace device {

/**
 * @brief Sensor configuration
 */
struct sensor_config {
    std::string name;  // e.g., "OS04A10", "OS08A20"
    std::string mode;  // e.g., "liner", "2to1wdr"
    
    sensor_config() {}
    sensor_config(const std::string& n, const std::string& m)
        : name(n), mode(m) {}
};

/**
 * @brief Feature configuration flags
 */
struct feature_config {
    bool osd_enabled;
    bool scene_auto_enabled;
    int32_t scene_mode;
    bool aiisp_enabled;
    std::string aiisp_model;
    bool yolov5_enabled;
    std::string yolov5_model;
    bool vo_enabled;
    std::string vo_interface_type;
    std::string vo_interface_sync;
    
    feature_config()
        : osd_enabled(false)
        , scene_auto_enabled(false)
        , scene_mode(0)
        , aiisp_enabled(false)
        , yolov5_enabled(false)
        , vo_enabled(false)
    {}
};

/**
 * @brief Camera configuration
 */
struct camera_config {
    int32_t camera_id;
    bool enabled;
    sensor_config sensor;
    std::vector<stream_config> streams;
    feature_config features;
    
    camera_config()
        : camera_id(-1)
        , enabled(true)
    {}
};

/**
 * @brief Camera status information
 */
struct camera_status {
    int32_t camera_id;
    bool is_running;
    sensor_config sensor;
    int32_t stream_count;
    std::vector<int32_t> stream_ids;
    
    struct {
        int32_t vi_device;
        int32_t vpss_group;
        std::vector<int32_t> venc_channels;
    } allocated_resources;
};

/**
 * @brief Allocated resources for a camera
 */
struct allocated_resources {
    int32_t vi_dev;
    int32_t vpss_grp;
    std::vector<int32_t> venc_channels;
    
    allocated_resources()
        : vi_dev(-1)
        , vpss_grp(-1)
    {}
    
    void clear() {
        vi_dev = -1;
        vpss_grp = -1;
        venc_channels.clear();
    }
    
    bool is_allocated() const {
        return (vi_dev >= 0 || vpss_grp >= 0 || !venc_channels.empty());
    }
};

// Forward declaration for stream instance (to be implemented later)
class stream_instance;

/**
 * @brief Camera Instance - Manages a single camera and its resources
 * 
 * Represents a single camera with its associated hardware resources:
 * - VI (Video Input) device
 * - VPSS (Video Processing SubSystem) group
 * - VENC (Video Encoding) channels for multiple streams
 * - Optional features (OSD, AIISP, YOLOv5, VO)
 * 
 * Implements observer pattern for distributing streams to listeners.
 */
class camera_instance : public std::enable_shared_from_this<camera_instance>,
                        public ceanic::util::stream_observer {
public:
    /**
     * @brief Construct a camera instance
     * @param camera_id Unique camera identifier
     * @param config Camera configuration
     */
    camera_instance(int32_t camera_id, const camera_config& config);
    
    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~camera_instance();
    
    // Prevent copying
    camera_instance(const camera_instance&) = delete;
    camera_instance& operator=(const camera_instance&) = delete;
    
    // Lifecycle Management
    
    /**
     * @brief Start the camera and all configured streams
     * @return true if successful, false otherwise
     */
    bool start();
    
    /**
     * @brief Stop the camera and release resources
     */
    void stop();
    
    /**
     * @brief Check if camera is running
     * @return true if running, false otherwise
     */
    bool is_running() const;
    
    // Stream Management
    
    /**
     * @brief Create a new stream for this camera
     * @param config Stream configuration
     * @return true if successful, false otherwise
     */
    bool create_stream(const stream_config& config);
    
    /**
     * @brief Destroy a stream
     * @param stream_id Stream ID to destroy
     * @return true if successful, false otherwise
     */
    bool destroy_stream(int32_t stream_id);
    
    /**
     * @brief Get stream instance by ID
     * @param stream_id Stream ID
     * @return Stream instance pointer or nullptr if not found
     */
    std::shared_ptr<stream_instance> get_stream(int32_t stream_id);
    
    /**
     * @brief List all stream IDs
     * @return Vector of stream IDs
     */
    std::vector<int32_t> list_streams() const;
    
    // Feature Management
    
    /**
     * @brief Enable a feature
     * @param feature_name Feature name (e.g., "osd", "aiisp", "yolov5", "vo")
     * @param config Feature-specific configuration (JSON or string)
     * @return true if successful, false otherwise
     */
    bool enable_feature(const std::string& feature_name, const std::string& config);
    
    /**
     * @brief Disable a feature
     * @param feature_name Feature name
     * @return true if successful, false otherwise
     */
    bool disable_feature(const std::string& feature_name);
    
    /**
     * @brief Check if a feature is enabled
     * @param feature_name Feature name
     * @return true if enabled, false otherwise
     */
    bool is_feature_enabled(const std::string& feature_name) const;
    
    // Information
    
    /**
     * @brief Get camera ID
     * @return Camera ID
     */
    int32_t camera_id() const { return m_camera_id; }
    
    /**
     * @brief Get camera configuration
     * @return Camera configuration
     */
    camera_config get_config() const { return m_config; }
    
    /**
     * @brief Get camera status
     * @return Current camera status
     */
    camera_status get_status() const;
    
    /**
     * @brief Get allocated resources
     * @return Allocated resources
     */
    allocated_resources get_allocated_resources() const { return m_resources; }

    bool get_isp_exposure_info(isp_exposure_t* val);

    // Observer Pattern (from device to streaming)
    void on_stream_come(ceanic::util::stream_obj_ptr obj, ceanic::util::stream_head* head,
                       const char* buf, int32_t len) override;
    void on_stream_error(ceanic::util::stream_obj_ptr obj, int32_t error) override;
    bool request_i_frame(int stream);
    bool get_stream_head(int stream, ceanic::util::media_head* mh);

    
private:
    // Internal initialization methods
    bool allocate_resources();
    void free_resources();
    bool init_vi();
    bool init_vpss();
    bool init_streams();
    bool init_features();

    void stop_streams();
    
    // Internal stream management (without locking)
    bool create_stream_internal(const stream_config& config);
    
    // Member variables
    int32_t m_camera_id;
    camera_config m_config;
    bool m_is_running;
    
    // Allocated hardware resources
    allocated_resources m_resources;
    
    // Streams (stream_id -> stream_instance)
    std::map<int32_t, std::shared_ptr<stream_instance>> m_streams;
    
    // Features tracking
    std::map<std::string, bool> m_enabled_features;

    std::shared_ptr<vi> m_vi_ptr;
    
    // Thread safety
    mutable std::mutex m_mutex;
};

} // namespace device
} // namespace hisilicon

#endif // CAMERA_INSTANCE_H
