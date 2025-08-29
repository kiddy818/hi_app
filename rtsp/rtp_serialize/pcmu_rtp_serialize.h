#ifndef pcmu_rtp_serialize_include_h
#define pcmu_rtp_serialize_include_h

#include <rtp_serialize.h>
#include <util/std.h>

namespace ceanic{namespace rtsp{

      class pcmu_rtp_serialize
        :public rtp_serialize
    {
        public:
            explicit pcmu_rtp_serialize();

            virtual ~pcmu_rtp_serialize();

            bool serialize(util::stream_head& head,const char* buf,int32_t len,rtp_session_ptr rs);
    };

}}//namespace

#endif

