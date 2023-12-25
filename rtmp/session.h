#ifndef beacon_rtmp_session_include_h
#define beacon_rtmp_session_include_h

#include <string>
#include <thread>
#include <mutex>
#include <util/stream_buf.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>

namespace beacon{namespace rtmp{

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

        private:
            void on_process_nalu();
            bool send_packet(const uint8_t* data,uint32_t len,uint32_t timestamp);
            bool send_sps_pps(uint32_t timestamp);
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

    class timed_session
    {
        public:
            timed_session(std::string url,uint32_t max_ms);
            timed_session(const timed_session& rs) = delete;
            timed_session& operator= (const timed_session& rs) = delete;
            ~timed_session();

            bool start();
            void stop();
            bool is_start();
            bool reset_max_tm(uint32_t max_ms);

            enum
            {
                TIMED_SESSION_SUCCESS = 0,
                TIMED_SESSION_OUT_OF_TIME = 1,
                TIMED_SESSION_OHTERS_ERROR = 2,
            };

            int input_one_nalu(const uint8_t* data,uint32_t len,uint32_t timestamp);

        private:
            session m_rs;
            uint32_t m_max_ms;
            uint32_t m_init_timestamp;
    };

}}//namespace

#endif
