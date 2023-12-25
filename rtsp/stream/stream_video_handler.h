#ifndef stream_video_handler_include_h
#define stream_video_handler_include_h
#include <stream_handler.h>
#include <rtp_session.h>
#include <rtp_serialize.h>

namespace beacon{namespace rtsp{

    class stream_video_handler
        : public stream_handler
    {
        public:
            stream_video_handler(rtp_session_ptr session_ptr, rtp_serialize_ptr serial_ptr);

            virtual ~stream_video_handler();

            bool start();

            void stop();

            int& get_rtcp_timeout();

        protected:
            virtual bool process_stream(util::stream_head* head, const char* data, int len);

        protected:
            rtp_session_ptr m_rtp_session;
            rtp_serialize_ptr m_rtp_serialize;
    };

}}//namespace

#endif

