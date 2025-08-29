#ifndef rtp_session_include_h
#define rtp_session_include_h
#include "rtp_type.h"
#include <util/std.h>
#include <thread>

namespace ceanic{namespace rtsp{

#define MAX_RTCP_TIMEOUT (60)
    class rtp_session
    {
        public:
            rtp_session();

            virtual ~rtp_session();

            virtual bool send_packet(rtp_packet_t* packet) = 0;

            virtual int32_t & rtcp_timeout()
            {
                return m_rtcp_timeout;
            }

        protected:
            int32_t m_rtcp_timeout;
    };

    typedef std::shared_ptr<rtp_session> rtp_session_ptr;

}}//namespace

#endif

