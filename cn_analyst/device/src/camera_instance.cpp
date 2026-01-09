#include "camera_instance.h"
#include <algorithm>

#include "dev_log.h"
#include "dev_venc.h"
#include "dev_osd.h"
#include "rtmp/session_manager.h"
#include "stream_manager.h"

namespace hisilicon {
namespace device {

// Stream instance placeholder (minimal implementation for now)
class stream_instance :public std::enable_shared_from_this<stream_instance> {
public:
    stream_instance(int32_t stream_id, const stream_config& config)
        : m_stream_id(stream_id), m_config(config), m_is_running(false)
        {
            // 构造函数中使用 std::make_shared，否则会有exception: bad_weak_ptr
        }
    
    int32_t stream_id() const { return m_stream_id; }
    stream_config get_config() const { return m_config; }
    bool is_running() const { return m_is_running; }
    
    bool start(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn) {
        switch(m_config.type)
        {
            // 第一个参数 int32_t chn 目前并没有使用到，用0
            // 第七个参数 ot_vpss_grp vpss_grp 构造时没有使用到，用-1，在start时需要
            // 第八个参数 ot_vpss_chn vpss_chn 构造时没有使用到，用-1，在start时需要
            case encoder_type::H264_CBR:
                m_venc_ptr = std::make_shared<venc_h264_cbr>(0, m_config.stream_id,
                    m_config.width, m_config.height, m_config.framerate, m_config.framerate,
                    -1, -1, m_config.bitrate);
                break;

            case encoder_type::H264_AVBR:
                m_venc_ptr = std::make_shared<venc_h264_avbr>(0, m_config.stream_id,
                    m_config.width, m_config.height, m_config.framerate, m_config.framerate,
                    -1, -1, m_config.bitrate);
                break;

            case encoder_type::H265_CBR:
                m_venc_ptr = std::make_shared<venc_h265_cbr>(0, m_config.stream_id,
                    m_config.width, m_config.height, m_config.framerate, m_config.framerate,
                    -1, -1, m_config.bitrate);
                break;

            case encoder_type::H265_AVBR:
                m_venc_ptr = std::make_shared<venc_h265_avbr>(0, m_config.stream_id,
                    m_config.width, m_config.height, m_config.framerate, m_config.framerate,
                    -1, -1, m_config.bitrate);
                break;

            default:
                DEV_WRITE_LOG_ERROR("invalid venc mode");
                m_venc_ptr = nullptr;
                break;
        }

        if (m_venc_ptr == nullptr)
        {
            return false;
        }

        m_is_running = true;
        m_venc_ptr->start(vpss_grp, vpss_chn);

        // 使用了默认的osd配置，后期可根据主视窗大小适配
        m_osd_ptr = std::make_shared<osd_date>(32,32,64, m_venc_ptr->venc_chn());
        m_osd_ptr->start();

        return true;
    }
    
    void stop() {
        m_osd_ptr->stop();
        m_venc_ptr->stop();

        m_is_running = false;
    }

    bool register_stream_observer(ceanic::util::stream_observer_ptr observer) {
        if (!m_venc_ptr || !observer) {
            return false;
        }

        m_venc_ptr->register_stream_observer(observer);
        return true;
    }

    bool unregister_stream_observer(ceanic::util::stream_observer_ptr observer) {
        if (!m_venc_ptr || !observer) {
            return false;
        }

        m_venc_ptr->unregister_stream_observer(observer);
        return true;
    }

    bool request_i_frame()
    {
        if (!m_venc_ptr) {
            return false;
        }

        m_venc_ptr->request_i_frame();
        return true;
    }

    bool get_stream_head(ceanic::util::media_head* mh)
    {
        if (!m_venc_ptr) {
            return false;
        }

        using namespace ceanic::util;
        memset(mh,0,sizeof(media_head));
#if 0
        if(stream == AI_STREAM_ID)
        {
            //aidetect stream
            mh->audio_info.acode = STREAM_AUDIO_ENCODE_NONE;
            mh->video_info.vcode = STREAM_VIDEO_ENCODE_H264;
            return true;
        }
#endif

        mh->video_info.w = m_venc_ptr->venc_w();
        mh->video_info.h = m_venc_ptr->venc_h();
        mh->video_info.fr = m_venc_ptr->venc_fr();
        switch(m_config.type) {
            case encoder_type::H264_CBR:
            case encoder_type::H264_VBR:
            case encoder_type::H264_AVBR:
                mh->video_info.vcode = STREAM_VIDEO_ENCODE_H264;
                break;
            case encoder_type::H265_CBR:
            case encoder_type::H265_VBR:
            case encoder_type::H265_AVBR:
                mh->video_info.vcode = STREAM_VIDEO_ENCODE_H265;
                break;
        }

        return true;
    }
    
private:
    int32_t m_stream_id;
    stream_config m_config;
    bool m_is_running;

    std::shared_ptr<venc> m_venc_ptr;
    std::shared_ptr<osd_date> m_osd_ptr;
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
        DEV_WRITE_LOG_ERROR("Failed to start camera instance @1 [Already running]");
        return false; // Already running
    }
    
    // Step 1: Allocate hardware resources
    if (!allocate_resources()) {
        DEV_WRITE_LOG_ERROR("Failed to start camera instance @2 [Allocate hardware resources failed]");
        free_resources();
        return false;
    }
    
    // Step 2: Initialize VI (Video Input)
    if (!init_vi()) {
        DEV_WRITE_LOG_ERROR("Failed to start camera instance @3 [Initialize VI]");
        free_resources();
        return false;
    }
    
    // Step 3: Initialize VPSS (Video Processing)
    if (!init_vpss()) {
        DEV_WRITE_LOG_ERROR("Failed to start camera instance @4 [Initialize VPSS]");
        free_resources();
        return false;
    }
    
    // Step 4: Initialize streams and encoders
    if (!init_streams()) {
        DEV_WRITE_LOG_ERROR("Failed to start camera instance @5 [Initialize streams and encoders]");

        m_vi_ptr->stop();
        free_resources();
        return false;
    }
    
    // Step 5: Initialize features (OSD, AIISP, etc.)
    if (!init_features()) {
        // Features are optional, so we don't fail if they can't be initialized
        // Just log a warning (would be done in init_features)

        DEV_WRITE_LOG_ERROR("Failed to start camera instance @6 [Initialize streams and encoders]");

        // Stop all streams
        m_vi_ptr->stop();
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
    stop_streams();
    
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

// init_streams() 实现完整的逻辑，这里是单个配置，暂时未调用
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
    
    if (!stream->start(m_vi_ptr->vpss_grp(), m_vi_ptr->vpss_chn())) {
        resource_manager::free_venc_channel(venc_chn);
        return false;
    }
    stream->register_stream_observer(shared_from_this());
    
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
        it->second->unregister_stream_observer(shared_from_this());
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

bool camera_instance::get_isp_exposure_info(isp_exposure_t* val)
{
    if(!m_vi_ptr)
    {
        return false;
    }

    std::shared_ptr<vi_isp> viisp = std::dynamic_pointer_cast<vi_isp>(m_vi_ptr);
    if(!viisp)
    {
        return false;
    }

    return viisp->get_isp_exposure_info(val);
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
    const auto& sensor_type = m_config.sensor.name;
    const auto& work_mode = m_config.sensor.mode;

    // sensor_type 的大小写转换为大写，以便与定义的枚举值匹配
    // 临时用大写匹配名字，小写匹配类型
    // std::transform(sensor_type.begin(), sensor_type.end(), sensor_type.begin(), ::tolower);
    // std::transform(work_mode.begin(), work_mode.end(), work_mode.begin(), ::tolower);
    if (sensor_type == "os04a10" || sensor_type == "OS04A10")
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
    else if (sensor_type == "os08a20" || sensor_type == "OS08A20")
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
        DEV_WRITE_LOG_ERROR("Vi start failed, unsupported sensor type: %s, work mode: %s",
            sensor_type.c_str(), work_mode.c_str());
        return false;
    }

    if(!m_vi_ptr->start())
    {
        DEV_WRITE_LOG_ERROR("Vi start failed");
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
            stop_streams();
            return false;
        }
    }
    return true;
}

void camera_instance::stop_streams() {
    // Cleanup already created streams
    for (auto& pair : m_streams) {
        if (pair.second) {
            pair.second->unregister_stream_observer(shared_from_this());
            pair.second->stop();
        }
    }
    m_streams.clear();
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

void camera_instance::on_stream_come(ceanic::util::stream_obj_ptr obj, ceanic::util::stream_head *head, const char *buf, int32_t len)
{
    int32_t chn = obj->chn();
    int32_t stream = obj->stream_id();
    std::string sname = obj->name();

#if 0
    //rtmp当前支持主码流/子码流 H264格式
    if(stream == 0 /* MAIN_STREAM_ID */ || stream == 1 /* SUB_STREAM_ID */)
    {
        if(strstr(m_venc_mode.c_str(),"H264") != NULL)
        {
            ceanic::rtmp::session_manager::instance()->process_data(chn,stream,head,(uint8_t*)buf,len);
        }
    }

    //mp4当前保存的是主码流
    if(stream == MAIN_STREAM_ID )
    {
        if(m_save)
        {
            m_save->input_data(head,buf,len);
        }
    }
#endif

    ceanic::rtsp::stream_manager::instance()->process_data(chn,stream,head,buf,len);
}

void camera_instance::on_stream_error(ceanic::util::stream_obj_ptr obj, int32_t error)
{
}

bool camera_instance::request_i_frame(int stream)
{
    auto it = m_streams.find(stream);
    if (it == m_streams.end()) {
        return false; // Stream not found
    }

    // Stop and remove stream
    if (it->second) {
        it->second->request_i_frame();
        return true;
    }

    return false;
}

bool camera_instance::get_stream_head(int stream, ceanic::util::media_head *mh)
{
    auto it = m_streams.find(stream);
    if (it == m_streams.end()) {
        return false; // Stream not found
    }

    // Stop and remove stream
    if (it->second) {
        it->second->get_stream_head(mh);
        return true;
    }

    return false;
}

} // namespace device
} // namespace hisilicon
