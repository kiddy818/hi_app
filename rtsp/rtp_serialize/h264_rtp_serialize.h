#ifndef h264_rtp_serialize_include_h
#define h264_rtp_serialize_include_h

#include <rtp_serialize.h>
#include <util/std.h>

namespace ceanic{namespace rtsp{

      class h264_rtp_serialize
        :public rtp_serialize
    {
        public:
            explicit h264_rtp_serialize(int payload);

            virtual ~h264_rtp_serialize();

            bool serialize(util::stream_head& head,const char* buf,int len,rtp_session_ptr rs);

        private:
            void process_nalu(util::nalu_t*nalu,rtp_session_ptr rs);
    };

}}//namespace

#endif

