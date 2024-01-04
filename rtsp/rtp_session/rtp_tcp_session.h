#ifndef rtp_tcp_session_include_h
#define rtp_tcp_session_include_h
#include "rtp_session.h"
#include <session.h>

namespace ceanic{namespace rtsp{

    class rtp_tcp_session
        : public rtp_session
    {
        public:
            rtp_tcp_session(session& sess, int rtp_id, int rtcp_id);

            virtual ~rtp_tcp_session();

            bool send_packet(rtp_packet* packet);

        protected:
            bool m_err;
            int m_rtp_id;
            int m_rtcp_id;

            session& m_sess;
    };

}}//namespace

#endif

