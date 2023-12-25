#ifndef rtp_udp_session_include_h
#define rtp_udp_session_include_h
#include "rtp_session.h"
#include <string>

namespace beacon{namespace rtsp{

    class rtp_udp_session
        :public rtp_session
    {
        public:
            rtp_udp_session(const char* remote_ip, short remote_rtp_port, short remote_rtcp_port, const char* local_ip, short local_rtp_port, short local_rtcp_port);

            virtual ~rtp_udp_session();

            bool send_packet(rtp_packet* packet);

        protected:
            std::string m_remote_ip;
            short m_remote_rtp_port;
            short m_remote_rtcp_port;
            short m_local_rtp_port;
            short m_local_rtcp_port;
            int m_rtp_socket;
            int m_rtcp_socket;
            struct sockaddr_in m_dst_addr;
    };

}}//namespace

#endif

