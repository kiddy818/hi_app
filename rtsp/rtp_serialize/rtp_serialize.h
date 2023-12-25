#ifndef rtp_serialize_include_h
#define rtp_serialize_include_h

#include <util/stream_type.h>
#include <vector>
#include <rtp_type.h>
#include <util/std.h>
#include <thread>
#include <rtp_session.h>

namespace beacon{namespace rtsp{

    class rtp_serialize
    {
        public:
            explicit rtp_serialize(int payload);

            virtual ~rtp_serialize();

            virtual bool serialize(util::stream_head& head, const char* buf, int len, rtp_session_ptr rs) = 0;

        protected:
            unsigned int get_random32();
            unsigned short get_ramdom16();

        protected:
            int m_payload;
            unsigned short m_seq;
            unsigned int m_ssrc;
    };

    typedef std::shared_ptr<rtp_serialize> rtp_serialize_ptr;

}}//namespace

#endif

