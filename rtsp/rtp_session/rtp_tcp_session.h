#ifndef rtp_tcp_session_include_h
#define rtp_tcp_session_include_h
#include "rtp_session.h"
#include <session.h>

namespace ceanic{namespace rtsp{

    class rtp_tcp_session
        : public rtp_session
    {
        public:
            rtp_tcp_session(session& sess, int32_t rtp_id, int32_t rtcp_id);

            virtual ~rtp_tcp_session();

            bool send_packet(rtp_packet_t* packet);

        protected:
            bool m_err;
            int32_t m_rtp_id;
            int32_t m_rtcp_id;

            session& m_sess;
    };

}}//namespace

#endif

