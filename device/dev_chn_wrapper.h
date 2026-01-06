#ifndef DEV_CHN_WRAPPER_H
#define DEV_CHN_WRAPPER_H

/**
 * @file dev_chn_wrapper.h
 * @brief Migration wrapper/adapter for legacy dev_chn class
 * 
 * This wrapper provides backward compatibility with the existing dev_chn interface
 * while using the new camera_manager and camera_instance infrastructure internally.
 * 
 * Purpose: Allow gradual migration from legacy single-camera architecture to
 * new multi-camera architecture without breaking existing code.
 * 
 * Status: MIGRATION WRAPPER - Bridge between old and new implementations
 */

#include "dev_chn.h"
#include "../cn_analyst/device/src/camera_manager.h"
#include "../cn_analyst/device/src/camera_instance.h"
#include <memory>
#include <string>

namespace hisilicon {
namespace dev {

/**
 * @brief Wrapper class that adapts dev_chn interface to new camera architecture
 * 
 * This class maintains the same public interface as dev_chn but delegates
 * all operations to the new camera_manager and camera_instance classes.
 * 
 * Usage:
 *   // Same as old code
 *   auto chn = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
 *   chn->start(1920, 1080, 30, 4096);
 *   // ... use as normal dev_chn ...
 *   chn->stop();
 * 
 * Migration Path:
 *   1. Replace dev_chn with chn_wrapper (minimal code changes)
 *   2. Test backward compatibility
 *   3. Gradually update code to use camera_manager directly
 *   4. Remove wrapper once migration complete
 */
class chn_wrapper : public ceanic::util::stream_observer,
                    public std::enable_shared_from_this<chn_wrapper> {
public:
    /**
     * @brief Constructor matching legacy dev_chn interface
     * @param vi_name Sensor name (e.g., "OS04A10", "OS08A20", "OS04A10_WDR")
     * @param venc_mode Encoder mode (e.g., "H264_CBR", "H265_AVBR")
     * @param chn_no Channel number (typically 0 for single camera)
     */
    chn_wrapper(const char* vi_name, const char* venc_mode, int chn_no);
    
    /**
     * @brief Destructor - cleans up camera resources
     */
    ~chn_wrapper();

    /**
     * @brief Start the camera with specified encoding parameters
     * @param venc_w Video width
     * @param venc_h Video height
     * @param fr Frame rate
     * @param bitrate Bitrate in kbps
     * @return true if successful, false otherwise
     */
    bool start(int venc_w, int venc_h, int fr, int bitrate);
    
    /**
     * @brief Stop the camera
     */
    void stop();
    
    /**
     * @brief Check if camera is started
     * @return true if running, false otherwise
     */
    bool is_start();

    /**
     * @brief Get ISP exposure information
     * @param val Output exposure info structure
     * @return true if successful, false otherwise
     */
    bool get_isp_exposure_info(isp_exposure_t* val);

    /**
     * @brief Start MP4 recording to file
     * @param file File path for recording
     * @return true if successful, false otherwise
     */
    bool start_save(const char* file);
    
    /**
     * @brief Stop MP4 recording
     */
    void stop_save();

    /**
     * @brief Trigger JPEG snapshot
     * @param file Output file path
     * @param quality JPEG quality (1-100)
     * @param str_info Text overlay info
     * @return true if successful, false otherwise
     */
    bool trigger_jpg(const char* file, int quality, const char* str_info);

    // Static methods - system-level operations
    static bool init(ot_vi_vpss_mode_type mode);
    static void release();
    static void start_capture(bool enable);

    // Stream observer callbacks
    void on_stream_come(ceanic::util::stream_obj_ptr sobj, 
                       ceanic::util::stream_head* head,
                       const char* buf, int32_t len) override;
    void on_stream_error(ceanic::util::stream_obj_ptr sobj, int32_t errno) override;

    // Scene management
    static bool scene_init(const char* dir_path);
    static bool scene_set_mode(int mode);
    static void scene_release();

    // SVC rate auto
    static bool rate_auto_init(const char* file);
    static void rate_auto_release();

    // AIISP features
    bool aiisp_start(const char* model_file, int mode);
    void aiisp_stop();

    // YOLOv5 detection
    bool yolov5_start(const char* model_file);
    void yolov5_stop();

    // Video output
    bool vo_start(const char* intf_type, const char* intf_sync);
    void vo_stop();

    // Stream utilities
    static bool get_stream_head(int chn, int stream, ceanic::util::media_head* mh);
    static bool request_i_frame(int chn, int stream);

    /**
     * @brief Get underlying camera instance (for advanced usage)
     * @return Camera instance pointer or nullptr
     */
    std::shared_ptr<hisilicon::device::camera_instance> get_camera_instance() const {
        return m_camera_instance;
    }

private:
    // Legacy interface state
    bool m_is_start;
    std::string m_vi_name;
    std::string m_venc_mode;
    int m_chn;
    
    // New architecture components
    std::shared_ptr<hisilicon::device::camera_instance> m_camera_instance;
    
    // Fallback to legacy implementation if needed
    std::shared_ptr<chn> m_legacy_chn;
    bool m_use_legacy;
    
    // Static state for system-level operations
    static bool s_camera_manager_initialized;
    static bool s_resource_manager_initialized;
    static ot_vi_vpss_mode_type s_vi_vpss_mode;
    
    // Helper methods
    hisilicon::device::encoder_type parse_encoder_type(const std::string& venc_mode);
    std::string parse_sensor_name(const std::string& vi_name);
    std::string parse_sensor_mode(const std::string& vi_name);
    void create_camera_config(int venc_w, int venc_h, int fr, int bitrate,
                             hisilicon::device::camera_config& config);
};

} // namespace dev
} // namespace hisilicon

#endif // DEV_CHN_WRAPPER_H
