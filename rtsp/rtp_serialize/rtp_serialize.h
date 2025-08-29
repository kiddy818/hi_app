#ifndef rtp_serialize_include_h
#define rtp_serialize_include_h

#include <util/stream_type.h>
#include <vector>
#include <rtp_type.h>
#include <util/std.h>
#include <thread>
#include <rtp_session.h>

namespace ceanic{namespace rtsp{

    class rtp_serialize
    {
        public:
            explicit rtp_serialize(int32_t payload);

            virtual ~rtp_serialize();

            virtual bool serialize(util::stream_head& head, const char* buf, int32_t len, rtp_session_ptr rs) = 0;

        protected:
            uint32_t get_random32();
            uint16_t get_ramdom16();

        protected:
            int32_t m_payload;
            uint16_t m_seq;
            uint32_t m_ssrc;
    };

    typedef std::shared_ptr<rtp_serialize> rtp_serialize_ptr;

}}//namespace

#endif

