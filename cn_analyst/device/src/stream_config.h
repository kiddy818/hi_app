#ifndef STREAM_CONFIG_H
#define STREAM_CONFIG_H

#include <cstdint>
#include <string>
#include <vector>

namespace hisilicon {
namespace device {

/**
 * @brief Video encoder type
 */
enum class encoder_type {
    H264_CBR,   // H.264 Constant Bit Rate
    H264_VBR,   // H.264 Variable Bit Rate
    H264_AVBR,  // H.264 Adaptive Variable Bit Rate
    H265_CBR,   // H.265 Constant Bit Rate
    H265_VBR,   // H.265 Variable Bit Rate
    H265_AVBR   // H.265 Adaptive Variable Bit Rate
};

/**
 * @brief Output configuration for stream
 */
struct stream_output_config {
    bool rtsp_enabled;
    std::string rtsp_url_path;
    
    bool rtmp_enabled;
    std::vector<std::string> rtmp_urls;
    
    bool mp4_enabled;
    std::string mp4_path;
    
    bool jpeg_enabled;
    int32_t jpeg_interval_sec;
    
    stream_output_config()
        : rtsp_enabled(false)
        , rtmp_enabled(false)
        , mp4_enabled(false)
        , jpeg_enabled(false)
        , jpeg_interval_sec(0)
    {}
};

/**
 * @brief Stream configuration
 */
struct stream_config {
    int32_t stream_id;
    std::string name;  // e.g., "main", "sub", "ai"
    
    encoder_type type;
    int32_t width;
    int32_t height;
    int32_t framerate;
    int32_t bitrate;  // in Kbps
    
    stream_output_config outputs;
    
    stream_config()
        : stream_id(-1)
        , type(encoder_type::H264_CBR)
        , width(1920)
        , height(1080)
        , framerate(30)
        , bitrate(4096)
    {}
};

/**
 * @brief Stream Configuration Validator and Parser
 */
class stream_config_helper {
public:
    /**
     * @brief Validate stream configuration
     * @param config Stream configuration to validate
     * @param error_msg Output parameter for error message if validation fails
     * @return true if valid, false otherwise
     */
    static bool validate(const stream_config& config, std::string& error_msg);
    
    /**
     * @brief Check if resolution is supported
     * @param width Frame width
     * @param height Frame height
     * @return true if resolution is supported
     */
    static bool is_resolution_supported(int32_t width, int32_t height);
    
    /**
     * @brief Check if framerate is supported
     * @param framerate Frames per second
     * @return true if framerate is supported
     */
    static bool is_framerate_supported(int32_t framerate);
    
    /**
     * @brief Check if bitrate is within valid range
     * @param bitrate Bitrate in Kbps
     * @param width Frame width
     * @param height Frame height
     * @return true if bitrate is reasonable
     */
    static bool is_bitrate_valid(int32_t bitrate, int32_t width, int32_t height);
    
    /**
     * @brief Get encoder type from string
     * @param type_str String representation of encoder type
     * @param type Output parameter for encoder type
     * @return true if conversion successful
     */
    static bool parse_encoder_type(const std::string& type_str, encoder_type& type);
    
    /**
     * @brief Convert encoder type to string
     * @param type Encoder type
     * @return String representation
     */
    static std::string encoder_type_to_string(encoder_type type);
    
    /**
     * @brief Check if encoder type is H.264
     * @param type Encoder type
     * @return true if H.264 variant
     */
    static bool is_h264(encoder_type type);
    
    /**
     * @brief Check if encoder type is H.265
     * @param type Encoder type
     * @return true if H.265 variant
     */
    static bool is_h265(encoder_type type);
    
    /**
     * @brief Get recommended bitrate for resolution
     * @param width Frame width
     * @param height Frame height
     * @param framerate Frames per second
     * @return Recommended bitrate in Kbps
     */
    static int32_t get_recommended_bitrate(int32_t width, int32_t height, int32_t framerate);
    
private:
    // Prevent instantiation
    stream_config_helper() = delete;
};

} // namespace device
} // namespace hisilicon

#endif // STREAM_CONFIG_H
