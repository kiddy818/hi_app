#include "dev_chn_wrapper.h"
#include "dev_log.h"
#include <cstring>

namespace hisilicon {
namespace dev {

// Static members initialization
bool chn_wrapper::s_camera_manager_initialized = false;
bool chn_wrapper::s_resource_manager_initialized = false;
ot_vi_vpss_mode_type chn_wrapper::s_vi_vpss_mode = OT_VI_ONLINE_VPSS_ONLINE;

chn_wrapper::chn_wrapper(const char* vi_name, const char* venc_mode, int chn_no)
    : m_is_start(false)
    , m_vi_name(vi_name)
    , m_venc_mode(venc_mode)
    , m_chn(chn_no)
    , m_use_legacy(false)
{
    DEV_WRITE_LOG_INFO("Creating chn_wrapper: vi_name=%s, venc_mode=%s, chn=%d", 
                       vi_name, venc_mode, chn_no);
    
    // Initialize camera manager if not already done
    if (!s_camera_manager_initialized) {
        // Camera manager will be initialized by init() static method
        DEV_WRITE_LOG_WARN("Camera manager not yet initialized, will use legacy implementation");
        m_use_legacy = true;
        m_legacy_chn = std::make_shared<chn>(vi_name, venc_mode, chn_no);
    }
}

chn_wrapper::~chn_wrapper()
{
    stop();
    DEV_WRITE_LOG_INFO("Destroyed chn_wrapper for chn=%d", m_chn);
}

bool chn_wrapper::start(int venc_w, int venc_h, int fr, int bitrate)
{
    if (m_is_start) {
        DEV_WRITE_LOG_WARN("Camera already started");
        return false;
    }
    
    // Use legacy implementation if camera manager not available
    if (m_use_legacy && m_legacy_chn) {
        DEV_WRITE_LOG_INFO("Using legacy dev_chn implementation");
        bool result = m_legacy_chn->start(venc_w, venc_h, fr, bitrate);
        if (result) {
            m_is_start = true;
        }
        return result;
    }
    
    // Use new camera architecture
    if (!s_camera_manager_initialized) {
        DEV_WRITE_LOG_ERROR("Camera manager not initialized");
        return false;
    }
    
    try {
        // Create camera configuration
        hisilicon::device::camera_config config;
        create_camera_config(venc_w, venc_h, fr, bitrate, config);
        
        // Create camera via camera_manager
        m_camera_instance = hisilicon::device::camera_manager::create_camera(config);
        if (!m_camera_instance) {
            DEV_WRITE_LOG_ERROR("Failed to create camera instance");
            return false;
        }
        
        // Start the camera
        if (!m_camera_instance->start()) {
            DEV_WRITE_LOG_ERROR("Failed to start camera");
            hisilicon::device::camera_manager::destroy_camera(m_camera_instance->camera_id());
            m_camera_instance.reset();
            return false;
        }
        
        m_is_start = true;
        DEV_WRITE_LOG_INFO("Camera started successfully via new architecture");
        return true;
        
    } catch (const std::exception& e) {
        DEV_WRITE_LOG_ERROR("Exception starting camera: %s", e.what());
        return false;
    }
}

void chn_wrapper::stop()
{
    if (!m_is_start) {
        return;
    }
    
    // Use legacy implementation
    if (m_use_legacy && m_legacy_chn) {
        m_legacy_chn->stop();
        m_is_start = false;
        return;
    }
    
    // Use new camera architecture
    if (m_camera_instance) {
        try {
            m_camera_instance->stop();
            hisilicon::device::camera_manager::destroy_camera(m_camera_instance->camera_id());
            m_camera_instance.reset();
            DEV_WRITE_LOG_INFO("Camera stopped via new architecture");
        } catch (const std::exception& e) {
            DEV_WRITE_LOG_ERROR("Exception stopping camera: %s", e.what());
        }
    }
    
    m_is_start = false;
}

bool chn_wrapper::is_start()
{
    if (m_use_legacy && m_legacy_chn) {
        return m_legacy_chn->is_start();
    }
    return m_is_start;
}

bool chn_wrapper::get_isp_exposure_info(isp_exposure_t* val)
{
    if (m_use_legacy && m_legacy_chn) {
        return m_legacy_chn->get_isp_exposure_info(val);
    }
    
    // TODO: Implement via camera_instance when ISP interface is integrated
    DEV_WRITE_LOG_WARN("get_isp_exposure_info not yet implemented in new architecture");
    return false;
}

bool chn_wrapper::start_save(const char* file)
{
    if (m_use_legacy && m_legacy_chn) {
        return m_legacy_chn->start_save(file);
    }
    
    // TODO: Implement MP4 save via camera_instance
    DEV_WRITE_LOG_WARN("start_save not yet implemented in new architecture");
    return false;
}

void chn_wrapper::stop_save()
{
    if (m_use_legacy && m_legacy_chn) {
        m_legacy_chn->stop_save();
        return;
    }
    
    // TODO: Implement via camera_instance
    DEV_WRITE_LOG_WARN("stop_save not yet implemented in new architecture");
}

bool chn_wrapper::trigger_jpg(const char* file, int quality, const char* str_info)
{
    if (m_use_legacy && m_legacy_chn) {
        return m_legacy_chn->trigger_jpg(file, quality, str_info);
    }
    
    // TODO: Implement JPEG snapshot via camera_instance
    DEV_WRITE_LOG_WARN("trigger_jpg not yet implemented in new architecture");
    return false;
}

bool chn_wrapper::init(ot_vi_vpss_mode_type mode)
{
    DEV_WRITE_LOG_INFO("Initializing chn_wrapper with mode=%d", mode);
    
    s_vi_vpss_mode = mode;
    
    // Initialize resource manager
    if (!s_resource_manager_initialized) {
        hisilicon::device::resource_limits limits;
        limits.max_vi_devices = 4;
        limits.max_vpss_groups = 32;
        limits.max_venc_channels = 16;
        limits.max_h264_channels = 12;
        limits.max_h265_channels = 8;
        
        if (!hisilicon::device::resource_manager::init(limits)) {
            DEV_WRITE_LOG_ERROR("Failed to initialize resource_manager");
            return false;
        }
        s_resource_manager_initialized = true;
        DEV_WRITE_LOG_INFO("Resource manager initialized");
    }
    
    // Initialize camera manager
    if (!s_camera_manager_initialized) {
        if (!hisilicon::device::camera_manager::init(4)) {
            DEV_WRITE_LOG_ERROR("Failed to initialize camera_manager");
            return false;
        }
        s_camera_manager_initialized = true;
        DEV_WRITE_LOG_INFO("Camera manager initialized (max 4 cameras)");
    }
    
    // Also initialize legacy system for compatibility
    bool legacy_result = chn::init(mode);
    
    return legacy_result;
}

void chn_wrapper::release()
{
    DEV_WRITE_LOG_INFO("Releasing chn_wrapper resources");
    
    // Release camera manager
    if (s_camera_manager_initialized) {
        hisilicon::device::camera_manager::release();
        s_camera_manager_initialized = false;
        DEV_WRITE_LOG_INFO("Camera manager released");
    }
    
    // Release resource manager
    if (s_resource_manager_initialized) {
        hisilicon::device::resource_manager::release();
        s_resource_manager_initialized = false;
        DEV_WRITE_LOG_INFO("Resource manager released");
    }
    
    // Also release legacy system
    chn::release();
}

void chn_wrapper::start_capture(bool enable)
{
    // Delegate to legacy implementation
    chn::start_capture(enable);
}

void chn_wrapper::on_stream_come(ceanic::util::stream_obj_ptr sobj,
                                ceanic::util::stream_head* head,
                                const char* buf, int32_t len)
{
    if (m_use_legacy && m_legacy_chn) {
        m_legacy_chn->on_stream_come(sobj, head, buf, len);
        return;
    }
    
    // TODO: Implement observer pattern integration with new architecture
}

void chn_wrapper::on_stream_error(ceanic::util::stream_obj_ptr sobj, int32_t errno)
{
    if (m_use_legacy && m_legacy_chn) {
        m_legacy_chn->on_stream_error(sobj, errno);
        return;
    }
    
    // TODO: Implement error handling with new architecture
}

bool chn_wrapper::scene_init(const char* dir_path)
{
    return chn::scene_init(dir_path);
}

bool chn_wrapper::scene_set_mode(int mode)
{
    return chn::scene_set_mode(mode);
}

void chn_wrapper::scene_release()
{
    chn::scene_release();
}

bool chn_wrapper::rate_auto_init(const char* file)
{
    return chn::rate_auto_init(file);
}

void chn_wrapper::rate_auto_release()
{
    chn::rate_auto_release();
}

bool chn_wrapper::aiisp_start(const char* model_file, int mode)
{
    if (m_use_legacy && m_legacy_chn) {
        return m_legacy_chn->aiisp_start(model_file, mode);
    }
    
    if (m_camera_instance) {
        // Enable AIISP feature on camera instance
        std::string model_path = model_file;
        return m_camera_instance->enable_feature("aiisp", model_path);
    }
    
    return false;
}

void chn_wrapper::aiisp_stop()
{
    if (m_use_legacy && m_legacy_chn) {
        m_legacy_chn->aiisp_stop();
        return;
    }
    
    if (m_camera_instance) {
        m_camera_instance->disable_feature("aiisp");
    }
}

bool chn_wrapper::yolov5_start(const char* model_file)
{
    if (m_use_legacy && m_legacy_chn) {
        return m_legacy_chn->yolov5_start(model_file);
    }
    
    if (m_camera_instance) {
        std::string model_path = model_file;
        return m_camera_instance->enable_feature("yolov5", model_path);
    }
    
    return false;
}

void chn_wrapper::yolov5_stop()
{
    if (m_use_legacy && m_legacy_chn) {
        m_legacy_chn->yolov5_stop();
        return;
    }
    
    if (m_camera_instance) {
        m_camera_instance->disable_feature("yolov5");
    }
}

bool chn_wrapper::vo_start(const char* intf_type, const char* intf_sync)
{
    if (m_use_legacy && m_legacy_chn) {
        return m_legacy_chn->vo_start(intf_type, intf_sync);
    }
    
    if (m_camera_instance) {
        // Enable VO feature
        return m_camera_instance->enable_feature("vo", "");
    }
    
    return false;
}

void chn_wrapper::vo_stop()
{
    if (m_use_legacy && m_legacy_chn) {
        m_legacy_chn->vo_stop();
        return;
    }
    
    if (m_camera_instance) {
        m_camera_instance->disable_feature("vo");
    }
}

bool chn_wrapper::get_stream_head(int chn, int stream, ceanic::util::media_head* mh)
{
    return chn::get_stream_head(chn, stream, mh);
}

bool chn_wrapper::request_i_frame(int chn, int stream)
{
    return chn::request_i_frame(chn, stream);
}

// Helper methods

hisilicon::device::encoder_type chn_wrapper::parse_encoder_type(const std::string& venc_mode)
{
    if (venc_mode == "H264_CBR") {
        return hisilicon::device::encoder_type::H264_CBR;
    } else if (venc_mode == "H264_VBR") {
        return hisilicon::device::encoder_type::H264_VBR;
    } else if (venc_mode == "H264_AVBR") {
        return hisilicon::device::encoder_type::H264_AVBR;
    } else if (venc_mode == "H265_CBR") {
        return hisilicon::device::encoder_type::H265_CBR;
    } else if (venc_mode == "H265_VBR") {
        return hisilicon::device::encoder_type::H265_VBR;
    } else if (venc_mode == "H265_AVBR") {
        return hisilicon::device::encoder_type::H265_AVBR;
    }
    
    DEV_WRITE_LOG_WARN("Unknown encoder type: %s, defaulting to H264_CBR", venc_mode.c_str());
    return hisilicon::device::encoder_type::H264_CBR;
}

std::string chn_wrapper::parse_sensor_name(const std::string& vi_name)
{
    // Remove _WDR suffix if present
    if (vi_name.find("_WDR") != std::string::npos) {
        return vi_name.substr(0, vi_name.find("_WDR"));
    }
    return vi_name;
}

std::string chn_wrapper::parse_sensor_mode(const std::string& vi_name)
{
    // Check if WDR mode
    if (vi_name.find("_WDR") != std::string::npos) {
        return "2to1wdr";
    }
    return "liner";
}

void chn_wrapper::create_camera_config(int venc_w, int venc_h, int fr, int bitrate,
                                       hisilicon::device::camera_config& config)
{
    // Set camera ID (auto-assign)
    config.camera_id = m_chn;
    config.enabled = true;
    
    // Parse sensor configuration
    config.sensor.name = parse_sensor_name(m_vi_name);
    config.sensor.mode = parse_sensor_mode(m_vi_name);
    
    // Create main stream configuration
    hisilicon::device::stream_config main_stream;
    main_stream.stream_id = MAIN_STREAM_ID;
    main_stream.name = "main";
    main_stream.type = parse_encoder_type(m_venc_mode);
    main_stream.width = venc_w;
    main_stream.height = venc_h;
    main_stream.framerate = fr;
    main_stream.bitrate = bitrate;
    main_stream.outputs.rtsp_enabled = true;
    main_stream.outputs.rtsp_url_path = "/stream1";
    
    config.streams.push_back(main_stream);
    
    // Create sub stream configuration (fixed resolution as per legacy code)
    hisilicon::device::stream_config sub_stream;
    sub_stream.stream_id = SUB_STREAM_ID;
    sub_stream.name = "sub";
    sub_stream.type = parse_encoder_type(m_venc_mode);
    sub_stream.width = 704;
    sub_stream.height = 576;
    sub_stream.framerate = fr;
    sub_stream.bitrate = 1000;
    sub_stream.outputs.rtsp_enabled = true;
    sub_stream.outputs.rtsp_url_path = "/stream2";
    
    config.streams.push_back(sub_stream);
    
    DEV_WRITE_LOG_INFO("Created camera config: sensor=%s, mode=%s, streams=%zu",
                       config.sensor.name.c_str(), config.sensor.mode.c_str(), 
                       config.streams.size());
}

} // namespace dev
} // namespace hisilicon
