#ifndef server_include_h
#define server_include_h

#include <thread>
#include <list>
#include <session.h>

namespace ceanic{namespace rtsp{

    class rtsp_server
    {
        public:
            rtsp_server(short port = 554);
            virtual ~rtsp_server();

            bool run();
            void stop();

        protected:
            void on_run();

        protected:
            int m_listen_s;
            short m_port;
            bool m_is_run;
            std::thread m_thread;
            std::list<session_ptr> m_session_ptrs;
    };

}}//namespace

#endif

