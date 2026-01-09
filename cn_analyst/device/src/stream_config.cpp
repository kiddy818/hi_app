#include "stream_config.h"
#include <algorithm>
#include <sstream>

namespace hisilicon {
namespace device {

// Supported resolutions (common video resolutions)
static const std::vector<std::pair<int32_t, int32_t>> SUPPORTED_RESOLUTIONS = {
    {3840, 2160}, // 4K UHD
    {2560, 1440}, // 2K QHD
    {1920, 1080}, // 1080p FHD
    {1280, 720},  // 720p HD
    {960, 540},   // qHD
    {704, 576},   // D1 PAL
    {704, 480},   // D1 NTSC
    {640, 480},   // VGA
    {640, 360},   // nHD
    {352, 288},   // CIF
    {320, 240}    // QVGA
};

// Supported framerates
static const std::vector<int32_t> SUPPORTED_FRAMERATES = {
    60, 50, 30, 25, 20, 15, 10, 5
};

bool stream_config_helper::validate(const stream_config& config, std::string& error_msg) {
    std::ostringstream oss;
    
    // Check stream ID
    if (config.stream_id < 0) {
        oss << "Invalid stream_id: " << config.stream_id << " (must be >= 0)";
        error_msg = oss.str();
        return false;
    }
    
    // Check name
    if (config.name.empty()) {
        error_msg = "Stream name cannot be empty";
        return false;
    }
    
    // Check resolution
    if (!is_resolution_supported(config.width, config.height)) {
        oss << "Unsupported resolution: " << config.width << "x" << config.height;
        error_msg = oss.str();
        return false;
    }
    
    // Check framerate
    if (!is_framerate_supported(config.framerate)) {
        oss << "Unsupported framerate: " << config.framerate << " fps";
        error_msg = oss.str();
        return false;
    }
    
    // Check bitrate
    if (!is_bitrate_valid(config.bitrate, config.width, config.height)) {
        oss << "Invalid bitrate: " << config.bitrate << " Kbps for resolution " 
            << config.width << "x" << config.height;
        error_msg = oss.str();
        return false;
    }
    
    // Check output configuration
    if (!config.outputs.rtsp_enabled && 
        !config.outputs.rtmp_enabled && 
        !config.outputs.mp4_enabled && 
        !config.outputs.jpeg_enabled) {
        error_msg = "At least one output must be enabled (RTSP, RTMP, MP4, or JPEG)";
        return false;
    }
    
    // Check RTSP configuration
    if (config.outputs.rtsp_enabled && config.outputs.rtsp_url_path.empty()) {
        error_msg = "RTSP enabled but URL path is empty";
        return false;
    }
    
    // Check RTMP configuration
    if (config.outputs.rtmp_enabled) {
        if (config.outputs.rtmp_urls.empty()) {
            error_msg = "RTMP enabled but no URLs configured";
            return false;
        }
        
        // RTMP only supports H.264 currently
        if (is_h265(config.type)) {
            error_msg = "RTMP currently only supports H.264 encoding (not H.265)";
            return false;
        }
    }
    
    // Check MP4 configuration
    if (config.outputs.mp4_enabled && config.outputs.mp4_path.empty()) {
        error_msg = "MP4 enabled but path is empty";
        return false;
    }
    
    // Check JPEG configuration
    if (config.outputs.jpeg_enabled && config.outputs.jpeg_interval_sec <= 0) {
        oss << "JPEG enabled but invalid interval: " << config.outputs.jpeg_interval_sec 
            << " (must be > 0)";
        error_msg = oss.str();
        return false;
    }
    
    return true;
}

bool stream_config_helper::is_resolution_supported(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    
    // Check against supported resolutions list
    for (const auto& res : SUPPORTED_RESOLUTIONS) {
        if (res.first == width && res.second == height) {
            return true;
        }
    }
    
    // Also allow custom resolutions that are within reasonable bounds
    // Max: 4K (3840x2160), Min: QCIF (176x144)
    if (width >= 176 && width <= 3840 && height >= 144 && height <= 2160) {
        // Check if width and height are multiples of 2 (required by most encoders)
        if (width % 2 == 0 && height % 2 == 0) {
            return true;
        }
    }
    
    return false;
}

bool stream_config_helper::is_framerate_supported(int32_t framerate) {
    if (framerate <= 0 || framerate > 60) {
        return false;
    }
    
    // Check against supported framerates list or any value 1-60
    return (framerate >= 1 && framerate <= 60);
}

bool stream_config_helper::is_bitrate_valid(int32_t bitrate, int32_t width, int32_t height) {
    if (bitrate <= 0) {
        return false;
    }
    
    // Calculate minimum and maximum reasonable bitrates
    // Formula: pixels * bits_per_pixel / 1024 (for Kbps)
    // Min: 0.01 bpp (very low quality), Max: 0.5 bpp per frame @ 30fps = 15 bpp/sec
    
    int64_t pixels = static_cast<int64_t>(width) * height;
    int32_t min_bitrate = static_cast<int32_t>(pixels * 0.01 / 1024); // in Kbps
    int32_t max_bitrate = static_cast<int32_t>(pixels * 0.25 / 1024);  // in Kbps
    
    // Absolute bounds: 64 Kbps to 100 Mbps
    min_bitrate = std::max(min_bitrate, 64);
    max_bitrate = std::min(max_bitrate, 102400);
    
    // For typical resolutions, allow up to 20 Mbps
    if (width <= 1920 && height <= 1080) {
        max_bitrate = std::max(max_bitrate, 20480); // 20 Mbps
    }

    // 当前支持最大码率80Mbps，支持的最大分辨率：6144×6144
    max_bitrate = std::max(max_bitrate, 80*1024);
    
    return (bitrate >= min_bitrate && bitrate <= max_bitrate);
}

bool stream_config_helper::parse_encoder_type(const std::string& type_str, encoder_type& type) {
    if (type_str == "H264_CBR") {
        type = encoder_type::H264_CBR;
        return true;
    } else if (type_str == "H264_VBR") {
        type = encoder_type::H264_VBR;
        return true;
    } else if (type_str == "H264_AVBR") {
        type = encoder_type::H264_AVBR;
        return true;
    } else if (type_str == "H265_CBR") {
        type = encoder_type::H265_CBR;
        return true;
    } else if (type_str == "H265_VBR") {
        type = encoder_type::H265_VBR;
        return true;
    } else if (type_str == "H265_AVBR") {
        type = encoder_type::H265_AVBR;
        return true;
    }
    
    return false;
}

std::string stream_config_helper::encoder_type_to_string(encoder_type type) {
    switch (type) {
        case encoder_type::H264_CBR:  return "H264_CBR";
        case encoder_type::H264_VBR:  return "H264_VBR";
        case encoder_type::H264_AVBR: return "H264_AVBR";
        case encoder_type::H265_CBR:  return "H265_CBR";
        case encoder_type::H265_VBR:  return "H265_VBR";
        case encoder_type::H265_AVBR: return "H265_AVBR";
        default: return "UNKNOWN";
    }
}

bool stream_config_helper::is_h264(encoder_type type) {
    return (type == encoder_type::H264_CBR || 
            type == encoder_type::H264_VBR || 
            type == encoder_type::H264_AVBR);
}

bool stream_config_helper::is_h265(encoder_type type) {
    return (type == encoder_type::H265_CBR || 
            type == encoder_type::H265_VBR || 
            type == encoder_type::H265_AVBR);
}

int32_t stream_config_helper::get_recommended_bitrate(int32_t width, int32_t height, int32_t framerate) {
    // Calculate bitrate based on resolution and framerate
    // Using formula: pixels_per_second * bits_per_pixel / 1024
    // Target: 0.15 bpp for good quality
    
    int64_t pixels_per_second = static_cast<int64_t>(width) * height * framerate;
    int32_t bitrate = static_cast<int32_t>(pixels_per_second * 0.15 / 1024);
    
    // Clamp to reasonable range
    bitrate = std::max(bitrate, 128);      // Minimum 128 Kbps
    bitrate = std::min(bitrate, 102400);   // Maximum 100 Mbps
    
    return bitrate;
}

} // namespace device
} // namespace hisilicon
