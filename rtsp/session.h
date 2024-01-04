#ifndef session_include_h
#define session_include_h

#include <iostream>
#include <util/std.h>
#include <event.h>
#include <boost/logic/tribool.hpp>
#include <mutex>
#include <thread>

namespace ceanic{namespace rtsp{

#define MAX_SESSION_TIMEOUT (60)
    class session
    {
        public:
            session(int s, int timeout);

            virtual ~session();

            virtual void reduce_session_timeout() = 0;
            virtual int get_session_timeout() = 0;

            virtual void reduce_rtcp_timeout() = 0;
            virtual int get_rtcp_timeout() = 0;

            int socket();

            std::string& ip();

            virtual boost::tribool handle_read(const char* data, int len) = 0;

            virtual void handle_reset(); 

            virtual bool start() = 0;

            virtual void stop() = 0;

            virtual bool is_start()
            {
                return m_start;
            }

            void on_idle();
            bool send_packet_n(const char* buf, int buf_len);

        protected:
            int m_socket;
            bool m_start;
            std::string m_ip;
            int m_timeout;

            std::mutex m_out_buf_mu;
            struct  evbuffer* m_out_buf;
    };

    typedef std::shared_ptr<session> session_ptr;

}}//namespace

#endif
