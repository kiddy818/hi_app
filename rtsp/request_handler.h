#ifndef request_handle_include_h
#define request_handle_include_h

#include <string>
#include <stream_handler.h>

namespace ceanic{namespace rtsp{

    struct request;
    class session;


    class request_handler
    {
        public:
            explicit request_handler(){}

            virtual ~request_handler(){}

            virtual void handle_request(const request& req, session& sess) = 0;

            virtual bool get_video_handle(stream_handler_ptr& handle) = 0;
    };

    typedef std::shared_ptr<request_handler> handler_ptr;

}}//namespace

#endif 

