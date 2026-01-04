#ifndef ceanic_rtmp_session_include_h
#define ceanic_rtmp_session_include_h

#include <string>
#include <thread>
#include <mutex>
#include <util/stream_buf.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>

namespace ceanic{namespace rtmp{

    typedef struct
    {
        uint32_t len;
        uint32_t timestamp;
        int32_t type;
    }_head;

    class session
    {
        public:
            session(std::string url);
            session(const session& rs) = delete;
            session& operator= (const session& rs) = delete;
            ~session();

            bool start();
            void stop();
            bool is_start();
            bool input_one_nalu(const uint8_t* data,uint32_t len,uint32_t timestamp);
            bool input_audio_frame(const uint8_t* data,uint32_t len,uint32_t timestamp);

        private:
            void on_process();
            bool process_video(_head* head,uint8_t* data);
            bool process_audio(_head* head,uint8_t* data);
            bool send_packet(bool is_video,const uint8_t* data,uint32_t len,uint32_t timestamp);
            bool send_sps_pps(uint32_t timestamp);
            bool send_aac_spec(uint32_t timestamp);
            bool send_metedata();

        private:
            static uint8_t* put_byte(uint8_t* out, uint8_t v);
            static uint8_t* put_be16(uint8_t* out, uint16_t v);
            static uint8_t* put_be24(uint8_t* out, uint32_t v);
            static uint8_t* put_be32(uint8_t* out, uint32_t v);
            static uint8_t* put_be64(uint8_t* out, uint64_t v);
            static uint8_t* put_amf_string(uint8_t* out, const char *str);
            static uint8_t* put_amf_double(uint8_t* out, double d);

        private:
            std::string m_url;
            std::thread m_thread;
            bool m_bstart;
            uint8_t m_sps[256];
            uint32_t m_sps_size;
            uint8_t m_pps[256];
            uint32_t m_pps_size;
            util::stream_buf m_sbuf;
            std::mutex m_sbuf_mu;
            RTMP* m_rtmp;
    };

}}//namespace

#endif
