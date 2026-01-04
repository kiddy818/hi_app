#ifndef session_include_h
#define session_include_h

#include <iostream>
#include <util/std.h>
#include <event.h>
#include <mutex>
#include <thread>
#include <optional>
#include <list>
#include <rtp_type.h>

namespace ceanic{namespace rtsp{

#define MAX_SESSION_TIMEOUT (60)
    class session
    {
        public:
            session(int32_t s, int32_t timeout);

            virtual ~session();

            virtual void reduce_session_timeout() = 0;
            virtual int32_t get_session_timeout() = 0;

            virtual void reduce_rtcp_timeout() = 0;
            virtual int32_t get_rtcp_timeout() = 0;

            int32_t socket();

            std::string& ip();

            virtual std::optional<bool> handle_read(const char* data, int32_t len) = 0;

            virtual void handle_reset(); 

            virtual bool start() = 0;

            virtual void stop() = 0;

            virtual bool is_start()
            {
                return m_start;
            }

            void on_idle();
            bool send_packet_n(const char* buf, int32_t buf_len);
            bool send_rtp_packet(rtp_packet_t* packet);

        protected:
            int32_t m_socket;
            bool m_start;
            std::string m_ip;
            int32_t m_timeout;

            std::mutex m_out_buf_mu;
            struct  evbuffer* m_out_buf;
    };

    typedef std::shared_ptr<session> session_ptr;

}}//namespace

#endif
