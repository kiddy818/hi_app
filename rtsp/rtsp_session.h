#ifndef rtsp_session_include_h
#define rtsp_session_include_h

#include "session.h"
#include "rtsp_request_handler.h"
#include "request.h"
#include "request_parser.h"

namespace ceanic{namespace rtsp{

    class rtsp_session
        :public session
    {
        public:
            rtsp_session(int32_t s, int32_t timeout);

            virtual ~rtsp_session();

            virtual std::optional<bool> handle_read(const char* data, int32_t len);

            void process_rtsp_request();

            virtual void handle_reset() ;

            virtual bool start();

            virtual void stop();

            virtual void reduce_session_timeout();
            virtual int32_t get_session_timeout();
            virtual void reduce_rtcp_timeout();
            virtual int32_t get_rtcp_timeout();

        protected:
            rtsp_request_handler m_handler;
            request_parser m_parser;
            request m_request;
    };

}}//namespace

#endif

